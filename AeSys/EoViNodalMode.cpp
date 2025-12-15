#include "stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "PrimState.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

double NodalModePickTolerance = 0.05;
EoUInt16 PreviousNodalCommand = 0;
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
  EoGePoint3d CurrentPnt = GetCursorPosition();

  POSITION GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);

    POSITION PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

      DWORD Mask = GetDocument()->GetPrimitiveMask(Primitive);
      Primitive->GetAllPts(pts);

      for (int i = 0; i < pts.GetSize(); i++) {
        if (EoGeVector3d(pts[i], CurrentPnt).Length() <= NodalModePickTolerance) {
          GetDocument()->UpdateNodalList(Group, Primitive, Mask, i, pts[i]);
        }
      }
    }
  }
  pts.RemoveAll();
}
void AeSysView::OnNodalModeLine() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  EoDbGroup* Group = SelectGroupAndPrimitive(CurrentPnt);
  if (Group != 0) {
    EoDbPrimitive* Primitive = EngagedPrimitive();

    DWORD Mask = GetDocument()->GetPrimitiveMask(Primitive);
    Primitive->GetAllPts(pts);

    for (int i = 0; i < pts.GetSize(); i++) { GetDocument()->UpdateNodalList(Group, Primitive, Mask, i, pts[i]); }
    pts.RemoveAll();
  }
}
void AeSysView::OnNodalModeArea() {
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

      POSITION GroupPosition = GetFirstVisibleGroupPosition();
      while (GroupPosition != 0) {
        EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);

        POSITION PrimitivePosition = Group->GetHeadPosition();
        while (PrimitivePosition != 0) {
          EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
          DWORD Mask = GetDocument()->GetPrimitiveMask(Primitive);
          Primitive->GetAllPts(pts);

          for (int i = 0; i < pts.GetSize(); i++) {
            if (pts[i].IsContained(MinExtent, MaxExtent)) {
              GetDocument()->UpdateNodalList(Group, Primitive, Mask, i, pts[i]);
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

      EoDbGroup* Group = new EoDbGroup;

      POSITION PointPosition = GetDocument()->GetFirstUniquePointPosition();
      while (PointPosition != 0) {
        EoGeUniquePoint* UniquePoint = GetDocument()->GetNextUniquePoint(PointPosition);
        EoDbLine* Primitive = new EoDbLine(UniquePoint->m_Point, UniquePoint->m_Point + Translate);
        Group->AddTail(Primitive);
      }
      GetDocument()->AddWorkLayerGroup(Group);
      GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);

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

      POSITION GroupPosition = GetDocument()->GetFirstNodalGroupPosition();
      while (GroupPosition != 0) {
        EoDbGroup* Group = GetDocument()->GetNextNodalGroup(GroupPosition);

        POSITION PrimitivePosition = Group->GetHeadPosition();
        while (PrimitivePosition != 0) {
          EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

          DWORD Mask = GetDocument()->GetPrimitiveMask(Primitive);
          if (Mask != 0) {
            if (Primitive->Is(EoDb::kLinePrimitive)) {
              if ((Mask & 3) == 3) {
                EoDbLine* LinePrimitive = static_cast<EoDbLine*>(Primitive);

                pts[0] = LinePrimitive->BeginPoint();
                pts[1] = LinePrimitive->EndPoint();
                pts[2] = pts[1] + Translate;
                pts[3] = pts[0] + Translate;

                EoDbGroup* NewGroup = new EoDbGroup(new EoDbPolygon(pts));
                GetDocument()->AddWorkLayerGroup(NewGroup);
                GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, NewGroup);
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
                  GetDocument()->AddWorkLayerGroup(NewGroup);
                  GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, NewGroup);
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
    auto mask = GetDocument()->GetPrimitiveMask(EngagedPrimitive());
    EoGePoint3dArray points;

    EngagedPrimitive()->GetAllPts(points);

    for (int i = 0; i < points.GetSize(); i++) {
      GetDocument()->UpdateNodalList(EngagedGroup(), EngagedPrimitive(), mask, i, points[i]);
    }
  }
}
void AeSysView::OnNodalModeReturn() {
  EoGePoint3d CurrentPnt = GetCursorPosition();

  switch (PreviousNodalCommand) {
    case ID_OP4:
      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        EoGeVector3d Translate(pts[0], CurrentPnt);

        POSITION MaskedPrimitivePosition = GetDocument()->GetFirstMaskedPrimitivePosition();
        while (MaskedPrimitivePosition != 0) {
          EoDbMaskedPrimitive* MaskedPrimitive = GetDocument()->GetNextMaskedPrimitive(MaskedPrimitivePosition);
          EoDbPrimitive* Primitive = MaskedPrimitive->GetPrimitive();
          DWORD Mask = MaskedPrimitive->GetMask();
          Primitive->TranslateUsingMask(Translate, Mask);
        }
        EoGeUniquePoint* Point;

        POSITION UniquePointPosition = GetDocument()->GetFirstUniquePointPosition();
        while (UniquePointPosition != 0) {
          Point = GetDocument()->GetNextUniquePoint(UniquePointPosition);
          Point->m_Point += Translate;
        }
        SetCursorPosition(CurrentPnt);
      }
      break;

    case ID_OP5:
      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        EoGeVector3d Translate(pts[0], CurrentPnt);

        POSITION GroupPosition = GetDocument()->GetFirstNodalGroupPosition();
        while (GroupPosition != 0) {
          EoDbGroup* Group = GetDocument()->GetNextNodalGroup(GroupPosition);
          GetDocument()->AddWorkLayerGroup(new EoDbGroup(*Group));
          GetDocument()->GetLastWorkLayerGroup()->Translate(Translate);
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
  if (PreviousNodalCommand == 0) {
    GetDocument()->DisplayUniquePoints();
    GetDocument()->DeleteNodalResources();
  } else {
    RubberBandingDisable();
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);

    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    ConstructPreviewGroup();
    GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupSafe, &m_PreviewGroup);
    m_PreviewGroup.DeletePrimitivesAndRemoveAll();
    pts.RemoveAll();

    ModeLineUnhighlightOp(PreviousNodalCommand);
  }
}
void AeSysView::DoNodalModeMouseMove() {
  EoGePoint3d CurrentPnt = GetCursorPosition();
  INT_PTR NumberOfPoints = pts.GetSize();

  switch (PreviousNodalCommand) {
    case ID_OP4:
      VERIFY(pts.GetSize() > 0);

      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        EoGeVector3d Translate(pts[0], CurrentPnt);

        GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();

        POSITION MaskedPrimitivePosition = GetDocument()->GetFirstMaskedPrimitivePosition();
        while (MaskedPrimitivePosition != 0) {
          EoDbMaskedPrimitive* MaskedPrimitive = GetDocument()->GetNextMaskedPrimitive(MaskedPrimitivePosition);
          EoDbPrimitive* Primitive = MaskedPrimitive->GetPrimitive();
          DWORD Mask = MaskedPrimitive->GetMask();
          m_PreviewGroup.AddTail(Primitive->Copy(Primitive));
          ((EoDbPrimitive*)m_PreviewGroup.GetTail())->TranslateUsingMask(Translate, Mask);
        }
        POSITION UniquePointPosition = GetDocument()->GetFirstUniquePointPosition();
        while (UniquePointPosition != 0) {
          EoGeUniquePoint* UniquePoint = GetDocument()->GetNextUniquePoint(UniquePointPosition);
          EoGePoint3d Point = (UniquePoint->m_Point) + Translate;
          m_PreviewGroup.AddTail(new EoDbPoint(252, 8, Point));
        }
        GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;

    case ID_OP5:

      if (pts[0] != CurrentPnt) {
        CurrentPnt = SnapPointToAxis(pts[0], CurrentPnt);
        pts.Add(CurrentPnt);

        EoGeVector3d Translate(pts[0], CurrentPnt);

        GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
        m_PreviewGroup.DeletePrimitivesAndRemoveAll();
        ConstructPreviewGroupForNodalGroups();
        m_PreviewGroup.Translate(Translate);
        GetDocument()->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &m_PreviewGroup);
      }
      break;
  }
  pts.SetSize(NumberOfPoints);
}

void AeSysView::ConstructPreviewGroup() {
  m_PreviewGroup.DeletePrimitivesAndRemoveAll();

  POSITION MaskedPrimitivePosition = GetDocument()->GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    EoDbMaskedPrimitive* MaskedPrimitive = GetDocument()->GetNextMaskedPrimitive(MaskedPrimitivePosition);
    EoDbPrimitive* Primitive = MaskedPrimitive->GetPrimitive();
    m_PreviewGroup.AddTail(Primitive->Copy(Primitive));
  }
  POSITION UniquePointPosition = GetDocument()->GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = GetDocument()->GetNextUniquePoint(UniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 8, UniquePoint->m_Point));
  }
}
void AeSysView::ConstructPreviewGroupForNodalGroups() {
  POSITION GroupPosition = GetDocument()->GetFirstNodalGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetDocument()->GetNextNodalGroup(GroupPosition);

    POSITION PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      m_PreviewGroup.AddTail(Primitive->Copy(Primitive));
    }
  }
  POSITION UniquePointPosition = GetDocument()->GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = GetDocument()->GetNextUniquePoint(UniquePointPosition);
    m_PreviewGroup.AddTail(new EoDbPoint(252, 8, UniquePoint->m_Point));
  }
}
