#pragma once

#ifndef __AFXWIN_H__
	#error include 'Stdafx.h' befor including this file for PCH
#endif

#if defined(DWGDIRECT)
#include "ExHostAppServices.h"
#include "ExSystemServices.h"
#endif // DWGDIRECT

#include "EoApOptions.h"

extern COLORREF crHotCols[256];
extern COLORREF crWarmCols[16];

extern COLORREF* pColTbl;

extern double dPWids[];

class AeSysApp : public CWinAppEx
#if defined(DWGDIRECT)
	, public ExSystemServices
	, public ExHostAppServices
{
protected:
  using CWinApp::operator new;
  using CWinApp::operator delete;
public:
	void addRef() {}
	void release() {}
#else
{
#endif // DWGDIRECT

	// Overrides
public:
	virtual BOOL InitInstance(void);
	virtual int ExitInstance(void);

protected:
	afx_msg void OnAppAbout(void);
	afx_msg void OnFileOpen(void);

public:

	static TCHAR szLeftMouseDown[60];
	static TCHAR szRightMouseDown[60]; 
	static TCHAR szLeftMouseUp[60];
	static TCHAR szRightMouseUp[60];

public:
	UINT m_nAppLook;
	BOOL m_bHiColorIcons;

	CMultiDocTemplate* m_PegDocTemplate;
	CMultiDocTemplate* m_TracingDocTemplate;
	
	WNDPROC	m_wpMainWnd;			
	CString	m_strAppPath;
	EoUInt16 m_wOpHighlighted;

private:
	HMENU m_hMenu;
	
	UINT m_nClipboardFormatEoGroups;
	int m_iGinRubTyp;
	int	m_iUnitsPrec;				
	EoDb::Units m_eUnits;				
	double m_dEngAngZ;				
	double m_dEngLen;				
	double m_dDimAngZ;				
	double m_dDimLen;				
	double m_dScale;				
	EoGePoint3d m_ptCursorPosDev;			
	EoGePoint3d	m_ptRubStart;
	EoGePoint3d	m_ptRubEnd;
	EoGePoint3d	m_ptHomePoint[9];
	char* m_pStrokeFontDef;			
	CString m_strShadowDir;
	bool m_bEditCFImage;
	bool m_bEditCFGroups;
	bool m_bEditCFText;

	int m_DeviceWidthInPixels;
	int m_DeviceHeightInPixels;
	int m_DeviceWidthInMillimeters;
	int m_DeviceHeightInMillimeters;

	double m_dExtractedNumber;
	CString m_strExtractedText;

public:
	bool m_bViewStateInfo;
	bool m_bViewModeInfo;
	bool m_TrapModeAddGroups;
	bool m_NodalModeAddGroups;
	int	m_Mode;
	int	m_iPrimModeId;
	int m_ModeResourceIdentifier;

public:
	AeSysApp();
	
	virtual void PreLoadState();
	
	void UpdateMDITabs(BOOL resetMDIChild);

	EoApOptions m_Options;

	int DeviceWidthInPixels() {return m_DeviceWidthInPixels;}
	int DeviceHeightInPixels() {return m_DeviceHeightInPixels;}
	int DeviceWidthInMillimeters() {return m_DeviceWidthInMillimeters;}
	int DeviceHeightInMillimeters() {return m_DeviceHeightInMillimeters;}
	
	bool m_bTrapHighlight;
	bool IsTrapHighlighted() {return m_bTrapHighlight;}
	EoInt16 m_nTrapHighlightPenColor;
	EoInt16 TrapHighlightPenColor() {return m_nTrapHighlightPenColor;}

	EoGePoint3d GetCursorPosition();
	void SetCursorPosition(EoGePoint3d pt);

#if defined(DWGDIRECT)
	ODCOLORREF m_background;
	const ODCOLORREF activeBackground() const {return m_background;}
	const ODCOLORREF* curPalette() const;
#endif // DWGDIRECT

	double ExtractedNumber() {return m_dExtractedNumber;}
	void ExtractedNumber(const double d) {m_dExtractedNumber = d;}
	CString ExtractedText() {return m_strExtractedText;}
	void ExtractedText(const CString& str) {m_strExtractedText = str;}

	void PenStylesLoad(const CString& strFileName);
	void PenColorsChoose();
	COLORREF PenColorsGetHot(EoInt16 nPenColor) {return (crHotCols[nPenColor]);} 
	void PenColorsLoad(const CString& strFileName);
	
	void PenWidthsLoad(const CString& strFileName);
	double PenWidthsGet(EoInt16 nPenColor) {return (dPWids[nPenColor]);}
	
	void HatchesLoad(const CString& strFileName);

	void InitGbls(CDC* deviceContext);

	double GetDimLen() {return (m_dDimLen);}
	double GetDimAngZ() {return (m_dDimAngZ);}
	bool GetEditCFImage() {return m_bEditCFImage;}
	bool GetEditCFGroups() {return m_bEditCFGroups;}
	bool GetEditCFText() {return m_bEditCFText;}
	double GetEngAngZ() {return (m_dEngAngZ);}
	double GetEngLen() {return (m_dEngLen);}
	double GetScale() {return m_dScale;}
	EoDb::Units GetUnits() {return (m_eUnits);}
	int	GetUnitsPrec() {return (m_iUnitsPrec);}
	
	EoGePoint3d HomePointGet(int i);
	void HomePointSave(int i, const EoGePoint3d& pt);
	
	void SetBackGround(COLORREF cr);
	void SetDimLen(double d) {m_dDimLen = d;}
	void SetDimAngZ(double d) {m_dDimAngZ = d;}
	void SetEngAngZ(double d) {m_dEngAngZ = d;}
	void SetEngLen(double d) {m_dEngLen = d;}
	void SetScale(double d) {if (d > DBL_EPSILON) m_dScale = d;}
	void SetUnits(EoDb::Units units) {m_eUnits = units;}
	void SetUnitsPrec(int iPrec) {if (iPrec > 0) m_iUnitsPrec = iPrec;}
	void LoadModeResources(int mode);

	void StatusLineDisplay(EStatusLineItem = All);

	char* StrokeFontGet() {return m_pStrokeFontDef;}
	void StrokeFontLoad(const CString& strPathName);
	void StrokeFontRelease();
	
private: // grid and axis constraints
	double m_AxisConstraintInfluenceAngle;
	double m_AxisConstraintOffsetAngle;
	bool m_DisplayGridWithLines;
	bool m_DisplayGridWithPoints;
	bool m_GridSnapIsOn;
	EoGePoint3d m_GridOrigin;
	int m_MaximumDotsPerLine;
	double m_XGridLineSpacing;
	double m_YGridLineSpacing;
	double m_ZGridLineSpacing;
	double m_XGridPointSpacing;
	double m_YGridPointSpacing;
	double m_ZGridPointSpacing;
	double m_XGridSnapSpacing;
	double m_YGridSnapSpacing;
	double m_ZGridSnapSpacing;

public:
	double AxisConstraintInfluenceAngle() {return m_AxisConstraintInfluenceAngle;}
	void AxisConstraintInfluenceAngle(double angle) {m_AxisConstraintInfluenceAngle = angle;}
	double AxisConstraintOffsetAngle() {return m_AxisConstraintOffsetAngle;}
	void AxisConstraintOffsetAngle(double angle) {m_AxisConstraintOffsetAngle = angle;}
	void SetAxisConstraintInfluenceAngle(double angle) {m_AxisConstraintInfluenceAngle = angle;}
	void InitializeConstraints();
	EoGePoint3d SnapPointToAxis(EoGePoint3d& beginPoint, EoGePoint3d& endPoint);
	EoGePoint3d	SnapPointToGrid(EoGePoint3d& pt);
	
	void DisplayGrid(AeSysView* view, CDC* deviceContext);
	bool DisplayGridWithLines() {return m_DisplayGridWithLines;}
	void DisplayGridWithLines(bool display) {m_DisplayGridWithLines = display;}
	void DisplayGridWithPoints(bool display) {m_DisplayGridWithPoints = display;}
	bool DisplayGridWithPoints() {return m_DisplayGridWithPoints;}
	bool IsGridSnapEnabled() {return m_GridSnapIsOn;}
	void GridSnapIsOn(bool snap) {m_GridSnapIsOn = snap;}

	EoGePoint3d GridOrign() {return (m_GridOrigin);}
	void GridOrign(EoGePoint3d& origin) {m_GridOrigin = origin;}
	void GetGridLineSpacing(double& x, double& y, double& z);
	void SetGridLineSpacing(double x, double y, double z);
	void GetGridPointSpacing(double& x, double& y, double& z);
	void SetGridPointSpacing(double x, double y, double z);
	void GetGridSnapSpacing(double& x, double& y, double& z);
	void SetGridSnapSpacing(double x, double y, double z);

	double	GetDialogItemDouble(HWND hDlg, int controlIndex);
	double	GetDialogItemUnitsText(HWND hDlg, int controlIndex);
	void	SetDialogItemDouble(HWND hDlg, int controlIndex, double value);
	void	SetDialogItemUnitsText(HWND hDlg, int controlIndex, double value);
	LRESULT GetCurrentListBoxSelectionText(HWND hDlg, int listIndex, CString& text);

public:
	static EoDb::FileTypes GetFileTypeFromPath(const CString& pathName);
	HINSTANCE GetInstance() {return (m_hInstance);}
	WNDPROC GetMainWndProc() {return (m_wpMainWnd);}	
	UINT CheckMenuItem(UINT uId, UINT uCheck) {return (::CheckMenuItem(m_hMenu, uId, uCheck));}
	HWND GetSafeHwnd() {return (AfxGetMainWnd()->GetSafeHwnd());}
	HMENU GetSubMenu(int nPos) {return (::GetSubMenu(m_hMenu, nPos));}
	UINT GetClipboardFormatEoGroups() 
	{
		return (m_nClipboardFormatEoGroups);
	}
	CString GetShadowDir() {return m_strShadowDir;}
	void SetShadowDir(const CString& strDir);

public:
	afx_msg void OnTrapCommandsHighlight();
	afx_msg void OnEditCfImage();
	afx_msg void OnEditCfGroups();
	afx_msg void OnEditCfText();
	afx_msg void OnFileRun();
	afx_msg void OnHelpContents();
	afx_msg void OnModeAnnotate();
	afx_msg void OnModeCut();
	afx_msg void OnModeDimension();
	afx_msg void OnModeDraw();
	afx_msg void OnModeDraw2();
	afx_msg void OnModeEdit();
	afx_msg void OnModeFixup();
	afx_msg void OnModeLPD();
	afx_msg void OnModeLetter();
	afx_msg void OnModeNodal();
	afx_msg void OnModePipe();
	afx_msg void OnModePower();
	afx_msg void OnModeRevise();
	afx_msg void OnModeRLPD();
	afx_msg void OnModeTrap();
	afx_msg void OnTrapCommandsAddGroups();
	afx_msg void OnViewModeInformation();
	afx_msg void OnViewStateInformation();

	afx_msg void OnUpdateModeAnnotate(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeCut(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeDimension(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeDraw(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeDraw2(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeEdit(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeFixup(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeLpd(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeNodal(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModePipe(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModePower(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeRlpd(CCmdUI *pCmdUI);
	afx_msg void OnUpdateModeTrap(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewModeinformation(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewStateinformation(CCmdUI *pCmdUI);
public:
	// Modifies the base accelerator table by defining the mode specific keys.
	void BuildModifiedAcceleratorTable(void);

	float m_AmbientLight[4];
	
	double m_L0Position[4];
	
	float m_L0Ambient[4];
	float m_L0Diffuse[4];
	float m_L0Specular[4];

	float m_MatAmbient[4];
	float m_MatDiffuse[4];
	float m_MatSpecular[4];

#if defined(GL_FUNCTIONALITY)
	HGLRC  hRC;
#endif // GL_FUNCTIONALITY

protected:
	DECLARE_MESSAGE_MAP()
};

extern AeSysApp app;

COLORREF AppGetTextCol();
