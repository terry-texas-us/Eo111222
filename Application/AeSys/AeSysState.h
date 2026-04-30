#pragma once

#include <afxwin.h>

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
  virtual void OnLButtonDown([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] UINT nFlags,
      [[maybe_unused]] CPoint point) {}
  virtual void OnMouseMove([[maybe_unused]] AeSysView* context,
      [[maybe_unused]] UINT nFlags,
      [[maybe_unused]] CPoint point) {}
  // Add more: OnRButtonDown, etc., as needed from AeSysView.cpp

  // Command handling (delegate MFC ON_COMMAND).
  // Return true to consume the command and prevent it reaching the legacy CurrentMode() switch.
  [[nodiscard]] virtual bool HandleCommand([[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT command) {
    return false;
  }

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