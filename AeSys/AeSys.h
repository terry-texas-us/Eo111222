#pragma once
#ifndef __AFXWIN_H__
#error include 'Stdafx.h' before including this file for PCH
#endif

#include <string>

#include "Eo.h"
#include "EoApOptions.h"
#include "EoDb.h"
#include "EoGePoint3d.h"

extern COLORREF ViewBackgroundColor;

extern COLORREF ColorPalette[256];
extern COLORREF GreyPalette[16];

extern COLORREF* pColTbl;

namespace App {
/** @brief Determines the file type based on the file extension of the provided path name.
    @param pathName The path name of the file.
    @return The determined file type.
*/
[[nodiscard]] EoDb::FileTypes FileTypeFromPath(const CString& pathName);

/** Retrieves the path of the executable from the command line, excluding the executable name itself.
   For example, if the command line is "C:\Program Files\MyApp\MyApp.exe", this function will return "C:\Program Files\MyApp".
*/
[[nodiscard]] CString PathFromCommandLine();

/** @brief Loads a string resource from the application's resource table.
 *  @param resourceIdentifier The resource ID of the string to load.
 *  @return The loaded string resource.
 */
[[nodiscard]] CString LoadStringResource(UINT resourceIdentifier);

[[nodiscard]] CString ResourceFolderPath();

[[nodiscard]] inline COLORREF ViewTextColor() { return (~(ViewBackgroundColor | 0xff000000)); }
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
  CString CustomLButtonDownCharacters{};
  CString CustomLButtonUpCharacters{L"{13}"};
  CString CustomRButtonDownCharacters{};
  CString CustomRButtonUpCharacters{L"{27}"};
  EoApOptions m_Options;

 private:
  EoGePoint3d m_HomePoints[9];
  CMultiDocTemplate* m_pegDocumentTemplate;
  CMultiDocTemplate* m_traDocumentTemplate;
  HMENU m_MainFrameMenuHandle;
  char* m_SimplexStrokeFont;

  double m_DeviceHeightInMillimeters;
  double m_DeviceHeightInPixels;
  double m_DeviceWidthInMillimeters;
  double m_DeviceWidthInPixels;
  double m_DimensionAngle;
  double m_DimensionLength;
  double m_EngagedAngle;
  double m_EngagedLength;
  CString m_ShadowFolderPath;
  UINT m_ClipboardFormatIdentifierForEoGroups;
  Eo::Units m_Units;
  int m_ArchitecturalUnitsFractionPrecision;
  int m_CurrentMode;
  int m_ModeResourceIdentifier;
  int m_PrimaryMode;

  std::int16_t m_TrapHighlightColor;
  bool m_ClipboardDataEoGroups;
  bool m_ClipboardDataImage;
  bool m_ClipboardDataText;
  bool m_TrapHighlighted;
  bool m_TrapModeAddGroups;
  bool m_HighColorMode;
  bool m_ModeInformationOverView;

 public:
  bool m_NodalModeAddGroups;

#if defined(USING_Direct2D)
  ID2D1Factory* m_Direct2dFactory;
#endif

 public:
  void AddModeInformationToMessageList();
  void AddStringToMessageList(const CString& message);
  void AddStringToMessageList(const std::wstring& message);
  void AddStringToMessageList(const wchar_t* message);
  void AddStringToMessageList(UINT stringResourceIdentifier);
  void AddStringToMessageList(UINT stringResourceIdentifier, const CString& string);
  // Modifies the base accelerator table by defining the mode specific keys.
  void BuildModifiedAcceleratorTable();
  
  // Accessors for mode management (needed by AeSysView)
  void SetModeResourceIdentifier(int resourceId) { m_ModeResourceIdentifier = resourceId; }
  void SetPrimaryMode(int mode) { m_PrimaryMode = mode; }
  void SetModeAddGroups(bool addGroups) { m_TrapModeAddGroups = addGroups; }
  [[nodiscard]] bool TrapModeAddGroups() const { return m_TrapModeAddGroups; }
    
  UINT CheckMenuItem(UINT uId, UINT uCheck) const { return (::CheckMenuItem(m_MainFrameMenuHandle, uId, uCheck)); }
  UINT ClipboardFormatIdentifierForEoGroups() const { return m_ClipboardFormatIdentifierForEoGroups; }
  int ConfirmMessageBox(UINT stringResourceIdentifier, const CString& string);
  [[nodiscard]] int CurrentMode() const { return m_CurrentMode; }
  [[nodiscard]] double DeviceHeightInMillimeters() const { return m_DeviceHeightInMillimeters; }
  [[nodiscard]] double DeviceHeightInPixels() const { return m_DeviceHeightInPixels; }
  [[nodiscard]] double DeviceWidthInMillimeters() const { return m_DeviceWidthInMillimeters; }
  [[nodiscard]] double DeviceWidthInPixels() const { return m_DeviceWidthInPixels; }
  [[nodiscard]] double DimensionAngle() const { return m_DimensionAngle; }
  [[nodiscard]] double DimensionLength() const { return m_DimensionLength; }
  void EditColorPalette();
  [[nodiscard]] double EngagedAngle() const { return m_EngagedAngle; }
  [[nodiscard]] double EngagedLength() const { return m_EngagedLength; }
  void FormatAngle(CString& angleAsString, const double angle, const int width, const int precision);

  /** @brief Formats a length value as a string with specified units and formatting options.
   * @param lengthAsString Output parameter that receives the formatted length string.
   * @param units The unit system to use for formatting the length.
   * @param length The length value to format.
   * @param minWidth The minimum field width for the formatted output.
   * @param precision The number of decimal places to display.
   * @note Formatting rules follow:
   * @verbatim
   * ArchitecturalS units formatted as follows:
   *	\S[feet]'[inches].[fraction numerator]/[fraction denominator];"
   * Architectural units formatted as follows:
   *	[feet]'[inches].[fraction numerator] [fraction denominator]"
   * Engineering units formatted as follows:
   *	[feet]'[inches].[decimal inches]"
   * All other units formatted using floating decimal.
   * @endverbatim
   */
  void FormatLength(CString& lengthAsString, Eo::Units units, const double length, const int minWidth = 0,
                    const int precision = 4);

  /** @brief Formats a length value as an architectural measurement string in feet and inches with fractional inches.
   * @param lengthAsBuffer The output buffer to receive the formatted architectural length string.
   * @param bufSize The size of the output buffer in wide characters.
   * @param units The architectural units style to use for formatting (e.g., ArchitecturalS for stacked fractions).
   * @param length The length value to format, in the internal unit system.
   */
  void FormatLengthArchitectural(LPWSTR lengthAsBuffer, const size_t bufSize, Eo::Units units, const double length);

  /** @brief Formats a length value in engineering units (feet and inches) and stores it in a buffer.
   * @param lengthAsBuffer Output buffer to receive the formatted length string.
   * @param bufSize The size of the output buffer in characters.
   * @param length The length value to format, in internal units.
   * @param width The minimum field width for formatting the fractional part.
   * @param precision The number of significant digits to display in the formatted output.
   */
  void FormatLengthEngineering(LPWSTR lengthAsBuffer, const size_t bufSize, const double length, const int width,
                               const int precision);

  /** @brief Formats a length value as a string with the specified units, width, and precision.
   * @param lengthAsString Output buffer that receives the formatted length string.
   * @param bufSize The size of the output buffer in characters.
   * @param units The units to use for formatting the length (e.g., feet, inches, meters, millimeters).
   * @param length The length value to format, in the base measurement system.
   * @param width The minimum field width for the formatted number.
   * @param precision The number of decimal places to display in the formatted number.
   */
  void FormatLengthSimple(LPWSTR lengthAsBuffr, const size_t bufSize, Eo::Units units, const double length,
                          const int width, const int precision);

  [[nodiscard]] int GetArchitecturalUnitsFractionPrecision() const { return m_ArchitecturalUnitsFractionPrecision; }
  [[nodiscard]] static EoGePoint3d GetCursorPosition();
  [[nodiscard]] static HINSTANCE GetInstance();
  [[nodiscard]] static CWnd* GetMainWindow();
  [[nodiscard]] static HWND GetSafeHwnd();
  [[nodiscard]] bool IsClipboardDataGroups() const { return m_ClipboardDataEoGroups; }
  [[nodiscard]] bool IsClipboardDataImage() const { return m_ClipboardDataImage; }
  [[nodiscard]] bool IsClipboardDataText() const { return m_ClipboardDataText; }
  [[nodiscard]] HMENU GetSubMenu(int position) const { return (::GetSubMenu(m_MainFrameMenuHandle, position)); }
  [[nodiscard]] Eo::Units GetUnits() const { return m_Units; }

  void LoadHatchesFromFile(const CString& strFileName);
  [[nodiscard]] bool HighColorMode() const { return m_HighColorMode; }
  [[nodiscard]] EoGePoint3d HomePointGet(int i) const;
  void HomePointSave(int i, const EoGePoint3d& pt);
  void InitGbls(CDC* deviceContext);
  [[nodiscard]] bool IsTrapHighlighted() const { return m_TrapHighlighted; }
  void LoadModeResources(int mode);
  void LoadSimplexStrokeFont(const CString& pathName);
  bool ModeInformationOverView() const { return m_ModeInformationOverView; }
  [[nodiscard]] double ParseLength(wchar_t* lengthAsString);
  [[nodiscard]] double ParseLength(Eo::Units units, wchar_t* inputLine);
  [[nodiscard]] auto PenColorsGetHot(std::int16_t color) { return (ColorPalette[color]); }
  void LoadPenColorsFromFile(const CString& pathName);

  [[nodiscard]] double LineWeight(std::int16_t penIndex);

  /** Loads the pen widths from a file.
 * The file is expected to have lines in the format:
 *   penIndex=penWidth
 * Lines starting with '#' or ';' are treated as comments and ignored.
 * If the penIndex is out of range, that line is ignored.
 */
  void LoadPenWidthsFromFile(const CString& pathName);

  [[nodiscard]] int PrimaryMode() const { return m_PrimaryMode; }
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

  /*** @brief Sets the shadow folder path for the application.
   * @param folder The name of the folder to be used as the shadow folder.
   * @return 0 if the folder was successfully created or already exists, or a non-zero error code if there was an error creating the folder.
   */
  [[nodiscard]] int SetShadowFolderPath(const CString& folder);
  void SetUnits(Eo::Units units) { m_Units = units; }
  [[nodiscard]] CString ShadowFolderPath() { return m_ShadowFolderPath; }
  char* SimplexStrokeFont() { return m_SimplexStrokeFont; }
  [[nodiscard]] std::int16_t TrapHighlightColor() const { return m_TrapHighlightColor; }
  void UpdateMDITabs(BOOL resetMDIChild);
  void WarningMessageBox(UINT stringResourceIdentifier);
  void WarningMessageBox(UINT stringResourceIdentifier, const CString& string);
  [[nodiscard]] const EoApOptions& PropertyOptions() const { return m_Options; }

 public:
  afx_msg void OnAppAbout();
  afx_msg void OnFileOpen();
  afx_msg void OnFileRun();
  afx_msg void OnHelpContents();
  afx_msg void OnModeLetter();
  afx_msg void OnModeRevise();
  afx_msg void OnTrapCommandsHighlight();
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