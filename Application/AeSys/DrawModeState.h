#pragma once
#if defined(USING_STATE_PATTERN)
#include <cstdint>

#include "AeSysState.h"

#include "EoGePoint3d.h"  // For m_pts

class AeSysView;

class DrawModeState : public AeSysState {
 private:
  std::uint16_t m_previousDrawCommand{};
  EoGePoint3dArray m_pts;  // For multi-point primitives (line, poly, etc.)

 public:
  // declare/define ctors as default to allow copying/moving if needed
  DrawModeState() = default;
  DrawModeState(const DrawModeState&) = delete;
  DrawModeState& operator=(const DrawModeState&) = delete;
  DrawModeState(DrawModeState&&) = delete;
  DrawModeState& operator=(DrawModeState&&) = delete;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;
  void HandleCommand(AeSysView* context, UINT command) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnLButtonDown(AeSysView* context, UINT nFlags, CPoint point) override;  // For point input

  // Drawing/Updates

  void OnDraw([[maybe_unused]] AeSysView* context, [[maybe_unused]] CDC* deviceContext) override;

  bool OnUpdate(AeSysView* context, CView* sender, LPARAM hint, CObject* objectHint);
};
#endif
