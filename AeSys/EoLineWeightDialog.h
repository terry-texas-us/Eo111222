#pragma once

// EoLineWeightDialog dialog

class EoLineWeightDialog : public CDialog
{
	DECLARE_DYNAMIC(EoLineWeightDialog)

public:
	EoLineWeightDialog(CWnd* parent = NULL);   // standard constructor
	EoLineWeightDialog(int originalLineWeight, CWnd* parent = NULL);
	virtual ~EoLineWeightDialog();

// Dialog Data
	enum { IDD = IDD_LINEWEIGHT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog(void);
private:
	int m_OriginalLineWeight;
public:
	CListBox m_LineWeightList;
	OdDb::LineWeight m_LineWeight;
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnLbnDblclkListLineweight();
};
