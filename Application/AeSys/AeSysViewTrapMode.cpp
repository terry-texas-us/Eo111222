#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPolygon.h"
#include "EoDlgTrapModify.h"
#include "Resource.h"
#include "TrapModeState.h"

namespace {
/// Returns the active TrapModeState from the view's state stack, or nullptr when
/// called outside trap/trapr mode (e.g. during PopAllModeStates teardown).
TrapModeState* TrapState(AeSysView* view) {
  return dynamic_cast<TrapModeState*>(view->GetCurrentState());
}
}  // namespace

void AeSysView::OnTrapModeRemoveAdd() {
  OnTrapCommandsAddGroups();
}

void AeSysView::OnTrapModePoint() {
  auto* document = GetDocument();

  const auto cursorPosition = GetCursorPosition();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextVisibleGroup(position);

    if (document->FindTrappedGroup(group) != nullptr) { continue; }

    if (group->SelectUsingPoint_(this, ptView)) { document->AddGroupToTrap(group); }
  }
  UpdateStateInformation(TrapCount);
}

void AeSysView::OnTrapModeStitch() {
  auto* trapState = TrapState(this);
  if (trapState == nullptr) { return; }

  if (trapState->PreviousOp() != ID_OP2) {
    trapState->SetPreviousPoint(GetCursorPosition());
    RubberBandingStartAtEnable(trapState->PreviousPoint(), Lines);
    trapState->SetPreviousOp(ModeLineHighlightOp(ID_OP2));
  } else {
    const EoGePoint3d pt = GetCursorPosition();

    if (trapState->PreviousPoint() == pt) { return; }

    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(trapState->PreviousPoint()), EoGePoint4d(pt)};

    ModelViewTransformPoints(2, ptView);

    auto position = GetFirstVisibleGroupPosition();
    while (position != nullptr) {
      auto* group = GetNextVisibleGroup(position);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      if (group->SelectUsingLine(this, EoGePoint3d{ptView[0]}, EoGePoint3d{ptView[1]})) {
        document->AddGroupToTrap(group);
      }
    }
    RubberBandingDisable();
    trapState->UnhighlightOp(this);
    UpdateStateInformation(TrapCount);
  }
}

void AeSysView::OnTrapModeField() {
  auto* trapState = TrapState(this);
  if (trapState == nullptr) { return; }

  if (trapState->PreviousOp() != ID_OP4) {
    trapState->SetPreviousPoint(GetCursorPosition());
    RubberBandingStartAtEnable(trapState->PreviousPoint(), Rectangles);
    trapState->SetPreviousOp(ModeLineHighlightOp(ID_OP4));
  } else {
    const auto cursorPosition = GetCursorPosition();
    if (trapState->PreviousPoint() == cursorPosition) { return; }

    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(trapState->PreviousPoint()), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    const EoGePoint3d ptMin = EoGePoint3d{EoGePoint4d::Min(ptView[0], ptView[1])};
    const EoGePoint3d ptMax = EoGePoint3d{EoGePoint4d::Max(ptView[0], ptView[1])};

    auto position = GetFirstVisibleGroupPosition();
    while (position != nullptr) {
      auto* group = GetNextVisibleGroup(position);

      if (document->FindTrappedGroup(group) != nullptr) { continue; }

      if (group->SelectUsingRectangle(this, ptMin, ptMax)) { document->AddGroupToTrap(group); }
    }
    RubberBandingDisable();
    trapState->UnhighlightOp(this);
    UpdateStateInformation(TrapCount);
  }
}

void AeSysView::OnTrapModeLast() {
  auto* document = GetDocument();

  auto position = document->GetLastWorkLayerGroupPosition();
  while (position != nullptr) {
    auto* group = document->GetPreviousWorkLayerGroup(position);

    if (!document->FindTrappedGroup(group)) {
      document->AddGroupToTrap(group);
      UpdateStateInformation(TrapCount);
      break;
    }
  }
}

void AeSysView::OnTrapModeEngage() {
  if (GroupIsEngaged()) {
    auto* document = GetDocument();

    auto position = document->FindWorkLayerGroup(EngagedGroup());

    auto* group = document->GetNextWorkLayerGroup(position);

    if (document->FindTrappedGroup(group) == nullptr) {
      document->AddGroupToTrap(group);
      UpdateStateInformation(TrapCount);
    }
  } else {
    app.AddModeInformationToMessageList();
  }
}

void AeSysView::OnTrapModeMenu() {
  CPoint currentPosition;
  ::GetCursorPos(&currentPosition);
  const auto trapMenu = ::LoadMenuW(AeSys::GetInstance(), MAKEINTRESOURCE(IDR_TRAP));
  auto* subMenu = CMenu::FromHandle(GetSubMenu(trapMenu, 0));
  subMenu->TrackPopupMenuEx(0, currentPosition.x, currentPosition.y, AfxGetMainWnd(), nullptr);
  ::DestroyMenu(trapMenu);
}

void AeSysView::OnTrapModeModify() {
  auto* document = GetDocument();
  if (!document->IsTrapEmpty()) {
    EoDlgTrapModify dialog(document);
    if (dialog.DoModal() == IDOK) { document->UpdateAllViews(nullptr, 0L, nullptr); }
  } else {
    app.AddModeInformationToMessageList();
  }
}

void AeSysView::OnTrapModeEscape() {
  auto* trapState = TrapState(this);
  if (trapState != nullptr) { trapState->OnEscape(this); }
}

void AeSysView::OnTraprModeRemoveAdd() {
  OnTrapCommandsAddGroups();
}

void AeSysView::OnTraprModePoint() {
  auto* document = GetDocument();

  const EoGePoint3d cursorPosition = GetCursorPosition();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = document->GetFirstTrappedGroupPosition();
  while (position != nullptr) {
    auto* group = document->GetNextTrappedGroup(position);

    if (group->SelectUsingPoint_(this, ptView)) {
      document->RemoveTrappedGroupAt(document->FindTrappedGroup(group));
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    }
  }
  UpdateStateInformation(TrapCount);
}

void AeSysView::OnTraprModeStitch() {
  auto* trapState = TrapState(this);
  if (trapState == nullptr) { return; }

  if (trapState->PreviousOp() != ID_OP2) {
    trapState->SetPreviousPoint(GetCursorPosition());
    RubberBandingStartAtEnable(trapState->PreviousPoint(), Lines);
    trapState->SetPreviousOp(ModeLineHighlightOp(ID_OP2));
  } else {
    const EoGePoint3d cursorPosition = GetCursorPosition();

    if (trapState->PreviousPoint() == cursorPosition) { return; }
    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(trapState->PreviousPoint()), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    auto position = document->GetFirstTrappedGroupPosition();
    while (position != nullptr) {
      auto* group = document->GetNextTrappedGroup(position);

      if (group->SelectUsingLine(this, EoGePoint3d{ptView[0]}, EoGePoint3d{ptView[1]})) {
        document->RemoveTrappedGroupAt(document->FindTrappedGroup(group));
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
    }
    RubberBandingDisable();
    trapState->UnhighlightOp(this);
    UpdateStateInformation(TrapCount);
  }
}

void AeSysView::OnTraprModeField() {
  auto* trapState = TrapState(this);
  if (trapState == nullptr) { return; }

  if (trapState->PreviousOp() != ID_OP4) {
    trapState->SetPreviousPoint(GetCursorPosition());
    RubberBandingStartAtEnable(trapState->PreviousPoint(), Rectangles);
    trapState->SetPreviousOp(ModeLineHighlightOp(ID_OP4));
  } else {
    const auto cursorPosition = GetCursorPosition();
    if (trapState->PreviousPoint() == cursorPosition) { return; }

    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(trapState->PreviousPoint()), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    const EoGePoint3d ptMin = EoGePoint3d{EoGePoint4d::Min(ptView[0], ptView[1])};
    const EoGePoint3d ptMax = EoGePoint3d{EoGePoint4d::Max(ptView[0], ptView[1])};

    auto position = document->GetFirstTrappedGroupPosition();
    while (position != nullptr) {
      auto* group = document->GetNextTrappedGroup(position);

      if (group->SelectUsingRectangle(this, ptMin, ptMax)) {
        document->RemoveTrappedGroupAt(document->FindTrappedGroup(group));
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
      }
    }
    RubberBandingDisable();
    trapState->UnhighlightOp(this);
    UpdateStateInformation(TrapCount);
  }
}
void AeSysView::OnTraprModeLast() {
  auto* document = GetDocument();

  if (!document->IsTrapEmpty()) {
    auto* group = document->RemoveLastTrappedGroup();
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, group);
    UpdateStateInformation(TrapCount);
  }
}
void AeSysView::OnTraprModeEngage() {
  // TODO: Add your command handler code here
}
void AeSysView::OnTraprModeMenu() {
  CPoint currentPosition;
  ::GetCursorPos(&currentPosition);
  const HMENU trapMenu = ::LoadMenu(AeSys::GetInstance(), MAKEINTRESOURCE(IDR_TRAP));
  CMenu* subMenu = CMenu::FromHandle(::GetSubMenu(trapMenu, 0));
  subMenu->TrackPopupMenuEx(0, currentPosition.x, currentPosition.y, AfxGetMainWnd(), nullptr);
  ::DestroyMenu(trapMenu);
}
void AeSysView::OnTraprModeModify() {
  auto* document = GetDocument();
  if (!document->IsTrapEmpty()) {
    EoDlgTrapModify dialog(document);
    if (dialog.DoModal() == IDOK) { document->UpdateAllViews(nullptr, 0L, nullptr); }
  } else {
    app.AddModeInformationToMessageList();
  }
}
void AeSysView::OnTraprModeEscape() {
  auto* trapState = TrapState(this);
  if (trapState != nullptr) { trapState->OnEscape(this); }
}
