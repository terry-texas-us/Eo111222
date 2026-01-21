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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
  EoGePoint3d CurrentPnt = GetCursorPosition();

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = GetNextVisibleGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

      DWORD Mask = document->GetPrimitiveMask(Primitive);
      Primitive->GetAllPts(pts);

      for (int i = 0; i < pts.GetSize(); i++) {
        if (EoGeVector3d(pts[i], CurrentPnt).Length() <= NodalModePickTolerance) {
          document->UpdateNodalList(Group, Primitive, Mask, i, pts[i]);
        }
      }
    }
  }
  pts.RemoveAll();
}
void AeSysView::OnNodalModeLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  auto* Group = SelectGroupAndPrimitive(CurrentPnt);
  if (Group != 0) {
    EoDbPrimitive* Primitive = EngagedPrimitive();

    auto* document = GetDocument();
    DWORD Mask = document->GetPrimitiveMask(Primitive);
    Primitive->GetAllPts(pts);

    for (int i = 0; i < pts.GetSize(); i++) { document->UpdateNodalList(Group, Primitive, Mask, i, pts[i]); }
    pts.RemoveAll();
  }
}
void AeSysView::OnNodalModeArea() {
  auto* document = GetDocument();
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP3) {
    PreviousNodalCursorPosition = CurrentPnt;
    RubberBandingStartAtEnable(CurrentPnt, Rectangles);
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP3);
  } else {
    if (PreviousNodalCursorPosition != CurrentPnt) {
      EoGePoint3d MinExtent;
      EoGePoint3d MaxExtent;
      EoGeLine(PreviousNodalCursorPosition, CurrentPnt).Extents(MinExtent, MaxExtent);

      auto GroupPosition = GetFirstVisibleGroupPosition();
      while (GroupPosition != nullptr) {
        auto* Group = GetNextVisibleGroup(GroupPosition);

        auto PrimitivePosition = Group->GetHeadPosition();
        while (PrimitivePosition != nullptr) {
          EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
          DWORD Mask = document->GetPrimitiveMask(Primitive);
          Primitive->GetAllPts(pts);

          for (int i = 0; i < pts.GetSize(); i++) {
            if (pts[i].IsContained(MinExtent, MaxExtent)) {
              document->UpdateNodalList(Group, Primitive, Mask, i, pts[i]);
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
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP4) {
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP4);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
    RubberBandingStartAtEnable(CurrentPnt, Lines);
    ConstructPreviewGroup();
  } else {
    OnNodalModeReturn();
  }
}
void AeSysView::OnNodalModeCopy() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP5) {
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP5);
    pts.RemoveAll();
    pts.Add(CurrentPnt);
    RubberBandingStartAtEnable(CurrentPnt, Lines);
    ConstructPreviewGroupForNodalGroups();
  } else {
    OnNodalModeReturn();
  }
}
void AeSysView::OnNodalModeToLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP6) {
    PreviousNodalCursorPosition = CurrentPnt;
    RubberBandingStartAtEnable(CurrentPnt, Lines);
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP6);
  } else {
    if (PreviousNodalCursorPosition != CurrentPnt) {
      CurrentPnt = SnapPointToAxis(PreviousNodalCursorPosition, CurrentPnt);
      EoGeVector3d Translate(PreviousNodalCursorPosition, CurrentPnt);

      auto* Group = new EoDbGroup;
      auto* document = GetDocument();

      auto PointPosition = document->GetFirstUniquePointPosition();
      while (PointPosition != 0) {
        EoGeUniquePoint* UniquePoint = document->GetNextUniquePoint(PointPosition);
        EoDbLine* Primitive = new EoDbLine(UniquePoint->m_Point, UniquePoint->m_Point + Translate);
        Group->AddTail(Primitive);
      }
      document->AddWorkLayerGroup(Group);
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

      SetCursorPosition(CurrentPnt);
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
  EoGePoint3d CurrentPnt = GetCursorPosition();
  if (PreviousNodalCommand != ID_OP7) {
    PreviousNodalCursorPosition = CurrentPnt;
    RubberBandingStartAtEnable(CurrentPnt, Lines);
    PreviousNodalCommand = ModeLineHighlightOp(ID_OP7);
  } else {
    if (PreviousNodalCursorPosition != CurrentPnt) {
      CurrentPnt = SnapPointToAxis(PreviousNodalCursorPosition, CurrentPnt);
      EoGeVector3d Translate(PreviousNodalCursorPosition, CurrentPnt);

      pts.SetSize(4);

      CDC* DeviceContext = GetDC();

      int PrimitiveState = pstate.Save();

      auto* document = GetDocument();
      auto GroupPosition = document->GetFirstNodalGroupPosition();
      while (GroupPosition != nullptr) {
        auto* Group = document->GetNextNodalGroup(GroupPosition);

        auto PrimitivePosition = Group->GetHeadPosition();
        while (PrimitivePosition != nullptr) {
          EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

          DWORD Mask = document->GetPrimitiveMask(Primitive);
          if (Mask != 0) {
            if (Primitive->Is(EoDb::kLinePrimitive)) {
              if ((Mask & 3) == 3) {
                EoDbLine* LinePrimitive = static_cast<EoDbLine*>(Primitive);

                pts[0] = LinePrimitive->BeginPoint();
                pts[1] = LinePrimitive->EndPoint();
                pts[2] = pts[1] + Translate;
                pts[3] = pts[0] + Translate;

                EoDbGroup* NewGroup = new EoDbGroup(new EoDbPolygon(pts));
                document->AddWorkLayerGroup(NewGroup);
                document->UpdateAllViews(nullptr, EoDb::kGroupSafe, NewGroup);
              }
            } else if (Primitive->Is(EoDb::kPolygonPrimitive)) {
              EoDbPolygon* pPolygon = static_cast<EoDbPolygon*>(Primitive);
              int iPts = pPolygon->GetPts();

              for (int i = 0; i < iPts; i++) {
                if (btest(Mask, i) && btest(Mask, ((i + 1) % iPts))) {
                  pts[0] = pPolygon->GetPt(i);
                  pts[1] = pPolygon->GetPt((i + 1) % iPts);
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
      }
      pstate.Restore(DeviceContext, PrimitiveState);

      pts.SetSize(0);

      SetCursorPosition(CurrentPnt);
      RubberBandingDisable();
      ModeLineUnhighlightOp(PreviousNodalCommand);
    }
  }
}
void AeSysView::OnNodalModeEmpty() { OnNodalModeEscape(); }

void AeSysView::OnNodalModeEngage() {
  if (GroupIsEngaged()) {
    auto* document = GetDocument();
    auto mask = document->GetPrimitiveMask(EngagedPrimitive());
    EoGePoint3dArray points;

    EngagedPrimitive()->GetAllPts(points);

    for (int i = 0; i < points.GetSize(); i++) {
      document->UpdateNodalList(EngagedGroup(), EngagedPrimitive(), mask, i, points[i]);
    }
  }
}
void AeSysView::OnNodalModeReturn() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  auto* document = GetDocument();

  switch (PreviousNodalCommand) {
    case ID_OP4:
      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        EoGeVector3d Translate(pts[0], CurrentPnt);

        auto MaskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
        while (MaskedPrimitivePosition != 0) {
          EoDbMaskedPrimitive* MaskedPrimitive = document->GetNextMaskedPrimitive(MaskedPrimitivePosition);
          EoDbPrimitive* Primitive = MaskedPrimitive->GetPrimitive();
          DWORD Mask = MaskedPrimitive->GetMask();
          Primitive->TranslateUsingMask(Translate, Mask);
        }
        EoGeUniquePoint* Point;

        auto UniquePointPosition = document->GetFirstUniquePointPosition();
        while (UniquePointPosition != 0) {
          Point = document->GetNextUniquePoint(UniquePointPosition);
          Point->m_Point += Translate;
        }
        SetCursorPosition(CurrentPnt);
      }
      break;

    case ID_OP5:
      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        EoGeVector3d Translate(pts[0], CurrentPnt);

        auto GroupPosition = document->GetFirstNodalGroupPosition();
        while (GroupPosition != nullptr) {
          auto* Group = document->GetNextNodalGroup(GroupPosition);
          document->AddWorkLayerGroup(new EoDbGroup(*Group));
          document->GetLastWorkLayerGroup()->Translate(Translate);
        }
        SetCursorPosition(CurrentPnt);
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
  EoGePoint3d CurrentPnt = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();
  auto* document = GetDocument();

  switch (PreviousNodalCommand) {
    case ID_OP4:
      VERIFY(pts.GetSize() > 0);

      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        EoGeVector3d Translate(pts[0], CurrentPnt);

        document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        auto MaskedPrimitivePosition = document->GetFirstMaskedPrimitivePosition();
        while (MaskedPrimitivePosition != 0) {
          EoDbMaskedPrimitive* MaskedPrimitive = document->GetNextMaskedPrimitive(MaskedPrimitivePosition);
          EoDbPrimitive* Primitive = MaskedPrimitive->GetPrimitive();
          DWORD Mask = MaskedPrimitive->GetMask();
          m_PreviewGroup.AddTail(Primitive->Copy(Primitive));
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

      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        EoGeVector3d Translate(pts[0], CurrentPnt);

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
    EoDbPrimitive* Primitive = MaskedPrimitive->GetPrimitive();
    m_PreviewGroup.AddTail(Primitive->Copy(Primitive));
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
    auto* Group = document->GetNextNodalGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      m_PreviewGroup.AddTail(Primitive->Copy(Primitive));
    }
  }
  auto UniquePointPosition = document->GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = document->GetNextUniquePoint(UniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 8, UniquePoint->m_Point));
  }
}
