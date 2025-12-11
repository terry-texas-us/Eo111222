#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "EoDlgFileManage.h"
#include "EoDlgSetupColor.h"
#include "EoDlgSetupLineType.h"

#include "Preview.h"

// EoDlgGetLayerName dialog

class EoDlgGetLayerName : public CDialog {
	DECLARE_DYNAMIC(EoDlgGetLayerName)

public:
	EoDlgGetLayerName(CWnd* pParent = NULL);
	virtual ~EoDlgGetLayerName();

// Dialog Data
	enum { IDD = IDD_GET_LAYER_NAME };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

public:
	CString m_Name;

protected:
	DECLARE_MESSAGE_MAP()
};

// EoDlgGetLayerName dialog

IMPLEMENT_DYNAMIC(EoDlgGetLayerName, CDialog)

BEGIN_MESSAGE_MAP(EoDlgGetLayerName, CDialog)
END_MESSAGE_MAP()

EoDlgGetLayerName::EoDlgGetLayerName(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgGetLayerName::IDD, pParent) {
}
EoDlgGetLayerName::~EoDlgGetLayerName() {
}
void EoDlgGetLayerName::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NAME, m_Name);
}

/// EoDlgFileManage dialog

IMPLEMENT_DYNAMIC(EoDlgFileManage, CDialog)

BEGIN_MESSAGE_MAP(EoDlgFileManage, CDialog)
	ON_WM_DRAWITEM()
	ON_BN_CLICKED(IDC_LAYRENAME, &EoDlgFileManage::OnBnClickedLayerRename)
	ON_BN_CLICKED(IDC_LAYMELT, &EoDlgFileManage::OnBnClickedLayerMelt)
	ON_BN_CLICKED(IDC_TRAOPEN, &EoDlgFileManage::OnBnClickedTracingOpen)
	ON_BN_CLICKED(IDC_TRAMAP, &EoDlgFileManage::OnBnClickedTracingMap)
	ON_BN_CLICKED(IDC_TRAVIEW, &EoDlgFileManage::OnBnClickedTracingView)
	ON_BN_CLICKED(IDC_TRACLOAK, &EoDlgFileManage::OnBnClickedTracingCloak)
	ON_BN_CLICKED(IDC_TRAFUSE, &EoDlgFileManage::OnBnClickedTracingFuse)
	ON_BN_CLICKED(IDC_TRAEXCLUDE, &EoDlgFileManage::OnBnClickedTracingExclude)
	ON_BN_CLICKED(IDC_TRAINCLUDE, &EoDlgFileManage::OnBnClickedTracingInclude)
	ON_LBN_SELCHANGE(IDC_BLOCKS_LIST, &EoDlgFileManage::OnLbnSelchangeBlocksList)
	ON_LBN_SELCHANGE(IDC_TRA, &EoDlgFileManage::OnLbnSelchangeTracingList)
	ON_NOTIFY(NM_CLICK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnNMClickLayersListControl)
	ON_NOTIFY(NM_DBLCLK, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnNMDblclkLayersListControl)
	ON_BN_CLICKED(IDC_MFCBUTTON_WORK, &EoDlgFileManage::OnBnClickedMfcbuttonWork)
	ON_BN_CLICKED(IDC_MFCBUTTON_NEW, &EoDlgFileManage::OnBnClickedMfcbuttonNew)
	ON_BN_CLICKED(IDC_MFCBUTTON_DEL, &EoDlgFileManage::OnBnClickedMfcbuttonDel)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LAYERS_LIST_CONTROL, &EoDlgFileManage::OnItemchangedLayersListControl)
END_MESSAGE_MAP()

EoDlgFileManage::EoDlgFileManage(CWnd* parent /*=NULL*/) :
	CDialog(EoDlgFileManage::IDD, parent) {
}
EoDlgFileManage::EoDlgFileManage(AeSysDoc* document, CWnd* parent /*=NULL*/)
: CDialog(EoDlgFileManage::IDD, parent), m_Document(document) {
}
EoDlgFileManage::~EoDlgFileManage() {
}
void EoDlgFileManage::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TRA, m_TracingList);
	DDX_Control(pDX, IDC_LAYERS_LIST_CONTROL, m_LayersListControl);
	DDX_Control(pDX, IDC_BLOCKS_LIST, m_BlocksList);
	DDX_Control(pDX, IDC_REFERENCES, m_References);
	DDX_Control(pDX, IDC_GROUPS, m_Groups);
	DDX_Control(pDX, IDC_TRACLOAK, m_TracingCloakRadioButton);
	DDX_Control(pDX, IDC_TRAOPEN, m_TracingOpenRadioButton);
	DDX_Control(pDX, IDC_TRAMAP, m_TracingMapRadioButton);
	DDX_Control(pDX, IDC_TRAVIEW, m_TracingViewRadioButton);
}
BOOL EoDlgFileManage::OnInitDialog() {
	CDialog::OnInitDialog();

	CString CaptionText;
	GetWindowTextW(CaptionText);
	CaptionText += L" - ";
	CaptionText += m_Document->GetPathName();
	SetWindowTextW(CaptionText);

	m_PreviewWindowHandle = GetDlgItem(IDC_LAYER_PREVIEW)->GetSafeHwnd();

	m_LayersListControl.DeleteAllItems();
	m_LayersListControl.InsertColumn(Status, L"Status", LVCFMT_LEFT, 32);
	m_LayersListControl.InsertColumn(Name, L"Layer", LVCFMT_LEFT, 96);
	m_LayersListControl.InsertColumn(On, L"On", LVCFMT_LEFT, 32);
	m_LayersListControl.InsertColumn(Freeze, L"Freeze in all VP", LVCFMT_LEFT, 32);
	m_LayersListControl.InsertColumn(Lock, L"Lock", LVCFMT_LEFT, 32);
	m_LayersListControl.InsertColumn(Color, L"Color", LVCFMT_LEFT, 48);
	m_LayersListControl.InsertColumn(LineType, L"LineType", LVCFMT_LEFT, 64);
	NumberOfColumns = m_LayersListControl.InsertColumn(LineWeight, L"LineWeight", LVCFMT_LEFT, 48);
	NumberOfColumns++;

	m_TracingList.SetHorizontalExtent(512);

	for (int i = 0; i < m_Document->GetLayerTableSize(); i++) {
		EoDbLayer* Layer = m_Document->GetLayerTableLayerAt(i);

		if (Layer->IsInternal()) {
			m_LayersListControl.InsertItem(i, NULL);
			m_LayersListControl.SetItemData(i, DWORD_PTR(Layer));
		}
		else {
			int ItemIndex = m_TracingList.AddString(Layer->Name());
			m_TracingList.SetItemData(ItemIndex, DWORD_PTR(Layer));
		}
	}
	m_BlocksList.SetHorizontalExtent(512);

	CString BlockName;
	EoDbBlock* Block;

	POSITION Position = m_Document->GetFirstBlockPosition();
	while (Position != NULL) {
		m_Document->GetNextBlock(Position, BlockName, Block);
		if (!Block->IsAnonymous()) {
			int ItemIndex = m_BlocksList.AddString(BlockName);
			m_BlocksList.SetItemData(ItemIndex, DWORD_PTR(Block));
		}
	}
	CBitmap Bitmap;
	Bitmap.LoadBitmap(IDB_LAYER_STATES_HC);
	m_StateImages.Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 1);
	m_StateImages.Add(&Bitmap, RGB(0, 0, 128));

	WndProcPreviewClear(m_PreviewWindowHandle);

	return TRUE;
}
void EoDlgFileManage::OnBnClickedLayerRename() {
	POSITION Position = m_LayersListControl.GetFirstSelectedItemPosition();
	if (Position != NULL) {
		int Item = m_LayersListControl.GetNextSelectedItem(Position);
		EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(Item);

		if (!Layer->Name().Compare(L"0")) {
			app.WarningMessageBox(IDS_MSG_LAYER_NO_RENAME_0);
		}
		else {
			EoDlgGetLayerName Dialog;
			Dialog.m_Name = Layer->Name();

			if (Dialog.DoModal() == IDOK) {
				LVFINDINFO ListViewFindInfo;

				ListViewFindInfo.flags = LVFI_STRING;
				ListViewFindInfo.psz = Dialog.m_Name;

				if (!Dialog.m_Name.IsEmpty() && m_LayersListControl.FindItem(&ListViewFindInfo) == - 1) {
					Layer->SetName(Dialog.m_Name);

					if (Layer->IsWork()) {
						m_Document->SetWorkLayer(Layer);
					}
					m_LayersListControl.SetItemText(Item, Name, Dialog.m_Name);
				}
			}
		}
	}
}
void EoDlgFileManage::OnBnClickedLayerMelt() {
	POSITION Position = m_LayersListControl.GetFirstSelectedItemPosition();
	if (Position != NULL) {
		int Item = m_LayersListControl.GetNextSelectedItem(Position);
		EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(Item);

		CString LayerName = Layer->Name();
		if (m_Document->LayerMelt(LayerName)) {
			m_LayersListControl.DeleteItem(Item);

			int ItemIndex = m_TracingList.AddString(Layer->Name());
			m_TracingList.SetItemData(ItemIndex, DWORD_PTR(Layer));
		}
	}
}
void EoDlgFileManage::OnBnClickedTracingOpen() {
	int CurrentSelection = m_TracingList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		if (m_TracingList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString LayerName;
			m_TracingList.GetText(CurrentSelection, LayerName);
			m_Document->TracingOpen(LayerName);
		}
	}
}
void EoDlgFileManage::OnBnClickedTracingMap() {
	int CurrentSelection = m_TracingList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		if (m_TracingList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString LayerName;
			m_TracingList.GetText(CurrentSelection, LayerName);
			m_Document->TracingMap(LayerName);
		}
	}
}
void EoDlgFileManage::OnBnClickedTracingView() {
	int CurrentSelection = m_TracingList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		if (m_TracingList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString LayerName;
			m_TracingList.GetText(CurrentSelection, LayerName);
			m_Document->TracingView(LayerName);
		}
	}
}
void EoDlgFileManage::OnBnClickedTracingCloak() {
	int CurrentSelection = m_TracingList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		if (m_TracingList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString LayerName;
			m_TracingList.GetText(CurrentSelection, LayerName);
			EoDbLayer* Layer = (EoDbLayer*) m_TracingList.GetItemData(CurrentSelection);
			if (Layer->IsOpened()) {
				app.WarningMessageBox(IDS_MSG_OPEN_TRACING_NO_CLOAK);
				m_TracingCloakRadioButton.SetCheck(0);
				m_TracingOpenRadioButton.SetCheck(1);
			}
			else {
				m_Document->UpdateAllViews(NULL, EoDb::kLayerErase, Layer);
				Layer->SetStateOff();
				Layer->SetTracingFlg(EoDbLayer::kTracingIsCloaked);
			}
		}
	}
}
void EoDlgFileManage::OnBnClickedTracingFuse() {
	int CurrentSelection = m_TracingList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		if (m_TracingList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString LayerName;
			m_TracingList.GetText(CurrentSelection, LayerName);
			EoDbLayer* Layer = (EoDbLayer*) m_TracingList.GetItemData(CurrentSelection);

			m_Document->TracingFuse(LayerName);
			m_TracingList.DeleteString(CurrentSelection);

			int ItemCount = m_LayersListControl.GetItemCount();
			m_LayersListControl.InsertItem(ItemCount, NULL);
			m_LayersListControl.SetItemData(ItemCount, DWORD_PTR(Layer));
		}
	}
}
void EoDlgFileManage::OnBnClickedTracingExclude() {
	int CurrentSelection = m_TracingList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		if (m_TracingList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString LayerName;
			m_TracingList.GetText(CurrentSelection, LayerName);
			EoDbLayer* Layer = (EoDbLayer*) m_TracingList.GetItemData(CurrentSelection);
			m_Document->UpdateAllViews(NULL, EoDb::kLayerErase, Layer);
			m_Document->RemoveLayerTableLayer(LayerName);
			m_TracingList.DeleteString(CurrentSelection);
		}
	}
}
void EoDlgFileManage::OnBnClickedTracingInclude() {
	static DWORD nFilterIndex = 1;

	CString Filter = EoAppLoadStringResource(IDS_OPENFILE_FILTER_TRACINGS);

	OPENFILENAME of;
	::ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = 0;
	of.hInstance = app.GetInstance();
	of.lpstrFilter = Filter;
	of.nFilterIndex = nFilterIndex;
	of.lpstrFile = new WCHAR[MAX_PATH];
	of.lpstrFile[0] = 0;
	of.nMaxFile = MAX_PATH;
	of.lpstrTitle = L"Include Tracing";
	of.Flags = OFN_HIDEREADONLY;
	of.lpstrDefExt = L"tra";

	if (GetOpenFileNameW(&of)) {
		nFilterIndex = of.nFilterIndex;

		CString strName = of.lpstrFile;
		CString strPath = strName.Left(of.nFileOffset);

		strName = strName.Mid(of.nFileOffset);

		if (m_Document->GetLayerTableLayer(strName) == 0) {
			if (m_Document->TracingMap(strName)) {
				EoDbLayer* pLayer = m_Document->GetLayerTableLayer(strName);
				pLayer->MakeResident();

				CString strOpenName = m_Document->GetPathName();

				if (strOpenName.Find(strPath) == - 1) {
					m_Document->TracingFuse(strName);

					strName = of.lpstrFile;
					strName = strName.Mid(of.nFileOffset, of.nFileExtension - of.nFileOffset - 1);
					pLayer->SetName(strName);
				}
				int ItemIndex = m_TracingList.AddString(strName);
				m_TracingList.SetItemData(ItemIndex, DWORD_PTR(pLayer));
			}
		}
	}
	delete [] of.lpstrFile;
}

void EoDlgFileManage::OnLbnSelchangeBlocksList() {
	int CurrentSelection = m_BlocksList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		if (m_BlocksList.GetTextLen(CurrentSelection) != LB_ERR) {
			CString BlockName;
			m_BlocksList.GetText(CurrentSelection, BlockName);

			EoDbBlock* Block = (EoDbBlock*) m_BlocksList.GetItemData(CurrentSelection);

			m_Groups.SetDlgItemInt(IDC_GROUPS, Block->GetCount(), FALSE);
			m_References.SetDlgItemInt(IDC_REFERENCES, m_Document->GetBlockReferenceCount(BlockName), FALSE);
			WndProcPreviewUpdate(m_PreviewWindowHandle, Block);
		}
	}
}
void EoDlgFileManage::OnLbnSelchangeTracingList() {
	int CurrentSelection = m_TracingList.GetCurSel();
	if (CurrentSelection != LB_ERR) {
		EoDbLayer* Layer = (EoDbLayer*) m_TracingList.GetItemData(CurrentSelection);

		m_TracingOpenRadioButton.SetCheck(Layer->IsOpened());
		m_TracingMapRadioButton.SetCheck(Layer->IsMapped());
		m_TracingViewRadioButton.SetCheck(Layer->IsViewed());
		m_TracingCloakRadioButton.SetCheck(Layer->IsOff());

		CString NumberOfGroups;
		NumberOfGroups.Format(L"%-4i", Layer->GetCount());
		m_Groups.SetWindowTextW(NumberOfGroups);

		_WndProcPreviewUpdate(m_PreviewWindowHandle, Layer);
	}
}
void EoDlgFileManage::OnBnClickedMfcbuttonWork() {
	POSITION Position = m_LayersListControl.GetFirstSelectedItemPosition();
	if (Position != NULL) {
		int Item = m_LayersListControl.GetNextSelectedItem(Position);
		EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(Item);
		EoDbLayer* PreviousWorkLayer = m_Document->SetWorkLayer(Layer);

		CString WorkLayer;
		WorkLayer.Format(L"Current Layer: %s", Layer->Name());
		GetDlgItem(IDC_STATIC_WORK_LAYER)->SetWindowTextW(WorkLayer);

		PreviousWorkLayer->MakeStateActive();

		m_Document->UpdateAllViews(NULL, EoDb::kLayerSafe, PreviousWorkLayer);
		m_Document->UpdateAllViews(NULL, EoDb::kLayerSafe, Layer);

		GetDlgItem(IDC_LAYERS_LIST_CONTROL)->RedrawWindow();
	}
}
void EoDlgFileManage::OnBnClickedMfcbuttonNew() {
	CString Name;

	int Suffix = 1;
	do {
		Name.Format(L"Layer%d", Suffix++);
	}
	while (m_Document->FindLayerTableLayer(Name) != - 1);

	EoDbLayer* Layer = new EoDbLayer(Name, EoDbLayer::kIsResident | EoDbLayer::kIsInternal | EoDbLayer::kIsActive, m_Document->ContinuousLineType());
	m_Document->AddLayerTableLayer(Layer);

	int ItemCount = m_LayersListControl.GetItemCount();
	m_LayersListControl.InsertItem(ItemCount, NULL);
	m_LayersListControl.SetItemData(ItemCount, DWORD_PTR(Layer));
}
void EoDlgFileManage::OnBnClickedMfcbuttonDel() {
	POSITION Position = m_LayersListControl.GetFirstSelectedItemPosition();
	if (Position != NULL) {
		int Item = m_LayersListControl.GetNextSelectedItem(Position);
		EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(Item);

		if (Layer->Name() == L"0") {
			app.WarningMessageBox(IDS_MSG_LAYER_NO_DELETE_0);
		}
		else if (Layer->IsWork()) {
			app.WarningMessageBox(IDS_MSG_LAYER_NO_DELETE_WORK, Layer->Name());
		}
		else {
			m_Document->UpdateAllViews(NULL, EoDb::kLayerErase, Layer);

			int LayerTableIndex = m_Document->FindLayerTableLayer(Layer->Name());
			m_Document->RemoveLayerTableLayerAt(LayerTableIndex);

			m_LayersListControl.DeleteItem(Item);
		}
	}
}
void EoDlgFileManage::DrawItem(CDC& deviceContext, int itemID, int labelIndex, const RECT& itemRectangle) {
	EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(itemID);

	switch (labelIndex) {
	case Status:
		if (Layer->IsWork()) {
			m_StateImages.Draw(&deviceContext, 8, ((CRect&) itemRectangle).TopLeft(), ILD_TRANSPARENT);
		}
		else if (Layer->IsActive()) {
			m_StateImages.Draw(&deviceContext, 9, ((CRect&) itemRectangle).TopLeft(), ILD_TRANSPARENT);
		}
		else if (Layer->IsStatic()) {
			m_StateImages.Draw(&deviceContext, 10, ((CRect&) itemRectangle).TopLeft(), ILD_TRANSPARENT);
		}
		break;
	case Name: {
			CString LayerName = Layer->Name();
			deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, LayerName, LayerName.GetLength(), NULL);
		}
		break;
	case On:
		m_StateImages.Draw(&deviceContext, Layer->IsOff() ? 3 : 2, ((CRect&) itemRectangle).TopLeft(), ILD_TRANSPARENT);
		break;
	case Freeze:
		m_StateImages.Draw(&deviceContext, /*Layer->isFrozen() ? 4 :*/ 5, ((CRect&) itemRectangle).TopLeft(), ILD_TRANSPARENT);
		break;
	case Lock:
		m_StateImages.Draw(&deviceContext, Layer->IsStatic() ? 0 : 1, ((CRect&) itemRectangle).TopLeft(), ILD_TRANSPARENT);
		break;
	case Color: {
			CRect ItemRectangle(itemRectangle);
			ItemRectangle.DeflateRect(1, 1);
			ItemRectangle.right = ItemRectangle.left + ItemRectangle.Height();

			CBrush Brush(Layer->Color());
			deviceContext.FillRect(&ItemRectangle, &Brush);

			CBrush FrameBrush;
			FrameBrush.CreateSysColorBrush(COLOR_WINDOWFRAME);
			deviceContext.FrameRect(&ItemRectangle, &FrameBrush);

			if(ItemRectangle.right + 4 < itemRectangle.right) {
				CString ColorName;
				ColorName.Format(L"%i", Layer->ColorIndex());
				deviceContext.ExtTextOutW(ItemRectangle.right + 4, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, ColorName, ColorName.GetLength(), NULL);
			}
		}
		break;
	case LineType: {
			CString LineTypeName = Layer->LineTypeName();
			deviceContext.ExtTextOutW(itemRectangle.left + 6, itemRectangle.top + 1, ETO_CLIPPED, &itemRectangle, LineTypeName, LineTypeName.GetLength(), NULL);
		}
		break;
	case LineWeight:
		//::DrawLineWeight(deviceContext, itemRectangle, Layer->lineWeight());
		break;
	}
}
void EoDlgFileManage::OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT lpDrawItemStruct) {
	if (controlIdentifier == IDC_LAYERS_LIST_CONTROL) {
		switch (lpDrawItemStruct->itemAction) {
		case ODA_DRAWENTIRE: {
				//clear item
				CRect rcItem(lpDrawItemStruct->rcItem);
				CDC DeviceContext;
				COLORREF rgbBkgnd = ::GetSysColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? COLOR_HIGHLIGHT : COLOR_WINDOW);
				DeviceContext.Attach(lpDrawItemStruct->hDC);
				CBrush br(rgbBkgnd);
				DeviceContext.FillRect(rcItem, &br);
				if(lpDrawItemStruct->itemState & ODS_FOCUS) {
					DeviceContext.DrawFocusRect(rcItem);
				}
				int itemID = lpDrawItemStruct->itemID;
				if(itemID != - 1) {
					// The text color is stored as the item data.
					COLORREF rgbText = (lpDrawItemStruct->itemState & ODS_SELECTED) ? ::GetSysColor(COLOR_HIGHLIGHTTEXT) : ::GetSysColor(COLOR_WINDOWTEXT);
					DeviceContext.SetBkColor(rgbBkgnd);
					DeviceContext.SetTextColor(rgbText);
					for (int labelIndex = 0; labelIndex < NumberOfColumns; ++labelIndex) {
						m_LayersListControl.GetSubItemRect(itemID, labelIndex, LVIR_LABEL, rcItem);
						DrawItem(DeviceContext, itemID, labelIndex, rcItem);
					}
				}
				DeviceContext.Detach();
			}
			break;

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
void EoDlgFileManage::OnNMClickLayersListControl(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	int Item = pNMItemActivate->iItem;
	int SubItem = pNMItemActivate->iSubItem;

	EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(Item);

	m_ClickToColumnName = false;
	switch(SubItem) {
	case Name:
		m_ClickToColumnName = true;
		return;
	case On:
		if (Layer->IsWork()) {
			app.WarningMessageBox(IDS_MSG_LAYER_NO_HIDDEN, Layer->Name());
		}
		else {
			if (Layer->IsOff()) {
				Layer->ClearStateFlag(EoDbLayer::kIsOff);
				m_Document->UpdateAllViews(NULL, EoDb::kLayer, Layer);
			}
			else {
				m_Document->UpdateAllViews(NULL, EoDb::kLayerErase, Layer);
				Layer->SetStateOff();
			}
		}
		break;
	case Freeze:
		//Layer->setIsFrozen(!Layer->isFrozen());
		break;
	case Lock:
		if (Layer->IsWork()) {
			app.WarningMessageBox(IDS_MSG_LAYER_NO_STATIC, Layer->Name());
		}
		else {
			if (Layer->IsStatic()) {
				Layer->ClearStateFlag(EoDbLayer::kIsStatic);
			}
			else {
				Layer->SetStateStatic();
			}
		}
		break;
	case Color: {
			EoDlgSetupColor Dialog;
			Dialog.m_ColorIndex = Layer->ColorIndex();
			if (Dialog.DoModal() == IDOK) {
				Layer->SetColorIndex(Dialog.m_ColorIndex);
			}
			break;
		}
	case LineType: {
			EoDlgSetupLineType Dialog(m_Document->LineTypeTable());
			if (Dialog.DoModal() == IDOK) {
				Layer->SetLineType(Dialog.m_LineType);
			}
			break;
		}
	case LineWeight: {
			//EoDlgLineWeight dlg(Layer->lineWeight());
			//if (IDOK == dlg.DoModal()) {
			//	Layer->setLineWeight(dlg.m_LineWeight);
			//}
			break;
		}
	}
	m_LayersListControl.Invalidate();

	*pResult = 0;
}
void EoDlgFileManage::OnNMDblclkLayersListControl(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	int Item = pNMItemActivate->iItem;
	int SubItem = pNMItemActivate->iSubItem;

	EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(Item);

	m_ClickToColumnName = false;
	switch(SubItem) {
	case Status: {
			EoDbLayer* PreviousWorkLayer = m_Document->SetWorkLayer(Layer);

			CString WorkLayer;
			WorkLayer.Format(L"Current Layer: %s", Layer->Name());
			GetDlgItem(IDC_STATIC_WORK_LAYER)->SetWindowTextW(WorkLayer);

			PreviousWorkLayer->MakeStateActive();

			m_Document->UpdateAllViews(NULL, EoDb::kLayerSafe, PreviousWorkLayer);
			m_Document->UpdateAllViews(NULL, EoDb::kLayerSafe, Layer);

			GetDlgItem(IDC_LAYERS_LIST_CONTROL)->RedrawWindow();
		}
	}
	*pResult = 0;
}
void EoDlgFileManage::OnItemchangedLayersListControl(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMLISTVIEW ListViewNotificationMessage = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	int Item = ListViewNotificationMessage->iItem;

	EoDbLayer* Layer = (EoDbLayer*) m_LayersListControl.GetItemData(Item);

	CString NumberOfGroups;
	NumberOfGroups.Format(L"%-4i", Layer->GetCount());
	m_Groups.SetWindowTextW(NumberOfGroups);

	EoDbPrimitive::SetLayerPenColorIndex(Layer->ColorIndex());
	EoDbPrimitive::SetLayerLineTypeIndex(Layer->LineTypeIndex());

	_WndProcPreviewUpdate(m_PreviewWindowHandle, Layer);

	*pResult = 0;
}
