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
  auto* Group = SelectCircleUsingPoint(cursorPosition, 0.02, circle);
  if (Group != nullptr) {
    cursorPosition = circle->Center();
    double CurrentRadius = circle->Radius();

    if (pts.IsEmpty()) {
      pts.Add(cursorPosition);
      m_PreviousOp = ModeLineHighlightOp(ID_OP2);
    } else {
      Group = new EoDbGroup;
      document->AddWorkLayerGroup(Group);
      EoGePoint3d pt1 = pts[0].ProjectToward(cursorPosition, m_PreviousRadius);
      EoGePoint3d pt2 = cursorPosition.ProjectToward(pts[0], CurrentRadius);
      Group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
      pts[0] = cursorPosition;
    }
    m_PreviousRadius = CurrentRadius;
  } else {
    if (pts.IsEmpty()) {
      pts.Add(cursorPosition);
    } else {
      cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

      Group = new EoDbGroup;
      document->AddWorkLayerGroup(Group);
      EoGePoint3d pt1 = pts[0].ProjectToward(cursorPosition, m_PreviousRadius);
      EoGePoint3d pt2 = cursorPosition.ProjectToward(pts[0], 0.0);
      Group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(renderState.Color(), renderState.LineTypeIndex()));

      pts[0] = cursorPosition;
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP2);
    m_PreviousRadius = 0.;
  }
}

void AeSysView::OnPowerModeGround() { DoPowerModeConductor(ID_OP4); }

void AeSysView::OnPowerModeHot() { DoPowerModeConductor(ID_OP5); }

void AeSysView::OnPowerModeSwitch() { DoPowerModeConductor(ID_OP6); }

void AeSysView::OnPowerModeNeutral() { DoPowerModeConductor(ID_OP7); }

void AeSysView::OnPowerModeHome() {
  auto* document = GetDocument();
  static EoGePoint3d PointOnCircuit;

  auto cursorPosition = GetCursorPosition();

  m_PowerConductor = false;
  m_PreviousOp = 0;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  if (!m_PowerArrow || (PointOnCircuit != cursorPosition)) {
    m_PowerArrow = false;
    EoDbLine* circuit{};
    auto* group = SelectLineUsingPoint(cursorPosition, circuit);
    if (group != nullptr) {
      cursorPosition = circuit->ProjectPointToLine(cursorPosition);
      if (circuit->RelOfPt(cursorPosition) <= 0.5) {
        m_CircuitEndPoint = circuit->End();
        if (cursorPosition.DistanceTo(circuit->Begin()) <= 0.1) cursorPosition = circuit->Begin();
      } else {
        m_CircuitEndPoint = circuit->Begin();
        if (cursorPosition.DistanceTo(circuit->End()) <= 0.1) cursorPosition = circuit->End();
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
  PointOnCircuit = cursorPosition;
}

void AeSysView::DoPowerModeMouseMove() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  auto numberOfPoints = pts.GetSize();

  switch (m_PreviousOp) {
    case ID_OP2:
      if (pts[0] != cursorPosition) {
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        EoDbConic* circle{};
        auto* group = SelectCircleUsingPoint(cursorPosition, 0.02, circle);
        if (group != nullptr) {
          double radius = circle->Radius();
          cursorPosition = circle->Center();
          cursorPosition = cursorPosition.ProjectToward(pts[0], radius);
        } else {
          cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        }
        auto pt1 = pts[0].ProjectToward(cursorPosition, m_PreviousRadius);
        m_PreviewGroup.AddTail(EoDbLine::CreateLine(pt1, cursorPosition)->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;
  }
  pts.SetSize(numberOfPoints);
}

void AeSysView::DoPowerModeConductor(std::uint16_t conductorType) {
  auto* document = GetDocument();
  static EoGePoint3d PointOnCircuit;

  EoGePoint3d cursorPosition = GetCursorPosition();

  m_PowerArrow = false;
  m_PreviousOp = 0;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  if (!m_PowerConductor || PointOnCircuit != cursorPosition) {
    m_PowerConductor = false;
    EoDbLine* circuit{};
    auto* group = SelectLineUsingPoint(cursorPosition, circuit);
    if (group != 0) {
      cursorPosition = circuit->ProjectPointToLine(cursorPosition);

      EoGePoint3d BeginPoint = circuit->Begin();
      m_CircuitEndPoint = circuit->End();

      if (std::abs(m_CircuitEndPoint.x - BeginPoint.x) > 0.025) {
        if (BeginPoint.x > m_CircuitEndPoint.x) m_CircuitEndPoint = BeginPoint;
      } else if (BeginPoint.y > m_CircuitEndPoint.y)
        m_CircuitEndPoint = BeginPoint;

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
  PointOnCircuit = cursorPosition;
}

void AeSysView::OnPowerModeReturn() { OnPowerModeEscape(); }

void AeSysView::OnPowerModeEscape() {
  auto* document = GetDocument();
  m_PowerArrow = false;
  m_PowerConductor = false;

  pts.RemoveAll();

  ModeLineUnhighlightOp(m_PreviousOp);
  m_PreviousOp = 0;

  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
}

void AeSysView::GenerateHomeRunArrow(EoGePoint3d& pointOnCircuit, EoGePoint3d& endPoint) const {
  auto* document = GetDocument();
  EoGePoint3dArray Points;
  Points.SetSize(3);

  Points[0] = pointOnCircuit.ProjectToward(endPoint, 0.05);

  EoGeLine Circuit(Points[0], endPoint);

  Circuit.ProjPtFrom_xy(0.0, -0.075, &Points[0]);
  Points[1] = pointOnCircuit;
  Circuit.ProjPtFrom_xy(0.0, 0.075, &Points[2]);

  auto* Group = new EoDbGroup;
  document->AddWorkLayerGroup(Group);
  Group->AddTail(new EoDbPolyline(2, 1, Points));
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
}

void AeSysView::GeneratePowerConductorSymbol(std::uint16_t conductorType, EoGePoint3d& pointOnCircuit, EoGePoint3d& endPoint) const {
  auto* document = GetDocument();
  EoGePoint3d Points[5]{};

  EoGeLine Circuit(pointOnCircuit, endPoint);
  auto* group = new EoDbGroup;

  switch (conductorType) {
    case ID_OP4: {
      Circuit.ProjPtFrom_xy(0.0, -0.1, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, 0.075, &Points[1]);
      Circuit.ProjPtFrom_xy(0.0, 0.0875, &Points[2]);
      group->AddTail(EoDbLine::CreateLine(Points[0], Points[1])->WithProperties(1, 1));
      
      auto* circle = EoDbConic::CreateCircleInView(Points[2], 0.0125);
      circle->SetColor(1);
      circle->SetLineTypeIndex(1);

      group->AddTail(circle);
    } break;

    case ID_OP5:
      Circuit.ProjPtFrom_xy(0.0, -0.1, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, 0.1, &Points[1]);
      group->AddTail(EoDbLine::CreateLine(Points[0], Points[1])->WithProperties(1, 1));
      break;

    case ID_OP6:
      Circuit.ProjPtFrom_xy(0.0, -0.1, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, 0.05, &Points[1]);

      Points[2] = pointOnCircuit.ProjectToward(endPoint, 0.025);

      EoGeLine(Points[2], endPoint).ProjPtFrom_xy(0.0, 0.075, &Points[3]);
      EoGeLine(pointOnCircuit, endPoint).ProjPtFrom_xy(0.0, 0.1, &Points[4]);
      group->AddTail(EoDbLine::CreateLine(Points[0], Points[1])->WithProperties(1, 1));
      group->AddTail(EoDbLine::CreateLine(Points[1], Points[3])->WithProperties(1, 1));
      group->AddTail(EoDbLine::CreateLine(Points[3], Points[4])->WithProperties(1, 1));
      break;

    case ID_OP7:
      Circuit.ProjPtFrom_xy(0.0, -0.05, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, 0.05, &Points[1]);
      group->AddTail(EoDbLine::CreateLine(Points[0], Points[1])->WithProperties(1, 1));
      break;

    default:
      delete group;
      return;
  }
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}
