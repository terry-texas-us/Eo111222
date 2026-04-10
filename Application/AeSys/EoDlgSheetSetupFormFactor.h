#pragma once

#include "Resource.h"

/// @brief Modal dialog for selecting paper-space sheet size and orientation.
///
/// Presents ARCH A–E sheet designations in a dropdown and landscape/portrait
/// radio buttons. On OK the selected size (in inches, landscape or portrait)
/// is available via SheetWidth() and SheetHeight().
class EoDlgSheetSetupFormFactor : public CDialog {
  DECLARE_DYNAMIC(EoDlgSheetSetupFormFactor)

 public:
  explicit EoDlgSheetSetupFormFactor(CWnd* parent = nullptr);

  enum { IDD = IDD_SHEET_SETUP_FORM_FACTOR };

  /// @brief Sheet width in inches after user confirmation.
  [[nodiscard]] double SheetWidth() const noexcept { return m_sheetWidth; }

  /// @brief Sheet height in inches after user confirmation.
  [[nodiscard]] double SheetHeight() const noexcept { return m_sheetHeight; }

  /// @brief Sets the initial sheet designation index (0=A … 4=E).
  void SetInitialDesignation(int index) noexcept { m_designationIndex = index; }

  /// @brief Sets the initial orientation (true = landscape).
  void SetInitialLandscape(bool landscape) noexcept { m_isLandscape = landscape; }

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

  DECLARE_MESSAGE_MAP()

 private:
  /// ARCH sheet sizes in landscape orientation (width × height in inches).
  struct SheetSize {
    const wchar_t* label;
    double width;   ///< Landscape width
    double height;  ///< Landscape height
  };

  static constexpr SheetSize kSheetSizes[] = {
      {L"ARCH A  (12\" \x00D7 9\")", 12.0, 9.0},
      {L"ARCH B  (18\" \x00D7 12\")", 18.0, 12.0},
      {L"ARCH C  (24\" \x00D7 18\")", 24.0, 18.0},
      {L"ARCH D  (36\" \x00D7 24\")", 36.0, 24.0},
      {L"ARCH E  (48\" \x00D7 36\")", 48.0, 36.0},
  };

  int m_designationIndex{4};  ///< Default to ARCH E
  bool m_isLandscape{true};   ///< Default to landscape
  double m_sheetWidth{48.0};
  double m_sheetHeight{36.0};
};
