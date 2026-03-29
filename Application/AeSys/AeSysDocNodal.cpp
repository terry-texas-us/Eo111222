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
  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) { delete GetNextUniquePoint(UniquePointPosition); }
  RemoveAllUniquePoints();
  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) { delete GetNextMaskedPrimitive(MaskedPrimitivePosition); }
  RemoveAllMaskedPrimitives();
  RemoveAllNodalGroups();
}
void AeSysDoc::UpdateNodalList(EoDbGroup* group, EoDbPrimitive* primitive, DWORD mask, int bit, EoGePoint3d point) {
  if (app.m_NodalModeAddGroups) {
    if (!btest(mask, bit)) {
      if (!FindNodalGroup(group)) { AddNodalGroup(group); }
      AddPrimitiveBit(primitive, bit);
      if (AddUniquePoint(point) == 1) {
        EoDbPoint PointPrimitive(252, 8, point);
        UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, &PointPrimitive);
      }
    }
  } else {
    if (btest(mask, bit)) {
      RemovePrimitiveBit(primitive, bit);

      if (RemoveUniquePoint(point) == 0) {
        EoDbPoint PointPrimitive(252, 8, point);
        UpdateAllViews(nullptr, EoDb::kPrimitiveEraseSafe, &PointPrimitive);
      }
    }
  }
}

int AeSysDoc::AddUniquePoint(const EoGePoint3d& point) {
  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = GetNextUniquePoint(UniquePointPosition);
    if (point == UniquePoint->m_Point) {
      (UniquePoint->m_References)++;
      return (UniquePoint->m_References);
    }
  }
  AddUniquePoint(new EoGeUniquePoint(point, 1));
  return (1);
}
void AeSysDoc::DisplayUniquePoints() {
  EoDbGroup Group;
  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    EoGeUniquePoint* UniquePoint = GetNextUniquePoint(UniquePointPosition);
    Group.AddTail(new EoDbPoint(252, 8, UniquePoint->m_Point));
  }
  UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, &Group);
  Group.DeletePrimitivesAndRemoveAll();
}
int AeSysDoc::RemoveUniquePoint(const EoGePoint3d& point) {
  int References = 0;

  auto UniquePointPosition = GetFirstUniquePointPosition();
  while (UniquePointPosition != 0) {
    auto position = UniquePointPosition;
    EoGeUniquePoint* UniquePoint = GetNextUniquePoint(UniquePointPosition);
    if (point == UniquePoint->m_Point) {
      References = --(UniquePoint->m_References);

      if (References == 0) {
        RemoveUniquePointAt(position);
        delete UniquePoint;
      }
      break;
    }
  }
  return References;
}
void AeSysDoc::AddPrimitiveBit(EoDbPrimitive* primitive, int bit) {
  EoDbMaskedPrimitive* MaskedPrimitive = 0;

  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    auto posCur = MaskedPrimitivePosition;
    MaskedPrimitive = GetNextMaskedPrimitive(MaskedPrimitivePosition);
    if (MaskedPrimitive->GetPrimitive() == primitive) {
      MaskedPrimitivePosition = posCur;
      break;
    }
  }
  if (MaskedPrimitivePosition == 0) {
    MaskedPrimitive = new EoDbMaskedPrimitive(primitive, 0);
    AddMaskedPrimitive(MaskedPrimitive);
  }
  MaskedPrimitive->SetMaskBit(bit);
}
void AeSysDoc::RemovePrimitiveBit(EoDbPrimitive* primitive, int bit) {
  EoDbMaskedPrimitive* MaskedPrimitive = 0;

  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    auto posCur = MaskedPrimitivePosition;
    MaskedPrimitive = GetNextMaskedPrimitive(MaskedPrimitivePosition);
    if (MaskedPrimitive->GetPrimitive() == primitive) {
      MaskedPrimitivePosition = posCur;
      break;
    }
  }
  if (MaskedPrimitivePosition != 0) { MaskedPrimitive->ClearMaskBit(bit); }
}
DWORD AeSysDoc::GetPrimitiveMask(EoDbPrimitive* primitive) {
  EoDbMaskedPrimitive* MaskedPrimitive = 0;

  auto MaskedPrimitivePosition = GetFirstMaskedPrimitivePosition();
  while (MaskedPrimitivePosition != 0) {
    auto posCur = MaskedPrimitivePosition;
    MaskedPrimitive = GetNextMaskedPrimitive(MaskedPrimitivePosition);
    if (MaskedPrimitive->GetPrimitive() == primitive) {
      MaskedPrimitivePosition = posCur;
      break;
    }
  }
  return ((MaskedPrimitivePosition != 0) ? MaskedPrimitive->GetMask() : 0UL);
}
