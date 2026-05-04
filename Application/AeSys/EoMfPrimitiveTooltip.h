#pragma once

/// @brief Owner-drawn popup tooltip for primitive hover metadata.
///
/// Displays Name;Value pairs (from EoDbPrimitive::FormatExtra + FormatGeometry) as a
/// styled popup window with bold label names and normal-weight values.  Colors and
/// fonts follow the chromeColors / pane palette so it integrates with the existing
/// light warm chrome theme.
///
/// Usage:
///   EoMfPrimitiveTooltip m_tooltip;          // member in AeSysView
///   m_tooltip.Create(this);                   // OnCreate
///   m_tooltip.Show(text, screenPoint);        // OnTimer — pass the raw "Name;Value\t..." string
///   m_tooltip.Hide();                         // OnMouseMove (cursor moved)
class EoMfPrimitiveTooltip : public CWnd {
 public:
  EoMfPrimitiveTooltip() = default;
  ~EoMfPrimitiveTooltip() override = default;

  EoMfPrimitiveTooltip(const EoMfPrimitiveTooltip&) = delete;
  EoMfPrimitiveTooltip& operator=(const EoMfPrimitiveTooltip&) = delete;

  /// @brief Creates the hidden popup window.  Call once from the owner's OnCreate.
  /// @param parent  Owner window — the popup is owned by this window.
  /// @return TRUE on success.
  BOOL Create(CWnd* parent);

  /// @brief Shows the popup at the given screen coordinates with the provided content.
  /// @param rawExtra  The "Name;Value\tName;Value\t..." string from FormatExtra + FormatGeometry.
  /// @param screenPos Screen pixel position (cursor tip) — the window is offset slightly.
  void Show(const CString& rawExtra, CPoint screenPos);

  /// @brief Hides the popup window immediately.
  void Hide();

  [[nodiscard]] bool IsVisible() const noexcept { return m_visible; }

 protected:
  afx_msg void OnPaint();
  DECLARE_MESSAGE_MAP()

 private:
  struct Row {
    CString label;  ///< text before the ';'
    CString value;  ///< text after the ';'
    bool isTitle{};  ///< true for the first row (the Type field) — rendered with accent color
  };

  /// @brief Parses "Name;Value\t..." into m_rows.
  void ParseRows(const CString& rawExtra);

  /// @brief Measures and repositions the window so all content fits, then shows it.
  void LayoutAndShow(CPoint screenPos);

  static constexpr int kPadX{10};   ///< Horizontal inner padding (px)
  static constexpr int kPadY{8};    ///< Vertical inner padding (px)
  static constexpr int kRowGap{3};  ///< Extra gap between rows (px)
  static constexpr int kTitleGap{5};///< Extra gap below title row (px)

  std::vector<Row> m_rows;
  CFont m_boldFont;    ///< Bold font for label column
  CFont m_normalFont;  ///< Normal font for value column
  bool m_fontsCreated{false};
  bool m_visible{false};

  void EnsureFonts(CDC& dc);
};
