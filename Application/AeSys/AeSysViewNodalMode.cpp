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
#include "Resource.h"

double NodalModePickTolerance{0.05};
std::uint16_t PreviousNodalCommand{};
EoGePoint3d PreviousNodalCursorPosition;

void AeSysView::OnNodalModeAddRemove() {
  app.m_NodalModeAddGroups = !app.m_NodalModeAddGroups;
  if (app.m_NodalModeAddGroups) {
    SetModeCursor(ID_MODE_NODAL);
  } else {
    SetModeCursor(ID_MODE_NODALR);
  }
}

void AeSysView::OnNodalModePoint() {
  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);

      const auto mask = document->GetPrimitiveMask(primitive);
      primitive->GetAllPoints(pts);

      for (int i = 0; i < pts.GetSize(); i++) {
        if (EoGeVector3d(pts[i], cursorPosition).Length() <= NodalModePickTolerance) {
          document->UpdateNodalList(group, primitive, mask, i, pts[i]);
        }
      }
    }
  }
  pts.RemoveAll();
}

void AeSysView::OnNodalModeLine() {
  const auto cursorPosition = GetCursorPosition();

  auto* group = SelectGroupAndPrimitive(cursorPosition);
  if (group != nullptr) {
    EoDbPrimitive* primitive = EngagedPrimitive();

    auto* document = GetDocument();
    const auto mask = document->GetPrimitiveMask(primitive);
    primitive->GetAllPoints(pts);

    for (int i = 0; i < pts.GetSize(); i++) { document->UpdateNodalList(group, primitive, mask, i, pts[i]); }
    pts.RemoveAll();
  }
}

void AeSysView::OnNodalModeArea() {
  auto* document = GetDocument();
  const auto cursorPosition = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP3) {
    PreviousNodalCursorPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Rectangles);
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP3);
  } else {
    if (PreviousNodalCursorPosition != cursorPosition) {
      EoGePoint3d minExtent;
      EoGePoint3d maxExtent;
      EoGeLine(PreviousNodalCursorPosition, cursorPosition).Extents(minExtent, maxExtent);

      auto groupPosition = GetFirstVisibleGroupPosition();
      while (groupPosition != nullptr) {
        auto* group = GetNextVisibleGroup(groupPosition);

        auto primitivePosition = group->GetHeadPosition();
        while (primitivePosition != nullptr) {
          auto* primitive = group->GetNext(primitivePosition);
          const auto mask = document->GetPrimitiveMask(primitive);
          primitive->GetAllPoints(pts);

          for (int i = 0; i < pts.GetSize(); i++) {
            if (pts[i].IsContained(minExtent, maxExtent)) {
              document->UpdateNodalList(group, primitive, mask, i, pts[i]);
            }
          }
        }
      }
      pts.RemoveAll();
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousNodalCommand);
  }
}

void AeSysView::OnNodalModeMove() {
  const auto cursorPosition = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP4) {
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP4);
    pts.RemoveAll();
    pts.Add(cursorPosition);
    RubberBandingStartAtEnable(cursorPosition, Lines);
    ConstructPreviewGroup();
  } else {
    OnNodalModeReturn();
  }
}
void AeSysView::OnNodalModeCopy() {
  const auto cursorPosition = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP5) {
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP5);
    pts.RemoveAll();
    pts.Add(cursorPosition);
    RubberBandingStartAtEnable(cursorPosition, Lines);
    ConstructPreviewGroupForNodalGroups();
  } else {
    OnNodalModeReturn();
  }
}

void AeSysView::OnNodalModeToLine() {
  auto cursorPosition = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP6) {
    PreviousNodalCursorPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Lines);
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP6);
  } else {
    if (PreviousNodalCursorPosition != cursorPosition) {
      cursorPosition = SnapPointToAxis(PreviousNodalCursorPosition, cursorPosition);
      const EoGeVector3d translate(PreviousNodalCursorPosition, cursorPosition);

      auto* group = new EoDbGroup;
      auto* document = GetDocument();

      auto pointPosition = document->GetFirstUniquePointPosition();
      while (pointPosition != nullptr) {
        EoGeUniquePoint* uniquePoint = document->GetNextUniquePoint(pointPosition);
        auto* line = EoDbLine::CreateLine(uniquePoint->m_Point, uniquePoint->m_Point + translate)
                         ->WithProperties(Gs::renderState);
        group->AddTail(line);
      }
      document->AddWorkLayerGroup(group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);

      SetCursorPosition(cursorPosition);
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousNodalCommand);
  }
}

/// <remarks>
/// The pen color used for any polygons added to drawing is the current pen color and
/// not the pen color of the reference primitives.
/// </remarks>
void AeSysView::OnNodalModeToPolygon() {
  auto cursorPosition = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP7) {
    PreviousNodalCursorPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Lines);
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP7);
  } else {
    if (PreviousNodalCursorPosition == cursorPosition) { return; }
    cursorPosition = SnapPointToAxis(PreviousNodalCursorPosition, cursorPosition);
    const EoGeVector3d translate(PreviousNodalCursorPosition, cursorPosition);

    pts.SetSize(4);

    auto* deviceContext = GetDC();
    const int savedRenderState = Gs::renderState.Save();

    auto* document = GetDocument();
    auto groupPosition = document->GetFirstNodalGroupPosition();
    while (groupPosition != nullptr) {
      auto* group = document->GetNextNodalGroup(groupPosition);

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);

        const auto mask = document->GetPrimitiveMask(primitive);
        if (mask == 0) { continue; }
        if (primitive->Is(EoDb::kLinePrimitive)) {
          if ((mask & 3) == 3) {
            const auto* line = dynamic_cast<EoDbLine*>(primitive);

            pts[0] = line->Begin();
            pts[1] = line->End();
            pts[2] = pts[1] + translate;
            pts[3] = pts[0] + translate;

            auto* newGroup = new EoDbGroup(new EoDbPolygon(pts));
            document->AddWorkLayerGroup(newGroup);
            document->UpdateAllViews(nullptr, EoDb::kGroupSafe, newGroup);
          }
        } else if (primitive->Is(EoDb::kPolygonPrimitive)) {
          auto* polygon = dynamic_cast<EoDbPolygon*>(primitive);
          const int numberOfVertices = polygon->NumberOfVertices();

          for (int i = 0; i < numberOfVertices; i++) {
            if (btest(mask, i) && btest(mask, ((i + 1) % numberOfVertices))) {
              pts[0] = polygon->Vertex(i);
              pts[1] = polygon->Vertex((i + 1) % numberOfVertices);
              pts[2] = pts[1] + translate;
              pts[3] = pts[0] + translate;

              auto* newGroup = new EoDbGroup(new EoDbPolygon(pts));
              document->AddWorkLayerGroup(newGroup);
              document->UpdateAllViews(nullptr, EoDb::kGroupSafe, newGroup);
            }
          }
        }
      }
    }
    Gs::renderState.Restore(deviceContext, savedRenderState);

    pts.SetSize(0);

    SetCursorPosition(cursorPosition);
    RubberBandingDisable();
    ModeLineUnhighlightOp(PreviousNodalCommand);
  }
}

void AeSysView::OnNodalModeEmpty() { OnNodalModeEscape(); }

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
  auto cursorPosition = GetCursorPosition();
  auto* document = GetDocument();

  switch (PreviousNodalCommand) {
    case ID_OP4:
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        const EoGeVector3d translate(pts[0], cursorPosition);

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
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        const EoGeVector3d translate(pts[0], cursorPosition);

        auto groupPosition = document->GetFirstNodalGroupPosition();
        while (groupPosition != nullptr) {
          auto* group = document->GetNextNodalGroup(groupPosition);
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
  pts.RemoveAll();
  RubberBandingDisable();
  ModeLineUnhighlightOp(PreviousNodalCommand);
  PreviousNodalCommand = 0;
}

void AeSysView::OnNodalModeEscape() {
  auto* document = GetDocument();
  if (PreviousNodalCommand == 0) {
    document->DisplayUniquePoints();
    document->DeleteNodalResources();
  } else {
    RubberBandingDisable();
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    InvalidateOverlay();
    ConstructPreviewGroup();
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    pts.RemoveAll();

    ModeLineUnhighlightOp(PreviousNodalCommand);
    PreviousNodalCommand = 0;
  }
}
void AeSysView::DoNodalModeMouseMove() {
  const EoDbHandleSuppressionScope suppressHandles;
  auto cursorPosition = GetCursorPosition();
  const auto numberOfPoints = pts.GetSize();
  auto* document = GetDocument();

  switch (PreviousNodalCommand) {
    case ID_OP4:
      VERIFY(pts.GetSize() > 0);

      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        const EoGeVector3d translate(pts[0], cursorPosition);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        auto maskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
        while (maskedPrimitivePosition != nullptr) {
          auto* maskedPrimitive = document->GetNextMaskedPrimitive(maskedPrimitivePosition);
          auto* primitive = maskedPrimitive->GetPrimitive();
          const auto mask = maskedPrimitive->GetMask();
          m_PreviewGroup.AddTail(primitive->Copy(primitive));
          static_cast<EoDbPrimitive*>(m_PreviewGroup.GetTail())->TranslateUsingMask(translate, mask);
        }
        auto uniquePointPosition = document->GetFirstUniquePointPosition();
        while (uniquePointPosition != nullptr) {
          auto* uniquePoint = document->GetNextUniquePoint(uniquePointPosition);
          EoGePoint3d point = (uniquePoint->m_Point) + translate;
          m_PreviewGroup.AddTail(new EoDbPoint(252, 8, point));
        }
        InvalidateOverlay();
      }
      break;

    case ID_OP5:

      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        const EoGeVector3d translate(pts[0], cursorPosition);

        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        ConstructPreviewGroupForNodalGroups();
        m_PreviewGroup.Translate(translate);
        InvalidateOverlay();
      }
      break;
  }
  pts.SetSize(numberOfPoints);
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
    auto* uniquePoint = document->GetNextUniquePoint(uniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 8, uniquePoint->m_Point));
  }
}
void AeSysView::ConstructPreviewGroupForNodalGroups() {
  const EoDbHandleSuppressionScope suppressHandles;
  auto* document = GetDocument();
  auto groupPosition = document->GetFirstNodalGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = document->GetNextNodalGroup(groupPosition);

    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      EoDbPrimitive* primitive = group->GetNext(primitivePosition);
      m_PreviewGroup.AddTail(primitive->Copy(primitive));
    }
  }
  auto uniquePointPosition = document->GetFirstUniquePointPosition();
  while (uniquePointPosition != nullptr) {
    auto* uniquePoint = document->GetNextUniquePoint(uniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 8, uniquePoint->m_Point));
  }
}
