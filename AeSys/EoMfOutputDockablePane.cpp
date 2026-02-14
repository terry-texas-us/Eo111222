#include "Stdafx.h"

#include <cassert>
#include <vector>
#include <wchar.h>

#include "AeSys.h"
#include "EoMfOutputDockablePane.h"
#include "Resource.h"

EoMfOutputDockablePane::EoMfOutputDockablePane() {}
EoMfOutputDockablePane::~EoMfOutputDockablePane() {}
BEGIN_MESSAGE_MAP(EoMfOutputDockablePane, CDockablePane)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_CREATE()
ON_WM_SIZE()
#pragma warning(pop)
END_MESSAGE_MAP()

int EoMfOutputDockablePane::OnCreate(LPCREATESTRUCT createStruct) {
  if (CDockablePane::OnCreate(createStruct) == -1) return -1;

  m_Font.CreateStockObject(DEFAULT_GUI_FONT);

  const CRect emptyRect{};

  if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, emptyRect, this, 1, CMFCTabCtrl::LOCATION_BOTTOM)) {
    ATLTRACE2(traceGeneral, 0, L"Failed to create output tab window\n");
    return -1;
  }
  constexpr DWORD sharedStyles =
      WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_EXTENDEDSEL;

  if (!m_OutputMessagesList.Create(sharedStyles, emptyRect, &m_wndTabs, 2) ||
      !m_OutputReportsList.Create(sharedStyles, emptyRect, &m_wndTabs, 4)) {
    ATLTRACE2(traceGeneral, 0, L"Failed to create output windows\n");
    return -1;
  }
  m_OutputMessagesList.SetFont(&m_Font);
  m_OutputReportsList.SetFont(&m_Font);

  // Attach list windows to tab:
  m_wndTabs.AddTab(&m_OutputMessagesList, App::LoadStringResource(IDS_OUTPUT_MESSAGES));
  m_wndTabs.AddTab(&m_OutputReportsList, App::LoadStringResource(IDS_OUTPUT_REPORTS));

  // Dummy data
  m_OutputMessagesList.AddString(L"Message output is being displayed here.");
  m_OutputReportsList.AddString(L"Reports output is being displayed here.");

  return 0;
}

void EoMfOutputDockablePane::OnSize(UINT type, int cx, int cy) {
  CDockablePane::OnSize(type, cx, cy);

  // Tab control should cover the whole client area:
  m_wndTabs.SetWindowPos(nullptr, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}
EoMfOutputListBox::EoMfOutputListBox() {}
EoMfOutputListBox::~EoMfOutputListBox() {}
BEGIN_MESSAGE_MAP(EoMfOutputListBox, CListBox)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_CONTEXTMENU()
ON_WM_WINDOWPOSCHANGING()
#pragma warning(pop)
ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
END_MESSAGE_MAP()

void EoMfOutputListBox::OnContextMenu(CWnd* window, CPoint point) {
  (void)window;

  auto* frameWindow = dynamic_cast<CMDIFrameWndEx*>(AfxGetMainWnd());
#ifdef _DEBUG
  assert(frameWindow != nullptr && "Main frame should be CMDIFrameWndEx when context menu is triggered");
#endif
  if (frameWindow != nullptr) {
    CMenu menu;
    menu.LoadMenu(IDR_OUTPUT_POPUP);

    auto* subMenu = menu.GetSubMenu(0);
    auto* popupMenu = new CMFCPopupMenu;

    if (!popupMenu->Create(this, point.x, point.y, subMenu->GetSafeHmenu(), FALSE, TRUE)) { return; }
    frameWindow->OnShowPopupMenu(popupMenu);
    UpdateDialogControls(this, FALSE);
  }
  SetFocus();
}
/** @brief Handles the "Copy" command from the context menu. Copies the selected text from the list box to the clipboard.
 *  @note If multiple items are selected, they are combined into a single string with newlines separating them before copying. */
void EoMfOutputListBox::OnEditCopy() {
  auto selectedCount = GetSelCount();
  if (selectedCount < 1) { return; }

  CString combinedText;

  if (selectedCount == 1) {
    int selectedIndex = GetCurSel();
    GetText(selectedIndex, combinedText);
  } else {  // Multiple selections
    std::vector<int> selectedIndices(static_cast<size_t>(selectedCount));
    GetSelItems(selectedCount, selectedIndices.data());

    bool first{true};
    for (const auto& index : selectedIndices) {
      CString lineText;
      GetText(index, lineText);
      if (!first) {
        combinedText += L"\r\n";  // Add newline between lines
      }
      combinedText += lineText;
      first = false;
    }
  }

  if (combinedText.IsEmpty()) { return; }

  if (OpenClipboard()) {
    EmptyClipboard();

    size_t size = static_cast<size_t>(combinedText.GetLength() + 1) * sizeof(wchar_t);
    auto clipboardDataHandle = GlobalAlloc(GMEM_MOVEABLE, size);

    if (clipboardDataHandle != nullptr) {
      wchar_t* clipboardData = static_cast<wchar_t*>(GlobalLock(clipboardDataHandle));
      if (clipboardData != nullptr) {
        wcscpy_s(clipboardData, static_cast<size_t>(combinedText.GetLength() + 1), combinedText);
        GlobalUnlock(clipboardDataHandle);
        SetClipboardData(CF_UNICODETEXT, clipboardDataHandle);
      } else {
        GlobalFree(clipboardDataHandle);
      }
    }
    CloseClipboard();
  }
}

void EoMfOutputListBox::OnEditClear() {
  if (GetCount() == 0) { return; }
  ResetContent();
}

void EoMfOutputListBox::OnViewOutput() {
  auto* parentPane = dynamic_cast<CDockablePane*>(GetOwner());
  auto* mainFrame = dynamic_cast<CMDIFrameWndEx*>(GetTopLevelFrame());

#ifdef _DEBUG
  assert(parentPane != nullptr && "Owner should be a CDockablePane");
  assert(mainFrame != nullptr && "Top level frame should be CMDIFrameWndEx");
#endif

  if (mainFrame != nullptr && parentPane != nullptr) {
    mainFrame->SetFocus();
    mainFrame->ShowPane(parentPane, FALSE, FALSE, FALSE);
    mainFrame->RecalcLayout();
  }
}
