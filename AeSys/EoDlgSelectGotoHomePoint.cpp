#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

#include "EoDlgSelectGotoHomePoint.h"

// EoDlgSelectGotoHomePoint dialog

IMPLEMENT_DYNAMIC(EoDlgSelectGotoHomePoint, CDialog)

BEGIN_MESSAGE_MAP(EoDlgSelectGotoHomePoint, CDialog)
	ON_CBN_EDITUPDATE(IDC_LIST, &EoDlgSelectGotoHomePoint::OnCbnEditupdateList)
	ON_CBN_SELCHANGE(IDC_LIST, &EoDlgSelectGotoHomePoint::OnCbnSelchangeList)
END_MESSAGE_MAP()

EoDlgSelectGotoHomePoint::EoDlgSelectGotoHomePoint(CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgSelectGotoHomePoint::IDD, pParent) {
}
EoDlgSelectGotoHomePoint::EoDlgSelectGotoHomePoint(AeSysView* activeView, CWnd* pParent /*=NULL*/) :
	CDialog(EoDlgSelectGotoHomePoint::IDD, pParent), m_ActiveView(activeView) {
}
EoDlgSelectGotoHomePoint::~EoDlgSelectGotoHomePoint() {
}
void EoDlgSelectGotoHomePoint::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_HomePointNames);
	DDX_Control(pDX, IDC_X, m_X);
	DDX_Control(pDX, IDC_Y, m_Y);
	DDX_Control(pDX, IDC_Z, m_Z);
}
BOOL EoDlgSelectGotoHomePoint::OnInitDialog() {
	CDialog::OnInitDialog();

	CString Names = EoAppLoadStringResource(IDS_HOME_POINT_GO_NAMES);
	m_HomePointNames.ResetContent();
	int Position = 0;
	while (Position < Names.GetLength()) {
		CString NamesItem = Names.Tokenize(L"\n", Position);
		m_HomePointNames.AddString(NamesItem);
	}
	m_HomePointNames.SetCurSel(9);

	EoGePoint3d Origin = m_ActiveView->GridOrign();

	CString Length;

	app.FormatLength(Length, max(app.GetUnits(), AeSys::kEngineering), Origin.x, 16, 8);
	SetDlgItemTextW(IDC_X, Length);
	app.FormatLength(Length, max(app.GetUnits(), AeSys::kEngineering), Origin.y, 16, 8);
	SetDlgItemTextW(IDC_Y, Length);
	app.FormatLength(Length, max(app.GetUnits(), AeSys::kEngineering), Origin.z, 16, 8);
	SetDlgItemTextW(IDC_Z, Length);

	return TRUE;
}
void EoDlgSelectGotoHomePoint::OnOK() {
	CDialog::OnOK();
}
void EoDlgSelectGotoHomePoint::OnCbnEditupdateList() {
	CString NamesItem;
	m_HomePointNames.GetWindowTextW(NamesItem);

	int NamesItemIndex = m_HomePointNames.FindString(- 1, NamesItem);

	if (NamesItemIndex != CB_ERR) {
		switch (NamesItemIndex) {
		case 9:
			m_ActiveView->SetCursorPosition(m_ActiveView->GridOrign());
			break;
		case 10:
			m_ActiveView->SetCursorPosition(AeSysDoc::GetDoc()->GetTrapPivotPoint());
			break;
		case 11:
			m_ActiveView->SetCursorPosition(m_ActiveView->CameraTarget());
			break;
		case 12:
			m_ActiveView->SetCursorPosition(EoGePoint3d::kOrigin);
			break;
		default:
			m_ActiveView->SetCursorPosition(app.HomePointGet(NamesItemIndex));
		}
		CDialog::OnOK();
	}
}
void EoDlgSelectGotoHomePoint::OnCbnSelchangeList() {
	int NamesItemIndex = m_HomePointNames.GetCurSel();

	if (NamesItemIndex != CB_ERR) {
		EoGePoint3d Point;
		switch (NamesItemIndex) {
		case 9:
			Point = m_ActiveView->GridOrign();
			break;
		case 10:
			Point = AeSysDoc::GetDoc()->GetTrapPivotPoint();
			break;
		case 11:
			Point = m_ActiveView->CameraTarget();
			break;
		case 12:
			Point = EoGePoint3d::kOrigin;
			break;
		default:
			Point = app.HomePointGet(NamesItemIndex);
		}
		CString Length;
		app.FormatLength(Length, max(app.GetUnits(), AeSys::kEngineering), Point.x, 16, 8);
		SetDlgItemTextW(IDC_X, Length);
		app.FormatLength(Length, max(app.GetUnits(), AeSys::kEngineering), Point.y, 16, 8);
		SetDlgItemTextW(IDC_Y, Length);
		app.FormatLength(Length, max(app.GetUnits(), AeSys::kEngineering), Point.z, 16, 8);
		SetDlgItemTextW(IDC_Z, Length);
	}
}
