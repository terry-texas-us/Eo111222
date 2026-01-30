#include "Stdafx.h"

#include <algorithm>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbConic.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPolyline.h"
#include "EoDbText.h"
#include "EoDlgAnnotateOptions.h"
#include "EoDlgSetText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeVector3d.h"
#include "PrimState.h"
#include "Resource.h"

void AeSysView::OnAnnotateModeOptions() {
  EoDlgAnnotateOptions Dialog(this);

  if (Dialog.DoModal() == IDOK) {}
}

void AeSysView::OnAnnotateModeLine() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (m_PreviousOp == 0) {
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP2, pts[0], cursorPosition)) {
      auto* Group = new EoDbGroup;
      document->AddWorkLayerGroup(Group);

      if (m_PreviousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, pts[0], Group); }
      Group->AddTail(new EoDbLine(1, 1, pts[0], cursorPosition));
      pts[0] = cursorPosition;
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP2);
}

void AeSysView::OnAnnotateModeArrow() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (m_PreviousOp == 0) {
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP3, pts[0], cursorPosition)) {
      auto* group = new EoDbGroup;

      if (m_PreviousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, pts[0], group); }
      group->AddTail(new EoDbLine(1, 1, pts[0], cursorPosition));
      GenerateLineEndItem(EndItemType(), EndItemSize(), pts[0], cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      pts[0] = cursorPosition;
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP3);
}

void AeSysView::OnAnnotateModeBubble() {
  auto* document = GetDocument();
  static CString CurrentText;
  auto cursorPosition = GetCursorPosition();

  EoDlgSetText dlg;
  dlg.m_strTitle = L"Set Bubble Text";
  dlg.m_sText = CurrentText;
  if (dlg.DoModal() == IDOK) { CurrentText = dlg.m_sText; }
  auto* group = new EoDbGroup;
  document->AddWorkLayerGroup(group);
  if (m_PreviousOp == 0) {  // No operation pending
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    EoGePoint3d pt(cursorPosition);

    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP4, pts[0], pt)) {
      if (m_PreviousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, pts[0], group); }
      group->AddTail(new EoDbLine(1, 1, pts[0], pt));
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP4);

  if (!CurrentText.IsEmpty()) {
    CDC* DeviceContext = GetDC();

    auto cameraDirection = CameraDirection();
    auto minorAxis = ViewUp();
    auto majorAxis = CrossProduct(minorAxis, cameraDirection);

    majorAxis *= 0.06;
    minorAxis *= 0.1;
    EoGeReferenceSystem referenceSystem(cursorPosition, majorAxis, minorAxis);

    int PrimitiveState = pstate.Save();
    pstate.SetColor(DeviceContext, 2);

    EoDbFontDefinition fd;
    pstate.GetFontDef(fd);
    fd.HorizontalAlignment(EoDb::kAlignCenter);
    fd.VerticalAlignment(EoDb::kAlignMiddle);

    EoDbCharacterCellDefinition ccd;
    pstate.GetCharCellDef(ccd);
    ccd.TextRotAngSet(0.0);
    pstate.SetCharCellDef(ccd);

    group->AddTail(new EoDbText(fd, referenceSystem, CurrentText));
    pstate.Restore(DeviceContext, PrimitiveState);
    ReleaseDC(DeviceContext);
  }
  if (NumberOfSides() == 0) {
    auto circle = EoDbConic::CreateCircleInView(cursorPosition, BubbleRadius());
    circle->SetColor(1);
    circle->SetLineTypeIndex(1);

    group->AddTail(circle);
  } else {
    group->AddTail(new EoDbPolyline(1, 1, cursorPosition, BubbleRadius(), NumberOfSides()));
  }
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  pts[0] = cursorPosition;
}

void AeSysView::OnAnnotateModeHook() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  auto* group = new EoDbGroup;
  if (m_PreviousOp == 0) {
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    EoGePoint3d pt(cursorPosition);

    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP5, pts[0], pt)) {
      if (m_PreviousOp == ID_OP3) GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, pts[0], group);

      group->AddTail(new EoDbLine(1, 1, pts[0], pt));
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP5);
  
  auto circle = EoDbConic::CreateCircleInView(cursorPosition, CircleRadius());
  circle->SetColor(1);
  circle->SetLineTypeIndex(1);

  group->AddTail(circle);

  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  pts[0] = cursorPosition;
}

void AeSysView::OnAnnotateModeUnderline() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  pts.RemoveAll();

  if (m_PreviousOp != 0) {
    ModeLineUnhighlightOp(m_PreviousOp);
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  EoDbText* pText = SelectTextUsingPoint(cursorPosition);
  if (pText != 0) {
    pText->GetBoundingBox(pts, GapSpaceFactor());

    auto* Group = new EoDbGroup;
    Group->AddTail(new EoDbLine(pstate.PenColor(), 1, pts[0], pts[1]));
    document->AddWorkLayerGroup(Group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

    pts.RemoveAll();
  }
}

void AeSysView::OnAnnotateModeBox() {
  auto cursorPosition = GetCursorPosition();
  if (m_PreviousOp != ID_OP7) {
    if (m_PreviousOp != 0) {
      RubberBandingDisable();
      ModeLineUnhighlightOp(m_PreviousOp);
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP7);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    EoGePoint3dArray ptsBox1;
    EoGePoint3dArray ptsBox2;
    bool bG1Flg = false;
    bool bG2Flg = false;
    EoDbText* pText = SelectTextUsingPoint(pts[0]);
    if (pText != 0) {
      pText->GetBoundingBox(ptsBox1, GapSpaceFactor());
      bG1Flg = true;
    }
    pText = SelectTextUsingPoint(cursorPosition);
    if (pText != 0) {
      pText->GetBoundingBox(ptsBox2, GapSpaceFactor());
      bG2Flg = true;
    }
    if (bG1Flg && bG2Flg) {
      pts.SetSize(4);

      pts[0] = ptsBox1[0];
      pts[2] = ptsBox1[0];
      for (int i = 1; i < 4; i++) {
        pts[0].x = std::min(pts[0].x, ptsBox1[i].x);
        pts[2].x = std::max(pts[2].x, ptsBox1[i].x);
        pts[0].y = std::min(pts[0].y, ptsBox1[i].y);
        pts[2].y = std::max(pts[2].y, ptsBox1[i].y);
      }
      for (int i = 0; i < 4; i++) {
        pts[0].x = std::min(pts[0].x, ptsBox2[i].x);
        pts[2].x = std::max(pts[2].x, ptsBox2[i].x);
        pts[0].y = std::min(pts[0].y, ptsBox2[i].y);
        pts[2].y = std::max(pts[2].y, ptsBox2[i].y);
      }
      pts[1].x = pts[2].x;
      pts[1].y = pts[0].y;
      pts[3].x = pts[0].x;
      pts[3].y = pts[2].y;

      auto* Group = new EoDbGroup;

      for (int i = 0; i < 4; i++) Group->AddTail(new EoDbLine(1, 1, pts[i], pts[(i + 1) % 4]));

      pts.RemoveAll();

      auto* document = GetDocument();
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    }
    ModeLineUnhighlightOp(m_PreviousOp);
  }
}

void AeSysView::OnAnnotateModeCutIn() {
  CDC* DeviceContext = GetDC();

  auto cursorPosition = GetCursorPosition();

  auto* Group = SelectLineUsingPoint(cursorPosition);
  if (Group != 0) {
    auto* document = GetDocument();
    auto* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

    cursorPosition = DetPt();

    CString CurrentText;

    EoDlgSetText dlg;
    dlg.m_strTitle = L"Set Cut-in Text";
    dlg.m_sText = CurrentText;
    if (dlg.DoModal() == IDOK) { CurrentText = dlg.m_sText; }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);

    int PrimitiveState = pstate.Save();

    if (!CurrentText.IsEmpty()) {
      EoGeLine Line = pLine->Ln();
      double angle = Line.AngleFromXAxisXY();
      if (angle > 0.25 * Eo::TwoPi && angle < 0.75 * Eo::TwoPi) angle += Eo::Pi;

      auto cameraDirection = CameraDirection();
      auto minorAxis = ViewUp();
      minorAxis.RotAboutArbAx(cameraDirection, angle);
      auto majorAxis = CrossProduct(minorAxis, cameraDirection);
      majorAxis *= 0.06;
      minorAxis *= 0.1;
      EoGeReferenceSystem referenceSystem(cursorPosition, majorAxis, minorAxis);

      auto color = pstate.PenColor();
      pstate.SetColor(DeviceContext, 2);

      EoDbFontDefinition fd;
      pstate.GetFontDef(fd);
      fd.HorizontalAlignment(EoDb::kAlignCenter);
      fd.VerticalAlignment(EoDb::kAlignMiddle);

      EoDbCharacterCellDefinition ccd;
      pstate.GetCharCellDef(ccd);
      ccd.TextRotAngSet(0.0);
      pstate.SetCharCellDef(ccd);

      auto* text = new EoDbText(fd, referenceSystem, CurrentText);
      pstate.SetColor(DeviceContext, color);

      Group->AddTail(text);

      EoGePoint3dArray ptsBox;
      text->GetBoundingBox(ptsBox, GapSpaceFactor());

      double dGap = EoGeVector3d(ptsBox[0], ptsBox[1]).Length();

      ptsBox[0] = cursorPosition.ProjectToward(pLine->BeginPoint(), dGap / 2.0);
      ptsBox[1] = cursorPosition.ProjectToward(pLine->EndPoint(), dGap / 2.0);

      double dRel[2]{};

      dRel[0] = pLine->RelOfPt(ptsBox[0]);
      dRel[1] = pLine->RelOfPt(ptsBox[1]);

      if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {
        EoDbLine* NewLinePrimitive = new EoDbLine(*pLine);
        pLine->EndPoint(ptsBox[0]);
        NewLinePrimitive->BeginPoint(ptsBox[1]);
        Group->AddTail(NewLinePrimitive);
      } else if (dRel[0] <= Eo::geometricTolerance)
        pLine->BeginPoint(ptsBox[1]);
      else if (dRel[1] >= 1.0 - Eo::geometricTolerance)
        pLine->EndPoint(ptsBox[0]);
    }
    document->UpdateAllViews(nullptr, EoDb::kGroup, Group);
    pstate.Restore(DeviceContext, PrimitiveState);
  }
  ReleaseDC(DeviceContext);
}

void AeSysView::OnAnnotateModeConstructionLine() {
  auto cursorPosition = GetCursorPosition();

  if (m_PreviousOp != ID_OP9) {
    m_PreviousOp = ModeLineHighlightOp(ID_OP9);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    auto* document = GetDocument();
    cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
    pts.Add(pts[0].ProjectToward(cursorPosition, 48.0));
    pts.Add(pts[1].ProjectToward(pts[0], 96.0));

    auto* Group = new EoDbGroup(new EoDbLine(15, 2, pts[1], pts[2]));
    document->AddWorkLayerGroup(Group);
    ModeLineUnhighlightOp(m_PreviousOp);
    pts.RemoveAll();
    m_PreviewGroup.RemoveAll();
  }
}

void AeSysView::OnAnnotateModeReturn() {
  // TODO: Add your command handler code here
}

void AeSysView::OnAnnotateModeEscape() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(m_PreviousOp);
}

bool AeSysView::CorrectLeaderEndpoints(int beginType, int endType, EoGePoint3d& beginPoint, EoGePoint3d& endPoint) const {
  double LineSegmentLength = EoGeVector3d(beginPoint, endPoint).Length();

  double BeginDistance = 0.;

  if (beginType == ID_OP4) {
    BeginDistance = BubbleRadius();
  } else if (beginType == ID_OP5) {
    BeginDistance = CircleRadius();
  }
  double EndDistance = 0.;

  if (endType == ID_OP4) {
    EndDistance = BubbleRadius();
  } else if (endType == ID_OP5)
    EndDistance = CircleRadius();

  if (LineSegmentLength > BeginDistance + EndDistance + Eo::geometricTolerance) {
    if (BeginDistance != 0.0) beginPoint = beginPoint.ProjectToward(endPoint, BeginDistance);
    if (EndDistance != 0.0) endPoint = endPoint.ProjectToward(beginPoint, EndDistance);
    return true;
  } else {
    app.AddModeInformationToMessageList();
    return false;
  }
}
void AeSysView::DoAnnotateModeMouseMove() {
  auto cursorPosition = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();
  pts.Add(cursorPosition);
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  switch (m_PreviousOp) {
    case ID_OP2:
    case ID_OP3:
      if (pts[0] != cursorPosition) {
        if (m_PreviousOp == ID_OP3) GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, pts[0], &m_PreviewGroup);
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP4:
    case ID_OP5: {
      m_PreviousPnt = pts[0];
      if (CorrectLeaderEndpoints(m_PreviousOp, 0, pts[0], pts[1])) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      pts[0] = m_PreviousPnt;
      break;
    }
    case ID_OP9:
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

        pts.Add(pts[0].ProjectToward(cursorPosition, 48.0));
        pts.Add(pts[2].ProjectToward(pts[0], 96.0));

        m_PreviewGroup.AddTail(new EoDbGroup(new EoDbLine(15, 2, pts[2], pts[3])));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;
  }
  pts.SetSize(NumberOfPoints);
}
void AeSysView::GenerateLineEndItem(int type, double size, EoGePoint3d& beginPoint, EoGePoint3d& endPoint, EoDbGroup* group) const {
  EoGeVector3d PlaneNormal = CameraDirection();

  EoGePoint3dArray ItemPoints;

  if (type == 1 || type == 2) {
    double dAng = 0.244978663127;
    double dLen = size / 0.970142500145;

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, dLen));
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, dAng));
    ItemPoints.Add(endPoint);
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, -dAng));
    EoDbPolyline* Primitive = new EoDbPolyline(1, 1, ItemPoints);
    if (type == 2) Primitive->SetFlag(0x0010);
    group->AddTail(Primitive);
  } else if (type == 3) {
    double dAng = 9.96686524912e-2;
    double dLen = size / 0.99503719021;

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, dLen));
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, dAng));
    ItemPoints.Add(endPoint);

    group->AddTail(new EoDbPolyline(1, 1, ItemPoints));
  } else if (type == 4) {
    double dAng = 0.785398163397;
    double dLen = 0.5 * size / 0.707106781187;

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, dLen));
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, dAng));
    ItemPoints.Add(ItemPoints[0].RotateAboutAxis(endPoint, PlaneNormal, Eo::Pi));
    group->AddTail(new EoDbPolyline(1, 1, ItemPoints));
  }
}
