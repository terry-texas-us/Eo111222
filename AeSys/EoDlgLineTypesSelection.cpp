#include "Stdafx.h"
#include "afxdialogex.h"
#include <Windows.h>
#include <afx.h>
#include <afxdd_.h>
#include <afxmsg_.h>
#include <afxstr.h>
#include <afxwin.h>
#include <algorithm>
#include <atltypes.h>

#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDlgLineTypesSelection.h"
#include "Resource.h"

namespace {
constexpr int lineTypePreviewColumnIndex = 2;
}

IMPLEMENT_DYNAMIC(EoDlgLineTypesSelection, CDialogEx)

EoDlgLineTypesSelection::EoDlgLineTypesSelection(CWnd* parent /*=nullptr*/)
    : CDialogEx(IDD_LINE_TYPES_DIALOG, parent) {}

EoDlgLineTypesSelection::EoDlgLineTypesSelection(EoDbLineTypeTable& lineTypes, CWnd* parent)
    : CDialogEx(IDD_LINE_TYPES_DIALOG, parent) {
  m_lineTypes = lineTypes;  // Or assign/move if using std::vector
}

EoDlgLineTypesSelection::~EoDlgLineTypesSelection() {}

void EoDlgLineTypesSelection::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LINE_TYPES_LIST, m_lineTypesListControl);
}

BEGIN_MESSAGE_MAP(EoDlgLineTypesSelection, CDialogEx)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_NOTIFY(NM_CUSTOMDRAW, IDC_LINE_TYPES_LIST, &EoDlgLineTypesSelection::OnNMCustomDrawList)
ON_WM_SIZE()
#pragma warning(pop)
ON_BN_CLICKED(IDOK, &EoDlgLineTypesSelection::OnBnClickedOk)
END_MESSAGE_MAP()

/** @brief Initializes the dialog and its controls.
 *
 * This method sets up the list control with appropriate styles and columns,
 * then populates it with line type data by calling PopulateList().
 *
 * @return TRUE unless you set the focus to a control
 *         EXCEPTION: OCX Property Pages should return FALSE
 */
BOOL EoDlgLineTypesSelection::OnInitDialog() {
  CDialogEx::OnInitDialog();

  m_lineTypesListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

  m_lineTypesListControl.InsertColumn(0, L"Index", LVCFMT_LEFT, 0);  // Hidden column for index
  m_lineTypesListControl.InsertColumn(1, L"Name", LVCFMT_LEFT, 160);
  m_lineTypesListControl.InsertColumn(2, L"Line Type Preview", LVCFMT_LEFT, 320);  // This is the custom-drawn column

  PopulateList();

  m_lineTypesListControl.SetColumnWidth(lineTypePreviewColumnIndex, LVSCW_AUTOSIZE_USEHEADER);

  if (m_selectedLineType != nullptr) {
    // Set the current line type in the list
    for (int i = 0; i < m_lineTypesListControl.GetItemCount(); ++i) {
      auto* lineType = reinterpret_cast<EoDbLineType*>(m_lineTypesListControl.GetItemData(i));
      if (lineType != nullptr && lineType->Name() == m_selectedLineType->Name()) {
        m_lineTypesListControl.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        m_lineTypesListControl.EnsureVisible(i, FALSE);
        break;
      }
    }
  }
  return TRUE;
}

void EoDlgLineTypesSelection::OnOK() {
  int selectedIndex = m_lineTypesListControl.GetNextItem(-1, LVNI_SELECTED);
  if (selectedIndex != -1) {
    m_selectedLineType = reinterpret_cast<EoDbLineType*>(m_lineTypesListControl.GetItemData(selectedIndex));
  } else {
    m_selectedLineType = nullptr;  // No selection
  }
  CDialogEx::OnOK();
}

BOOL EoDlgLineTypesSelection::PreTranslateMessage(MSG* messageStruct) {
  if (messageStruct->message == WM_KEYDOWN) {
    // Check for digit keys '0' to '9'
    if (messageStruct->wParam >= '0' && messageStruct->wParam <= '9') {
      auto index = static_cast<int>(messageStruct->wParam - '0');
      int itemCount = m_lineTypesListControl.GetItemCount();
      if (index < itemCount) {
        m_lineTypesListControl.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        m_lineTypesListControl.EnsureVisible(index, FALSE);
        m_lineTypesListControl.SetExtendedStyle(m_lineTypesListControl.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
        m_lineTypesListControl.SetFocus();
      }
      return TRUE;  // Handled
    }
  }
  return CDialogEx::PreTranslateMessage(messageStruct);
}

/** @brief Populates the list control with line type data.
 *
 * This method iterates through the m_lineTypes collection and adds each line type
 * to the m_lineTypesListControl control. It sets up the item text for each column and
 * associates the line type pointer with the list item for later retrieval.
 *
 * Note: Consider using modern C++ containers (e.g., std::vector<std::shared_ptr<EoDbLineType>>)
 * for better memory management and safety instead of CObList.
 */
void EoDlgLineTypesSelection::PopulateList() {
  m_lineTypesListControl.DeleteAllItems();

  POSITION position = m_lineTypes.GetStartPosition();
  int index = 0;
  while (position != nullptr) {
    CString name;
    EoDbLineType* lineType{};
    m_lineTypes.GetNextAssoc(position, name, lineType);

    CString indexString = lineType->IndexToString();
    int indexItem = m_lineTypesListControl.InsertItem(lineType->Index(), indexString);
    m_lineTypesListControl.SetItemText(indexItem, 1, lineType->Name());
    m_lineTypesListControl.SetItemText(indexItem, lineTypePreviewColumnIndex, L"");  // Empty text for preview column; we'll draw it custom

    // Store pointer for later retrieval (unsafe if list is modified; consider unique IDs or modern containers)
    m_lineTypesListControl.SetItemData(indexItem, reinterpret_cast<DWORD_PTR>(lineType));

    index++;
  }
}

void EoDlgLineTypesSelection::OnSize(UINT type, int x, int y) {
  CDialogEx::OnSize(type, x, y);
  CRect clientRect;
  GetClientRect(&clientRect);
  if (m_lineTypesListControl.GetSafeHwnd() != nullptr) {
    clientRect.left += 10;
    clientRect.top += 10;
    clientRect.right -= 10;
    clientRect.bottom -= 56;
    m_lineTypesListControl.MoveWindow(clientRect);
    m_lineTypesListControl.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
  }
}

void EoDlgLineTypesSelection::OnNMCustomDrawList(NMHDR* pNMHDR, LRESULT* result) {
  auto* listViewCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
  *result = CDRF_DODEFAULT;

  switch (listViewCustomDraw->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
      *result = CDRF_NOTIFYITEMDRAW;
      break;

    case CDDS_ITEMPREPAINT:
      *result = CDRF_NOTIFYSUBITEMDRAW;
      break;

    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
      if (listViewCustomDraw->iSubItem == lineTypePreviewColumnIndex)  // Line type preview column
      {
        int item = static_cast<int>(listViewCustomDraw->nmcd.dwItemSpec);
        auto* lineType = reinterpret_cast<EoDbLineType*>(m_lineTypesListControl.GetItemData(item));

        if (lineType) {
          CDC controlContext;
          controlContext.Attach(listViewCustomDraw->nmcd.hdc);
          CRect controlRect(listViewCustomDraw->nmcd.rc);

          auto dpi = static_cast<double>(GetDpiForSystem());

          auto state = m_lineTypesListControl.GetItemState(item, LVIS_SELECTED);
          COLORREF backgroundColor = (state & LVIS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW);
          controlContext.FillSolidRect(controlRect, backgroundColor);

          const auto& dashElements = lineType->DashElements();  // returns vector<double>: +ve dash, -ve space, 0 dot

          if (!dashElements.empty()) {
            // Draw the line type pattern as a horizontal preview line centered in the control rectangle
            int yCenter = controlRect.top + controlRect.Height() / 2;
            double x = controlRect.left + 4.0;  // Padding
            double xEnd = controlRect.right - 4.0;

            CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
            CPen* oldPen = controlContext.SelectObject(&pen);

            while (x < xEnd) {
              for (double len : dashElements) {
                if (len > 0.0) {
                  controlContext.MoveTo(static_cast<int>(x), yCenter);
                  x += len * dpi;
                  controlContext.LineTo(static_cast<int>(std::min(x, xEnd)), yCenter);
                } else if (len < 0.0) {
                  x -= std::min(xEnd, len * dpi);
                } else {
                  controlContext.SetPixel(static_cast<int>(x), yCenter, RGB(0, 0, 0));
                  x += 1.0;  // Small advance
                }
                if (x >= xEnd) break;
              }
            }
            controlContext.SelectObject(oldPen);
          }
          controlContext.Detach();
        }
        *result = CDRF_SKIPDEFAULT;  // We handled drawing, skip default text/paint
      }
      break;
  }
}
void EoDlgLineTypesSelection::OnBnClickedOk() {
  // TODO: Add your control notification handler code here
  CDialogEx::OnOK();
}
