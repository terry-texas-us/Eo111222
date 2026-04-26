#include "Stdafx.h"

#include <climits>
#include <cstdint>
#include <cstring>

#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbAttrib.h"
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
EoDbGroup::EoDbGroup(EoDbPrimitive* primitive) {
  AddTail(primitive);
}

EoDbGroup::EoDbGroup(const EoDbGroup& group) {
  ATLTRACE2(traceGeneral,
      3,
      L"EoDbGroup(copy from group): this=%p, source=%p, count=%d\n",
      this,
      &group,
      static_cast<int>(group.GetCount()));

  EoDbPrimitive* primitive{};

  auto position = group.GetHeadPosition();
  while (position != nullptr) { AddTail((group.GetNext(position))->Copy(primitive)); }
}

EoDbPrimitive* EoDbGroup::GetAt(POSITION position) {
  return static_cast<EoDbPrimitive*>(CObList::GetAt(position));
}

EoDbGroup::EoDbGroup(const EoDbBlock& block) {
  ATLTRACE2(traceGeneral,
      3,
      L"EoDbGroup(copy from block): this=%p, source=%p, count=%d\n",
      this,
      &block,
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
  const auto treeItem = tvAddItem(tree, parent, L"<Group>", this);
  AddPrimsToTreeViewControl(tree, treeItem);
  return treeItem;
}

void EoDbGroup::BreakPolylines() {
  auto* document = AeSysDoc::GetDoc();
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto primitivePosition = position;
    auto* primitive = GetNext(position);
    if (primitive->Is(EoDb::kPolylinePrimitive)) {
      const auto color = primitive->Color();
      const auto& lineTypeName = primitive->LineTypeName();
      const auto lineWeight = primitive->LineWeight();

      EoGePoint3dArray points;
      static_cast<EoDbPolyline*>(primitive)->GetAllPoints(points);

      if (points.GetSize() >= 2) {
        for (auto i = 0; i < points.GetSize() - 1; i++) {
          auto* line = EoDbLine::CreateLine(points[i], points[i + 1])->WithProperties(color, lineTypeName, lineWeight);
          document->RegisterHandle(line);
          CObList::InsertBefore(primitivePosition, line);
        }

        if (static_cast<EoDbPolyline*>(primitive)->IsLooped()) {
          auto* line = EoDbLine::CreateLine(points[points.GetUpperBound()], points[0])
                           ->WithProperties(color, lineTypeName, lineWeight);
          document->RegisterHandle(line);
          CObList::InsertBefore(primitivePosition, line);
        }
      }
      this->RemoveAt(primitivePosition);
      document->UnregisterHandle(primitive->Handle());
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
  auto* document = AeSysDoc::GetDoc();
  int numberOfGroupReferences{};
  do {
    numberOfGroupReferences = 0;
    auto position = GetHeadPosition();
    while (position != nullptr) {
      auto primitivePosition = position;
      auto* primitive = GetNext(position);
      if (primitive->Is(EoDb::kGroupReferencePrimitive)) {
        EoDbBlock* block{};
        if (document->LookupBlock(static_cast<EoDbBlockReference*>(primitive)->BlockName(), block) != 0) {
          numberOfGroupReferences++;
          auto* blockReference = static_cast<EoDbBlockReference*>(primitive);

          // Convert in-group ATTRIBs to plain EoDbText primitives
          for (const auto attribHandle : blockReference->AttributeHandles()) {
            auto attribPos = Find(document->FindPrimitiveByHandle(attribHandle));
            if (attribPos != nullptr) {
              auto* attribPrimitive = static_cast<EoDbAttrib*>(GetAt(attribPos));

              // Build a plain EoDbText with the same visual properties
              EoDbFontDefinition fontDef;
              attribPrimitive->GetFontDef(fontDef);
              EoGeReferenceSystem refSys;
              attribPrimitive->GetRefSys(refSys);
              auto* textPrimitive = new EoDbText(fontDef, refSys, attribPrimitive->Text());
              textPrimitive->SetColor(attribPrimitive->Color());
              textPrimitive->SetLineTypeName(attribPrimitive->LineTypeName());
              textPrimitive->SetLayerName(attribPrimitive->LayerName());
              textPrimitive->SetLineWeight(attribPrimitive->LineWeight());
              textPrimitive->SetLineTypeScale(attribPrimitive->LineTypeScale());
              textPrimitive->SetTextGenerationFlags(attribPrimitive->TextGenerationFlags());
              textPrimitive->SetExtrusion(attribPrimitive->Extrusion());
              if (attribPrimitive->IsFromMText()) {
                textPrimitive->SetMTextProperties(*attribPrimitive->MTextProperties());
              }

              document->RegisterHandle(textPrimitive);
              CObList::SetAt(attribPos, textPrimitive);
              document->UnregisterHandle(attribPrimitive->Handle());
              delete attribPrimitive;
            }
          }

          auto* temporaryGroupTransformed = new EoDbGroup(*block);
          auto basePoint = block->BasePoint();
          EoGeTransformMatrix transformMatrix = blockReference->BuildTransformMatrix(basePoint);
          temporaryGroupTransformed->Transform(transformMatrix);
          document->RegisterGroupHandles(temporaryGroupTransformed);
          this->InsertBefore(primitivePosition, temporaryGroupTransformed);
          this->RemoveAt(primitivePosition);
          document->UnregisterHandle(primitive->Handle());
          delete primitive;
          temporaryGroupTransformed->RemoveAll();
          delete temporaryGroupTransformed;
        }
      }
    }
  } while (numberOfGroupReferences != 0);
}

void EoDbGroup::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->Display(view, renderDevice);
  }
}
bool EoDbGroup::FindAndRemovePrim(EoDbPrimitive* primitive) {
  auto position = Find(primitive);

  if (position != nullptr) {
    RemoveAt(position);
    return true;
  }
  return false;
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
    CObList::InsertBefore(insertPosition, static_cast<CObject*>(primitive));
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

void EoDbGroup::GetExtents(AeSysView* view,
    EoGePoint3d& ptMin,
    EoGePoint3d& ptMax,
    const EoGeTransformMatrix& transformMatrix) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->GetExtents(view, ptMin, ptMax, transformMatrix);
  }
}

int EoDbGroup::GetLineTypeRefCount(const std::wstring& lineTypeName) {
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);

    if (_wcsicmp(primitive->LineTypeName().c_str(), lineTypeName.c_str()) == 0) { count++; }
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
  const EoGeLine line(pt1, pt2);
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
    const EoDbCharacterCellDefinition& characterCellDefinition,
    int attributes) {
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
void EoDbGroup::ModifyLineType(const std::wstring& lineTypeName) {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->SetLineTypeName(lineTypeName);
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
  auto* document = AeSysDoc::GetDoc();
  auto basePosition = GetHeadPosition();
  while (basePosition != nullptr) {
    auto* basePrimitive = GetNext(basePosition);

    auto testPosition = basePosition;
    while (testPosition != nullptr) {
      auto testPositionSave = testPosition;
      auto* testPrimitive = GetNext(testPosition);

      if (basePrimitive->Identical(testPrimitive)) {
        RemoveAt(testPositionSave);
        document->UnregisterHandle(testPrimitive->Handle());
        delete testPrimitive;
      }
    }
  }
}

int EoDbGroup::RemoveEmptyNotesAndDelete() {
  auto* document = AeSysDoc::GetDoc();
  int count{};

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto posPrev = position;
    auto* primitive = GetNext(position);
    if (primitive->Is(EoDb::kTextPrimitive)) {
      if (static_cast<EoDbText*>(primitive)->Text().GetLength() == 0) {
        RemoveAt(posPrev);
        document->UnregisterHandle(primitive->Handle());
        delete primitive;
        count++;
      }
    }
  }
  return count;
}

EoDbPrimitive* EoDbGroup::SelPrimUsingPoint(AeSysView* view,
    const EoGePoint4d& point,
    double& selectionDistance,
    EoGePoint3d& selectionPoint) {
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
 * This method iterates through all primitives in the group, deletes each primitive, and then removes all primitives
 * from the group.
 */
void EoDbGroup::DeletePrimitivesAndRemoveAll() {
  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    delete (primitive);
  }
  RemoveAll();
}

void EoDbGroup::SortTextOnY() {
  int lastSwappedIndex{};
  int iCount = static_cast<int>(GetCount());

  do {
    lastSwappedIndex = 0;
    auto position = GetHeadPosition();
    for (int i = 1; i < iCount; i++) {
      auto firstPosition = position;
      auto* firstPrimitive = GetNext(firstPosition);

      auto secondPosition = firstPosition;
      auto* secondPrimitive = GetNext(secondPosition);
      if (firstPrimitive->Is(EoDb::kTextPrimitive) && secondPrimitive->Is(EoDb::kTextPrimitive)) {
        const double firstTextPrimitiveY = static_cast<EoDbText*>(firstPrimitive)->ReferenceOrigin().y;
        double secondTextPrimitiveY = static_cast<EoDbText*>(secondPrimitive)->ReferenceOrigin().y;
        if (firstTextPrimitiveY < secondTextPrimitiveY) {
          SetAt(position, secondPrimitive);
          SetAt(firstPosition, firstPrimitive);
          lastSwappedIndex = i;
        }
      } else if (!firstPrimitive->Is(EoDb::kTextPrimitive) && secondPrimitive->Is(EoDb::kTextPrimitive)) {
        // Bubble text primitives before non-text primitives
        SetAt(position, secondPrimitive);
        SetAt(firstPosition, firstPrimitive);
        lastSwappedIndex = i;
      }

      position = firstPosition;
    }
    iCount = lastSwappedIndex;
  } while (lastSwappedIndex != 0);
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

void EoDbGroup::Write(CFile& file, EoDb::PegFileVersion fileVersion) {
  EoDb::WriteUInt16(file, std::uint16_t(GetCount()));

  for (auto position = GetHeadPosition(); position != nullptr;) {
    auto* primitive = GetNext(position);
    primitive->Write(file);
    if (fileVersion == EoDb::PegFileVersion::AE2026) {
      EoDb::WriteUInt64(file, primitive->Handle());
      EoDb::WriteUInt64(file, primitive->OwnerHandle());
      EoDb::WriteInt16(file, EoDxfLineWeights::LineWeightToDxfIndex(primitive->LineWeight()));
      EoDb::WriteDouble(file, primitive->LineTypeScale());
      primitive->WriteV2Extension(file);
    }
  }
}

void EoDbGroup::Write(CFile& file, std::uint8_t* buffer) {
  // group flags
  buffer[0] = 0;
  // number of primitives in group
  const auto primitiveCount = static_cast<std::int16_t>(GetCount());
  std::memcpy(&buffer[1], &primitiveCount, sizeof(primitiveCount));

  auto position = GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = GetNext(position);
    primitive->Write(file, buffer);
  }
}
