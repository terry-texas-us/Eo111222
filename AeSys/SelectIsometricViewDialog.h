#pragma once

// SelectIsometricViewDialog dialog

class SelectIsometricViewDialog : public CDialog
{
	DECLARE_DYNAMIC(SelectIsometricViewDialog)

public:
	SelectIsometricViewDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~SelectIsometricViewDialog();

// Dialog Data
	enum { IDD = IDD_SELECT_ISOMETRIC_VIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	int m_LeftRight;
	int m_FrontBack;
	int m_AboveUnder;
};
