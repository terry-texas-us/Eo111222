#include "Stdafx.h"

#include <algorithm>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "AnnotateModeState.h"
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
#include "EoGsRenderState.h"
#include "Resource.h"

namespace {
/// Returns the active AnnotateModeState from the view's state stack, or nullptr when
/// called outside annotate mode (e.g. during PopAllModeStates teardown).
AnnotateModeState* AnnotateState(AeSysView* view) {
  return dynamic_cast<AnnotateModeState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnAnnotateModeOptions() {
  EoDlgAnnotateOptions dialog(this);

  if (dialog.DoModal() == IDOK) {}
}

void AeSysView::OnAnnotateModeLine() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (previousOp == 0) {
    points.RemoveAll();
    points.Add(cursorPosition);
  } else {
    if (CorrectLeaderEndpoints(previousOp, ID_OP2, points[0], cursorPosition)) {
      auto* group = new EoDbGroup;
      document->AddWorkLayerGroup(group);

      if (previousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, points[0], group); }
      group->AddTail(EoDbLine::CreateLine(points[0], cursorPosition)->WithProperties(1, L"CONTINUOUS"));
      points[0] = cursorPosition;
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    }
  }
  previousOp = ModeLineHighlightOp(ID_OP2);
}

void AeSysView::OnAnnotateModeArrow() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (previousOp == 0) {
    points.RemoveAll();
    points.Add(cursorPosition);
  } else {
    if (CorrectLeaderEndpoints(previousOp, ID_OP3, points[0], cursorPosition)) {
      auto* group = new EoDbGroup;

      if (previousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, points[0], group); }
      group->AddTail(EoDbLine::CreateLine(points[0], cursorPosition)->WithProperties(1, L"CONTINUOUS"));
      GenerateLineEndItem(EndItemType(), EndItemSize(), points[0], cursorPosition, group);
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      points[0] = cursorPosition;
      m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    }
  }
  previousOp = ModeLineHighlightOp(ID_OP3);
}

void AeSysView::OnAnnotateModeBubble() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto* document = GetDocument();
  static CString currentText;
  auto cursorPosition = GetCursorPosition();

  EoDlgSetText dialog;
  dialog.m_strTitle = L"Set Bubble Text";
  dialog.m_sText = currentText;
  if (dialog.DoModal() == IDOK) { currentText = dialog.m_sText; }
  auto* group = new EoDbGroup;
  document->AddWorkLayerGroup(group);
  if (previousOp == 0) {  // No operation pending
    points.RemoveAll();
    points.Add(cursorPosition);
  } else {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();

    EoGePoint3d pt(cursorPosition);

    if (CorrectLeaderEndpoints(previousOp, ID_OP4, points[0], pt)) {
      if (previousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, points[0], group); }
      group->AddTail(EoDbLine::CreateLine(points[0], pt)->WithProperties(1, L"CONTINUOUS"));
    }
  }
  previousOp = ModeLineHighlightOp(ID_OP4);

  if (!currentText.IsEmpty()) {
    auto* deviceContext = GetDC();

    const auto cameraDirection = CameraDirection();
    auto minorAxis = ViewUp();
    auto majorAxis = CrossProduct(minorAxis, cameraDirection);

    majorAxis *= 0.06;
    minorAxis *= 0.1;
    EoGeReferenceSystem referenceSystem(cursorPosition, majorAxis, minorAxis);

    const int savedRenderState = Gs::renderState.Save();
    Gs::renderState.SetColor(deviceContext, 2);

    EoDbFontDefinition fontDefinition = Gs::renderState.FontDefinition();
    fontDefinition.SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

    auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();
    characterCellDefinition.SetRotationAngle(0.0);
    Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);

    group->AddTail(new EoDbText(fontDefinition, referenceSystem, currentText));
    Gs::renderState.Restore(deviceContext, savedRenderState);
    ReleaseDC(deviceContext);
  }
  if (NumberOfSides() == 0) {
    auto circle = EoDbConic::CreateCircleInView(cursorPosition, BubbleRadius());
    circle->SetColor(1);
    circle->SetLineTypeName(L"CONTINUOUS");

    group->AddTail(circle);
  } else {
    group->AddTail(new EoDbPolyline(1, 1, cursorPosition, BubbleRadius(), NumberOfSides()));
  }
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  points[0] = cursorPosition;
}

void AeSysView::OnAnnotateModeHook() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  auto* group = new EoDbGroup;
  if (previousOp == 0) {
    points.RemoveAll();
    points.Add(cursorPosition);
  } else {
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();

    EoGePoint3d pt(cursorPosition);

    if (CorrectLeaderEndpoints(previousOp, ID_OP5, points[0], pt)) {
      if (previousOp == ID_OP3) { GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, points[0], group); }

      group->AddTail(EoDbLine::CreateLine(points[0], pt)->WithProperties(1, L"CONTINUOUS"));
    }
  }
  previousOp = ModeLineHighlightOp(ID_OP5);

  const auto circle = EoDbConic::CreateCircleInView(cursorPosition, CircleRadius());
  circle->SetColor(1);
  circle->SetLineTypeName(L"CONTINUOUS");

  group->AddTail(circle);

  document->AddWorkLayerGroup(group);
  document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
  points[0] = cursorPosition;
}

void AeSysView::OnAnnotateModeUnderline() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();

  points.RemoveAll();

  if (previousOp != 0) {
    ModeLineUnhighlightOp(previousOp);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
  EoDbText* pText = SelectTextUsingPoint(cursorPosition);
  if (pText != nullptr) {
    pText->GetBoundingBox(points, GapSpaceFactor());

    auto* group = new EoDbGroup;
    group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(Gs::renderState.Color(), L"CONTINUOUS"));
    document->AddWorkLayerGroup(group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

    points.RemoveAll();
  }
}

void AeSysView::OnAnnotateModeBox() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  const auto cursorPosition = GetCursorPosition();
  if (previousOp != ID_OP7) {
    if (previousOp != 0) {
      RubberBandingDisable();
      ModeLineUnhighlightOp(previousOp);
    }
    previousOp = ModeLineHighlightOp(ID_OP7);
    points.RemoveAll();
    points.Add(cursorPosition);
  } else {
    EoGePoint3dArray ptsBox1;
    EoGePoint3dArray ptsBox2;
    bool bG1Flg = false;
    bool bG2Flg = false;
    EoDbText* pText = SelectTextUsingPoint(points[0]);
    if (pText != nullptr) {
      pText->GetBoundingBox(ptsBox1, GapSpaceFactor());
      bG1Flg = true;
    }
    pText = SelectTextUsingPoint(cursorPosition);
    if (pText != nullptr) {
      pText->GetBoundingBox(ptsBox2, GapSpaceFactor());
      bG2Flg = true;
    }
    if (bG1Flg && bG2Flg) {
      points.SetSize(4);

      points[0] = ptsBox1[0];
      points[2] = ptsBox1[0];
      for (int i = 1; i < 4; i++) {
        points[0].x = std::min(points[0].x, ptsBox1[i].x);
        points[2].x = std::max(points[2].x, ptsBox1[i].x);
        points[0].y = std::min(points[0].y, ptsBox1[i].y);
        points[2].y = std::max(points[2].y, ptsBox1[i].y);
      }
      for (int i = 0; i < 4; i++) {
        points[0].x = std::min(points[0].x, ptsBox2[i].x);
        points[2].x = std::max(points[2].x, ptsBox2[i].x);
        points[0].y = std::min(points[0].y, ptsBox2[i].y);
        points[2].y = std::max(points[2].y, ptsBox2[i].y);
      }
      points[1].x = points[2].x;
      points[1].y = points[0].y;
      points[3].x = points[0].x;
      points[3].y = points[2].y;

      auto* group = new EoDbGroup;

      for (int i = 0; i < 4; i++) {
        group->AddTail(EoDbLine::CreateLine(points[i], points[(i + 1) % 4])->WithProperties(1, L"CONTINUOUS"));
      }

      points.RemoveAll();

      auto* document = GetDocument();
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
    ModeLineUnhighlightOp(previousOp);
  }
}

void AeSysView::OnAnnotateModeCutIn() {
  auto* deviceContext = GetDC();

  auto cursorPosition = GetCursorPosition();

  auto* group = SelectLineUsingPoint(cursorPosition);
  if (group != nullptr) {
    auto* document = GetDocument();
    auto* pLine = static_cast<EoDbLine*>(EngagedPrimitive());

    cursorPosition = DetPt();

    CString currentText;

    EoDlgSetText dialog;
    dialog.m_strTitle = L"Set Cut-in Text";
    dialog.m_sText = currentText;
    if (dialog.DoModal() == IDOK) { currentText = dialog.m_sText; }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);

    const int savedRenderState = Gs::renderState.Save();

    if (!currentText.IsEmpty()) {
      EoGeLine line = pLine->Line();
      double angle = line.AngleFromXAxisXY();
      if (angle > 0.25 * Eo::TwoPi && angle < 0.75 * Eo::TwoPi) { angle += Eo::Pi; }

      const auto cameraDirection = CameraDirection();
      auto minorAxis = ViewUp();
      minorAxis.RotateAboutArbitraryAxis(cameraDirection, angle);
      auto majorAxis = CrossProduct(minorAxis, cameraDirection);
      majorAxis *= 0.06;
      minorAxis *= 0.1;
      EoGeReferenceSystem referenceSystem(cursorPosition, majorAxis, minorAxis);

      const auto color = Gs::renderState.Color();
      Gs::renderState.SetColor(deviceContext, 2);

      EoDbFontDefinition fontDefinition = Gs::renderState.FontDefinition();
      fontDefinition.SetAlignment(EoDb::HorizontalAlignment::Center, EoDb::VerticalAlignment::Middle);

      auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();
      characterCellDefinition.SetRotationAngle(0.0);
      Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);

      auto* text = new EoDbText(fontDefinition, referenceSystem, currentText);
      Gs::renderState.SetColor(deviceContext, color);

      group->AddTail(text);

      EoGePoint3dArray ptsBox;
      text->GetBoundingBox(ptsBox, GapSpaceFactor());

      const double dGap = EoGeVector3d(ptsBox[0], ptsBox[1]).Length();

      ptsBox[0] = cursorPosition.ProjectToward(pLine->Begin(), dGap / 2.0);
      ptsBox[1] = cursorPosition.ProjectToward(pLine->End(), dGap / 2.0);

      double dRel[2]{};

      dRel[0] = pLine->RelOfPt(ptsBox[0]);
      dRel[1] = pLine->RelOfPt(ptsBox[1]);

      if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {
        auto* newLinePrimitive = new EoDbLine(*pLine);
        pLine->SetEndPoint(ptsBox[0]);
        newLinePrimitive->SetBeginPoint(ptsBox[1]);
        group->AddTail(newLinePrimitive);
      } else if (dRel[0] < Eo::geometricTolerance) {
        pLine->SetBeginPoint(ptsBox[1]);
      } else if (dRel[1] >= 1.0 - Eo::geometricTolerance) {
        pLine->SetEndPoint(ptsBox[0]);
      }
    }
    document->UpdateAllViews(nullptr, EoDb::kGroup, group);
    Gs::renderState.Restore(deviceContext, savedRenderState);
  }
  ReleaseDC(deviceContext);
}

void AeSysView::OnAnnotateModeConstructionLine() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousOp = state->PreviousOpRef();

  auto cursorPosition = GetCursorPosition();

  if (previousOp != ID_OP9) {
    previousOp = ModeLineHighlightOp(ID_OP9);
    points.RemoveAll();
    points.Add(cursorPosition);
  } else {
    auto* document = GetDocument();
    cursorPosition = SnapPointToAxis(points[0], cursorPosition);
    points.Add(points[0].ProjectToward(cursorPosition, 48.0));
    points.Add(points[1].ProjectToward(points[0], 96.0));

    auto* group = new EoDbGroup(EoDbLine::CreateLine(points[1], points[2])->WithProperties(15, L"Dash2"));
    document->AddWorkLayerGroup(group);
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    ModeLineUnhighlightOp(previousOp);
    points.RemoveAll();
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
  }
}

void AeSysView::OnAnnotateModeReturn() {
  // TODO: Add your command handler code here
}

void AeSysView::OnAnnotateModeEscape() {
  auto* state = AnnotateState(this);
  if (state != nullptr) { state->OnEscape(this); }
}

bool AeSysView::CorrectLeaderEndpoints(int beginType,
    int endType,
    EoGePoint3d& beginPoint,
    EoGePoint3d& endPoint) const {
  const double lineSegmentLength = EoGeVector3d(beginPoint, endPoint).Length();

  double beginDistance{};

  if (beginType == ID_OP4) {
    beginDistance = BubbleRadius();
  } else if (beginType == ID_OP5) {
    beginDistance = CircleRadius();
  }
  double endDistance{};

  if (endType == ID_OP4) {
    endDistance = BubbleRadius();
  } else if (endType == ID_OP5) {
    endDistance = CircleRadius();
  }

  if (lineSegmentLength > beginDistance + endDistance + Eo::geometricTolerance) {
    if (beginDistance != 0.0) { beginPoint = beginPoint.ProjectToward(endPoint, beginDistance); }
    if (endDistance != 0.0) { endPoint = endPoint.ProjectToward(beginPoint, endDistance); }
    return true;
  } else {
    app.AddModeInformationToMessageList();
    return false;
  }
}

void AeSysView::DoAnnotateModeMouseMove() {
  auto* state = AnnotateState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  const auto previousOp = state->PreviousOp();

  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = GetCursorPosition();
  const auto numberOfPoints = points.GetSize();
  points.Add(cursorPosition);
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  switch (previousOp) {
    case ID_OP2:
    case ID_OP3:
      if (points[0] != cursorPosition) {
        if (previousOp == ID_OP3) {
          GenerateLineEndItem(EndItemType(), EndItemSize(), cursorPosition, points[0], &m_PreviewGroup);
        }
        m_PreviewGroup.AddTail(new EoDbPolyline(points));
      }
      break;

    case ID_OP4:
    case ID_OP5: {
      const auto savedFirst = points[0];
      if (CorrectLeaderEndpoints(previousOp, 0, points[0], points[1])) { m_PreviewGroup.AddTail(new EoDbPolyline(points)); }
      points[0] = savedFirst;
      break;
    }
    case ID_OP9:
      if (points[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(points[0], cursorPosition);

        points.Add(points[0].ProjectToward(cursorPosition, 48.0));
        points.Add(points[2].ProjectToward(points[0], 96.0));

        m_PreviewGroup.AddTail(EoDbLine::CreateLine(points[2], points[3])->WithProperties(15, L"Dash2"));
      }
      break;
  }
  InvalidateOverlay();
  points.SetSize(numberOfPoints);
}

void AeSysView::GenerateLineEndItem(int type,
    double size,
    const EoGePoint3d& beginPoint,
    const EoGePoint3d& endPoint,
    EoDbGroup* group) const {
  const auto cameraDirection = CameraDirection();

  EoGePoint3dArray itemPoints;

  if (type == 1 || type == 2) {
    constexpr double angle{0.244978663127};
    const double length{size / 0.970142500145};

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, length));
    itemPoints.Add(pt.RotateAboutAxis(endPoint, cameraDirection, angle));
    itemPoints.Add(endPoint);
    itemPoints.Add(pt.RotateAboutAxis(endPoint, cameraDirection, -angle));
    auto* polyline = new EoDbPolyline(1, 1, itemPoints);
    if (type == 2) { polyline->SetClosed(true); }
    group->AddTail(polyline);
  } else if (type == 3) {
    constexpr double angle{9.96686524912e-2};
    const double length{size / 0.99503719021};

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, length));
    itemPoints.Add(pt.RotateAboutAxis(endPoint, cameraDirection, angle));
    itemPoints.Add(endPoint);

    group->AddTail(new EoDbPolyline(1, 1, itemPoints));
  } else if (type == 4) {
    constexpr double angle{0.785398163397};
    const double length{0.5 * size / 0.707106781187};

    EoGePoint3d pt(endPoint.ProjectToward(beginPoint, length));
    itemPoints.Add(pt.RotateAboutAxis(endPoint, cameraDirection, angle));
    itemPoints.Add(itemPoints[0].RotateAboutAxis(endPoint, cameraDirection, Eo::Pi));
    group->AddTail(new EoDbPolyline(1, 1, itemPoints));
  }
}
