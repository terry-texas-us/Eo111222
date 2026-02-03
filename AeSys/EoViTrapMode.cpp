#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPolygon.h"
#include "EoDlgTrapModify.h"
#include "Resource.h"

void AeSysView::OnTrapModeRemoveAdd() { app.OnTrapCommandsAddGroups(); }

void AeSysView::OnTrapModePoint() {
  auto* document = GetDocument();

  auto cursorPosition = GetCursorPosition();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* Group = GetNextVisibleGroup(position);

    if (document->FindTrappedGroup(Group) != 0) { continue; }

    if (Group->SelectUsingPoint_(this, ptView)) { document->AddGroupToTrap(Group); }
  }
  UpdateStateInformation(TrapCount);
}

void AeSysView::OnTrapModeStitch() {
  if (m_PreviousOp != ID_OP2) {
    m_PreviousPnt = GetCursorPosition();
    RubberBandingStartAtEnable(m_PreviousPnt, Lines);
    m_PreviousOp = ModeLineHighlightOp(ID_OP2);
  } else {
    EoGePoint3d pt = GetCursorPosition();

    if (m_PreviousPnt == pt) return;

    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(pt)};

    ModelViewTransformPoints(2, ptView);

    auto position = GetFirstVisibleGroupPosition();
    while (position != nullptr) {
      auto* Group = GetNextVisibleGroup(position);

      if (document->FindTrappedGroup(Group) != 0) { continue; }

      if (Group->SelectUsingLine(this, ptView[0], ptView[1])) { document->AddGroupToTrap(Group); }
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(m_PreviousOp);
    UpdateStateInformation(TrapCount);
  }
}

void AeSysView::OnTrapModeField() {
  if (m_PreviousOp != ID_OP4) {
    m_PreviousPnt = GetCursorPosition();
    RubberBandingStartAtEnable(m_PreviousPnt, Rectangles);
    m_PreviousOp = ModeLineHighlightOp(ID_OP4);
  } else {
    auto cursorPosition = GetCursorPosition();
    if (m_PreviousPnt == cursorPosition) return;

    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    EoGePoint3d ptMin = EoGePoint4d::Min(ptView[0], ptView[1]);
    EoGePoint3d ptMax = EoGePoint4d::Max(ptView[0], ptView[1]);

    auto position = GetFirstVisibleGroupPosition();
    while (position != nullptr) {
      auto* Group = GetNextVisibleGroup(position);

      if (document->FindTrappedGroup(Group) != 0) { continue; }

      if (Group->SelectUsingRectangle(this, ptMin, ptMax)) { document->AddGroupToTrap(Group); }
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(m_PreviousOp);
    UpdateStateInformation(TrapCount);
  }
}

void AeSysView::OnTrapModeLast() {
  auto* document = GetDocument();

  auto position = document->GetLastWorkLayerGroupPosition();
  while (position != nullptr) {
    auto* Group = document->GetPreviousWorkLayerGroup(position);

    if (!document->FindTrappedGroup(Group)) {
      document->AddGroupToTrap(Group);
      UpdateStateInformation(TrapCount);
      break;
    }
  }
}

void AeSysView::OnTrapModeEngage() {
  if (GroupIsEngaged()) {
    auto* document = GetDocument();

    auto position = document->FindWorkLayerGroup(EngagedGroup());

    auto* Group = document->GetNextWorkLayerGroup(position);

    if (document->FindTrappedGroup(Group) == 0) {
      document->AddGroupToTrap(Group);
      UpdateStateInformation(TrapCount);
    }
  } else {
    app.AddModeInformationToMessageList();
  }
}
void AeSysView::OnTrapModeMenu() {
  CPoint CurrentPosition;
  ::GetCursorPos(&CurrentPosition);
  HMENU TrapMenu = ::LoadMenu(app.GetInstance(), MAKEINTRESOURCE(IDR_TRAP));
  CMenu* SubMenu = CMenu::FromHandle(GetSubMenu(TrapMenu, 0));
  SubMenu->TrackPopupMenuEx(0, CurrentPosition.x, CurrentPosition.y, AfxGetMainWnd(), 0);
  ::DestroyMenu(TrapMenu);
}
void AeSysView::OnTrapModeModify() {
  auto* document = GetDocument();
  if (!document->IsTrapEmpty()) {
    EoDlgTrapModify Dialog(document);
    if (Dialog.DoModal() == IDOK) { document->UpdateAllViews(nullptr, 0L, nullptr); }
  } else {
    app.AddModeInformationToMessageList();
  }
}

void AeSysView::OnTrapModeEscape() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(m_PreviousOp);
}

void AeSysView::OnTraprModeRemoveAdd() { app.OnTrapCommandsAddGroups(); }

void AeSysView::OnTraprModePoint() {
  auto* document = GetDocument();

  EoGePoint3d cursorPosition = GetCursorPosition();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = document->GetFirstTrappedGroupPosition();
  while (position != nullptr) {
    auto* Group = document->GetNextTrappedGroup(position);

    if (Group->SelectUsingPoint_(this, ptView)) {
      document->RemoveTrappedGroupAt(document->FindTrappedGroup(Group));
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    }
  }
  UpdateStateInformation(TrapCount);
}

void AeSysView::OnTraprModeStitch() {
  if (m_PreviousOp != ID_OP2) {
    m_PreviousPnt = GetCursorPosition();
    RubberBandingStartAtEnable(m_PreviousPnt, Lines);
    m_PreviousOp = ModeLineHighlightOp(ID_OP2);
  } else {
    EoGePoint3d cursorPosition = GetCursorPosition();

    if (m_PreviousPnt == cursorPosition) return;
    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    auto position = document->GetFirstTrappedGroupPosition();
    while (position != nullptr) {
      auto* Group = document->GetNextTrappedGroup(position);

      if (Group->SelectUsingLine(this, ptView[0], ptView[1])) {
        document->RemoveTrappedGroupAt(document->FindTrappedGroup(Group));
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(m_PreviousOp);
    UpdateStateInformation(TrapCount);
  }
}

void AeSysView::OnTraprModeField() {
  if (m_PreviousOp != ID_OP4) {
    m_PreviousPnt = GetCursorPosition();
    RubberBandingStartAtEnable(m_PreviousPnt, Rectangles);
    m_PreviousOp = ModeLineHighlightOp(ID_OP4);
  } else {
    auto cursorPosition = GetCursorPosition();
    if (m_PreviousPnt == cursorPosition) { return; }

    auto* document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    EoGePoint3d ptMin = EoGePoint4d::Min(ptView[0], ptView[1]);
    EoGePoint3d ptMax = EoGePoint4d::Max(ptView[0], ptView[1]);

    auto position = document->GetFirstTrappedGroupPosition();
    while (position != nullptr) {
      auto* Group = document->GetNextTrappedGroup(position);

      if (Group->SelectUsingRectangle(this, ptMin, ptMax)) {
        document->RemoveTrappedGroupAt(document->FindTrappedGroup(Group));
        document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(m_PreviousOp);
    UpdateStateInformation(TrapCount);
  }
}
void AeSysView::OnTraprModeLast() {
  auto* document = GetDocument();

  if (!document->IsTrapEmpty()) {
    auto* Group = document->RemoveLastTrappedGroup();
    document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
    UpdateStateInformation(TrapCount);
  }
}
void AeSysView::OnTraprModeEngage() {
  // TODO: Add your command handler code here
}
void AeSysView::OnTraprModeMenu() {
  CPoint CurrentPosition;
  ::GetCursorPos(&CurrentPosition);
  HMENU TrapMenu = ::LoadMenu(app.GetInstance(), MAKEINTRESOURCE(IDR_TRAP));
  CMenu* SubMenu = CMenu::FromHandle(::GetSubMenu(TrapMenu, 0));
  SubMenu->TrackPopupMenuEx(0, CurrentPosition.x, CurrentPosition.y, AfxGetMainWnd(), 0);
  ::DestroyMenu(TrapMenu);
}
void AeSysView::OnTraprModeModify() {
  auto* document = GetDocument();
  if (!document->IsTrapEmpty()) {
    EoDlgTrapModify Dialog(document);
    if (Dialog.DoModal() == IDOK) { document->UpdateAllViews(nullptr, 0L, nullptr); }
  } else {
    app.AddModeInformationToMessageList();
  }
}
void AeSysView::OnTraprModeEscape() {
  RubberBandingDisable();
  ModeLineUnhighlightOp(m_PreviousOp);
}
