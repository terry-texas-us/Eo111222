#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "ddeGItms.h"

namespace dde {
PITEMINFO DimAngZInfo;
PITEMINFO DimLenInfo;
PITEMINFO EngAngZInfo;
PITEMINFO EngLenInfo;
PITEMINFO ExtNumInfo;
PITEMINFO ExtStrInfo;
PITEMINFO RelPosXInfo;
PITEMINFO RelPosYInfo;
PITEMINFO RelPosZInfo;
PITEMINFO ScaleInfo;
}  // namespace dde
// dimension z axis angle interface
bool dde::DimAngZPoke(UINT, HSZ, HSZ, HDDEDATA hData) {
  wchar_t sz[32]{};
  DdeGetData(hData, (LPBYTE)sz, (DWORD)sizeof(sz), (DWORD)0);
  app.SetDimensionAngle(_wtof(sz));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::DimAng);
  return true;
}
HDDEDATA dde::DimAngZRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", app.DimensionAngle());
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Dimension length interface
bool dde::DimLenPoke(UINT, HSZ, HSZ, HDDEDATA hData) {
  wchar_t sz[32]{};
  DdeGetData(hData, (LPBYTE)sz, (DWORD)sizeof(sz), (DWORD)0);
  app.SetDimensionLength(_wtof(sz));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::DimLen);

  return true;
}
HDDEDATA dde::DimLenRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", app.DimensionLength());
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Engaged length interface (no poke)
HDDEDATA dde::EngLenRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", app.EngagedLength());
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Engaged z axis angle interface (no poke)
HDDEDATA dde::EngAngZRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", app.EngagedAngle());
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Extracted number interface (no poke)
HDDEDATA dde::ExtNumRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", app.ExtractedNumber());
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Extracted string interface (no poke)
HDDEDATA dde::ExtStrRequest(UINT wFmt, HSZ, HSZ hszItem) {
  int SizeOfString = app.ExtractedString().GetLength() + 1;
  wchar_t* sz = new wchar_t[SizeOfString];
  wcscpy_s(sz, SizeOfString, app.ExtractedString());

  HDDEDATA ReturnValue = dde::MakeCFText(wFmt, sz, hszItem);
  delete[] sz;

  return ReturnValue;
}
// Relative x position interface (no poke)
HDDEDATA dde::RelPosXRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", AeSysView::GetActiveView()->GetRelPos().x);
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Relative y position interface (no poke)
HDDEDATA dde::RelPosYRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", AeSysView::GetActiveView()->GetRelPos().y);
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Relative z position interface (no poke)
HDDEDATA dde::RelPosZRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", AeSysView::GetActiveView()->GetRelPos().z);
  return dde::MakeCFText(wFmt, sz, hszItem);
}
// Scale interface
bool dde::ScalePoke(UINT, HSZ, HSZ, HDDEDATA hData) {
  wchar_t sz[32]{};
  DdeGetData(hData, (LPBYTE)sz, (DWORD)sizeof(sz), (DWORD)0);
  AeSysView::GetActiveView()->SetWorldScale(_wtof(sz));
  return true;
}
HDDEDATA dde::ScaleRequest(UINT wFmt, HSZ, HSZ hszItem) {
  wchar_t sz[32]{};
  swprintf_s(sz, 32, L"%f", AeSysView::GetActiveView()->GetWorldScale());
  return dde::MakeCFText(wFmt, sz, hszItem);
}
