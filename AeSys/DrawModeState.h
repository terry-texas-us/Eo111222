#pragma once
#if defined(USING_STATE_PATTERN)
#include <cstdint>

#include "AeSysState.h"

#include "EoGePoint3d.h"  // For m_pts

class AeSysView;

class DrawModeState : public AeSysState {
 private:
  std::uint16_t m_PreviousDrawCommand = 0;  // Migrated from legacy
  EoGePoint3dArray m_pts;                   // For multi-point primitives (line, poly, etc.)

 public:
  // declare/define ctors as default to allow copying/moving if needed
  DrawModeState() = default;
  DrawModeState(const DrawModeState&) = delete;
  DrawModeState& operator=(const DrawModeState&) = delete;
  DrawModeState(DrawModeState&&) = delete;
  DrawModeState& operator=(DrawModeState&&) = delete;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;
  void HandleCommand(AeSysView* context, UINT nID) override;
  void OnMouseMove(AeSysView* context, UINT nFlags, CPoint point) override;
  void OnLButtonDown(AeSysView* context, UINT nFlags, CPoint point) override;                // For point input
  void OnUpdate(AeSysView* context, CView* pSender, LPARAM lHint, CObject* pHint) override;  // For preview updates
  // Add OnDraw if draw mode needs custom overlays
};
#endif
