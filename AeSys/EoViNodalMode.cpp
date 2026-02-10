#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "PrimState.h"
#include "Resource.h"

double NodalModePickTolerance{0.05};
EoUInt16 PreviousNodalCommand{};
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
  auto cursorPosition = GetCursorPosition();

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(GroupPosition);

    auto PrimitivePosition = group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      auto* primitive = group->GetNext(PrimitivePosition);

      DWORD Mask = document->GetPrimitiveMask(primitive);
      primitive->GetAllPoints(pts);

      for (int i = 0; i < pts.GetSize(); i++) {
        if (EoGeVector3d(pts[i], cursorPosition).Length() <= NodalModePickTolerance) {
          document->UpdateNodalList(group, primitive, Mask, i, pts[i]);
        }
      }
    }
  }
  pts.RemoveAll();
}

void AeSysView::OnNodalModeLine() {
  auto cursorPosition = GetCursorPosition();

  auto* group = SelectGroupAndPrimitive(cursorPosition);
  if (group != nullptr) {
    EoDbPrimitive* primitive = EngagedPrimitive();

    auto* document = GetDocument();
    DWORD Mask = document->GetPrimitiveMask(primitive);
    primitive->GetAllPoints(pts);

    for (int i = 0; i < pts.GetSize(); i++) { document->UpdateNodalList(group, primitive, Mask, i, pts[i]); }
    pts.RemoveAll();
  }
}

void AeSysView::OnNodalModeArea() {
  auto* document = GetDocument();
  auto cursorPosition = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP3) {
    PreviousNodalCursorPosition = cursorPosition;
    RubberBandingStartAtEnable(cursorPosition, Rectangles);
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP3);
  } else {
    if (PreviousNodalCursorPosition != cursorPosition) {
      EoGePoint3d MinExtent;
      EoGePoint3d MaxExtent;
      EoGeLine(PreviousNodalCursorPosition, cursorPosition).Extents(MinExtent, MaxExtent);

      auto GroupPosition = GetFirstVisibleGroupPosition();
      while (GroupPosition != nullptr) {
        auto* group = GetNextVisibleGroup(GroupPosition);

        auto PrimitivePosition = group->GetHeadPosition();
        while (PrimitivePosition != nullptr) {
          auto* primitive = group->GetNext(PrimitivePosition);
          DWORD Mask = document->GetPrimitiveMask(primitive);
          primitive->GetAllPoints(pts);

          for (int i = 0; i < pts.GetSize(); i++) {
            if (pts[i].IsContained(MinExtent, MaxExtent)) {
              document->UpdateNodalList(group, primitive, Mask, i, pts[i]);
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
  auto cursorPosition = GetCursorPosition();
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
  auto cursorPosition = GetCursorPosition();
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
      EoGeVector3d Translate(PreviousNodalCursorPosition, cursorPosition);

      auto* group = new EoDbGroup;
      auto* document = GetDocument();

      auto PointPosition = document->GetFirstUniquePointPosition();
      while (PointPosition != 0) {
        EoGeUniquePoint* UniquePoint = document->GetNextUniquePoint(PointPosition);
        auto* line =
            new EoDbLine(pstate.Color(), pstate.LineType(), UniquePoint->m_Point, UniquePoint->m_Point + Translate);
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
    EoGeVector3d Translate(PreviousNodalCursorPosition, cursorPosition);

    pts.SetSize(4);

    auto* deviceContext = GetDC();
    int primitiveState = pstate.Save();

    auto* document = GetDocument();
    auto groupPosition = document->GetFirstNodalGroupPosition();
    while (groupPosition != nullptr) {
      auto* group = document->GetNextNodalGroup(groupPosition);

      auto primitivePosition = group->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = group->GetNext(primitivePosition);

        DWORD mask = document->GetPrimitiveMask(primitive);
        if (mask == 0) { continue; }
        if (primitive->Is(EoDb::kLinePrimitive)) {
          if ((mask & 3) == 3) {
            auto* line = static_cast<EoDbLine*>(primitive);

            pts[0] = line->Begin();
            pts[1] = line->End();
            pts[2] = pts[1] + Translate;
            pts[3] = pts[0] + Translate;

            EoDbGroup* NewGroup = new EoDbGroup(new EoDbPolygon(pts));
            document->AddWorkLayerGroup(NewGroup);
            document->UpdateAllViews(nullptr, EoDb::kGroupSafe, NewGroup);
          }
        } else if (primitive->Is(EoDb::kPolygonPrimitive)) {
          auto* polygon = static_cast<EoDbPolygon*>(primitive);
          int numberOfVertices = polygon->NumberOfVertices();

          for (int i = 0; i < numberOfVertices; i++) {
            if (btest(mask, i) && btest(mask, ((i + 1) % numberOfVertices))) {
              pts[0] = polygon->Vertex(i);
              pts[1] = polygon->Vertex((i + 1) % numberOfVertices);
              pts[2] = pts[1] + Translate;
              pts[3] = pts[0] + Translate;

              EoDbGroup* NewGroup = new EoDbGroup(new EoDbPolygon(pts));
              document->AddWorkLayerGroup(NewGroup);
              document->UpdateAllViews(nullptr, EoDb::kGroupSafe, NewGroup);
            }
          }
        }
      }
    }
    pstate.Restore(deviceContext, primitiveState);

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
    auto mask = document->GetPrimitiveMask(EngagedPrimitive());
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
        EoGeVector3d Translate(pts[0], cursorPosition);

        auto MaskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
        while (MaskedPrimitivePosition != 0) {
          EoDbMaskedPrimitive* MaskedPrimitive = document->GetNextMaskedPrimitive(MaskedPrimitivePosition);
          auto* primitive = MaskedPrimitive->GetPrimitive();
          DWORD Mask = MaskedPrimitive->GetMask();
          primitive->TranslateUsingMask(Translate, Mask);
        }
        EoGeUniquePoint* Point;

        auto UniquePointPosition = document->GetFirstUniquePointPosition();
        while (UniquePointPosition != 0) {
          Point = document->GetNextUniquePoint(UniquePointPosition);
          Point->m_Point += Translate;
        }
        SetCursorPosition(cursorPosition);
      }
      break;

    case ID_OP5:
      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        EoGeVector3d Translate(pts[0], cursorPosition);

        auto GroupPosition = document->GetFirstNodalGroupPosition();
        while (GroupPosition != nullptr) {
          auto* Group = document->GetNextNodalGroup(GroupPosition);
          document->AddWorkLayerGroup(new EoDbGroup(*Group));
          document->GetLastWorkLayerGroup()->Translate(Translate);
        }
        SetCursorPosition(cursorPosition);
      }
      break;

    default:
      return;
  }
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();
  pts.RemoveAll();
  RubberBandingDisable();
  ModeLineUnhighlightOp(PreviousNodalCommand);
}
void AeSysView::OnNodalModeEscape() {
  auto* document = GetDocument();
  if (PreviousNodalCommand == 0) {
    document->DisplayUniquePoints();
    document->DeleteNodalResources();
  } else {
    RubberBandingDisable();
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    ConstructPreviewGroup();
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    pts.RemoveAll();

    ModeLineUnhighlightOp(PreviousNodalCommand);
  }
}
void AeSysView::DoNodalModeMouseMove() {
  auto cursorPosition = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();
  auto* document = GetDocument();

  switch (PreviousNodalCommand) {
    case ID_OP4:
      VERIFY(pts.GetSize() > 0);

      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        EoGeVector3d Translate(pts[0], cursorPosition);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        auto MaskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
        while (MaskedPrimitivePosition != 0) {
          EoDbMaskedPrimitive* MaskedPrimitive = document->GetNextMaskedPrimitive(MaskedPrimitivePosition);
          auto* primitive = MaskedPrimitive->GetPrimitive();
          DWORD Mask = MaskedPrimitive->GetMask();
          m_PreviewGroup.AddTail(primitive->Copy(primitive));
          ((EoDbPrimitive*)m_PreviewGroup.GetTail())->TranslateUsingMask(Translate, Mask);
        }
        auto UniquePointPosition = document->GetFirstUniquePointPosition();
        while (UniquePointPosition != 0) {
          EoGeUniquePoint* UniquePoint = document->GetNextUniquePoint(UniquePointPosition);
          EoGePoint3d Point = (UniquePoint->m_Point) + Translate;
          m_PreviewGroup.AddTail(new EoDbPoint(252, 8, Point));
        }
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP5:

      if (pts[0] != cursorPosition) {
        cursorPosition = SnapPointToAxis(pts[0], cursorPosition);
        pts.Add(cursorPosition);

        EoGeVector3d Translate(pts[0], cursorPosition);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        ConstructPreviewGroupForNodalGroups();
        m_PreviewGroup.Translate(Translate);
        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;
  }
  pts.SetSize(NumberOfPoints);
}

void AeSysView::ConstructPreviewGroup() {
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  auto* document = GetDocument();
  auto MaskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    EoDbMaskedPrimitive* MaskedPrimitive = document->GetNextMaskedPrimitive(MaskedPrimitivePosition);
    auto* primitive = MaskedPrimitive->GetPrimitive();
    m_PreviewGroup.AddTail(primitive->Copy(primitive));
  }
  auto UniquePointPosition = document->GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = document->GetNextUniquePoint(UniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 8, UniquePoint->m_Point));
  }
}
void AeSysView::ConstructPreviewGroupForNodalGroups() {
  auto* document = GetDocument();
  auto GroupPosition = document->GetFirstNodalGroupPosition();
  while (GroupPosition != nullptr) {
    auto* group = document->GetNextNodalGroup(GroupPosition);

    auto PrimitivePosition = group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* primitive = group->GetNext(PrimitivePosition);
      m_PreviewGroup.AddTail(primitive->Copy(primitive));
    }
  }
  auto UniquePointPosition = document->GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = document->GetNextUniquePoint(UniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 8, UniquePoint->m_Point));
  }
}
