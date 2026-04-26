#include "Stdafx.h"

#ifdef USING_DDE
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbJobFile.h"
#include "EoDbText.h"
#include "EoGeReferenceSystem.h"
#include "EoGsRenderState.h"
#include "Resource.h"
#include "ddeCmds.h"

using namespace dde;

/// @brief Sets the Text Height.
bool dde::ExecNoteHT(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  EoDbCharacterCellDefinition characterCellDefinition = Gs::renderState.CharacterCellDefinition();
  characterCellDefinition.SetHeight(_wtof(szBuf));
  Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);
  return true;
}
/// @brief Sets the Fill.
bool dde::ExecFill(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};
  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);
  Gs::renderState.SetPolygonIntStyleId(std::uint16_t(_wtoi(szBuf)));
  return true;
}
/// @brief Sets the Scale.
bool dde::ExecScale(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  AeSysView::GetActiveView()->SetWorldScale(_wtof(szBuf));
  return true;
}
/// @brief Sets the Diamond Length.
bool dde::ExecDL(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  app.SetDimensionLength(app.ParseLength(app.GetUnits(), szBuf));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::DimLen);
  return true;
}
/// @brief Sets the Diamond Angle.
bool dde::ExecDA(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[32]{};
  _tcsncpy_s(szBuf, 32, ppArgs[0], sizeof(szBuf) - 1);
  app.SetDimensionAngle(_wtof(szBuf));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::DimAng);
  return true;
}
/// @brief
bool dde::ExecTracingBlank(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->LayerBlank(ppArgs[PathKey]);

  return true;
}
/// @brief Maps a tracing file.
bool dde::ExecTracingMap(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->TracingMap(ppArgs[PathKey]);

  return true;
}
/// @brief Opens a tracing file.
bool dde::ExecTracingOpen(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->TracingOpen(ppArgs[PathKey]);

  return true;
}
/// @brief Views a tracing file.
bool dde::ExecTracingView(PTOPICINFO, LPTSTR, UINT, UINT uiNargs, LPTSTR *ppArgs) {
  int PathKey = uiNargs - 1;

  auto *document = AeSysDoc::GetDoc();
  document->TracingView(ppArgs[PathKey]);

  return true;
}
/// @brief Gets a tracing file.
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
/// @brief Set the position of the cursor.
bool dde::ExecGotoPoint(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  int iStakeId = _wtoi(szBuf);
  AeSysView::GetActiveView()->SetCursorPosition(app.HomePointGet(iStakeId));

  return true;
}
/// @brief Sets the pen color.
bool dde::ExecPen(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  // TODO: left broken while moving device context get out of SetPenColor
  // Gs::renderState.SetPenColor(DeviceContext, std::int16_t(_wtoi(szBuf)));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Pen);

  return true;
}
/// @brief Sets the line type
bool dde::ExecLine(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  // TODO: left broken while moving device context get out of SetLineType
  // Gs::renderState.SetLineType(DeviceContext, std::int16_t(_wtoi(szBuf)));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Line);

  return true;
}
/// @brief Adds a note the drawing at the current cursor position.
bool dde::ExecNote(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  auto *document = AeSysDoc::GetDoc();

  EoGePoint3d ptPvt = app.GetCursorPosition();

  EoDbFontDefinition fd = Gs::renderState.FontDefinition();
  EoDbCharacterCellDefinition characterCellDefinition = Gs::renderState.CharacterCellDefinition();

  EoGeReferenceSystem referenceSystem(ptPvt, characterCellDefinition);

  auto *Group = new EoDbGroup(new EoDbText(fd, referenceSystem, CString(ppArgs[0])));
  document->AddWorkLayerGroup(Group);
  document->UpdateAllViews(nullptr, EoDb::kGroup, Group);

  ptPvt = text_GetNewLinePos(fd, referenceSystem, 1.0, 0);
  AeSysView::GetActiveView()->SetCursorPosition(ptPvt);

  return true;
}
/// @brief Posts message to force key driven action.
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
/// @brief Sets a home point.
bool dde::ExecSetPoint(PTOPICINFO, LPTSTR, UINT, UINT, LPTSTR *ppArgs) {
  wchar_t szBuf[8]{};

  _tcsncpy_s(szBuf, 8, ppArgs[0], sizeof(szBuf) - 1);

  int iStakeId = _wtoi(szBuf);

  app.HomePointSave(iStakeId, app.GetCursorPosition());

  return true;
}
#endif
