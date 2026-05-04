#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>

#include "EoPipeProtocol.h"

/// @brief Lightweight named-pipe server that exposes the AeSys CLI over a
/// byte-stream pipe so that Python (and other out-of-process clients) can
/// drive the application without a pywin32 DDE dependency.
///
/// ## Protocol  (see EoPipeProtocol.h for the full specification)
/// Each client sends a UTF-8 newline-terminated command line:
///   "TEXT 10,20 \"Hello\"\n"
///
/// The server dispatches it through the same ExecuteCommandLine path used by
/// the interactive command tab and writes back a single UTF-8 response line:
///
///   OK                          -- command dispatched, no result payload
///   OK <value>                  -- command dispatched with a scalar result
///   ERROR:UNKNOWN_COMMAND <n>   -- verb not in the command registry
///   ERROR:PARSE <token>         -- a coordinate or argument token was malformed
///   ERROR:NO_VIEW               -- no active MDI view
///   ERROR:INTERNAL <msg>        -- unexpected internal failure
///   WARN <msg>                  -- dispatched with a non-fatal advisory
///
/// ## Python usage example
///   PIPE = r'\\.\pipe\AeSys'
///   with open(PIPE, 'r+b', buffering=0) as f:
///       f.write(b'LINE 0,0 100,100\n')
///       print(f.readline())   # b'OK\n'
///
/// ## Threading model
/// The server runs one persistent background thread that loops:
///   ConnectNamedPipe → read line → dispatch on main thread → write reply → disconnect
/// Dispatch is marshalled onto the MFC message-pump thread via
/// SendMessageW(WM_APP_PIPE_COMMAND) so all document mutations happen on the
/// UI thread with no additional locking.
///
class EoNamedPipeServer {
 public:
  EoNamedPipeServer() = default;
  EoNamedPipeServer(const EoNamedPipeServer&) = delete;
  EoNamedPipeServer& operator=(const EoNamedPipeServer&) = delete;
  ~EoNamedPipeServer();

  /// @brief Start the server thread.  hDispatchWnd must be a valid HWND that
  /// will receive WM_APP_PIPE_COMMAND messages on the UI thread.
  /// Safe to call multiple times; subsequent calls are no-ops if already running.
  void Start(HWND hDispatchWnd);

  /// @brief Stop the server thread and wait for it to exit.
  /// Signals the stop event and cancels any pending ConnectNamedPipe.
  void Stop();

  [[nodiscard]] bool IsRunning() const noexcept { return m_running.load(); }

  /// @brief Custom window message sent to hDispatchWnd to marshal a command
  /// onto the UI thread.
  ///   WPARAM = 0 (reserved)
  ///   LPARAM = reinterpret_cast<LPARAM>(CommandContext*)  — heap-allocated,
  ///            ownership transfers to the message handler which must delete it.
  static constexpr UINT WM_APP_PIPE_COMMAND = WM_APP + 10;

  /// @brief Payload carried by WM_APP_PIPE_COMMAND.
  /// The UI-thread handler fills @p responseUtf8 with a complete protocol line
  /// (including the trailing '\n') before returning.  The server thread relays
  /// that string verbatim to the pipe client.
  /// If @p responseUtf8 is left empty the server falls back to "OK\n".
  struct CommandContext {
    std::wstring commandLine;
    std::string responseUtf8;  ///< complete response line, e.g. "OK\n" or "ERROR:NO_VIEW\n"
  };

 private:
  void ServerThread();

  HWND m_hDispatchWnd{nullptr};
  std::thread m_thread;
  std::atomic<bool> m_running{false};
  HANDLE m_hStopEvent{nullptr};
};
