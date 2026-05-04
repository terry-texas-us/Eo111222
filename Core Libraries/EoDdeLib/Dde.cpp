
#include <cwchar>
#include <cwctype>
#include <new>

#include "DdeStringUtil.h"
#include "dde.h"
#include "ddeSys.h"

using namespace dde;

// Standard format name lookup table
// String names for standard Windows Clipboard formats
CFTAGNAME dde::CFNames[] = {CF_TEXT,
    L"TEXT",
    CF_BITMAP,
    L"BITMAP",
    CF_METAFILEPICT,
    L"METAFILEPICT",
    CF_SYLK,
    L"SYLK",
    CF_DIF,
    L"DIF",
    CF_TIFF,
    L"TIFF",
    CF_OEMTEXT,
    L"OEMTEXT",
    CF_DIB,
    L"DIB",
    CF_PALETTE,
    L"PALETTE",
    CF_PENDATA,
    L"PENDATA",
    CF_RIFF,
    L"RIFF",
    CF_WAVE,
    L"WAVE",
    0,
    0};
// Local data
SERVERINFO dde::ServerInfo;
// Format lists
std::uint16_t dde::SysFormatList[] = {CF_UNICODETEXT, 0};
std::uint16_t dde::MyFormats[] = {CF_UNICODETEXT, 0};

/// @brief Initialize DDE server infrastructure: DDEML, service name, and system topic.
/// App-specific topic/item/command registrations should be done by the caller after this returns true.
/// @param serviceName The DDE service name to register.
/// @param pfnInitError Callback invoked on initialization failure (may be nullptr to silently fail).
/// @param hMainWindow Main window handle passed to pfnInitError for fatal shutdown.
/// @param pfnFallbackExec Fallback handler for execute transactions on topics with no exec function and no command list
/// (may be nullptr).
/// @return true if initialization succeeded, false on DDEML error.
bool dde::Initialize(const wchar_t* serviceName,
    PINITERRORFN pfnInitError,
    HWND hMainWindow,
    PFALLBACKEXECFN pfnFallbackExec) {
  if (ServerInfo.dwInstance != 0) { return true; }  // already initialized

  ServerInfo.pfnStdCallback = reinterpret_cast<PFNCALLBACK>(StdCallback);
  ServerInfo.pfnCustomCallback = 0;
  ServerInfo.pfnFallbackExec = pfnFallbackExec;

  DWORD dwFilterFlags = CBF_FAIL_SELFCONNECTIONS;

  UINT uiResult = DdeInitialize(&ServerInfo.dwInstance, ServerInfo.pfnStdCallback, dwFilterFlags, 0L);
  if (uiResult != DMLERR_NO_ERROR) {
    if (pfnInitError) { pfnInitError(serviceName, hMainWindow); }
    return false;
  }
  ServerInfo.lpszServiceName = serviceName;
  ServerInfo.hszServiceName = DdeCreateStringHandle(ServerInfo.dwInstance, serviceName, CP_WINUNICODE);
  if (!ServerInfo.hszServiceName) {  // failed to intern the service name string
    DdeUninitialize(ServerInfo.dwInstance);
    ServerInfo.dwInstance = 0;
    if (pfnInitError) { pfnInitError(serviceName, hMainWindow); }
    return false;
  }

  // Register the name of the service
  if (!DdeNameService(ServerInfo.dwInstance, ServerInfo.hszServiceName, (HSZ)0, DNS_REGISTER)) {
    DdeFreeStringHandle(ServerInfo.dwInstance, ServerInfo.hszServiceName);
    ServerInfo.hszServiceName = 0;
    ServerInfo.lpszServiceName = nullptr;
    DdeUninitialize(ServerInfo.dwInstance);
    ServerInfo.dwInstance = 0;
    if (pfnInitError) { pfnInitError(serviceName, hMainWindow); }
    return false;
  }

  // Add system topic items (library-portable)
  ItemAdd(SZDDESYS_TOPIC, SZDDESYS_ITEM_FORMATS, SysFormatList, SysReqFormats, 0);
  ItemAdd(SZDDESYS_TOPIC, SZDDESYS_ITEM_HELP, SysFormatList, SysReqHelp, 0);
  ItemAdd(SZDDESYS_TOPIC, SZDDESYS_ITEM_SYSITEMS, SysFormatList, SysReqItems, 0);
  ItemAdd(SZDDESYS_TOPIC, SZDDE_ITEM_ITEMLIST, SysFormatList, SysReqItems, 0);
  ItemAdd(SZDDESYS_TOPIC, SZDDESYS_ITEM_TOPICS, SysFormatList, SysReqTopics, 0);
  ItemAdd(SZDDESYS_TOPIC, L"Protocols", SysFormatList, SysReqProtocols, 0);

  return true;
}
/// @brief Tidy up and close down DDEML.
void dde::Uninitialize() {
  // Unregister the service name
  DdeNameService(ServerInfo.dwInstance, ServerInfo.hszServiceName, 0, DNS_UNREGISTER);
  // Free the name handle
  DdeFreeStringHandle(ServerInfo.dwInstance, ServerInfo.hszServiceName);
  ServerInfo.hszServiceName = 0;
  ServerInfo.lpszServiceName = nullptr;

  // Remove all topics (frees HSZ handles, items, commands, and conversations)
  while (ServerInfo.pTopicList) { TopicRemove(ServerInfo.pTopicList->pszTopicName); }

  // Release DDEML
  DdeUninitialize(ServerInfo.dwInstance);
  ServerInfo.dwInstance = 0;

  // Clear callback pointers
  ServerInfo.pfnCustomCallback = 0;
  ServerInfo.pfnStdCallback = 0;
  ServerInfo.pfnFallbackExec = nullptr;
}
// DDE callback function called from DDEML
HDDEDATA WINAPI
dde::StdCallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData, DWORD dwData1, DWORD dwData2) {
  HDDEDATA hDdeData = 0;

  switch (wType) {
    case XTYP_CONNECT_CONFIRM:  // Confirm that a conversation has been established with a
      // client and provide the server with the conversation handle
      ConversationAdd(hConv, hsz1);
      break;

    case XTYP_DISCONNECT:  // Remove a conversation from the list
      ConversationRemove(hConv, hsz1);
      break;

    case XTYP_WILDCONNECT:  // Sent when service name and/or topic name is nullptr
      if ((hsz2 == nullptr) || !DdeCmpStringHandles(hsz2, ServerInfo.hszServiceName)) { return (DoWildConnect(hsz1)); }
      break;

      // For all other messages we see if we want them here and if not,
      // they get passed on to the user callback if one is defined.

    case XTYP_ADVSTART:
    case XTYP_CONNECT:
    case XTYP_EXECUTE:
    case XTYP_REQUEST:
    case XTYP_ADVREQ:
    case XTYP_ADVDATA:
    case XTYP_POKE:

      // Try and process them here first.
      if (DoCallback(wType, wFmt, hConv, hsz1, hsz2, hData, &hDdeData)) { return hDdeData; }

      // Fall Through to allow the custom callback a chance
    default:
      if (ServerInfo.pfnCustomCallback != 0) {
        return (ServerInfo.pfnCustomCallback(wType, wFmt, hConv, hsz1, hsz2, hData, dwData1, dwData2));
      }
  }
  return (HDDEDATA)0;
}
// @brief Process a generic callback.
bool dde::DoCallback(UINT wType,
    UINT wFmt,
    HCONV hConv,
    HSZ hszTopic,
    HSZ hszItem,
    HDDEDATA hData,
    HDDEDATA* phReturnData) {
  PTOPICINFO pTopic = TopicFind(hszTopic);

  if (!pTopic) {  // Don't know the topic
    return false;
  }

  if (wType == XTYP_EXECUTE) {  // Execute request for the topic
    if (pTopic->pfnExec) {  // User supplied a function to handle this
      if ((*pTopic->pfnExec)(wFmt, hszTopic, hData)) {  // Call the exec function to process it
        *phReturnData = (HDDEDATA)(DWORD)DDE_FACK;
        return true;
      }
    } else if (pTopic->pCmdList) {  // Try to parse and execute the request
      if (ProcessExecRequest(pTopic, hData)) {
        *phReturnData = (HDDEDATA)(DWORD)DDE_FACK;
        return true;
      }
    } else {
      if (ServerInfo.pfnFallbackExec) { ServerInfo.pfnFallbackExec(hData); }
      *phReturnData = (HDDEDATA)(DWORD)DDE_FACK;
      return true;
    }
    *phReturnData = (HDDEDATA)DDE_FNOTPROCESSED;  // Either no function or it didn't get handled by the function
    return true;
  }
  if (wType == XTYP_CONNECT) {  // Connect request.
    *phReturnData = (HDDEDATA)(DWORD)TRUE;  // Conversation established
    return true;
  }
  // For any other transaction we need to be sure this is an item we support and in some cases,
  // that the format requested is supported for that item.

  PITEMINFO pItem = ItemFind(pTopic, hszItem);
  if (!pItem) {  // Not an item we support
    return false;
  }

  // See if this is a supported format

  LPWORD pFormat = pItem->pFormatList;
  while (*pFormat) {
    if (*pFormat == wFmt) { break; }
    pFormat++;
  }
  if (!*pFormat) {  // not a format we support
    return false;
  }

  PPOKEFN pfnPoke;
  CONVINFO ci{};
  PREQUESTFN pfnRequest;

  switch (wType) {
    case XTYP_ADVSTART:  // Start an advise request.  Topic/item and format are ok.
      *phReturnData = (HDDEDATA)(DWORD)TRUE;
      break;

    case XTYP_POKE:  // We did the data change ourself.
    case XTYP_ADVDATA:  // It came from elsewhere.
      *phReturnData = (HDDEDATA)DDE_FNOTPROCESSED;
      pfnPoke = pItem->pfnPoke;
      if (!pfnPoke) {  // No poke function for this item
        pfnPoke = pTopic->pfnPoke;  // Use poke function for this topic in general.
      }
      if (pfnPoke) {
        if ((*pfnPoke)(wFmt, hszTopic, hszItem, hData)) {  // Data at the server has changed.
          ci.cb = sizeof(CONVINFO);
          if (DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci)) {
            if (!(ci.wStatus & ST_ISSELF)) {  // It came from elsewhere (not a poke)
              DdePostAdvise(ServerInfo.dwInstance, hszTopic, hszItem);  // Advise ourself of the change.
            }
          }
          *phReturnData = (HDDEDATA)(DWORD)DDE_FACK;  // Say we took it
        }
      }
      break;

    case XTYP_ADVREQ:
    case XTYP_REQUEST:  // Attempt to start an advise or get the data on a topic/item
      // See if we have a request function for this item or
      // a generic one for the topic
      pfnRequest = pItem->pfnRequest;
      if (!pfnRequest) { pfnRequest = pTopic->pfnRequest; }
      if (pfnRequest) {
        *phReturnData = (*pfnRequest)(wFmt, hszTopic, hszItem);
      } else {
        *phReturnData = (HDDEDATA)0;
      }
      break;

    default:
      break;
  }
  // Say we processed the transaction in some way
  return true;
}
/// @brief Add a list of formats to main list ensuring that each item only exists in the list once.
void dde::AddFormatsToList(LPWORD pMain, int iMax, LPWORD pList) {
  LPWORD pFmt;

  if (!pMain || !pList) { return; }

  int iCount{};
  LPWORD pLast = pMain;

  while (*pLast) {  // Count what we have to start with
    pLast++;
    iCount++;
  }
  while ((iCount < iMax - 1) && *pList) {  // Walk the new list adding unique items if there is room (reserve one slot for terminator)
    pFmt = pMain;
    while (*pFmt) {
      if (*pFmt == *pList) {  // Already got this one
        goto next_fmt;
      }

      pFmt++;
    }
    *pLast++ = *pList;  // Put it on the end of the list
    iCount++;

  next_fmt:
    pList++;
  }
  *pLast = 0;  // Stick a null on the end to terminate the list
}
/// @brief
/// Process a wild connect request. Since we only support one service, this is much simpler.
/// If hszTopic is 0 we supply a list of all the topics we currently support.  If it's not 0,
/// we supply a list of topics (zero or one items) which match the requested topic.
/// The list is terminated by a 0 entry.
///
HDDEDATA dde::DoWildConnect(HSZ hszTopic) {
  PTOPICINFO pTopic;
  HDDEDATA hData;
  PHSZPAIR pHszPair;

  int iTopics = 0;

  if (hszTopic == 0) {  // Count all the topics we have
    pTopic = ServerInfo.pTopicList;
    while (pTopic) {
      iTopics++;
      pTopic = pTopic->pNext;
    }

  } else {  // Look for specific topic
    pTopic = ServerInfo.pTopicList;
    while (pTopic) {
      if (DdeCmpStringHandles(pTopic->hszTopicName, hszTopic) == 0) {
        iTopics++;
        break;
      }
      pTopic = pTopic->pNext;
    }
  }
  if (!iTopics) {  // No match or no topics at all. No Connect.
    return (HDDEDATA)0;
  }

  // Big enough for all the HSZPAIRS we'll be sending back plus space for a 0 entry on the end
  hData = DdeCreateDataHandle(ServerInfo.dwInstance, 0, (iTopics + 1) * sizeof(HSZPAIR), 0, (HSZ)0, 0, 0);

  if (!hData) {  // Failed to create mem object!
    return (HDDEDATA)0;
  }

  pHszPair = (PHSZPAIR)DdeAccessData(hData, 0);
  if (!pHszPair) {  // DdeAccessData failed — release the data handle and bail
    DdeFreeDataHandle(hData);
    return (HDDEDATA)0;
  }
  if (hszTopic == 0) {  // all the topics (includes the system topic)
    pTopic = ServerInfo.pTopicList;
    while (pTopic) {
      pHszPair->hszSvc = ServerInfo.hszServiceName;
      pHszPair->hszTopic = pTopic->hszTopicName;
      pHszPair++;
      pTopic = pTopic->pNext;
    }
  } else {  // the one topic asked for
    pHszPair->hszSvc = ServerInfo.hszServiceName;
    pHszPair->hszTopic = hszTopic;
    pHszPair++;
  }
  pHszPair->hszSvc = 0;  // Terminator on the end
  pHszPair->hszTopic = 0;
  DdeUnaccessData(hData);

  return hData;
}
PEXECCMDFNINFO dde::ExecCmdAdd(const wchar_t* pszTopic,
    const wchar_t* pszCmdName,
    PEXECCMDFN pExecCmdFn,
    UINT uiMinArgs,
    UINT uiMaxArgs) {
  PEXECCMDFNINFO pCmd = 0;

  PTOPICINFO pTopic = TopicFind(pszTopic);

  if (!pTopic) {  // Do Not already have this topic.\tWe need to add this as a new topic
    pTopic = TopicAdd(pszTopic, 0, 0, 0);
  }

  if (!pTopic) {
    return 0;  // failed
  }

  pCmd = ExecCmdFind(pTopic, pszCmdName);

  if (pCmd) {  // Already have this command.  Just update the info in it
    pCmd->pFn = pExecCmdFn;
    pCmd->uiMinArgs = uiMinArgs;
    pCmd->uiMaxArgs = uiMaxArgs;
  } else {  // New command item
    pCmd = new (std::nothrow) EXECCMDFNINFO{};
    if (!pCmd) { return 0; }

    pCmd->pszCmdName = pszCmdName;
    pCmd->pTopic = pTopic;
    pCmd->pFn = pExecCmdFn;
    pCmd->uiMinArgs = uiMinArgs;
    pCmd->uiMaxArgs = uiMaxArgs;
    // Add it to the existing cmd list for this topic
    pCmd->pNext = pTopic->pCmdList;
    pTopic->pCmdList = pCmd;

    // If this was the first command added to the list, add the 'Result' command too.
    // This supports the Execute Control 1 protocol.
    // Guard with ExecCmdFind so a caller-registered 'Result' handler is never overwritten.
    if (!ExecCmdFind(pTopic, L"Result")) {
      ExecCmdAdd(pszTopic, L"Result", SysResultExecCmd, 1, 1);
    }
  }
  return pCmd;
}
/// @brief Find a DDE execute command from its string name.
PEXECCMDFNINFO dde::ExecCmdFind(PTOPICINFO pTopic, const wchar_t* lpszCmd) {
  PEXECCMDFNINFO pCmd = pTopic->pCmdList;

  while (pCmd) {
    if (_wcsicmp(pCmd->pszCmdName, lpszCmd) == 0) { break; }

    pCmd = pCmd->pNext;
  }
  return pCmd;
}
bool dde::ExecCmdRemove(const wchar_t* pszTopic, const wchar_t* pszCmdName) {
  PTOPICINFO pTopic = TopicFind(pszTopic);

  if (!pTopic) {  // See if we have this topic
    return false;
  }

  PEXECCMDFNINFO pPrevCmd = 0;
  PEXECCMDFNINFO pCmd = pTopic->pCmdList;

  while (pCmd) {  // Walk the topic item list looking for this cmd.
    if (_wcsicmp(pCmd->pszCmdName, pszCmdName) == 0) {  // Found it.  Unlink it from the list.
      if (pPrevCmd) {
        pPrevCmd->pNext = pCmd->pNext;
      } else {
        pTopic->pCmdList = pCmd->pNext;
      }

      // Free the memory associated with it
      delete pCmd;
      return true;
    }
    pPrevCmd = pCmd;
    pCmd = pCmd->pNext;
  }
  return false;  // We don't have that one
}
/// @brief Get the text name of a Clipboard format from its id
wchar_t* dde::GetCFNameFromId(std::uint16_t wFmt, wchar_t* lpBuf, int iSize) {
  PCFTAGNAME pCTN = CFNames;

  // Try for a standard one first
  while (pCTN->wFmt) {
    if (pCTN->wFmt == wFmt) {
      wcsncpy_s(lpBuf, iSize, pCTN->pszName, _TRUNCATE);
      return lpBuf;
    }
    pCTN++;
  }
  if (GetClipboardFormatName(wFmt, lpBuf, iSize) == 0) {  // It's unknown (not registered)
    *lpBuf = '\0';
  }

  return lpBuf;
}

// Return a string in CF_UNICODETEXT format
HDDEDATA dde::MakeCFText(UINT wFmt, const wchar_t* string, HSZ hszItem) {
  if (wFmt != CF_UNICODETEXT) { return 0; }

  return (DdeCreateDataHandle(ServerInfo.dwInstance,
      (LPBYTE)string,
      (static_cast<DWORD>(wcslen(string)) + 1) * sizeof(wchar_t),
      0,
      hszItem,
      CF_UNICODETEXT,
      0));
}
/// @brief Create a data item containing the names of all the formats supplied in a list.
// Returns: A DDE data handle to a list of the format names.
HDDEDATA dde::MakeDataFromFormatList(LPWORD pFmt, std::uint16_t wFmt, HSZ hszItem) {
  int cb;
  wchar_t Buffer[256]{};

  HDDEDATA hData = DdeCreateDataHandle(ServerInfo.dwInstance, 0, 0, 0, hszItem, wFmt, 0);  // Empty data object to fill
  if (!hData) { return 0; }
  int cbOffset = 0;

  while (*pFmt) {  // Walk the format list
    if (cbOffset != 0) {  // Put in a tab delimiter
      DdeAddData(hData, (LPBYTE)L"\t", sizeof(wchar_t), cbOffset);
      cbOffset += sizeof(wchar_t);
    }
    GetCFNameFromId(*pFmt, Buffer, sizeof(Buffer) / sizeof(wchar_t));  // the string name of the format
    cb = static_cast<int>(wcslen(Buffer)) * static_cast<int>(sizeof(wchar_t));
    DdeAddData(hData, (LPBYTE)Buffer, cb, cbOffset);
    cbOffset += cb;
    pFmt++;
  }
  DdeAddData(hData, (LPBYTE)L"", sizeof(wchar_t), cbOffset);  // Put a null terminator on the end
  return hData;
}
/// @brief Post an advise request about an item
void dde::PostAdvise(PITEMINFO pItemInfo) {
  if (pItemInfo && pItemInfo->pTopic) {
    DdePostAdvise(ServerInfo.dwInstance, pItemInfo->pTopic->hszTopicName, pItemInfo->hszItemName);
  }
}
// DDE Execute command parser

// Return the narrow label string used in structured error replies.
const wchar_t* dde::ParseResultLabel(ParseResult r) noexcept {
  switch (r) {
    case ParseResult::Ok:
      return L"Ok";
    case ParseResult::MissingOpenBracket:
      return L"MissingOpenBracket";
    case ParseResult::UnknownCommand:
      return L"UnknownCommand";
    case ParseResult::OpTableOverflow:
      return L"OpTableOverflow";
    case ParseResult::TooManyArguments:
      return L"TooManyArguments";
    case ParseResult::TooFewArguments:
      return L"TooFewArguments";
    case ParseResult::MissingCloseParen:
      return L"MissingCloseParen";
    case ParseResult::MissingCloseBracket:
      return L"MissingCloseBracket";
  }
  return L"Unknown";
}
/// @brief Process a DDE execute command line.
// Notes:\tSupport for the 'Execute Control 1' protocol is provided allowing
//\t\treturn information to be sent back to the caller.
// Returns: true if no errors occur in parsing or executing the commands.
//\t\tfalse if any error occurs.
bool dde::ProcessExecRequest(PTOPICINFO pTopic, HDDEDATA hData) {
  bool bResult = false;
  POP OpTable[MAXOPS];
  PPOP ppOp, ppArg;
  UINT uiNargs;
  wchar_t* pArgBuf = 0;
  PCONVERSATIONINFO pCI;
  wchar_t szResult[MAXRESULTSIZE];

  if (!hData) { return false; }

  wchar_t* pData = reinterpret_cast<wchar_t*>(DdeAccessData(hData, 0));

  if (!pData) { return false; }

  // Allocate double required size we might need so we can avoid doing any space tests.
  auto argumentBufferSize = 2 * wcslen(pData) + 2;
  pArgBuf = new (std::nothrow) wchar_t[argumentBufferSize];
  if (!pArgBuf) { goto PER_exit; }

  ::ZeroMemory(pArgBuf, argumentBufferSize * sizeof(wchar_t));
  pCI = ConversationFind(pTopic->hszTopicName);  // Get a pointer to the current conversation

  while (pData && *pData) {  // Parse and execute each command in turn
    const ParseResult pr = ParseCmd(&pData, pTopic, OpTable, MAXOPS, pArgBuf);

    if (pr != ParseResult::Ok) {
      // Build a structured error string: "ERROR:reason"
      swprintf_s(szResult, MAXRESULTSIZE, L"ERROR:%s", ParseResultLabel(pr));
      if (pCI && pCI->pResultItem) {
        if (pCI->pResultItem->hData) { DdeFreeDataHandle(pCI->pResultItem->hData); }
        pCI->pResultItem->hData = DdeCreateDataHandle(ServerInfo.dwInstance,
            (LPBYTE)szResult,
            (static_cast<DWORD>(wcslen(szResult)) + 1) * sizeof(wchar_t),
            0,
            pCI->pResultItem->hszItemName,
            CF_UNICODETEXT,
            0);
      }
      goto PER_exit;
    }
    ppOp = OpTable;

    while (*ppOp) {  // Execute the op list
      uiNargs = 0;
      ppArg = ppOp + 1;
      while (*ppArg) {  // Count the number of args
        uiNargs++;
        ppArg++;
      }
      ppArg = ppOp + 1;  // Call the function, passing the address of the first arg
      szResult[0] = L'\0';
      bResult = (*((PEXECCMDFN)*ppOp))(pTopic, szResult, MAXRESULTSIZE, uiNargs, (wchar_t**)ppArg);

      // If the handler left the result buffer empty, synthesise "OK" so that the
      // Python client always receives a non-empty response string.
      if (szResult[0] == L'\0') { wcsncpy_s(szResult, MAXRESULTSIZE, L"OK", _TRUNCATE); }

      if (pCI && pCI->pResultItem) {
        // Write whatever the handler produced (or the synthesised "OK") back to
        // the result item so it is visible to a DDE Request immediately after.
        if (pCI->pResultItem->hData) { DdeFreeDataHandle(pCI->pResultItem->hData); }
        pCI->pResultItem->hData = DdeCreateDataHandle(ServerInfo.dwInstance,
            (LPBYTE)szResult,
            (static_cast<DWORD>(wcslen(szResult)) + 1) * sizeof(wchar_t),
            0,
            pCI->pResultItem->hszItemName,
            CF_UNICODETEXT,
            0);
      }

      if (!bResult) { goto PER_exit; }

      while (*ppOp) {  // move on to the next function
        ppOp++;
      }
      ppOp++;
    }
  }
  bResult = true;  // if we get this far we're done

PER_exit:

  DdeUnaccessData(hData);
  if (pArgBuf) { delete[] pArgBuf; }

  return bResult;
}
/// @brief Parses a single DDE command from the execute stream.
/// @param ppszCmdLine Pointer to current scan position; advanced past the parsed command on success.
/// @param pTopic      Topic whose command list is searched.
/// @param pOpTable    Caller-supplied op table; filled with fn-pointer then arg pointers then 0.
/// @param uiOpTableSize Capacity of pOpTable in POP slots.
/// @param pArgBuf     Caller-supplied scratch buffer; argument strings are written here.
/// @return ParseResult::Ok on success; a specific failure code otherwise.
dde::ParseResult dde::ParseCmd(wchar_t** ppszCmdLine,
    PTOPICINFO pTopic,
    PPOP pOpTable,
    UINT uiOpTableSize,
    wchar_t* pArgBuf) {
  wchar_t* pArg;
  PPOP ppOp = pOpTable;
  PEXECCMDFNINFO pExecFnInfo;
  UINT uiNargs;
  wchar_t cTerm = L'\0';

  *ppOp = 0;
  wchar_t* pCmd = dde::SkipWhiteSpace(*ppszCmdLine);

  if (!dde::ScanForChar(L'[', &pCmd)) { return ParseResult::MissingOpenBracket; }

  pExecFnInfo = ScanForCommand(pTopic->pCmdList, &pCmd);
  if (!pExecFnInfo) { return ParseResult::UnknownCommand; }

  if (static_cast<UINT>(ppOp - pOpTable) + 2 > uiOpTableSize) { return ParseResult::OpTableOverflow; }

  *ppOp++ = pExecFnInfo->pFn;

  uiNargs = 0;
  if (dde::ScanForChar(L'(', &pCmd)) {
    do {
      if (static_cast<UINT>(ppOp - pOpTable) + 2 > uiOpTableSize) { return ParseResult::TooManyArguments; }
      pArg = dde::ScanForString(&pCmd, &cTerm, &pArgBuf);
      if (pArg) {
        *ppOp++ = pArg;
        uiNargs++;
      }
    } while (cTerm == L',');

    if ((cTerm != L')') && (!dde::ScanForChar(L')', &pCmd))) { return ParseResult::MissingCloseParen; }
  }
  if (!dde::ScanForChar(L']', &pCmd)) { return ParseResult::MissingCloseBracket; }

  if (uiNargs < pExecFnInfo->uiMinArgs) { return ParseResult::TooFewArguments; }
  if (uiNargs > pExecFnInfo->uiMaxArgs) { return ParseResult::TooManyArguments; }

  *ppOp++ = 0;
  pCmd = dde::SkipWhiteSpace(pCmd);
  *ppOp = 0;
  *ppszCmdLine = pCmd;

  return ParseResult::Ok;
}
/// @brief Process a DDE Execute 'Result' command.
// Notes:\tThis command creates a temporary item under the current topic
//\t\twhich will contain the result of the next command to be executed.
// Returns: true if the command executes with no errors, otherwise it is false.
//\tpTopic\tPointer to a topic info structure.
//\t\t\tPointer the the buffer to receive the result string.
//\t\t\tSize of the return buffer.
//\t\t\tNumber of arguments in the argument list.
//\t\t\tppArgs\tA list of pointers to the arguments.
bool dde::SysResultExecCmd(PTOPICINFO pTopic, wchar_t*, UINT, UINT, wchar_t** ppArgs) {
  PCONVERSATIONINFO pCI = ConversationFind(pTopic->hszTopicName);

  if (!pCI) {
    return false;  // internal error
  }

  if (pCI->pResultItem) {  // Already have a temporary result item. Get rid of it
    if (pCI->pResultItem->hData) { DdeFreeDataHandle(pCI->pResultItem->hData); }
    ItemRemove(pTopic->pszTopicName, pCI->pResultItem->itemName);
    pCI->pResultItem = nullptr;  // struct was deleted by ItemRemove; null immediately
  }

  pCI->pResultItem = ItemAdd(pTopic->pszTopicName,
      ppArgs[0],
      SysFormatList,
      SysReqResultInfo,
      0);  // Add a new temporary result item to the current conversation

  return true;
}
/// @brief Return the 'result' info for a given item and delete the item.
// Notes:\tThe item is deleted after the data is returned.
// Returns: A DDE data handle to an object containing the return string.
HDDEDATA dde::SysReqResultInfo(UINT wFmt, HSZ hszTopic, HSZ hszItem) {
  PTOPICINFO pTopic = TopicFind(hszTopic);
  if (!pTopic) { return 0; }

  PITEMINFO pItem = ItemFind(pTopic, hszItem);
  if (!pItem) { return 0; }

  HDDEDATA hData = pItem->hData;
  pItem->hData = 0;  // take ownership before ItemRemove deletes the struct

  if (!hData) {  // No data to return. Send back an empty string
    hData = DdeCreateDataHandle(ServerInfo.dwInstance, (LPBYTE)L"", sizeof(wchar_t), 0, hszItem, CF_UNICODETEXT, 0);
  }

  // Clear the conversation's result-item pointer before deletion so it is never dangling.
  PCONVERSATIONINFO pCI = ConversationFind(hszTopic);
  if (pCI && pCI->pResultItem == pItem) { pCI->pResultItem = nullptr; }

  ItemRemove(pTopic->pszTopicName, pItem->itemName);

  return hData;
}
/// @brief Scan for a valid command.
// Notes:\tIf found, the scan pointer is updated.
// Returns: Pointer to the command info if found, 0 if not.
//\tpCmdInfo\tPointer to the current command list.
//\tppStr\t\tPointer to the current scan pointer.
PEXECCMDFNINFO dde::ScanForCommand(PEXECCMDFNINFO pCmdInfo, wchar_t** ppStr) {
  wchar_t* pStart = dde::SkipWhiteSpace(*ppStr);
  wchar_t* p = pStart;

  if (!iswalpha(*p)) {  // First char is not alpha
    return 0;
  }

  while (iswalnum(*p) || *p == L'_') {  // Collect alpha-num and underscore chars.
    p++;
  }

  // Copy the candidate token into a local buffer to avoid mutating the DDEML-owned data buffer.
  const auto tokenLength = static_cast<std::size_t>(p - pStart);
  wchar_t localToken[MAXRESULTSIZE]{};
  if (tokenLength == 0 || tokenLength >= MAXRESULTSIZE) { return 0; }
  wcsncpy_s(localToken, MAXRESULTSIZE, pStart, tokenLength);

  while (pCmdInfo) {  // Search for a command that matches the token
    if (_wcsicmp(localToken, pCmdInfo->pszCmdName) == 0) {
      *ppStr = p;
      return pCmdInfo;
    }
    pCmdInfo = pCmdInfo->pNext;
  }
  return 0;  // not found
}
