#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbSpline.h"
#include "EoDlgBlockInsert.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "PrimState.h"
#include "Resource.h"

EoUInt16 PreviousDrawCommand{};

void AeSysView::OnDrawModeOptions() { AeSysDoc::GetDoc()->OnSetupOptionsDraw(); }

void AeSysView::OnDrawModePoint() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  auto* Group = new EoDbGroup(new EoDbPoint(pstate.PenColor(), pstate.PointStyle(), CurrentPnt));
  auto* document = GetDocument();
  document->AddWorkLayerGroup(Group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
}

void AeSysView::OnDrawModeLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (PreviousDrawCommand != ID_OP2) {
    pts.RemoveAll();
    pts.Add(CurrentPnt);
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP2);
  } else {
    auto* document = GetDocument();
    CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);

    auto* Group = new EoDbGroup(new EoDbLine(pts[0], CurrentPnt));
    document->AddWorkLayerGroup(Group);
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
  auto* document = GetDocument();
  if (document->BlockTableSize() > 0) {
    EoDlgBlockInsert Dialog(document);
    Dialog.DoModal();
  }
}

void AeSysView::OnDrawModeReturn() {
  auto* document = GetDocument();
  auto CurrentPnt = GetCursorPosition();

  INT_PTR NumberOfPoints = pts.GetSize();
  EoDbGroup* Group{};

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

      if (NumberOfPoints == 1) { return; }

      EoDbConic* conic = new EoDbConic(pts[0], pts[1], pts[2]);
      if (conic->SweepAngle() == 0.0) {
        delete conic;
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;
      }
      Group = new EoDbGroup(conic);
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
      Group = new EoDbGroup(new EoDbConic(pts[0], CurrentPnt));
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
      EoGeVector3d majorAxis(pts[0], pts[1]);
      EoGeVector3d minorAxis(pts[0], pts[2]);

      if (minorAxis.Length() < Eo::geometricTolerance) { break; }
      auto extrusion = CrossProduct(majorAxis, minorAxis);
      extrusion.Normalize();
      double ratio = minorAxis.Length() / majorAxis.Length();
      auto* conic = new EoDbConic(pts[0], extrusion, majorAxis, ratio);
      conic->SetColor(pstate.PenColor());
      conic->SetLineTypeIndex(pstate.LineType());

      Group = new EoDbGroup(conic);
      
      break;
    }
    default:
      return;
  }
  document->AddWorkLayerGroup(Group);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(PreviousDrawCommand);
}

void AeSysView::OnDrawModeEscape() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(PreviousDrawCommand);
}

void AeSysView::OnDrawModeShiftReturn() {
  if (PreviousDrawCommand == ID_OP3) {
    auto* Group = new EoDbGroup(new EoDbPolyline(pts));
    auto* document = GetDocument();
    document->AddWorkLayerGroup(Group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
  }
  ModeLineUnhighlightOp(PreviousDrawCommand);
  pts.RemoveAll();
}
void AeSysView::DoDrawModeMouseMove() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();

  auto* document = GetDocument();
  switch (PreviousDrawCommand) {
    case ID_OP2:
      VERIFY(pts.GetSize() > 0);

      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP3:
      CurrentPnt = SnapPointToAxis(pts[NumberOfPoints - 1], CurrentPnt);
      pts.Add(CurrentPnt);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      if (NumberOfPoints == 1) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      } else {
        m_PreviewGroup.AddTail(new EoDbPolygon(pts));
      }
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP4:
      CurrentPnt = SnapPointToAxis(pts[NumberOfPoints - 1], CurrentPnt);
      pts.Add(CurrentPnt);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      if (NumberOfPoints == 2) {
        pts.Add(pts[0] + EoGeVector3d(pts[1], CurrentPnt));
        pts.Add(pts[0]);
      }
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP5:
      pts.Add(CurrentPnt);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      if (NumberOfPoints == 1) { m_PreviewGroup.AddTail(new EoDbPolyline(pts)); }
      if (NumberOfPoints == 2) { m_PreviewGroup.AddTail(new EoDbConic(pts[0], pts[1], CurrentPnt)); }
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP6:
      pts.Add(CurrentPnt);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      m_PreviewGroup.AddTail(new EoDbSpline(pts));
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP7:
      if (pts[0] != CurrentPnt) {
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        m_PreviewGroup.AddTail(new EoDbConic(pts[0], CurrentPnt));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP8:
      pts.Add(CurrentPnt);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      if (NumberOfPoints == 1) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      } else {
        EoGeVector3d majorAxis(pts[0], pts[1]);
        EoGeVector3d minorAxis(pts[0], CurrentPnt);
        
        if (minorAxis.Length() < Eo::geometricTolerance) { break; }
        auto extrusion = CrossProduct(majorAxis, minorAxis);
        extrusion.Normalize();
        double ratio = minorAxis.Length() / majorAxis.Length();
        auto* conic = new EoDbConic(pts[0], extrusion, majorAxis, ratio);
        conic->SetColor(pstate.PenColor());
        conic->SetLineTypeIndex(pstate.LineType());

        m_PreviewGroup.AddTail(conic);
      }
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;
  }
  pts.SetSize(NumberOfPoints);
}
