#include "stdafx.h"

#include "EoCtrlFindComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// EoCtrlFindComboBox

IMPLEMENT_SERIAL(EoCtrlFindComboBox, CMFCToolBarComboBoxButton, 1)

BOOL EoCtrlFindComboBox::m_HasFocus = FALSE;

BOOL EoCtrlFindComboBox::NotifyCommand(int notifyCode) {
  ATLTRACE2(atlTraceGeneral, 0, L"EoCtrlFindComboBox::NotifyCommand(%i)\n", notifyCode);

  BOOL CommandProcessed = CMFCToolBarComboBoxButton::NotifyCommand(notifyCode);

  switch (notifyCode) {
    case CBN_KILLFOCUS:
      m_HasFocus = FALSE;

      CommandProcessed = TRUE;
      break;

    case CBN_SETFOCUS: {
      m_HasFocus = TRUE;

      CommandProcessed = TRUE;
      break;
    }
  }
  return CommandProcessed;
}
