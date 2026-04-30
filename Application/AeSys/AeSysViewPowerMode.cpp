#include "Stdafx.h"

#include <cmath>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPolyline.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGsRenderState.h"
#include "EoPowerGeometry.h"
#include "PowerModeState.h"
#include "Resource.h"

namespace {
/// Returns the active PowerModeState from the view's state stack, or nullptr when
/// called outside power mode (e.g. during PopAllModeStates teardown).
PowerModeState* PowerState(AeSysView* view) {
  return dynamic_cast<PowerModeState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnPowerModeOptions() {
  // TODO: Add your command handler code here
}

void AeSysView::OnPowerModeCircuit() {
  auto* state = PowerState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();
  auto& powerArrow = state->PowerArrowRef();
  auto& powerConductor = state->PowerConductorRef();
  auto& previousRadius = state->PreviousRadiusRef();

  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  powerArrow = false;
  powerConductor = false;

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  EoDbConic* circle{};
  auto* group = SelectCircleUsingPoint(cursorPosition, 0.02, circle);
  if (group != nullptr) {
    cursorPosition = circle->Center();
    const double currentRadius = circle->Radius();

    if (points.IsEmpty()) {
      points.Add(cursorPosition);
      previousOp = ModeLineHighlightOp(ID_OP2);
    } else {
      group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      const EoGePoint3d pt1 = points[0].ProjectToward(cursorPosition, previousRadius);
      const EoGePoint3d pt2 = cursorPosition.ProjectToward(points[0], currentRadius);
      group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(Gs::renderState));
      InvalidateScene();
      points[0] = cursorPosition;
    }
    previousRadius = currentRadius;
  } else {
    if (points.IsEmpty()) {
      points.Add(cursorPosition);
    } else {
      cursorPosition = SnapPointToAxis(points[0], cursorPosition);

      group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      const EoGePoint3d pt1 = points[0].ProjectToward(cursorPosition, previousRadius);
      const EoGePoint3d pt2 = cursorPosition.ProjectToward(points[0], 0.0);
      group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(Gs::renderState));
      InvalidateScene();

      points[0] = cursorPosition;
    }
    previousOp = ModeLineHighlightOp(ID_OP2);
    previousRadius = 0.;
  }
}

void AeSysView::OnPowerModeGround() {
  DoPowerModeConductor(ID_OP4);
}

void AeSysView::OnPowerModeHot() {
  DoPowerModeConductor(ID_OP5);
}

void AeSysView::OnPowerModeSwitch() {
  DoPowerModeConductor(ID_OP6);
}

void AeSysView::OnPowerModeNeutral() {
  DoPowerModeConductor(ID_OP7);
}

void AeSysView::OnPowerModeHome() {
  auto* state = PowerState(this);
  if (state == nullptr) { return; }
  auto& powerArrow = state->PowerArrowRef();
  auto& powerConductor = state->PowerConductorRef();
  auto& circuitEndPoint = state->CircuitEndPointRef();
  auto& pointOnCircuit = state->PointOnCircuitHomeRef();

  auto cursorPosition = GetCursorPosition();

  powerConductor = false;
  state->SetPreviousOp(0);

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  if (!powerArrow || (pointOnCircuit != cursorPosition)) {
    powerArrow = false;
    EoDbLine* circuit{};
    const auto* group = SelectLineUsingPoint(cursorPosition, circuit);
    if (group != nullptr) {
      cursorPosition = circuit->ProjectPointToLine(cursorPosition);
      if (circuit->RelOfPt(cursorPosition) <= 0.5) {
        circuitEndPoint = circuit->End();
        if (cursorPosition.DistanceTo(circuit->Begin()) <= 0.1) { cursorPosition = circuit->Begin(); }
      } else {
        circuitEndPoint = circuit->Begin();
        if (cursorPosition.DistanceTo(circuit->End()) <= 0.1) { cursorPosition = circuit->End(); }
      }
      powerArrow = cursorPosition.DistanceTo(circuitEndPoint) > m_powerConfig.conductorSpacing;
      GenerateHomeRunArrow(cursorPosition, circuitEndPoint);
      cursorPosition = cursorPosition.ProjectToward(circuitEndPoint, m_powerConfig.conductorSpacing);
      SetCursorPosition(cursorPosition);
    }
  } else {
    powerArrow = cursorPosition.DistanceTo(circuitEndPoint) > m_powerConfig.conductorSpacing;
    GenerateHomeRunArrow(cursorPosition, circuitEndPoint);
    cursorPosition = cursorPosition.ProjectToward(circuitEndPoint, m_powerConfig.conductorSpacing);
    SetCursorPosition(cursorPosition);
  }
  pointOnCircuit = cursorPosition;
}

void AeSysView::DoPowerModeMouseMove() {
  // Preview logic moved to PowerModeState::OnMouseMove.
}

void AeSysView::DoPowerModeConductor(std::uint16_t conductorType) {
  auto* state = PowerState(this);
  if (state == nullptr) { return; }
  auto& powerArrow = state->PowerArrowRef();
  auto& powerConductor = state->PowerConductorRef();
  auto& circuitEndPoint = state->CircuitEndPointRef();
  auto& pointOnCircuit = state->PointOnCircuitConductorRef();

  EoGePoint3d cursorPosition = GetCursorPosition();

  powerArrow = false;
  state->SetPreviousOp(0);

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  if (!powerConductor || pointOnCircuit != cursorPosition) {
    powerConductor = false;
    EoDbLine* circuit{};
    const auto* group = SelectLineUsingPoint(cursorPosition, circuit);
    if (group != nullptr) {
      cursorPosition = circuit->ProjectPointToLine(cursorPosition);

      const EoGePoint3d beginPoint = circuit->Begin();
      circuitEndPoint = circuit->End();

      if (std::abs(circuitEndPoint.x - beginPoint.x) > 0.025) {
        if (beginPoint.x > circuitEndPoint.x) { circuitEndPoint = beginPoint; }
      } else if (beginPoint.y > circuitEndPoint.y) {
        circuitEndPoint = beginPoint;
      }

      GeneratePowerConductorSymbol(conductorType, cursorPosition, circuitEndPoint);
      cursorPosition = cursorPosition.ProjectToward(circuitEndPoint, m_powerConfig.conductorSpacing);
      SetCursorPosition(cursorPosition);
      powerConductor = cursorPosition.DistanceTo(circuitEndPoint) > m_powerConfig.conductorSpacing;
    }
  } else {
    GeneratePowerConductorSymbol(conductorType, cursorPosition, circuitEndPoint);
    cursorPosition = cursorPosition.ProjectToward(circuitEndPoint, m_powerConfig.conductorSpacing);
    SetCursorPosition(cursorPosition);
    powerConductor = cursorPosition.DistanceTo(circuitEndPoint) > m_powerConfig.conductorSpacing;
  }
  pointOnCircuit = cursorPosition;
}

void AeSysView::OnPowerModeReturn() {
  OnPowerModeEscape();
}

void AeSysView::OnPowerModeEscape() {
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  auto* state = PowerState(this);
  if (state == nullptr) { return; }
  state->PowerArrowRef() = false;
  state->PowerConductorRef() = false;
  state->Points().RemoveAll();
  state->UnhighlightOp(this);
}

void AeSysView::GenerateHomeRunArrow(EoGePoint3d& pointOnCircuit, const EoGePoint3d& endPoint) const {
  auto* document = GetDocument();
  auto* group = new EoDbGroup;
  Power::FillHomeRunArrow(group, pointOnCircuit, endPoint);
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::GeneratePowerConductorSymbol(std::uint16_t conductorType,
    const EoGePoint3d& pointOnCircuit,
    const EoGePoint3d& endPoint) const {
  auto* document = GetDocument();
  auto* group = new EoDbGroup;
  if (!Power::FillConductorSymbol(conductorType, group, pointOnCircuit, endPoint)) {
    delete group;
    return;
  }
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

