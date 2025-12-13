#pragma once

class EoCtrlFindComboBox : public CMFCToolBarComboBoxButton {
  DECLARE_SERIAL(EoCtrlFindComboBox)

 public:
  EoCtrlFindComboBox()
      : CMFCToolBarComboBoxButton(ID_EDIT_FIND_COMBO, GetCmdMgr()->GetCmdImage(ID_EDIT_FIND), CBS_DROPDOWN) {}
  
  EoCtrlFindComboBox(const EoCtrlFindComboBox&) = delete;
  EoCtrlFindComboBox& operator=(const EoCtrlFindComboBox&) = delete;

 protected:
  static BOOL m_HasFocus;

 public:
  BOOL HasFocus() const override { return m_HasFocus; }

 protected:
  virtual BOOL NotifyCommand(int notifyCode);
};
