#pragma once

// EoDlgFileManage dialog

class EoDlgFileManage : public CDialog
{
	DECLARE_DYNAMIC(EoDlgFileManage)

public:
	EoDlgFileManage(CWnd* pParent = NULL);   // standard constructor
	virtual ~EoDlgFileManage();

// Dialog Data
	enum { IDD = IDD_FILE_MANAGE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
public:
	CListBox m_BlocksList;
	CListBox m_LayerList;
	CListBox m_TracingList;
	CComboBox m_PenColorList;
	CComboBox m_LineTypeList;
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	void OdaDrawEntire(LPDRAWITEMSTRUCT lpDrawItemStruct, int inflate);
	void OdaSelect(LPDRAWITEMSTRUCT lpDrawItemStruct, int inflate);
	void OdaFocus(LPDRAWITEMSTRUCT lpDrawItemStruct, int inflate);
	afx_msg void OnCbnSelchangeLinetype();
	afx_msg void OnCbnSelchangeColorId();
};
void	DlgProcFileManageDoLayerActive(HWND);
void	DlgProcFileManageDoLayerDelete(HWND);
void	DlgProcFileManageDoLayerHidden(HWND);
void	DlgProcFileManageDoLayerMelt(HWND);
void	DlgProcFileManageDoLayerRename(HWND);
void	DlgProcFileManageDoLayerStatic(HWND);
void	DlgProcFileManageDoLayerWork(HWND);
void	DlgProcFileManageDoTracingCloak(HWND);
void	DlgProcFileManageDoTracingExclude(HWND);
void	DlgProcFileManageDoTracingFuse(HWND);
void	DlgProcFileManageDoTracingInclude(HWND);
void	DlgProcFileManageDoTracingMap(HWND);
void	DlgProcFileManageDoTracingOpen(HWND);
void	DlgProcFileManageDoTracingView(HWND);

void	DlgProcFileManageInit(HWND);
