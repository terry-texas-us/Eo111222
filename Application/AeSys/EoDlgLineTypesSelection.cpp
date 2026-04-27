#include "Stdafx.h"

#include <algorithm>

#include "Eo.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDlgLineTypesSelection.h"
#include "Resource.h"

namespace {
constexpr int lineTypePreviewColumnIndex = 2;

/// @brief Returns a sort priority for special linetype names.
/// Lower values sort first: ByBlock=0, ByLayer=1, CONTINUOUS=2, everything else=3.
int LineTypeSortPriority(const CString& name) {
  if (name.CompareNoCase(L"ByBlock") == 0) { return 0; }
  if (name.CompareNoCase(L"ByLayer") == 0) { return 1; }
  if (name.CompareNoCase(L"CONTINUOUS") == 0) { return 2; }
  return 3;
}

/// @brief Case-insensitive comparison callback for SortItems.
/// ByBlock, ByLayer, and CONTINUOUS always sort before other entries;
/// remaining entries are sorted alphabetically.
int CALLBACK CompareLineTypeNames(LPARAM lParam1, LPARAM lParam2, LPARAM /*lParamSort*/) {
  const auto* lineType1 = reinterpret_cast<const EoDbLineType*>(lParam1);
  const auto* lineType2 = reinterpret_cast<const EoDbLineType*>(lParam2);
  if (lineType1 == nullptr || lineType2 == nullptr) { return 0; }
  const int priority1 = LineTypeSortPriority(lineType1->Name());
  int priority2 = LineTypeSortPriority(lineType2->Name());
  if (priority1 != priority2) { return priority1 - priority2; }
  return lineType1->Name().CompareNoCase(lineType2->Name());
}
}  // namespace

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
  DDX_Control(pDX, IDC_FILE_LINE_TYPES_LIST, m_fileLineTypesListControl);
}

BEGIN_MESSAGE_MAP(EoDlgLineTypesSelection, CDialogEx)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_NOTIFY(NM_CUSTOMDRAW, IDC_LINE_TYPES_LIST, &EoDlgLineTypesSelection::OnNMCustomDrawList)
ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILE_LINE_TYPES_LIST, &EoDlgLineTypesSelection::OnNMCustomDrawFileList)
ON_BN_CLICKED(IDC_LOAD_LINE_TYPES_FILE, &EoDlgLineTypesSelection::OnBnClickedLoadFile)
#pragma warning(pop)
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

  // Set up the document line types list
  m_lineTypesListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
  m_lineTypesListControl.InsertColumn(0, L"Index", LVCFMT_LEFT, 0);
  m_lineTypesListControl.InsertColumn(1, L"Name", LVCFMT_LEFT, 160);
  m_lineTypesListControl.InsertColumn(2, L"Line Type Preview", LVCFMT_LEFT, 320);

  PopulateList();

  m_lineTypesListControl.SetColumnWidth(lineTypePreviewColumnIndex, LVSCW_AUTOSIZE_USEHEADER);

  if (m_selectedLineType != nullptr) {
    for (int i = 0; i < m_lineTypesListControl.GetItemCount(); ++i) {
      const auto* lineType = reinterpret_cast<const EoDbLineType*>(m_lineTypesListControl.GetItemData(i));
      if (lineType != nullptr && lineType->Name().CompareNoCase(m_selectedLineType->Name()) == 0) {
        m_lineTypesListControl.SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        m_lineTypesListControl.EnsureVisible(i, FALSE);
        break;
      }
    }
  }

  // Set up the file-loaded line types list (same column structure, initially empty)
  m_fileLineTypesListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
  m_fileLineTypesListControl.InsertColumn(0, L"Index", LVCFMT_LEFT, 0);
  m_fileLineTypesListControl.InsertColumn(1, L"Name", LVCFMT_LEFT, 160);
  m_fileLineTypesListControl.InsertColumn(2, L"Line Type Preview", LVCFMT_LEFT, 320);
  m_fileLineTypesListControl.SetColumnWidth(lineTypePreviewColumnIndex, LVSCW_AUTOSIZE_USEHEADER);

  return TRUE;
}

void EoDlgLineTypesSelection::OnOK() {
  m_selectedLineType = nullptr;
  m_selectedFromFileList = false;

  // Prefer file list selection when present
  int fileSelectedIndex = m_fileLineTypesListControl.GetNextItem(-1, LVNI_SELECTED);
  if (fileSelectedIndex != -1) {
    m_selectedLineType = reinterpret_cast<EoDbLineType*>(m_fileLineTypesListControl.GetItemData(fileSelectedIndex));
    m_selectedFromFileList = true;
  }
  // Fall back to document list selection
  if (m_selectedLineType == nullptr) {
    int selectedIndex = m_lineTypesListControl.GetNextItem(-1, LVNI_SELECTED);
    if (selectedIndex != -1) {
      m_selectedLineType = reinterpret_cast<EoDbLineType*>(m_lineTypesListControl.GetItemData(selectedIndex));
    }
  }
  CDialogEx::OnOK();
}

BOOL EoDlgLineTypesSelection::PreTranslateMessage(MSG* messageStruct) {
  if (messageStruct->message == WM_KEYDOWN) {
    // Check for digit keys '0' to '9'
    if (messageStruct->wParam >= '0' && messageStruct->wParam <= '9') {
      const auto index = static_cast<int>(messageStruct->wParam - '0');
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
    const int indexItem = m_lineTypesListControl.InsertItem(lineType->Index(), indexString);
    m_lineTypesListControl.SetItemText(indexItem, 1, lineType->Name());
    m_lineTypesListControl.SetItemText(
        indexItem, lineTypePreviewColumnIndex, L"");  // Empty text for preview column; we'll draw it custom

    // Store pointer for later retrieval (unsafe if list is modified; consider unique IDs or modern containers)
    m_lineTypesListControl.SetItemData(indexItem, reinterpret_cast<DWORD_PTR>(lineType));

    index++;
  }
  m_lineTypesListControl.SortItems(CompareLineTypeNames, 0);
}

void EoDlgLineTypesSelection::OnBnClickedLoadFile() {
  CFileDialog fileDialog(TRUE,
      L"txt",
      nullptr,
      OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
      L"Line Type Files (*.txt)|*.txt|All Files (*.*)|*.*||",
      this);
  fileDialog.m_ofn.lpstrTitle = L"Load Line Types";

  if (fileDialog.DoModal() != IDOK) { return; }

  m_fileLineTypes.RemoveAll();
  m_fileLineTypes.LoadLineTypesFromTxtFile(fileDialog.GetPathName());

  PopulateFileList();
}

void EoDlgLineTypesSelection::PopulateFileList() {
  m_fileLineTypesListControl.DeleteAllItems();

  POSITION position = m_fileLineTypes.GetStartPosition();
  int index = 0;
  while (position != nullptr) {
    CString name;
    EoDbLineType* lineType{};
    m_fileLineTypes.GetNextAssoc(position, name, lineType);

    CString indexString = lineType->IndexToString();
    const int indexItem = m_fileLineTypesListControl.InsertItem(lineType->Index(), indexString);
    m_fileLineTypesListControl.SetItemText(indexItem, 1, lineType->Name());
    m_fileLineTypesListControl.SetItemText(indexItem, lineTypePreviewColumnIndex, L"");
    m_fileLineTypesListControl.SetItemData(indexItem, reinterpret_cast<DWORD_PTR>(lineType));

    index++;
  }
  m_fileLineTypesListControl.SortItems(CompareLineTypeNames, 0);
  m_fileLineTypesListControl.SetColumnWidth(lineTypePreviewColumnIndex, LVSCW_AUTOSIZE_USEHEADER);
}

void EoDlgLineTypesSelection::OnNMCustomDrawList(NMHDR* pNMHDR, LRESULT* result) {
  DrawLineTypePreview(m_lineTypesListControl, reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR), result);
}

void EoDlgLineTypesSelection::OnNMCustomDrawFileList(NMHDR* pNMHDR, LRESULT* result) {
  DrawLineTypePreview(m_fileLineTypesListControl, reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR), result);
}

void EoDlgLineTypesSelection::DrawLineTypePreview(const CListCtrl& listControl,
    NMLVCUSTOMDRAW* listViewCustomDraw,
    LRESULT* result) {
  *result = CDRF_DODEFAULT;

  switch (listViewCustomDraw->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
      *result = CDRF_NOTIFYITEMDRAW;
      break;

    case CDDS_ITEMPREPAINT:
      *result = CDRF_NOTIFYSUBITEMDRAW;
      break;

    case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
      if (listViewCustomDraw->iSubItem == lineTypePreviewColumnIndex) {
        const auto item = static_cast<int>(listViewCustomDraw->nmcd.dwItemSpec);
        const auto* lineType = reinterpret_cast<const EoDbLineType*>(listControl.GetItemData(item));

        if (lineType) {
          CDC controlContext;
          controlContext.Attach(listViewCustomDraw->nmcd.hdc);
          CRect controlRect(listViewCustomDraw->nmcd.rc);

          const auto state = listControl.GetItemState(item, LVIS_SELECTED);
          const COLORREF backgroundColor =
              (state & LVIS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW);
          controlContext.FillSolidRect(controlRect, backgroundColor);

          const auto& dashElements = lineType->DashElements();

          if (!dashElements.empty()) {
            const int yCenter = controlRect.top + controlRect.Height() / 2;
            const double xStart = controlRect.left + 4.0;
            const double xEnd = controlRect.right - 4.0;
            const double availableWidth = xEnd - xStart;

            // Compute total pattern length from absolute dash/gap values.
            double patternLength = lineType->GetPatternLength();
            if (patternLength < Eo::geometricTolerance) { patternLength = 1.0; }

            // Scale the pattern so it repeats ~3 times across the preview width.
            constexpr double targetRepetitions = 3.0;
            const double scale = availableWidth / (patternLength * targetRepetitions);

            CPen pen(PS_SOLID, 1, Eo::colorBlack);
            CPen* oldPen = controlContext.SelectObject(&pen);

            double x = xStart;
            while (x < xEnd) {
              for (const double len : dashElements) {
                double pixelLen = std::abs(len) * scale;
                pixelLen = std::max(pixelLen, 1.0);
                if (len > 0.0) {
                  controlContext.MoveTo(static_cast<int>(x), yCenter);
                  x += pixelLen;
                  controlContext.LineTo(static_cast<int>(std::min(x, xEnd)), yCenter);
                } else if (len < 0.0) {
                  x += pixelLen;
                } else {
                  controlContext.SetPixel(static_cast<int>(x), yCenter, Eo::colorBlack);
                  x += 1.0;
                }
                if (x >= xEnd) { break; }
              }
            }
            controlContext.SelectObject(oldPen);
          }
          controlContext.Detach();
        }
        *result = CDRF_SKIPDEFAULT;
      }
      break;
  }
}
