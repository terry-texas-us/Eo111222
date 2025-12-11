#include "stdafx.h"

#include "FindComboBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// FindComboBox

IMPLEMENT_SERIAL(FindComboBox, CMFCToolBarComboBoxButton, 1)

BOOL FindComboBox::m_HasFocus = FALSE;

BOOL FindComboBox::NotifyCommand(int notifyCode)
{
	ATLTRACE2(atlTraceGeneral, 0, L"FindComboBox::NotifyCommand(%i)\n", notifyCode);

	BOOL CommandProcessed = CMFCToolBarComboBoxButton::NotifyCommand(notifyCode);

	switch (notifyCode)
	{
	case CBN_KILLFOCUS:
		m_HasFocus = FALSE;

		CommandProcessed = TRUE;
		break;

	case CBN_SETFOCUS:
		{
			m_HasFocus = TRUE;

			CommandProcessed = TRUE;
			break;
		}
	}
	return CommandProcessed;
}