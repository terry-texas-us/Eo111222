#pragma once

// CDlgSetLength dialog

class CDlgSetLength : public CDialog
{
	DECLARE_DYNAMIC(CDlgSetLength)

public:
	CDlgSetLength(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetLength();

// Dialog Data
	enum { IDD = IDD_SET_LENGTH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
public:
	double	m_dLength;
	CString m_strTitle;

protected:
	DECLARE_MESSAGE_MAP()
};