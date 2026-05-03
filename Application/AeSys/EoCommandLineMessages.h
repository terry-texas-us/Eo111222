#pragma once

/// @brief Custom window message posted by the command-line tab to inject a
/// world-coordinate click into the active view.
///
/// Usage:
///   WPARAM = 0 (reserved)
///   LPARAM = reinterpret_cast<LPARAM>(new EoGePoint3d{x, y, z})
///
/// The view handler owns the heap object and must delete it.
#define WM_CMDLINE_INJECT_POINT (WM_APP + 1)
