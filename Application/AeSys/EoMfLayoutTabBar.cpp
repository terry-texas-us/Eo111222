#include "Stdafx.h"

#include <algorithm>
#include <cmath>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbViewport.h"
#include "EoDxfBase.h"
#include "EoMfLayoutTabBar.h"
#include "Resource.h"

IMPLEMENT_DYNAMIC(EoMfLayoutTabBar, CMFCTabCtrl)

BEGIN_MESSAGE_MAP(EoMfLayoutTabBar, CMFCTabCtrl)
ON_BN_CLICKED(IDC_LAYOUT_SPACE_LABEL, &EoMfLayoutTabBar::OnSpaceLabelClicked)
ON_BN_CLICKED(IDC_VIEWPORT_LOCK_BUTTON, &EoMfLayoutTabBar::OnLockButtonClicked)
ON_CBN_SELCHANGE(IDC_VIEWPORT_SCALE_COMBO, &EoMfLayoutTabBar::OnScaleComboChanged)
END_MESSAGE_MAP()

// Viewport scale presets matching File → Plot Scales (excluding "Fit to paper").
// Scale value = paperUnits / drawingUnits = paper height / view height.
static constexpr struct {
  const wchar_t* label;
  double scale;
} scalePresets[] = {
    {L"1:1", 1.0 / 1.0},
    {L"1:2", 1.0 / 2.0},
    {L"1:4", 1.0 / 4.0},
    {L"1:5", 1.0 / 5.0},
    {L"1:8", 1.0 / 8.0},
    {L"1:10", 1.0 / 10.0},
    {L"1:16", 1.0 / 16.0},
    {L"1:20", 1.0 / 20.0},
    {L"1:30", 1.0 / 30.0},
    {L"1:40", 1.0 / 40.0},
    {L"1:50", 1.0 / 50.0},
    {L"1:100", 1.0 / 100.0},
    {L"1/8\" = 1'-0\"", 0.125 / 12.0},
    {L"3/16\" = 1'-0\"", 0.1875 / 12.0},
    {L"1/4\" = 1'-0\"", 0.25 / 12.0},
    {L"3/8\" = 1'-0\"", 0.375 / 12.0},
    {L"1/2\" = 1'-0\"", 0.5 / 12.0},
    {L"3/4\" = 1'-0\"", 0.75 / 12.0},
    {L"1\" = 1'-0\"", 1.0 / 12.0},
    {L"1-1/2\" = 1'-0\"", 1.5 / 12.0},
    {L"3\" = 1'-0\"", 3.0 / 12.0},
};

static constexpr int scalePresetCount = static_cast<int>(std::size(scalePresets));

EoMfLayoutTabBar::~EoMfLayoutTabBar() {
  // Destroy dummy windows before CMFCTabCtrl destructor accesses them
  m_dummyWindows.clear();
}

BOOL EoMfLayoutTabBar::CreateTabBar(CWnd* parentWindow, UINT controlId) {
  CRect rect(0, 0, 0, 0);
  if (!Create(CMFCTabCtrl::STYLE_3D, rect, parentWindow, controlId, CMFCTabCtrl::LOCATION_BOTTOM, FALSE)) {
    return FALSE;
  }
  EnableTabSwap(FALSE);
  m_nTabBorderSize = 0;

  // Create a font matching the tab bar's default font size
  NONCLIENTMETRICSW ncm{};
  ncm.cbSize = sizeof(ncm);
  SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
  m_controlFont.CreateFontIndirectW(&ncm.lfMessageFont);

  // MODEL/PAPER toggle button — flat push-button style, always visible
  m_spaceLabel.Create(L"MODEL", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, CRect(0, 0, 0, 0), this,
      IDC_LAYOUT_SPACE_LABEL);
  m_spaceLabel.SetFont(&m_controlFont);

  // Scale combo — hidden until a viewport is activated
  m_scaleCombo.Create(WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL, CRect(0, 0, 0, 0), this,
      IDC_VIEWPORT_SCALE_COMBO);
  m_scaleCombo.SetFont(&m_controlFont);

  // Lock button — hidden until a viewport is activated
  m_lockButton.Create(
      L"\U0001F513", WS_CHILD | BS_PUSHBUTTON | BS_FLAT, CRect(0, 0, 0, 0), this, IDC_VIEWPORT_LOCK_BUTTON);
  m_lockButton.SetFont(&m_controlFont);

  ApplyColorScheme();

  return TRUE;
}

void EoMfLayoutTabBar::PopulateFromDocument(AeSysDoc* document) {
  m_populating = true;

  RemoveAllTabs();
  m_dummyWindows.clear();
  m_blockRecordHandles.clear();

  if (document == nullptr) {
    m_populating = false;
    return;
  }

  auto addTab = [this](const wchar_t* label, std::uint64_t blockRecordHandle) {
    auto dummy = std::make_unique<CStatic>();
    dummy->Create(L"", WS_CHILD, CRect(0, 0, 0, 0), this);
    AddTab(dummy.get(), label, static_cast<UINT>(-1), FALSE);
    m_dummyWindows.push_back(std::move(dummy));
    m_blockRecordHandles.push_back(blockRecordHandle);
  };

  // Always add a "Model" tab at index 0
  addTab(L"Model", 0);

  // Collect paper-space layouts sorted by tab order
  auto& layouts = document->Layouts();
  std::vector<const EoDxfLayout*> paperLayouts;
  for (const auto& layout : layouts) {
    if (!layout.IsModelLayout()) { paperLayouts.push_back(&layout); }
  }
  std::sort(paperLayouts.begin(), paperLayouts.end(),
      [](const EoDxfLayout* a, const EoDxfLayout* b) { return a->m_tabOrder < b->m_tabOrder; });

  for (const auto* layout : paperLayouts) {
    addTab(layout->m_layoutName.c_str(), layout->m_blockRecordHandle);
  }

  // Select the tab matching the current active space and layout handle
  int selectedIndex = 0;
  if (document->ActiveSpace() != EoDxf::Space::ModelSpace) {
    auto activeHandle = document->ActiveLayoutHandle();
    for (int i = 1; i < static_cast<int>(m_blockRecordHandles.size()); ++i) {
      if (m_blockRecordHandles[i] == activeHandle) {
        selectedIndex = i;
        break;
      }
    }
  }
  SetActiveTab(selectedIndex);

  // Update the space label to reflect the current active space
  bool isPaperSpace = document->ActiveSpace() == EoDxf::Space::PaperSpace;
  m_spaceLabel.SetWindowTextW(isPaperSpace ? L"PAPER" : L"MODEL");

  m_populating = false;
}

std::uint64_t EoMfLayoutTabBar::BlockRecordHandleAt(int tabIndex) const noexcept {
  if (tabIndex < 0 || tabIndex >= static_cast<int>(m_blockRecordHandles.size())) { return 0; }
  return m_blockRecordHandles[tabIndex];
}

int EoMfLayoutTabBar::PreferredHeight() const {
  int tabsHeight = GetTabsHeight();
  return (tabsHeight > 0) ? tabsHeight : 24;
}

void EoMfLayoutTabBar::UpdateViewportState(const EoDbViewport* viewport, bool isPaperSpace) {
  m_currentViewport = viewport;

  // Three-state label:
  //   Model space: "MODEL"
  //   Paper space, viewport active: "MODEL" (click deactivates viewport)
  //   Paper space, no viewport: "PAPER" (click activates viewport)
  if (!isPaperSpace) {
    m_spaceLabel.SetWindowTextW(L"MODEL");
  } else if (viewport != nullptr) {
    m_spaceLabel.SetWindowTextW(L"MODEL");
  } else {
    m_spaceLabel.SetWindowTextW(L"PAPER");
  }

  if (viewport != nullptr && isPaperSpace) {
    // Show lock button and scale combo when a viewport is active in paper space
    m_lockButton.SetWindowTextW(viewport->IsDisplayLocked() ? L"\U0001F512" : L"\U0001F513");
    m_lockButton.ShowWindow(SW_SHOW);

    double viewportScale = ComputeViewportScale(viewport);
    PopulateScaleCombo(viewportScale);
    m_scaleCombo.ShowWindow(SW_SHOW);
  } else {
    // Hide viewport-specific controls when in model space or no viewport is active
    m_lockButton.ShowWindow(SW_HIDE);
    m_scaleCombo.ShowWindow(SW_HIDE);
  }

  RepositionControls();
}

void EoMfLayoutTabBar::RepositionControls() {
  if (m_spaceLabel.GetSafeHwnd() == nullptr) { return; }

  CRect clientRect;
  GetClientRect(&clientRect);

  const int barHeight = PreferredHeight();
  const int controlHeight = barHeight - 4;  // 2px top + 2px bottom margin
  const int topMargin = 2;
  const int rightMargin = 4;
  const int controlGap = 4;

  // Measure the space label text width
  CClientDC dc(&m_spaceLabel);
  CFont* oldFont = dc.SelectObject(&m_controlFont);
  CSize spaceLabelSize;
  CString spaceLabelText;
  m_spaceLabel.GetWindowTextW(spaceLabelText);
  spaceLabelSize = dc.GetTextExtent(spaceLabelText);
  dc.SelectObject(oldFont);

  const int spaceLabelWidth = spaceLabelSize.cx + 16;  // padding

  // Layout from the right edge: Space Label → Lock → Scale
  int xPosition = clientRect.right - rightMargin;

  // Scale combo (rightmost, if visible) — wide enough for ~18 characters
  if (m_scaleCombo.IsWindowVisible()) {
    const auto dpi = static_cast<int>(GetDpiForWindow(GetSafeHwnd()));
    const int scaleComboWidth = MulDiv(140, dpi, 96);
    xPosition -= scaleComboWidth;
    m_scaleCombo.MoveWindow(xPosition, topMargin, scaleComboWidth, controlHeight + 200);  // drop height
    xPosition -= controlGap;
  }

  // Lock button (middle, if visible)
  if (m_lockButton.IsWindowVisible()) {
    const int lockButtonWidth = controlHeight;  // square
    xPosition -= lockButtonWidth;
    m_lockButton.MoveWindow(xPosition, topMargin, lockButtonWidth, controlHeight);
    xPosition -= controlGap;
  }

  // Space label (always visible, just left of the viewport controls)
  xPosition -= spaceLabelWidth;
  m_spaceLabel.MoveWindow(xPosition, topMargin, spaceLabelWidth, controlHeight);
}

void EoMfLayoutTabBar::ApplyColorScheme() {
  // The tab bar background and tab drawing are handled by EoMfVisualManager.
  // Child controls use the default system colors which work well with the light chrome.
  // No explicit theming needed for the flat buttons and combo — they inherit from
  // the parent window's visual style.
}

void EoMfLayoutTabBar::PopulateScaleCombo(double viewportScale) {
  m_scaleCombo.ResetContent();

  int selectedIndex = -1;
  for (int i = 0; i < scalePresetCount; ++i) {
    m_scaleCombo.AddString(scalePresets[i].label);
    // Check if this preset matches the current viewport scale (within 0.1% tolerance)
    if (std::abs(viewportScale - scalePresets[i].scale) < scalePresets[i].scale * 0.001) { selectedIndex = i; }
  }

  if (selectedIndex < 0) {
    // Current scale doesn't match any preset — add a "Custom" entry
    CString customLabel;
    customLabel.Format(L"Custom (1:%.2f)", 1.0 / viewportScale);
    int customIndex = m_scaleCombo.AddString(customLabel);
    m_scaleCombo.SetCurSel(customIndex);
  } else {
    m_scaleCombo.SetCurSel(selectedIndex);
  }
}

double EoMfLayoutTabBar::ComputeViewportScale(const EoDbViewport* viewport) noexcept {
  if (viewport == nullptr || viewport->ViewHeight() < 1e-10 || viewport->Height() < 1e-10) { return 1.0; }
  // Scale = paper-space viewport height / model-space view height
  return viewport->Height() / viewport->ViewHeight();
}

// --- Message handlers ---

void EoMfLayoutTabBar::OnSpaceLabelClicked() {
  auto* parentView = static_cast<AeSysView*>(GetParent());
  if (parentView == nullptr) { return; }
  auto* document = parentView->GetDocument();
  if (document == nullptr) { return; }

  if (document->ActiveSpace() == EoDxf::Space::ModelSpace) {
    // --- Model space: switch to paper space ---
    document->OnViewModelSpace();

    // Sync the tab selection to the active layout
    bool isPaperSpace = document->ActiveSpace() == EoDxf::Space::PaperSpace;
    if (isPaperSpace) {
      auto activeHandle = document->ActiveLayoutHandle();
      for (int i = 1; i < static_cast<int>(m_blockRecordHandles.size()); ++i) {
        if (m_blockRecordHandles[i] == activeHandle) {
          m_populating = true;
          SetActiveTab(i);
          m_populating = false;
          break;
        }
      }
      // If no matching layout tab found and there's at least one paper-space tab, select it
      if (m_blockRecordHandles.size() > 1 && GetActiveTab() == 0) {
        m_populating = true;
        SetActiveTab(1);
        m_populating = false;
      }
    }

    UpdateViewportState(nullptr, isPaperSpace);
  } else if (parentView->IsViewportActive()) {
    // --- Paper space, viewport active: deactivate the viewport ---
    parentView->DeactivateViewport();
  } else {
    // --- Paper space, no viewport active: activate the last-used or first viewport ---
    EoDbViewport* viewportToActivate = m_lastActiveViewport;

    // Validate the last-active viewport is still reachable
    if (viewportToActivate != nullptr) {
      auto* found = document->HitTestViewport(viewportToActivate->CenterPoint());
      if (found != viewportToActivate) { viewportToActivate = nullptr; }
    }

    // Fall back to the first viewport in the layout
    if (viewportToActivate == nullptr) { viewportToActivate = document->FindFirstViewport(); }

    if (viewportToActivate != nullptr) { parentView->SetActiveViewportPrimitive(viewportToActivate); }
  }

  // Refresh mode cursor — paper space is always white background
  parentView->SetModeCursor(app.CurrentMode());

  // Return focus to the view
  parentView->SetFocus();
}

void EoMfLayoutTabBar::OnScaleComboChanged() {
  if (m_currentViewport == nullptr) { return; }

  int selectedIndex = m_scaleCombo.GetCurSel();
  if (selectedIndex < 0 || selectedIndex >= scalePresetCount) { return; }  // "Custom" entry — ignore

  double targetScale = scalePresets[selectedIndex].scale;
  double paperHeight = m_currentViewport->Height();
  if (paperHeight < 1e-10) { return; }

  // Compute the new viewHeight that achieves the target scale
  double newViewHeight = paperHeight / targetScale;

  // Apply to the viewport (const_cast is safe — the viewport is owned by the document,
  // and we hold a non-owning pointer for display purposes)
  auto* viewport = const_cast<EoDbViewport*>(m_currentViewport);
  viewport->SetViewHeight(newViewHeight);

  // Refresh the view
  auto* parentView = static_cast<AeSysView*>(GetParent());
  if (parentView != nullptr) {
    parentView->InvalidateScene();
    parentView->SetFocus();
  }
}

void EoMfLayoutTabBar::OnLockButtonClicked() {
  if (m_currentViewport == nullptr) { return; }

  auto* viewport = const_cast<EoDbViewport*>(m_currentViewport);
  bool newLocked = !viewport->IsDisplayLocked();
  viewport->SetDisplayLocked(newLocked);

  m_lockButton.SetWindowTextW(newLocked ? L"\U0001F512" : L"\U0001F513");

  // Return focus to the view
  auto* parentView = static_cast<AeSysView*>(GetParent());
  if (parentView != nullptr) { parentView->SetFocus(); }
}
