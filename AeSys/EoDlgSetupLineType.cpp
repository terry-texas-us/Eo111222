#include "Stdafx.h"

#include "AeSysView.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDlgSetupLineType.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "PrimState.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(EoDlgSetupLineType, CDialog)

BEGIN_MESSAGE_MAP(EoDlgSetupLineType, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_DRAWITEM()
#pragma warning(pop)
END_MESSAGE_MAP()

enum LineTypesListColumnLabels { Name = 0, Appearance, Description };

EoDlgSetupLineType::EoDlgSetupLineType(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetupLineType::IDD, pParent), m_LineType(nullptr), m_LineTypeTable(nullptr) {}

EoDlgSetupLineType::EoDlgSetupLineType(EoDbLineTypeTable* lineTypeTable, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetupLineType::IDD, pParent), m_LineType(nullptr), m_LineTypeTable(lineTypeTable) {}

EoDlgSetupLineType::~EoDlgSetupLineType() {}

void EoDlgSetupLineType::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_LINETYPES_LIST_CONTROL, m_LineTypesListControl);
}

BOOL EoDlgSetupLineType::OnInitDialog() {
  CDialog::OnInitDialog();

  m_LineTypesListControl.DeleteAllItems();
  m_LineTypesListControl.InsertColumn(Name, L"Name", LVCFMT_LEFT, 128);
  m_LineTypesListControl.InsertColumn(Appearance, L"Apearance", LVCFMT_LEFT, 144);
  m_LineTypesListControl.InsertColumn(Description, L"Description", LVCFMT_LEFT, 128);

  if (m_LineTypeTable != nullptr) { m_LineTypeTable->FillListControl(m_LineTypesListControl); }
  // TODO: Select the current LineType

  return TRUE;
}

void EoDlgSetupLineType::OnOK() {
  auto Position = m_LineTypesListControl.GetFirstSelectedItemPosition();
  if (Position != nullptr) {
    int Item = m_LineTypesListControl.GetNextSelectedItem(Position);
    m_LineType = (EoDbLineType*)m_LineTypesListControl.GetItemData(Item);
  }
  CDialog::OnOK();
}

void EoDlgSetupLineType::OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct) {
  if (controlIdentifier == IDC_LINETYPES_LIST_CONTROL) {
    switch (drawItemStruct->itemAction) {
      case ODA_DRAWENTIRE: {
        CRect itemRectangle(drawItemStruct->rcItem);
        COLORREF backgroundColor =
            ::GetSysColor((drawItemStruct->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW);

        CDC deviceContext;
        deviceContext.Attach(drawItemStruct->hDC);
        CBrush backgroundBrush(backgroundColor);
        deviceContext.FillRect(itemRectangle, &backgroundBrush);

        if (drawItemStruct->itemState & ODS_FOCUS) { deviceContext.DrawFocusRect(itemRectangle); }
        int Item = static_cast<int>(drawItemStruct->itemID);
        if (Item != -1) {
          COLORREF rgbText = (drawItemStruct->itemState & ODS_SELECTED) ? ::GetSysColor(COLOR_HIGHLIGHTTEXT)
                                                                        : ::GetSysColor(COLOR_WINDOWTEXT);
          deviceContext.SetBkColor(backgroundColor);
          deviceContext.SetTextColor(rgbText);

          EoDbLineType* lineType = (EoDbLineType*)m_LineTypesListControl.GetItemData(Item);

          CRect subItemRectangle;
          m_LineTypesListControl.GetSubItemRect(Item, Name, LVIR_LABEL, subItemRectangle);
          CString name = lineType->Name();
          deviceContext.ExtTextOutW(subItemRectangle.left + 6, subItemRectangle.top + 1, ETO_CLIPPED, &subItemRectangle,
                                    name, static_cast<UINT>(name.GetLength()), nullptr);

          m_LineTypesListControl.GetSubItemRect(Item, Appearance, LVIR_LABEL, subItemRectangle);

          pstate.SetPen(nullptr, &deviceContext, 0, static_cast<EoInt16>(lineType->Index()));

          auto* activeView = AeSysView::GetActiveView();

          activeView->ViewportPushActive();
          activeView->PushViewTransform();

          activeView->SetViewportSize(subItemRectangle.right + subItemRectangle.left,
                                      subItemRectangle.bottom + subItemRectangle.top);

          double UExtent = static_cast<double>(subItemRectangle.right + subItemRectangle.left) /
                           static_cast<double>(deviceContext.GetDeviceCaps(LOGPIXELSX));
          double VExtent = static_cast<double>(subItemRectangle.bottom + subItemRectangle.top) /
                           static_cast<double>(deviceContext.GetDeviceCaps(LOGPIXELSY));
          activeView->ModelViewInitialize();

          activeView->SetViewWindow(0.0, 0.0, UExtent, VExtent);
          activeView->SetCameraTarget(EoGePoint3d::kOrigin);
          activeView->SetCameraPosition(EoGeVector3d::positiveUnitZ);
          double UMin =
              static_cast<double>(subItemRectangle.left) / static_cast<double>(deviceContext.GetDeviceCaps(LOGPIXELSX));
          double UMax = static_cast<double>(subItemRectangle.right) /
                        static_cast<double>(deviceContext.GetDeviceCaps(LOGPIXELSX));

          EoGeLine Line(EoGePoint3d(UMin, VExtent / 2.0, 0.0), EoGePoint3d(UMax, VExtent / 2.0, 0.0));
          Line.Display(activeView, &deviceContext);

          activeView->PopViewTransform();
          activeView->ViewportPopActive();

          m_LineTypesListControl.GetSubItemRect(Item, Description, LVIR_LABEL, subItemRectangle);
          CString Description = lineType->Description();
          deviceContext.ExtTextOutW(subItemRectangle.left + 6, subItemRectangle.top + 1, ETO_CLIPPED, &subItemRectangle,
                                    Description, static_cast<UINT>(Description.GetLength()), nullptr);
        }
        deviceContext.Detach();
      } break;

      case ODA_SELECT:
        ::InvertRect(drawItemStruct->hDC, &(drawItemStruct->rcItem));
        break;

      case ODA_FOCUS:
        // TODO: Focus indication?
        break;
    }
    return;
  }
  CDialog::OnDrawItem(controlIdentifier, drawItemStruct);
}
