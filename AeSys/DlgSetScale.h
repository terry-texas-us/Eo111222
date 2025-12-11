#pragma once

// CDlgSetScale dialog

class CDlgSetScale : public CDialog
{
	DECLARE_DYNAMIC(CDlgSetScale)

public:
	CDlgSetScale(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetScale();

// Dialog Data
	enum { IDD = IDD_SET_SCALE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
	double m_Scale;

protected:
	DECLARE_MESSAGE_MAP()
};
