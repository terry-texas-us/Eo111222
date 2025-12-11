#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDlgActiveViewKeyplan.h"

HBITMAP EoDlgActiveViewKeyplan::m_hbmKeyplan = NULL;
CRect EoDlgActiveViewKeyplan::m_rcWnd;

// EoDlgActiveViewKeyplan dialog

IMPLEMENT_DYNAMIC(EoDlgActiveViewKeyplan, CDialog)

BEGIN_MESSAGE_MAP(EoDlgActiveViewKeyplan, CDialog)
	ON_BN_CLICKED(IDC_RECALL, &EoDlgActiveViewKeyplan::OnBnClickedRecall)
	ON_BN_CLICKED(IDC_SAVE, &EoDlgActiveViewKeyplan::OnBnClickedSave)
	ON_EN_KILLFOCUS(IDC_RATIO, &EoDlgActiveViewKeyplan::OnEnKillfocusRatio)
END_MESSAGE_MAP()

EoDlgActiveViewKeyplan::EoDlgActiveViewKeyplan(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgActiveViewKeyplan::IDD, pParent), m_dRatio(0) {
}
EoDlgActiveViewKeyplan::EoDlgActiveViewKeyplan(AeSysView* view, CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgActiveViewKeyplan::IDD, pParent), m_ActiveView(view), m_dRatio(0) {
}
EoDlgActiveViewKeyplan::~EoDlgActiveViewKeyplan() {
}
void EoDlgActiveViewKeyplan::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_RATIO, m_dRatio);
	DDV_MinMaxDouble(pDX, m_dRatio, .0001, 10000.);
}
BOOL EoDlgActiveViewKeyplan::OnInitDialog() {
	CDialog::OnInitDialog();

	float ViewportWidth = m_ActiveView->WidthInInches();
	float ViewportHeight = m_ActiveView->HeightInInches();

	float UMin = - ViewportWidth * 0.5f;
	float UMax = UMin + ViewportWidth;

	float VMin = - ViewportHeight * 0.5f;
	float VMax = VMin + ViewportHeight;

	m_ActiveView->ModelViewAdjustWindow(UMin, VMin, UMax, VMax, 1.0f);

	auto UExtent = static_cast<float>(fabs(UMax - UMin));
	auto VExtent = static_cast<float>(fabs(VMax - VMin));

	EoGePoint3d CursorPosition = app.GetCursorPosition();
	EoGeVector3d Direction = m_ActiveView->CameraDirection();
	EoGePoint3d Target = m_ActiveView->CameraTarget();
	EoGeLine::IntersectionWithPln(CursorPosition, Direction, Target, Direction, &CursorPosition);

	UMin = static_cast<float>(CursorPosition.x) - (UExtent * 0.5f);
	UMax = UMin + UExtent;
	VMin = static_cast<float>(CursorPosition.y) - (VExtent * 0.5f);
	VMax = VMin + VExtent;

	CRect KeyplanArea;
	GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&KeyplanArea);

	float UMinOverview = static_cast<float>(Target.x) + m_ActiveView->OverviewUMin();
	float VMinOverview = static_cast<float>(Target.y) + m_ActiveView->OverviewVMin();

	m_rcWnd.left = EoRound((UMin - UMinOverview) / m_ActiveView->OverviewUExt() * KeyplanArea.right);
	m_rcWnd.right = EoRound((UMax - UMinOverview) / m_ActiveView->OverviewUExt() * KeyplanArea.right);
	m_rcWnd.top = EoRound((1.0f - (VMax - VMinOverview) / m_ActiveView->OverviewVExt()) * KeyplanArea.bottom);
	m_rcWnd.bottom = EoRound((1.0f - (VMin - VMinOverview) / m_ActiveView->OverviewVExt()) * KeyplanArea.bottom);

	Refresh();
	return TRUE;
}
void EoDlgActiveViewKeyplan::OnOK() {
	CDialog::OnOK();

	CRect rcKey;

	GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);

	EoGePoint3d ptTarget = m_ActiveView->CameraTarget();

	float OverviewUMin = m_ActiveView->OverviewUMin();
	float UMin = m_rcWnd.left / static_cast<float>(rcKey.right) * m_ActiveView->OverviewUExt() + OverviewUMin;
	float UMax = m_rcWnd.right / static_cast<float>(rcKey.right) * m_ActiveView->OverviewUExt() + OverviewUMin;

	float OverviewVMin = m_ActiveView->OverviewVMin();
	float VMin = (- m_rcWnd.bottom / static_cast<float>(rcKey.bottom + 1)) * m_ActiveView->OverviewVExt() + OverviewVMin;
	float VMax = (- m_rcWnd.top / static_cast<float>(rcKey.bottom + 1)) * m_ActiveView->OverviewVExt() + OverviewVMin;

	float Ratio = static_cast<float>(m_ActiveView->WidthInInches() / fabs(UMax - UMin));
	m_ActiveView->CopyActiveModelViewToPreviousModelView();
	m_ActiveView->ModelViewAdjustWindow(UMin, VMin, UMax, VMax, Ratio);
	m_ActiveView->SetViewWindow(UMin, VMin, UMax, VMax);
}
// EoDlgActiveViewKeyplan message handlers


void EoDlgActiveViewKeyplan::OnBnClickedRecall() {
	// TODO: Add your control notification handler code here
}
void EoDlgActiveViewKeyplan::OnBnClickedSave() {
	// TODO: Add your control notification handler code here
}

void EoDlgActiveViewKeyplan::Refresh() {
	CDC dcMem;
	dcMem.CreateCompatibleDC(NULL);
	if (m_hbmKeyplan == 0) {
		CRect rcKey;
		GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);
		CDC* DeviceContext = m_ActiveView->GetDC();
		m_hbmKeyplan = CreateCompatibleBitmap(DeviceContext->GetSafeHdc(), rcKey.right, rcKey.bottom);
	}
	dcMem.SelectObject(CBitmap::FromHandle(m_hbmKeyplan));

	BITMAP bitmap;

	::GetObject(m_hbmKeyplan, sizeof(BITMAP), (LPTSTR) &bitmap);
	dcMem.PatBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, BLACKNESS);

	// Build a view volume which provides an overview bitmap

	m_ActiveView->ViewportPushActive();
	m_ActiveView->SetViewportSize(bitmap.bmWidth, bitmap.bmHeight);
	m_ActiveView->SetDeviceWidthInInches(static_cast<float>(dcMem.GetDeviceCaps(HORZSIZE)) / EoMmPerInch);
	m_ActiveView->SetDeviceHeightInInches(static_cast<float>(dcMem.GetDeviceCaps(VERTSIZE)) / EoMmPerInch);

	m_ActiveView->PushViewTransform();
	m_ActiveView->ModelViewInitialize();

	EoDbPolygon::SetSpecialPolygonStyle(EoDb::kHollow);
	AeSysDoc::GetDoc()->DisplayAllLayers(m_ActiveView, &dcMem);
	EoDbPolygon::SetSpecialPolygonStyle(- 1);

	GetDlgItem(IDC_KEYPLAN_AREA)->InvalidateRect(0, TRUE);

	m_ActiveView->PopViewTransform();
	m_ActiveView->ViewportPopActive();

}
void EoDlgActiveViewKeyplan::OnEnKillfocusRatio() {
	UpdateData(TRUE);

	SendDlgItemMessageW(IDC_KEYPLAN_AREA, WM_USER_ON_NEW_RATIO, 0, (LPARAM) (LPDWORD) &m_dRatio);
}
