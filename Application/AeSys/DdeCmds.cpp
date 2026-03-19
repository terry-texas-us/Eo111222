#include "Stdafx.h"

#if defined(USING_DDE)
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Resource.h"

#include "ddeCmds.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbJobFile.h"
#include "EoDbText.h"
#include "EoGeReferenceSystem.h"
#include "EoGsRenderState.h"

using namespace dde;

/// <summary>Sets the Text Height.</summary>
bool dde::ExecNoteHT(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  EoDbCharacterCellDefinition characterCellDefinition = renderState.CharacterCellDefinition();
  characterCellDefinition.SetHeight(_wtof(szBuf));
  renderState.SetCharacterCellDefinition(characterCellDefinition);
  return true;
}
/// <summary>Sets the Fill.</summary>
bool dde::ExecFill(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};
  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);
  renderState.SetPolygonIntStyleId(std::uint16_t(_wtoi(szBuf)));
  return true;
}
/// <summary>Sets the Scale.</summary>
bool dde::ExecScale(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  AeSysView::GetActiveView()->SetWorldScale(_wtof(szBuf));
  return true;
}
/// <summary>Sets the Diamond Length.</summary>
bool dde::ExecDL(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  app.SetDimensionLength(app.ParseLength(app.GetUnits(), szBuf));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::DimLen);
  return true;
}
/// <summary>Sets the Diamond Angle.</summary>
bool dde::ExecDA(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  app.SetDimensionAngle(_wtof(szBuf));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::DimAng);
  return true;
}
/// <summary></summary>
bool dde::ExecTracingBlank(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->LayerBlank(ppArgs[PathKey]);

  return true;
}
/// <summary>Maps a tracing file.</summary>
bool dde::ExecTracingMap(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->TracingMap(ppArgs[PathKey]);

  return true;
}
/// <summary>Opens a tracing file.</summary>
bool dde::ExecTracingOpen(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->TracingOpen(ppArgs[PathKey]);

  return true;
}
/// <summary>Views a tracing file.</summary>
bool dde::ExecTracingView(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->TracingView(ppArgs[PathKey]);

  return true;
}
/// <summary>Gets a tracing file.</summary>
bool dde::ExecFileGet(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  CString PathName(ppArgs[uiNargs - 1]);

  CFile File(PathName, CFile::modeRead | CFile::shareDenyNone);
  if (File == CFile::hFileNull) {
    app.WarningMessageBox(IDS_MSG_TRACING_OPEN_FAILURE, PathName);
    return false;
  }
  auto *document = AeSysDoc::GetDoc();

  EoDbLayer *Layer = document->GetWorkLayer();

  EoDbJobFile JobFile;
  JobFile.ReadHeader(File);
  JobFile.ReadLayer(File, Layer);

  EoGePoint3d PivotPoint(app.GetCursorPosition());
  document->SetTrapPivotPoint(PivotPoint);

  document->AddGroupsToAllViews(Layer);
  document->RemoveAllTrappedGroups();
  document->AddGroupsToTrap(Layer);
  document->TranslateTrappedGroups(EoGeVector3d(EoGePoint3d::kOrigin, PivotPoint));
  return true;
}
/// <summary>Set the position of the cursor.</summary>
bool dde::ExecGotoPoint(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  int iStakeId = _wtoi(szBuf);
  AeSysView::GetActiveView()->SetCursorPosition(app.HomePointGet(iStakeId));

  return true;
}
/// <summary>Sets the pen color.</summary>
bool dde::ExecPen(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  // TODO: left broken while moving device context get out of SetPenColor
  // renderState.SetPenColor(DeviceContext, std::int16_t(_wtoi(szBuf)));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Pen);

  return true;
}
/// <summary>Sets the line type</summary>
bool dde::ExecLine(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  // TODO: left broken while moving device context get out of SetLineType
  // renderState.SetLineType(DeviceContext, std::int16_t(_wtoi(szBuf)));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Line);

  return true;
}
/// <summary>Adds a note the drawing at the current cursor position.</summary>
bool dde::ExecNote(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  auto *document = AeSysDoc::GetDoc();

  EoGePoint3d ptPvt = app.GetCursorPosition();

  EoDbFontDefinition fd = renderState.FontDefinition();
  EoDbCharacterCellDefinition characterCellDefinition = renderState.CharacterCellDefinition();

  EoGeReferenceSystem referenceSystem(ptPvt, characterCellDefinition);

  auto *Group = new EoDbGroup(new EoDbText(fd, referenceSystem, CString(ppArgs[0])));
  document->AddWorkLayerGroup(Group);
  document->UpdateAllViews(nullptr, EoDb::kGroup, Group);

  ptPvt = text_GetNewLinePos(fd, referenceSystem, 1.0, 0);
  AeSysView::GetActiveView()->SetCursorPosition(ptPvt);

  return true;
}
/// <summary>Posts message to force key driven action.</summary>
bool dde::ExecSend(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  HWND hWndTarget = GetFocus();
  LPTSTR pIdx = (LPTSTR)ppArgs[0];
  while (*pIdx != 0) {
    if (*pIdx == '{') {
      *pIdx++;
      int iVkValue = _wtoi(pIdx);
      ::PostMessage(hWndTarget, WM_KEYDOWN, iVkValue, 0L);
      while ((*pIdx != 0) && (*pIdx != '}')) { pIdx++; }
    } else {
      ::PostMessage(hWndTarget, WM_CHAR, *pIdx, 0L);
      pIdx++;
    }
  }
  return true;
}
/// <summary>Sets a home point.</summary>
bool dde::ExecSetPoint(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  int iStakeId = _wtoi(szBuf);

  app.HomePointSave(iStakeId, app.GetCursorPosition());

  return true;
}
#endif
