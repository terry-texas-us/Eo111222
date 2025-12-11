#pragma once

int msgConfirm(UINT uiMsgId);
int msgConfirm(UINT uiMsgId, const CString& strVal);

void msgInformation(const CString& message);
void msgInformation(UINT stringResourceIdentifier);
void msgInformation(UINT stringResourceIdentifier, const CString& strVal);

void msgWarning(UINT uiMsgId);
void msgWarning(UINT uiMsgId, const CString& strVal);
