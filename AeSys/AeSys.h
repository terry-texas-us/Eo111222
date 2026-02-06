#pragma once
#ifndef __AFXWIN_H__
#error include 'Stdafx.h' before including this file for PCH
#endif

#include <string>

#include "EoApOptions.h"
#include "EoDb.h"
#include "EoGePoint3d.h"

extern COLORREF ViewBackgroundColor;
extern COLORREF RubberbandColor;

extern COLORREF ColorPalette[256];
extern COLORREF GreyPalette[16];

extern COLORREF* pColTbl;

namespace App {
/** Retrieves the path of the executable from the command line, excluding the executable name itself.
   For example, if the command line is "C:\Program Files\MyApp\MyApp.exe", this function will return "C:\Program Files\MyApp".
*/
[[nodiscard]] CString PathFromCommandLine();

/** @brief Loads a string resource from the application's resource table.
 *  @param resourceIdentifier The resource ID of the string to load.
 *  @return The loaded string resource.
 */
[[nodiscard]] CString LoadStringResource(UINT resourceIdentifier);

inline COLORREF ViewTextColor() { return (~(ViewBackgroundColor | 0xff000000)); }
}  // namespace App

class AeSys : public CWinAppEx {
 public:
  AeSys();
  AeSys(const AeSys&) = delete;
  AeSys& operator=(const AeSys&) = delete;
  // Overrides
 public:
  BOOL InitInstance() override;
  int ExitInstance() override;
  void PreLoadState() override;

 public:
  // This is a legacy feature. All values are empty strings now for normal MouseButton command processing.
  // User may still change through user interface, so must not assume empty.
  CString CustomLButtonDownCharacters{};
  CString CustomLButtonUpCharacters{L"{13}"};
  CString CustomRButtonDownCharacters{};
  CString CustomRButtonUpCharacters{L"{27}"};

 private:
  int m_ArchitecturalUnitsFractionPrecision{};
  bool m_ClipboardDataEoGroups{};
  bool m_ClipboardDataImage{};
  bool m_ClipboardDataText{};
  UINT m_ClipboardFormatIdentifierForEoGroups{};
  int m_CurrentMode{};
  double m_DeviceHeightInMillimeters{0.0};
  double m_DeviceHeightInPixels{0.0};
  double m_DeviceWidthInMillimeters{0.0};
  double m_DeviceWidthInPixels{0.0};
  double m_DimensionAngle{0.0};
  double m_DimensionLength{0.0};
  double m_EngagedAngle{0.0};
  double m_EngagedLength{0.0};
  bool m_HighColorMode{};
  EoGePoint3d m_HomePoints[9]{};
  HMENU m_MainFrameMenuHandle{};
  bool m_ModeInformationOverView{};
  int m_ModeResourceIdentifier{};
  CMultiDocTemplate* m_PegDocTemplate{};
  int m_PrimaryMode{};
  CString m_ShadowFolderPath;
  char* m_SimplexStrokeFont{};
  CMultiDocTemplate* m_TracingDocTemplate{};
  EoInt16 m_TrapHighlightColor{};
  bool m_TrapHighlighted{};
  bool m_TrapModeAddGroups{};
  Eo::Units m_Units{Eo::Units::Engineering};

 public:
  bool m_NodalModeAddGroups;
  EoApOptions m_Options;

#if defined(USING_Direct2D)
  ID2D1Factory* m_Direct2dFactory;
#endif  // USING_Direct2D

 public:
  void AddModeInformationToMessageList();
  void AddStringToMessageList(const CString& message);
  void AddStringToMessageList(const std::wstring& message);
  void AddStringToMessageList(const wchar_t* message);
  void AddStringToMessageList(UINT stringResourceIdentifier);
  void AddStringToMessageList(UINT stringResourceIdentifier, const CString& string);
  // Modifies the base accelerator table by defining the mode specific keys.
  void BuildModifiedAcceleratorTable();
  UINT CheckMenuItem(UINT uId, UINT uCheck) const { return (::CheckMenuItem(m_MainFrameMenuHandle, uId, uCheck)); }
  UINT ClipboardFormatIdentifierForEoGroups() const { return (m_ClipboardFormatIdentifierForEoGroups); }
  int ConfirmMessageBox(UINT stringResourceIdentifier, const CString& string);
  int CurrentMode() const { return m_CurrentMode; }
  double DeviceHeightInMillimeters() const { return m_DeviceHeightInMillimeters; }
  double DeviceHeightInPixels() const { return m_DeviceHeightInPixels; }
  double DeviceWidthInMillimeters() const { return m_DeviceWidthInMillimeters; }
  double DeviceWidthInPixels() const { return m_DeviceWidthInPixels; }
  double DimensionAngle() const { return (m_DimensionAngle); }
  double DimensionLength() const { return (m_DimensionLength); }
  void EditColorPalette();
  double EngagedAngle() const { return (m_EngagedAngle); }
  double EngagedLength() const { return (m_EngagedLength); }
  void FormatAngle(CString& angleAsString, const double angle, const int width, const int precision);

  /// @brief Formats a length value as a string with specified units and formatting options.
  /// @param lengthAsString Output parameter that receives the formatted length string.
  /// @param units The unit system to use for formatting the length.
  /// @param length The length value to format.
  /// @param minWidth The minimum field width for the formatted output.
  /// @param precision The number of decimal places to display.
  /// @note Formatting rules follow:
  /// @verbatim
  /// ArchitecturalS units formatted as follows:
  ///	\S[feet]'[inches].[fraction numerator]/[fraction denominator];"
  /// Architectural units formatted as follows:
  ///	[feet]'[inches].[fraction numerator] [fraction denominator]"
  /// Engineering units formatted as follows:
  ///	[feet]'[inches].[decimal inches]"
  /// All other units formatted using floating decimal.
  /// @endverbatim
  void FormatLength(CString& lengthAsString, Eo::Units units, const double length, const int minWidth = 0,
                    const int precision = 4);

  /// @brief Formats a length value as an architectural measurement string in feet and inches with fractional inches.
  /// @param lengthAsBuffer The output buffer to receive the formatted architectural length string.
  /// @param bufSize The size of the output buffer in wide characters.
  /// @param units The architectural units style to use for formatting (e.g., ArchitecturalS for stacked fractions).
  /// @param length The length value to format, in the internal unit system.
  void FormatLengthArchitectural(LPWSTR lengthAsBuffer, const size_t bufSize, Eo::Units units, const double length);

  /// @brief Formats a length value in engineering units (feet and inches) and stores it in a buffer.
  /// @param lengthAsBuffer Output buffer to receive the formatted length string.
  /// @param bufSize The size of the output buffer in characters.
  /// @param length The length value to format, in internal units.
  /// @param width The minimum field width for formatting the fractional part.
  /// @param precision The number of significant digits to display in the formatted output.
  void FormatLengthEngineering(LPWSTR lengthAsBuffer, const size_t bufSize, const double length, const int width,
                               const int precision);

  /// @brief Formats a length value as a string with the specified units, width, and precision.
  /// @param lengthAsString Output buffer that receives the formatted length string.
  /// @param bufSize The size of the output buffer in characters.
  /// @param units The units to use for formatting the length (e.g., feet, inches, meters, millimeters).
  /// @param length The length value to format, in the base measurement system.
  /// @param width The minimum field width for the formatted number.
  /// @param precision The number of decimal places to display in the formatted number.
  void FormatLengthSimple(LPWSTR lengthAsBuffr, const size_t bufSize, Eo::Units units, const double length, const int width,
                          const int precision);

  int GetArchitecturalUnitsFractionPrecision() const { return (m_ArchitecturalUnitsFractionPrecision); }
  EoGePoint3d GetCursorPosition();
  bool IsClipboardDataGroups() const { return m_ClipboardDataEoGroups; }
  bool IsClipboardDataImage() const { return m_ClipboardDataImage; }
  bool IsClipboardDataText() const { return m_ClipboardDataText; }
  static EoDb::FileTypes GetFileTypeFromPath(const CString& pathName);
  HINSTANCE GetInstance() { return (m_hInstance); }
  HWND GetSafeHwnd() { return (AfxGetMainWnd()->GetSafeHwnd()); }
  HMENU GetSubMenu(int position) const { return (::GetSubMenu(m_MainFrameMenuHandle, position)); }
  Eo::Units GetUnits() const { return (m_Units); }
  /// <summary>Finds the greatest common divisor of arbitrary integers.</summary>
  /// <returns>First number if second number is zero, greatest common divisor otherwise.</returns>
  int GreatestCommonDivisor(const int number1, const int number2);
  void LoadHatchesFromFile(const CString& strFileName);
  bool HighColorMode() const { return m_HighColorMode; }
  EoGePoint3d HomePointGet(int i) const;
  void HomePointSave(int i, const EoGePoint3d& pt);
  void InitGbls(CDC* deviceContext);
  bool IsTrapHighlighted() const { return m_TrapHighlighted; }
  void LoadModeResources(int mode);
  void LoadSimplexStrokeFont(const CString& pathName);
  bool ModeInformationOverView() const { return m_ModeInformationOverView; }
  double ParseLength(wchar_t* lengthAsString);
  double ParseLength(Eo::Units units, wchar_t* inputLine);
  COLORREF PenColorsGetHot(EoInt16 color) { return (ColorPalette[color]); }
  void LoadPenColorsFromFile(const CString& pathName);

  double PenWidthsGet(EoInt16 penIndex);

  /** Loads the pen widths from a file.
 * The file is expected to have lines in the format:
 *   penIndex=penWidth
 * Lines starting with '#' or ';' are treated as comments and ignored.
 * If the penIndex is out of range, that line is ignored.
 */
  void LoadPenWidthsFromFile(const CString& pathName);

  int PrimaryMode() const { return m_PrimaryMode; }
  void ReleaseSimplexStrokeFont();
  void SetArchitecturalUnitsFractionPrecision(const int precision) {
    if (precision > 0) { m_ArchitecturalUnitsFractionPrecision = precision; }
  }
  /// <summary> Positions cursor at targeted position.</summary>
  void SetCursorPosition(EoGePoint3d pt);
  void SetDimensionAngle(double angle) { m_DimensionAngle = angle; }
  void SetDimensionLength(double length) { m_DimensionLength = length; }
  void SetEngagedAngle(double angle) { m_EngagedAngle = angle; }
  void SetEngagedLength(double length) { m_EngagedLength = length; }
  CString ResourceFolderPath();
  int SetShadowFolderPath(const CString& folder);
  void SetUnits(Eo::Units units) { m_Units = units; }
  CString ShadowFolderPath() { return m_ShadowFolderPath; }
  char* SimplexStrokeFont() { return m_SimplexStrokeFont; }
  EoInt16 TrapHighlightColor() const { return m_TrapHighlightColor; }
  void UpdateMDITabs(BOOL resetMDIChild);
  void WarningMessageBox(UINT stringResourceIdentifier);
  void WarningMessageBox(UINT stringResourceIdentifier, const CString& string);

 public:
  afx_msg void OnAppAbout();
  afx_msg void OnFileOpen();
  afx_msg void OnFileRun();
  afx_msg void OnHelpContents();
  afx_msg void OnModeAnnotate();
  afx_msg void OnModeCut();
  afx_msg void OnModeDimension();
  afx_msg void OnModeDraw();
  afx_msg void OnModeDraw2();
  afx_msg void OnModeEdit();
  afx_msg void OnModeFixup();
  afx_msg void OnModeLetter();
  afx_msg void OnModeLPD();
  afx_msg void OnModeNodal();
  afx_msg void OnModePipe();
  afx_msg void OnModePower();
  afx_msg void OnModeRevise();
  afx_msg void OnModeTrap();
  afx_msg void OnTrapCommandsAddGroups();
  afx_msg void OnTrapCommandsHighlight();
  afx_msg void OnUpdateModeAnnotate(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeCut(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeDimension(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeDraw(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeDraw2(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeEdit(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeFixup(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeLpd(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeNodal(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModePipe(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModePower(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeTrap(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewModeinformation(CCmdUI* pCmdUI);
  afx_msg void OnViewModeInformation();
#if defined(USING_DDE)
 private:
  double m_ExtractedNumber;
  CString m_ExtractedString;

 public:
  double ExtractedNumber() { return m_ExtractedNumber; }
  void SetExtractedNumber(const double number) { m_ExtractedNumber = number; }
  CString ExtractedString() { return m_ExtractedString; }
  void SetExtractedString(const CString& string) { m_ExtractedString = string; }
#endif  // USING_DDE
 protected:
  DECLARE_MESSAGE_MAP()
 public:
  afx_msg void OnEditCfImage();
  afx_msg void OnUpdateEditCfImage(CCmdUI* pCmdUI);
  afx_msg void OnEditCfText();
  afx_msg void OnUpdateEditCfText(CCmdUI* pCmdUI);
  afx_msg void OnEditCfGroups();
  afx_msg void OnUpdateEditCfGroups(CCmdUI* pCmdUI);
  afx_msg void OnUpdateTrapcommandsHighlight(CCmdUI* pCmdUI);
  afx_msg void OnUpdateTrapcommandsAddgroups(CCmdUI* pCmdUI);
};

extern AeSys app;