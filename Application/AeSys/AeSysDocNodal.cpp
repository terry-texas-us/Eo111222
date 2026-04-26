#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbMaskedPrimitive.h"
#include "EoDbPoint.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"
#include "EoGeUniquePoint.h"

void AeSysDoc::DeleteNodalResources() {
  auto uniquePointPosition = GetFirstUniquePointPosition();
  while (uniquePointPosition != nullptr) { delete GetNextUniquePoint(uniquePointPosition); }
  RemoveAllUniquePoints();
  auto maskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (maskedPrimitivePosition != nullptr) { delete GetNextMaskedPrimitive(maskedPrimitivePosition); }
  RemoveAllMaskedPrimitives();
  RemoveAllNodalGroups();
}
void AeSysDoc::UpdateNodalList(EoDbGroup* group, EoDbPrimitive* primitive, DWORD mask, int bit, EoGePoint3d point) {
  if (app.m_NodalModeAddGroups) {
    if (!btest(mask, bit)) {
      if (!FindNodalGroup(group)) { AddNodalGroup(group); }
      AddPrimitiveBit(primitive, bit);
      if (AddUniquePoint(point) == 1) {
        EoDbPoint pointPrimitive(252, 8, point);
        UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, &pointPrimitive);
      }
    }
  } else {
    if (btest(mask, bit)) {
      RemovePrimitiveBit(primitive, bit);

      if (RemoveUniquePoint(point) == 0) {
        EoDbPoint pointPrimitive(252, 8, point);
        UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, &pointPrimitive);
      }
    }
  }
}

int AeSysDoc::AddUniquePoint(const EoGePoint3d& point) {
  auto uniquePointPosition = GetFirstUniquePointPosition();
  while (uniquePointPosition != nullptr) {
    EoGeUniquePoint* uniquePoint = GetNextUniquePoint(uniquePointPosition);
    if (point == uniquePoint->m_Point) {
      (uniquePoint->m_References)++;
      return (uniquePoint->m_References);
    }
  }
  AddUniquePoint(new EoGeUniquePoint(point, 1));
  return (1);
}
void AeSysDoc::DisplayUniquePoints() {
  EoDbGroup group;
  auto uniquePointPosition = GetFirstUniquePointPosition();
  while (uniquePointPosition != nullptr) {
    EoGeUniquePoint* uniquePoint = GetNextUniquePoint(uniquePointPosition);
    group.AddTail(new EoDbPoint(252, 8, uniquePoint->m_Point));
  }
  UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &group);
  group.DeletePrimitivesAndRemoveAll();
}
int AeSysDoc::RemoveUniquePoint(const EoGePoint3d& point) {
  int references{};

  auto uniquePointPosition = GetFirstUniquePointPosition();
  while (uniquePointPosition != nullptr) {
    auto position = uniquePointPosition;
    EoGeUniquePoint* uniquePoint = GetNextUniquePoint(uniquePointPosition);
    if (point == uniquePoint->m_Point) {
      references = --(uniquePoint->m_References);

      if (references == 0) {
        RemoveUniquePointAt(position);
        delete uniquePoint;
      }
      break;
    }
  }
  return references;
}
void AeSysDoc::AddPrimitiveBit(EoDbPrimitive* primitive, int bit) {
  EoDbMaskedPrimitive* maskedPrimitive{};

  auto maskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (maskedPrimitivePosition != nullptr) {
    auto posCur = maskedPrimitivePosition;
    maskedPrimitive = GetNextMaskedPrimitive(maskedPrimitivePosition);
    if (maskedPrimitive->GetPrimitive() == primitive) {
      maskedPrimitivePosition = posCur;
      break;
    }
  }
  if (maskedPrimitivePosition == nullptr) {
    maskedPrimitive = new EoDbMaskedPrimitive(primitive, 0);
    AddMaskedPrimitive(maskedPrimitive);
  }
  maskedPrimitive->SetMaskBit(bit);
}
void AeSysDoc::RemovePrimitiveBit(EoDbPrimitive* primitive, int bit) {
  EoDbMaskedPrimitive* maskedPrimitive{};

  auto maskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (maskedPrimitivePosition != nullptr) {
    auto posCur = maskedPrimitivePosition;
    maskedPrimitive = GetNextMaskedPrimitive(maskedPrimitivePosition);
    if (maskedPrimitive->GetPrimitive() == primitive) {
      maskedPrimitivePosition = posCur;
      break;
    }
  }
  if (maskedPrimitivePosition != nullptr) { maskedPrimitive->ClearMaskBit(bit); }
}
DWORD AeSysDoc::GetPrimitiveMask(EoDbPrimitive* primitive) {
  EoDbMaskedPrimitive* maskedPrimitive = nullptr;

  auto maskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (maskedPrimitivePosition != nullptr) {
    auto posCur = maskedPrimitivePosition;
    maskedPrimitive = GetNextMaskedPrimitive(maskedPrimitivePosition);
    if (maskedPrimitive->GetPrimitive() == primitive) {
      maskedPrimitivePosition = posCur;
      break;
    }
  }
  return ((maskedPrimitivePosition != nullptr) ? maskedPrimitive->GetMask() : 0UL);
}
