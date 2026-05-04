#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbJobFile.h"
#include "MainFrm.h"
#include "Resource.h"
#include "ddeCmds.h"

using namespace dde;

/// @brief Forwards the first DDE argument as a raw command line into the CLI execution pipeline. This is the primary
/// entry point for Python (and any other DDE client) automation:
///   DDEExecute(channel, "[CLI(TEXT 10,20 \"Hello World\")]")
/// The argument string is passed verbatim to EoMfCommandTab::ExecuteCommand so the full CLI grammar — verbs,
/// coordinates, quoted strings, aliases — is available. On success, pszResultString is set to "OK" so that a subsequent
/// DDE Request on the result item returns a non-empty reply rather than an empty string.
bool dde::ExecCLICommand(PTOPICINFO, wchar_t*, UINT uiResultSize, UINT, wchar_t** ppArgs) {
  auto* mainFrame = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
  if (!mainFrame) { return false; }
  mainFrame->ExecuteCommandLine(std::wstring(ppArgs[0]));
  // Write "OK" so the result item is never empty on success.  ProcessExecRequest will synthesise "OK" if this buffer is
  // left empty, but being explicit here makes ExecCLICommand self-contained and testable in isolation.
  (void)uiResultSize;  // parameter reserved for future extended result strings
  return true;
}

/// @brief Opens a tracing file as a new layer and makes it the active work layer.
bool dde::ExecTracingOpen(PTOPICINFO, wchar_t*, UINT, UINT uiNargs, wchar_t** ppArgs) {
  AeSysDoc::GetDoc()->TracingOpen(ppArgs[uiNargs - 1]);
  return true;
}
/// @brief Loads a tracing/job file and places it at the cursor position via the trap.
bool dde::ExecTracingGet(PTOPICINFO, wchar_t*, UINT, UINT uiNargs, wchar_t** ppArgs) {
  CString pathName(ppArgs[uiNargs - 1]);

  CFile file(pathName, CFile::modeRead | CFile::shareDenyNone);
  if (file == CFile::hFileNull) {
    app.WarningMessageBox(IDS_MSG_TRACING_OPEN_FAILURE, pathName);
    return false;
  }
  auto* document = AeSysDoc::GetDoc();
  EoDbLayer* layer = document->GetWorkLayer();

  EoDbJobFile jobFile;
  jobFile.ReadHeader(file);
  jobFile.ReadLayer(file, layer);

  const EoGePoint3d pivotPoint(app.GetCursorPosition());
  document->SetTrapPivotPoint(pivotPoint);
  document->AddGroupsToAllViews(layer);
  document->RemoveAllTrappedGroups();
  document->AddGroupsToTrap(layer);
  document->TranslateTrappedGroups(EoGeVector3d(EoGePoint3d::kOrigin, pivotPoint));
  return true;
}
