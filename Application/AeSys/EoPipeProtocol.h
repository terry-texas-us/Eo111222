#pragma once

/// @file EoPipeProtocol.h
/// @brief Wire-level constants and helpers for the AeSys named-pipe CLI transport.
///
/// ## Response convention (one UTF-8 line per request, terminated by '\n')
///
/// Every request sent to the pipe produces exactly one response line:
///
///   OK                          -- command dispatched, no result payload
///   OK <value>                  -- command dispatched with a scalar / string result
///   ERROR:UNKNOWN_COMMAND <n>   -- verb not in the command registry
///   ERROR:PARSE <token>         -- a coordinate or argument token was malformed
///   ERROR:NO_VIEW               -- no active MDI view to receive the command
///   ERROR:INTERNAL <msg>        -- unexpected internal failure (I/O, alloc, etc.)
///   WARN <msg>                  -- dispatched but with a non-fatal advisory
///
/// Tags are ASCII upper-case; the separator after a tag is a single space.
/// ERROR: codes are SCREAMING_SNAKE after the colon so callers can switch on
/// just the code without parsing the trailing message text.
///
/// This convention mirrors the DDE ParseResult enum in EoDdeLib so that a
/// future unified transport layer can share response-building code.

#include <string>
#include <string_view>

namespace EoPipe {

/// @brief The Win32 named-pipe path used by the server and all clients.
/// Must match the name used in EoNamedPipeServer.cpp.
inline constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\AeSys";

/// @brief Maximum bytes read from the pipe in a single call.
/// Commands longer than this are rejected with ERROR:INTERNAL.
inline constexpr DWORD kMaxRequestBytes = 4096;

/// @brief Maximum bytes written to the pipe in a single response.
inline constexpr DWORD kMaxResponseBytes = 512;

// ---------------------------------------------------------------------------
// Response-building helpers
// ---------------------------------------------------------------------------

/// @brief Builds an "OK" response with no value payload.
[[nodiscard]] inline std::string ResponseOk() {
    return "OK\n";
}

/// @brief Builds an "OK <value>" response.
[[nodiscard]] inline std::string ResponseOkValue(std::string_view value) {
    std::string s;
    s.reserve(3 + 1 + value.size() + 1);
    s += "OK ";
    s += value;
    s += '\n';
    return s;
}

/// @brief Builds an "ERROR:<code> <detail>" response.
/// @param code   SCREAMING_SNAKE code, e.g. "UNKNOWN_COMMAND".
/// @param detail Trailing human-readable detail (may be empty).
[[nodiscard]] inline std::string ResponseError(std::string_view code, std::string_view detail = {}) {
    std::string s;
    s.reserve(6 + code.size() + 1 + detail.size() + 1);
    s += "ERROR:";
    s += code;
    if (!detail.empty()) {
        s += ' ';
        s += detail;
    }
    s += '\n';
    return s;
}

/// @brief Builds a "WARN <msg>" response.
[[nodiscard]] inline std::string ResponseWarn(std::string_view msg) {
    std::string s;
    s.reserve(5 + msg.size() + 1);
    s += "WARN ";
    s += msg;
    s += '\n';
    return s;
}

// ---------------------------------------------------------------------------
// Request helpers
// ---------------------------------------------------------------------------

/// @brief Returns true when @p response starts with "OK" (with or without value).
[[nodiscard]] inline bool IsOk(std::string_view response) noexcept {
    return response.size() >= 2
        && response[0] == 'O'
        && response[1] == 'K'
        && (response.size() == 2 || response[2] == ' ' || response[2] == '\n');
}

/// @brief Returns true when @p response starts with "ERROR:".
[[nodiscard]] inline bool IsError(std::string_view response) noexcept {
    return response.size() >= 6 && response.substr(0, 6) == "ERROR:";
}

/// @brief Extracts the error code from an "ERROR:<CODE> ..." response.
/// Returns an empty view when the response is not an error line.
[[nodiscard]] inline std::string_view ErrorCode(std::string_view response) noexcept {
    if (!IsError(response)) { return {}; }
    const std::string_view rest = response.substr(6);
    const auto sp = rest.find(' ');
    const auto nl = rest.find('\n');
    const auto end = std::min(sp, nl);
    return (end == std::string_view::npos) ? rest : rest.substr(0, end);
}

}  // namespace EoPipe
