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

  // Input handling (keypad, mouse)
  virtual bool HandleKeypad([[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT nChar,
      [[maybe_unused]] UINT nRepCnt, [[maybe_unused]] UINT nFlags) {
    return false;
  }
  virtual void OnLButtonDown(
      [[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {}
  virtual void OnMouseMove(
      [[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT nFlags, [[maybe_unused]] CPoint point) {}
  // Add more: OnRButtonDown, etc., as needed from AeSysView.cpp

  // Command handling (delegate MFC ON_COMMAND)
  virtual void HandleCommand([[maybe_unused]] AeSysView* context, [[maybe_unused]] UINT command) {}

  // Drawing/Updates
  virtual void OnDraw([[maybe_unused]] AeSysView* context, [[maybe_unused]] CDC* deviceContext) {}

  virtual bool OnUpdate([[maybe_unused]] AeSysView* context, [[maybe_unused]] CView* sender,
      [[maybe_unused]] LPARAM hint, [[maybe_unused]] CObject* objectHint) {
    return false;
  }
};