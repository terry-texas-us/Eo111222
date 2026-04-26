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
#include "Resource.h"

void AeSysView::OnPowerModeOptions() {
  // TODO: Add your command handler code here
}

void AeSysView::OnPowerModeCircuit() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  m_PowerArrow = false;
  m_PowerConductor = false;

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  EoDbConic* circle{};
  auto* group = SelectCircleUsingPoint(cursorPosition, 0.02, circle);
  if (group != nullptr) {
    cursorPosition = circle->Center();
    const double currentRadius = circle->Radius();

    if (pts.IsEmpty()) {
      pts.Add(cursorPosition);
      m_PreviousOp = ModeLineHighlightOp(ID_OP2);
    } else {
      group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      const EoGePoint3d pt1 = pts[0].ProjectToward(cursorPosition, m_PreviousRadius);
      const EoGePoint3d pt2 = cursorPosition.ProjectToward(pts[0], currentRadius);
      group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(Gs::renderState));
      InvalidateScene();
      pts[0] = cursorPosition;
    }
    m_PreviousRadius = currentRadius;
  } else {
    if (pts.IsEmpty()) {
      pts.Add(cursorPosition);
    } else {
      cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

      group = new EoDbGroup;
      document->AddWorkLayerGroup(group);
      const EoGePoint3d pt1 = pts[0].ProjectToward(cursorPosition, m_PreviousRadius);
      const EoGePoint3d pt2 = cursorPosition.ProjectToward(pts[0], 0.0);
      group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(Gs::renderState));
      InvalidateScene();

      pts[0] = cursorPosition;
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP2);
    m_PreviousRadius = 0.;
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
  static EoGePoint3d pointOnCircuit;

  auto cursorPosition = GetCursorPosition();

  m_PowerConductor = false;
  m_PreviousOp = 0;

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  if (!m_PowerArrow || (pointOnCircuit != cursorPosition)) {
    m_PowerArrow = false;
    EoDbLine* circuit{};
    auto* group = SelectLineUsingPoint(cursorPosition, circuit);
    if (group != nullptr) {
      cursorPosition = circuit->ProjectPointToLine(cursorPosition);
      if (circuit->RelOfPt(cursorPosition) <= 0.5) {
        m_CircuitEndPoint = circuit->End();
        if (cursorPosition.DistanceTo(circuit->Begin()) <= 0.1) { cursorPosition = circuit->Begin(); }
      } else {
        m_CircuitEndPoint = circuit->Begin();
        if (cursorPosition.DistanceTo(circuit->End()) <= 0.1) { cursorPosition = circuit->End(); }
      }
      m_PowerArrow = cursorPosition.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
      GenerateHomeRunArrow(cursorPosition, m_CircuitEndPoint);
      cursorPosition = cursorPosition.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
      SetCursorPosition(cursorPosition);
    }
  } else {
    m_PowerArrow = cursorPosition.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
    GenerateHomeRunArrow(cursorPosition, m_CircuitEndPoint);
    cursorPosition = cursorPosition.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
    SetCursorPosition(cursorPosition);
  }
  pointOnCircuit = cursorPosition;
}

void AeSysView::DoPowerModeMouseMove() {
  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = GetCursorPosition();
  const auto numberOfPoints = pts.GetSize();

  switch (m_PreviousOp) {
    case ID_OP2:
      if (pts[0] != cursorPosition) {
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        EoDbConic* circle{};
        auto* group = SelectCircleUsingPoint(cursorPosition, 0.02, circle);
        if (group != nullptr) {
          const double radius = circle->Radius();
          cursorPosition = circle->Center();
          cursorPosition = cursorPosition.ProjectToward(pts[0], radius);
        } else {
          cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        }
        const auto pt1 = pts[0].ProjectToward(cursorPosition, m_PreviousRadius);
        m_PreviewGroup.AddTail(EoDbLine::CreateLine(pt1, cursorPosition)->WithProperties(Gs::renderState));
        InvalidateOverlay();
      }
      break;
  }
  pts.SetSize(numberOfPoints);
}

void AeSysView::DoPowerModeConductor(std::uint16_t conductorType) {
  static EoGePoint3d pointOnCircuit;

  EoGePoint3d cursorPosition = GetCursorPosition();

  m_PowerArrow = false;
  m_PreviousOp = 0;

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();

  if (!m_PowerConductor || pointOnCircuit != cursorPosition) {
    m_PowerConductor = false;
    EoDbLine* circuit{};
    auto* group = SelectLineUsingPoint(cursorPosition, circuit);
    if (group != nullptr) {
      cursorPosition = circuit->ProjectPointToLine(cursorPosition);

      const EoGePoint3d beginPoint = circuit->Begin();
      m_CircuitEndPoint = circuit->End();

      if (std::abs(m_CircuitEndPoint.x - beginPoint.x) > 0.025) {
        if (beginPoint.x > m_CircuitEndPoint.x) { m_CircuitEndPoint = beginPoint; }
      } else if (beginPoint.y > m_CircuitEndPoint.y) {
        m_CircuitEndPoint = beginPoint;
      }

      GeneratePowerConductorSymbol(conductorType, cursorPosition, m_CircuitEndPoint);
      cursorPosition = cursorPosition.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
      SetCursorPosition(cursorPosition);
      m_PowerConductor = cursorPosition.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
    }
  } else {
    GeneratePowerConductorSymbol(conductorType, cursorPosition, m_CircuitEndPoint);
    cursorPosition = cursorPosition.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
    SetCursorPosition(cursorPosition);
    m_PowerConductor = cursorPosition.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
  }
  pointOnCircuit = cursorPosition;
}

void AeSysView::OnPowerModeReturn() {
  OnPowerModeEscape();
}

void AeSysView::OnPowerModeEscape() {
  m_PowerArrow = false;
  m_PowerConductor = false;

  pts.RemoveAll();

  ModeLineUnhighlightOp(m_PreviousOp);
  m_PreviousOp = 0;

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateOverlay();
}

void AeSysView::GenerateHomeRunArrow(EoGePoint3d& pointOnCircuit, EoGePoint3d& endPoint) const {
  auto* document = GetDocument();
  EoGePoint3dArray points;
  points.SetSize(3);

  points[0] = pointOnCircuit.ProjectToward(endPoint, 0.05);

  const EoGeLine circuit(points[0], endPoint);

  circuit.ProjPtFrom_xy(0.0, -0.075, &points[0]);
  points[1] = pointOnCircuit;
  circuit.ProjPtFrom_xy(0.0, 0.075, &points[2]);

  auto* group = new EoDbGroup;
  document->AddWorkLayerGroup(group);
  group->AddTail(new EoDbPolyline(2, 1, points));
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::GeneratePowerConductorSymbol(std::uint16_t conductorType,
    EoGePoint3d& pointOnCircuit,
    EoGePoint3d& endPoint) const {
  auto* document = GetDocument();
  EoGePoint3d points[5]{};

  const EoGeLine circuit(pointOnCircuit, endPoint);
  auto* group = new EoDbGroup;

  switch (conductorType) {
    case ID_OP4: {
      circuit.ProjPtFrom_xy(0.0, -0.1, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.075, &points[1]);
      circuit.ProjPtFrom_xy(0.0, 0.0875, &points[2]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));

      auto* circle = EoDbConic::CreateCircleInView(points[2], 0.0125);
      circle->SetColor(1);
      circle->SetLineTypeName(L"CONTINUOUS");

      group->AddTail(circle);
    } break;

    case ID_OP5:
      circuit.ProjPtFrom_xy(0.0, -0.1, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.1, &points[1]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));
      break;

    case ID_OP6:
      circuit.ProjPtFrom_xy(0.0, -0.1, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.05, &points[1]);

      points[2] = pointOnCircuit.ProjectToward(endPoint, 0.025);

      EoGeLine(points[2], endPoint).ProjPtFrom_xy(0.0, 0.075, &points[3]);
      EoGeLine(pointOnCircuit, endPoint).ProjPtFrom_xy(0.0, 0.1, &points[4]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));
      group->AddTail(EoDbLine::CreateLine(points[1], points[3])->WithProperties(1, L"CONTINUOUS"));
      group->AddTail(EoDbLine::CreateLine(points[3], points[4])->WithProperties(1, L"CONTINUOUS"));
      break;

    case ID_OP7:
      circuit.ProjPtFrom_xy(0.0, -0.05, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.05, &points[1]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));
      break;

    default:
      delete group;
      return;
  }
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}
