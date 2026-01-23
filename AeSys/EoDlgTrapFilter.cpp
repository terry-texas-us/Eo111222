#include "Stdafx.h"

#include <climits>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoDlgTrapFilter.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(EoDlgTrapFilter, CDialog)

EoDlgTrapFilter::EoDlgTrapFilter(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgTrapFilter::IDD, pParent) {}
EoDlgTrapFilter::EoDlgTrapFilter(AeSysDoc* document, CWnd* pParent /*=nullptr*/) : CDialog(EoDlgTrapFilter::IDD, pParent), m_Document(document) {}
EoDlgTrapFilter::~EoDlgTrapFilter() {}
void EoDlgTrapFilter::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_TRAP_FILTER_LINE_LIST, m_FilterLineComboBoxControl);
  DDX_Control(dataExchange, IDC_TRAP_FILTER_ELEMENT_LIST, m_FilterPrimitiveTypeListBoxControl);
}
BOOL EoDlgTrapFilter::OnInitDialog() {
  CDialog::OnInitDialog();

  SetDlgItemInt(IDC_TRAP_FILTER_PEN_ID, 1, FALSE);

  EoDbLineTypeTable* LineTypeTable = m_Document->LineTypeTable();

  CString Name;
  EoDbLineType* LineType;
  auto Position = LineTypeTable->GetStartPosition();
  while (Position) {
    LineTypeTable->GetNextAssoc(Position, Name, LineType);
    m_FilterLineComboBoxControl.AddString(Name);
  }
  m_FilterLineComboBoxControl.SetCurSel(0);

  CString PrimitiveTypes = EoAppLoadStringResource(IDS_PRIMITIVE_FILTER_LIST);

  int TypesPosition = 0;
  while (TypesPosition < PrimitiveTypes.GetLength()) { m_FilterPrimitiveTypeListBoxControl.AddString(PrimitiveTypes.Tokenize(L"\n", TypesPosition)); }
  m_FilterPrimitiveTypeListBoxControl.SetCurSel(0);

  return TRUE;
}
void EoDlgTrapFilter::OnOK() {
  if (IsDlgButtonChecked(IDC_TRAP_FILTER_PEN)) {
    EoInt16 color = EoInt16(GetDlgItemInt(IDC_TRAP_FILTER_PEN_ID, 0, FALSE));
    FilterByColor(color);
  }
  if (IsDlgButtonChecked(IDC_TRAP_FILTER_LINE)) {
    EoInt16 LineTypeIndex = SHRT_MAX;
    wchar_t szBuf[32]{};

    if (GetDlgItemTextW(IDC_TRAP_FILTER_LINE_LIST, (LPTSTR)szBuf, sizeof(szBuf) / sizeof(wchar_t))) {
      EoDbLineTypeTable* LineTypeTable = m_Document->LineTypeTable();
      EoDbLineType* LineType;
      if (LineTypeTable->Lookup(szBuf, LineType)) { LineTypeIndex = LineType->Index(); }
    }
    if (LineTypeIndex != SHRT_MAX) { FilterByLineType(LineTypeIndex); }
  }
  if (IsDlgButtonChecked(IDC_TRAP_FILTER_ELEMENT)) {
    switch (m_FilterPrimitiveTypeListBoxControl.GetCurSel()) {
      case 0:
        FilterByPrimitiveType(EoDb::kEllipsePrimitive);
        break;
      case 1:
        FilterByPrimitiveType(EoDb::kGroupReferencePrimitive);
        break;
      case 2:
        FilterByPrimitiveType(EoDb::kLinePrimitive);
        break;
      case 3:
        FilterByPrimitiveType(EoDb::kPointPrimitive);
        break;
      case 4:
        FilterByPrimitiveType(EoDb::kTextPrimitive);
        break;
      case 5:
        FilterByPrimitiveType(EoDb::kPolygonPrimitive);
        break;
      case 6:
        FilterByPrimitiveType(EoDb::kPolylinePrimitive);
    }
  }
  CDialog::OnOK();
}
void EoDlgTrapFilter::FilterByColor(EoInt16 colorIndex) {
  auto GroupPosition = m_Document->GetFirstTrappedGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = m_Document->GetNextTrappedGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->Color() == colorIndex) {
        m_Document->RemoveTrappedGroup(Group);
        m_Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        break;
      }
    }
  }
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void EoDlgTrapFilter::FilterByLineType(int lineType) {
  auto GroupPosition = m_Document->GetFirstTrappedGroupPosition();
  while (GroupPosition != nullptr) {
    auto* Group = m_Document->GetNextTrappedGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->LineTypeIndex() == lineType) {
        m_Document->RemoveTrappedGroup(Group);
        m_Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        break;
      }
    }
  }
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void EoDlgTrapFilter::FilterByPrimitiveType(const EoDb::PrimitiveTypes primitiveType) {
  auto GroupPosition = m_Document->GetFirstTrappedGroupPosition();
  while (GroupPosition != nullptr) {
    bool bFilter{};

    auto* Group = m_Document->GetNextTrappedGroup(GroupPosition);

    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);

      switch (primitiveType) {
        case EoDb::kLinePrimitive:
          bFilter = Primitive->Is(EoDb::kLinePrimitive);
          break;
        case EoDb::kEllipsePrimitive:
          bFilter = Primitive->Is(EoDb::kEllipsePrimitive);
          break;
        case EoDb::kGroupReferencePrimitive:
          bFilter = Primitive->Is(EoDb::kGroupReferencePrimitive);
          break;
        case EoDb::kTextPrimitive:
          bFilter = Primitive->Is(EoDb::kTextPrimitive);
          break;
        case EoDb::kPolygonPrimitive:
          bFilter = Primitive->Is(EoDb::kPolygonPrimitive);
          break;
        case EoDb::kPolylinePrimitive:
          bFilter = Primitive->Is(EoDb::kPolylinePrimitive);
          break;
        case EoDb::kPointPrimitive:
          bFilter = Primitive->Is(EoDb::kPointPrimitive);
          break;
        case EoDb::kArcPrimitive:
          bFilter = Primitive->Is(EoDb::kArcPrimitive);
          break;
        case EoDb::kCirclePrimitive:
          bFilter = Primitive->Is(EoDb::kCirclePrimitive);
          break;
        case EoDb::kInsertPrimitive:
        case EoDb::kSplinePrimitive:
        case EoDb::kCSplinePrimitive:
        case EoDb::kTagPrimitive:
        case EoDb::kDimensionPrimitive:
          // These types are not in the filter list UI, so ignore them.
          break;
      }
      if (bFilter) {
        m_Document->RemoveTrappedGroup(Group);
        m_Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
        break;
      }
    }
  }
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
