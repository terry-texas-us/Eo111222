#include "stdafx.h"

#include "afxdialogex.h"
#include <Windows.h>
#include <afx.h>
#include <afxdd_.h>
#include <afxmsg_.h>
#include <afxstr.h>
#include <afxwin.h>
#include <atltypes.h>

#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDlgLineTypesSelection.h"
#include "Resource.h"

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
ON_NOTIFY(NM_CUSTOMDRAW, IDC_LINE_TYPES_LIST, &EoDlgLineTypesSelection::OnNMCustomDrawList)
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

  // Enable visual styles for modern look (if not already in app manifest)
  m_lineTypesListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

  // Insert columns (adjust widths and names based on DBLineType data)
  m_lineTypesListControl.InsertColumn(0, L"Name", LVCFMT_LEFT, 150);
  m_lineTypesListControl.InsertColumn(1, L"Description", LVCFMT_LEFT, 200);
  m_lineTypesListControl.InsertColumn(2, L"Line Type Preview", LVCFMT_LEFT, 150);  // This is the custom-drawn column

  PopulateList();

  return TRUE;
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
    EoDbLineType* lineType{nullptr};
    m_lineTypes.GetNextAssoc(position, name, lineType);

    // Insert item and subitems
    index = m_lineTypesListControl.InsertItem(index, lineType->Name());
    m_lineTypesListControl.SetItemText(index, 1, lineType->Description());
    m_lineTypesListControl.SetItemText(index, 2, L"");  // Empty text for preview column; we'll draw it custom

    // Store pointer for later retrieval (unsafe if list is modified; consider unique IDs or modern containers)
    m_lineTypesListControl.SetItemData(index, reinterpret_cast<DWORD_PTR>(lineType));

    index++;
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

    case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
      if (listViewCustomDraw->iSubItem == 2)  // Line type preview column
      {
        int item = static_cast<int>(listViewCustomDraw->nmcd.dwItemSpec);
        EoDbLineType* lineType = reinterpret_cast<EoDbLineType*>(m_lineTypesListControl.GetItemData(item));

        if (lineType) {
          CDC controlContext;
          controlContext.Attach(listViewCustomDraw->nmcd.hdc);
          CRect controlRect(listViewCustomDraw->nmcd.rc);

          //CRgn clipRegion;
          //clipRegion.CreateRectRgn(controlRect.left, controlRect.top, controlRect.right, controlRect.bottom);
          //controlContext.SelectClipRgn(&clipRegion);

          controlContext.FillSolidRect(controlRect, GetSysColor(COLOR_WINDOW));

          const auto& pattern = lineType->DashElements();  // returns vector<double>: +ve dash, -ve space, 0 dot

          if (!pattern.empty()) {
            // Draw the line type pattern as a horizontal preview line centered in the control rectangle
            int yCenter = controlRect.top + controlRect.Height() / 2;
            double x = controlRect.left + 5.0;  // Padding
            double xEnd = controlRect.right - 5.0;

            CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
            CPen* oldPen = controlContext.SelectObject(&pen);

            while (x < xEnd) {
              for (double len : pattern) {
                if (len > 0.0) {
                  controlContext.MoveTo(static_cast<int>(x), yCenter);
                  x += len * 96.;
                  controlContext.LineTo(static_cast<int>(x), yCenter);
                } else if (len < 0.0) {
                  x -= len * 96.;
                } else {
                  controlContext.SetPixel(static_cast<int>(x), yCenter, RGB(0, 0, 0));
                  x += 1.0;  // Small advance
                }

                if (x >= xEnd) break;
              }
            }
            //controlContext.SelectClipRgn(nullptr);
            controlContext.SelectObject(oldPen);
          }
          controlContext.Detach();
        }
        *result = CDRF_SKIPDEFAULT;  // We handled drawing, skip default text/paint
      }
      break;
  }
}