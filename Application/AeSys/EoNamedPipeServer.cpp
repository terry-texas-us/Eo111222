#include "Stdafx.h"

#include <algorithm>

#include "EoNamedPipeServer.h"
#include "EoPipeProtocol.h"

namespace {

/// @brief Read bytes from the pipe until a newline or the buffer is full.
/// Returns the number of bytes stored in @p buf (excluding any trailing \r\n or \0).
/// Returns 0 when the client disconnects or an error occurs.
DWORD ReadLine(HANDLE hPipe, char* buf, DWORD capacity) {
    DWORD total = 0;
    while (total < capacity - 1) {
        char ch = 0;
        DWORD bytesRead = 0;
        if (!ReadFile(hPipe, &ch, 1, &bytesRead, nullptr) || bytesRead == 0) { break; }
        if (ch == '\n') { break; }
        if (ch != '\r') { buf[total++] = ch; }
    }
    buf[total] = '\0';
    return total;
}

}  // namespace

EoNamedPipeServer::~EoNamedPipeServer() {
    Stop();
}

void EoNamedPipeServer::Start(HWND hDispatchWnd) {
    if (m_running.load()) { return; }

    m_hDispatchWnd = hDispatchWnd;
    m_hStopEvent = CreateEventW(nullptr, TRUE /*manual reset*/, FALSE, nullptr);
    m_running.store(true);
    m_thread = std::thread(&EoNamedPipeServer::ServerThread, this);
}

void EoNamedPipeServer::Stop() {
    if (!m_running.load()) { return; }
    m_running.store(false);

    if (m_hStopEvent) { SetEvent(m_hStopEvent); }

    // Wake up any blocking ConnectNamedPipe by opening and immediately closing
    // a client connection to the pipe.
    HANDLE hWake = CreateFileW(EoPipe::kPipeName, GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hWake != INVALID_HANDLE_VALUE) { CloseHandle(hWake); }

    if (m_thread.joinable()) { m_thread.join(); }

    if (m_hStopEvent) {
        CloseHandle(m_hStopEvent);
        m_hStopEvent = nullptr;
    }
}

void EoNamedPipeServer::ServerThread() {
    constexpr DWORD kBufSize = EoPipe::kMaxRequestBytes;

    while (m_running.load()) {
        // Create (or re-create) the pipe instance for the next client.
        HANDLE hPipe = CreateNamedPipeW(EoPipe::kPipeName,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1 /*max instances*/,
            EoPipe::kMaxResponseBytes, kBufSize,
            0 /*default timeout*/,
            nullptr /*default security*/);

        if (hPipe == INVALID_HANDLE_VALUE) {
            // Avoid a tight spin if pipe creation fails (e.g. access denied).
            WaitForSingleObject(m_hStopEvent, 500);
            continue;
        }

        // Block until a client connects or the stop event fires.
        BOOL connected = ConnectNamedPipe(hPipe, nullptr)
            ? TRUE
            : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!connected || !m_running.load()) {
            CloseHandle(hPipe);
            continue;
        }

        // Serve the connected client: read lines, dispatch, write replies.
        char readBuf[kBufSize];
        while (m_running.load()) {
            const DWORD len = ReadLine(hPipe, readBuf, kBufSize);

            if (len == 0) {
                // Distinguish clean disconnect from a genuine empty line.
                if (GetLastError() != ERROR_SUCCESS) { break; }
                // Empty line: must still write a response — the client is
                // blocking on readline(). Silently skipping would deadlock it.
                DWORD written = 0;
                const std::string okReply = EoPipe::ResponseOk();
                WriteFile(hPipe, okReply.data(), static_cast<DWORD>(okReply.size()), &written, nullptr);
                continue;
            }

            // Convert UTF-8 command line to wide string.
            const int wlen = MultiByteToWideChar(CP_UTF8, 0, readBuf,
                static_cast<int>(len), nullptr, 0);
            std::wstring wCmd;
            if (wlen > 0) {
                wCmd.resize(static_cast<std::size_t>(wlen));
                MultiByteToWideChar(CP_UTF8, 0, readBuf,
                    static_cast<int>(len), wCmd.data(), wlen);
            }

            std::string reply;
            if (m_hDispatchWnd && IsWindow(m_hDispatchWnd)) {
                // Marshal the command onto the UI thread synchronously.
                // SendMessage blocks until the handler returns — safe here
                // because the server thread is not the UI thread.
                auto* ctx = new CommandContext{wCmd, {}};
                SendMessageW(m_hDispatchWnd, WM_APP_PIPE_COMMAND, 0,
                    reinterpret_cast<LPARAM>(ctx));
                reply = ctx->responseUtf8.empty()
                    ? EoPipe::ResponseOk()
                    : ctx->responseUtf8;
                delete ctx;
            } else {
                reply = EoPipe::ResponseError("NO_VIEW", "no dispatch window");
            }

            DWORD written = 0;
            WriteFile(hPipe, reply.data(), static_cast<DWORD>(reply.size()), &written, nullptr);
        }

        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
}

