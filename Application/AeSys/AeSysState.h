#pragma once

#include <afxwin.h>

#include "EoGePoint3d.h"

class AeSysView;

// Base state interface
class AeSysState {
 public:
  AeSysState() = default;
  virtual ~AeSysState() = default;

  // Explicitly delete specials to enforce non-copyable/movable
  AeSysState(const AeSysState&) = delete;
  AeSysState& operator=(const AeSysState&) = delete;
  AeSysState(AeSysState&&) = delete;
  AeSysState& operator=(AeSysState&&) = delete;

  // Lifecycle
  virtual void OnEnter([[maybe_unused]] AeSysView* context) {}
  virtual void OnExit([[maybe_unused]] AeSysView* context) {}

  // Universal Return/Escape delegates — return true to consume the event and
  // prevent it from reaching the legacy CurrentMode() switch on AeSysView.
  virtual bool OnReturn([[maybe_unused]] AeSysView* context) { return false; }
  virtual bool OnEscape([[maybe_unused]] AeSysView* context) { return false; }

  // Input handling (keypad, mouse)
  virtual bool HandleKeypad([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] UINT nChar,
      [[maybe_unused]] UINT nRepCnt,
      [[maybe_unused]] UINT nFlags) {
    return false;
  }
  /// Returns the op-command ID that is currently "active" in this mode (e.g.
  /// ID_OP2 while a line gesture is in progress). Used by the default
  /// OnLButtonDown to re-fire the same command on each successive click.
  /// Override in each concrete state to return its named m_previous* field.
  [[nodiscard]] virtual UINT GetActiveOp() const noexcept { return 0; }

  /// Default LMB handler: re-fires the currently active op through HandleCommand
  /// so that each click advances the gesture (e.g. adds a polygon vertex).
  /// States that need richer LMB behaviour (e.g. PickAndDrag) override this.
  virtual void OnLButtonDown(AeSysView* context,
      [[maybe_unused]] UINT nFlags,
      [[maybe_unused]] CPoint point) {
    (void)HandleCommand(context, GetActiveOp());
  }
  virtual void OnLButtonUp([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] UINT nFlags,
      [[maybe_unused]] CPoint point) {}
  virtual void OnRButtonDown([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] UINT nFlags,
      [[maybe_unused]] CPoint point) {}
  virtual void OnRButtonUp([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] UINT nFlags,
      [[maybe_unused]] CPoint point) {}
  virtual void OnMouseMove([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] UINT nFlags,
      [[maybe_unused]] CPoint point) {}

  /// Populate \menu\ with context-sensitive items for an RMB click.
  /// Return true if the menu has at least one item and should be shown; false to fall
  /// through to the default cancel (OnRButtonUp) behaviour.
  /// Override in states that want a context menu instead of a plain cancel.
  [[nodiscard]] virtual bool BuildContextMenu(
      [[maybe_unused]] AeSysView* context,
      [[maybe_unused]] CMenu& menu) {
    return false;
  }

  // Command handling (delegate MFC ON_COMMAND).
  // Return true to consume the command and prevent it reaching the legacy CurrentMode() switch.
  [[nodiscard]] virtual bool HandleCommand([[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT command) {
    return false;
  }

  /// Returns the prompt string the command line should display while this mode
  /// state is active, e.g. L"Specify next point or [Undo/Close]".
  /// Returns an empty string when the state has no prompt to offer.
  [[nodiscard]] virtual const wchar_t* PromptString() const noexcept { return L""; }

  /// Returns a short mode name for display in the CMD status bar pane
  /// (e.g. L"Draw", L"Trap", L"Edit"). Shown when the command tab is not focused.
  /// Base returns empty string; concrete states override with their label.
  [[nodiscard]] virtual const wchar_t* ModeLabel() const noexcept { return L""; }

  /// Returns true when a drawing gesture is in progress and the dynamic input
  /// tooltip should be shown near the cursor.
  [[nodiscard]] virtual bool HasActiveGesture() const noexcept { return false; }

  /// Returns the world-space anchor point for the current gesture segment
  /// (the previously placed point).  Only meaningful when HasActiveGesture() is true.
  /// Returns a zero point by default.
  [[nodiscard]] virtual EoGePoint3d GestureAnchorWorld() const noexcept { return {}; }

  /// Returns a context-sensitive prompt string for the dynamic input tooltip.
  /// More specific than PromptString() -- varies per click count within the op.
  /// Base returns L"" -- tooltip uses its own fallback text.
  [[nodiscard]] virtual const wchar_t* GesturePrompt() const noexcept { return L""; }

  /// Return true to block a WM_COMMAND from reaching the document/view handlers.
  /// Sub-modes that hold raw pointers into document data (PickAndDragState,
  /// PrimitiveMendState) must block any command that could invalidate those pointers
  /// (delete, cut, open, new, etc.) while they are active.
  [[nodiscard]] virtual bool ShouldBlockCommand([[maybe_unused]] UINT commandId) const noexcept { return false; }

  // Drawing/Updates
  // Called after DisplayAllLayers completes — override to add mode-specific overlays.
  // Do NOT call DisplayAllLayers from an override; the scene has already been rendered.
  virtual void OnDraw([[maybe_unused]] AeSysView* context, [[maybe_unused]] CDC* deviceContext) {}

  virtual bool OnUpdate([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] CView* sender,
      [[maybe_unused]] LPARAM hint,
      [[maybe_unused]] CObject* objectHint) {
    return false;
  }
};