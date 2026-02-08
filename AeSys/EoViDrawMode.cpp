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
  auto cursorPosition = GetCursorPosition();

  auto* Group = new EoDbGroup(new EoDbPoint(pstate.Color(), pstate.PointStyle(), cursorPosition));
  auto* document = GetDocument();
  document->AddWorkLayerGroup(Group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
}

void AeSysView::OnDrawModeLine() {
  auto cursorPosition = GetCursorPosition();
  if (PreviousDrawCommand != ID_OP2) {
    pts.RemoveAll();
    pts.Add(cursorPosition);
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP2);
  } else {
    auto* document = GetDocument();
    cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

    auto* Group = new EoDbGroup(new EoDbLine(pts[0], cursorPosition));
    document->AddWorkLayerGroup(Group);
    pts[0] = cursorPosition;
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
}

void AeSysView::OnDrawModePolygon() {
  auto cursorPosition = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP3) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP3);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    INT_PTR NumberOfPoints = pts.GetSize();

    if (pts[NumberOfPoints - 1] != cursorPosition) {
      cursorPosition = SnapPointToAxis(pts[NumberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);
    }
  }
}

void AeSysView::OnDrawModeQuad() {
  auto cursorPosition = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP4) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP4);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeArc() {
  auto cursorPosition = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP5) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP5);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeBspline() {
  auto cursorPosition = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP6) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP6);

    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    pts.Add(cursorPosition);
  }
}

void AeSysView::OnDrawModeCircle() {
  auto cursorPosition = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP7) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP7);
    pts.RemoveAll();
    pts.Add(cursorPosition);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeEllipse() {
  auto cursorPosition = GetCursorPosition();

  if (PreviousDrawCommand != ID_OP8) {
    PreviousDrawCommand = ModeLineHighlightOp(ID_OP8);
    pts.RemoveAll();
    pts.Add(cursorPosition);
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
  auto cursorPosition = GetCursorPosition();

  INT_PTR NumberOfPoints = pts.GetSize();
  EoDbGroup* group{};

  switch (PreviousDrawCommand) {
    case ID_OP2:
      cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
      group = new EoDbGroup(new EoDbLine(pts[0], cursorPosition));
      break;

    case ID_OP3:
      if (NumberOfPoints == 1) return;

      if (pts[NumberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      cursorPosition = SnapPointToAxis(pts[NumberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);
      group = new EoDbGroup(new EoDbPolygon(pts));
      break;

    case ID_OP4:
      if (pts[NumberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      cursorPosition = SnapPointToAxis(pts[NumberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);

      if (NumberOfPoints == 1) return;

      pts.Add(pts[0] + EoGeVector3d(pts[1], pts[2]));

      group = new EoDbGroup;

      for (int i = 0; i < 4; i++) { group->AddTail(new EoDbLine(pts[i], pts[(i + 1) % 4])); }

      break;

    case ID_OP5: {
      if (pts[NumberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      pts.Add(cursorPosition);

      if (NumberOfPoints == 1) { return; }
      EoGePoint3d start{pts[0]};
      EoGePoint3d intermediate{pts[1]};
      EoGePoint3d end{pts[2]};
      auto* radialArc = EoDbConic::CreateRadialArcFrom3Points(start, intermediate, end);
      if (radialArc == nullptr) {
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;
      }
      radialArc->SetColor(pstate.Color());
      radialArc->SetLineTypeIndex(pstate.LineType());

      if (fabs(radialArc->SweepAngle()) < Eo::geometricTolerance) {
        delete radialArc;
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;
      }
      group = new EoDbGroup(radialArc);
      break;
    }
    case ID_OP6:
      if (NumberOfPoints == 1) return;

      pts.Add(cursorPosition);

      group = new EoDbGroup(new EoDbSpline(pts));
      break;

    case ID_OP7: {
      double radius = EoGePoint3d::Distance(pts[0], cursorPosition);
      if (radius < Eo::geometricTolerance) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      group = new EoDbGroup(EoDbConic::CreateCircleInView(pts[0], radius));
    } break;

    case ID_OP8: {
      if (pts[NumberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      pts.Add(cursorPosition);
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
      auto* ellipse = EoDbConic::CreateEllipse(pts[0], extrusion, majorAxis, ratio);
      ellipse->SetColor(pstate.Color());
      ellipse->SetLineTypeIndex(pstate.LineType());

      group = new EoDbGroup(ellipse);

      break;
    }
    default:
      return;
  }
  document->AddWorkLayerGroup(group);
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
  EoGePoint3d cursorPosition = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();

  auto* document = GetDocument();
  switch (PreviousDrawCommand) {
    case ID_OP2:
      VERIFY(pts.GetSize() > 0);

      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP3:
      cursorPosition = SnapPointToAxis(pts[NumberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);

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
      cursorPosition = SnapPointToAxis(pts[NumberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      if (NumberOfPoints == 2) {
        pts.Add(pts[0] + EoGeVector3d(pts[1], cursorPosition));
        pts.Add(pts[0]);
      }
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP5:
      pts.Add(cursorPosition);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();

      if (NumberOfPoints == 1) { m_PreviewGroup.AddTail(new EoDbPolyline(pts)); }
      if (NumberOfPoints == 2) { 
        EoGePoint3d start{pts[0]};
        EoGePoint3d intermediate{pts[1]};
        auto radialArc = EoDbConic::CreateRadialArcFrom3Points(start, intermediate, cursorPosition);
        if (radialArc == nullptr) { break; }

        radialArc->SetColor(pstate.Color());
        radialArc->SetLineTypeIndex(pstate.LineType());
        m_PreviewGroup.AddTail(radialArc);
      }
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP6:
      pts.Add(cursorPosition);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      m_PreviewGroup.AddTail(new EoDbSpline(pts));
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP7: {
      double radius = EoGePoint3d::Distance(pts[0], cursorPosition);
      if (radius > Eo::geometricTolerance) {
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        m_PreviewGroup.AddTail(EoDbConic::CreateCircleInView(pts[0], radius));
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
    } break;

    case ID_OP8:
      pts.Add(cursorPosition);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      if (NumberOfPoints == 1) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      } else {
        EoGeVector3d majorAxis(pts[0], pts[1]);
        EoGeVector3d minorAxis(pts[0], cursorPosition);

        if (minorAxis.Length() < Eo::geometricTolerance) { break; }
        auto extrusion = CrossProduct(majorAxis, minorAxis);
        extrusion.Normalize();
        double ratio = minorAxis.Length() / majorAxis.Length();
        auto* ellipse = EoDbConic::CreateEllipse(pts[0], extrusion, majorAxis, ratio);
        ellipse->SetColor(pstate.Color());
        ellipse->SetLineTypeIndex(pstate.LineType());

        m_PreviewGroup.AddTail(ellipse);
      }
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;
  }
  pts.SetSize(NumberOfPoints);
}
