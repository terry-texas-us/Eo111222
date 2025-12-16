#pragma once

#ifndef __AFXWIN_H__
#error include 'Stdafx.h' befor including this file for PCH
#endif

#if defined(USING_ODA)
#include "D:/Teigha/TD_vc10mtdbg/TD/Extensions/ExServices/ExHostAppServices.h"
#include "D:/Teigha/TD_vc10mtdbg/TD/Extensions/ExServices/ExSystemServices.h"
#endif  // USING_ODA

#include "EoApOptions.h"
#include "EoDb.h"
#include "EoGePoint3d.h"

extern COLORREF ViewBackgroundColor;
extern COLORREF RubberbandColor;

extern COLORREF ColorPalette[256];
extern COLORREF GreyPalette[16];

extern COLORREF* pColTbl;

extern double dPWids[];

class AeSys : public CWinAppEx
#if defined(USING_ODA)
    ,
              public ExSystemServices,
              public ExHostAppServices {
 protected:
  using CWinApp::operator new;
  using CWinApp::operator delete;
  void addRef() {}
  void release() {}

 public:
#else   // ! USING_ODA
{
#endif  // USING_ODA

 public:
  AeSys();
  AeSys(const AeSys&) = delete;
  AeSys& operator=(const AeSys&) = delete;
  // Overrides
 public:
  virtual BOOL InitInstance();
  virtual int ExitInstance();
  virtual void PreLoadState();

 public:
  enum Units {
    kArchitecturalS = -1,  // Embedded S format
    kArchitectural,
    kEngineering,
    kFeet,
    kInches,
    kMeters,
    kMillimeters,
    kCentimeters,
    kDecimeters,
    kKilometers
  };
  static CString CustomLButtonDownCharacters;
  static CString CustomLButtonUpCharacters;
  static CString CustomRButtonDownCharacters;
  static CString CustomRButtonUpCharacters;

 private:
  int m_ArchitecturalUnitsFractionPrecision{0};
  bool m_ClipboardDataEoGroups{false};
  bool m_ClipboardDataImage{false};
  bool m_ClipboardDataText{false};
  UINT m_ClipboardFormatIdentifierForEoGroups{0};
  int m_CurrentMode{0};
  double m_DeviceHeightInMillimeters{0.0};
  double m_DeviceHeightInPixels{0.0};
  double m_DeviceWidthInMillimeters{0.0};
  double m_DeviceWidthInPixels{0.0};
  double m_DimensionAngle{0.0};
  double m_DimensionLength{0.0};
  double m_EngagedAngle{0.0};
  double m_EngagedLength{0.0};
  bool m_HighColorMode{false};
  EoGePoint3d m_HomePoints[9];
  HMENU m_MainFrameMenuHandle{nullptr};
  bool m_ModeInformationOverView{false};
  int m_ModeResourceIdentifier{0};
  CMultiDocTemplate* m_PegDocTemplate{nullptr};
  int m_PrimaryMode{0};
  CString m_ShadowFolderPath;
  char* m_SimplexStrokeFont{nullptr};
  CMultiDocTemplate* m_TracingDocTemplate{nullptr};
  EoInt16 m_TrapHighlightColor{0};
  bool m_TrapHighlighted{false};
  bool m_TrapModeAddGroups{false};
  Units m_Units{kEngineering};

 public:
  bool m_NodalModeAddGroups;
  EoApOptions m_Options;

#if defined(USING_Direct2D)
  ID2D1Factory* m_Direct2dFactory;
#endif  // USING_Direct2D

 public:
  void AddModeInformationToMessageList();
  void AddStringToMessageList(const CString& message);
  void AddStringToMessageList(UINT stringResourceIdentifier);
  void AddStringToMessageList(UINT stringResourceIdentifier, const CString& string);
  // Modifies the base accelerator table by defining the mode specific keys.
  void BuildModifiedAcceleratorTable();
  UINT CheckMenuItem(UINT uId, UINT uCheck) { return (::CheckMenuItem(m_MainFrameMenuHandle, uId, uCheck)); }
  UINT ClipboardFormatIdentifierForEoGroups() { return (m_ClipboardFormatIdentifierForEoGroups); }
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
  void FormatLength(CString& lengthAsString, Units units, const double length, const int minWidth = 0,
                    const int precision = 4);

  /// @brief Formats a length value as an architectural measurement string in feet and inches with fractional inches.
  /// @param lengthAsBuffer The output buffer to receive the formatted architectural length string.
  /// @param bufSize The size of the output buffer in wide characters.
  /// @param units The architectural units style to use for formatting (e.g., kArchitecturalS for stacked fractions).
  /// @param length The length value to format, in the internal unit system.
  void FormatLengthArchitectural(LPWSTR lengthAsBuffer, const size_t bufSize, Units units, const double length);

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
  void FormatLengthSimple(LPWSTR lengthAsBuffr, const size_t bufSize, Units units, const double length, const int width,
                          const int precision);

  int GetArchitecturalUnitsFractionPrecision() { return (m_ArchitecturalUnitsFractionPrecision); }
  EoGePoint3d GetCursorPosition();
  bool IsClipboardDataGroups() { return m_ClipboardDataEoGroups; }
  bool IsClipboardDataImage() { return m_ClipboardDataImage; }
  bool IsClipboardDataText() { return m_ClipboardDataText; }
  static EoDb::FileTypes GetFileTypeFromPath(const CString& pathName);
  HINSTANCE GetInstance() { return (m_hInstance); }
  HWND GetSafeHwnd() { return (AfxGetMainWnd()->GetSafeHwnd()); }
  HMENU GetSubMenu(int position) { return (::GetSubMenu(m_MainFrameMenuHandle, position)); }
  Units GetUnits() { return (m_Units); }
  /// <summary>Finds the greatest common divisor of arbitrary integers.</summary>
  /// <returns>First number if second number is zero, greatest common divisor otherwise.</returns>
  int GreatestCommonDivisor(const int number1, const int number2);
  void LoadHatchesFromFile(const CString& strFileName);
  bool HighColorMode() const { return m_HighColorMode; }
  EoGePoint3d HomePointGet(int i);
  void HomePointSave(int i, const EoGePoint3d& pt);
  void InitGbls(CDC* deviceContext);
  bool IsTrapHighlighted() { return m_TrapHighlighted; }
  void LoadModeResources(int mode);
  void LoadSimplexStrokeFont(const CString& pathName);
  bool ModeInformationOverView() const { return m_ModeInformationOverView; }
  double ParseLength(LPWSTR lengthAsString);
  double ParseLength(Units units, LPWSTR);
  COLORREF PenColorsGetHot(EoInt16 nPenColor) { return (ColorPalette[nPenColor]); }
  void LoadPenColorsFromFile(const CString& pathName);
  double PenWidthsGet(EoInt16 nPenColor) { return (dPWids[nPenColor]); }
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
  void SetUnits(Units units) { m_Units = units; }
  CString ShadowFolderPath() { return m_ShadowFolderPath; }
  char* SimplexStrokeFont() { return m_SimplexStrokeFont; }
  EoInt16 TrapHighlightColor() { return m_TrapHighlightColor; }
  void UpdateMDITabs(BOOL resetMDIChild);
  void WarningMessageBox(UINT stringResourceIdentifier);
  void WarningMessageBox(UINT stringResourceIdentifier, const CString& string);

 public:
  afx_msg void OnAppAbout();
  afx_msg void OnEditClipboardDataGroups();
  afx_msg void OnEditClipboardDataText();
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
  afx_msg void OnModeRLPD();
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
  afx_msg void OnUpdateModeRlpd(CCmdUI* pCmdUI);
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

CString EoAppGetPathFromCommandLine();
CString EoAppLoadStringResource(UINT resourceIdentifier);
COLORREF AppGetTextCol();
