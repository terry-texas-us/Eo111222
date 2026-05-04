#pragma once

#include "dde.h"

namespace dde {
/// @brief Forwards the first DDE argument as a raw command line into the CLI execution pipeline.
bool ExecCLICommand(PTOPICINFO, wchar_t*, UINT, UINT, wchar_t**);

/// @brief Opens a tracing file as a new layer and makes it the active work layer.
bool ExecTracingOpen(PTOPICINFO, wchar_t*, UINT, UINT, wchar_t**);

/// @brief Loads a tracing/job file and places it at the cursor position via the trap.
bool ExecTracingGet(PTOPICINFO, wchar_t*, UINT, UINT, wchar_t**);
}  // namespace dde
