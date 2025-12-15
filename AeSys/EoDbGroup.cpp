#include "stdafx.h"

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbBlockReference.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolyline.h"
#include "EoDbText.h"

EoDbPrimitive* EoDbGroup::sm_PrimitiveToIgnore = static_cast<EoDbPrimitive*>(nullptr);

EoDbGroup::EoDbGroup() {}
EoDbGroup::EoDbGroup(EoDbPrimitive* primitive) { AddTail(primitive); }
EoDbGroup::EoDbGroup(const EoDbGroup& group) {
  EoDbPrimitive* Primitive;

  POSITION position = group.GetHeadPosition();
  while (position != 0) { AddTail((group.GetNext(position))->Copy(Primitive)); }
}
EoDbPrimitive* EoDbGroup::GetAt(POSITION position) { return (EoDbPrimitive*)CObList::GetAt(position); }
EoDbGroup::EoDbGroup(const EoDbBlock& block) {
  EoDbPrimitive* Primitive;

  POSITION position = block.GetHeadPosition();
  while (position != 0) { AddTail((block.GetNext(position))->Copy(Primitive)); }
}
void EoDbGroup::AddPrimsToTreeViewControl(HWND tree, HTREEITEM parent) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->AddToTreeViewControl(tree, parent);
  }
}
HTREEITEM EoDbGroup::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  HTREEITEM TreeItem = tvAddItem(tree, parent, const_cast<LPWSTR>(L"<Group>"), this);
  AddPrimsToTreeViewControl(tree, TreeItem);
  return TreeItem;
}
void EoDbGroup::BreakPolylines() {
  POSITION Position = GetHeadPosition();
  while (Position != 0) {
    POSITION PrimitivePosition = Position;
    EoDbPrimitive* Primitive = GetNext(Position);
    if (Primitive->Is(EoDb::kPolylinePrimitive)) {
      EoInt16 nPenColor = Primitive->PenColor();
      EoInt16 LineType = Primitive->LineType();

      EoGePoint3dArray pts;
      static_cast<EoDbPolyline*>(Primitive)->GetAllPts(pts);

      for (EoUInt16 w = 0; w < pts.GetSize() - 1; w++)
        CObList::InsertBefore(PrimitivePosition, new EoDbLine(nPenColor, LineType, pts[w], pts[w + 1]));

      if (static_cast<EoDbPolyline*>(Primitive)->IsLooped())
        CObList::InsertBefore(PrimitivePosition, new EoDbLine(nPenColor, LineType, pts[pts.GetUpperBound()], pts[0]));

      this->RemoveAt(PrimitivePosition);
      delete Primitive;
    } else if (Primitive->Is(EoDb::kGroupReferencePrimitive)) {
      EoDbBlock* Block;
      if (AeSysDoc::GetDoc()->LookupBlock(static_cast<EoDbBlockReference*>(Primitive)->GetName(), Block) != 0) {
        Block->BreakPolylines();
      }
    }
  }
}
void EoDbGroup::BreakSegRefs() {
  int iSegRefs;
  do {
    iSegRefs = 0;
    POSITION Position = GetHeadPosition();
    while (Position != 0) {
      POSITION PrimitivePosition = Position;
      EoDbPrimitive* Primitive = GetNext(Position);
      if (Primitive->Is(EoDb::kGroupReferencePrimitive)) {
        iSegRefs++;
        EoDbBlock* Block;
        if (AeSysDoc::GetDoc()->LookupBlock(static_cast<EoDbBlockReference*>(Primitive)->GetName(), Block) != 0) {
          EoDbGroup* pSegT = new EoDbGroup(*Block);
          EoGePoint3d ptBase = Block->GetBasePt();
          EoGeTransformMatrix tm = static_cast<EoDbBlockReference*>(Primitive)->BuildTransformMatrix(ptBase);
          pSegT->Transform(tm);

          this->InsertBefore(PrimitivePosition, pSegT);
          this->RemoveAt(PrimitivePosition);
          delete Primitive;
          pSegT->RemoveAll();
          delete pSegT;
        }
      }
    }
  } while (iSegRefs != 0);
}
void EoDbGroup::Display(AeSysView* view, CDC* deviceContext) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->Display(view, deviceContext);
  }
}
POSITION EoDbGroup::FindAndRemovePrim(EoDbPrimitive* primitive) {
  POSITION Position = Find(primitive);

  if (Position != 0) RemoveAt(Position);

  return (Position);
}
EoDbPoint* EoDbGroup::GetFirstDifferentPoint(EoDbPoint* pointPrimitive) {
  POSITION Position = GetHeadPosition();
  while (Position != 0) {
    EoDbPrimitive* Primitive = GetNext(Position);
    if (Primitive != pointPrimitive && Primitive->Is(EoDb::kPointPrimitive)) {
      return (static_cast<EoDbPoint*>(Primitive));
    }
  }
  return 0;
}
void EoDbGroup::InsertBefore(POSITION insertPosition, EoDbGroup* group) {
  POSITION Position = group->GetHeadPosition();
  while (Position != 0) {
    EoDbPrimitive* Primitive = group->GetNext(Position);
    CObList::InsertBefore(insertPosition, (CObject*)Primitive);
  }
}
int EoDbGroup::GetBlockRefCount(const CString& strBlkNam) {
  int iCount = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->Is(EoDb::kGroupReferencePrimitive)) {
      if (static_cast<EoDbBlockReference*>(Primitive)->GetName() == strBlkNam) iCount++;
    }
  }
  return (iCount);
}
void EoDbGroup::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->GetExtents(view, ptMin, ptMax, tm);
  }
}
int EoDbGroup::GetLineTypeRefCount(EoInt16 lineType) {
  int iCount = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);

    if (Primitive->LineType() == lineType) { iCount++; }
  }
  return (iCount);
}
bool EoDbGroup::IsInView(AeSysView* view) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->IsInView(view)) { return true; }
  }
  return false;
}
bool EoDbGroup::SelectUsingLine(AeSysView* view, const EoGePoint3d& pt1, const EoGePoint3d& pt2) {
  EoGeLine Line(pt1, pt2);
  EoGePoint3dArray ptsInt;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->SelectUsingLine(view, Line, ptsInt)) { return true; }
  }
  return false;
}
bool EoDbGroup::SelectUsingPoint_(AeSysView* view, EoGePoint4d point) {
  EoGePoint3d ptSel;
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->SelectUsingPoint(view, point, ptSel)) { return true; }
  }
  return false;
}
bool EoDbGroup::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->SelectUsingRectangle(view, pt1, pt2)) { return true; }
  }
  return false;
}
void EoDbGroup::ModifyNotes(EoDbFontDefinition& fd, EoDbCharacterCellDefinition& ccd, int iAtt) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->Is(EoDb::kTextPrimitive)) { static_cast<EoDbText*>(Primitive)->ModifyNotes(fd, ccd, iAtt); }
  }
}
void EoDbGroup::ModifyPenColor(EoInt16 nPenColor) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->PenColor(nPenColor);
  }
}
void EoDbGroup::ModifyLineType(EoInt16 lineType) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->LineType(lineType);
  }
}
void EoDbGroup::PenTranslation(EoUInt16 wCols, EoInt16* pColNew, EoInt16* pCol) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);

    for (EoUInt16 w = 0; w < wCols; w++) {
      if (Primitive->PenColor() == pCol[w]) {
        Primitive->PenColor(pColNew[w]);
        break;
      }
    }
  }
}
void EoDbGroup::RemoveDuplicatePrimitives() {
  POSITION BasePosition = GetHeadPosition();
  while (BasePosition != 0) {
    EoDbPrimitive* BasePrimitive = GetNext(BasePosition);

    POSITION TestPosition = BasePosition;
    while (TestPosition != 0) {
      POSITION TestPositionSave = TestPosition;
      EoDbPrimitive* TestPrimitive = GetNext(TestPosition);

      if (BasePrimitive->Identical(TestPrimitive)) {
        RemoveAt(TestPositionSave);
        delete TestPrimitive;
      }
    }
  }
}
int EoDbGroup::RemoveEmptyNotesAndDelete() {
  int iCount = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    POSITION posPrev = position;
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->Is(EoDb::kTextPrimitive)) {
      if (static_cast<EoDbText*>(Primitive)->Text().GetLength() == 0) {
        RemoveAt(posPrev);
        delete Primitive;
        iCount++;
      }
    }
  }
  return (iCount);
}
EoDbPrimitive* EoDbGroup::SelPrimUsingPoint(AeSysView* view, const EoGePoint4d& point, double& dPicApert,
                                            EoGePoint3d& pDetPt) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);

    if (Primitive->SelectUsingPoint(view, point, pDetPt)) {
      dPicApert = point.DistanceToPointXY(EoGePoint4d(pDetPt));
      return (Primitive);
    }
  }
  return 0;
}
EoDbPrimitive* EoDbGroup::SelPrimAtCtrlPt(AeSysView* view, const EoGePoint4d& ptView, EoGePoint3d* ptCtrl) {
  EoDbPrimitive* EngagedPrimitive = 0;

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);

    if (Primitive == sm_PrimitiveToIgnore) { continue; }

    EoGePoint3d pt = Primitive->SelectAtControlPoint(view, ptView);

    if (EoDbPrimitive::ControlPointIndex() != USHRT_MAX) {
      EngagedPrimitive = Primitive;

      EoGePoint4d ptView4(pt);
      view->ModelViewTransformPoint(ptView4);
      *ptCtrl = ptView4;
    }
  }
  return (EngagedPrimitive);
}
void EoDbGroup::DeletePrimitivesAndRemoveAll() {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    delete (Primitive);
  }
  RemoveAll();
}
void EoDbGroup::SortTextOnY() {
  int iT;
  int iCount = (int)GetCount();

  do {
    iT = 0;

    POSITION position = GetHeadPosition();
    for (int i = 1; i < iCount; i++) {
      POSITION pos1 = position;
      EoDbPrimitive* pPrim1 = GetNext(pos1);

      POSITION pos2 = pos1;
      EoDbPrimitive* pPrim2 = GetNext(pos2);

      if (pPrim1->Is(EoDb::kTextPrimitive) && pPrim2->Is(EoDb::kTextPrimitive)) {
        double dY1 = static_cast<EoDbText*>(pPrim1)->ReferenceOrigin().y;
        double dY2 = static_cast<EoDbText*>(pPrim2)->ReferenceOrigin().y;
        if (dY1 < dY2) {
          SetAt(position, pPrim2);
          SetAt(pos1, pPrim1);
          iT = i;
        }
      } else if (pPrim1->Is(EoDb::kTextPrimitive) || pPrim2->Is(EoDb::kTextPrimitive)) {
        SetAt(position, pPrim2);
        SetAt(pos1, pPrim1);
        iT = i;
      }

      position = pos1;
    }
    iCount = iT;
  } while (iT != 0);
}
void EoDbGroup::Square(AeSysView* view) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    if (Primitive->Is(EoDb::kLinePrimitive)) { static_cast<EoDbLine*>(Primitive)->Square(view); }
  }
}
void EoDbGroup::Transform(EoGeTransformMatrix& tm) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->Transform(tm);
  }
}
void EoDbGroup::Translate(EoGeVector3d v) {
  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->Translate(v);
  }
}
void EoDbGroup::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(GetCount()));

  for (POSITION position = GetHeadPosition(); position != 0;) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->Write(file);
  }
}
void EoDbGroup::Write(CFile& file, EoByte* buffer) {
  // group flags
  buffer[0] = 0;
  // number of primitives in group
  *((EoInt16*)&buffer[1]) = EoInt16(GetCount());

  POSITION position = GetHeadPosition();
  while (position != 0) {
    EoDbPrimitive* Primitive = GetNext(position);
    Primitive->Write(file, buffer);
  }
}
