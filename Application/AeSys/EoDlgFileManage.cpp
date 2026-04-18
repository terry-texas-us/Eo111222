#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbLayer.h"
#include "EoDbPrimitive.h"
#include "EoDlgFileManage.h"
#include "EoDlgLineTypesSelection.h"
#include "EoDlgSetupColor.h"
#include "Resource.h"
#include "WndProcPreview.h"

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

BEGIN_MESSAGE_MAP(EoDlgFileManage, CDialog)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_DRAWITEM()
#pragma warning(pop)
ON_BN_CLICKED(IDC_LAYRENAME, &EoDlgFileManage::OnBnClickedLayerRename)
ON_BN_CLICKED(IDC_LAYMELT, &EoDlgFileManage::OnBnClickedLayerMelt)
ON_BN_CLICKED(IDC_MFCBUTTON_DEL, &EoDlgFileManage::OnBnClickedMfcbuttonDel)
ON_BN_CLICKED(IDC_MFCBUTTON_NEW, &EoDlgFileManage::OnBnClickedMfcbuttonNew)
ON_BN_CLICKED(IDC_MFCBUTTON_WORK, &EoDlgFileManage::OnBnClickedMfcbuttonWork)
ON_BN_CLICKED(IDC_TRAOPEN, &EoDlgFileManage::OnBnClickedTracingOpen)
ON_BN_CLICKED(IDC_TRAMAP, &EoDlgFileManage::OnBnClickedTracingMap)
ON_BN_CLICKED(IDC_TRAVIEW, &EoDlgFileManage::OnBnClickedTracingView)
ON_BN_CLICKED(IDC_TRACLOAK, &EoDlgFileManage::OnBnClickedTracingCloak)
ON_BN_CLICKED(IDC_TRAFUSE, &EoDlgFileManage::OnBnClickedTracingFuse)
ON_BN_CLICKED(IDC_TRAEXCLUDE, &EoDlgFileManage::OnBnClickedTracingExclude)
ON_BN_CLICKED(IDC_TRAINCLUDE, &EoDlgFileManage::OnBnClickedTracingInclude)
ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgFileManage::OnLbnSelchangeBlocksList)
ON_LBN_SELCHANGE(IDC_TRA, &EoDlgFileManage::OnLbnSelchangeTracingList)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_NOTIFY(NM_CLICK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnNMClickLayersListControl)
ON_NOTIFY(NM_DBLCLK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnNMDblclkLayersListControl)
ON_NOTIFY(LVN_ITEMCHANGED, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnItemchangedLayersListControl)
#pragma warning(pop)
END_MESSAGE_MAP()

EoDlgFileManage::EoDlgFileManage(CWnd* parent /*=nullptr*/) : CDialog(EoDlgFileManage::IDD, parent) {}
EoDlgFileManage::EoDlgFileManage(AeSysDoc* document, CWnd* parent /*=nullptr*/)
    : CDialog(EoDlgFileManage::IDD, parent), m_Document(document) {}
EoDlgFileManage::~EoDlgFileManage() {}
void EoDlgFileManage::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_TRA, m_tracingList);
  DDX_Control(dataExchange, IDC_LAYERS_LIST_CONTROL, m_layersListControl);
  DDX_Control(dataExchange, IDC_BLOCKS_LIST, m_blocksList);
  DDX_Control(dataExchange, IDC_REFERENCES, m_references);
  DDX_Control(dataExchange, IDC_GROUPS, m_groups);
  DDX_Control(dataExchange, IDC_TRACLOAK, m_tracingCloakRadioButton);
  DDX_Control(dataExchange, IDC_TRAOPEN, m_tracingOpenRadioButton);
  DDX_Control(dataExchange, IDC_TRAMAP, m_tracingMapRadioButton);
  DDX_Control(dataExchange, IDC_TRAVIEW, m_tracingViewRadioButton);
}
BOOL EoDlgFileManage::OnInitDialog() {
  CDialog::OnInitDialog();

  CString captionText;
  GetWindowTextW(captionText);
  captionText += L" - ";
  captionText += m_Document->GetPathName();
  SetWindowTextW(captionText);

  m_previewWindowHandle = GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd();

  m_layersListControl.DeleteAllItems();
  m_layersListControl.InsertColumn(Status, L"Status", LVCFMT_LEFT, 56);
  m_layersListControl.InsertColumn(Name, L"Layer", LVCFMT_LEFT, 104);
  m_layersListControl.InsertColumn(On, L"On", LVCFMT_CENTER, 34);
  m_layersListControl.InsertColumn(Freeze, L"Freeze", LVCFMT_CENTER, 60);
  m_layersListControl.InsertColumn(Lock, L"Lock", LVCFMT_CENTER, 48);
  m_layersListControl.InsertColumn(Plot, L"Plot", LVCFMT_CENTER, 40);
  m_layersListControl.InsertColumn(Color, L"Color", LVCFMT_LEFT, 72);
  m_layersListControl.InsertColumn(LineType, L"LineType", LVCFMT_LEFT, 120);
  m_numberOfColumns = m_layersListControl.InsertColumn(LineWeight, L"LineWeight", LVCFMT_LEFT, 96);
  m_numberOfColumns++;

  m_tracingList.SetHorizontalExtent(512);

  for (int i = 0; i < m_Document->GetLayerTableSize(); i++) {
    EoDbLayer* Layer = m_Document->GetLayerTableLayerAt(i);

    if (Layer->IsInternal()) {
      m_layersListControl.InsertItem(i, L"");
      m_layersListControl.SetItemData(i, DWORD_PTR(Layer));
    } else {
      int ItemIndex = m_tracingList.AddString(Layer->Name());
      m_tracingList.SetItemData(ItemIndex, DWORD_PTR(Layer));
    }
  }
  m_blocksList.SetHorizontalExtent(512);

  CString blockName;
  EoDbBlock* block{};

  auto position = m_Document->GetFirstBlockPosition();
  while (position != nullptr) {
    m_Document->GetNextBlock(position, blockName, block);
    if (block->IsAnonymous() || block->IsSystemBlock(blockName)) { continue; }
    if (block->IsModelSpace(blockName.GetString()) || block->IsPaperSpace(blockName.GetString())) { continue; }
    auto itemIndex = m_blocksList.AddString(blockName);
    m_blocksList.SetItemData(itemIndex, DWORD_PTR(block));
  }
  CBitmap Bitmap;
  Bitmap.LoadBitmapW(IDB_LAYER_STATES);
  m_stateImages.Create(24, 24, ILC_COLOR32 | ILC_MASK, 0, 1);
  m_stateImages.Add(&Bitmap, Eo::bitmapMaskColor);

  WndProcPreviewClear(m_previewWindowHandle);

  return TRUE;
}
void EoDlgFileManage::OnBnClickedLayerRename() {
  auto position = m_layersListControl.GetFirstSelectedItemPosition();
  if (position == nullptr) { return; }

  int item = m_layersListControl.GetNextSelectedItem(position);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));
  if (!layer->Name().Compare(L"0")) {
    app.WarningMessageBox(IDS_MSG_LAYER_NO_RENAME_0);
    return;
  }
  EoDlgGetLayerName Dialog;
  Dialog.m_Name = layer->Name();

  if (Dialog.DoModal() != IDOK) { return; }

  LVFINDINFO ListViewFindInfo{};
  ListViewFindInfo.flags = LVFI_STRING;
  ListViewFindInfo.psz = Dialog.m_Name;

  if (!Dialog.m_Name.IsEmpty() && m_layersListControl.FindItem(&ListViewFindInfo) == -1) {
    layer->SetName(Dialog.m_Name);

    if (layer->IsWork()) { m_Document->SetWorkLayer(layer); }
    m_layersListControl.SetItemText(item, Name, Dialog.m_Name);
  }
}

void EoDlgFileManage::OnBnClickedLayerMelt() {
  auto position = m_layersListControl.GetFirstSelectedItemPosition();
  if (position != nullptr) {
    int item = m_layersListControl.GetNextSelectedItem(position);
    auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

    CString layerName = layer->Name();
    if (m_Document->LayerMelt(layerName)) {
      m_layersListControl.DeleteItem(item);
      int itemIndex = m_tracingList.AddString(layerName);
      m_tracingList.SetItemData(itemIndex, DWORD_PTR(layer));
    }
  }
}

void EoDlgFileManage::OnBnClickedTracingOpen() {
  int CurrentSelection = m_tracingList.GetCurSel();
  if (CurrentSelection != LB_ERR) {
    if (m_tracingList.GetTextLen(CurrentSelection) != LB_ERR) {
      CString LayerName;
      m_tracingList.GetText(CurrentSelection, LayerName);
      m_Document->TracingOpen(LayerName);
    }
  }
}
void EoDlgFileManage::OnBnClickedTracingMap() {
  int CurrentSelection = m_tracingList.GetCurSel();
  if (CurrentSelection != LB_ERR) {
    if (m_tracingList.GetTextLen(CurrentSelection) != LB_ERR) {
      CString LayerName;
      m_tracingList.GetText(CurrentSelection, LayerName);
      m_Document->TracingMap(LayerName);
    }
  }
}
void EoDlgFileManage::OnBnClickedTracingView() {
  int CurrentSelection = m_tracingList.GetCurSel();
  if (CurrentSelection != LB_ERR) {
    if (m_tracingList.GetTextLen(CurrentSelection) != LB_ERR) {
      CString LayerName;
      m_tracingList.GetText(CurrentSelection, LayerName);
      m_Document->TracingView(LayerName);
    }
  }
}
void EoDlgFileManage::OnBnClickedTracingCloak() {
  int currentSelection = m_tracingList.GetCurSel();
  if (currentSelection == LB_ERR) { return; }
  if (m_tracingList.GetTextLen(currentSelection) == LB_ERR) { return; }

  CString layerName;
  m_tracingList.GetText(currentSelection, layerName);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_tracingList.GetItemData(currentSelection));
  if (layer->IsOpened()) {
    app.WarningMessageBox(IDS_MSG_OPEN_TRACING_NO_CLOAK);
    m_tracingCloakRadioButton.SetCheck(0);
    m_tracingOpenRadioButton.SetCheck(1);
  } else {
    m_Document->UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
    layer->SetStateOff();
    layer->SetTracingState(static_cast<std::uint16_t>(EoDbLayer::TracingState::isCloaked));
  }
}

void EoDlgFileManage::OnBnClickedTracingFuse() {
  int CurrentSelection = m_tracingList.GetCurSel();
  if (CurrentSelection == LB_ERR) { return; }
  if (m_tracingList.GetTextLen(CurrentSelection) == LB_ERR) { return; }

  CString layerName;
  m_tracingList.GetText(CurrentSelection, layerName);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_tracingList.GetItemData(CurrentSelection));

  m_Document->TracingFuse(layerName);
  m_tracingList.DeleteString(static_cast<UINT>(CurrentSelection));
  int itemCount = m_layersListControl.GetItemCount();
  m_layersListControl.InsertItem(itemCount, L"");
  m_layersListControl.SetItemData(itemCount, DWORD_PTR(layer));
}

void EoDlgFileManage::OnBnClickedTracingExclude() {
  int currentSelection = m_tracingList.GetCurSel();
  if (currentSelection == LB_ERR) { return; }

  if (m_tracingList.GetTextLen(currentSelection) == LB_ERR) { return; }
  CString layerName;
  m_tracingList.GetText(currentSelection, layerName);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_tracingList.GetItemData(currentSelection));
  m_Document->UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
  m_Document->RemoveLayerTableLayer(layerName);
  m_tracingList.DeleteString(static_cast<UINT>(currentSelection));
}

void EoDlgFileManage::OnBnClickedTracingInclude() {
  static DWORD filterIndex{1};

  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER_TRACINGS);

  OPENFILENAME of{};
  of.lStructSize = sizeof(OPENFILENAME);
  of.hInstance = AeSys::GetInstance();
  of.lpstrFilter = filter;
  of.nFilterIndex = filterIndex;
  of.lpstrFile = new wchar_t[MAX_PATH];
  of.lpstrFile[0] = 0;
  of.nMaxFile = MAX_PATH;
  of.lpstrTitle = L"Include Tracing";
  of.Flags = OFN_HIDEREADONLY;
  of.lpstrDefExt = L"tra";

  if (GetOpenFileNameW(&of)) {
    filterIndex = of.nFilterIndex;

    CString strName = of.lpstrFile;
    CString strPath = strName.Left(of.nFileOffset);

    strName = strName.Mid(of.nFileOffset);

    if (m_Document->GetLayerTableLayer(strName) == 0) {
      if (m_Document->TracingMap(strName)) {
        auto* layer = m_Document->GetLayerTableLayer(strName);
        layer->MakeResident();

        CString strOpenName = m_Document->GetPathName();

        if (strOpenName.Find(strPath) == -1) {
          m_Document->TracingFuse(strName);

          strName = of.lpstrFile;
          strName = strName.Mid(of.nFileOffset, of.nFileExtension - of.nFileOffset - 1);
          layer->SetName(strName);
        }
        int ItemIndex = m_tracingList.AddString(strName);
        m_tracingList.SetItemData(ItemIndex, DWORD_PTR(layer));
      }
    }
  }
  delete[] of.lpstrFile;
}

void EoDlgFileManage::OnLbnSelchangeBlocksList() {
  int CurrentSelection = m_blocksList.GetCurSel();
  if (CurrentSelection != LB_ERR) {
    if (m_blocksList.GetTextLen(CurrentSelection) != LB_ERR) {
      CString BlockName;
      m_blocksList.GetText(CurrentSelection, BlockName);
      EoDbBlock* Block = (EoDbBlock*)m_blocksList.GetItemData(CurrentSelection);

      m_groups.SetDlgItemInt(IDC_GROUPS, static_cast<UINT>(Block->GetCount()), FALSE);
      m_references.SetDlgItemInt(
          IDC_REFERENCES, static_cast<UINT>(m_Document->GetBlockReferenceCount(BlockName)), FALSE);
      WndProcPreviewUpdateBlock(m_previewWindowHandle, Block);
    }
  }
}

void EoDlgFileManage::OnLbnSelchangeTracingList() {
  int currentSelection = m_tracingList.GetCurSel();
  if (currentSelection != LB_ERR) { return; }
  auto* layer = reinterpret_cast<EoDbLayer*>(m_tracingList.GetItemData(currentSelection));

  m_tracingOpenRadioButton.SetCheck(layer->IsOpened());
  m_tracingMapRadioButton.SetCheck(layer->IsMapped());
  m_tracingViewRadioButton.SetCheck(layer->IsViewed());
  m_tracingCloakRadioButton.SetCheck(layer->IsOff());

  CString numberOfGroups;
  numberOfGroups.Format(L"%-4i", static_cast<int>(layer->GetCount()));
  m_groups.SetWindowTextW(numberOfGroups);

  WndProcPreviewUpdateLayer(m_previewWindowHandle, layer);
}

void EoDlgFileManage::OnBnClickedMfcbuttonWork() {
  auto position = m_layersListControl.GetFirstSelectedItemPosition();
  if (position == nullptr) { return; }

  int item = m_layersListControl.GetNextSelectedItem(position);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));
  auto* previousWorkLayer = m_Document->SetWorkLayer(layer);

  CString workLayer;
  workLayer.Format(L"Current Layer: %s", layer->Name().GetString());
  GetDlgItem(IDC_STATIC_WORK_LAYER)->SetWindowTextW(workLayer);

  previousWorkLayer->SetStateActive();

  m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, previousWorkLayer);
  m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);

  GetDlgItem(IDC_LAYERS_LIST_CONTROL)->RedrawWindow();
}

void EoDlgFileManage::OnBnClickedMfcbuttonNew() {
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

void EoDlgFileManage::OnBnClickedMfcbuttonDel() {
  auto position = m_layersListControl.GetFirstSelectedItemPosition();
  if (position == nullptr) { return; }
  int item = m_layersListControl.GetNextSelectedItem(position);
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

  if (layer->Name() == L"0") {
    app.WarningMessageBox(IDS_MSG_LAYER_NO_DELETE_0);
  } else if (layer->IsWork()) {
    app.WarningMessageBox(IDS_MSG_LAYER_NO_DELETE_WORK, layer->Name());
  } else {
    m_Document->UpdateAllViews(nullptr, EoDb::kLayerErase, layer);

    auto layerTableIndex = m_Document->FindLayerTableLayer(layer->Name());
    m_Document->RemoveLayerTableLayerAt(layerTableIndex);

    m_layersListControl.DeleteItem(item);
  }
}

void EoDlgFileManage::DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& itemRectangle) {
  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(itemID));

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
      deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, layerName,
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
      colorRectangle.OffsetRect(0, -2);

      CBrush colorBrush(layer->ColorValue());
      deviceContext.FillRect(&colorRectangle, &colorBrush);

      CBrush frameBrush;
      frameBrush.CreateSysColorBrush(COLOR_WINDOWFRAME);
      deviceContext.FrameRect(&colorRectangle, &frameBrush);
      if (colorRectangle.right + 4 < itemRectangle.right) {
        CString colorName;
        colorName.Format(L"%i", layer->ColorIndex());
        deviceContext.ExtTextOutW(colorRectangle.right + 4, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle,
            colorName, static_cast<UINT>(colorName.GetLength()), nullptr);
      }
    } break;
    case LineType: {
      CString LineTypeName = layer->LineTypeName();
      deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle,
          LineTypeName, static_cast<UINT>(LineTypeName.GetLength()), nullptr);
    } break;
    case LineWeight: {
      CString lineWeight;
      lineWeight.Format(L"%hd", static_cast<std::int16_t>(layer->LineWeight()));
      deviceContext.ExtTextOutW(itemRectangle.left + 8, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, lineWeight,
          static_cast<UINT>(lineWeight.GetLength()), nullptr);
    } break;
  }
}

void EoDlgFileManage::OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT lpDrawItemStruct) {
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

void EoDlgFileManage::OnNMClickLayersListControl(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

  auto item = pNMItemActivate->iItem;
  if (item < 0) { return; }

  auto subItem = pNMItemActivate->iSubItem;

  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

  m_clickToColumnName = false;
  switch (subItem) {
    case Name:
      m_clickToColumnName = true;
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

void EoDlgFileManage::OnNMDblclkLayersListControl(NMHDR* pNMHDR, LRESULT* result) {
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

  auto item = pNMItemActivate->iItem;
  if (item < 0) { return; }

  auto subItem = pNMItemActivate->iSubItem;

  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

  m_clickToColumnName = false;
  switch (subItem) {
    case Status: {
      auto* previousWorkLayer = m_Document->SetWorkLayer(layer);

      CString workLayer;
      workLayer.Format(L"Current Layer: %s", layer->Name().GetString());
      GetDlgItem(IDC_STATIC_WORK_LAYER)->SetWindowTextW(workLayer);

      previousWorkLayer->SetStateActive();

      m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, previousWorkLayer);
      m_Document->UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);

      GetDlgItem(IDC_LAYERS_LIST_CONTROL)->RedrawWindow();
    }
  }
  *result = 0;
}

void EoDlgFileManage::OnItemchangedLayersListControl(NMHDR* pNMHDR, LRESULT* result) {
  LPNMLISTVIEW ListViewNotificationMessage = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

  int item = ListViewNotificationMessage->iItem;

  auto* layer = reinterpret_cast<EoDbLayer*>(m_layersListControl.GetItemData(item));

  CString numberOfGroups;
  numberOfGroups.Format(L"%-4i", static_cast<int>(layer->GetCount()));
  m_groups.SetWindowTextW(numberOfGroups);

  EoDbPrimitive::SetLayerColor(layer->ColorIndex());
  EoDbPrimitive::SetLayerLineTypeIndex(layer->LineTypeIndex());
  EoDbPrimitive::SetLayerLineTypeName(std::wstring(layer->LineTypeName()));

  WndProcPreviewUpdateLayer(m_previewWindowHandle, layer);

  *result = 0;
}
