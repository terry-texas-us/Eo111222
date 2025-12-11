#pragma once

// EoDlgActiveViewKeyplan dialog

class EoDlgActiveViewKeyplan : public CDialog {
	DECLARE_DYNAMIC(EoDlgActiveViewKeyplan)

public:
	EoDlgActiveViewKeyplan(CWnd* pParent = NULL);
	EoDlgActiveViewKeyplan(AeSysView* view, CWnd* pParent = NULL);
	virtual ~EoDlgActiveViewKeyplan();

// Dialog Data
	enum { IDD = IDD_ACTIVE_VIEW_KEYPLAN };

	static HBITMAP	m_hbmKeyplan;
	static CRect	m_rcWnd;

	double m_dRatio;

protected:
	static bool	bKeyplan;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	AeSysView* m_ActiveView;
	void Refresh();

public:
	afx_msg void OnBnClickedRecall();
	afx_msg void OnBnClickedSave();
	afx_msg void OnEnKillfocusRatio();

protected:
	DECLARE_MESSAGE_MAP()
};

// User defined message sent by EoDlgActiveViewKeyplan to WndProcKeyPlan when a new zoom ratio is set using an edit control (LPARAM is the new value of ratio)
#define WM_USER_ON_NEW_RATIO WM_USER + 1

