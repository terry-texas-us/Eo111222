#pragma once
#if defined(USING_STATE_PATTERN)
#include "AeSysState.h"

class AeSysView;

class IdleState : public AeSysState {
 public:
  // declare/define ctors as default to allow copying/moving if needed
  IdleState() = default;
  IdleState(const IdleState&) = delete;
  IdleState& operator=(const IdleState&) = delete;
  IdleState(IdleState&&) = delete;
  IdleState& operator=(IdleState&&) = delete;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;
  // Override others only if idle has specific behavior (e.g., basic selection)
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  // ... add more as needed
};
#endif