#pragma once
#include "AeSysState.h"
#include "EoGePoint3d.h"

class AeSysView;
class EoDbPrimitive;

/// @brief Transient mend sub-mode state, pushed on top of the primary mode state
/// when the user presses `M` (Primitive Mend).
///
/// Phase 2E: owns all mend sub-mode data previously scattered across AeSysView
/// members (m_PrimitiveToMend, m_PrimitiveToMendCopy, m_MendPrimitiveBegin,
/// m_MendPrimitiveVertexIndex).  Return commits the drag; Escape reverts it.
/// Both pop back cleanly to the primary mode.
class PrimitiveMendState : public AeSysState {
 public:
  PrimitiveMendState() noexcept = default;
  PrimitiveMendState(const PrimitiveMendState&) = delete;
  PrimitiveMendState& operator=(const PrimitiveMendState&) = delete;
  PrimitiveMendState(PrimitiveMendState&&) = delete;
  PrimitiveMendState& operator=(PrimitiveMendState&&) = delete;
  ~PrimitiveMendState() override;

  void OnEnter(AeSysView* context) override;
  void OnExit(AeSysView* context) override;
  void OnMouseMove(AeSysView* context, UINT flags, CPoint point) override;
  [[nodiscard]] bool ShouldBlockCommand(UINT commandId) const noexcept override;

  /// Returns true when a primitive was successfully selected on enter.
  [[nodiscard]] bool IsEngaged() const noexcept { return m_primitiveToMend != nullptr; }

  // Accessors used by AeSysView::MendStateReturn / MendStateEscape
  [[nodiscard]] EoDbPrimitive* PrimitiveToMend() const noexcept { return m_primitiveToMend; }
  [[nodiscard]] EoDbPrimitive* PrimitiveToMendCopy() const noexcept { return m_primitiveToMendCopy; }
  /// Transfers ownership of the working copy out (caller has already freed or reassigned it).
  void ConsumeCopy() noexcept { m_primitiveToMendCopy = nullptr; }

 private:
  EoDbPrimitive* m_primitiveToMend{};     // non-owning — lives in document layer
  EoDbPrimitive* m_primitiveToMendCopy{}; // owning working copy, freed in OnExit/dtor
  EoGePoint3d m_mendPrimitiveBegin{};
  DWORD m_mendPrimitiveVertexIndex{};

  void DeleteCopy() noexcept;
};
