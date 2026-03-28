#include "Stdafx.h"

#if defined(USING_DDE)
#include "AeSys.h"
#include "Resource.h"

#include "dde.h"
#include "ddeCmds.h"
#include "ddeGItms.h"

namespace dde {
void RegisterAeSysTopics();
}

using namespace dde;

/// @brief Error callback for DDE initialization failure — displays a warning and destroys the main window.
static void OnDdeInitError(LPCWSTR serviceNameForDisplay, HWND hMainWindow) {
  app.WarningMessageBox(IDS_MSG_DDE_INIT_FAILURE, serviceNameForDisplay);
  if (hMainWindow) { ::DestroyWindow(hMainWindow); }
}

/// @brief Fallback exec handler — posts the first character of the DDE data as a WM_CHAR message.
static void OnDdeFallbackExec(HDDEDATA hData) {
  char sz[32]{};
  DdeGetData(hData, (LPBYTE)sz, (DWORD)sizeof(sz), (DWORD)0);
  ::PostMessage(app.GetSafeHwnd(), WM_CHAR, (WPARAM)sz[0], (LPARAM)1);
}

/// @brief Initialize DDE and register all AeSys-specific topics, items, and commands.
void dde::RegisterAeSysTopics() {
  if (!dde::Initialize(L"AeSys", OnDdeInitError, app.GetSafeHwnd(), OnDdeFallbackExec)) { return; }

  // System topic execute commands
  ExecCmdAdd(SZDDESYS_TOPIC, L"TracingOpen", ExecTracingOpen, 1, 2);
  ExecCmdAdd(SZDDESYS_TOPIC, L"TracingMap", ExecTracingMap, 1, 2);
  ExecCmdAdd(SZDDESYS_TOPIC, L"TracingView", ExecTracingView, 1, 2);

  // General topic items
  DimAngZInfo = ItemAdd(L"General", L"DimAngZ", MyFormats, DimAngZRequest, DimAngZPoke);
  DimLenInfo = ItemAdd(L"General", L"DimLen", MyFormats, DimLenRequest, DimLenPoke);
  EngAngZInfo = ItemAdd(L"General", L"EngAngZ", MyFormats, EngAngZRequest, 0);
  EngLenInfo = ItemAdd(L"General", L"EngLen", MyFormats, EngLenRequest, 0);
  ExtNumInfo = ItemAdd(L"General", L"ExtNum", MyFormats, ExtNumRequest, 0);
  ExtStrInfo = ItemAdd(L"General", L"ExtStr", MyFormats, ExtStrRequest, 0);
  RelPosZInfo = ItemAdd(L"General", L"RelPosZ", MyFormats, RelPosZRequest, 0);
  RelPosYInfo = ItemAdd(L"General", L"RelPosY", MyFormats, RelPosYRequest, 0);
  RelPosXInfo = ItemAdd(L"General", L"RelPosX", MyFormats, RelPosXRequest, 0);
  ScaleInfo = ItemAdd(L"General", L"Scale", MyFormats, ScaleRequest, ScalePoke);

  // Commands topic
  TopicAdd(L"Commands", 0, 0, 0);

  ExecCmdAdd(L"Commands", L"TracingBlank", ExecTracingBlank, 1, 2);
  ExecCmdAdd(L"Commands", L"TracingMap", ExecTracingMap, 1, 2);
  ExecCmdAdd(L"Commands", L"TracingOpen", ExecTracingOpen, 1, 2);
  ExecCmdAdd(L"Commands", L"TracingView", ExecTracingView, 1, 2);
  ExecCmdAdd(L"Commands", L"FileGet", ExecFileGet, 1, 2);
  ExecCmdAdd(L"Commands", L"GotoPoint", ExecGotoPoint, 1, 1);
  ExecCmdAdd(L"Commands", L"Line", ExecLine, 1, 1);
  ExecCmdAdd(L"Commands", L"Pen", ExecPen, 1, 1);
  ExecCmdAdd(L"Commands", L"Note", ExecNote, 1, 1);
  ExecCmdAdd(L"Commands", L"Send", ExecSend, 1, 1);
  ExecCmdAdd(L"Commands", L"SetPoint", ExecSetPoint, 1, 1);
  ExecCmdAdd(L"Commands", L"DimAngZ", ExecDA, 1, 1);
  ExecCmdAdd(L"Commands", L"DimLen", ExecDL, 1, 1);
  ExecCmdAdd(L"Commands", L"Scale", ExecScale, 1, 1);
  ExecCmdAdd(L"Commands", L"Fill", ExecFill, 1, 1);
  ExecCmdAdd(L"Commands", L"NoteHT", ExecNoteHT, 1, 1);
}
#endif
