#pragma once

// CDlgViewZoom dialog

// User defined message sent by CDlgViewZoom to WndProcKeyPlan when a new
// zoom ratio is set using an edit control
//		LPARAM is the new value of ratio
#define WM_USER_ON_NEW_RATIO WM_USER + 1

class CDlgViewZoom : public CDialog
{
	DECLARE_DYNAMIC(CDlgViewZoom)

public:
	CDlgViewZoom(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgViewZoom();

// Dialog Data
	enum { IDD = IDD_VIEW_ZOOM };
	
	static HBITMAP	m_hbmKeyplan;
	static CRect	m_rcWnd;

	double m_dRatio;

protected:
	static bool		bKeyplan;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	void Refresh();
	

public:
	afx_msg void OnBnClickedBest();
	afx_msg void OnBnClickedLast();
	afx_msg void OnBnClickedMoreless();
	afx_msg void OnBnClickedNormal();
	afx_msg void OnBnClickedOverview();
	afx_msg void OnBnClickedPan();
	afx_msg void OnBnClickedRecall();
	afx_msg void OnBnClickedSave();
	afx_msg void OnEnChangeRatio();

protected:
	DECLARE_MESSAGE_MAP()
};
