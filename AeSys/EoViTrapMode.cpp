#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbPolygon.h"
#include "EoDlgTrapModify.h"
#include "Resource.h"

void AeSysView::OnTrapModeRemoveAdd() { app.OnTrapCommandsAddGroups(); }

void AeSysView::OnTrapModePoint() {
  auto* Document = GetDocument();

  auto cursorPosition = GetCursorPosition();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto Position = GetFirstVisibleGroupPosition();
  while (Position != nullptr) {
    auto* Group = GetNextVisibleGroup(Position);

    if (Document->FindTrappedGroup(Group) != 0) continue;

    if (Group->SelectUsingPoint_(this, ptView)) { Document->AddGroupToTrap(Group); }
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

    auto* Document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(pt)};

    ModelViewTransformPoints(2, ptView);

    auto Position = GetFirstVisibleGroupPosition();
    while (Position != 0) {
      auto* Group = GetNextVisibleGroup(Position);

      if (Document->FindTrappedGroup(Group) != 0) continue;

      if (Group->SelectUsingLine(this, ptView[0], ptView[1])) { Document->AddGroupToTrap(Group); }
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

    auto* Document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    EoGePoint3d ptMin = EoGePoint4d::Min(ptView[0], ptView[1]);
    EoGePoint3d ptMax = EoGePoint4d::Max(ptView[0], ptView[1]);

    auto Position = GetFirstVisibleGroupPosition();
    while (Position != 0) {
      auto* Group = GetNextVisibleGroup(Position);

      if (Document->FindTrappedGroup(Group) != 0) continue;

      if (Group->SelectUsingRectangle(this, ptMin, ptMax)) { Document->AddGroupToTrap(Group); }
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(m_PreviousOp);
    UpdateStateInformation(TrapCount);
  }
}

void AeSysView::OnTrapModeLast() {
  auto* Document = GetDocument();

  auto Position = Document->GetLastWorkLayerGroupPosition();
  while (Position != 0) {
    auto* Group = Document->GetPreviousWorkLayerGroup(Position);

    if (!Document->FindTrappedGroup(Group)) {
      Document->AddGroupToTrap(Group);
      UpdateStateInformation(TrapCount);
      break;
    }
  }
}

void AeSysView::OnTrapModeEngage() {
  if (GroupIsEngaged()) {
    auto* Document = GetDocument();

    auto Position = Document->FindWorkLayerGroup(EngagedGroup());

    auto* Group = Document->GetNextWorkLayerGroup(Position);

    if (Document->FindTrappedGroup(Group) == 0) {
      Document->AddGroupToTrap(Group);
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
  auto* Document = GetDocument();

  EoGePoint3d cursorPosition = GetCursorPosition();

  EoGePoint4d ptView(cursorPosition);
  ModelViewTransformPoint(ptView);

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto Position = Document->GetFirstTrappedGroupPosition();
  while (Position != 0) {
    auto* Group = Document->GetNextTrappedGroup(Position);

    if (Group->SelectUsingPoint_(this, ptView)) {
      Document->RemoveTrappedGroupAt(Document->FindTrappedGroup(Group));
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
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
    auto* Document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    auto Position = Document->GetFirstTrappedGroupPosition();
    while (Position != 0) {
      auto* Group = Document->GetNextTrappedGroup(Position);

      if (Group->SelectUsingLine(this, ptView[0], ptView[1])) {
        Document->RemoveTrappedGroupAt(Document->FindTrappedGroup(Group));
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
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

    auto* Document = GetDocument();

    EoGePoint4d ptView[] = {EoGePoint4d(m_PreviousPnt), EoGePoint4d(cursorPosition)};

    ModelViewTransformPoints(2, ptView);

    EoGePoint3d ptMin = EoGePoint4d::Min(ptView[0], ptView[1]);
    EoGePoint3d ptMax = EoGePoint4d::Max(ptView[0], ptView[1]);

    auto Position = Document->GetFirstTrappedGroupPosition();
    while (Position != 0) {
      auto* Group = Document->GetNextTrappedGroup(Position);

      if (Group->SelectUsingRectangle(this, ptMin, ptMax)) {
        Document->RemoveTrappedGroupAt(Document->FindTrappedGroup(Group));
        Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      }
    }
    RubberBandingDisable();
    ModeLineUnhighlightOp(m_PreviousOp);
    UpdateStateInformation(TrapCount);
  }
}
void AeSysView::OnTraprModeLast() {
  auto* Document = GetDocument();

  if (!Document->IsTrapEmpty()) {
    auto* Group = Document->RemoveLastTrappedGroup();
    Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
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
