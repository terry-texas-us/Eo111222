#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbPolygon.h"
#include "EoDlgActiveViewKeyplan.h"
#include "Resource.h"

HBITMAP EoDlgActiveViewKeyplan::m_hbmKeyplan = nullptr;
CRect EoDlgActiveViewKeyplan::m_rcWnd;

// EoDlgActiveViewKeyplan dialog

IMPLEMENT_DYNAMIC(EoDlgActiveViewKeyplan, CDialog)

BEGIN_MESSAGE_MAP(EoDlgActiveViewKeyplan, CDialog)
ON_BN_CLICKED(IDC_RECALL, &EoDlgActiveViewKeyplan::OnBnClickedRecall)
ON_BN_CLICKED(IDC_SAVE, &EoDlgActiveViewKeyplan::OnBnClickedSave)
ON_EN_KILLFOCUS(IDC_RATIO, &EoDlgActiveViewKeyplan::OnEnKillfocusRatio)
END_MESSAGE_MAP()

EoDlgActiveViewKeyplan::EoDlgActiveViewKeyplan(CWnd* pParent /*=nullptr*/) : CDialog(EoDlgActiveViewKeyplan::IDD, pParent), m_dRatio(0) {}
EoDlgActiveViewKeyplan::EoDlgActiveViewKeyplan(AeSysView* view, CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgActiveViewKeyplan::IDD, pParent), m_dRatio(0), m_ActiveView(view) {}
EoDlgActiveViewKeyplan::~EoDlgActiveViewKeyplan() {}
void EoDlgActiveViewKeyplan::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
  DDX_Text(dataExchange, IDC_RATIO, m_dRatio);
  DDV_MinMaxDouble(dataExchange, m_dRatio, 0.0001, 10000.0);
}
BOOL EoDlgActiveViewKeyplan::OnInitDialog() {
  CDialog::OnInitDialog();

  double ViewportWidth = m_ActiveView->WidthInInches();
  double ViewportHeight = m_ActiveView->HeightInInches();

  double UMin = -ViewportWidth * 0.5;
  double UMax = UMin + ViewportWidth;

  double VMin = -ViewportHeight * 0.5;
  double VMax = VMin + ViewportHeight;

  m_ActiveView->ModelViewAdjustWindow(UMin, VMin, UMax, VMax, 1.0);

  auto UExtent = fabs(UMax - UMin);
  auto VExtent = fabs(VMax - VMin);

  EoGePoint3d CursorPosition = app.GetCursorPosition();
  EoGeVector3d Direction = m_ActiveView->CameraDirection();
  EoGePoint3d Target = m_ActiveView->CameraTarget();
  EoGeLine::IntersectionWithPln(CursorPosition, Direction, Target, Direction, &CursorPosition);

  UMin = CursorPosition.x - (UExtent * 0.5);
  UMax = UMin + UExtent;
  VMin = CursorPosition.y - (VExtent * 0.5);
  VMax = VMin + VExtent;

  CRect KeyplanArea;
  GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&KeyplanArea);

  double UMinOverview = Target.x + m_ActiveView->OverviewUMin();
  double VMinOverview = Target.y + m_ActiveView->OverviewVMin();

  m_rcWnd.left = Eo::Round((UMin - UMinOverview) / m_ActiveView->OverviewUExt() * KeyplanArea.right);
  m_rcWnd.right = Eo::Round((UMax - UMinOverview) / m_ActiveView->OverviewUExt() * KeyplanArea.right);
  m_rcWnd.top = Eo::Round((1.0 - (VMax - VMinOverview) / m_ActiveView->OverviewVExt()) * KeyplanArea.bottom);
  m_rcWnd.bottom = Eo::Round((1.0 - (VMin - VMinOverview) / m_ActiveView->OverviewVExt()) * KeyplanArea.bottom);

  Refresh();
  return TRUE;
}
void EoDlgActiveViewKeyplan::OnOK() {
  CDialog::OnOK();

  CRect rcKey;

  GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);

  EoGePoint3d ptTarget = m_ActiveView->CameraTarget();

  double OverviewUMin = m_ActiveView->OverviewUMin();
  double UMin = m_rcWnd.left / static_cast<double>(rcKey.right) * m_ActiveView->OverviewUExt() + OverviewUMin;
  double UMax = m_rcWnd.right / static_cast<double>(rcKey.right) * m_ActiveView->OverviewUExt() + OverviewUMin;

  double OverviewVMin = m_ActiveView->OverviewVMin();
  double VMin = (-m_rcWnd.bottom / static_cast<double>(rcKey.bottom + 1)) * m_ActiveView->OverviewVExt() + OverviewVMin;
  double VMax = (-m_rcWnd.top / static_cast<double>(rcKey.bottom + 1)) * m_ActiveView->OverviewVExt() + OverviewVMin;

  double Ratio = (m_ActiveView->WidthInInches() / fabs(UMax - UMin));
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
  dcMem.CreateCompatibleDC(nullptr);
  if (m_hbmKeyplan == 0) {
    CRect rcKey;
    GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);
    CDC* DeviceContext = m_ActiveView->GetDC();
    m_hbmKeyplan = CreateCompatibleBitmap(DeviceContext->GetSafeHdc(), rcKey.right, rcKey.bottom);
  }
  dcMem.SelectObject(CBitmap::FromHandle(m_hbmKeyplan));

  BITMAP bitmap;

  ::GetObject(m_hbmKeyplan, sizeof(BITMAP), (LPTSTR)&bitmap);
  dcMem.PatBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, BLACKNESS);

  // Build a view volume which provides an overview bitmap

  m_ActiveView->ViewportPushActive();
  m_ActiveView->SetViewportSize(bitmap.bmWidth, bitmap.bmHeight);
  m_ActiveView->SetDeviceWidthInInches(static_cast<double>(dcMem.GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch);
  m_ActiveView->SetDeviceHeightInInches(static_cast<double>(dcMem.GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch);

  m_ActiveView->PushViewTransform();
  m_ActiveView->ModelViewInitialize();

  EoDbPolygon::SetSpecialPolygonStyle(EoDb::kHollow);
  AeSysDoc::GetDoc()->DisplayAllLayers(m_ActiveView, &dcMem);
  EoDbPolygon::SetSpecialPolygonStyle(-1);

  GetDlgItem(IDC_KEYPLAN_AREA)->InvalidateRect(0, TRUE);

  m_ActiveView->PopViewTransform();
  m_ActiveView->ViewportPopActive();
}
void EoDlgActiveViewKeyplan::OnEnKillfocusRatio() {
  UpdateData(TRUE);

  SendDlgItemMessageW(IDC_KEYPLAN_AREA, WM_USER_ON_NEW_RATIO, 0, (LPARAM)(LPDWORD)&m_dRatio);
}
