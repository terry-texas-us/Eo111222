#pragma once
#include "AeSysState.h"
#include "EoDbGroup.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"

class AeSysView;

/// @brief Transient pick-and-drag sub-mode state, pushed on top of the primary mode
/// state when the user presses `;` (primitive edit) or `:` (group edit).
///
/// Phase 2D: owns all edit sub-mode data previously scattered across AeSysView members
/// (m_SubModeEditGroup, m_SubModeEditPrimitive, m_SubModeEditBeginPoint,
/// m_SubModeEditEndPoint, m_tmEditSeg).  Escape pops this state and returns cleanly
/// to the primary mode below it on the stack — no PrimaryMode sidecar needed.
///
/// app.CurrentMode() is still set to ID_MODE_PRIMITIVE_EDIT / ID_MODE_GROUP_EDIT so
/// the existing OnOp* handlers (transform, copy, rotate) work without changes.
class PickAndDragState : public AeSysState {
 public:
  enum class Kind { Primitive, Group };

  explicit PickAndDragState(Kind kind) noexcept : m_kind{kind} {}
  PickAndDragState(const PickAndDragState&) = delete;
  PickAndDragState& operator=(const PickAndDragState&) = delete;
  PickAndDragState(PickAndDragState&&) = delete;
  PickAndDragState& operator=(PickAndDragState&&) = delete;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;
  void OnMouseMove(AeSysView* context, UINT flags, CPoint point) override;

  // --- Accessors used by the existing Do* handlers (DoEditPrimitive*, DoEditGroup*) ---
  [[nodiscard]] Kind GetKind() const noexcept { return m_kind; }
  [[nodiscard]] EoDbGroup* SubModeEditGroup() const noexcept { return m_subModeEditGroup; }
  [[nodiscard]] EoDbPrimitive* SubModeEditPrimitive() const noexcept { return m_subModeEditPrimitive; }
  [[nodiscard]] EoGePoint3d& SubModeEditBeginPoint() noexcept { return m_subModeEditBeginPoint; }
  [[nodiscard]] EoGePoint3d& SubModeEditEndPoint() noexcept { return m_subModeEditEndPoint; }
  [[nodiscard]] EoGeTransformMatrix& EditSegTransform() noexcept { return m_tmEditSeg; }

  void SetSubModeEditGroup(EoDbGroup* group) noexcept { m_subModeEditGroup = group; }
  void SetSubModeEditPrimitive(EoDbPrimitive* primitive) noexcept { m_subModeEditPrimitive = primitive; }

 private:
  Kind m_kind;
  EoDbGroup* m_subModeEditGroup{};
  EoDbPrimitive* m_subModeEditPrimitive{};
  EoGePoint3d m_subModeEditBeginPoint{};
  EoGePoint3d m_subModeEditEndPoint{};
  EoGeTransformMatrix m_tmEditSeg{};
};
