#include "Stdafx.h"

#include <cmath>
#include <cstdint>

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
#include "EoGsRenderState.h"
#include "Resource.h"

namespace {
std::uint16_t previousDrawCommand{};
}
void AeSysView::OnDrawModeOptions() { AeSysDoc::GetDoc()->OnSetupOptionsDraw(); }

void AeSysView::OnDrawModePoint() {
  auto cursorPosition = GetCursorPosition();

  auto* group = new EoDbGroup(new EoDbPoint(renderState.Color(), renderState.PointStyle(), cursorPosition));
  auto* document = GetDocument();
  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
}

void AeSysView::OnDrawModeLine() {
  auto cursorPosition = GetCursorPosition();
  if (previousDrawCommand != ID_OP2) {
    pts.RemoveAll();
    pts.Add(cursorPosition);
    previousDrawCommand = ModeLineHighlightOp(ID_OP2);
  } else {
    auto* document = GetDocument();
    cursorPosition = SnapPointToAxis(pts[0], cursorPosition);

    auto* group = new EoDbGroup(
        EoDbLine::CreateLine(pts[0], cursorPosition)->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
    document->AddWorkLayerGroup(group);
    pts[0] = cursorPosition;
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  }
}

void AeSysView::OnDrawModePolygon() {
  auto cursorPosition = GetCursorPosition();

  if (previousDrawCommand != ID_OP3) {
    previousDrawCommand = ModeLineHighlightOp(ID_OP3);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    auto numberOfPoints = pts.GetSize();

    if (pts[numberOfPoints - 1] != cursorPosition) {
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);
    }
  }
}

void AeSysView::OnDrawModeQuad() {
  auto cursorPosition = GetCursorPosition();

  if (previousDrawCommand != ID_OP4) {
    previousDrawCommand = ModeLineHighlightOp(ID_OP4);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeArc() {
  auto cursorPosition = GetCursorPosition();

  if (previousDrawCommand != ID_OP5) {
    previousDrawCommand = ModeLineHighlightOp(ID_OP5);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeBspline() {
  auto cursorPosition = GetCursorPosition();

  if (previousDrawCommand != ID_OP6) {
    previousDrawCommand = ModeLineHighlightOp(ID_OP6);

    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    pts.Add(cursorPosition);
  }
}

void AeSysView::OnDrawModeCircle() {
  auto cursorPosition = GetCursorPosition();

  if (previousDrawCommand != ID_OP7) {
    previousDrawCommand = ModeLineHighlightOp(ID_OP7);
    pts.RemoveAll();
    pts.Add(cursorPosition);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeEllipse() {
  auto cursorPosition = GetCursorPosition();

  if (previousDrawCommand != ID_OP8) {
    previousDrawCommand = ModeLineHighlightOp(ID_OP8);
    pts.RemoveAll();
    pts.Add(cursorPosition);
  } else {
    OnDrawModeReturn();
  }
}

void AeSysView::OnDrawModeInsert() {
  auto* document = GetDocument();
  if (document->BlockTableSize() > 0) {
    EoDlgBlockInsert dialog(document);
    dialog.DoModal();
  }
}

void AeSysView::OnDrawModeReturn() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();

  auto numberOfPoints = pts.GetSize();
  EoDbGroup* group{};

  switch (previousDrawCommand) {
    case ID_OP2:
      cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
      group = new EoDbGroup(EoDbLine::CreateLine(pts[0], cursorPosition)
              ->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
      break;

    case ID_OP3:
      if (numberOfPoints == 1) { return; }

      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);
      group = new EoDbGroup(new EoDbPolygon(pts));
      break;

    case ID_OP4:
      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);

      if (numberOfPoints == 1) { return; }

      pts.Add(pts[0] + EoGeVector3d(pts[1], pts[2]));

      group = new EoDbGroup;

      for (int i = 0; i < 4; i++) {
        group->AddTail(EoDbLine::CreateLine(pts[i], pts[(i + 1) % 4])
                ->WithProperties(renderState.Color(), renderState.LineTypeIndex()));
      }

      break;

    case ID_OP5: {
      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      pts.Add(cursorPosition);

      if (numberOfPoints == 1) { return; }
      EoGePoint3d start{pts[0]};
      EoGePoint3d intermediate{pts[1]};
      EoGePoint3d end{pts[2]};
      auto* radialArc = EoDbConic::CreateRadialArcFrom3Points(start, intermediate, end);
      if (radialArc == nullptr) {
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;
      }
      radialArc->SetColor(renderState.Color());
      radialArc->SetLineTypeIndex(renderState.LineTypeIndex());

      if (fabs(radialArc->SweepAngle()) < Eo::geometricTolerance) {
        delete radialArc;
        app.AddStringToMessageList(IDS_MSG_PTS_COLINEAR);
        return;
      }
      group = new EoDbGroup(radialArc);
      break;
    }
    case ID_OP6:
      if (numberOfPoints == 1) { return; }

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
      if (pts[numberOfPoints - 1] == cursorPosition) {
        app.AddStringToMessageList(IDS_MSG_PTS_COINCIDE);
        return;
      }
      pts.Add(cursorPosition);
      if (numberOfPoints == 1) {
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
      ellipse->SetColor(renderState.Color());
      ellipse->SetLineTypeIndex(renderState.LineTypeIndex());

      group = new EoDbGroup(ellipse);

      break;
    }
    default:
      return;
  }
  document->AddWorkLayerGroup(group);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(previousDrawCommand);
}

void AeSysView::OnDrawModeEscape() {
  auto* document = GetDocument();
  document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  ModeLineUnhighlightOp(previousDrawCommand);
}

void AeSysView::OnDrawModeShiftReturn() {
  if (previousDrawCommand == ID_OP3) {
    auto* group = new EoDbGroup(new EoDbPolyline(pts));
    auto* document = GetDocument();
    document->AddWorkLayerGroup(group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  }
  ModeLineUnhighlightOp(previousDrawCommand);
  pts.RemoveAll();
}
#if defined(USING_STATE_PATTERN)
void AeSysView::OnDrawCommand(UINT command) {
  auto* state = GetCurrentState();
  if (state) {
    state->HandleCommand(this, command);
  } else {
    // Fallback or error
  }
}
#endif
void AeSysView::DoDrawModeMouseMove() {
  auto cursorPosition = GetCursorPosition();
  auto numberOfPoints = pts.GetSize();

  auto* document = GetDocument();
  switch (previousDrawCommand) {
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
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
      if (numberOfPoints == 1) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      } else {
        m_PreviewGroup.AddTail(new EoDbPolygon(pts));
      }
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;

    case ID_OP4:
      cursorPosition = SnapPointToAxis(pts[numberOfPoints - 1], cursorPosition);
      pts.Add(cursorPosition);

      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      if (numberOfPoints == 2) {
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

      if (numberOfPoints == 1) { m_PreviewGroup.AddTail(new EoDbPolyline(pts)); }
      if (numberOfPoints == 2) {
        EoGePoint3d start{pts[0]};
        EoGePoint3d intermediate{pts[1]};
        auto radialArc = EoDbConic::CreateRadialArcFrom3Points(start, intermediate, cursorPosition);
        if (radialArc == nullptr) { break; }

        radialArc->SetColor(renderState.Color());
        radialArc->SetLineTypeIndex(renderState.LineTypeIndex());
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
      if (numberOfPoints == 1) {
        m_PreviewGroup.AddTail(new EoDbPolyline(pts));
      } else {
        EoGeVector3d majorAxis(pts[0], pts[1]);
        EoGeVector3d minorAxis(pts[0], cursorPosition);

        if (minorAxis.Length() < Eo::geometricTolerance) { break; }
        auto extrusion = CrossProduct(majorAxis, minorAxis);
        extrusion.Normalize();
        double ratio = minorAxis.Length() / majorAxis.Length();
        auto* ellipse = EoDbConic::CreateEllipse(pts[0], extrusion, majorAxis, ratio);
        ellipse->SetColor(renderState.Color());
        ellipse->SetLineTypeIndex(renderState.LineTypeIndex());

        m_PreviewGroup.AddTail(ellipse);
      }
      document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      break;
  }
  pts.SetSize(numberOfPoints);
}
