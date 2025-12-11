#pragma once

// EoDlgSetupLineType dialog

class EoDlgSetupLineType : public CDialog {
	DECLARE_DYNAMIC(EoDlgSetupLineType)

public:
	EoDlgSetupLineType(CWnd* pParent = NULL);
	EoDlgSetupLineType(EoDbLineTypeTable* lineTypeTable, CWnd* pParent = NULL);
	virtual ~EoDlgSetupLineType();

// Dialog Data
	enum { IDD = IDD_SETUP_LINETYPE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	enum LineTypesListColumnLabels {
		Name,
		Appearance,
		Description
	};
	EoDbLineTypeTable* m_LineTypeTable;
	CListCtrl m_LineTypesListControl;

public:
	EoDbLineType* m_LineType;

	afx_msg void OnDrawItem(int controlIdentifier, LPDRAWITEMSTRUCT drawItemStruct);

protected:
	DECLARE_MESSAGE_MAP()
};