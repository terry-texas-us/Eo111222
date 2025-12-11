#pragma once

// CDlgSetText dialog

class CDlgSetText : public CDialog
{
	DECLARE_DYNAMIC(CDlgSetText)

public:
	CDlgSetText(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetText();

// Dialog Data
	enum { IDD = IDD_SET_TEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog(void);

public:
	CString m_sText;
	CString m_strTitle;

protected:
	DECLARE_MESSAGE_MAP()
};
