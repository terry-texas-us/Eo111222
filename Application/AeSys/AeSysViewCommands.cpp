#include "Stdafx.h"

#include <cassert>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbBitmapFile.h"
#include "EoDlgActiveViewKeyplan.h"
#include "EoDlgSetAngle.h"
#include "EoDlgSetLength.h"
#include "EoDlgSetScale.h"
#include "EoDlgSetUnitsAndPrecision.h"
#include "EoDlgSetupConstraints.h"
#include "EoDlgSetupCustomMouseCharacters.h"
#include "EoGePoint3d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "MainFrm.h"
#include "Resource.h"

#if defined(USING_STATE_PATTERN)
#include "AeSysState.h"
#include "DrawModeState.h"
#endif

#if defined(USING_DDE)
#include "Dde.h"
#include "DdeGItms.h"
#endif

void AeSysView::OnFilePlotFull() {
  m_Plot = true;
  m_PlotScaleFactor = 1.0f;
  CView::OnFilePrint();
}

void AeSysView::OnFilePlotHalf() {
  m_Plot = true;
  m_PlotScaleFactor = 0.5f;
  CView::OnFilePrint();
}

void AeSysView::OnFilePlotQuarter() {
  m_Plot = true;
  m_PlotScaleFactor = 0.25f;
  CView::OnFilePrint();
}

void AeSysView::OnFilePrint() {
  m_Plot = false;
  m_PlotScaleFactor = 1.0f;
  CView::OnFilePrint();
}

UINT AeSysView::NumPages(CDC* deviceContext, double scaleFactor, UINT& horizontalPages, UINT& verticalPages) {
  EoGePoint3d ptMin;
  EoGePoint3d ptMax;
  EoGeTransformMatrix transformMatrix;
  auto* document = GetDocument();

  document->GetExtents(this, ptMin, ptMax, transformMatrix);

  double HorizontalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch;
  double VerticalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch;

  horizontalPages = static_cast<UINT>(Eo::Round(((ptMax.x - ptMin.x) * scaleFactor / HorizontalSizeInInches) + 0.5f));
  verticalPages = static_cast<UINT>(Eo::Round(((ptMax.y - ptMin.y) * scaleFactor / VerticalSizeInInches) + 0.5f));

  return horizontalPages * verticalPages;
}

void AeSysView::OnSetupScale() {
  EoDlgSetScale dlg;
  dlg.m_Scale = GetWorldScale();
  if (dlg.DoModal() == IDOK) { SetWorldScale(dlg.m_Scale); }
}

void AeSysView::OnViewRendered() {
  m_ViewRendered = !m_ViewRendered;
  InvalidateScene();
}

void AeSysView::OnViewTrueTypeFonts() {
  m_ViewTrueTypeFonts = !m_ViewTrueTypeFonts;
  InvalidateScene();
}

void AeSysView::OnViewPenWidths() {
  m_ViewPenWidths = !m_ViewPenWidths;
  InvalidateScene();
}

void AeSysView::OnViewSolid() {}

void AeSysView::OnViewWindow() {
  CPoint CurrentPosition;
  ::GetCursorPos(&CurrentPosition);
  HMENU WindowMenu = ::LoadMenu(AeSys::GetInstance(), MAKEINTRESOURCE(IDR_WINDOW));
  CMenu* SubMenu = CMenu::FromHandle(::GetSubMenu(WindowMenu, 0));
  SubMenu->TrackPopupMenuEx(TPM_LEFTALIGN, CurrentPosition.x, CurrentPosition.y, AfxGetMainWnd(), 0);
  ::DestroyMenu(WindowMenu);
}

void AeSysView::OnViewWireframe() {
  m_ViewWireframe = !m_ViewWireframe;
  InvalidateScene();
}

void AeSysView::OnViewDirect2D() {
  m_useD2D = !m_useD2D;
  if (m_useD2D) {
    CreateD2DRenderTarget();
    if (!m_d2dRenderTarget) {
      // Failed to create render target — fall back to GDI
      m_useD2D = false;
    }
  } else {
    DiscardD2DResources();
    // Re-create GDI back buffer for the current client area
    CRect clientRect;
    GetClientRect(&clientRect);
    RecreateBackBuffer(clientRect.Width(), clientRect.Height());
  }
  InvalidateScene();
}

void AeSysView::OnUpdateViewDirect2D(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_useD2D); }

void AeSysView::OnViewAliased() {
  m_d2dAliased = !m_d2dAliased;
  InvalidateScene();
}

void AeSysView::OnUpdateViewAliased(CCmdUI* pCmdUI) {
  pCmdUI->Enable(m_useD2D);
  pCmdUI->SetCheck(m_d2dAliased);
}

void AeSysView::OnViewWindowKeyplan() {
  EoDlgActiveViewKeyplan dlg(this);
  dlg.m_dRatio = m_Viewport.WidthInInches() / m_ViewTransform.UExtent();

  if (dlg.DoModal() == IDOK) { InvalidateScene(); }
}

void AeSysView::OnViewOdometer() {
  m_ViewOdometer = !m_ViewOdometer;
  InvalidateScene();
}

void AeSysView::OnViewRefresh() { InvalidateScene(); }

void AeSysView::OnUpdateViewRendered(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewRendered); }

void AeSysView::OnUpdateViewWireframe(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewWireframe); }

void AeSysView::OnSetupDimLength() {
  EoDlgSetLength dialog;

  dialog.SetTitle(L"Set Dimension Length");
  dialog.SetLength(app.DimensionLength());
  if (dialog.DoModal() == IDOK) {
    app.SetDimensionLength(dialog.Length());
    UpdateStateInformation(DimLen);
#if defined(USING_DDE)
    dde::PostAdvise(dde::DimLenInfo);
#endif
  }
}

void AeSysView::OnSetupDimAngle() {
  EoDlgSetAngle dlg;

  dlg.m_strTitle = L"Set Dimension Angle";
  dlg.m_dAngle = app.DimensionAngle();
  if (dlg.DoModal() == IDOK) {
    app.SetDimensionAngle(dlg.m_dAngle);
    UpdateStateInformation(DimAng);
#if defined(USING_DDE)
    dde::PostAdvise(dde::DimAngZInfo);
#endif
  }
}

void AeSysView::OnSetupUnits() {
  EoDlgSetUnitsAndPrecision Dialog;
  Dialog.m_Units = app.GetUnits();
  Dialog.m_Precision = app.GetArchitecturalUnitsFractionPrecision();

  if (Dialog.DoModal() == IDOK) {
    app.SetUnits(Dialog.m_Units);
    app.SetArchitecturalUnitsFractionPrecision(Dialog.m_Precision);
  }
}

void AeSysView::OnSetupConstraints() {
  EoDlgSetupConstraints Dialog(this);

  if (Dialog.DoModal() == IDOK) { UpdateStateInformation(All); }
}

void AeSysView::OnSetupMouseButtons() {
  EoDlgSetupCustomMouseCharacters dialog;
  if (dialog.DoModal() == IDOK) {}
}

void AeSysView::OnRelativeMovesEngDown() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngDownRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngIn() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z -= app.EngagedLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngLeft() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngLeftRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngOut() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z += app.EngagedLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngRight() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngRightRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngUp() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngUpRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesRight() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.x += app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesUp() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.y += app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesLeft() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.x -= app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesDown() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.y -= app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesIn() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z -= app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesOut() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z += app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesRightRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x += app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesUpRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y += app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesLeftRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x -= app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesDownRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y -= app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnHelpKey() { ::WinHelpW(GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"READY")); }

/** @brief Retrieves the active view in the MDI application.
 * @note This function assumes that the main window is a CMDIFrameWndEx and that the active child window is a
 * CMDIChildWndEx containing an AeSysView.
 * @return A pointer to the active AeSysView, or nullptr if no active view is found.
 */
AeSysView* AeSysView::GetActiveView() {
  auto* frameWindow = dynamic_cast<CMDIFrameWndEx*>(AfxGetMainWnd());
  if (frameWindow == nullptr) {
    // Legitimately nullptr during CMainFrame::OnCreate() - not an error
    return nullptr;
  }
  auto* childWindow = dynamic_cast<CMDIChildWndEx*>(frameWindow->MDIGetActive());
#ifdef _DEBUG
  assert(childWindow != nullptr &&
         "No active MDI child window - should always have an active document after initialization");
#endif
  if (childWindow == nullptr) { return nullptr; }

  auto* view = childWindow->GetActiveView();
#ifdef _DEBUG
  assert(view != nullptr && "Active MDI child has no view - this indicates a framework error");
#endif
  if (view == nullptr) { return nullptr; }

  auto* activeView = dynamic_cast<AeSysView*>(view);
#ifdef _DEBUG
  assert(activeView != nullptr &&
         "Active view is not an AeSysView (possible splitter windows or multi-view configuration");
#endif
  return activeView;
}

void AeSysView::OnUpdateViewOdometer(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewOdometer); }

void AeSysView::OnUpdateViewTrueTypeFonts(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewTrueTypeFonts); }

void AeSysView::OnBackgroundImageLoad() {
  CFileDialog dlg(TRUE, L"bmp", L"*.bmp");
  dlg.m_ofn.lpstrTitle = L"Load Background Image";

  if (dlg.DoModal() == IDOK) {
    EoDbBitmapFile BitmapFile(dlg.GetPathName());

    BitmapFile.Load(dlg.GetPathName(), m_backgroundImageBitmap, m_backgroundImagePalette);
    m_viewBackgroundImage = true;
    InvalidateScene();
  }
}

void AeSysView::OnBackgroundImageRemove() {
  if (static_cast<HBITMAP>(m_backgroundImageBitmap) != 0) {
    m_backgroundImageBitmap.DeleteObject();
    m_backgroundImagePalette.DeleteObject();
    m_viewBackgroundImage = false;

    InvalidateScene();
  }
}

void AeSysView::OnViewBackgroundImage() {
  m_viewBackgroundImage = !m_viewBackgroundImage;
  InvalidateScene();
}

void AeSysView::OnUpdateViewBackgroundImage(CCmdUI* pCmdUI) {
  pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) != 0);
  pCmdUI->SetCheck(m_viewBackgroundImage);
}

void AeSysView::OnUpdateBackgroundimageLoad(CCmdUI* pCmdUI) {
  pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) == 0);
}

void AeSysView::OnUpdateBackgroundimageRemove(CCmdUI* pCmdUI) {
  pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) != 0);
}

void AeSysView::OnUpdateViewPenwidths(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewPenWidths); }

void AeSysView::OnOp0() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
    case ID_MODE_GROUP_EDIT:
      OnEditModeOptions();
      break;
  }
}

void AeSysView::OnOp2() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP2);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP2);
      break;
  }
}

void AeSysView::OnOp3() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP3);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP3);
      break;
  }
}

void AeSysView::OnOp4() {
  auto* document = GetDocument();
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      document->InitializeGroupAndPrimitiveEdit();
      break;

    case ID_MODE_GROUP_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      document->InitializeGroupAndPrimitiveEdit();
      break;
  }
}

void AeSysView::OnOp5() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveCopy();
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupCopy();
      break;
  }
}

void AeSysView::OnOp6() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP6);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP6);
      break;
  }
}

void AeSysView::OnOp7() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP7);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP7);
      break;
  }
}

void AeSysView::OnOp8() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP8);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP8);
      break;
  }
}

void AeSysView::OnReturn() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      InitializeGroupAndPrimitiveEdit();
      break;

    case ID_MODE_GROUP_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      InitializeGroupAndPrimitiveEdit();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      MendPrimitiveReturn();
      break;
  }
}

void AeSysView::OnEscape() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveEscape();
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupEscape();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      MendPrimitiveEscape();
      break;
  }
}

/** @brief Handler for the Find command, which retrieves the text from the Find combo box in the main frame and logs it.
 * @note Currently, this function only verifies the combo box text and logs it, but it is intended to be expanded with
 * the actual Find command implementation in the future.
 */
void AeSysView::OnFind() {
  constexpr auto mainFrameErrorMsg = L"Main frame should exist when Find command is triggered";

  // Get the main frame window
  auto* mainFrame = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
#ifdef _DEBUG
  assert(mainFrame != nullptr && mainFrameErrorMsg);
#endif
  if (mainFrame == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"%ls\n", static_cast<const wchar_t*>(mainFrameErrorMsg));
    return;
  }
  // Get the find combo box control
  constexpr auto findComboErrorMsg = L"Find combo should exist in toolbar";

  auto* findComboBox = mainFrame->GetFindCombo();
#ifdef _DEBUG
  assert(findComboBox != nullptr && findComboErrorMsg);
#endif
  if (findComboBox == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"%ls\n", static_cast<const wchar_t*>(findComboErrorMsg));
    return;
  }

  CString findComboBoxText;
  VerifyFindString(findComboBox, findComboBoxText);

  if (findComboBoxText.IsEmpty()) { return; }

  // @todo Find command implementation should go here, currently just verifying the combo box text and logging it
  ATLTRACE2(
      traceGeneral, 1, L"Verifying the FindComboBox text and logging: `%ls`\n", static_cast<LPCWSTR>(findComboBoxText));
}

/** @brief Verifies the text in the Find combo box and updates it if necessary.
 * @param findComboBox The combo box button to verify.
 * @param findComboBoxText A reference to a CString that will be updated with the current text of the combo box if the
 * last command was from the button.
 * @note This function checks if the last command was triggered by the provided combo box button. If it was, it
 * retrieves the current text from the combo box. It then checks if this text already exists in the combo box's list of
 * items. If it does, it removes it and re-inserts it at the top of the list to ensure that the most recently used
 * search term is easily accessible. Finally, if the last command was not from the button, it sets the combo box text to
 * the verified text.
 */
void AeSysView::VerifyFindString(CMFCToolBarComboBoxButton* findComboBox, CString& findComboBoxText) {
  if (findComboBox == nullptr) { return; }
  BOOL isLastCommandFromButton = CMFCToolBar::IsLastCommandFromButton(findComboBox);

  if (isLastCommandFromButton) { findComboBoxText = findComboBox->GetText(); }
  auto* ComboBox = findComboBox->GetComboBox();

  if (!findComboBoxText.IsEmpty()) {
    const int count = ComboBox->GetCount();
    int Position{};

    while (Position < count) {
      CString LBText;
      ComboBox->GetLBText(Position, LBText);

      if (LBText.GetLength() == findComboBoxText.GetLength()) {
        if (LBText == findComboBoxText) { break; }
      }
      Position++;
    }
    // Text need to move to initial position
    if (Position < count) { ComboBox->DeleteString(static_cast<UINT>(Position)); }
    ComboBox->InsertString(0, findComboBoxText);
    ComboBox->SetCurSel(0);

    if (!isLastCommandFromButton) { findComboBox->SetText(findComboBoxText); }
  }
}

void AeSysView::OnEditFind() { ATLTRACE2(traceGeneral, 1, L"AeSysView::OnEditFind() - Entering\n"); }

void AeSysView::SetWorldScale(double scale) {
  if (scale > Eo::geometricTolerance) {
    m_WorldScale = scale;
    UpdateStateInformation(Scale);

    CMainFrame* MainFrame = (CMainFrame*)(AfxGetMainWnd());
    MainFrame->GetPropertiesPane().GetActiveViewScaleProperty().SetValue(m_WorldScale);

#if defined(USING_DDE)
    dde::PostAdvise(dde::ScaleInfo);
#endif
  }
}

void AeSysView::OnViewStateInformation() {
  m_ViewStateInformation = !m_ViewStateInformation;
  AeSysDoc::GetDoc()->UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysView::OnUpdateViewStateinformation(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewStateInformation); }

// Mode switching commands (moved from AeSys.cpp for proper command routing)

void AeSysView::OnModeAnnotate() {
  app.SetModeResourceIdentifier(IDR_ANNOTATE_MODE);
  app.SetPrimaryMode(ID_MODE_ANNOTATE);
  app.LoadModeResources(ID_MODE_ANNOTATE);
}

void AeSysView::OnModeCut() {
  app.SetModeResourceIdentifier(IDR_CUT_MODE);
  app.SetPrimaryMode(ID_MODE_CUT);
  app.LoadModeResources(ID_MODE_CUT);
}

void AeSysView::OnModeDimension() {
  app.SetModeResourceIdentifier(IDR_DIMENSION_MODE);
  app.SetPrimaryMode(ID_MODE_DIMENSION);
  app.LoadModeResources(ID_MODE_DIMENSION);
}

void AeSysView::OnModeDraw() {
  app.SetModeResourceIdentifier(IDR_DRAW_MODE);
  app.SetPrimaryMode(ID_MODE_DRAW);
  app.LoadModeResources(ID_MODE_DRAW, this);
#if defined(USING_STATE_PATTERN)
  PushState(std::make_unique<DrawModeState>());
#endif
}

void AeSysView::OnModeDraw2() {
  app.SetModeResourceIdentifier(IDR_DRAW2_MODE);
  app.SetPrimaryMode(ID_MODE_DRAW2);
  app.LoadModeResources(ID_MODE_DRAW2);
}

void AeSysView::OnModeEdit() {
  app.SetModeResourceIdentifier(IDR_EDIT_MODE);
  app.LoadModeResources(ID_MODE_EDIT);
}

void AeSysView::OnModeFixup() {
  app.SetModeResourceIdentifier(IDR_FIXUP_MODE);
  app.LoadModeResources(ID_MODE_FIXUP);
}

void AeSysView::OnModeLPD() {
  app.SetModeResourceIdentifier(IDR_LPD_MODE);
  app.LoadModeResources(ID_MODE_LPD);
}

void AeSysView::OnModeNodal() {
  app.SetModeResourceIdentifier(IDR_NODAL_MODE);
  app.LoadModeResources(ID_MODE_NODAL);
}

void AeSysView::OnModePipe() {
  app.SetModeResourceIdentifier(IDR_PIPE_MODE);
  app.LoadModeResources(ID_MODE_PIPE);
}

void AeSysView::OnModePower() {
  app.SetModeResourceIdentifier(IDR_POWER_MODE);
  app.LoadModeResources(ID_MODE_POWER);
}

void AeSysView::OnModeTrap() {
  if (app.TrapModeAddGroups()) {
    app.SetModeResourceIdentifier(IDR_TRAP_MODE);
    app.LoadModeResources(ID_MODE_TRAP);
  } else {
    app.SetModeResourceIdentifier(IDR_TRAPR_MODE);
    app.LoadModeResources(ID_MODE_TRAPR);
  }
}

void AeSysView::OnTrapCommandsAddGroups() {
  app.SetModeAddGroups(!app.TrapModeAddGroups());
  app.LoadModeResources(app.TrapModeAddGroups() ? ID_MODE_TRAP : ID_MODE_TRAPR);
  OnModeTrap();
}

// Update UI handlers
void AeSysView::OnUpdateModeAnnotate(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_ANNOTATE); }

void AeSysView::OnUpdateModeCut(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_CUT); }

void AeSysView::OnUpdateModeDimension(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_DIMENSION); }

void AeSysView::OnUpdateModeDraw(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_DRAW); }

void AeSysView::OnUpdateModeDraw2(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_DRAW2); }

void AeSysView::OnUpdateModeEdit(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_EDIT); }

void AeSysView::OnUpdateModeFixup(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_FIXUP); }

void AeSysView::OnUpdateModeLpd(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_LPD); }

void AeSysView::OnUpdateModeNodal(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_NODAL); }

void AeSysView::OnUpdateModePipe(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_PIPE); }

void AeSysView::OnUpdateModePower(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_POWER); }

void AeSysView::OnUpdateModeTrap(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_TRAP); }
