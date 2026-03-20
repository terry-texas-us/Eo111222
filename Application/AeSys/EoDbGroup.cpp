#include "Stdafx.h"

#include <climits>
#include <cstdint>
#include <cstring>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

EoDbPrimitive* EoDbGroup::sm_PrimitiveToIgnore = static_cast<EoDbPrimitive*>(nullptr);

EoDbGroup::EoDbGroup() {}
EoDbGroup::EoDbGroup(EoDbPrimitive* primitive) { AddTail(primitive); }

EoDbGroup::EoDbGroup(const EoDbGroup& group) {
  ATLTRACE2(traceGeneral, 3, L"EoDbGroup(copy from group): this=%p, source=%p, count=%d\n", this, &group,
      static_cast<int>(group.GetCount()));

  EoDbPrimitive* primitive{};

  auto position = group.GetHeadPosition();
  while (position != nullptr) { AddTail((group.GetNext(position))->Copy(primitive)); }
}

EoDbPrimitive* EoDbGroup::GetAt(POSITION position) { return (EoDbPrimitive*)CObList::GetAt(position); }

EoDbGroup::EoDbGroup(const EoDbBlock& block) {
  ATLTRACE2(traceGeneral, 3, L"EoDbGroup(copy from block): this=%p, source=%p, count=%d\n", this, &block,
      static_cast<int>(block.GetCount()));

  EoDbPrimitive* primitive{};

  auto position = block.GetHeadPosition();
  while (position != nullptr) { AddTail((block.GetNext(position))->Copy(primitive)); }
}

void EoDbGroup::AddPrimsToTreeViewControl(HWND tree, HTREEITEM parent) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->AddToTreeViewControl(tree, parent);
  }
}

HTREEITEM EoDbGroup::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Group>"};
  auto treeItem = tvAddItem(tree, parent, label.GetBuffer(), this);
  AddPrimsToTreeViewControl(tree, treeItem);
  return treeItem;
}

void EoDbGroup::BreakPolylines() {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto PrimitivePosition = position;
    auto* primitive = GetNext(position);
    if (primitive->Is(EoDb::kPolylinePrimitive)) {
      auto color = primitive->Color();
      auto lineTypeIndex = primitive->LineTypeIndex();

      EoGePoint3dArray points;
      static_cast<EoDbPolyline*>(primitive)->GetAllPoints(points);

      if (points.GetSize() >= 2) {
        for (auto i = 0; i < points.GetSize() - 1; i++) {
          CObList::InsertBefore(
              PrimitivePosition, EoDbLine::CreateLine(points[i], points[i + 1])->WithProperties(color, lineTypeIndex));
        }

        if (static_cast<EoDbPolyline*>(primitive)->IsLooped()) {
          CObList::InsertBefore(PrimitivePosition,
              EoDbLine::CreateLine(points[points.GetUpperBound()], points[0])->WithProperties(color, lineTypeIndex));
        }
      }
      this->RemoveAt(PrimitivePosition);
      delete primitive;
    } else if (primitive->Is(EoDb::kGroupReferencePrimitive)) {
      EoDbBlock* block{};
      if (AeSysDoc::GetDoc()->LookupBlock(static_cast<EoDbBlockReference*>(primitive)->BlockName(), block) != 0) {
        block->BreakPolylines();
      }
    }
  }
}

void EoDbGroup::ExplodeBlockReferences() {
  int numberOfGroupReferences{};
  do {
    numberOfGroupReferences = 0;
    auto position = GetHeadPosition();
    while (position != nullptr) {
      auto primitivePosition = position;
      auto* primitive = GetNext(position);
      if (primitive->Is(EoDb::kGroupReferencePrimitive)) {
        EoDbBlock* block{};
        if (AeSysDoc::GetDoc()->LookupBlock(static_cast<EoDbBlockReference*>(primitive)->BlockName(), block) != 0) {
          numberOfGroupReferences++;
          auto* temporaryGroupTransformed = new EoDbGroup(*block);
          auto basePoint = block->BasePoint();
          EoGeTransformMatrix transformMatrix =
              static_cast<EoDbBlockReference*>(primitive)->BuildTransformMatrix(basePoint);
          temporaryGroupTransformed->Transform(transformMatrix);
          this->InsertBefore(primitivePosition, temporaryGroupTransformed);
          this->RemoveAt(primitivePosition);
          delete primitive;
          temporaryGroupTransformed->RemoveAll();
          delete temporaryGroupTransformed;
        }
      }
    }
  } while (numberOfGroupReferences != 0);
}

void EoDbGroup::Display(AeSysView* view, CDC* deviceContext) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->Display(view, deviceContext);
  }
}
POSITION EoDbGroup::FindAndRemovePrim(EoDbPrimitive* primitive) {
  auto position = Find(primitive);

  if (position != nullptr) { RemoveAt(position); }

  return position;
}

EoDbPoint* EoDbGroup::GetFirstDifferentPoint(EoDbPoint* pointPrimitive) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive != pointPrimitive && primitive->Is(EoDb::kPointPrimitive)) {
      return (static_cast<EoDbPoint*>(primitive));
    }
  }
  return nullptr;
}

void EoDbGroup::InsertBefore(POSITION insertPosition, EoDbGroup* group) {
  auto position = group->GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = group->GetNext(position);
    CObList::InsertBefore(insertPosition, (CObject*)primitive);
  }
}

int EoDbGroup::GetBlockRefCount(const CString& blockName) {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive->Is(EoDb::kGroupReferencePrimitive)) {
      if (static_cast<EoDbBlockReference*>(primitive)->BlockName() == blockName) { count++; }
    }
  }
  return count;
}

void EoDbGroup::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->GetExtents(view, ptMin, ptMax, transformMatrix);
  }
}

int EoDbGroup::GetLineTypeRefCount(std::int16_t lineType) {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);

    if (primitive->LineTypeIndex() == lineType) { count++; }
  }
  return count;
}
bool EoDbGroup::IsInView(AeSysView* view) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive->IsInView(view)) { return true; }
  }
  return false;
}

bool EoDbGroup::SelectUsingLine(AeSysView* view, const EoGePoint3d& pt1, const EoGePoint3d& pt2) {
  EoGeLine line(pt1, pt2);
  EoGePoint3dArray intersections;

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive->SelectUsingLine(view, line, intersections)) { return true; }
  }
  return false;
}

bool EoDbGroup::SelectUsingPoint_(AeSysView* view, EoGePoint4d point) {
  EoGePoint3d selectionPoint;
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive->SelectUsingPoint(view, point, selectionPoint)) { return true; }
  }
  return false;
}
bool EoDbGroup::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive->SelectUsingRectangle(view, pt1, pt2)) { return true; }
  }
  return false;
}
void EoDbGroup::ModifyNotes(const EoDbFontDefinition& fontDefinition,
    const EoDbCharacterCellDefinition& characterCellDefinition, int attributes) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive->Is(EoDb::kTextPrimitive)) {
      static_cast<EoDbText*>(primitive)->ModifyNotes(fontDefinition, characterCellDefinition, attributes);
    }
  }
}
void EoDbGroup::ModifyColor(std::int16_t color) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->SetColor(color);
  }
}
void EoDbGroup::ModifyLineType(std::int16_t lineType) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->SetLineTypeIndex(lineType);
  }
}
void EoDbGroup::PenTranslation(std::uint16_t wCols, std::int16_t* pColNew, std::int16_t* pCol) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);

    for (auto i = 0; i < wCols; i++) {
      if (primitive->Color() == pCol[i]) {
        primitive->SetColor(pColNew[i]);
        break;
      }
    }
  }
}

void EoDbGroup::RemoveDuplicatePrimitives() {
  auto basePosition = GetHeadPosition();
  while (basePosition != nullptr) {
    auto* basePrimitive = GetNext(basePosition);

    auto testPosition = basePosition;
    while (testPosition != nullptr) {
      auto testPositionSave = testPosition;
      auto* testPrimitive = GetNext(testPosition);

      if (basePrimitive->Identical(testPrimitive)) {
        RemoveAt(testPositionSave);
        delete testPrimitive;
      }
    }
  }
}

int EoDbGroup::RemoveEmptyNotesAndDelete() {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto posPrev = position;
    auto* primitive = GetNext(position);
    if (primitive->Is(EoDb::kTextPrimitive)) {
      if (static_cast<EoDbText*>(primitive)->Text().GetLength() == 0) {
        RemoveAt(posPrev);
        delete primitive;
        count++;
      }
    }
  }
  return count;
}

EoDbPrimitive* EoDbGroup::SelPrimUsingPoint(
    AeSysView* view, const EoGePoint4d& point, double& selectionDistance, EoGePoint3d& selectionPoint) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);

    if (primitive->SelectUsingPoint(view, point, selectionPoint)) {
      selectionDistance = point.DistanceToPointXY(EoGePoint4d(selectionPoint));
      return primitive;
    }
  }
  return nullptr;
}

EoDbPrimitive* EoDbGroup::SelPrimAtCtrlPt(AeSysView* view, const EoGePoint4d& ptView, EoGePoint3d* ptCtrl) {
  EoDbPrimitive* engagedPrimitive{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);

    if (primitive == sm_PrimitiveToIgnore) { continue; }

    EoGePoint3d pt = primitive->SelectAtControlPoint(view, ptView);

    if (EoDbPrimitive::ControlPointIndex() != SHRT_MAX) {
      engagedPrimitive = primitive;

      EoGePoint4d ndcPoint(pt);
      view->ModelViewTransformPoint(ndcPoint);
      *ptCtrl = EoGePoint3d{ndcPoint};
    }
  }
  return engagedPrimitive;
}
/** @brief Deletes all primitives in the group and removes them from the group.
 *
 * This method iterates through all primitives in the group, deletes each primitive, and then removes all primitives from the group.
 */
void EoDbGroup::DeletePrimitivesAndRemoveAll() {
  int deleted{};
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    deleted++;
    delete (primitive);
  }
  RemoveAll();
}

void EoDbGroup::SortTextOnY() {
  int iT;
  int iCount = (int)GetCount();

  do {
    iT = 0;

    auto position = GetHeadPosition();
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
      } else if (!pPrim1->Is(EoDb::kTextPrimitive) && pPrim2->Is(EoDb::kTextPrimitive)) {
        // Bubble text primitives before non-text primitives
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
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    if (primitive->Is(EoDb::kLinePrimitive)) { static_cast<EoDbLine*>(primitive)->Square(view); }
  }
}
void EoDbGroup::Transform(const EoGeTransformMatrix& transformMatrix) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->Transform(transformMatrix);
  }
}
void EoDbGroup::Translate(EoGeVector3d v) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->Translate(v);
  }
}
void EoDbGroup::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(GetCount()));

  for (auto position = GetHeadPosition(); position != nullptr;) {
    auto* primitive = GetNext(position);
    primitive->Write(file);
  }
}

void EoDbGroup::Write(CFile& file, std::uint8_t* buffer) {
  // group flags
  buffer[0] = 0;
  // number of primitives in group
  auto primitiveCount = static_cast<std::int16_t>(GetCount());
  std::memcpy(&buffer[1], &primitiveCount, sizeof(primitiveCount));

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->Write(file, buffer);
  }
}
