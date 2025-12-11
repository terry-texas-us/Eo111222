#pragma once

// EoDlgSetLength dialog

class EoDlgSetLength : public CDialog {
	DECLARE_DYNAMIC(EoDlgSetLength)

public:
	EoDlgSetLength(CWnd* pParent = NULL);
	virtual ~EoDlgSetLength();

// Dialog Data
	enum { IDD = IDD_SET_LENGTH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
public:
	double	m_dLength;
	CString m_strTitle;

protected:
	DECLARE_MESSAGE_MAP()
};