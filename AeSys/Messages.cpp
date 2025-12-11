#include "stdafx.h"
#include "Mainfrm.h"
#include "AeSys.h"

int msgConfirm(UINT uiMsgId)
{
	WCHAR Message[256];

	::LoadStringW(app.GetInstance(), uiMsgId, Message, sizeof(Message) / sizeof(WCHAR));
	
	LPTSTR NextToken = NULL;
	LPTSTR pMsg = wcstok_s(Message, L"\t", &NextToken);
	LPTSTR pCap = wcstok_s(0, L"\n", &NextToken);
	
	return (MessageBoxW(0, pMsg, pCap, MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2));
}
int msgConfirm(UINT uiMsgId, const CString& strVal)
{
	WCHAR FormatSpecification[256];
	WCHAR Message[256];

	::LoadStringW(app.GetInstance(), uiMsgId, FormatSpecification, sizeof(FormatSpecification) / sizeof(WCHAR));
	
	swprintf_s(Message, 256, FormatSpecification, strVal);
	
	LPTSTR NextToken = NULL;
	LPTSTR pMsg = wcstok_s(Message, L"\t", &NextToken);
	LPTSTR pCap = wcstok_s(0, L"\n", &NextToken);
	
	return (MessageBoxW(0, pMsg, pCap, MB_ICONINFORMATION | MB_YESNOCANCEL | MB_DEFBUTTON2));
}
void msgWarning(UINT uiMsgId)
{
	WCHAR Message[256];

	::LoadStringW(app.GetInstance(), uiMsgId, Message, sizeof(Message) / sizeof(WCHAR));
	
	LPTSTR NextToken = NULL;
	LPTSTR pMsg = wcstok_s(Message, L"\t", &NextToken);
	LPTSTR pCap = wcstok_s(0, L"\n", &NextToken);
	
	MessageBoxW(0, pMsg, pCap, MB_ICONWARNING | MB_OK);
}
void msgWarning(UINT uiMsgId, const CString& strVal)
{
	WCHAR FormatSpecification[256];
	WCHAR Message[256];
	
	::LoadStringW(app.GetInstance(), uiMsgId, FormatSpecification, sizeof(FormatSpecification) / sizeof(WCHAR));
	
	swprintf_s(Message, 256, FormatSpecification, strVal);
	
	LPTSTR NextToken = NULL;
	LPTSTR pMsg = wcstok_s(Message, L"\t", &NextToken);
	LPTSTR pCap = wcstok_s(0, L"\n", &NextToken);
	
	MessageBoxW(0, pMsg, pCap, MB_ICONWARNING | MB_OK);
}
void msgInformation(const CString& message)
{
	CMainFrame* MainFrame = (CMainFrame*) (AfxGetMainWnd());
	
	MainFrame->GetOutputPane().AddStringToMessageList(message);
	if (!MainFrame->GetOutputPane().IsWindowVisible())
	{
		MainFrame->SetPaneText(1, message);
	}
}
void msgInformation(UINT stringResourceIdentifier)
{
	WCHAR Message[256];
	if (stringResourceIdentifier == 0)
	{
		wcscpy_s(Message, 256, L"AeSys");
		::LoadStringW(app.GetInstance(), app.m_Mode, Message, sizeof(Message) / sizeof(WCHAR));
		LPTSTR NextToken = NULL;
		wcstok_s(Message, L"\n", &NextToken);
	}
	else
	{
		::LoadStringW(app.GetInstance(), stringResourceIdentifier, Message, sizeof(Message) / sizeof(WCHAR));
	}
	msgInformation(Message);
}
void msgInformation(UINT stringResourceIdentifier, const CString& strVal)
{
	WCHAR FormatSpecification[256];
	WCHAR Message[256];

	::LoadStringW(app.GetInstance(), stringResourceIdentifier, FormatSpecification, sizeof(FormatSpecification) / sizeof(WCHAR));
	
	swprintf_s(Message, 256, FormatSpecification, strVal);
	
	msgInformation(Message);
}