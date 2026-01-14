#include "stdafx.h"

#if defined(USING_ODA)
#include "EoDlgLayerPropertiesManager.h"
#include "EoDlgLineWeight.h"

#include "DbLayerTable.h"
#include "DbLayerTableRecord.h"
#include "DbSymUtl.h"
#include "DbViewport.h"

OdDb::LineWeight LineWeightByIndex(char lineWeight) {
  switch (lineWeight) {
    case 0:
      return OdDb::kLnWt000;
    case 1:
      return OdDb::kLnWt005;
    case 2:
      return OdDb::kLnWt009;
    case 3:
      return OdDb::kLnWt013;
    case 4:
      return OdDb::kLnWt015;
    case 5:
      return OdDb::kLnWt018;
    case 6:
      return OdDb::kLnWt020;
    case 7:
      return OdDb::kLnWt025;
    case 8:
      return OdDb::kLnWt030;
    case 9:
      return OdDb::kLnWt035;
    case 10:
      return OdDb::kLnWt040;
    case 11:
      return OdDb::kLnWt050;
    case 12:
      return OdDb::kLnWt053;
    case 13:
      return OdDb::kLnWt060;
    case 14:
      return OdDb::kLnWt070;
    case 15:
      return OdDb::kLnWt080;
    case 16:
      return OdDb::kLnWt090;
    case 17:
      return OdDb::kLnWt100;
    case 18:
      return OdDb::kLnWt106;
    case 19:
      return OdDb::kLnWt120;
    case 20:
      return OdDb::kLnWt140;
    case 21:
      return OdDb::kLnWt158;
    case 22:
      return OdDb::kLnWt200;
    case 23:
      return OdDb::kLnWt211;
    case 30:
      return OdDb::kLnWtByBlock;
    case 31:
      return OdDb::kLnWtByLwDefault;
  }
  return OdDb::kLnWtByLayer;
}

CString StringByLineWeight(int lineWeight, bool lineWeightByIndex) {
  if (lineWeightByIndex) { lineWeight = LineWeightByIndex(char(lineWeight)); }
  CString LineWeightText = L"";
  switch (lineWeight) {
    case OdDb::kLnWtByLayer:
      LineWeightText = L"Layer";
      break;
    case OdDb::kLnWtByBlock:
      LineWeightText = L"Block";
      break;
    case OdDb::kLnWtByLwDefault:
      LineWeightText = L"Default";
      break;
    default:
      LineWeightText.Format(L"%1.2f mm", (float)lineWeight / 100);
  }
  return LineWeightText;
}

// EoDlgLayerPropertiesManager dialog

IMPLEMENT_DYNAMIC(EoDlgLayerPropertiesManager, CDialog)

BEGIN_MESSAGE_MAP(EoDlgLayerPropertiesManager, CDialog)
ON_WM_CREATE()
ON_WM_DRAWITEM()
ON_WM_SIZE()
ON_WM_SIZING()
ON_BN_CLICKED(IDC_BUTTON_ADD, &EoDlgLayerPropertiesManager::OnBnClickedButtonAdd)
ON_BN_CLICKED(IDC_BUTTON_CURRENT, &EoDlgLayerPropertiesManager::OnBnClickedButtonCurrent)
ON_NOTIFY(NM_DBLCLK, IDC_LAYER_FILTER_TREE, &EoDlgLayerPropertiesManager::OnNMDblclkLayerFilterTree)
ON_NOTIFY(TVN_KEYDOWN, IDC_LAYER_FILTER_TREE, &EoDlgLayerPropertiesManager::OnTvnKeydownLayerFilterTree)
ON_NOTIFY(NM_CLICK, IDC_LAYERS_LIST_CONTROL, &EoDlgLayerPropertiesManager::OnNMClickListLayersList)
ON_NOTIFY(NM_DBLCLK, IDC_LAYERS_LIST_CONTROL, &EoDlgLayerPropertiesManager::OnNMDblclkListLayersList)
ON_NOTIFY(NM_RCLICK, IDC_LAYERS_LIST_CONTROL, &EoDlgLayerPropertiesManager::OnNMRClickListLayersList)
END_MESSAGE_MAP()

EoDlgLayerPropertiesManager::EoDlgLayerPropertiesManager(CWnd* parent /*=nullptr*/)
    : CDialog(EoDlgLayerPropertiesManager::IDD, parent) {}
EoDlgLayerPropertiesManager::EoDlgLayerPropertiesManager(OdDbDatabase* database, CWnd* parent /*=nullptr*/)
    : CDialog(EoDlgLayerPropertiesManager::IDD, parent), m_Database(database) {}

EoDlgLayerPropertiesManager::~EoDlgLayerPropertiesManager() {}

void EoDlgLayerPropertiesManager::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Control(dataExchange, IDC_LAYERS_LIST_CONTROL, m_LayerList);
  DDX_Control(dataExchange, IDC_LAYER_FILTER_TREE, m_TreeFilters);
}

// EoDlgLayerPropertiesManager message handlers

int EoDlgLayerPropertiesManager::OnCreate(LPCREATESTRUCT createStructure) {
  if (CDialog::OnCreate(createStructure) == -1) { return -1; }
  m_InititialWidth = createStructure->cx;
  m_InititialHeight = createStructure->cy;

  return 0;
}

void EoDlgLayerPropertiesManager::OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT lpDrawItemStruct) {
  if (IDC_LAYERS_LIST_CONTROL == controlIdentifier) {
    CRect rcItem;
    switch (lpDrawItemStruct->itemAction) {
      case ODA_DRAWENTIRE: {
        //clear item
        rcItem = lpDrawItemStruct->rcItem;
        CDC DeviceContext;
        COLORREF rgbBkgnd =
            ::GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW);
        DeviceContext.Attach(lpDrawItemStruct->hDC);
        CBrush br(rgbBkgnd);
        DeviceContext.FillRect(rcItem, &br);
        if (lpDrawItemStruct->itemState & ODS_FOCUS) { DeviceContext.DrawFocusRect(rcItem); }
        int itemID = lpDrawItemStruct->itemID;
        if (itemID != -1) {
          // The text color is stored as the item data.
          COLORREF rgbText = (lpDrawItemStruct->itemState & ODS_SELECTED) ? ::GetSysColor(COLOR_HIGHLIGHTTEXT)
                                                                          : ::GetSysColor(COLOR_WINDOWTEXT);
          DeviceContext.SetBkColor(rgbBkgnd);
          DeviceContext.SetTextColor(rgbText);
          for (int labelIndex = 0; labelIndex < columns_count; ++labelIndex) {
            m_LayerList.GetSubItemRect(itemID, labelIndex, LVIR_LABEL, rcItem);
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

void EoDlgLayerPropertiesManager::OnSize(UINT type, int newWidth, int newHeight) {
  CDialog::OnSize(type, newWidth, newHeight);

  CRect itemRect;
  CRect dlgRect;
  if (m_LayerList.m_hWnd) {
    m_LayerList.GetWindowRect(&itemRect);
    ScreenToClient(itemRect);
    GetWindowRect(&dlgRect);
    itemRect.right += (dlgRect.Width() - m_DeltaWidth);
    itemRect.bottom += (dlgRect.Height() - m_DeltaHeight);
    m_LayerList.MoveWindow(itemRect);
  }
  if (GetDlgItem(IDC_STATIC_CURRENT_LAYER)) {
    GetDlgItem(IDC_STATIC_CURRENT_LAYER)->GetWindowRect(&itemRect);
    ScreenToClient(itemRect);
    GetWindowRect(&dlgRect);
    itemRect.right += (dlgRect.Width() - m_DeltaWidth);
    GetDlgItem(IDC_STATIC_CURRENT_LAYER)->MoveWindow(itemRect);
  }
  if (GetDlgItem(IDC_STATIC_LAYER_STATISTIC)) {
    GetDlgItem(IDC_STATIC_LAYER_STATISTIC)->GetWindowRect(&itemRect);
    ScreenToClient(itemRect);
    GetWindowRect(&dlgRect);
    GetDlgItem(IDC_STATIC_LAYER_STATISTIC)
        ->MoveWindow(itemRect.left, itemRect.top + (dlgRect.Height() - m_DeltaHeight),
                     itemRect.Width() + (dlgRect.Width() - m_DeltaWidth), itemRect.Height());
  }
  if (GetDlgItem(IDCANCEL)) {
    GetDlgItem(IDCANCEL)->GetWindowRect(&itemRect);
    ScreenToClient(itemRect);
    GetWindowRect(&dlgRect);
    GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
    GetDlgItem(IDCANCEL)->MoveWindow(itemRect.left + (dlgRect.Width() - m_DeltaWidth),
                                     itemRect.top + (dlgRect.Height() - m_DeltaHeight), itemRect.Width(),
                                     itemRect.Height());
    GetDlgItem(IDCANCEL)->ShowWindow(SW_SHOW);
  }
  if (GetDlgItem(IDOK)) {
    GetDlgItem(IDOK)->GetWindowRect(&itemRect);
    ScreenToClient(itemRect);
    GetWindowRect(&dlgRect);
    GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
    GetDlgItem(IDOK)->MoveWindow(itemRect.left + (dlgRect.Width() - m_DeltaWidth),
                                 itemRect.top + (dlgRect.Height() - m_DeltaHeight), itemRect.Width(),
                                 itemRect.Height());
    GetDlgItem(IDOK)->ShowWindow(SW_SHOW);
  }
}

void EoDlgLayerPropertiesManager::OnSizing(UINT side, LPRECT rectangle) {
  CDialog::OnSizing(side, rectangle);

  CRect rct(*rectangle);

  CRect dlgRect;
  GetWindowRect(&dlgRect);
  m_DeltaWidth = dlgRect.Width();
  m_DeltaHeight = dlgRect.Height();

  if (rct.Width() < m_InititialWidth) {
    switch (side) {
      case WMSZ_LEFT:
      case WMSZ_BOTTOMLEFT:
      case WMSZ_TOPLEFT:
        rectangle->left = rectangle->right - m_InititialWidth;
        break;
      case WMSZ_RIGHT:
      case WMSZ_BOTTOMRIGHT:
      case WMSZ_TOPRIGHT:
        rectangle->right = rectangle->left + m_InititialWidth;
        break;
    }
  }
  if (rct.Height() < m_InititialHeight) {
    switch (side) {
      case WMSZ_BOTTOM:
      case WMSZ_BOTTOMLEFT:
      case WMSZ_BOTTOMRIGHT:
        rectangle->bottom = rectangle->top + m_InititialHeight;
        break;
      case WMSZ_TOP:
      case WMSZ_TOPLEFT:
      case WMSZ_TOPRIGHT:
        rectangle->top = rectangle->bottom - m_InititialHeight;
        break;
    }
  }
}

void EoDlgLayerPropertiesManager::OnBnClickedButtonAdd() {
  OdDbLayerTablePtr Layers = m_Database->getLayerTableId().safeOpenObject(OdDb::kForWrite);
  OdDbLayerTableRecordPtr Layer = OdDbLayerTableRecord::createObject();

  OdString Name;
  int Suffix = 1;
  do { Name.format(OD_T("Layer%d"), Suffix++); } while (Layers->has(Name));

  Layer->setName(Name);
  Layers->add(Layer);

  int ItemCount = m_LayerList.GetItemCount();
  m_LayerList.InsertItem(ItemCount, nullptr);
  m_LayerList.SetItemData(ItemCount, (unsigned long)(OdDbStub*)Layer->objectId());
}

void EoDlgLayerPropertiesManager::OnBnClickedButtonCurrent() {
  OdDbLayerTableRecordPtr Layer;
  if (OpenSelectedLayer(Layer)) {
    m_Database->setCLAYER(Layer->objectId());
    UpdateCurrentLayerInfoField();
    GetDlgItem(IDC_LAYERS_LIST_CONTROL)->RedrawWindow();
  }
}

void EoDlgLayerPropertiesManager::OnNMDblclkLayerFilterTree(NMHDR* /* pNMHDR */, LRESULT* pResult) {
  if (HTREEITEM h = m_TreeFilters.GetSelectedItem()) {
    OdLyLayerFilter* lf = (OdLyLayerFilter*)(void*)m_TreeFilters.GetItemData(h);
    if (!lf->dynamicallyGenerated() && !lf->isIdFilter()) {
      //OdaLayerFilterPropDlg(lf, this).DoModal();
    }
  }
  *pResult = 0;
}

void EoDlgLayerPropertiesManager::OnTvnKeydownLayerFilterTree(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMTVKEYDOWN pTVKeyDown = reinterpret_cast<LPNMTVKEYDOWN>(pNMHDR);

  if (pTVKeyDown->wVKey == VK_DELETE) {
    if (HTREEITEM SelectedItem = m_TreeFilters.GetSelectedItem()) {
      OdLyLayerFilter* Filter = (OdLyLayerFilter*)(void*)m_TreeFilters.GetItemData(SelectedItem);
      if (Filter->dynamicallyGenerated()) return;
      if (AfxMessageBox(L"Delete this filter?", MB_YESNO) != IDYES) return;
      Filter->parent()->removeNested(Filter);
      m_TreeFilters.DeleteItem(SelectedItem);

      OdLyLayerFilter* Root = (OdLyLayerFilter*)(void*)m_TreeFilters.GetItemData(m_TreeFilters.GetRootItem());
      ::odlyGetLayerFilterManager(m_Database)->setFilters(Root, Root);
    }
  }
  *pResult = 0;
}
void EoDlgLayerPropertiesManager::OnNMClickListLayersList(NMHDR* pNMHDR, LRESULT* pResult) {
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

  int Item = pNMItemActivate->iItem;
  int SubItem = pNMItemActivate->iSubItem;

  OdDbObjectId ItemData = (OdDbStub*)(unsigned long)m_LayerList.GetItemData(Item);
  OdDbLayerTableRecordPtr Layer = ItemData.safeOpenObject(OdDb::kForWrite);
  m_ClickToColumnName = false;
  switch (SubItem) {
    case column_name:
      m_ClickToColumnName = true;
      return;
    case column_on:
      Layer->setIsOff(!Layer->isOff());
      break;
    case column_freeze:
      Layer->setIsFrozen(!Layer->isFrozen());
      break;
    case column_lock:
      Layer->setIsLocked(!Layer->isLocked());
      break;
    case column_color: {
      //OdaLayerPropColorDlg dlg(Layer->colorIndex());
      //if (IDOK == dlg.DoModal()) {
      //	Layer->setColorIndex((OdInt16)dlg.m_nColor);
      //}
      break;
    }
    case column_linetype: {
      //OdaLayerPropLineTypeDlg dlg(m_pDb, Layer->linetypeObjectId(), true);
      //if (IDOK == dlg.DoModal()) {
      //	Layer->setLinetypeObjectId(dlg.m_originalId);
      //}
      break;
    }
    case column_lineweight: {
      EoDlgLineWeight dlg(Layer->lineWeight());
      if (IDOK == dlg.DoModal()) { Layer->setLineWeight(dlg.m_LineWeight); }
      break;
    }
    case column_plotstyle:
      break;
    case column_plot:
      Layer->setIsPlottable(!Layer->isPlottable());
      break;
    case column_vpfreeze:
    case column_descr:
      if (SubItem != column_desc) {
        OdDbViewportPtr pVp = OdDbViewport::cast(Layer->database()->activeViewportId().safeOpenObject(OdDb::kForWrite));
        if (pVp.get()) {
          OdDbObjectIdArray ids(1);
          ids.append(ItemData);
          if (pVp->isLayerFrozenInViewport(ItemData))
            pVp->thawLayersInViewport(ids);
          else
            pVp->freezeLayersInViewport(ids);
        }
      } else {
      }
      break;
    case column_vpcolor: {
      //OdaLayerPropColorDlg dlg(Layer->colorIndex());
      //if (IDOK == dlg.DoModal()) {
      //	OdCmColor col;
      //	col.setColorIndex((OdInt16)dlg.m_nColor);
      //	Layer->setColor(col, m_vp);
      //}
      break;
    }
    case column_vplinetype: {
      //OdaLayerPropLineTypeDlg dlg(m_pDb, Layer->linetypeObjectId(), true);
      //if (IDOK == dlg.DoModal()) {
      //	Layer->setLinetypeObjectId(dlg.m_originalId, m_vp);
      //}
      break;
    }
    case column_vplineweight: {
      EoDlgLineWeight dlg(Layer->lineWeight());
      if (IDOK == dlg.DoModal()) { Layer->setLineWeight(dlg.m_LineWeight, m_ActiveViewport); }
      break;
    }
    case column_vpplotstyle:
      break;
  }
  m_LayerList.Invalidate();

  *pResult = 0;
}

void EoDlgLayerPropertiesManager::OnNMDblclkListLayersList(NMHDR* /* pNMHDR */, LRESULT* pResult) {
  if (m_ClickToColumnName) { OnBnClickedButtonCurrent(); }
  *pResult = 0;
}

void EoDlgLayerPropertiesManager::OnNMRClickListLayersList(NMHDR* /* pNMHDR */, LRESULT* pResult) {
  //LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<NMITEMACTIVATE>(pNMHDR);
  // TODO: Add your control notification handler code here
  *pResult = 0;
}

BOOL EoDlgLayerPropertiesManager::OnInitDialog() {
  CDialog::OnInitDialog();

  m_LayerList.DeleteAllItems();
  m_LayerList.InsertColumn(column_status, L"Status", LVCFMT_LEFT, 48);
  m_LayerList.InsertColumn(column_name, L"Name", LVCFMT_LEFT, 96);
  m_LayerList.InsertColumn(column_on, L"On", LVCFMT_LEFT, 32);
  m_LayerList.InsertColumn(column_freeze, L"Freeze in all VP", LVCFMT_LEFT, 32);
  m_LayerList.InsertColumn(column_lock, L"Lock", LVCFMT_LEFT, 48);
  m_LayerList.InsertColumn(column_color, L"Color", LVCFMT_LEFT, 48);
  m_LayerList.InsertColumn(column_linetype, L"Linetype", LVCFMT_LEFT, 64);
  m_LayerList.InsertColumn(column_lineweight, L"Lineweight", LVCFMT_LEFT, 64);
  m_LayerList.InsertColumn(column_plotstyle, L"Plot Style", LVCFMT_LEFT, 64);
  columns_count = m_LayerList.InsertColumn(column_plot, L"Plot", LVCFMT_LEFT, 16);

  if (m_Database->getTILEMODE() == 0) {
    m_ActiveViewport = m_Database->activeViewportId();
    m_LayerList.InsertColumn(column_vpcolor, L"VP Freeze", LVCFMT_LEFT, 16);
    m_LayerList.InsertColumn(column_vpcolor, L"VP Color", LVCFMT_LEFT, 48);
    m_LayerList.InsertColumn(column_vplinetype, L"VP Linetype", LVCFMT_LEFT, 64);
    m_LayerList.InsertColumn(column_vplineweight, L"VP Lineweight", LVCFMT_LEFT, 64);
    columns_count = m_LayerList.InsertColumn(column_vpplotstyle, L"Plot Style", LVCFMT_LEFT, 64);
  }
  column_desc = m_LayerList.InsertColumn(++columns_count, L"Description", LVCFMT_LEFT, 96);
  columns_count++;

  OdDbLayerTablePtr Layers = m_Database->getLayerTableId().safeOpenObject();
  Layers->generateUsageData();

  OdDbSymbolTableIteratorPtr Iterator = Layers->newIterator();
  int i = 0;
  for (Iterator->start(); !Iterator->done(); Iterator->step(), ++i) {
    m_LayerList.InsertItem(i, nullptr);
    m_LayerList.SetItemData(i, (unsigned long)(OdDbStub*)Iterator->getRecordId());
  }

  CString str;
  str.Format(L"%d Total layers", m_LayerList.GetItemCount());
  GetDlgItem(IDC_STATIC_LAYER_STATISTIC)->SetWindowTextW(str);

  UpdateCurrentLayerInfoField();

  CBitmap Bitmap;
  Bitmap.LoadBitmap(IDB_LAYER_FILTERS);
  m_pTreeImages.Create(16, 16, ILC_COLOR32, 0, 1);
  m_pTreeImages.Add(&Bitmap, RGB(0, 0, 0));
  m_TreeFilters.SetImageList(&m_pTreeImages, TVSIL_NORMAL);
  Bitmap.DeleteObject();

  Bitmap.LoadBitmap(IDB_LAYER_STATES_HC);
  m_stateImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1);
  m_stateImages.Add(&Bitmap, RGB(0, 0, 128));

  UpdateFiltersTree();

  return 0;
}
bool EoDlgLayerPropertiesManager::OpenSelectedLayer(OdSmartPtr<OdDbLayerTableRecord>& layer) {
  int SelectionMark = m_LayerList.GetSelectionMark();
  if (SelectionMark > -1) {
    OdDbObjectId ItemData = (OdDbStub*)(unsigned long)m_LayerList.GetItemData(SelectionMark);
    layer = ItemData.openObject(OdDb::kForWrite);
    return !layer.isNull();
  }
  return false;
}

void EoDlgLayerPropertiesManager::UpdateCurrentLayerInfoField() {
  CString Text;
  Text.Format(L"Current Layer: %s", (LPCWSTR)OdDbSymUtil::getSymbolName(m_Database->getCLAYER()));
  GetDlgItem(IDC_STATIC_CURRENT_LAYER)->SetWindowTextW(Text);
}
///<summary>Recursive filter tree filling helper</summary>
static void updateFilterTree(CTreeCtrl& tree, HTREEITEM parent, const OdLyLayerFilter* root,
                             const OdLyLayerFilter* current) {
  if (root) {
    HTREEITEM TreeItem = tree.InsertItem(root->name(), parent);
    tree.SetItemData(TreeItem, (DWORD)(void*)root);
    int Image = root->isIdFilter() ? 2 : 1;
    tree.SetItemImage(TreeItem, Image, Image);
    for (unsigned int i = 0; i < root->getNestedFilters().length(); i++) {
      updateFilterTree(tree, TreeItem, root->getNestedFilters()[i], current);
    }
    if (current == root) { tree.SelectItem(TreeItem); }
  }
}
void EoDlgLayerPropertiesManager::UpdateFiltersTree() {
  m_TreeFilters.DeleteAllItems();
  OdLyLayerFilterManagerPtr FilterManager = ::odlyGetLayerFilterManager(m_Database);
  OdLyLayerFilterPtr pCurrent;
  if (FilterManager->getFilters(m_pRootFilter, pCurrent) != eOk) { return; }
  updateFilterTree(m_TreeFilters, TVI_ROOT, m_pRootFilter, pCurrent);
  m_TreeFilters.SetItemImage(m_TreeFilters.GetRootItem(), 0, 0);
}
void DrawColorBox(CDC& deviceContext, const RECT& itemRectangle, const OdCmColor& color) {
  CBrush Brush(RGB(color.red(), color.green(), color.blue()));
  CBrush* OldBrush = deviceContext.SelectObject(&Brush);
  CRect ItemRectangle(itemRectangle);
  ItemRectangle.DeflateRect(2, 2, 2, 2);
  ItemRectangle.right = ItemRectangle.left + ItemRectangle.bottom - ItemRectangle.top;
  deviceContext.Rectangle(&ItemRectangle);
  deviceContext.SelectObject(OldBrush);
  ItemRectangle.SetRect(ItemRectangle.right + 4, itemRectangle.top, itemRectangle.right, itemRectangle.bottom);
  if (ItemRectangle.left <= itemRectangle.right) {
    OdString TextOut = color.colorNameForDisplay();
    deviceContext.ExtTextOutW(ItemRectangle.left, ItemRectangle.top, ETO_CLIPPED, &itemRectangle, TextOut,
                              TextOut.getLength(), nullptr);
  }
}
void DrawLineWeight(CDC& deviceContext, const RECT& itemRectangle, const OdDb::LineWeight lineWeight) {
  double PixelsPerLogicalMillimeter = static_cast<double>(deviceContext.GetDeviceCaps(LOGPIXELSY) / Eo::MmPerInch);
  int PixelWidth = (lineWeight <= 0) ? 0 : int((double(lineWeight) / 100. * PixelsPerLogicalMillimeter) + 0.5);

  LOGBRUSH Brush;
  Brush.lbStyle = BS_SOLID;
  Brush.lbColor = RGB(0x00, 0x00, 0x00);
  Brush.lbHatch = 0;

  CPen Pen(PS_SOLID | PS_GEOMETRIC | PS_ENDCAP_SQUARE, PixelWidth, &Brush);

  //CPen Pen(PS_SOLID, PixelWidth, RGB(0x00, 0x00, 0x00));
  CPen* OldPen = deviceContext.SelectObject(&Pen);

  CRect ItemRectangle(itemRectangle);
  ItemRectangle.DeflateRect(2, 2, 2, 2);
  ItemRectangle.right = ItemRectangle.left + 64;

  deviceContext.MoveTo(ItemRectangle.left, ItemRectangle.CenterPoint().y);
  deviceContext.LineTo(ItemRectangle.right, ItemRectangle.CenterPoint().y);
  deviceContext.SelectObject(OldPen);

  ItemRectangle.SetRect(ItemRectangle.right + 8, itemRectangle.top, itemRectangle.right, itemRectangle.bottom);
  if (ItemRectangle.left <= itemRectangle.right) {
    OdString TextOut = StringByLineWeight(lineWeight, false);
    deviceContext.ExtTextOutW(ItemRectangle.left, ItemRectangle.top, ETO_CLIPPED, &itemRectangle, TextOut,
                              TextOut.getLength(), nullptr);
  }
}
void DrawPlotStyle(CDC& deviceContext, const RECT& itemRectangle, const CString& textOut,
                   const OdDbDatabase* database) {
  if (database->getPSTYLEMODE() == 1) {
    COLORREF OldTextColor = deviceContext.SetTextColor(GetSysColor(COLOR_GRAYTEXT));
    deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, textOut,
                              textOut.GetLength(), nullptr);
    deviceContext.SetTextColor(OldTextColor);
  } else {
    deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, textOut,
                              textOut.GetLength(), nullptr);
  }
}
///<remarks>The list item data is the layer table record object</remarks>
void EoDlgLayerPropertiesManager::DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& itemRectangle) {
  CString TextOut;
  OdDbObjectId ItemData = (OdDbStub*)(unsigned long)m_LayerList.GetItemData(itemID);
  OdDbLayerTableRecordPtr Layer = ItemData.safeOpenObject();
  switch (labelIndex) {
    case column_status:
      if (Layer->objectId() == ItemData.database()->getCLAYER()) {
        m_stateImages.Draw(&deviceContext, 8, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      } else if (Layer->isInUse()) {
        m_stateImages.Draw(&deviceContext, 9, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      }
      break;
    case column_name:
      TextOut = (LPCWSTR)Layer->getName();
      deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, TextOut,
                                TextOut.GetLength(), nullptr);
      break;
    case column_on:
      m_stateImages.Draw(&deviceContext, Layer->isOff() ? 3 : 2, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      break;
    case column_freeze:
      m_stateImages.Draw(&deviceContext, Layer->isFrozen() ? 4 : 5, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      break;
    case column_lock:
      m_stateImages.Draw(&deviceContext, Layer->isLocked() ? 0 : 1, ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
      break;
    case column_color:
      ::DrawColorBox(deviceContext, itemRectangle, Layer->color());
      break;
    case column_linetype:
      TextOut = (LPCWSTR)OdDbSymUtil::getSymbolName(Layer->linetypeObjectId());
      deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, TextOut,
                                TextOut.GetLength(), nullptr);
      break;
    case column_lineweight:
      ::DrawLineWeight(deviceContext, itemRectangle, Layer->lineWeight());
      break;
    case column_plotstyle:
      TextOut = (LPCWSTR)Layer->plotStyleName();
      ::DrawPlotStyle(deviceContext, itemRectangle, TextOut, m_Database);
      break;
    case column_plot:
      m_stateImages.Draw(&deviceContext, Layer->isPlottable() ? 6 : 7, ((CRect&)itemRectangle).TopLeft(),
                         ILD_TRANSPARENT);
      break;
    case column_descr:
    case column_vpfreeze:
      if (labelIndex != column_desc) {
        OdDbViewportPtr Viewport = OdDbViewport::cast(Layer->database()->activeViewportId().safeOpenObject());
        if (Viewport.get()) {
          m_stateImages.Draw(&deviceContext, Viewport->isLayerFrozenInViewport(ItemData) ? 4 : 5,
                             ((CRect&)itemRectangle).TopLeft(), ILD_TRANSPARENT);
        }
      } else {
        TextOut = (LPCWSTR)Layer->description();
        deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, TextOut,
                                  TextOut.GetLength(), nullptr);
      }
      break;
    case column_vpcolor:
      ::DrawColorBox(deviceContext, itemRectangle, Layer->color(m_ActiveViewport));
      break;
    case column_vplinetype:
      TextOut = (LPCWSTR)OdDbSymUtil::getSymbolName(Layer->linetypeObjectId(m_ActiveViewport));
      deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, TextOut,
                                TextOut.GetLength(), nullptr);
      break;
    case column_vplineweight:
      ::DrawLineWeight(deviceContext, itemRectangle, Layer->lineWeight(m_ActiveViewport));
      break;
    case column_vpplotstyle:
      TextOut = (LPCWSTR)Layer->plotStyleName(m_ActiveViewport);
      ::DrawPlotStyle(deviceContext, itemRectangle, TextOut, m_Database);
      break;
  }
}
#endif
