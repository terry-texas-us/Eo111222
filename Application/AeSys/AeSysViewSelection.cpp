#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbLine.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "Resource.h"

void AeSysView::DeleteLastGroup() {
  if (m_VisibleGroupList.IsEmpty()) {
    app.AddStringToMessageList(IDS_MSG_NO_DET_GROUPS_IN_VIEW);
  } else {
    auto* document = GetDocument();
    auto* Group = m_VisibleGroupList.RemoveTail();

    document->AnyLayerRemove(Group);
    if (document->RemoveTrappedGroup(Group) != 0) {  // Display it normal color so the erase xor will work
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      UpdateStateInformation(TrapCount);
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
    document->DeletedGroupsAddHead(Group);
    app.AddStringToMessageList(IDS_SEG_DEL_TO_RESTORE);
  }
}

void AeSysView::BreakAllPolylines() {
  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* Group = GetNextVisibleGroup(position);
    Group->BreakPolylines();
  }
}

void AeSysView::ExplodeAllBlockReferences() {
  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextVisibleGroup(position);
    group->ExplodeBlockReferences();
  }
}

void AeSysView::ResetView() {
  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;
}

EoDbGroup* AeSysView::SelSegAndPrimAtCtrlPt(const EoGePoint4d& pt) {
  EoDbPrimitive* primitive{};
  EoGePoint3d engagedPoint;

  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;

  EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextVisibleGroup(position);
    primitive = group->SelPrimAtCtrlPt(this, pt, &engagedPoint);
    if (primitive != nullptr) {
      m_ptDet = engagedPoint;
      m_ptDet = transformMatrix * m_ptDet;
      m_EngagedGroup = group;
      m_EngagedPrimitive = primitive;
    }
  }
  return m_EngagedGroup;
}

EoDbGroup* AeSysView::SelectGroupAndPrimitive(const EoGePoint3d& point) {
  EoGePoint3d engagedPoint;

  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;

  EoGePoint4d ptView(point);
  ModelViewTransformPoint(ptView);

  EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  double apertureSize = m_SelectApertureSize;

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextVisibleGroup(position);
    auto* primitive = group->SelPrimUsingPoint(this, ptView, apertureSize, engagedPoint);
    if (primitive != nullptr) {
      m_ptDet = engagedPoint;
      m_ptDet = transformMatrix * m_ptDet;
      m_EngagedGroup = group;
      m_EngagedPrimitive = primitive;
      return group;
    }
  }
  return nullptr;
}

EoDbGroup* AeSysView::SelectCircleUsingPoint(EoGePoint3d& point, double tolerance, EoDbConic*& circle) {
  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (!primitive->Is(EoDb::kConicPrimitive)) { continue; }
      auto* conic = static_cast<EoDbConic*>(primitive);
      if (conic->Subclass() != EoDbConic::ConicType::Circle) { continue; }
      if (point.DistanceTo(conic->Center()) <= tolerance) {
        circle = conic;
        return group;
      }
    }
  }
  return nullptr;
}

EoDbGroup* AeSysView::SelectLineUsingPoint(EoGePoint3d& point, EoDbLine*& line) {
  EoGePoint4d ptView(point);
  ModelViewTransformPoint(ptView);

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive->Is(EoDb::kLinePrimitive)) {
        EoGePoint3d PointOnLine;
        if (primitive->SelectUsingPoint(this, ptView, PointOnLine)) {
          line = static_cast<EoDbLine*>(primitive);
          return group;
        }
      }
    }
  }
  return nullptr;
}

EoDbGroup* AeSysView::SelectLineUsingPoint(const EoGePoint3d& pt) {
  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;

  EoGePoint3d engagedPoint;

  EoGePoint4d ptView(pt);
  ModelViewTransformPoint(ptView);

  double tol = m_SelectApertureSize;

  EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(GroupPosition);
    auto PrimitivePosition = group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* primitive = group->GetNext(PrimitivePosition);
      if (primitive->Is(EoDb::kLinePrimitive)) {
        if (primitive->SelectUsingPoint(this, ptView, engagedPoint)) {
          tol = ptView.DistanceToPointXY(EoGePoint4d(engagedPoint));

          m_ptDet = engagedPoint;
          m_ptDet = transformMatrix * m_ptDet;
          m_EngagedGroup = group;
          m_EngagedPrimitive = primitive;
        }
      }
    }
  }
  return m_EngagedGroup;
}

EoDbText* AeSysView::SelectTextUsingPoint(const EoGePoint3d& point) {
  EoGePoint4d ptView(point);
  ModelViewTransformPoint(ptView);

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive->Is(EoDb::kTextPrimitive)) {
        EoGePoint3d ptProj;
        if (static_cast<EoDbText*>(primitive)->SelectUsingPoint(this, ptView, ptProj)) {
          return static_cast<EoDbText*>(primitive);
        }
      }
    }
  }
  return nullptr;
}

void AeSysView::OnToolsPrimitiveSnapto() {
  auto cursorPosition = GetCursorPosition();

  EoGePoint3d ptDet;

  if (GroupIsEngaged()) {
    auto* primitive = m_EngagedPrimitive;

    EoGePoint4d ptView(cursorPosition);
    ModelViewTransformPoint(ptView);

    EoDbPolygon::EdgeToEvaluate() = EoDbPolygon::Edge();

    if (primitive->SelectUsingPoint(this, ptView, ptDet)) {
      ptDet = primitive->GoToNextControlPoint();
      m_ptDet = ptDet;

      primitive->AddReportToMessageList(ptDet);
      SetCursorPosition(ptDet);
      return;
    }
  }
  if (SelectGroupAndPrimitive(cursorPosition) != nullptr) {
    ptDet = m_ptDet;
    m_EngagedPrimitive->AddReportToMessageList(ptDet);
    SetCursorPosition(ptDet);
  }
}

void AeSysView::OnPrimPerpJump() {
  auto cursorPosition = GetCursorPosition();

  if (SelectGroupAndPrimitive(cursorPosition) != nullptr) {
    if (m_EngagedPrimitive->Is(EoDb::kLinePrimitive)) {
      auto* engagedLine = static_cast<EoDbLine*>(m_EngagedPrimitive);
      cursorPosition = engagedLine->ProjectPointToLine(m_ptCursorPosWorld);
      SetCursorPosition(cursorPosition);
    }
  }
}
