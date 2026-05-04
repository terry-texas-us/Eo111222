#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <cstdint>
#include <ddeml.h>

// String names for some standard DDE strings not defined in DDEML.H

namespace dde {
// Some constants
const std::uint16_t MAXFORMATS = 128;  // max no of CF formats we will list
const std::uint16_t MAXOPS = 128;  // max no of opcodes we can execute
const std::uint16_t MAXRESULTSIZE = 1024;  // largest result string returned

/// @brief Typed result from ParseCmd — allows callers to distinguish specific
/// failure modes rather than testing a bare bool and reading a raw error string.
enum class ParseResult {
  Ok,
  MissingOpenBracket,  ///< '[' not found before command name
  UnknownCommand,       ///< command name not in the topic's command list
  OpTableOverflow,      ///< internal MAXOPS limit reached
  TooManyArguments,     ///< caller supplied more args than uiMaxArgs
  TooFewArguments,      ///< caller supplied fewer args than uiMinArgs
  MissingCloseParen,    ///< ')' not found after argument list
  MissingCloseBracket,  ///< ']' not found after ')'
};

/// @brief Produce a short ASCII label for a ParseResult value suitable for
/// embedding in a structured error reply (e.g. "ERROR:TooFewArguments").
const wchar_t* ParseResultLabel(ParseResult r) noexcept;

// Definition for a DDE Request processing function
typedef HDDEDATA(REQUESTFN)(UINT wFmt, HSZ hszTopic, HSZ hszItem);
typedef REQUESTFN *PREQUESTFN;

// Definition for a DDE Poke processing function
typedef bool(POKEFN)(UINT wFmt, HSZ hszTopic, HSZ hszItem, HDDEDATA hData);
typedef POKEFN *PPOKEFN;

// Definition for a DDE Execute processing function
typedef bool(EXECFN)(UINT wFmt, HSZ hszTopic, HDDEDATA hData);
typedef EXECFN *PEXECFN;

// Structure used to hold a clipboard id and its text name
typedef struct _CFTAGNAME {
  std::uint16_t wFmt;
  const wchar_t *pszName;
} CFTAGNAME, *PCFTAGNAME;
extern CFTAGNAME CFNames[];

// Definition for a DDE execute command procession function
typedef bool(
    EXECCMDFN)(struct _TOPICINFO *pTopic, wchar_t *pszResultString, UINT uiResultSize, UINT uiNargs, wchar_t **ppArgs);
typedef EXECCMDFN *PEXECCMDFN;

// Structure used to store information on a DDE item
typedef struct _ITEMINFO {
  struct _ITEMINFO *pNext;  // pointer to the next item
  const wchar_t* itemName;  // pointer to its name
  HSZ hszItemName;  // DDE string handle for the name
  struct _TOPICINFO *pTopic;  // pointer to the topic it belongs to
  LPWORD pFormatList;  // ptr to null term list of CF format words.
  PREQUESTFN pfnRequest;  // pointer to the item specific request processor
  PPOKEFN pfnPoke;  // pointer to the item specific poke processor
  HDDEDATA hData;  // data for this item
} ITEMINFO, *PITEMINFO;

// Structure used to store information on a DDE execute command processor function
typedef struct _EXECCMDFNINFO {
  struct _EXECCMDFNINFO *pNext;  // pointer to the next item
  struct _TOPICINFO *pTopic;  // pointer to the topic it belongs to
  const wchar_t* pszCmdName;  // The name of the command
  PEXECCMDFN pFn;  // A pointer to the function
  UINT uiMinArgs;  // min number of args
  UINT uiMaxArgs;  // max number of args
} EXECCMDFNINFO, *PEXECCMDFNINFO;

// Structure used to store information about a DDE conversation
typedef struct _DDE_CONVERSATIONINFO {
  struct _DDE_CONVERSATIONINFO *pNext;  // pointer to the next one
  HCONV hConv;  // handle to the conversation
  HSZ hszTopicName;  // HSZ for the topic of the conversation
  PITEMINFO pResultItem;  // pointer to a temp result item
} CONVERSATIONINFO, *PCONVERSATIONINFO;

// Structure used to store information on a DDE topic
typedef struct _TOPICINFO {
  struct _TOPICINFO *pNext;  // pointer to the next topic
  const wchar_t* pszTopicName;  // pointer to its string name
  HSZ hszTopicName;  // DDE string handle for the name
  PITEMINFO pItemList;  // pointer to its item list
  PEXECFN pfnExec;  // pointer to its DDE Execute processor
  PREQUESTFN pfnRequest;  // pointer to the generic request processor
  PPOKEFN pfnPoke;  // pointer to the generic poke processor
  PEXECCMDFNINFO pCmdList;  // pointer to the execute command list
} TOPICINFO, *PTOPICINFO;

// Define pointer type used in exec cmd op table

typedef void *POP;
typedef POP *PPOP;

/// @brief Callback invoked when DDE initialization fails.
/// serviceNameForDisplay The service name string for display in the error message.
/// hMainWindow The main window handle to destroy on fatal failure, or nullptr.
typedef void(INITERRORFN)(const wchar_t* serviceNameForDisplay, HWND hMainWindow);
typedef INITERRORFN *PINITERRORFN;

/// @brief Fallback callback invoked when a topic has no exec function and no command list.
/// hData The DDE data handle from the execute transaction.
typedef void(FALLBACKEXECFN)(HDDEDATA hData);
typedef FALLBACKEXECFN *PFALLBACKEXECFN;

// Structure used to store information on a DDE server which has only one service
typedef struct _SERVERINFO {
  const wchar_t* lpszServiceName;  // pointer to the service string name
  HSZ hszServiceName;  // DDE string handle for the name
  PTOPICINFO pTopicList;  // pointer to the topic list
  DWORD dwInstance;  // DDE Instance value
  PFNCALLBACK pfnStdCallback;  // pointer to standard DDE callback fn
  PFNCALLBACK pfnCustomCallback;  // pointer to custom DDE callback fn
  PCONVERSATIONINFO pConvList;  // pointer to the active conversation list
  PFALLBACKEXECFN pfnFallbackExec;  // pointer to fallback exec handler (replaces direct PostMessage)
} SERVERINFO, *PSERVERINFO;

extern SERVERINFO ServerInfo;
extern std::uint16_t SysFormatList[];
extern std::uint16_t MyFormats[];

void AddCommands(const wchar_t*);
void AddFormatsToList(LPWORD pMain, int iMax, LPWORD pList);
void AddSystemTopic(const wchar_t*, LPWORD);

bool ConversationAdd(HCONV hConv, HSZ hszTopic);
PCONVERSATIONINFO ConversationFind(HSZ hszTopic);
bool ConversationRemove(HCONV hConv, HSZ hszTopic);

bool DoCallback(UINT wType, UINT wFmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData, HDDEDATA *phReturnData);
HDDEDATA DoWildConnect(HSZ hszTopic);

PEXECCMDFNINFO ExecCmdAdd(const wchar_t* pszTopic, const wchar_t* pszCmdName, PEXECCMDFN pExecCmdFn, UINT uiMinArgs, UINT uiMaxArgs);
PEXECCMDFNINFO ExecCmdFind(PTOPICINFO pTopic, const wchar_t* lpszCmd);
bool ExecCmdRemove(const wchar_t* pszTopic, const wchar_t* pszCmdName);

PITEMINFO ItemAdd(const wchar_t* lpszTopic, const wchar_t* lpszItem, LPWORD pFormatList, PREQUESTFN lpReqFn, PPOKEFN lpPokeFn);
PITEMINFO ItemFind(PTOPICINFO pTopic, HSZ hszItem);
PITEMINFO ItemFind(PTOPICINFO pTopic, const wchar_t* lpszItem);
bool ItemRemove(const wchar_t* lpszTopic, const wchar_t* lpszItem);

PTOPICINFO TopicAdd(const wchar_t* lpszTopic, PEXECFN pfnExec, PREQUESTFN pfnRequest, PPOKEFN pfnPoke);
PTOPICINFO TopicFind(HSZ hszName);
PTOPICINFO TopicFind(const wchar_t* lpszName);
bool TopicRemove(const wchar_t* lpszTopic);

wchar_t *GetCFNameFromId(std::uint16_t wFmt, wchar_t *lpBuf, int iSize);
HDDEDATA MakeCFText(UINT, const wchar_t*, HSZ);
HDDEDATA MakeDataFromFormatList(LPWORD pFmt, std::uint16_t wFmt, HSZ hszItem);
ParseResult ParseCmd(wchar_t **ppszCmdLine,
    PTOPICINFO pTopic,
    PPOP pOpTable,
    UINT uiOpTableSize,
    wchar_t *pArgBuf);
void PostAdvise(PITEMINFO pItemInfo);
bool ProcessExecRequest(PTOPICINFO pTopic, HDDEDATA hData);
PEXECCMDFNINFO ScanForCommand(PEXECCMDFNINFO pCmdInfo, wchar_t **ppStr);
bool Initialize(const wchar_t* serviceName, PINITERRORFN pfnInitError, HWND hMainWindow, PFALLBACKEXECFN pfnFallbackExec);
HDDEDATA WINAPI StdCallback(UINT, UINT, HCONV, HSZ, HSZ, HDDEDATA, DWORD, DWORD);
HDDEDATA SysReqResultInfo(UINT wFmt, HSZ hszTopic, HSZ hszItem);
bool SysResultExecCmd(PTOPICINFO pTopic, wchar_t *pszResult, UINT uiResultSize, UINT uiNargs, wchar_t **ppArgs);
HDDEDATA TopicReqFormats(UINT wFmt, HSZ hszTopic, HSZ hszItem);
void Uninitialize();
}  // namespace dde
