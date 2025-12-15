#include "stdafx.h"
#include "AeSysView.h"

#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDlgSetupLineType.h"
#include "PrimState.h"

IMPLEMENT_DYNAMIC(EoDlgSetupLineType, CDialog)

BEGIN_MESSAGE_MAP(EoDlgSetupLineType, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_DRAWITEM()
#pragma warning(pop)
END_MESSAGE_MAP()

EoDlgSetupLineType::EoDlgSetupLineType(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgSetupLineType::IDD, pParent) {}
EoDlgSetupLineType::EoDlgSetupLineType(EoDbLineTypeTable* lineTypeTable, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgSetupLineType::IDD, pParent), m_LineTypeTable(lineTypeTable) {}
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

  m_LineTypeTable->FillListControl(m_LineTypesListControl);

  // TODO: Select the current LineType

  return TRUE;
}
void EoDlgSetupLineType::OnOK() {
  POSITION Position = m_LineTypesListControl.GetFirstSelectedItemPosition();
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
        CRect ItemRectangle(drawItemStruct->rcItem);
        COLORREF BackgroundColor =
            ::GetSysColor((drawItemStruct->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW);

        CDC DeviceContext;
        DeviceContext.Attach(drawItemStruct->hDC);
        CBrush BackgroundBrush(BackgroundColor);
        DeviceContext.FillRect(ItemRectangle, &BackgroundBrush);

        if (drawItemStruct->itemState & ODS_FOCUS) { DeviceContext.DrawFocusRect(ItemRectangle); }
        int Item = static_cast<int>(drawItemStruct->itemID);
        if (Item != -1) {
          COLORREF rgbText = (drawItemStruct->itemState & ODS_SELECTED) ? ::GetSysColor(COLOR_HIGHLIGHTTEXT)
                                                                        : ::GetSysColor(COLOR_WINDOWTEXT);
          DeviceContext.SetBkColor(BackgroundColor);
          DeviceContext.SetTextColor(rgbText);

          EoDbLineType* LineType = (EoDbLineType*)m_LineTypesListControl.GetItemData(Item);

          CRect SubItemRectangle;
          m_LineTypesListControl.GetSubItemRect(Item, Name, LVIR_LABEL, SubItemRectangle);
          CString Name = LineType->Name();
          DeviceContext.ExtTextOutW(SubItemRectangle.left + 6, SubItemRectangle.top + 1, ETO_CLIPPED, &SubItemRectangle,
                                    Name, static_cast<UINT>(Name.GetLength()), nullptr);

          m_LineTypesListControl.GetSubItemRect(Item, Appearance, LVIR_LABEL, SubItemRectangle);

          pstate.SetPen(nullptr, &DeviceContext, 0, static_cast<EoInt16>(LineType->Index()));

          AeSysView* ActiveView = AeSysView::GetActiveView();

          ActiveView->ViewportPushActive();
          ActiveView->PushViewTransform();

          ActiveView->SetViewportSize(SubItemRectangle.right + SubItemRectangle.left,
                                      SubItemRectangle.bottom + SubItemRectangle.top);

          double UExtent = static_cast<double>(SubItemRectangle.right + SubItemRectangle.left) /
                           static_cast<double>(DeviceContext.GetDeviceCaps(LOGPIXELSX));
          double VExtent = static_cast<double>(SubItemRectangle.bottom + SubItemRectangle.top) /
                           static_cast<double>(DeviceContext.GetDeviceCaps(LOGPIXELSY));
          ActiveView->ModelViewInitialize();

          ActiveView->SetViewWindow(0.0, 0.0, UExtent, VExtent);
          ActiveView->SetCameraTarget(EoGePoint3d::kOrigin);
          ActiveView->SetCameraPosition(EoGeVector3d::kZAxis);
          double UMin =
              static_cast<double>(SubItemRectangle.left) / static_cast<double>(DeviceContext.GetDeviceCaps(LOGPIXELSX));
          double UMax = static_cast<double>(SubItemRectangle.right) /
                        static_cast<double>(DeviceContext.GetDeviceCaps(LOGPIXELSX));

          EoGeLine Line(EoGePoint3d(UMin, VExtent / 2., 0.0), EoGePoint3d(UMax, VExtent / 2., 0.0));
          Line.Display(ActiveView, &DeviceContext);

          ActiveView->PopViewTransform();
          ActiveView->ViewportPopActive();

          m_LineTypesListControl.GetSubItemRect(Item, Description, LVIR_LABEL, SubItemRectangle);
          CString Description = LineType->Description();
          DeviceContext.ExtTextOutW(SubItemRectangle.left + 6, SubItemRectangle.top + 1, ETO_CLIPPED, &SubItemRectangle,
                                    Description, static_cast<UINT>(Description.GetLength()), nullptr);
        }
        DeviceContext.Detach();
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
