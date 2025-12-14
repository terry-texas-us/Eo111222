#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDlgAnnotateOptions.h"
#include "EoDlgSetText.h"

void AeSysView::OnAnnotateModeOptions() {
  EoDlgAnnotateOptions Dialog(this);

  if (Dialog.DoModal() == IDOK) {}
}

void AeSysView::OnAnnotateModeLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (m_PreviousOp == 0) {
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP2, pts[0], CurrentPnt)) {
      EoDbGroup* Group = new EoDbGroup;
      GetDocument()->AddWorkLayerGroup(Group);

      if (m_PreviousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), CurrentPnt, pts[0], Group); }
      Group->AddTail(new EoDbLine(1, 1, pts[0], CurrentPnt));
      pts[0] = CurrentPnt;
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP2);
}

void AeSysView::OnAnnotateModeArrow() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (m_PreviousOp == 0) {
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP3, pts[0], CurrentPnt)) {
      EoDbGroup* Group = new EoDbGroup;

      if (m_PreviousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), CurrentPnt, pts[0], Group); }
      Group->AddTail(new EoDbLine(1, 1, pts[0], CurrentPnt));
      GenerateLineEndItem(EndItemType(), EndItemSize(), pts[0], CurrentPnt, Group);
      GetDocument()->AddWorkLayerGroup(Group);
      GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      pts[0] = CurrentPnt;
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP3);
}

void AeSysView::OnAnnotateModeBubble() {
  static CString CurrentText;
  EoGePoint3d CurrentPnt = GetCursorPosition();

  EoDlgSetText dlg;
  dlg.m_strTitle = L"Set Bubble Text";
  dlg.m_sText = CurrentText;
  if (dlg.DoModal() == IDOK) { CurrentText = dlg.m_sText; }
  EoDbGroup* Group = new EoDbGroup;
  GetDocument()->AddWorkLayerGroup(Group);
  if (m_PreviousOp == 0) {  // No operation pending
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    EoGePoint3d pt(CurrentPnt);

    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP4, pts[0], pt)) {
      if (m_PreviousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), CurrentPnt, pts[0], Group); }
      Group->AddTail(new EoDbLine(1, 1, pts[0], pt));
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP4);

  if (!CurrentText.IsEmpty()) {
    CDC* DeviceContext = GetDC();

    EoGeVector3d PlaneNormal = CameraDirection();
    EoGeVector3d MinorAxis = ViewUp();
    EoGeVector3d MajorAxis = MinorAxis;
    MajorAxis.RotAboutArbAx(PlaneNormal, -HALF_PI);

    MajorAxis *= .06;
    MinorAxis *= .1;
    EoGeReferenceSystem ReferenceSystem(CurrentPnt, MajorAxis, MinorAxis);

    int PrimitiveState = pstate.Save();
    pstate.SetPenColor(DeviceContext, 2);

    EoDbFontDefinition fd;
    pstate.GetFontDef(fd);
    fd.HorizontalAlignment(EoDb::kAlignCenter);
    fd.VerticalAlignment(EoDb::kAlignMiddle);

    EoDbCharacterCellDefinition ccd;
    pstate.GetCharCellDef(ccd);
    ccd.TextRotAngSet(0.0);
    pstate.SetCharCellDef(ccd);

    Group->AddTail(new EoDbText(fd, ReferenceSystem, CurrentText));
    pstate.Restore(DeviceContext, PrimitiveState);
    ReleaseDC(DeviceContext);
  }
  if (NumberOfSides() == 0) {
    Group->AddTail(new EoDbEllipse(1, 1, CurrentPnt, BubbleRadius()));
  } else {
    Group->AddTail(new EoDbPolyline(1, 1, CurrentPnt, BubbleRadius(), NumberOfSides()));
  }
  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  pts[0] = CurrentPnt;
}

void AeSysView::OnAnnotateModeHook() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  EoDbGroup* Group = new EoDbGroup;
  if (m_PreviousOp == 0) {
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();

    EoGePoint3d pt(CurrentPnt);

    if (CorrectLeaderEndpoints(m_PreviousOp, ID_OP5, pts[0], pt)) {
      if (m_PreviousOp == ID_OP3) GenerateLineEndItem(EndItemType(), EndItemSize(), CurrentPnt, pts[0], Group);

      Group->AddTail(new EoDbLine(1, 1, pts[0], pt));
    }
  }
  m_PreviousOp = ModeLineHighlightOp(ID_OP5);
  Group->AddTail(new EoDbEllipse(1, 1, CurrentPnt, CircleRadius()));
  GetDocument()->AddWorkLayerGroup(Group);
  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  pts[0] = CurrentPnt;
}

void AeSysView::OnAnnotateModeUnderline() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  pts.RemoveAll();

  if (m_PreviousOp != 0) {
    ModeLineUnhighlightOp(m_PreviousOp);
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
  EoDbText* pText = SelectTextUsingPoint(CurrentPnt);
  if (pText != 0) {
    pText->GetBoundingBox(pts, GapSpaceFactor());

    EoDbGroup* Group = new EoDbGroup;
    Group->AddTail(new EoDbLine(pstate.PenColor(), 1, pts[0], pts[1]));
    GetDocument()->AddWorkLayerGroup(Group);
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

    pts.RemoveAll();
  }
}

void AeSysView::OnAnnotateModeBox() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (m_PreviousOp != ID_OP7) {
    if (m_PreviousOp != 0) {
      RubberBandingDisable();
      ModeLineUnhighlightOp(m_PreviousOp);
    }
    m_PreviousOp = ModeLineHighlightOp(ID_OP7);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
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
    pText = SelectTextUsingPoint(CurrentPnt);
    if (pText != 0) {
      pText->GetBoundingBox(ptsBox2, GapSpaceFactor());
      bG2Flg = true;
    }
    if (bG1Flg && bG2Flg) {
      pts.SetSize(4);

      pts[0] = ptsBox1[0];
      pts[2] = ptsBox1[0];
      for (int i = 1; i < 4; i++) {
        pts[0].x = EoMin(pts[0].x, ptsBox1[i].x);
        pts[2].x = EoMax(pts[2].x, ptsBox1[i].x);
        pts[0].y = EoMin(pts[0].y, ptsBox1[i].y);
        pts[2].y = EoMax(pts[2].y, ptsBox1[i].y);
      }
      for (int i = 0; i < 4; i++) {
        pts[0].x = EoMin(pts[0].x, ptsBox2[i].x);
        pts[2].x = EoMax(pts[2].x, ptsBox2[i].x);
        pts[0].y = EoMin(pts[0].y, ptsBox2[i].y);
        pts[2].y = EoMax(pts[2].y, ptsBox2[i].y);
      }
      pts[1].x = pts[2].x;
      pts[1].y = pts[0].y;
      pts[3].x = pts[0].x;
      pts[3].y = pts[2].y;

      EoDbGroup* Group = new EoDbGroup;

      for (int i = 0; i < 4; i++) Group->AddTail(new EoDbLine(1, 1, pts[i], pts[(i + 1) % 4]));

      pts.RemoveAll();

      GetDocument()->AddWorkLayerGroup(Group);
      GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    }
    ModeLineUnhighlightOp(m_PreviousOp);
  }
}

void AeSysView::OnAnnotateModeCutIn() {
  CDC* DeviceContext = GetDC();

  EoGePoint3d CurrentPnt = GetCursorPosition();

  EoDbGroup* Group = SelectLineUsingPoint(CurrentPnt);
  if (Group != 0) {
    EoDbLine* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

    CurrentPnt = DetPt();

    CString CurrentText;

    EoDlgSetText dlg;
    dlg.m_strTitle = L"Set Cut-in Text";
    dlg.m_sText = CurrentText;
    if (dlg.DoModal() == IDOK) { CurrentText = dlg.m_sText; }
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);

    int PrimitiveState = pstate.Save();

    if (!CurrentText.IsEmpty()) {
      EoGeLine Line = pLine->Ln();
      double dAng = Line.AngleFromXAxisXY();
      if (dAng > .25 * TWOPI && dAng < .75 * TWOPI) dAng += PI;

      EoGeVector3d PlaneNormal = CameraDirection();
      EoGeVector3d MinorAxis = ViewUp();
      MinorAxis.RotAboutArbAx(PlaneNormal, dAng);
      EoGeVector3d MajorAxis = MinorAxis;
      MajorAxis.RotAboutArbAx(PlaneNormal, -HALF_PI);
      MajorAxis *= .06;
      MinorAxis *= .1;
      EoGeReferenceSystem ReferenceSystem(CurrentPnt, MajorAxis, MinorAxis);

      EoInt16 nPenColor = pstate.PenColor();
      pstate.SetPenColor(DeviceContext, 2);

      EoDbFontDefinition fd;
      pstate.GetFontDef(fd);
      fd.HorizontalAlignment(EoDb::kAlignCenter);
      fd.VerticalAlignment(EoDb::kAlignMiddle);

      EoDbCharacterCellDefinition ccd;
      pstate.GetCharCellDef(ccd);
      ccd.TextRotAngSet(0.0);
      pstate.SetCharCellDef(ccd);

      EoDbText* TextPrimitive = new EoDbText(fd, ReferenceSystem, CurrentText);
      pstate.SetPenColor(DeviceContext, nPenColor);

      Group->AddTail(TextPrimitive);

      EoGePoint3dArray ptsBox;
      TextPrimitive->GetBoundingBox(ptsBox, GapSpaceFactor());

      double dGap = EoGeVector3d(ptsBox[0], ptsBox[1]).Length();

      ptsBox[0] = CurrentPnt.ProjectToward(pLine->BeginPoint(), dGap / 2.);
      ptsBox[1] = CurrentPnt.ProjectToward(pLine->EndPoint(), dGap / 2.);

      double dRel[2];

      dRel[0] = pLine->RelOfPt(ptsBox[0]);
      dRel[1] = pLine->RelOfPt(ptsBox[1]);

      if (dRel[0] > DBL_EPSILON && dRel[1] < 1. - DBL_EPSILON) {
        EoDbLine* NewLinePrimitive = new EoDbLine(*pLine);
        pLine->EndPoint(ptsBox[0]);
        NewLinePrimitive->BeginPoint(ptsBox[1]);
        Group->AddTail(NewLinePrimitive);
      } else if (dRel[0] <= DBL_EPSILON)
        pLine->BeginPoint(ptsBox[1]);
      else if (dRel[1] >= 1. - DBL_EPSILON)
        pLine->EndPoint(ptsBox[0]);
    }
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroup, Group);
    pstate.Restore(DeviceContext, PrimitiveState);
  }
  ReleaseDC(DeviceContext);
}

void AeSysView::OnAnnotateModeConstructionLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (m_PreviousOp != ID_OP9) {
    m_PreviousOp = ModeLineHighlightOp(ID_OP9);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
    pts.Add(pts[0].ProjectToward(CurrentPnt, 48.));
    pts.Add(pts[1].ProjectToward(pts[0], 96.));

    EoDbGroup* Group = new EoDbGroup(new EoDbLine(15, 2, pts[1], pts[2]));
    GetDocument()->AddWorkLayerGroup(Group);
    ModeLineUnhighlightOp(m_PreviousOp);
    pts.RemoveAll();
    m_PreviewGroup.RemoveAll();
  }
}

void AeSysView::OnAnnotateModeReturn() {
  // TODO: Add your command handler code here
}

void AeSysView::OnAnnotateModeEscape() {
  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(m_PreviousOp);
}

bool AeSysView::CorrectLeaderEndpoints(int beginType, int endType, EoGePoint3d& beginPoint, EoGePoint3d& endPoint) {
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

  if (LineSegmentLength > BeginDistance + EndDistance + DBL_EPSILON) {
    if (BeginDistance != 0.0) beginPoint = beginPoint.ProjectToward(endPoint, BeginDistance);
    if (EndDistance != 0.0) endPoint = endPoint.ProjectToward(beginPoint, EndDistance);
    return true;
  } else {
    app.AddModeInformationToMessageList();
    return false;
  }
}
void AeSysView::DoAnnotateModeMouseMove() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();
  pts.Add(CurrentPnt);

  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  switch (m_PreviousOp) {
    case ID_OP2:
    case ID_OP3:
      if (pts[0] != CurrentPnt) {
        if (m_PreviousOp == ID_OP3)
          GenerateLineEndItem(EndItemType(), EndItemSize(), CurrentPnt, pts[0], &m_PreviewGroup);
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
        GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP4:
    case ID_OP5: {
      m_PreviousPnt = pts[0];
      if (CorrectLeaderEndpoints(m_PreviousOp, 0, pts[0], pts[1])) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
        GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      pts[0] = m_PreviousPnt;
      break;
    }
    case ID_OP9:
      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);

        pts.Add(pts[0].ProjectToward(CurrentPnt, 48.));
        pts.Add(pts[2].ProjectToward(pts[0], 96.));

        m_PreviewGroup.AddTail(new EoDbGroup(new EoDbLine(15, 2, pts[2], pts[3])));
        GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;
  }
  pts.SetSize(NumberOfPoints);
}
void AeSysView::GenerateLineEndItem(int type, double size, EoGePoint3d& beginPoint, EoGePoint3d& endPoint,
                                    EoDbGroup* group) {
  EoGeVector3d PlaneNormal = CameraDirection();

  EoGePoint3dArray ItemPoints;

  if (type == 1 || type == 2) {
    double dAng = .244978663127;
    double dLen = size / .970142500145;

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, dLen));
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, dAng));
    ItemPoints.Add(endPoint);
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, -dAng));
    EoDbPolyline* Primitive = new EoDbPolyline(1, 1, ItemPoints);
    if (type == 2) Primitive->SetFlag(0x0010);
    group->AddTail(Primitive);
  } else if (type == 3) {
    double dAng = 9.96686524912e-2;
    double dLen = size / .99503719021;

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, dLen));
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, dAng));
    ItemPoints.Add(endPoint);

    group->AddTail(new EoDbPolyline(1, 1, ItemPoints));
  } else if (type == 4) {
    double dAng = .785398163397;
    double dLen = .5 * size / .707106781187;

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, dLen));
    ItemPoints.Add(pt.RotateAboutAxis(endPoint, PlaneNormal, dAng));
    ItemPoints.Add(ItemPoints[0].RotateAboutAxis(endPoint, PlaneNormal, PI));
    group->AddTail(new EoDbPolyline(1, 1, ItemPoints));
  }
}
