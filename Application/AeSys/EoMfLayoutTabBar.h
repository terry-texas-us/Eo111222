#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class AeSysDoc;
class EoDbViewport;

/// @brief CMFCTabCtrl-based tab bar for switching between Model and paper-space layouts.
/// Placed at the bottom of the AeSysView client area. Tabs are populated from
/// AeSysDoc::Layouts() — "Model" tab is always present. Selecting a tab switches
/// the document's active space and layout handle, then triggers a full view refresh.
///
/// Additionally hosts state controls to the right of the tab strip:
/// - **Space state label**: Three-state clickable button:
///   - In model space: shows "MODEL". Click switches to paper space.
///   - In paper space, no viewport active: shows "PAPER". Click activates the last-used
///     viewport (or the first viewport if none was previously active).
///   - In paper space, viewport active: shows "MODEL". Click deactivates the viewport.
///   - Return to model space is only via the Model tab.
/// - **Lock button**: Toggles the viewport's display lock (prevents pan/zoom).
///   Visible only when a viewport is activated in paper space. Position 2 (after label).
/// - **Scale combo**: Dropdown showing the active viewport's scale with presets matching
///   File → Plot Scales. Visible only when a viewport is activated in paper space.
///   Shows "Custom" when the scale does not match a preset.
///
/// Inherits from CMFCTabCtrl so that EoMfVisualManager's existing tab drawing overrides
/// (OnDrawTab, OnFillTab, OnEraseTabsArea, GetTabTextColor, etc.) apply automatically —
/// no custom owner-draw painting needed.
class EoMfLayoutTabBar : public CMFCTabCtrl {
  DECLARE_DYNAMIC(EoMfLayoutTabBar)
  DECLARE_MESSAGE_MAP()

 public:
  EoMfLayoutTabBar() = default;
  ~EoMfLayoutTabBar() override;
  EoMfLayoutTabBar(const EoMfLayoutTabBar&) = delete;
  EoMfLayoutTabBar& operator=(const EoMfLayoutTabBar&) = delete;

  /// @brief Creates the tab bar with STYLE_3D, LOCATION_BOTTOM, no close button.
  /// Also creates the child state controls (space label, lock button, scale combo).
  BOOL CreateTabBar(CWnd* parentWindow, UINT controlId);

  /// @brief Populates tabs from the document's layout list.
  /// Always adds a "Model" tab at index 0. Paper-space layouts follow in tab-order.
  /// Selects the tab matching the document's current active space and layout handle.
  void PopulateFromDocument(AeSysDoc* document);

  /// @brief Returns the block record handle associated with the given tab index.
  /// Returns 0 for the Model tab (index 0) or if the index is out of range.
  [[nodiscard]] std::uint64_t BlockRecordHandleAt(int tabIndex) const noexcept;

  /// @brief Returns true if the given tab index represents the Model tab.
  [[nodiscard]] bool IsModelTab(int tabIndex) const noexcept { return tabIndex == 0; }

  /// @brief Returns the preferred height for the tab bar (tab strip only, no content area).
  [[nodiscard]] int PreferredHeight() const;

  /// @brief Returns true while PopulateFromDocument is adding/selecting tabs.
  /// Used by the parent view to suppress AFX_WM_CHANGE_ACTIVE_TAB handling during population.
  [[nodiscard]] bool IsPopulating() const noexcept { return m_populating; }

  /// @brief Updates the visibility and content of the viewport state controls.
  /// @param viewport The currently active viewport (nullptr if none).
  /// @param isPaperSpace True if the document is in paper space.
  /// Called from AeSysView when viewport state changes (activate, deactivate, tab switch).
  void UpdateViewportState(const EoDbViewport* viewport, bool isPaperSpace);

  /// @brief Refreshes the space transfer button visibility based on trap state.
  /// Called when the trap contents change (groups added or removed).
  void UpdateSpaceTransferButton();

  /// @brief Repositions the state controls to the right of the tab strip.
  /// Called from AeSysView::OnSize when the tab bar is resized.
  void RepositionControls();

  /// @brief Applies the current color scheme to the state controls.
  void ApplyColorScheme();

  /// @brief Records the last-activated viewport so it can be reactivated via the space label.
  /// Called from AeSysView::SetActiveViewportPrimitive when a viewport is activated.
  void SetLastActiveViewport(EoDbViewport* viewport) noexcept { m_lastActiveViewport = viewport; }

  /// @brief Returns the last-activated viewport for this layout (may be stale — caller must validate).
  [[nodiscard]] EoDbViewport* LastActiveViewport() const noexcept { return m_lastActiveViewport; }

  /// @brief Clears the last-active viewport tracking (e.g., on layout tab switch).
  void ClearLastActiveViewport() noexcept { m_lastActiveViewport = nullptr; }

 private:
  /// @brief Block record handles parallel to tab indices.
  /// Index 0 = Model (handle 0), index 1+ = paper-space layout block record handles.
  std::vector<std::uint64_t> m_blockRecordHandles;

  /// @brief Dummy child windows required by CMFCTabCtrl::AddTab — one per tab.
  /// CMFCTabCtrl associates each tab with a CWnd*; these invisible statics satisfy that API.
  std::vector<std::unique_ptr<CStatic>> m_dummyWindows;

  /// @brief Re-entrancy guard — true while PopulateFromDocument is executing.
  bool m_populating{false};

  // --- State controls (positioned to the right of the tab strip) ---
  // Layout order: Space Label → Lock Button → Scale Combo

  /// @brief Space state label — three-state button (MODEL / PAPER).
  /// In model space: "MODEL" (click → paper space).
  /// In paper space, no viewport: "PAPER" (click → activate viewport).
  /// In paper space, viewport active: "MODEL" (click → deactivate viewport).
  CButton m_spaceLabel;

  /// @brief Viewport display lock toggle. Shows a lock/unlock glyph.
  /// Visible only when a viewport is activated in paper space. Position 2 (after label).
  CButton m_lockButton;

  /// @brief Space transfer button. Moves trapped groups between model and paper space.
  /// Visible only when a viewport is activated in paper space and the trap is not empty.
  CButton m_spaceTransferButton;

  /// @brief Viewport scale dropdown. Populated with plot-matching scale presets.
  /// Visible only when a viewport is activated in paper space. Position 3 (after lock).
  CComboBox m_scaleCombo;

  /// @brief Font used by the state controls (matches the tab bar font size).
  CFont m_controlFont;

  /// @brief The viewport whose state is currently displayed. nullptr when no viewport is active.
  const EoDbViewport* m_currentViewport{};

  /// @brief The last viewport that was activated in this layout. Used to reactivate
  /// the same viewport when the user clicks the space label in paper space (PAPER → MODEL).
  /// May be stale if the viewport has been deleted — caller must validate.
  EoDbViewport* m_lastActiveViewport{};

  /// @brief Populates the scale combo with plot-matching presets and selects the matching entry.
  void PopulateScaleCombo(double viewportScale);

  /// @brief Returns the scale ratio (paper height / view height) for the given viewport.
  [[nodiscard]] static double ComputeViewportScale(const EoDbViewport* viewport) noexcept;

  // --- Message handlers ---
  afx_msg void OnSpaceLabelClicked();
  afx_msg void OnScaleComboChanged();
  afx_msg void OnLockButtonClicked();
  afx_msg void OnSpaceTransferClicked();
};
