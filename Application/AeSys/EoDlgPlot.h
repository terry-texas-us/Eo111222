#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Resource.h"

/// @brief Defines which portion of the drawing to plot.
enum class PlotArea : int {
  Layout = 0,  ///< Entire paper-space layout (paper space) or drawing limits (model space).
  Extents = 1,  ///< All objects — computed from document extents.
  Display = 2,  ///< Current screen view.
  Window = 3,  ///< User-defined rectangle (future — pick corners).
};

/// @brief Aggregates all user-selected plot settings produced by the Plot dialog.
///
/// Consumed by the print lifecycle (OnPreparePrinting, OnBeginPrinting, OnPrepareDC)
/// to configure the printer device context and view transform.
struct PlotSettings {
  // Printer
  std::wstring printerName;  ///< Display name of the selected Windows printer.
  bool printToFile{};  ///< If true, prompt for output file path.

  // Paper
  int paperSizeIndex{-1};  ///< Index into the device's paper-size list (-1 = default).
  short dmPaperSize{};  ///< DEVMODE dmPaperSize value for the selected paper.
  bool landscape{true};  ///< true = landscape, false = portrait.

  // Plot area
  PlotArea plotArea{PlotArea::Extents};

  // Plot offset
  bool centerPlot{true};  ///< Auto-center the drawing on the sheet.
  double offsetX{};  ///< Manual X offset from lower-left of printable area (inches).
  double offsetY{};  ///< Manual Y offset from lower-left of printable area (inches).

  // Plot scale
  bool fitToPaper{true};  ///< Auto-scale to fill the sheet.
  int scalePresetIndex{-1};  ///< Index into standard scale list (-1 = custom).
  double customPaperUnits{1.0};  ///< Paper units (inches) in custom scale ratio.
  double customDrawingUnits{1.0};  ///< Drawing units in custom scale ratio.
  bool scaleLineweights{};  ///< Scale lineweights proportionally with plot scale.

  // Copies
  int copies{1};

  /// @brief Computes the effective scale factor (paper inches per drawing unit).
  /// Returns 0.0 when fitToPaper is true (scale determined at print time from extents).
  [[nodiscard]] double EffectiveScaleFactor() const noexcept {
    if (fitToPaper) { return 0.0; }
    if (customDrawingUnits < 1.0e-9) { return 1.0; }
    return customPaperUnits / customDrawingUnits;
  }
};

/// @brief Describes a paper size reported by a printer driver.
struct PlotPaperSize {
  std::wstring name;  ///< Display name (e.g., "Letter 8 1/2 x 11 in").
  short dmPaperSize{};  ///< DEVMODE dmPaperSize constant.
  double widthMm{};  ///< Width in millimetres (landscape orientation).
  double heightMm{};  ///< Height in millimetres (landscape orientation).
};

/// @brief Standard plot scale preset entry.
struct PlotScalePreset {
  const wchar_t* label;  ///< Display text (e.g., "1:1", "1/4\" = 1'-0\"").
  double paperUnits;  ///< Inches on paper.
  double drawingUnits;  ///< Drawing units.
};

/// @brief Unified Plot dialog — replaces Print Setup, Print, Plot submenu, and Print Preview.
///
/// Presents printer/plotter selection, paper size, orientation, plot area, offset,
/// scale, copies, and a placeholder preview area. On OK the caller receives a fully
/// populated PlotSettings struct that drives the MFC print lifecycle.
class EoDlgPlot : public CDialog {
  DECLARE_DYNAMIC(EoDlgPlot)

 public:
  explicit EoDlgPlot(CWnd* parent = nullptr);

  enum { IDD = IDD_PLOT };

  /// @brief Plot settings produced by this dialog on IDOK.
  [[nodiscard]] const PlotSettings& Settings() const noexcept { return m_settings; }

  /// @brief Pre-populate the dialog with settings from a previous run or defaults.
  void SetInitialSettings(const PlotSettings& settings) { m_settings = settings; }

 protected:
  void DoDataExchange(CDataExchange* dataExchange) override;
  BOOL OnInitDialog() override;
  void OnOK() override;

  DECLARE_MESSAGE_MAP()

  afx_msg void OnPrinterSelectionChanged();
  afx_msg void OnPrinterProperties();
  afx_msg void OnFitToPaperClicked();
  afx_msg void OnCenterPlotClicked();
  afx_msg void OnOrientationChanged();
  afx_msg void OnPaperSizeSelectionChanged();
  afx_msg void OnScaleSelectionChanged();

 private:
  /// Populates the printer combo with installed Windows printers.
  void PopulatePrinterCombo();

  /// Populates the paper-size combo for the currently selected printer.
  void PopulatePaperSizeCombo();

  /// Populates the scale preset combo.
  void PopulateScaleCombo();

  /// Enables/disables controls based on current checkbox state.
  void UpdateControlStates();

  /// Updates the printable-area label for the current paper selection.
  void UpdatePrintableAreaLabel();

  PlotSettings m_settings;

  /// Paper sizes reported by the currently selected printer.
  std::vector<PlotPaperSize> m_paperSizes;

  /// Standard scale presets.
  static constexpr PlotScalePreset kScalePresets[] = {
      {L"Fit to paper", 0.0, 0.0},
      {L"1:1", 1.0, 1.0},
      {L"1:2", 1.0, 2.0},
      {L"1:4", 1.0, 4.0},
      {L"1:5", 1.0, 5.0},
      {L"1:8", 1.0, 8.0},
      {L"1:10", 1.0, 10.0},
      {L"1:16", 1.0, 16.0},
      {L"1:20", 1.0, 20.0},
      {L"1:30", 1.0, 30.0},
      {L"1:40", 1.0, 40.0},
      {L"1:50", 1.0, 50.0},
      {L"1:100", 1.0, 100.0},
      {L"1/8\" = 1'-0\"", 0.125, 12.0},
      {L"3/16\" = 1'-0\"", 0.1875, 12.0},
      {L"1/4\" = 1'-0\"", 0.25, 12.0},
      {L"3/8\" = 1'-0\"", 0.375, 12.0},
      {L"1/2\" = 1'-0\"", 0.5, 12.0},
      {L"3/4\" = 1'-0\"", 0.75, 12.0},
      {L"1\" = 1'-0\"", 1.0, 12.0},
      {L"1-1/2\" = 1'-0\"", 1.5, 12.0},
      {L"3\" = 1'-0\"", 3.0, 12.0},
  };
};
