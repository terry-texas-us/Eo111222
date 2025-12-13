#include "stdafx.h"
#include "AeSys.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDlgBlockInsert.h"

EoUInt16 PreviousDrawCommand = 0;

void AeSysView::OnDrawModeOptions() { AeSysDoc::GetDoc()->OnSetupOptionsDraw(); }

void AeSysView::OnDrawModePoint() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  EoDbGroup* Group = new EoDbGroup(new EoDbPoint(pstate.PenColor(), pstate.PointStyle(), CurrentPnt));
  GetDocument()->AddWorkLayerGroup(Group);
  GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
}

void AeSysView::OnDrawModeLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (PreviousDrawCommand != ID_OP2) {
    pts.RemoveAll();
    pts.Add(CurrentPnt);
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP2);
  } else {
    CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);

    EoDbGroup* Group = new EoDbGroup(new EoDbLine(pts[0], CurrentPnt));
    GetDocument()->AddWorkLayerGroup(Group);
    pts[0] = CurrentPnt;
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
}

void AeSysView::OnDrawModePolygon() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP3) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP3);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    INT_PTR NumberOfPoints = pts.GetSize();

    if (pts[NumberOfPoints - 1] != CurrentPnt) {
      CurrentPnt = SnapPointToAxis(pts[NumberOfPoints - 1], CurrentPnt);
      pts.Add(CurrentPnt);
    }
  }
}

void AeSysView::OnDrawModeQuad() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP4) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP4);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeArc() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP5) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP5);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeBspline() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP6) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP6);

    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    pts.Add(CurrentPnt);
  }
}

void AeSysView::OnDrawModeCircle() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP7) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP7);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeEllipse() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP8) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP8);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeInsert() {
  if (GetDocument()->BlockTableSize() > 0) {
    EoDlgBlockInsert Dialog;
    Dialog.DoModal();
  }
}

void AeSysView::OnDrawModeReturn() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  INT_PTR NumberOfPoints = pts.GetSize();
  EoDbGroup* Group = 0;

  switch (PreviousDrawCommand) {
    case ID_OP2:
      CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
      Group = new EoDbGroup(new EoDbLine(pts[0], CurrentPnt));
      break;

    case ID_OP3:
      if (NumberOfPoints == 1) return;

      if (pts[NumberOfPoints - 1] == CurrentPnt) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      CurrentPnt = SnapPointToAxis(pts[NumberOfPoints - 1], CurrentPnt);
      pts.Add(CurrentPnt);
      Group = new EoDbGroup(new EoDbPolygon(pts));
      break;

    case ID_OP4:
      if (pts[NumberOfPoints - 1] == CurrentPnt) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      CurrentPnt = SnapPointToAxis(pts[NumberOfPoints - 1], CurrentPnt);
      pts.Add(CurrentPnt);

      if (NumberOfPoints == 1) return;

      pts.Add(pts[0] + EoGeVector3d(pts[1], pts[2]));

      Group = new EoDbGroup;

      for (int i = 0; i < 4; i++) Group->AddTail(new EoDbLine(pts[i], pts[(i + 1) % 4]));

      break;

    case ID_OP5: {
      if (pts[NumberOfPoints - 1] == CurrentPnt) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      pts.Add(CurrentPnt);

      if (NumberOfPoints == 1) return;

      EoDbEllipse* pArc = new EoDbEllipse(pts[0], pts[1], pts[2]);
      if (pArc->GetSwpAng() == 0.0) {
        delete pArc;
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;
      }
      Group = new EoDbGroup(pArc);
      break;
    }
    case ID_OP6:
      if (NumberOfPoints == 1) return;

      pts.Add(CurrentPnt);

      Group = new EoDbGroup(new EoDbSpline(pts));
      break;

    case ID_OP7:
      if (pts[NumberOfPoints - 1] == CurrentPnt) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      Group = new EoDbGroup(new EoDbEllipse(pts[0], CurrentPnt));
      break;

    case ID_OP8: {
      if (pts[NumberOfPoints - 1] == CurrentPnt) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      pts.Add(CurrentPnt);
      if (NumberOfPoints == 1) {
        SetCursorPosition(pts[0]);
        return;
      }
      EoGeVector3d MajorAxis(pts[0], pts[1]);
      EoGeVector3d MinorAxis(pts[0], pts[2]);

      Group = new EoDbGroup(new EoDbEllipse(pts[0], MajorAxis, MinorAxis, TWOPI));
      break;
    }
    default:
      return;
  }
  GetDocument()->AddWorkLayerGroup(Group);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(PreviousDrawCommand);
}

void AeSysView::OnDrawModeEscape() {
  GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(PreviousDrawCommand);
}

void AeSysView::OnDrawModeShiftReturn() {
  if (PreviousDrawCommand == ID_OP3) {
    EoDbGroup* Group = new EoDbGroup(new EoDbPolyline(pts));
    GetDocument()->AddWorkLayerGroup(Group);
    GetDocument()->UpdateAllViews(NULL, EoDb::kGroupSafe, Group);
  }
  ModeLineUnhighlightOp(PreviousDrawCommand);
  pts.RemoveAll();
}
void AeSysView::DoDrawModeMouseMove() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();

  switch (PreviousDrawCommand) {
    case ID_OP2:
      VERIFY(pts.GetSize() > 0);

      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
        GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP3:
      CurrentPnt = SnapPointToAxis(pts[NumberOfPoints - 1], CurrentPnt);
      pts.Add(CurrentPnt);

      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      if (NumberOfPoints == 1) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      } else {
        m_PreviewGroup.AddTail(new EoDbPolygon(pts));
      }
      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP4:
      CurrentPnt = SnapPointToAxis(pts[NumberOfPoints - 1], CurrentPnt);
      pts.Add(CurrentPnt);

      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      if (NumberOfPoints == 2) {
        pts.Add(pts[0] + EoGeVector3d(pts[1], CurrentPnt));
        pts.Add(pts[0]);
      }
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP5:
      pts.Add(CurrentPnt);

      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      if (NumberOfPoints == 1) { m_PreviewGroup.AddTail(new EoDbPolyline(pts)); }
      if (NumberOfPoints == 2) { m_PreviewGroup.AddTail(new EoDbEllipse(pts[0], pts[1], CurrentPnt)); }
      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP6:
      pts.Add(CurrentPnt);

      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);

      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      m_PreviewGroup.AddTail(new EoDbSpline(pts));
      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP7:
      if (pts[0] != CurrentPnt) {
        GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        m_PreviewGroup.AddTail(new EoDbEllipse(pts[0], CurrentPnt));
        GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP8:
      pts.Add(CurrentPnt);

      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      if (NumberOfPoints == 1) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      } else {
        EoGeVector3d MajorAxis(pts[0], pts[1]);
        EoGeVector3d MinorAxis(pts[0], CurrentPnt);

        m_PreviewGroup.AddTail(new EoDbEllipse(pts[0], MajorAxis, MinorAxis, TWOPI));
      }
      GetDocument()->UpdateAllViews(NULL, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;
  }
  pts.SetSize(NumberOfPoints);
}
