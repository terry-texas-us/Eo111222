#include "stdafx.h"

#include "DlgViewZoom.h"

HBITMAP		CDlgViewZoom::m_hbmKeyplan = NULL;
CRect		CDlgViewZoom::m_rcWnd;
bool		CDlgViewZoom::bKeyplan = false;

// CDlgViewZoom dialog

IMPLEMENT_DYNAMIC(CDlgViewZoom, CDialog)

CDlgViewZoom::CDlgViewZoom(CWnd* pParent /*=NULL*/) : 
	CDialog(CDlgViewZoom::IDD, pParent), m_dRatio(0)
{
}
CDlgViewZoom::~CDlgViewZoom()
{
}
void CDlgViewZoom::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_RATIO, m_dRatio);
	DDV_MinMaxDouble(pDX, m_dRatio, .00000001, 10000000.);
}
BOOL CDlgViewZoom::OnInitDialog()
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	
	CDialog::OnInitDialog();
	
	double ViewportWidth = ActiveView->WidthInInches();
	double ViewportHeight = ActiveView->HeightInInches(); 
	
	double UMin = - ViewportWidth * .5; 
	double UMax = UMin + ViewportWidth;

	double VMin = - ViewportHeight * .5; 
	double VMax = VMin + ViewportHeight;

	ActiveView->ModelViewAdjustWindow(UMin, VMin, UMax, VMax, 1.);
	
	double UExtent = fabs(UMax - UMin);
	double VExtent = fabs(VMax - VMin);

	EoGePoint3d CursorPosition = app.GetCursorPosition();
	EoGeVector3d Direction = ActiveView->ModelViewGetDirection();
	EoGePoint3d Target = ActiveView->ModelViewGetTarget();
	CLine::IntersectionWithPln(CursorPosition, Direction, Target, Direction, &CursorPosition);
				
	UMin = CursorPosition.x - (UExtent * .5); 
	UMax = UMin + UExtent;
	VMin = CursorPosition.y - (VExtent * .5); 
	VMax = VMin + VExtent;
			
	CRect KeyplanArea;
	GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&KeyplanArea);
	
	double UMinOverview = Target.x + ActiveView->OverGetUMin();
	double VMinOverview = Target.y + ActiveView->OverGetVMin();

	m_rcWnd.left =   Round((UMin - UMinOverview) / ActiveView->OverGetUExt() * KeyplanArea.right);
	m_rcWnd.right =  Round((UMax - UMinOverview) / ActiveView->OverGetUExt() * KeyplanArea.right);
	m_rcWnd.top =    Round((1. - (VMax - VMinOverview) / ActiveView->OverGetVExt()) * KeyplanArea.bottom);
	m_rcWnd.bottom = Round((1. - (VMin - VMinOverview) / ActiveView->OverGetVExt()) * KeyplanArea.bottom);
			
	CRect MoreArea;
	GetWindowRect(&MoreArea);
			
	TCHAR szBuf[16];
	
	_stprintf_s(szBuf, 16, _T("%05u %05u"), MoreArea.Width(), MoreArea.Height());
	
	GetDlgItem(IDC_LESS_AREA)->SetWindowText(szBuf);
	GetDlgItem(IDC_KEYPLAN_AREA)->EnableWindow(bKeyplan);	
		
	Refresh();
	return TRUE;
}
void CDlgViewZoom::OnOK()
{
	CDialog::OnOK();

	AeSysView* ActiveView = AeSysView::GetActiveView();

	if (bKeyplan)
	{
		CRect rcKey;
	
		GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);
						
		EoGePoint3d ptTarget = ActiveView->ModelViewGetTarget();
		double dUMinOverview = ActiveView->OverGetUMin();
		double dVMinOverview = ActiveView->OverGetVMin();

		double UMin = m_rcWnd.left / double(rcKey.right) * ActiveView->OverGetUExt() + dUMinOverview;
		double UMax = m_rcWnd.right / double(rcKey.right) * ActiveView->OverGetUExt() + dUMinOverview;
		
		double VMin = (- m_rcWnd.bottom / double(rcKey.bottom + 1)) * ActiveView->OverGetVExt() + dVMinOverview;
		double VMax = (- m_rcWnd.top / double(rcKey.bottom + 1)) * ActiveView->OverGetVExt() + dVMinOverview;
		
		double dRatio = ActiveView->WidthInInches() / fabs(UMax - UMin);
		ActiveView->CopyActiveModelViewToPreviousModelView();
		ActiveView->ModelViewAdjustWindow(UMin, VMin, UMax, VMax, dRatio);
		ActiveView->ModelViewSetWindow(UMin, VMin, UMax, VMax);
	}
	else
	{	
		ActiveView->CopyActiveModelViewToPreviousModelView();
		ActiveView->DoWindowPan(m_dRatio);
	}
}
BEGIN_MESSAGE_MAP(CDlgViewZoom, CDialog)
	ON_BN_CLICKED(IDC_BEST, OnBnClickedBest)
	ON_BN_CLICKED(IDC_LAST, OnBnClickedLast)
	ON_BN_CLICKED(IDC_MORELESS, OnBnClickedMoreless)
	ON_BN_CLICKED(IDC_NORMAL, OnBnClickedNormal)
	ON_BN_CLICKED(IDC_OVERVIEW, OnBnClickedOverview)
	ON_BN_CLICKED(IDC_PAN, OnBnClickedPan)
	ON_EN_CHANGE(IDC_RATIO, OnEnChangeRatio)
	ON_BN_CLICKED(IDC_RECALL, OnBnClickedRecall)
	ON_BN_CLICKED(IDC_SAVE, OnBnClickedSave)
END_MESSAGE_MAP()

// CDlgViewZoom message handlers

void CDlgViewZoom::OnBnClickedBest()
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
	AeSysView* ActiveView = AeSysView::GetActiveView();

	EoGePoint3d ptMin;
	EoGePoint3d ptMax;
	
	Document->GetExtents(ptMin, ptMax, ActiveView->ModelViewGetMatrix());
	
	// extents return range - 1 to 1

	if (ptMin.x < ptMax.x)
	{
		ActiveView->CopyActiveModelViewToPreviousModelView();
		
		double UExtent = ActiveView->ModelViewGetUExt() * (ptMax.x - ptMin.x) / 2.;
		double VExtent = ActiveView->ModelViewGetVExt() * (ptMax.y - ptMin.y) / 2.;
		
		ActiveView->ModelViewSetCenteredWnd(UExtent, VExtent);
		
		CTMat tm; 
		Document->GetExtents(ptMin, ptMax, tm);
		
		EoGePoint3d ptTarget = EoGePoint3d((ptMin.x + ptMax.x) / 2., (ptMin.y + ptMax.y) / 2., (ptMin.z + ptMax.z) / 2.);
		
		ActiveView->ModelViewSetTarget(ptTarget);
		ActiveView->ModelViewSetCameraPosition(ActiveView->ModelViewGetDirection());
		
		ActiveView->SetCursorPosition(ptTarget);
	}
	EndDialog(IDOK);
}

void CDlgViewZoom::OnBnClickedLast()
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	ActiveView->ExchangeActiveAndPreviousModelViews();
	EndDialog(IDOK);
}

// Enables/disables the keyplan.  Enabling the keyplan causes the dialog to grow.
// Info indicating size and position are buried in a control.
void CDlgViewZoom::OnBnClickedMoreless()
{
	TCHAR szBuf[16];
	GetDlgItem(IDC_LESS_AREA)->GetWindowText(szBuf, sizeof(szBuf) / sizeof(TCHAR));
	
	if (bKeyplan)
	{
		CRect rcDef;
		
		GetDlgItem(IDC_LESS_AREA)->GetWindowRect(&rcDef);
		SetWindowPos(0, 0, 0, _tstoi(szBuf), rcDef.Height(), SWP_NOZORDER | SWP_NOMOVE);
	}
	else
	{	
		SetWindowPos(0, 0, 0, _tstoi(szBuf), _tstoi(szBuf + 6), SWP_NOZORDER | SWP_NOMOVE);
	}
	bKeyplan = !bKeyplan;
	GetDlgItem(IDC_KEYPLAN_AREA)->EnableWindow(bKeyplan);

	Refresh();
}

void CDlgViewZoom::OnBnClickedNormal()
{
	AeSysView* ActiveView = AeSysView::GetActiveView();

	ActiveView->CopyActiveModelViewToPreviousModelView();
	ActiveView->DoWindowPan(1.);
	EndDialog(IDOK);
}

void CDlgViewZoom::OnBnClickedOverview()
{
	AeSysView* ActiveView = AeSysView::GetActiveView();
	ActiveView->ModelViewInitialize();
	EndDialog(IDOK);
}

void CDlgViewZoom::OnBnClickedPan()
{
	AeSysView* ActiveView = AeSysView::GetActiveView();

	ActiveView->CopyActiveModelViewToPreviousModelView();
	ActiveView->DoWindowPan(m_dRatio);
	EndDialog(IDOK);
}
void CDlgViewZoom::OnBnClickedRecall()
{
	// TODO: Add your control notification handler code here
}
void CDlgViewZoom::OnBnClickedSave()
{
	// TODO: Add your control notification handler code here
}

void CDlgViewZoom::Refresh()
{
	AeSysDoc* Document = AeSysDoc::GetDoc();
	AeSysView* ActiveView = AeSysView::GetActiveView();
	CDC* DeviceContext = (ActiveView == NULL) ? NULL : ActiveView->GetDC();

	TCHAR szBuf[16];
	GetDlgItem(IDC_LESS_AREA)->GetWindowText(szBuf, sizeof(szBuf) / sizeof(TCHAR));
	
	if (bKeyplan)
	{
		GetDlgItem(IDC_MORELESS)->SetWindowText(_T("&Less"));
	
		CDC dcMem;
		dcMem.CreateCompatibleDC(NULL);
		if (m_hbmKeyplan == 0)
		{
			CRect rcKey;
			GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);
			m_hbmKeyplan = CreateCompatibleBitmap(DeviceContext->GetSafeHdc(), rcKey.right, rcKey.bottom);
		}
		dcMem.SelectObject(CBitmap::FromHandle(m_hbmKeyplan));
	
		BITMAP bitmap;
		
		::GetObject(m_hbmKeyplan, sizeof(BITMAP), (LPTSTR) &bitmap);
		dcMem.PatBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, BLACKNESS);
	
		// Build a view volume which provides an overview bitmap

		ActiveView->ViewportPushActive();
		ActiveView->SetViewportSize(bitmap.bmWidth, bitmap.bmHeight);
		ActiveView->SetDeviceWidthInInches(double(dcMem.GetDeviceCaps(HORZSIZE)) / EoMmPerInch);
		ActiveView->SetDeviceHeightInInches(double(dcMem.GetDeviceCaps(VERTSIZE)) / EoMmPerInch);
		
		ActiveView->ModelViewPushActive();
		ActiveView->ModelViewInitialize();
												
		EoDbPrimitive::SpecPolygonStyle() = EoDb::Hollow;
		Document->DisplayAllLayers(ActiveView, &dcMem);
		EoDbPrimitive::SpecPolygonStyle() = - 1;

		GetDlgItem(IDC_KEYPLAN_AREA)->InvalidateRect(0, TRUE);
		
		ActiveView->ModelViewPopActive();
		ActiveView->ViewportPopActive();
	}
	else
	{			
		GetDlgItem(IDC_MORELESS)->SetWindowText(_T("&More"));
		
		CRect rcLessArea;
		GetDlgItem(IDC_LESS_AREA)->GetWindowRect(&rcLessArea);
		SetWindowPos(0, 0, 0, _tstoi(szBuf), rcLessArea.Height(), SWP_NOZORDER | SWP_NOMOVE);
	}
}
void CDlgViewZoom::OnEnChangeRatio()
{
	if (GetFocus() == GetDlgItem(IDC_RATIO))
	{
		TCHAR szBuf[32];

		GetDlgItemText(IDC_RATIO, szBuf, sizeof(szBuf) / sizeof(TCHAR));
		double dRatio = _tstof(szBuf);
		
		if (dRatio >= DBL_EPSILON)
			SendDlgItemMessage(IDC_KEYPLAN_AREA, WM_USER_ON_NEW_RATIO, 0, (LPARAM) (LPDWORD) &dRatio);
	}
}
