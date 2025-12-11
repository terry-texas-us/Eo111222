#include "stdafx.h"

#include "DlgProcFileManage.h"
#include "Preview.h"
#include "FileJob.h"

#include <afxcview.h>

IMPLEMENT_DYNAMIC(EoDlgFileManage, CDialog)

EoDlgFileManage::EoDlgFileManage(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgFileManage::IDD, pParent)
{
}
EoDlgFileManage::~EoDlgFileManage()
{
}
void EoDlgFileManage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLOR_ID, m_PenColorList);
	DDX_Control(pDX, IDC_LINETYPE, m_LineTypeList);
	DDX_Control(pDX, IDC_TRA, m_TracingList);
	DDX_Control(pDX, IDC_LAY, m_LayerList);
	DDX_Control(pDX, IDC_BLOCKS_LIST, m_BlocksList);
}
BOOL EoDlgFileManage::OnInitDialog()
{
	CDialog::OnInitDialog();

	AeSysDoc* Document = AeSysDoc::GetDoc();
	EoDbLineTypeTable* LineTypeTable = Document->LineTypeTable();

	TCHAR szBuf[MAX_PATH]; 
	GetWindowText(szBuf, MAX_PATH);
	
	CString strName = szBuf;
	strName += _T(" - ");
	strName += Document->GetPathName();
	SetWindowText(strName);
		
	m_TracingList.SetHorizontalExtent(512);
	
	for (int i = 0; i < Document->GetLayerTableSize(); i++)
	{
		CLayer* Layer = Document->GetLayerTableLayerAt(i);
		strName = Layer->GetName();

		if (Layer->IsInternal())
		{
			m_LayerList.AddString(strName);
		}
		else
		{
			m_TracingList.AddString(strName);
		}
	}
	for (int Index = 0; Index < sizeof(ColorPalette) / sizeof(COLORREF); Index++)
	{
		_itot_s(Index, szBuf, MAX_PATH, 10);
		m_PenColorList.AddString(szBuf);
	}
	LineTypeTable->FillComboBox(m_LineTypeList);
				
	m_BlocksList.SetHorizontalExtent(512);
	
	CString strKey;
	EoDbBlock* pBlock;

	POSITION pos = Document->GetFirstBlockPosition();
	while (pos != NULL)
	{
		Document->GetNextBlock(pos, strKey, pBlock);
		if (!pBlock->IsAnonymous())
		{
			m_BlocksList.AddString(strKey);
		}
	}
	//WndProcPreviewClear(dialogHandle);

	return TRUE;
}
void EoDlgFileManage::OnOK()
{
	int CurrentSelection = m_LineTypeList.GetCurSel();
	if (CurrentSelection != CB_ERR)
	{
		EoDbLineType* LineType = (EoDbLineType*) m_LineTypeList.GetItemData(CurrentSelection);

		pstate.SetLineType(NULL, LineType->Index());
	}
	CDialog::OnOK();
}
BEGIN_MESSAGE_MAP(EoDlgFileManage, CDialog)
	ON_WM_DRAWITEM()
	ON_CBN_SELCHANGE(IDC_LINETYPE, &EoDlgFileManage::OnCbnSelchangeLinetype)
	ON_CBN_SELCHANGE(IDC_COLOR_ID, &EoDlgFileManage::OnCbnSelchangeColorId)
END_MESSAGE_MAP()

void EoDlgFileManage::OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (controlIdentifier == IDC_LINETYPES)
	{
		if (lpDrawItemStruct->itemID == - 1)	// Empty combo box .. Draw only focus rectangle
		{
			OdaFocus(lpDrawItemStruct, 0);
		}
		else
		{
			switch (lpDrawItemStruct->itemAction)
			{
			case ODA_DRAWENTIRE:
				OdaDrawEntire(lpDrawItemStruct, 0);
				break;

			case ODA_SELECT:
				OdaSelect(lpDrawItemStruct, 0);				  
				break;

			case ODA_FOCUS:
				OdaFocus(lpDrawItemStruct, 0);
				break;
			}
		}
		return;
	}
	CDialog::OnDrawItem(controlIdentifier, lpDrawItemStruct);
}
/// <summary>Draw the pen style number and a sample line showing its appearance.</summary>
void EoDlgFileManage::OdaDrawEntire(LPDRAWITEMSTRUCT lpDrawItemStruct, int inflate)
{
	//int CurrentSelection = m_LineTypeList.GetCurSel();
	//if (CurrentSelection != CB_ERR)
	{
		int CurrentItemLength = m_LineTypeList.GetLBTextLen(lpDrawItemStruct->itemID);
		if (CurrentItemLength != CB_ERR)
		{
			CString Name;
			m_LineTypeList.GetLBText(lpDrawItemStruct->itemID, Name);
		
			EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();
			EoDbLineType* LineType;
			if (LineTypeTable->Lookup(Name, LineType))
			{
				CDC DeviceContext;
				DeviceContext.Attach(lpDrawItemStruct->hDC);

				CRect rc;
				::CopyRect(&rc, &lpDrawItemStruct->rcItem);

				DeviceContext.ExtTextOut(rc.right - 72, rc.top + 2, ETO_CLIPPED, &rc, Name, CurrentItemLength, 0);

				pstate.SetPen(NULL, &DeviceContext, 0, LineType->Index());

				rc.right -= 80;

				AeSysView* ActiveView = AeSysView::GetActiveView();

				ActiveView->ViewportPushActive();
				ActiveView->SetViewportSize(rc.right + rc.left, rc.bottom + rc.top);

				double UExtent = double(rc.right + rc.left) / double(DeviceContext.GetDeviceCaps(LOGPIXELSX)); 
				double VExtent = double(rc.bottom + rc.top) / double(DeviceContext.GetDeviceCaps(LOGPIXELSY)); 

				ActiveView->ModelViewPushActive();
				ActiveView->ModelViewInitialize();
				ActiveView->ModelViewSetWindow(0., 0., UExtent, VExtent);
				ActiveView->ModelViewSetTarget(EoGePoint3d::kOrigin);
				ActiveView->ModelViewSetCameraPosition(EoGeVector3d::kZAxis);

				CLine ln(EoGePoint3d(0., VExtent / 2., 0.), EoGePoint3d(UExtent, VExtent / 2., 0.));
				ln.Display(ActiveView, &DeviceContext);

				ActiveView->ModelViewPopActive();
				ActiveView->ViewportPopActive();

				OdaSelect(lpDrawItemStruct, 0);
				OdaFocus(lpDrawItemStruct, 0);

				DeviceContext.Detach();
			}
		}
	}
}
void EoDlgFileManage::OdaFocus(LPDRAWITEMSTRUCT lpDrawItemStruct, int inflate)
{
	CRect rc;
	::CopyRect(&rc, &lpDrawItemStruct->rcItem);
	::InflateRect(&rc, inflate, inflate);

	HBRUSH	hbr;
	if (lpDrawItemStruct->itemState & ODS_FOCUS)
	{
		hbr = (HBRUSH) ::GetStockObject(GRAY_BRUSH);
	}
	else
	{
		hbr = ::CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	}
	::FrameRect(lpDrawItemStruct->hDC, &rc, hbr);
	::DeleteObject(hbr);
}
void EoDlgFileManage::OdaSelect(LPDRAWITEMSTRUCT lpDrawItemStruct, int inflate)
{
	CRect rc;
	::CopyRect(&rc, &lpDrawItemStruct->rcItem);
	::InflateRect(&rc, inflate, inflate);

	HBRUSH	hbr;
	if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		hbr = (HBRUSH) ::GetStockObject(BLACK_BRUSH);
	}
	else
	{
		hbr = ::CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	}
	::FrameRect(lpDrawItemStruct->hDC, &rc, hbr);
	::DeleteObject(hbr);
}

///////////////////////////////////////////////////
void DlgProcFileManageDoLayerColor(HWND hDlg);

TCHAR szLayerName[64];

BOOL CALLBACK DlgProcFileManage(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	EoUInt16 wNotifyCode = HIWORD(wParam);

	if (message == WM_INITDIALOG)
	{
		DlgProcFileManageInit(hDlg);
		return (TRUE);
	}
	else if (message == WM_COMMAND)
	{
		AeSysDoc* Document = AeSysDoc::GetDoc();

		switch (LOWORD(wParam))
		{
		case IDC_BLOCKS_LIST:
			if (wNotifyCode == LBN_SELCHANGE)
			{
				CString strName;
				app.GetCurrentListBoxSelectionText(hDlg, IDC_BLOCKS_LIST, strName);
				EoDbBlock* pBlock;
				Document->LookupBlock(strName, pBlock);
				SetDlgItemInt(hDlg, IDC_GROUPS, (UINT) pBlock->GetCount(), FALSE);
				SetDlgItemInt(hDlg, IDC_REFERENCES, Document->GetBlockReferenceCount(strName), FALSE);
				WndProcPreviewUpdate(hDlg, pBlock);
			}
			break;

		case IDC_COLOR_ID:
			if (wNotifyCode == CBN_EDITCHANGE || wNotifyCode == CBN_SELENDOK)
				DlgProcFileManageDoLayerColor(hDlg);
			break;

		case IDC_LAY:
			if (wNotifyCode == LBN_SELCHANGE)
			{
				CString strName;
				app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName);

				CLayer* pLayer = Document->GetLayerTableLayer(strName);

				::SendDlgItemMessage(hDlg, IDC_LAYWORK, BM_SETCHECK, pLayer->IsWork(), 0L);
				::SendDlgItemMessage(hDlg, IDC_LAYACTIVE, BM_SETCHECK, pLayer->IsActive(), 0L);
				::SendDlgItemMessage(hDlg, IDC_LAYSTATIC, BM_SETCHECK, pLayer->IsStatic(), 0L);
				::SendDlgItemMessage(hDlg, IDC_LAYHIDDEN, BM_SETCHECK, pLayer->IsOff(), 0L);

				SetDlgItemInt(hDlg, IDC_GROUPS, pLayer->GetCount(), FALSE);
				SetDlgItemInt(hDlg, IDC_COLOR_ID, pLayer->PenColor(), TRUE);
				::SendMessage(::GetDlgItem(hDlg, IDC_LINETYPE), CB_SETCURSEL, pLayer->LineType(), TRUE);

				EoDbPrimitive::LayerPenColor() = pLayer->PenColor();
				EoDbPrimitive::LayerLineType() = pLayer->LineType();

				WndProcPreviewUpdate(hDlg, pLayer);
			}
			break;

		case IDC_LAYACTIVE:
			DlgProcFileManageDoLayerActive(hDlg);
			break;

		case IDC_LAYDELETE:
			DlgProcFileManageDoLayerDelete(hDlg);
			break;

		case IDC_LAYHIDDEN:
			DlgProcFileManageDoLayerHidden(hDlg);
			break;

		case IDC_LAYMELT:
			DlgProcFileManageDoLayerMelt(hDlg);
			break;

		case IDC_LAYNEW:
			szLayerName[0] = 0;
			if (::DialogBox(app.GetInstance(), MAKEINTRESOURCE(IDD_GET_LAYER_NAME), app.GetSafeHwnd(), DlgProcGetLayerName))
			{
				if (_tcslen(szLayerName) > 0)
				{
					CLayer* pLayer = new CLayer(szLayerName, EoDb::kIsResident | EoDb::kIsInternal | EoDb::kIsActive);
					Document->AddLayerTableLayer(pLayer);
					::SendDlgItemMessage(hDlg, IDC_LAY, LB_ADDSTRING, 0, (LPARAM) szLayerName);
				}
			}
			break;

		case IDC_LAYRENAME:
			DlgProcFileManageDoLayerRename(hDlg);
			break;

		case IDC_LAYSTATIC:
			DlgProcFileManageDoLayerStatic(hDlg);
			break;

		case IDC_LAYWORK:
			DlgProcFileManageDoLayerWork(hDlg);
			break;

		case IDC_LINETYPE:
			if (wNotifyCode == CBN_SELCHANGE)
			{
				CString strName;
				if (app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName) != LB_ERR)
				{
					CLayer* pLayer = Document->GetLayerTableLayer(strName);

					HWND hWndComboBox = HWND(lParam);
					LRESULT lrCurSel = ::SendMessage(hWndComboBox, CB_GETCURSEL, 0, 0L);
					if (lrCurSel != CB_ERR)
					{
						::SendMessage(hWndComboBox, CB_GETLBTEXT, (WPARAM) lrCurSel, (LPARAM) (LPCTSTR) strName);
						pLayer->SetLineTypeName(strName);
					}
				}
			}
			break;

		case IDC_SOLO:
			break;

		case IDC_TRA:
			if (wNotifyCode == LBN_SELCHANGE)
			{
				CString strName;
				app.GetCurrentListBoxSelectionText(hDlg, IDC_TRA, strName);
				CLayer* pLayer = Document->GetLayerTableLayer(strName);

				::SendDlgItemMessage(hDlg, IDC_TRAOPEN, BM_SETCHECK, pLayer->IsOpened(), 0L);
				::SendDlgItemMessage(hDlg, IDC_TRAMAP, BM_SETCHECK, pLayer->IsMapped(), 0L);
				::SendDlgItemMessage(hDlg, IDC_TRAVIEW, BM_SETCHECK, pLayer->IsViewed(), 0L);
				::SendDlgItemMessage(hDlg, IDC_TRACLOAK, BM_SETCHECK, pLayer->IsOff(), 0L);

				SetDlgItemInt(hDlg, IDC_GROUPS, pLayer->GetCount(), FALSE);
				WndProcPreviewUpdate(hDlg, pLayer);
			}
			break;

		case IDC_TRACLOAK:
			DlgProcFileManageDoTracingCloak(hDlg);
			break;

		case IDC_TRAEXCLUDE:
			DlgProcFileManageDoTracingExclude(hDlg);
			break;

		case IDC_TRAFUSE:
			DlgProcFileManageDoTracingFuse(hDlg);
			break;

		case IDC_TRAINCLUDE:
			DlgProcFileManageDoTracingInclude(hDlg);
			return (TRUE);

		case IDC_TRAMAP:
			DlgProcFileManageDoTracingMap(hDlg);
			break;

		case IDC_TRAOPEN:
			DlgProcFileManageDoTracingOpen(hDlg);
			break;

		case IDC_TRAVIEW:
			DlgProcFileManageDoTracingView(hDlg);
			break;

		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, TRUE);
			return (TRUE);
		}
	}
	return (FALSE); 		
}

	// Layer is made active.
	// This is a warm state meaning the layer is displayed using hot color set,
	// is detectable, and may have its groups modified or deleted.
	// No new groups are added to an active layer. 
	// Zero or more layers may be active.
	
void DlgProcFileManageDoLayerActive(HWND hDlg)
{	
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName) != LB_ERR)
	{
		CLayer* pLayer = Document->GetLayerTableLayer(strName);
		
		if (pLayer == 0)
		{
		}
		else
		{	
			if (pLayer->IsWork())
			{
				msgWarning(IDS_MSG_LAYER_NO_ACTIVE, strName);
				::SendDlgItemMessage(hDlg, IDC_LAYACTIVE, BM_SETCHECK, 0, 0L);
				::SendDlgItemMessage(hDlg, IDC_LAYWORK, BM_SETCHECK, 1, 0L);
			}
			else
			{
				pLayer->SetStateActive();
				Document->UpdateAllViews(NULL, EoDb::kLayerSafe, pLayer);
			}
		}
	}
}

// Selected layer is deleted. 
// Hot layer must be warm'ed before it may be deleted.
// Layer "0" may never be deleted.
void DlgProcFileManageDoLayerDelete(HWND hDlg)
{	
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	LRESULT lrCurSel = app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName);
	if (lrCurSel != LB_ERR)
	{
		int iLayerId = Document->FindLayerTableLayer(strName);
		CLayer* pLayer = Document->GetLayerTableLayerAt(iLayerId);
		
		if (iLayerId == 0)
			msgWarning(IDS_MSG_LAYER_NO_DELETE_0);
		else if (pLayer->IsWork())
			msgWarning(IDS_MSG_LAYER_NO_DELETE_WORK, strName);
		else
		{
			Document->UpdateAllViews(NULL, EoDb::kLayerErase, pLayer);
			Document->RemoveLayerTableLayerAt(iLayerId);
			::SendDlgItemMessage(hDlg, IDC_LAY, LB_DELETESTRING, (WPARAM) lrCurSel, 0L);
		}
	}
}

// Selected layer is hidden. 
void DlgProcFileManageDoLayerHidden(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName) != LB_ERR)
	{
		CLayer* pLayer = Document->GetLayerTableLayer(strName);

		if (pLayer->IsWork())
		{
			msgWarning(IDS_MSG_LAYER_NO_HIDDEN, strName);
			::SendDlgItemMessage(hDlg, IDC_LAYHIDDEN, BM_SETCHECK, 0, 0L);
			::SendDlgItemMessage(hDlg, IDC_LAYWORK, BM_SETCHECK, 1, 0L);
		}
		else
		{
			Document->UpdateAllViews(NULL, EoDb::kLayerErase, pLayer);
			pLayer->SetStateOff();
		}
	}			
}

/// <summary>Selected layer is converted to a tracing.</summary>
void DlgProcFileManageDoLayerMelt(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
	
	CString strName;
	
	LRESULT lrCurSel = app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName);
	if (lrCurSel != LB_ERR)
	{
		if (Document->LayerMelt(strName))
		{
			::SendDlgItemMessage(hDlg, IDC_LAY, LB_DELETESTRING, (WPARAM) lrCurSel, 0L);
			::SendDlgItemMessage(hDlg, IDC_TRA, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strName);
		}
	}
}

void DlgProcFileManageDoLayerRename(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	LRESULT lrCurSel = app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName);
	if (lrCurSel != LB_ERR)
	{
		int iLayerId = Document->FindLayerTableLayer(strName);
		CLayer* pLayer = Document->GetLayerTableLayerAt(iLayerId);
		
		if (iLayerId == 0)
			msgWarning(IDS_MSG_LAYER_NO_RENAME_0);
		else
		{
			_tcscpy_s(szLayerName, 64, strName);
			if (::DialogBox(app.GetInstance(), MAKEINTRESOURCE(IDD_GET_LAYER_NAME), app.GetSafeHwnd(), DlgProcGetLayerName))
			{
				if (_tcslen(szLayerName) > 0)
				{
					strName = szLayerName;
					pLayer->SetName(strName);
					
					if (pLayer->IsWork())
						Document->SetWorkLayer(pLayer);

					::SendDlgItemMessage(hDlg, IDC_LAY, LB_DELETESTRING, (WPARAM) lrCurSel, 0L);
					lrCurSel = ::SendDlgItemMessage(hDlg, IDC_LAY, LB_ADDSTRING, 0, (LPARAM) szLayerName);
					::SendDlgItemMessage(hDlg, IDC_LAY, LB_SETCURSEL, (WPARAM) lrCurSel, 0L);
				}
			}
		}	
	}
}

void DlgProcFileManageDoLayerStatic(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName) != LB_ERR)
	{
		CLayer* pLayer = Document->GetLayerTableLayer(strName);
		if (pLayer->IsWork())
		{
			msgWarning(IDS_MSG_LAYER_NO_STATIC, strName);
			::SendDlgItemMessage(hDlg, IDC_LAYSTATIC, BM_SETCHECK, 0, 0L);
			::SendDlgItemMessage(hDlg, IDC_LAYWORK, BM_SETCHECK, 1, 0L);
		}
		else
		{
			pLayer->SetStateCold();
			Document->UpdateAllViews(NULL, EoDb::kLayerSafe, pLayer);
		}
	}
}
// Selected layer is made the working layer. 
// This is a hot state meaning the layer is displayed using hot color set,
// is detectable, and may have its groups modified or deleted.
// New groups may be added to the layer.
// Only one layer is in this state at a time.
void DlgProcFileManageDoLayerWork(HWND hDlg)
{	
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName) != LB_ERR)
	{
		CLayer* pLayer = Document->GetLayerTableLayer(strName);
		
		Document->SetWorkLayer(pLayer);
		Document->UpdateAllViews(NULL, EoDb::kLayerSafe, pLayer);
	}
}

void DlgProcFileManageDoTracingCloak(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_TRA, strName) != LB_ERR)
	{
		CLayer* pLayer = Document->GetLayerTableLayer(strName);
		if (pLayer->IsOpened())
		{
			msgWarning(IDS_MSG_OPEN_TRACING_NO_CLOAK);
			::SendDlgItemMessage(hDlg, IDC_TRACLOAK, BM_SETCHECK, 0, 0L);
			::SendDlgItemMessage(hDlg, IDC_TRAOPEN, BM_SETCHECK, 1, 0L);
		}
		else
		{
			Document->UpdateAllViews(NULL, EoDb::kLayerErase, pLayer);
			pLayer->SetStateOff();
			pLayer->SetTracingFlg(EoDb::kTracingIsCloaked);
		}
	}			
}

void DlgProcFileManageDoTracingExclude(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	LRESULT lrCurSel = app.GetCurrentListBoxSelectionText(hDlg, IDC_TRA, strName);

	if (lrCurSel != LB_ERR)
	{
		CLayer* pLayer = Document->GetLayerTableLayer(strName);
		Document->UpdateAllViews(NULL, EoDb::kLayerErase, pLayer);
		Document->RemoveLayerTableLayer(strName);
		::SendDlgItemMessage(hDlg, IDC_TRA, LB_DELETESTRING, (WPARAM) lrCurSel, 0L);
	}
}

void DlgProcFileManageDoTracingFuse(HWND hDlg)
{	
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	LRESULT lrCurSel = app.GetCurrentListBoxSelectionText(hDlg, IDC_TRA, strName);

	if (lrCurSel != LB_ERR)
	{
		Document->TracingFuse(strName);
		
		::SendDlgItemMessage(hDlg, IDC_TRA, LB_DELETESTRING, (WPARAM) lrCurSel, 0L);
		::SendDlgItemMessage(hDlg, IDC_LAY, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strName);
	}
}

void DlgProcFileManageDoTracingInclude(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	static DWORD nFilterIndex = 1;

	TCHAR Filter[256];
	::LoadString(app.GetInstance(), IDS_OPENFILE_FILTER_TRACINGS, Filter, sizeof(Filter) / sizeof(TCHAR));

	OPENFILENAME of;
	::ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = 0;
	of.hInstance = app.GetInstance();
	of.lpstrFilter = Filter;
	of.nFilterIndex = nFilterIndex;
	of.lpstrFile = new TCHAR[MAX_PATH]; 
	of.lpstrFile[0] = 0;
	of.nMaxFile = MAX_PATH;
	of.lpstrTitle = _T("Include Tracing");
	of.Flags = OFN_HIDEREADONLY;
	of.lpstrDefExt = _T("tra");
	
	if (GetOpenFileName(&of))
	{
		nFilterIndex = of.nFilterIndex;
		
		CString strName = of.lpstrFile;
		CString strPath = strName.Left(of.nFileOffset);

		strName = strName.Mid(of.nFileOffset);

		if (Document->GetLayerTableLayer(strName) == 0)
		{
			if (Document->TracingMap(strName))
			{
				CLayer* pLayer = Document->GetLayerTableLayer(strName);
				pLayer->SetStateFlg(EoDb::kIsResident);
				
				CString strOpenName = Document->GetPathName();
				
				if (strOpenName.Find(strPath) == - 1)
				{
					Document->TracingFuse(strName);
					
					strName = of.lpstrFile;
					strName = strName.Mid(of.nFileOffset, of.nFileExtension - of.nFileOffset - 1);
					pLayer->SetName(strName);
				}
				::SendDlgItemMessage(hDlg, IDC_TRA, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strName);
			}
		}
	}
	delete [] of.lpstrFile;
}

// Selected tracing is mapped.
// This is a cold state meaning the tracing is displayed using warm color set,
// is not detectable, and may not have its groups modified or deleted.
// No new groups may be added to the tracing. 
// Any number of tracings may be active.
void DlgProcFileManageDoTracingMap(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_TRA, strName) != LB_ERR)
	{
		Document->TracingMap(strName);
	}	
}

void DlgProcFileManageDoTracingOpen(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_TRA, strName) != LB_ERR)
	{
		Document->TracingOpen(strName);
	}
}

void DlgProcFileManageDoTracingView(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	if (app.GetCurrentListBoxSelectionText(hDlg, IDC_TRA, strName) != LB_ERR)
	{
		Document->TracingView(strName);
	}	
}

void DlgProcFileManageInit(HWND dialogHandle)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
	EoDbLineTypeTable* LineTypeTable = Document->LineTypeTable();

	TCHAR szBuf[MAX_PATH]; 
	::GetWindowText(dialogHandle, szBuf, MAX_PATH);
	
	CString strName = szBuf;
	strName += _T(" - ");
	strName += Document->GetPathName();
	::SetWindowText(dialogHandle, strName);
		
	::SendDlgItemMessage(dialogHandle, IDC_TRA, LB_SETHORIZONTALEXTENT, 512, 0);
	
	for (int i = 0; i < Document->GetLayerTableSize(); i++)
	{
		CLayer* pLayer = Document->GetLayerTableLayerAt(i);
		strName = pLayer->GetName();
		int iCtrlId = pLayer->IsInternal() ? IDC_LAY : IDC_TRA;
		::SendDlgItemMessage(dialogHandle, iCtrlId, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strName);
	}

	for (int i = 0; i < sizeof(ColorPalette) / sizeof(COLORREF); i++)
	{
		_itot_s(i, szBuf, MAX_PATH, 10);
		::SendDlgItemMessage(dialogHandle, IDC_COLOR_ID, CB_ADDSTRING, 0, (LPARAM) szBuf);
	}
	LineTypeTable->FillComboBox(::GetDlgItem(dialogHandle, IDC_LINETYPE));
				
	::SendDlgItemMessage(dialogHandle, IDC_BLOCKS_LIST, LB_SETHORIZONTALEXTENT, 512, 0);
	
	CString strKey;
	EoDbBlock* pBlock;

	POSITION pos = Document->GetFirstBlockPosition();
	while (pos != NULL)
	{
		Document->GetNextBlock(pos, strKey, pBlock);
		if (!pBlock->IsAnonymous())
			::SendDlgItemMessage(dialogHandle, IDC_BLOCKS_LIST, LB_ADDSTRING, 0, (LPARAM) (LPCTSTR) strKey);
	}
	WndProcPreviewClear(dialogHandle);
}
void DlgProcFileManageDoLayerColor(HWND hDlg)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	CString strName;
	
	LRESULT lrCurSel = app.GetCurrentListBoxSelectionText(hDlg, IDC_LAY, strName);
	if (lrCurSel != LB_ERR)
	{
		int iLayerId = Document->FindLayerTableLayer(strName);
		CLayer* pLayer = Document->GetLayerTableLayerAt(iLayerId);
		
		EoInt16 nPenColor = (EoInt16) ::GetDlgItemInt(hDlg, IDC_COLOR_ID, 0, FALSE);
		pLayer->SetPenColor(nPenColor);
		Document->UpdateAllViews(NULL, EoDb::kLayerSafe, pLayer);

		EoDbPrimitive::LayerPenColor() = pLayer->PenColor();
		EoDbPrimitive::LayerLineType() = pLayer->LineType();

		WndProcPreviewUpdate(hDlg, pLayer);
	}
}
//////////////////////////////////////////////////////////////////

void EoDlgFileManage::OnCbnSelchangeLinetype()
{
	int CurrentSelection = m_LayerList.GetCurSel();
	if (CurrentSelection != LB_ERR)
	{
		if (m_LayerList.GetTextLen(CurrentSelection) != LB_ERR) 
		{
			CString LayerName;
			m_LayerList.GetText(CurrentSelection, LayerName);
			int CurrentSelection = m_LineTypeList.GetCurSel();
			if (CurrentSelection != CB_ERR)
			{
				EoDbLineType* LineType = (EoDbLineType*) m_LineTypeList.GetItemData(CurrentSelection);
				CLayer* Layer = AeSysDoc::GetDoc()->GetLayerTableLayer(LayerName);
				Layer->SetLineTypeName(LineType->Name());
			}
		}
	}
}
void EoDlgFileManage::OnCbnSelchangeColorId()
{
	int CurrentSelection = m_LayerList.GetCurSel();
	if (CurrentSelection != LB_ERR)
	{
		if (m_LayerList.GetTextLen(CurrentSelection) != LB_ERR) 
		{
			CString LayerName;
			m_LayerList.GetText(CurrentSelection, LayerName);
			int CurrentSelection = m_PenColorList.GetCurSel();
			if (CurrentSelection != CB_ERR)
			{
				EoInt16 PenColor = (EoInt16) m_PenColorList.GetDlgItemInt(CurrentSelection);
				CLayer* Layer = AeSysDoc::GetDoc()->GetLayerTableLayer(LayerName);
				Layer->SetPenColor(PenColor);
				
				AeSysDoc::GetDoc()->UpdateAllViews(NULL, EoDb::kLayerSafe, Layer);

				EoDbPrimitive::LayerPenColor() = Layer->PenColor();
				EoDbPrimitive::LayerLineType() = Layer->LineType();

				//WndProcPreviewUpdate(hDlg, Layer);
			}
		}
	}
}