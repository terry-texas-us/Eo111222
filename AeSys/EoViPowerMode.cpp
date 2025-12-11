#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

void AeSysView::OnPowerModeOptions() {
  // TODO: Add your command handler code here
}

void AeSysView::OnPowerModeCircuit() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  m_PowerArrow = false;
  m_PowerConductor = false;

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  EoDbEllipse* SymbolCircle;
  EoDbGroup* Group = SelectCircleUsingPoint(CurrentPnt, .02, SymbolCircle);
  if (Group != 0) {
    CurrentPnt = SymbolCircle->Center();
    double CurrentRadius = SymbolCircle->GetMajAx().Length();

    if (pts.IsEmpty()) {
      pts.Add(CurrentPnt);
      m_PreviousOp = ModeLineHighlightOp(ID_OP2);
    } else {
      Group = new EoDbGroup;
      GetDocument()->AddWorkLayerGroup(Group);
      EoGePoint3d pt1 = pts[0].ProjectToward(CurrentPnt, m_PreviousRadius);
      EoGePoint3d pt2 = CurrentPnt.ProjectToward(pts[0], CurrentRadius);
      Group->AddTail(new EoDbLine(pt1, pt2));
      pts[0] = CurrentPnt;
    }
    m_PreviousRadius = CurrentRadius;
  } else {
    if (pts.IsEmpty()) {
      pts.Add(CurrentPnt);
    } else {
      CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);

      Group = new EoDbGroup;
      GetDocument()->AddWorkLayerGroup(Group);
      EoGePoint3d pt1 = pts[0].ProjectToward(CurrentPnt, m_PreviousRadius);
      EoGePoint3d pt2 = CurrentPnt.ProjectToward(pts[0], 0.0);
      Group->AddTail(new EoDbLine(pt1, pt2));

      pts[0] = CurrentPnt;
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
  static EoGePoint3d PointOnCircuit;

  EoGePoint3d CurrentPnt = GetCursorPosition();

  m_PowerConductor = false;
  m_PreviousOp = 0;

  GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  if (!m_PowerArrow || (PointOnCircuit != CurrentPnt)) {
    m_PowerArrow = false;
    EoDbLine* Circuit;
    EoDbGroup* Group = SelectLineUsingPoint(CurrentPnt, Circuit);
    if (Group != 0) {
      CurrentPnt = Circuit->ProjPt(CurrentPnt);
      if (Circuit->RelOfPt(CurrentPnt) <= .5) {
        m_CircuitEndPoint = Circuit->EndPoint();
        if (CurrentPnt.DistanceTo(Circuit->BeginPoint()) <= .1) CurrentPnt = Circuit->BeginPoint();
      } else {
        m_CircuitEndPoint = Circuit->BeginPoint();
        if (CurrentPnt.DistanceTo(Circuit->EndPoint()) <= .1) CurrentPnt = Circuit->EndPoint();
      }
      m_PowerArrow = CurrentPnt.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
      GenerateHomeRunArrow(CurrentPnt, m_CircuitEndPoint);
      CurrentPnt = CurrentPnt.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
      SetCursorPosition(CurrentPnt);
    }
  } else {
    m_PowerArrow = CurrentPnt.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
    GenerateHomeRunArrow(CurrentPnt, m_CircuitEndPoint);
    CurrentPnt = CurrentPnt.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
    SetCursorPosition(CurrentPnt);
  }
  PointOnCircuit = CurrentPnt;
}

void AeSysView::DoPowerModeMouseMove() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  int NumberOfPoints = pts.GetSize();

  switch (m_PreviousOp) {
    case ID_OP2:
      if (pts[0] != CurrentPnt) {
        GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        EoDbEllipse* SymbolCircle;
        EoDbGroup* Group = SelectCircleUsingPoint(CurrentPnt, .02, SymbolCircle);
        if (Group != 0) {
          double CurrentRadius = SymbolCircle->GetMajAx().Length();
          CurrentPnt = SymbolCircle->Center();
          CurrentPnt = CurrentPnt.ProjectToward(pts[0], CurrentRadius);
        } else {
          CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        }
        EoGePoint3d pt1 = pts[0].ProjectToward(CurrentPnt, m_PreviousRadius);
        m_PreviewGroup.AddTail(new EoDbLine(pt1, CurrentPnt));
        GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;
  }
  pts.SetSize(NumberOfPoints);
}
void AeSysView::DoPowerModeConductor(EoUInt16 conductorType) {
  static EoGePoint3d PointOnCircuit;

  EoGePoint3d CurrentPnt = GetCursorPosition();

  m_PowerArrow = false;
  m_PreviousOp = 0;

  GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  if (!m_PowerConductor || PointOnCircuit != CurrentPnt) {
    m_PowerConductor = false;
    EoDbLine* Circuit;
    EoDbGroup* Group = SelectLineUsingPoint(CurrentPnt, Circuit);
    if (Group != 0) {
      CurrentPnt = Circuit->ProjPt(CurrentPnt);

      EoGePoint3d BeginPoint = Circuit->BeginPoint();
      m_CircuitEndPoint = Circuit->EndPoint();

      if (fabs(m_CircuitEndPoint.x - BeginPoint.x) > .025) {
        if (BeginPoint.x > m_CircuitEndPoint.x) m_CircuitEndPoint = BeginPoint;
      } else if (BeginPoint.y > m_CircuitEndPoint.y)
        m_CircuitEndPoint = BeginPoint;

      GeneratePowerConductorSymbol(conductorType, CurrentPnt, m_CircuitEndPoint);
      CurrentPnt = CurrentPnt.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
      SetCursorPosition(CurrentPnt);
      m_PowerConductor = CurrentPnt.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
    }
  } else {
    GeneratePowerConductorSymbol(conductorType, CurrentPnt, m_CircuitEndPoint);
    CurrentPnt = CurrentPnt.ProjectToward(m_CircuitEndPoint, m_PowerConductorSpacing);
    SetCursorPosition(CurrentPnt);
    m_PowerConductor = CurrentPnt.DistanceTo(m_CircuitEndPoint) > m_PowerConductorSpacing;
  }
  PointOnCircuit = CurrentPnt;
}

void AeSysView::OnPowerModeReturn() { OnPowerModeEscape(); }

void AeSysView::OnPowerModeEscape() {
  m_PowerArrow = false;
  m_PowerConductor = false;

  pts.RemoveAll();

  ModeLineUnhighlightOp(m_PreviousOp);
  m_PreviousOp = 0;

  GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
}

void AeSysView::GenerateHomeRunArrow(EoGePoint3d& pointOnCircuit, EoGePoint3d& endPoint) {
  EoGePoint3dArray Points;
  Points.SetSize(3);

  Points[0] = pointOnCircuit.ProjectToward(endPoint, .05);

  EoGeLine Circuit(Points[0], endPoint);

  Circuit.ProjPtFrom_xy(0.0, -.075, &Points[0]);
  Points[1] = pointOnCircuit;
  Circuit.ProjPtFrom_xy(0.0, .075, &Points[2]);

  EoDbGroup* Group = new EoDbGroup;
  GetDocument()->AddWorkLayerGroup(Group);
  Group->AddTail(new EoDbPolyline(2, 1, Points));
  GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
}
void AeSysView::GeneratePowerConductorSymbol(EoUInt16 conductorType, EoGePoint3d& pointOnCircuit,
                                             EoGePoint3d& endPoint) {
  EoGePoint3d Points[5];

  EoGeLine Circuit(pointOnCircuit, endPoint);
  EoDbGroup* Group = new EoDbGroup;

  switch (conductorType) {
    case ID_OP4:
      Circuit.ProjPtFrom_xy(0.0, -.1, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, .075, &Points[1]);
      Circuit.ProjPtFrom_xy(0.0, .0875, &Points[2]);
      Group->AddTail(new EoDbLine(1, 1, Points[0], Points[1]));
      Group->AddTail(new EoDbEllipse(1, 1, Points[2], .0125));
      break;

    case ID_OP5:
      Circuit.ProjPtFrom_xy(0.0, -.1, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, .1, &Points[1]);
      Group->AddTail(new EoDbLine(1, 1, Points[0], Points[1]));
      break;

    case ID_OP6:
      Circuit.ProjPtFrom_xy(0.0, -.1, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, .05, &Points[1]);

      Points[2] = pointOnCircuit.ProjectToward(endPoint, .025);

      EoGeLine(Points[2], endPoint).ProjPtFrom_xy(0.0, .075, &Points[3]);
      EoGeLine(pointOnCircuit, endPoint).ProjPtFrom_xy(0.0, .1, &Points[4]);
      Group->AddTail(new EoDbLine(1, 1, Points[0], Points[1]));
      Group->AddTail(new EoDbLine(1, 1, Points[1], Points[3]));
      Group->AddTail(new EoDbLine(1, 1, Points[3], Points[4]));
      break;

    case ID_OP7:
      Circuit.ProjPtFrom_xy(0.0, -.05, &Points[0]);
      Circuit.ProjPtFrom_xy(0.0, .05, &Points[1]);
      Group->AddTail(new EoDbLine(1, 1, Points[0], Points[1]));
      break;

    default:
      delete Group;
      return;
  }
  GetDocument()->AddWorkLayerGroup(Group);
  GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
}
