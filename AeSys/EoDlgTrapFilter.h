#pragma once

// EoDlgTrapFilter dialog

class EoDlgTrapFilter : public CDialog {
	DECLARE_DYNAMIC(EoDlgTrapFilter)

public:
	EoDlgTrapFilter(CWnd* pParent = NULL);
	EoDlgTrapFilter(AeSysDoc* document, CWnd* pParent = NULL);
	virtual ~EoDlgTrapFilter();

// Dialog Data
	enum { IDD = IDD_TRAP_FILTER };

	AeSysDoc* m_Document;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();

public:
	CComboBox m_FilterLineComboBoxControl;
	CListBox m_FilterPrimitiveTypeListBoxControl;

	void FilterByColor(EoInt16 colorIndex);
	void FilterByLineType(int lineType);
	void FilterByPrimitiveType(const EoDb::PrimitiveTypes primitiveType);

protected:
	DECLARE_MESSAGE_MAP()
};
