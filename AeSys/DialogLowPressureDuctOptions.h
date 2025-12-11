#pragma once

// DialogLowPressureDuctOptions dialog

class DialogLowPressureDuctOptions : public CDialog
{
	DECLARE_DYNAMIC(DialogLowPressureDuctOptions)

public:
	DialogLowPressureDuctOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~DialogLowPressureDuctOptions();

// Dialog Data
	enum { IDD = IDD_DLGPROC_LPD_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	double m_Width;
	double m_Depth;
	double m_RadiusFactor;
	bool m_GenerateVanes;
	int m_Justification;
	bool m_BeginWithTransition;
	
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedGenVanes();
	afx_msg void OnEnChangeWidth();
};
