#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoGeUniquePoint.h"
#include "EoGsRenderState.h"
#include "EoMsNodal.h"
#include "Resource.h"

double NodalModePickTolerance{0.05};

namespace {
/// Returns the active NodalModeState if Nodal mode is engaged, else nullptr.
/// Nodal handlers are only invoked from the Nodal command map, so a non-null
/// state is the normal contract — callers should treat null as "mode not
/// active" and fall through quietly rather than crash.
NodalModeState* NodalState(AeSysView* view) {
  if (view == nullptr) { return nullptr; }
  return dynamic_cast<NodalModeState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnNodalModeAddRemove() {
  app.m_NodalModeAddGroups = !app.m_NodalModeAddGroups;
  if (app.m_NodalModeAddGroups) {
    SetModeCursor(ID_MODE_NODAL);
  } else {
    SetModeCursor(ID_MODE_NODALR);
  }
}

void AeSysView::OnNodalModePoint() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();

  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);

      const auto mask = document->GetPrimitiveMask(primitive);
      primitive->GetAllPoints(points);

      for (int i = 0; i < points.GetSize(); i++) {
        if (EoGeVector3d(points[i], cursorPosition).Length() <= NodalModePickTolerance) {
          document->UpdateNodalList(group, primitive, mask, i, points[i]);
        }
      }
    }
  }
  points.RemoveAll();
  InvalidateOverlay();
}

void AeSysView::OnNodalModeLine() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();

  const auto cursorPosition = GetCursorPosition();

  auto* group = SelectGroupAndPrimitive(cursorPosition);
  if (group != nullptr) {
    EoDbPrimitive* primitive = EngagedPrimitive();

    auto* document = GetDocument();
    const auto mask = document->GetPrimitiveMask(primitive);
    primitive->GetAllPoints(points);

    for (int i = 0; i < points.GetSize(); i++) { document->UpdateNodalList(group, primitive, mask, i, points[i]); }
    points.RemoveAll();
    InvalidateOverlay();
  }
}

void AeSysView::OnNodalModeArea() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousCommand = state->PreviousCommandRef();
  auto& previousCursorPosition = state->PreviousCursorPositionRef();

  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();
  if (previousCommand != ID_OP3) {
    previousCursorPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Rectangles);
    previousCommand = ModeLineHighlightOp(ID_OP3);
  } else {
    if (previousCursorPosition != cursorPosition) {
      EoGePoint3d minExtent;
      EoGePoint3d maxExtent;
      EoGeLine(previousCursorPosition, cursorPosition).Extents(minExtent, maxExtent);

      auto groupPosition = GetFirstVisibleGroupPosition();
      while (groupPosition != nullptr) {
        auto* group = GetNextVisibleGroup(groupPosition);

        auto primitivePosition = group->GetHeadPosition();
        while (primitivePosition != nullptr) {
          auto* primitive = group->GetNext(primitivePosition);
          const auto mask = document->GetPrimitiveMask(primitive);
          primitive->GetAllPoints(points);

          for (int i = 0; i < points.GetSize(); i++) {
            if (points[i].IsContained(minExtent, maxExtent)) {
              document->UpdateNodalList(group, primitive, mask, i, points[i]);
            }
          }
        }
      }
      points.RemoveAll();
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(state->PreviousCommandRef());
    InvalidateOverlay();
  }
}

void AeSysView::OnNodalModeMove() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();

  const auto cursorPosition = GetCursorPosition();
  if (state->PreviousCommand() != ID_OP4) {
    state->SetPreviousCommand(ModeLineHighlightOp(ID_OP4));
    points.RemoveAll();
    points.Add(cursorPosition);
    RubberBandingStartAtEnable(cursorPosition, Lines);
    ConstructPreviewGroup();
  } else {
    OnNodalModeReturn();
  }
}
void AeSysView::OnNodalModeCopy() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();

  const auto cursorPosition = GetCursorPosition();
  if (state->PreviousCommand() != ID_OP5) {
    state->SetPreviousCommand(ModeLineHighlightOp(ID_OP5));
    points.RemoveAll();
    points.Add(cursorPosition);
    RubberBandingStartAtEnable(cursorPosition, Lines);
    ConstructPreviewGroupForNodalGroups();
  } else {
    OnNodalModeReturn();
  }
}

void AeSysView::OnNodalModeToLine() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& previousCommand = state->PreviousCommandRef();
  auto& previousCursorPosition = state->PreviousCursorPositionRef();

  auto cursorPosition = GetCursorPosition();
  if (previousCommand != ID_OP6) {
    previousCursorPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Lines);
    previousCommand = ModeLineHighlightOp(ID_OP6);
  } else {
    if (previousCursorPosition != cursorPosition) {
      cursorPosition = SnapPointToAxis(previousCursorPosition, cursorPosition);
      const EoGeVector3d translate(previousCursorPosition, cursorPosition);

      auto* group = new EoDbGroup;
      auto* document = GetDocument();

      auto pointPosition = document->GetFirstUniquePointPosition();
      while (pointPosition != nullptr) {
        const auto* uniquePoint = document->GetNextUniquePoint(pointPosition);
        auto* line = EoDbLine::CreateLine(uniquePoint->m_Point, uniquePoint->m_Point + translate)
                         ->WithProperties(Gs::renderState);
        group->AddTail(line);
      }
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      SetCursorPosition(cursorPosition);
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(state->PreviousCommandRef());
  }
}

/// <remarks>
/// The pen color used for any polygons added to drawing is the current pen color and
/// not the pen color of the reference primitives.
/// </remarks>
void AeSysView::OnNodalModeToPolygon() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();
  auto& previousCommand = state->PreviousCommandRef();
  auto& previousCursorPosition = state->PreviousCursorPositionRef();

  auto cursorPosition = GetCursorPosition();
  if (previousCommand != ID_OP7) {
    previousCursorPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Lines);
    previousCommand = ModeLineHighlightOp(ID_OP7);
  } else {
    if (previousCursorPosition == cursorPosition) { return; }
    cursorPosition = SnapPointToAxis(previousCursorPosition, cursorPosition);
    const EoGeVector3d translate(previousCursorPosition, cursorPosition);

    points.SetSize(4);

    auto* deviceContext = GetDC();
    const int savedRenderState = Gs::renderState.Save();

    auto* document = GetDocument();
    auto groupPosition = document->GetFirstNodalGroupPosition();
    while (groupPosition != nullptr) {
      const auto* group = document->GetNextNodalGroup(groupPosition);

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);

        const auto mask = document->GetPrimitiveMask(primitive);
        if (mask == 0) { continue; }
        if (primitive->Is(EoDb::kLinePrimitive)) {
          if ((mask & 3) == 3) {
            const auto* line = dynamic_cast<EoDbLine*>(primitive);

            points[0] = line->Begin();
            points[1] = line->End();
            points[2] = points[1] + translate;
            points[3] = points[0] + translate;

            auto* newGroup = new EoDbGroup(new EoDbPolygon(points));
            document->AddWorkLayerGroup(newGroup);
            document->UpdateAllViews(nullptr, EoDb::kGroupSafe, newGroup);
          }
        } else if (primitive->Is(EoDb::kPolygonPrimitive)) {
          const auto* polygon = dynamic_cast<EoDbPolygon*>(primitive);
          const int numberOfVertices = polygon->NumberOfVertices();

          for (int i = 0; i < numberOfVertices; i++) {
            if (btest(mask, i) && btest(mask, ((i + 1) % numberOfVertices))) {
              points[0] = polygon->Vertex(i);
              points[1] = polygon->Vertex((i + 1) % numberOfVertices);
              points[2] = points[1] + translate;
              points[3] = points[0] + translate;

              auto* newGroup = new EoDbGroup(new EoDbPolygon(points));
              document->AddWorkLayerGroup(newGroup);
              document->UpdateAllViews(nullptr, EoDb::kGroupSafe, newGroup);
            }
          }
        }
      }
    }
    Gs::renderState.Restore(deviceContext, savedRenderState);

    points.SetSize(0);

    SetCursorPosition(cursorPosition);
    RubberBandingDisable();
    ModeLineUnhighlightOp(state->PreviousCommandRef());
  }
}

void AeSysView::OnNodalModeEmpty() {
  OnNodalModeEscape();
}

void AeSysView::OnNodalModeEngage() {
  if (GroupIsEngaged()) {
    auto* document = GetDocument();
    const auto mask = document->GetPrimitiveMask(EngagedPrimitive());
    EoGePoint3dArray points;

    EngagedPrimitive()->GetAllPoints(points);

    for (int i = 0; i < points.GetSize(); i++) {
      document->UpdateNodalList(EngagedGroup(), EngagedPrimitive(), mask, i, points[i]);
    }
  }
}
void AeSysView::OnNodalModeReturn() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }
  auto& points = state->Points();

  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  switch (state->PreviousCommand()) {
    case ID_OP4:
      if (points[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(points[0], cursorPosition);
        const EoGeVector3d translate(points[0], cursorPosition);

        auto maskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
        while (maskedPrimitivePosition != nullptr) {
          auto* maskedPrimitive = document->GetNextMaskedPrimitive(maskedPrimitivePosition);
          auto* primitive = maskedPrimitive->GetPrimitive();
          const auto mask = maskedPrimitive->GetMask();
          primitive->TranslateUsingMask(translate, mask);
        }
        EoGeUniquePoint* point;

        auto uniquePointPosition = document->GetFirstUniquePointPosition();
        while (uniquePointPosition != nullptr) {
          point = document->GetNextUniquePoint(uniquePointPosition);
          point->m_Point += translate;
        }
        SetCursorPosition(cursorPosition);
      }
      break;

    case ID_OP5:
      if (points[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(points[0], cursorPosition);
        const EoGeVector3d translate(points[0], cursorPosition);

        auto groupPosition = document->GetFirstNodalGroupPosition();
        while (groupPosition != nullptr) {
          const auto* group = document->GetNextNodalGroup(groupPosition);
          document->AddWorkLayerGroup(new EoDbGroup(*group));
          document->GetLastWorkLayerGroup()->Translate(translate);
        }
        SetCursorPosition(cursorPosition);
      }
      break;

    default:
      return;
  }
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  InvalidateScene();
  points.RemoveAll();
  RubberBandingDisable();
  ModeLineUnhighlightOp(state->PreviousCommandRef());
}

void AeSysView::OnNodalModeEscape() {
  auto* state = NodalState(this);
  if (state == nullptr) { return; }

  auto* document = GetDocument();
  if (state->PreviousCommand() == 0) {
    document->DisplayUniquePoints();
    document->DeleteNodalResources();
  } else {
    auto& points = state->Points();
    RubberBandingDisable();
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
    ConstructPreviewGroup();
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    points.RemoveAll();

    ModeLineUnhighlightOp(state->PreviousCommandRef());
  }
}
void AeSysView::DoNodalModeMouseMove() {
  // Preview logic moved to NodalModeState::OnMouseMove.
}

void AeSysView::ConstructPreviewGroup() {
  const EoDbHandleSuppressionScope suppressHandles;
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  auto* document = GetDocument();
  auto maskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
  while (maskedPrimitivePosition != nullptr) {
    auto* maskedPrimitive = document->GetNextMaskedPrimitive(maskedPrimitivePosition);
    auto* primitive = maskedPrimitive->GetPrimitive();
    m_PreviewGroup.AddTail(primitive->Copy(primitive));
  }
  auto uniquePointPosition = document->GetFirstUniquePointPosition();
  while (uniquePointPosition != nullptr) {
    const auto* uniquePoint = document->GetNextUniquePoint(uniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 65, uniquePoint->m_Point));
  }
}
void AeSysView::ConstructPreviewGroupForNodalGroups() {
  const EoDbHandleSuppressionScope suppressHandles;
  auto* document = GetDocument();
  auto groupPosition = document->GetFirstNodalGroupPosition();
  while (groupPosition != nullptr) {
    const auto* group = document->GetNextNodalGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      EoDbPrimitive* primitive = group->GetNext(primitivePosition);
      m_PreviewGroup.AddTail(primitive->Copy(primitive));
    }
  }
  auto uniquePointPosition = document->GetFirstUniquePointPosition();
  while (uniquePointPosition != nullptr) {
    const auto* uniquePoint = document->GetNextUniquePoint(uniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 65, uniquePoint->m_Point));
  }
}
