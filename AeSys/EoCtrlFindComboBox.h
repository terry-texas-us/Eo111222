#pragma once

class EoCtrlFindComboBox : public CMFCToolBarComboBoxButton {
	DECLARE_SERIAL(EoCtrlFindComboBox)

public: // Construction
	EoCtrlFindComboBox() : CMFCToolBarComboBoxButton(ID_EDIT_FIND_COMBO, GetCmdMgr()->GetCmdImage(ID_EDIT_FIND), CBS_DROPDOWN) {
	}

protected: // Attributes
	static BOOL m_HasFocus;

public:
	static BOOL HasFocus() {
		return m_HasFocus;
	}
// Overrides
protected:
	virtual BOOL NotifyCommand(int notifyCode);
};

