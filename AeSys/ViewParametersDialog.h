#pragma once

// ViewParametersDialog dialog

class ViewParametersDialog : public CDialog
{
	DECLARE_DYNAMIC(ViewParametersDialog)

public:
	ViewParametersDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ViewParametersDialog();

// Dialog Data
	enum { IDD = IDD_VIEW_PARAMETERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_PerspectiveProjection;
	unsigned long m_ModelView;
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedApply();
	afx_msg void OnEnChangePositionX();
	afx_msg void OnEnChangePositionY();
	afx_msg void OnEnChangePositionZ();
	afx_msg void OnEnChangeTargetX();
	afx_msg void OnEnChangeTargetY();
	afx_msg void OnEnChangeTargetZ();
	afx_msg void OnEnChangeFrontClipDistance();
	afx_msg void OnEnChangeBackClipDistance();
	afx_msg void OnEnChangeLensLength();
	afx_msg void OnBnClickedPerspectiveProjection();
};
