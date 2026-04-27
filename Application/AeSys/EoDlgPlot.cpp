#include "Stdafx.h"

#include <winspool.h>

#include "Eo.h"
#include "EoDlgPlot.h"

#pragma comment(lib, "winspool.lib")

IMPLEMENT_DYNAMIC(EoDlgPlot, CDialog)

EoDlgPlot::EoDlgPlot(CWnd* parent) : CDialog(IDD, parent) {}

void EoDlgPlot::DoDataExchange(CDataExchange* dataExchange) {
  CDialog::DoDataExchange(dataExchange);
}

BEGIN_MESSAGE_MAP(EoDlgPlot, CDialog)
ON_CBN_SELCHANGE(IDC_PLOT_PRINTER_COMBO, OnPrinterSelectionChanged)
ON_BN_CLICKED(IDC_PLOT_PRINTER_PROPERTIES, OnPrinterProperties)
ON_CBN_SELCHANGE(IDC_PLOT_PAPER_SIZE_COMBO, OnPaperSizeSelectionChanged)
ON_CBN_SELCHANGE(IDC_PLOT_SCALE_COMBO, OnScaleSelectionChanged)
ON_BN_CLICKED(IDC_PLOT_FIT_TO_PAPER, OnFitToPaperClicked)
ON_BN_CLICKED(IDC_PLOT_CENTER_PLOT, OnCenterPlotClicked)
ON_BN_CLICKED(IDC_PLOT_ORIENTATION_LANDSCAPE, OnOrientationChanged)
ON_BN_CLICKED(IDC_PLOT_ORIENTATION_PORTRAIT, OnOrientationChanged)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
// OnInitDialog — populate all controls with initial/default values
// ---------------------------------------------------------------------------

BOOL EoDlgPlot::OnInitDialog() {
  CDialog::OnInitDialog();

  PopulatePrinterCombo();
  PopulateScaleCombo();

  // Orientation
  CheckRadioButton(IDC_PLOT_ORIENTATION_LANDSCAPE,
      IDC_PLOT_ORIENTATION_PORTRAIT,
      m_settings.landscape ? IDC_PLOT_ORIENTATION_LANDSCAPE : IDC_PLOT_ORIENTATION_PORTRAIT);

  // Plot area
  int plotAreaId = IDC_PLOT_AREA_LAYOUT;
  switch (m_settings.plotArea) {
    case PlotArea::Layout:
      plotAreaId = IDC_PLOT_AREA_LAYOUT;
      break;
    case PlotArea::Extents:
      plotAreaId = IDC_PLOT_AREA_EXTENTS;
      break;
    case PlotArea::Display:
      plotAreaId = IDC_PLOT_AREA_DISPLAY;
      break;
    case PlotArea::Window:
      plotAreaId = IDC_PLOT_AREA_WINDOW;
      break;
  }
  CheckRadioButton(IDC_PLOT_AREA_LAYOUT, IDC_PLOT_AREA_WINDOW, plotAreaId);

  // Plot offset
  CheckDlgButton(IDC_PLOT_CENTER_PLOT, m_settings.centerPlot ? BST_CHECKED : BST_UNCHECKED);
  {
    CString str;
    str.Format(L"%.4f", m_settings.offsetX);
    SetDlgItemText(IDC_PLOT_OFFSET_X, str);
    str.Format(L"%.4f", m_settings.offsetY);
    SetDlgItemText(IDC_PLOT_OFFSET_Y, str);
  }

  // Plot scale
  CheckDlgButton(IDC_PLOT_FIT_TO_PAPER, m_settings.fitToPaper ? BST_CHECKED : BST_UNCHECKED);
  {
    CString str;
    str.Format(L"%.4f", m_settings.customPaperUnits);
    SetDlgItemText(IDC_PLOT_CUSTOM_SCALE_EDIT, str);
    str.Format(L"%.4f", m_settings.customDrawingUnits);
    SetDlgItemText(IDC_PLOT_CUSTOM_UNITS_EDIT, str);
  }

  CheckDlgButton(IDC_PLOT_SCALE_LINEWEIGHTS, m_settings.scaleLineweights ? BST_CHECKED : BST_UNCHECKED);

  // Copies
  SetDlgItemInt(IDC_PLOT_COPIES, static_cast<UINT>(m_settings.copies), FALSE);

  // Print to file
  CheckDlgButton(IDC_PLOT_PRINT_TO_FILE, m_settings.printToFile ? BST_CHECKED : BST_UNCHECKED);

  UpdateControlStates();

  return TRUE;
}

void EoDlgPlot::OnOK() {
  const auto* printerCombo = static_cast<const CComboBox*>(GetDlgItem(IDC_PLOT_PRINTER_COMBO));
  if (printerCombo != nullptr) {
    CString name;
    const int sel = printerCombo->GetCurSel();
    if (sel != CB_ERR) {
      printerCombo->GetLBText(sel, name);
      m_settings.printerName = static_cast<const wchar_t*>(name);
    }
  }

  m_settings.printToFile = IsDlgButtonChecked(IDC_PLOT_PRINT_TO_FILE) == BST_CHECKED;

  // Paper size
  const auto* paperCombo = static_cast<const CComboBox*>(GetDlgItem(IDC_PLOT_PAPER_SIZE_COMBO));
  if (paperCombo != nullptr) {
    const int sel = paperCombo->GetCurSel();
    if (sel != CB_ERR && sel < static_cast<int>(m_paperSizes.size())) {
      m_settings.paperSizeIndex = sel;
      m_settings.dmPaperSize = m_paperSizes[sel].dmPaperSize;
    }
  }

  // Orientation
  m_settings.landscape = IsDlgButtonChecked(IDC_PLOT_ORIENTATION_LANDSCAPE) == BST_CHECKED;

  // Plot area
  if (IsDlgButtonChecked(IDC_PLOT_AREA_LAYOUT) == BST_CHECKED) {
    m_settings.plotArea = PlotArea::Layout;
  } else if (IsDlgButtonChecked(IDC_PLOT_AREA_EXTENTS) == BST_CHECKED) {
    m_settings.plotArea = PlotArea::Extents;
  } else if (IsDlgButtonChecked(IDC_PLOT_AREA_DISPLAY) == BST_CHECKED) {
    m_settings.plotArea = PlotArea::Display;
  } else if (IsDlgButtonChecked(IDC_PLOT_AREA_WINDOW) == BST_CHECKED) {
    m_settings.plotArea = PlotArea::Window;
  }

  // Plot offset
  m_settings.centerPlot = IsDlgButtonChecked(IDC_PLOT_CENTER_PLOT) == BST_CHECKED;
  {
    CString str;
    GetDlgItemText(IDC_PLOT_OFFSET_X, str);
    m_settings.offsetX = _wtof(str);
    GetDlgItemText(IDC_PLOT_OFFSET_Y, str);
    m_settings.offsetY = _wtof(str);
  }

  // Plot scale
  m_settings.fitToPaper = IsDlgButtonChecked(IDC_PLOT_FIT_TO_PAPER) == BST_CHECKED;
  m_settings.scaleLineweights = IsDlgButtonChecked(IDC_PLOT_SCALE_LINEWEIGHTS) == BST_CHECKED;
  {
    const auto* scaleCombo = static_cast<const CComboBox*>(GetDlgItem(IDC_PLOT_SCALE_COMBO));
    if (scaleCombo != nullptr) { m_settings.scalePresetIndex = scaleCombo->GetCurSel(); }

    CString str;
    GetDlgItemText(IDC_PLOT_CUSTOM_SCALE_EDIT, str);
    m_settings.customPaperUnits = _wtof(str);
    if (m_settings.customPaperUnits <= 0.0) { m_settings.customPaperUnits = 1.0; }

    GetDlgItemText(IDC_PLOT_CUSTOM_UNITS_EDIT, str);
    m_settings.customDrawingUnits = _wtof(str);
    if (m_settings.customDrawingUnits <= 0.0) { m_settings.customDrawingUnits = 1.0; }
  }

  // If a scale preset is selected (not the "Fit to paper" entry 0 and not Custom), override custom fields
  if (!m_settings.fitToPaper && m_settings.scalePresetIndex > 0
      && m_settings.scalePresetIndex < static_cast<int>(std::size(kScalePresets))) {
    const auto& preset = kScalePresets[m_settings.scalePresetIndex];
    m_settings.customPaperUnits = preset.paperUnits;
    m_settings.customDrawingUnits = preset.drawingUnits;
  }

  // Copies
  BOOL translated{};
  const UINT copies = GetDlgItemInt(IDC_PLOT_COPIES, &translated, FALSE);
  m_settings.copies = translated && copies > 0 ? static_cast<int>(copies) : 1;

  CDialog::OnOK();
}

// ---------------------------------------------------------------------------
// Printer enumeration
// ---------------------------------------------------------------------------

void EoDlgPlot::PopulatePrinterCombo() {
  auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_PLOT_PRINTER_COMBO));
  if (combo == nullptr) { return; }
  combo->ResetContent();

  // Get the default printer name via GetDefaultPrinterW (avoids CPrintDialog
  // whose destructor would GlobalFree the app's shared DEVMODE/DEVNAMES handles).
  CString defaultPrinter;
  {
    DWORD bufferSize{};
    ::GetDefaultPrinterW(nullptr, &bufferSize);
    if (bufferSize > 0) {
      std::vector<wchar_t> buffer(bufferSize);
      if (::GetDefaultPrinterW(buffer.data(), &bufferSize)) { defaultPrinter = buffer.data(); }
    }
  }

  // Enumerate installed printers
  DWORD needed{};
  DWORD returned{};
  ::EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, nullptr, 2, nullptr, 0, &needed, &returned);

  if (needed > 0) {
    std::vector<BYTE> buffer(needed);
    if (::EnumPrintersW(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
            nullptr,
            2,
            buffer.data(),
            static_cast<DWORD>(buffer.size()),
            &needed,
            &returned)) {
      const auto* printers = reinterpret_cast<const PRINTER_INFO_2*>(buffer.data());
      int defaultIndex = -1;
      for (DWORD i = 0; i < returned; ++i) {
        const int idx = combo->AddString(printers[i].pPrinterName);
        if (defaultPrinter.CompareNoCase(printers[i].pPrinterName) == 0) { defaultIndex = idx; }
      }
      if (defaultIndex >= 0) {
        combo->SetCurSel(defaultIndex);
      } else if (returned > 0) {
        combo->SetCurSel(0);
      }
    }
  }

  // Use pre-selected printer name if provided
  if (!m_settings.printerName.empty()) {
    const int idx = combo->FindStringExact(-1, m_settings.printerName.c_str());
    if (idx != CB_ERR) { combo->SetCurSel(idx); }
  }

  PopulatePaperSizeCombo();
}

// ---------------------------------------------------------------------------
// Paper size enumeration from the selected printer driver
// ---------------------------------------------------------------------------

void EoDlgPlot::PopulatePaperSizeCombo() {
  auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_PLOT_PAPER_SIZE_COMBO));
  if (combo == nullptr) { return; }
  combo->ResetContent();
  m_paperSizes.clear();

  // Get currently selected printer name
  const auto* printerCombo = static_cast<const CComboBox*>(GetDlgItem(IDC_PLOT_PRINTER_COMBO));
  if (printerCombo == nullptr) { return; }
  CString printerName;
  const int sel = printerCombo->GetCurSel();
  if (sel == CB_ERR) { return; }
  printerCombo->GetLBText(sel, printerName);

  // Query paper names
  const int count = ::DeviceCapabilitiesW(printerName, nullptr, DC_PAPERNAMES, nullptr, nullptr);
  if (count <= 0) { return; }

  // Paper names are 64-wchar blocks
  std::vector<wchar_t> nameBuffer(static_cast<size_t>(count) * 64);
  ::DeviceCapabilitiesW(printerName, nullptr, DC_PAPERNAMES, nameBuffer.data(), nullptr);

  // Paper IDs
  std::vector<WORD> paperIds(count);
  ::DeviceCapabilitiesW(printerName, nullptr, DC_PAPERS, reinterpret_cast<wchar_t*>(paperIds.data()), nullptr);

  // Paper sizes (tenths of mm)
  std::vector<POINT> paperSizes(count);
  ::DeviceCapabilitiesW(printerName, nullptr, DC_PAPERSIZE, reinterpret_cast<wchar_t*>(paperSizes.data()), nullptr);

  int selectIndex = 0;
  for (int i = 0; i < count; ++i) {
    PlotPaperSize ps;
    ps.name = &nameBuffer[static_cast<size_t>(i) * 64];
    ps.dmPaperSize = static_cast<short>(paperIds[i]);
    ps.widthMm = paperSizes[i].x / 10.0;
    ps.heightMm = paperSizes[i].y / 10.0;
    m_paperSizes.push_back(ps);

    combo->AddString(ps.name.c_str());

    // Match previously selected paper
    if (m_settings.dmPaperSize != 0 && ps.dmPaperSize == m_settings.dmPaperSize) { selectIndex = i; }
  }

  if (!m_paperSizes.empty()) { combo->SetCurSel(selectIndex); }

  UpdatePrintableAreaLabel();
}

// ---------------------------------------------------------------------------
// Scale presets
// ---------------------------------------------------------------------------

void EoDlgPlot::PopulateScaleCombo() {
  auto* combo = static_cast<CComboBox*>(GetDlgItem(IDC_PLOT_SCALE_COMBO));
  if (combo == nullptr) { return; }
  combo->ResetContent();

  for (const auto& preset : kScalePresets) { combo->AddString(preset.label); }

  // Default: "1:1" (index 1) unless fitToPaper is checked
  int selectIndex = m_settings.fitToPaper ? 0 : 1;
  if (m_settings.scalePresetIndex >= 0 && m_settings.scalePresetIndex < static_cast<int>(std::size(kScalePresets))) {
    selectIndex = m_settings.scalePresetIndex;
  }
  combo->SetCurSel(selectIndex);
}

// ---------------------------------------------------------------------------
// Control state management
// ---------------------------------------------------------------------------

void EoDlgPlot::UpdateControlStates() {
  const bool fitToPaper = IsDlgButtonChecked(IDC_PLOT_FIT_TO_PAPER) == BST_CHECKED;

  // When fit-to-paper is checked, disable manual scale controls
  GetDlgItem(IDC_PLOT_SCALE_COMBO)->EnableWindow(!fitToPaper);
  GetDlgItem(IDC_PLOT_CUSTOM_SCALE_EDIT)->EnableWindow(!fitToPaper);
  GetDlgItem(IDC_PLOT_CUSTOM_UNITS_EDIT)->EnableWindow(!fitToPaper);

  const bool centerPlot = IsDlgButtonChecked(IDC_PLOT_CENTER_PLOT) == BST_CHECKED;

  // When center is checked, disable manual offset fields
  GetDlgItem(IDC_PLOT_OFFSET_X)->EnableWindow(!centerPlot);
  GetDlgItem(IDC_PLOT_OFFSET_Y)->EnableWindow(!centerPlot);
}

void EoDlgPlot::UpdatePrintableAreaLabel() {
  const auto* combo = static_cast<const CComboBox*>(GetDlgItem(IDC_PLOT_PAPER_SIZE_COMBO));
  if (combo == nullptr) { return; }
  const int sel = combo->GetCurSel();
  if (sel == CB_ERR || sel >= static_cast<int>(m_paperSizes.size())) {
    SetDlgItemText(IDC_PLOT_PRINTABLE_AREA, L"");
    return;
  }

  const auto& ps = m_paperSizes[sel];
  double widthInches = ps.widthMm / Eo::MmPerInch;
  double heightInches = ps.heightMm / Eo::MmPerInch;

  const bool landscape = IsDlgButtonChecked(IDC_PLOT_ORIENTATION_LANDSCAPE) == BST_CHECKED;
  if (!landscape) { std::swap(widthInches, heightInches); }

  CString label;
  label.Format(L"%.2f \x00D7 %.2f in  (%.0f \x00D7 %.0f mm)",
      widthInches,
      heightInches,
      landscape ? ps.widthMm : ps.heightMm,
      landscape ? ps.heightMm : ps.widthMm);
  SetDlgItemText(IDC_PLOT_PRINTABLE_AREA, label);
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------

void EoDlgPlot::OnPrinterSelectionChanged() {
  PopulatePaperSizeCombo();
  UpdateControlStates();
}

void EoDlgPlot::OnPrinterProperties() {
  const auto* printerCombo = static_cast<const CComboBox*>(GetDlgItem(IDC_PLOT_PRINTER_COMBO));
  if (printerCombo == nullptr) { return; }
  CString printerName;
  const int sel = printerCombo->GetCurSel();
  if (sel == CB_ERR) { return; }
  printerCombo->GetLBText(sel, printerName);

  HANDLE hPrinter{};
  if (!::OpenPrinterW(printerName.GetBuffer(), &hPrinter, nullptr)) { return; }
  printerName.ReleaseBuffer();

  // Show the printer properties dialog
  ::PrinterProperties(GetSafeHwnd(), hPrinter);
  ::ClosePrinter(hPrinter);

  // Refresh paper sizes in case the user changed settings
  PopulatePaperSizeCombo();
}

void EoDlgPlot::OnFitToPaperClicked() {
  UpdateControlStates();
}

void EoDlgPlot::OnCenterPlotClicked() {
  UpdateControlStates();
}

void EoDlgPlot::OnOrientationChanged() {
  UpdatePrintableAreaLabel();
}

void EoDlgPlot::OnPaperSizeSelectionChanged() {
  UpdatePrintableAreaLabel();
}

void EoDlgPlot::OnScaleSelectionChanged() {
  const auto* combo = static_cast<const CComboBox*>(GetDlgItem(IDC_PLOT_SCALE_COMBO));
  if (combo == nullptr) { return; }
  const int sel = combo->GetCurSel();
  if (sel == CB_ERR) { return; }

  if (sel == 0) {
    // "Fit to paper" preset — mirror what the checkbox does
    CheckDlgButton(IDC_PLOT_FIT_TO_PAPER, BST_CHECKED);
    UpdateControlStates();
    return;
  }

  CheckDlgButton(IDC_PLOT_FIT_TO_PAPER, BST_UNCHECKED);
  if (sel < static_cast<int>(std::size(kScalePresets))) {
    const auto& preset = kScalePresets[sel];
    CString text;
    text.Format(L"%.4f", preset.paperUnits);
    SetDlgItemText(IDC_PLOT_CUSTOM_SCALE_EDIT, text);
    text.Format(L"%.4f", preset.drawingUnits);
    SetDlgItemText(IDC_PLOT_CUSTOM_UNITS_EDIT, text);
  }
  UpdateControlStates();
}
