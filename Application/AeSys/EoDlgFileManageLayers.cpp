#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbLayer.h"
#include "EoDbPrimitive.h"
#include "EoDlgFileManageLayers.h"
#include "EoDlgLineTypesSelection.h"
#include "EoDlgSetupColor.h"
#include "Resource.h"

/// @brief Dialog class for getting a layer name from the user.
class EoDlgGetLayerName : public CDialog {
 public:
  EoDlgGetLayerName(CWnd* parent = nullptr);
  EoDlgGetLayerName(const EoDlgGetLayerName&) = delete;
  EoDlgGetLayerName& operator=(const EoDlgGetLayerName&) = delete;

  virtual ~EoDlgGetLayerName();

  enum { IDD = IDD_GET_LAYER_NAME };

 protected:
  virtual void DoDataExchange(CDataExchange* dataExchange);

 public:
  CString m_Name;
};

EoDlgGetLayerName::EoDlgGetLayerName(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgGetLayerName::IDD, pParent) {}
EoDlgGetLayerName::~EoDlgGetLayerName() {}
void EoDlgGetLayerName::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Text(dataExchange, IDC_NAME, m_Name);
}

BEGIN_MESSAGE_MAP(EoDlgFileManageLayers, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_DRAWITEM()
#pragma warning(pop)
ON_BN_CLICKED(IDC_MFCBUTTON_DEL, &EoDlgFileManageLayers::OnBnClickedMfcbuttonDel)
ON_BN_CLICKED(IDC_MFCBUTTON_NEW, &EoDlgFileManageLayers::OnBnClickedMfcbuttonNew)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_NOTIFY(NM_CLICK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManageLayers::OnNMClickLayersListControl)
ON_NOTIFY(NM_RCLICK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManageLayers::OnNMRclickLayersListControl)
ON_NOTIFY(NM_DBLCLK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManageLayers::OnNMDblclkLayersListControl)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManageLayers::OnItemchangedLayersListControl)
ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManageLayers::OnLvnBeginLabelEdit)
ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManageLayers::OnLvnEndLabelEdit)
ON_MESSAGE(WM_APP, &EoDlgFileManageLayers::OnDeferredEditLabel)
ON_WM_TIMER()
#pragma warning(pop)
END_MESSAGE_MAP()

EoDlgFileManageLayers::EoDlgFileManageLayers(CWnd* parent /*=nullptr*/) : CDialog(EoDlgFileManageLayers::IDD, parent) {}
EoDlgFileManageLayers::EoDlgFileManageLayers(AeSysDoc* document, CWnd* parent /*=nullptr*/)
    : CDialog(EoDlgFileManageLayers::IDD, parent), m_Document(document) {}
EoDlgFileManageLayers::~EoDlgFileManageLayers() {}

BOOL EoDlgFileManageLayers::PreTranslateMessage(MSG* pMsg) {
  if (m_toolTip.GetSafeHwnd() != nullptr) { m_toolTip.RelayEvent(pMsg); }
  if (pMsg->message == WM_LBUTTONDOWN && pMsg->hwnd == m_layersListControl.GetSafeHwnd()) {
    m_preClickSelectedItem = m_selectedItemForEdit;
  }
  return CDialog::PreTranslateMessage(pMsg);
}

void EoDlgFileManageLayers::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_LAYERS_LIST_CONTROL, m_layersListControl);
  DDX_Control(dataExchange, IDC_GROUPS, m_groups);
}

BOOL EoDlgFileManageLayers::OnInitDialog() {
  CDialog::OnInitDialog();

  CString captionText;
  GetWindowTextW(captionText);
  captionText += L" - ";
  captionText += m_Document->GetPathName();
  SetWindowTextW(captionText);

  m_previewWindowHandle = GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd();

  m_layersListControl.DeleteAllItems();
  m_layersListControl.InsertColumn(Status, L"Status", LVCFMT_LEFT, 56);
  m_layersListControl.InsertColumn(Name, L"Layer", LVCFMT_LEFT, 192);
  m_layersListControl.InsertColumn(On, L"On", LVCFMT_CENTER, 34);
  m_layersListControl.InsertColumn(Freeze, L"Freeze", LVCFMT_CENTER, 60);
  m_layersListControl.InsertColumn(Lock, L"Lock", LVCFMT_CENTER, 48);
  m_layersListControl.InsertColumn(Plot, L"Plot", LVCFMT_CENTER, 40);
  m_layersListControl.InsertColumn(Color, L"Color", LVCFMT_LEFT, 72);
  m_layersListControl.InsertColumn(LineType, L"LineType", LVCFMT_LEFT, 120);
  m_numberOfColumns = m_layersListControl.InsertColumn(LineWeight, L"LineWeight", LVCFMT_LEFT, 96);
  m_numberOfColumns++;

  for (int i = 0; i < m_Document->GetLayerTableSize(); i++) {
    EoDbLayer* layer = m_Document->GetLayerTableLayerAt(i);
    m_layersListControl.InsertItem(i, L"");
    m_layersListControl.SetItemData(i, DWORD_PTR(layer));
  }

  CBitmap bitmap;
  bitmap.LoadBitmapW(IDB_LAYER_STATES);
  m_stateImages.Create(24, 24, ILC_COLOR32 | ILC_MASK, 0, 1);
  m_stateImages.Add(&bitmap, Eo::bitmapMaskColor);

  // Apply glyphs from the shared state image list to the New and Delete buttons
  if (auto* newButton = static_cast<CMFCButton*>(GetDlgItem(IDC_MFCBUTTON_NEW))) {
    HICON hIcon = m_stateImages.ExtractIcon(11);
    newButton->SetImage(hIcon, TRUE);
    newButton->SetWindowTextW(L"");
  }
  if (auto* deleteButton = static_cast<CMFCButton*>(GetDlgItem(IDC_MFCBUTTON_DEL))) {
    HICON hIcon = m_stateImages.ExtractIcon(13);
    deleteButton->SetImage(hIcon, TRUE);
    deleteButton->SetWindowTextW(L"");
  }

  // Tooltips for the icon-only buttons
  m_toolTip.Create(this, TTS_ALWAYSTIP);
  m_toolTip.SetDelayTime(TTDT_INITIAL, 400);
  if (auto* newButton = GetDlgItem(IDC_MFCBUTTON_NEW)) { m_toolTip.AddTool(newButton, L"New Layer"); }
  if (auto* deleteButton = GetDlgItem(IDC_MFCBUTTON_DEL)) { m_toolTip.AddTool(deleteButton, L"Delete / Detach Layer"); }
  m_toolTip.Activate(TRUE);

  if (auto* currentLayer = m_Document->GetWorkLayer()) {
    CString layerName;
    layerName.Format(L"%s", currentLayer->Name().GetString());
    GetDlgItem(IDC_STATIC_WORK_LAYER)->SetWindowTextW(layerName);
  }

  return TRUE;
}

void EoDlgFileManageLayers::OnBnClickedMfcbuttonNew() {
  CString name;

  int suffix{1};
  do { name.Format(L"Layer%d", suffix++); } while (m_Document->FindLayerTableLayer(name) != -1);

  constexpr EoDbLayer::State commonState =
      EoDbLayer::State::isResident | EoDbLayer::State::isInternal | EoDbLayer::State::isActive;

  auto* layer = new EoDbLayer(name, commonState);
  layer->SetLineType(m_Document->ContinuousLineType());
  m_Document->AddLayerTableLayer(layer);

  int itemCount = m_layersListControl.GetItemCount();
  m_layersListControl.InsertItem(itemCount, L"");
  m_layersListControl.SetItemData(itemCount, DWORD_PTR(layer));
}

void EoDlgFileManageLayers::OnBnClickedMfcbuttonDel() {
  auto position = m_layersListControl.GetFirstSelectedItemPosition();
  if (position == nullptr) { return; }
  int item = m_layersListControl.GetNextSelectedItem(position);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

  if (layer->Name() == L"0") {
    app.WarningMessageBox(IDS_MSG_LAYER_NO_DELETE_0);
  } else if (layer->IsWork()) {
    app.WarningMessageBox(IDS_MSG_LAYER_NO_DELETE_WORK, layer->Name());
  } else if (layer->IsTracingLayer()) {
    // Tracing layers are XREF references — detach: remove from document and list without deleting on-disk file
    m_Document->UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
    auto& modelLayers = m_Document->SpaceLayers(EoDxf::Space::ModelSpace);
    for (int n = 0; n < static_cast<int>(modelLayers.GetSize()); n++) {
      if (modelLayers.GetAt(n) == layer) {
        auto pos2 = layer->GetHeadPosition();
        while (pos2 != nullptr) { m_Document->UnregisterGroupHandles(layer->GetNext(pos2)); }
        m_Document->UnregisterHandle(layer->Handle());
        layer->DeleteGroupsAndRemoveAll();
        delete layer;
        modelLayers.RemoveAt(n);
        break;
      }
    }
    m_layersListControl.DeleteItem(item);
  } else {
    m_Document->UpdateAllViews(nullptr, EoDb::kLayerErase, layer);

    // Search and remove directly from model-space layer array — avoids
    // FindLayerTableLayer / RemoveLayerTableLayerAt which both use
    // ActiveSpaceLayers() and fail when paper space is active.
    auto& modelLayers = m_Document->SpaceLayers(EoDxf::Space::ModelSpace);
    for (int n = 0; n < static_cast<int>(modelLayers.GetSize()); n++) {
      if (modelLayers.GetAt(n) == layer) {
        auto pos2 = layer->GetHeadPosition();
        while (pos2 != nullptr) { m_Document->UnregisterGroupHandles(layer->GetNext(pos2)); }
        m_Document->UnregisterHandle(layer->Handle());
        layer->DeleteGroupsAndRemoveAll();
        delete layer;
        modelLayers.RemoveAt(n);
        break;
      }
    }

    m_layersListControl.DeleteItem(item);
  }
}

void EoDlgFileManageLayers::DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& itemRectangle) {
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(itemID));
  if (layer == nullptr) { return; }
  switch (labelIndex) {
    case Status:
      if (layer->IsWork()) {
        m_stateImages.Draw(&deviceContext, 8, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      } else if (layer->IsActive()) {
        m_stateImages.Draw(&deviceContext, 9, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      } else if (layer->IsStatic()) {
        m_stateImages.Draw(&deviceContext, 10, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      }
      break;
    case Name: {
      CString layerName = layer->Name();
      deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 4, ETO_CLIPPED, &itemRectangle, layerName,
          static_cast<UINT>(layerName.GetLength()), nullptr);
    } break;
    case On: {
      int iconX = itemRectangle.left + ((itemRectangle.right - itemRectangle.left) - 24) / 2;
      int iconY = itemRectangle.top + ((itemRectangle.bottom - itemRectangle.top) - 24) / 2;
      m_stateImages.Draw(&deviceContext, layer->IsOff() ? 3 : 2, CPoint(iconX, iconY), ILD_TRANSPARENT);
    } break;
    case Freeze: {
      int iconX = itemRectangle.left + ((itemRectangle.right - itemRectangle.left) - 24) / 2;
      int iconY = itemRectangle.top + ((itemRectangle.bottom - itemRectangle.top) - 24) / 2;
      m_stateImages.Draw(&deviceContext, layer->IsFrozen() ? 4 : 5, CPoint(iconX, iconY), ILD_TRANSPARENT);
    } break;
    case Lock: {
      int iconX = itemRectangle.left + ((itemRectangle.right - itemRectangle.left) - 24) / 2;
      int iconY = itemRectangle.top + ((itemRectangle.bottom - itemRectangle.top) - 24) / 2;
      m_stateImages.Draw(&deviceContext, layer->IsStatic() ? 0 : 1, CPoint(iconX, iconY), ILD_TRANSPARENT);
    } break;
    case Plot: {
      int iconX = itemRectangle.left + ((itemRectangle.right - itemRectangle.left) - 24) / 2;
      int iconY = itemRectangle.top + ((itemRectangle.bottom - itemRectangle.top) - 24) / 2;
      m_stateImages.Draw(&deviceContext, layer->PlottingFlag() ? 6 : 7, CPoint(iconX, iconY), ILD_TRANSPARENT);
    } break;
    case Color: {
      CRect colorRectangle(itemRectangle);
      colorRectangle.DeflateRect(4, 4);
      colorRectangle.right = colorRectangle.left + colorRectangle.Height();

      CBrush colorBrush(layer->ColorValue());
      deviceContext.FillRect(&colorRectangle, &colorBrush);

      CBrush frameBrush;
      frameBrush.CreateSysColorBrush(COLOR_WINDOWFRAME);
      deviceContext.FrameRect(&colorRectangle, &frameBrush);
      if (colorRectangle.right + 4 < itemRectangle.right) {
        CString colorName;
        colorName.Format(L"%i", layer->ColorIndex());
        deviceContext.ExtTextOutW(colorRectangle.right + 4, itemRectangle.top + 4, ETO_CLIPPED, &itemRectangle,
            colorName, static_cast<UINT>(colorName.GetLength()), nullptr);
      }
    } break;
    case LineType: {
      CString LineTypeName = layer->LineTypeName();
      deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 4, ETO_CLIPPED, &itemRectangle,
          LineTypeName, static_cast<UINT>(LineTypeName.GetLength()), nullptr);
    } break;
    case LineWeight: {
      CString lineWeight;
      lineWeight.Format(L"%hd", static_cast<std::int16_t>(layer->LineWeight()));
      deviceContext.ExtTextOutW(itemRectangle.left + 8, itemRectangle.top + 4, ETO_CLIPPED, &itemRectangle, lineWeight,
          static_cast<UINT>(lineWeight.GetLength()), nullptr);
    } break;
  }
}

void EoDlgFileManageLayers::OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT lpDrawItemStruct) {
  if (controlIdentifier == IDC_LAYERS_LIST_CONTROL) {
    switch (lpDrawItemStruct->itemAction) {
      case ODA_DRAWENTIRE: {
        // clear item
        CRect rcItem(lpDrawItemStruct->rcItem);
        CDC DeviceContext;
        COLORREF rgbBkgnd =
            ::GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW);
        DeviceContext.Attach(lpDrawItemStruct->hDC);
        CBrush br(rgbBkgnd);
        DeviceContext.FillRect(rcItem, &br);
        if (lpDrawItemStruct->itemState & ODS_FOCUS) { DeviceContext.DrawFocusRect(rcItem); }
        int itemID = static_cast<int>(lpDrawItemStruct->itemID);
        if (itemID != -1) {
          // The text color is stored as the item data.
          COLORREF rgbText = (lpDrawItemStruct->itemState & ODS_SELECTED) ? ::GetSysColor(COLOR_HIGHLIGHTTEXT)
                                                                          : ::GetSysColor(COLOR_WINDOWTEXT);
          DeviceContext.SetBkColor(rgbBkgnd);
          DeviceContext.SetTextColor(rgbText);
          for (int labelIndex = 0; labelIndex < m_numberOfColumns; ++labelIndex) {
            m_layersListControl.GetSubItemRect(itemID, labelIndex, LVIR_LABEL, rcItem);
            DrawItem(DeviceContext, itemID, labelIndex, rcItem);
          }
        }
        DeviceContext.Detach();
      } break;

      case ODA_SELECT:
        ::InvertRect(lpDrawItemStruct->hDC, &(lpDrawItemStruct->rcItem));
        break;

      case ODA_FOCUS:
        //::DrawFocusRect(lpDrawItemStruct->hDC, &(lpDrawItemStruct->rcItem));
        break;
    }
    return;
  }
  CDialog::OnDrawItem(controlIdentifier, lpDrawItemStruct);
}

void EoDlgFileManageLayers::OnNMRclickLayersListControl(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  const int item = pNMItemActivate->iItem;
  *pResult = 0;
  if (item < 0) { return; }

  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));
  if (layer == nullptr) { return; }

  const bool isTracing = layer->IsTracingLayer();
  const bool isWork = layer->IsWork();
  const bool isLayer0 = (layer->Name() == L"0");

  CMenu menu;
  menu.CreatePopupMenu();

  if (!isTracing) {
    menu.AppendMenuW(isWork ? (MF_STRING | MF_GRAYED) : MF_STRING, 1, L"Set Current Work");
    menu.AppendMenuW(isLayer0 ? (MF_STRING | MF_GRAYED) : MF_STRING, 2, L"Rename");
    menu.AppendMenuW(MF_SEPARATOR, 0, static_cast<LPCWSTR>(nullptr));
    menu.AppendMenuW((isWork || isLayer0) ? (MF_STRING | MF_GRAYED) : MF_STRING, 3, L"Delete");
  } else {
    menu.AppendMenuW(MF_STRING, 4, L"Edit Tracing");
    menu.AppendMenuW(MF_STRING, 5, L"Reload Tracing");
    menu.AppendMenuW(MF_SEPARATOR, 0, static_cast<LPCWSTR>(nullptr));
    menu.AppendMenuW(MF_STRING, 3, L"Detach");
  }

  CPoint screenPoint;
  ::GetCursorPos(&screenPoint);
  const int choice = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, this, nullptr);

  switch (choice) {
    case 1: {  // Set Current Work
      auto* previousWorkLayer = m_Document->SetWorkLayer(layer);
      CString layerName;
      layerName.Format(L"%s", layer->Name().GetString());
      GetDlgItem(IDC_STATIC_WORK_LAYER)->SetWindowTextW(layerName);
      previousWorkLayer->SetStateActive();
      m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, previousWorkLayer);
      m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
      GetDlgItem(IDC_LAYERS_LIST_CONTROL)->RedrawWindow();
      break;
    }
    case 2:  // Rename — trigger in-place edit
      m_layersListControl.SetFocus();
      m_layersListControl.EditLabel(item);
      break;
    case 3:  // Delete / Detach — reuse the Del button logic
      m_layersListControl.SetItemState(item, LVIS_SELECTED, LVIS_SELECTED);
      OnBnClickedMfcbuttonDel();
      break;
    case 4:  // Edit Tracing
      EndDialog(IDCANCEL);
      m_Document->EnterEmbeddedTracingEditMode(layer);
      break;
    case 5:  // Reload Tracing
      m_Document->ReloadTracingLayer(layer);
      m_Document->UpdateAllViews(nullptr);
      m_layersListControl.Invalidate();
      break;
  }
}

void EoDlgFileManageLayers::OnNMClickLayersListControl(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

  auto item = pNMItemActivate->iItem;
  if (item < 0) { return; }

  auto subItem = pNMItemActivate->iSubItem;

  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

  m_clickToColumnName = false;
  switch (subItem) {
    case Status:
      // Single click on the status icon sets the work layer (non-tracing layers only)
      if (!layer->IsTracingLayer()) {
        auto* previousWorkLayer = m_Document->SetWorkLayer(layer);

        CString layerName;
        layerName.Format(L"%s", layer->Name().GetString());
        GetDlgItem(IDC_STATIC_WORK_LAYER)->SetWindowTextW(layerName);

        previousWorkLayer->SetStateActive();

        m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, previousWorkLayer);
        m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);

        GetDlgItem(IDC_LAYERS_LIST_CONTROL)->RedrawWindow();
        return;
      }
      break;
    case Name:
      if (!layer->IsTracingLayer() && layer->Name() != L"0" && item == m_preClickSelectedItem) {
        m_clickToColumnName = true;
        PostMessage(WM_APP, static_cast<WPARAM>(item), 0);
      }
      return;
    case On:
      if (layer->IsWork()) {
        app.WarningMessageBox(IDS_MSG_LAYER_NO_HIDDEN, layer->Name());
      } else {
        if (layer->IsOff()) {
          layer->ClearStateFlag(static_cast<std::uint16_t>(EoDbLayer::State::isOff));
          m_Document->UpdateAllViews(nullptr, EoDb::kLayer, layer);
        } else {
          m_Document->UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
          layer->SetStateOff();
        }
      }
      break;
    case Freeze:
      if (!layer->IsWork()) { layer->SetFrozen(!layer->IsFrozen()); }
      break;
    case Lock:
      if (layer->IsWork()) {
        app.WarningMessageBox(IDS_MSG_LAYER_NO_STATIC, layer->Name());
      } else {
        if (layer->IsStatic()) {
          layer->ClearStateFlag(static_cast<std::uint16_t>(EoDbLayer::State::isStatic));
        } else {
          layer->SetStateStatic();
        }
      }
      break;
    case Color: {
      EoDlgSetupColor dialog;
      dialog.m_ColorIndex = static_cast<std::uint16_t>(layer->ColorIndex());
      if (dialog.DoModal() == IDOK) { layer->SetColorIndex(static_cast<std::int16_t>(dialog.m_ColorIndex)); }
      break;
    }
    case LineType: {
      auto lineTypes = m_Document->LineTypeTable();
      EoDlgLineTypesSelection dialog(*lineTypes);
      if (dialog.DoModal() == IDOK) { layer->SetLineType(dialog.GetSelectedLineType()); }
      break;
    }
    case Plot:
      layer->SetPlottingFlag(!layer->PlottingFlag());
      break;
    case LineWeight:
      break;
  }
  m_layersListControl.Invalidate();

  *pResult = 0;
}

void EoDlgFileManageLayers::OnNMDblclkLayersListControl(NMHDR* pNMHDR, LRESULT* result) {
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

  auto item = pNMItemActivate->iItem;
  if (item < 0) { return; }

  auto subItem = pNMItemActivate->iSubItem;
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

  if (subItem == Name && !layer->IsTracingLayer() && layer->Name() != L"0") {
    // Double-click on the Name column launches in-place edit directly
    m_layersListControl.EditLabel(item);
  }
  *result = 0;
}

void EoDlgFileManageLayers::OnItemchangedLayersListControl(NMHDR* pNMHDR, LRESULT* result) {
  LPNMLISTVIEW ListViewNotificationMessage = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

  int item = ListViewNotificationMessage->iItem;

  // Track which item is selected so the Name-column single-click guard works correctly
  if (ListViewNotificationMessage->uNewState & LVIS_SELECTED) {
    m_selectedItemForEdit = item;
  } else if (ListViewNotificationMessage->uOldState & LVIS_SELECTED) {
    if (m_selectedItemForEdit == item) { m_selectedItemForEdit = -1; }
  }

  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));
  if (layer == nullptr) { return; }

  CString numberOfGroups;
  numberOfGroups.Format(L"%-4i", static_cast<int>(layer->GetCount()));
  m_groups.SetWindowTextW(numberOfGroups);

  EoDbPrimitive::SetLayerColor(layer->ColorIndex());
  EoDbPrimitive::SetLayerLineTypeIndex(layer->LineTypeIndex());
  EoDbPrimitive::SetLayerLineTypeName(std::wstring(layer->LineTypeName()));

  *result = 0;
}

void EoDlgFileManageLayers::OnLvnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) {
  NMLVDISPINFOW* dispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(dispInfo->item.iItem));

  // Disallow in-place edit for layer "0" and tracing layers
  if (layer == nullptr || layer->Name() == L"0" || layer->IsTracingLayer()) {
    *pResult = TRUE;  // cancel the edit
    return;
  }
  // EditLabel always creates the edit box over column 0.
  // Use a 1ms timer to reposition after the list view finishes placing the control.
  m_editItemForRepos = dispInfo->item.iItem;
  SetTimer(101, 1, nullptr);
  *pResult = FALSE;  // allow
}

void EoDlgFileManageLayers::OnLvnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) {
  NMLVDISPINFOW* dispInfo = reinterpret_cast<NMLVDISPINFOW*>(pNMHDR);
  *pResult = FALSE;

  if (dispInfo->item.pszText == nullptr) { return; }  // user cancelled

  CString newName(dispInfo->item.pszText);
  newName.Trim();
  if (newName.IsEmpty()) { return; }

  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(dispInfo->item.iItem));
  if (layer == nullptr) { return; }

  // Reject if the name already exists
  LVFINDINFO findInfo{};
  findInfo.flags = LVFI_STRING;
  findInfo.psz = newName;
  if (m_layersListControl.FindItem(&findInfo) != -1) { return; }

  layer->SetName(newName);
  if (layer->IsWork()) { m_Document->SetWorkLayer(layer); }

  // Trigger a full redraw so the owner-draw Name cell reflects the new name
  m_layersListControl.RedrawItems(dispInfo->item.iItem, dispInfo->item.iItem);
}

LRESULT EoDlgFileManageLayers::OnDeferredEditLabel(WPARAM wParam, LPARAM) {
  auto item = static_cast<int>(wParam);
  if (item >= 0 && item < m_layersListControl.GetItemCount()) {
    m_layersListControl.SetFocus();
    m_layersListControl.EditLabel(item);
  }
  return 0;
}

void EoDlgFileManageLayers::OnTimer(UINT_PTR nIDEvent) {
  if (nIDEvent == 101) {
    KillTimer(101);
    auto* editCtrl = m_layersListControl.GetEditControl();
    if (editCtrl != nullptr && editCtrl->IsWindowVisible() && m_editItemForRepos >= 0) {
      auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(m_editItemForRepos));
      CRect nameRect;
      CHeaderCtrl* headerCtrl = m_layersListControl.GetHeaderCtrl();
      if (headerCtrl != nullptr) {
        headerCtrl->GetItemRect(Name, &nameRect);
        // nameRect is in header-control client coords; convert to list-control client coords.
        headerCtrl->ClientToScreen(&nameRect);
        m_layersListControl.ScreenToClient(&nameRect);
      } else {
        m_layersListControl.GetSubItemRect(m_editItemForRepos, Name, LVIR_BOUNDS, nameRect);
      }
      CRect editRect;
      editCtrl->GetWindowRect(editRect);
      m_layersListControl.ScreenToClient(editRect);
      // Preserve the row-correct Y and height; replace only left and right.
      editRect.left = nameRect.left;
      editRect.right = nameRect.right;
      editCtrl->MoveWindow(editRect);
      if (layer != nullptr) {
        editCtrl->SetWindowTextW(layer->Name());
        editCtrl->SetSel(0, -1);
      }
    }
    m_editItemForRepos = -1;
    return;
  }
  CDialog::OnTimer(nIDEvent);
}
