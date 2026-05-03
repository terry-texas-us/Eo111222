#pragma once

#include "EoGePoint3d.h"

/// @brief Near-cursor dynamic input tooltip (AeSys dynamic input, Level 2).
///
///   When no anchor is available (first point):
///     Row 1: context prompt  (e.g. "Specify center point")
///     Row 2: X nnn.nnnn      Y nnn.nnnn
///
///   When anchor is available (subsequent points):
///     Row 1: context prompt  (e.g. "Specify point on circle")
///     Row 2: nnn.nnnn  >  nnn.nnn\xb0
///
/// Never shown when idle.  Status-bar odometer fills that role.
class EoDynInputTooltip : public CWnd {
 public:
  EoDynInputTooltip() = default;
  ~EoDynInputTooltip() override = default;
  EoDynInputTooltip(const EoDynInputTooltip&) = delete;
  EoDynInputTooltip& operator=(const EoDynInputTooltip&) = delete;

  /// @brief Create the layered popup.  Call once from AeSysView::OnCreate.
  BOOL Create(CWnd* ownerView);

  /// @brief Show the tooltip near the cursor.
  /// @param prompt   Context string from the active state (e.g. "Specify next vertex").
  ///                 If null or empty, "Specify point" is used as a fallback.
  /// @param anchor   If equal to EoGePoint3d{} (no anchor yet), X/Y absolute coords are shown;
  ///                 otherwise distance and angle are shown using AeSys ">" convention.
  void Show(CPoint cursorScreen, const EoGePoint3d& worldPos,
            const EoGePoint3d& anchorWorld, const wchar_t* prompt);

  /// @brief Hide the tooltip (call when gesture ends or mode exits).
  void Hide();

  [[nodiscard]] bool IsVisible() const noexcept { return m_visible; }

 protected:
  afx_msg void OnPaint();
  DECLARE_MESSAGE_MAP()

 private:
  CString m_promptRow;  ///< context prompt string
  CString m_valueRow;   ///< "X nnn  Y nnn" or "dist  >  angle\xb0"
  bool m_visible{false};

  void PositionAndShow(CPoint cursorScreen);
  void EnsureFont();

  CFont m_font;
  // Equal horizontal and vertical cursor offset for the tooltip UL corner.
  static constexpr int kOffsetX{14};
  static constexpr int kOffsetY{14};
  static constexpr int kPadX{7};
  static constexpr int kPadY{3};
};
