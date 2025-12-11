#pragma once

class EoMfWorkspaceDockablePane : public CDockablePane
{
public: // construction
	EoMfWorkspaceDockablePane();

protected: // Attributes
	CMFCPropertyGridCtrl m_wndPropList;

	enum PropertyId
	{
		kTabsStyle,
		kTabLocation,
		kTabsAutoColor,
		kTabIcons,
		kTabsBorderSize
	};

public: // Overrides

public: // Implementation
	virtual ~EoMfWorkspaceDockablePane(void);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT createStruct);
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnPaint(void);
	afx_msg LRESULT OnPropertyChanged(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

protected: // Operations
	void SetPropState(void);

public: // Operations
};
