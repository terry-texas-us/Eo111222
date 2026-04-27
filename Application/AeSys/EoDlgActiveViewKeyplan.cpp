#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbPolygon.h"
#include "EoDlgActiveViewKeyplan.h"
#include "EoGeLine.h"
#include "EoGsRenderDeviceGdi.h"
#include "Resource.h"

HBITMAP EoDlgActiveViewKeyplan::m_hbmKeyplan = nullptr;
CRect EoDlgActiveViewKeyplan::m_rcWnd;

BEGIN_MESSAGE_MAP(EoDlgActiveViewKeyplan, CDialog)
ON_BN_CLICKED(IDC_RECALL, &EoDlgActiveViewKeyplan::OnBnClickedRecall)
ON_BN_CLICKED(IDC_SAVE, &EoDlgActiveViewKeyplan::OnBnClickedSave)
ON_EN_KILLFOCUS(IDC_RATIO, &EoDlgActiveViewKeyplan::OnEnKillfocusRatio)
END_MESSAGE_MAP()

EoDlgActiveViewKeyplan::EoDlgActiveViewKeyplan(CWnd* pParent /*=nullptr*/)
    : CDialog(EoDlgActiveViewKeyplan::IDD, pParent), m_dRatio(0) {}
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

  const double viewportWidth = m_ActiveView->WidthInInches();
  const double viewportHeight = m_ActiveView->HeightInInches();

  double uMin = -viewportWidth * 0.5;
  double uMax = uMin + viewportWidth;

  double vMin = -viewportHeight * 0.5;
  double vMax = vMin + viewportHeight;

  m_ActiveView->ModelViewAdjustWindow(uMin, vMin, uMax, vMax, 1.0);

  const auto uExtent = std::abs(uMax - uMin);
  const auto vExtent = std::abs(vMax - vMin);

  auto cursorPosition = app.GetCursorPosition();
  const auto cameraDirection = m_ActiveView->CameraDirection();
  const auto target = m_ActiveView->CameraTarget();
  EoGeLine::IntersectionWithPln(cursorPosition, cameraDirection, target, cameraDirection, &cursorPosition);

  uMin = cursorPosition.x - (uExtent * 0.5);
  uMax = uMin + uExtent;
  vMin = cursorPosition.y - (vExtent * 0.5);
  vMax = vMin + vExtent;

  CRect keyplanArea;
  GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&keyplanArea);

  const double uMinOverview = target.x + m_ActiveView->OverviewUMin();
  const double vMinOverview = target.y + m_ActiveView->OverviewVMin();

  m_rcWnd.left = Eo::Round((uMin - uMinOverview) / m_ActiveView->OverviewUExt() * keyplanArea.right);
  m_rcWnd.right = Eo::Round((uMax - uMinOverview) / m_ActiveView->OverviewUExt() * keyplanArea.right);
  m_rcWnd.top = Eo::Round((1.0 - (vMax - vMinOverview) / m_ActiveView->OverviewVExt()) * keyplanArea.bottom);
  m_rcWnd.bottom = Eo::Round((1.0 - (vMin - vMinOverview) / m_ActiveView->OverviewVExt()) * keyplanArea.bottom);

  Refresh();
  return TRUE;
}

void EoDlgActiveViewKeyplan::OnOK() {
  CDialog::OnOK();

  CRect rcKey;

  GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);

  EoGePoint3d ptTarget = m_ActiveView->CameraTarget();

  const double overviewUMin = m_ActiveView->OverviewUMin();
  double uMin = m_rcWnd.left / static_cast<double>(rcKey.right) * m_ActiveView->OverviewUExt() + overviewUMin;
  double uMax = m_rcWnd.right / static_cast<double>(rcKey.right) * m_ActiveView->OverviewUExt() + overviewUMin;

  const double overviewVMin = m_ActiveView->OverviewVMin();
  double vMin = (-m_rcWnd.bottom / static_cast<double>(rcKey.bottom + 1)) * m_ActiveView->OverviewVExt() + overviewVMin;
  double vMax = (-m_rcWnd.top / static_cast<double>(rcKey.bottom + 1)) * m_ActiveView->OverviewVExt() + overviewVMin;

  const double ratio = (m_ActiveView->WidthInInches() / std::abs(uMax - uMin));
  m_ActiveView->CopyActiveModelViewToPreviousModelView();
  m_ActiveView->ModelViewAdjustWindow(uMin, vMin, uMax, vMax, ratio);
  m_ActiveView->SetViewWindow(uMin, vMin, uMax, vMax);
}

void EoDlgActiveViewKeyplan::OnBnClickedRecall() {
  // TODO: Add your control notification handler code here
}
void EoDlgActiveViewKeyplan::OnBnClickedSave() {
  // TODO: Add your control notification handler code here
}

void EoDlgActiveViewKeyplan::Refresh() {
  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);
  if (m_hbmKeyplan == nullptr) {
    CRect rcKey;
    GetDlgItem(IDC_KEYPLAN_AREA)->GetClientRect(&rcKey);
    const CDC* deviceContext = m_ActiveView->GetDC();
    m_hbmKeyplan = CreateCompatibleBitmap(deviceContext->GetSafeHdc(), rcKey.right, rcKey.bottom);
  }
  dcMem.SelectObject(CBitmap::FromHandle(m_hbmKeyplan));

  BITMAP bitmap{};

  ::GetObject(m_hbmKeyplan, sizeof(BITMAP), (LPTSTR)&bitmap);
  dcMem.PatBlt(0, 0, bitmap.bmWidth, bitmap.bmHeight, BLACKNESS);

  // Build a view volume which provides an overview bitmap

  m_ActiveView->ViewportPushActive();
  m_ActiveView->SetViewportSize(bitmap.bmWidth, bitmap.bmHeight);
  m_ActiveView->SetDeviceWidthInInches(static_cast<double>(dcMem.GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch);
  m_ActiveView->SetDeviceHeightInInches(static_cast<double>(dcMem.GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch);

  m_ActiveView->PushViewTransform();
  m_ActiveView->ModelViewInitialize();

  EoDbPolygon::SetSpecialPolygonStyle(EoDb::PolygonStyle::Hollow);
  EoGsRenderDeviceGdi renderDevice(&dcMem);
  AeSysDoc::GetDoc()->DisplayAllLayers(m_ActiveView, &renderDevice);
  EoDbPolygon::SetSpecialPolygonStyle(EoDb::PolygonStyle::Special);

  GetDlgItem(IDC_KEYPLAN_AREA)->InvalidateRect(nullptr, TRUE);

  m_ActiveView->PopViewTransform();
  m_ActiveView->ViewportPopActive();
}
void EoDlgActiveViewKeyplan::OnEnKillfocusRatio() {
  UpdateData(TRUE);

  SendDlgItemMessageW(IDC_KEYPLAN_AREA, WM_USER_ON_NEW_RATIO, 0, (LPARAM)(LPDWORD)&m_dRatio);
}
