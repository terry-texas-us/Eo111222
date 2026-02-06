#include "Stdafx.h"

#include <algorithm>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbFontDefinition.h"
#include "EoDbPrimitive.h"
#include "PrimState.h"
#include "Resource.h"

// State list maintenance
CPrimState* psSav[] = {0, 0, 0, 0};

const CPrimState& CPrimState::operator=(const CPrimState& other) {
  m_fontDefinition = other.m_fontDefinition;

  m_color = other.m_color;
  m_LineTypeIndex = other.m_LineTypeIndex;
  m_pointStyle = other.m_pointStyle;
  m_PolygonInteriorStyle = other.m_PolygonInteriorStyle;
  m_PolygonInteriorStyleIndex = other.m_PolygonInteriorStyleIndex;

  return (*this);
}
void CPrimState::Restore(CDC* deviceContext, int iSaveId) {
  if (iSaveId >= static_cast<int>(sizeof(psSav) / sizeof(psSav[0]))) return;

  if (psSav[iSaveId] != 0) {
    SetPen(nullptr, deviceContext, psSav[iSaveId]->PenColor(), psSav[iSaveId]->LineType());

    m_fontDefinition = psSav[iSaveId]->m_fontDefinition;

    SetTxtAlign(deviceContext, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());

    SetPolygonIntStyle(psSav[iSaveId]->PolygonIntStyle());
    SetPolygonIntStyleId(psSav[iSaveId]->PolygonIntStyleId());

    delete psSav[iSaveId];
    psSav[iSaveId] = 0;
  }
}
int CPrimState::Save() {
  int iSaveId = sizeof(psSav) / sizeof(psSav[0]) - 1;

  while (iSaveId >= 0 && psSav[iSaveId] != 0) iSaveId--;

  if (iSaveId < 0) {
    app.WarningMessageBox(IDS_MSG_SAVE_STATE_LIST_ERROR);
  } else {
    SetPolygonIntStyle(pstate.PolygonIntStyle());
    psSav[iSaveId] = new CPrimState;
    *psSav[iSaveId] = pstate;
  }
  // return id to use for restore reference
  return (iSaveId);
}

void CPrimState::SetPen(AeSysView* view, CDC* deviceContext, EoInt16 color, EoInt16 lineTypeIndex) {
  if (EoDbPrimitive::SpecialColor() != 0) { color = EoDbPrimitive::SpecialColor(); }
  if (color == EoDbPrimitive::COLOR_BYLAYER) { color = EoDbPrimitive::LayerColor(); }
  if (lineTypeIndex == EoDbPrimitive::LINETYPE_BYLAYER) { lineTypeIndex = EoDbPrimitive::LayerLineTypeIndex(); }
  m_color = color;
  m_LineTypeIndex = lineTypeIndex;

  double LogicalWidth = 0.;

  if (view && view->PenWidthsOn()) {
    int LogicalPixelsX = deviceContext->GetDeviceCaps(LOGPIXELSX);
    LogicalWidth = app.LineWeight(color) * double(LogicalPixelsX);
    LogicalWidth *= std::min(1.0, view->WidthInInches() / view->UExtent());
    LogicalWidth = Eo::Round(LogicalWidth);
  }
  if (deviceContext) { ManagePenResources(deviceContext, color, int(LogicalWidth), lineTypeIndex); }
}

void CPrimState::ManagePenResources(CDC* deviceContext, EoInt16 penColor, int penWidth, EoInt16 lineType) {
  static const int NumberOfPens = 8;
  static HPEN hPen[NumberOfPens] = {0, 0, 0, 0, 0, 0, 0, 0};
  static COLORREF crColRef[NumberOfPens];
  static EoInt16 LineTypes[NumberOfPens];
  static int PenWidths[NumberOfPens];
  static HPEN hPenCur;

  switch (lineType) {
    case 0:
      lineType = PS_NULL;
      break;

    case 2:
      lineType = PS_DOT;
      break;

    case 3:
      lineType = PS_DASH;
      break;

    case 6:
      lineType = PS_DASHDOT;
      break;

    case 7:
      lineType = PS_DASHDOTDOT;
      break;

    default:
      lineType = PS_SOLID;
  }
  if (deviceContext) { deviceContext->SetTextColor(pColTbl[penColor]); }

  int iPen = 0;
  for (int i = 0; i < NumberOfPens; i++) {
    if (hPen[i] && LineTypes[i] == lineType && PenWidths[i] == penWidth && crColRef[i] == pColTbl[penColor]) {
      hPenCur = hPen[i];
      if (deviceContext) { deviceContext->SelectObject(CPen::FromHandle(hPenCur)); }
      return;
    }
    if (hPen[i] == 0) { iPen = i; }
  }
  HPEN NewPenHandle = ::CreatePen(lineType, penWidth, pColTbl[penColor]);

  if (NewPenHandle) {
    hPenCur = NewPenHandle;
    if (deviceContext) { deviceContext->SelectObject(CPen::FromHandle(NewPenHandle)); }
    if (hPen[iPen]) { ::DeleteObject(hPen[iPen]); }
    hPen[iPen] = NewPenHandle;
    LineTypes[iPen] = lineType;
    PenWidths[iPen] = penWidth;
    crColRef[iPen] = pColTbl[penColor];
  }
}
void CPrimState::SetColor(CDC* deviceContext, EoInt16 color) {
  m_color = color;
  if (deviceContext) { ManagePenResources(deviceContext, color, 0, m_LineTypeIndex); }
}
void CPrimState::SetLineType(CDC* deviceContext, EoInt16 lineTypeIndex) {
  m_LineTypeIndex = lineTypeIndex;
  if (deviceContext) { ManagePenResources(deviceContext, m_color, 0, lineTypeIndex); }
}
int CPrimState::SetROP2(CDC* deviceContext, int iDrawMode) {
  // Sets the current foreground mix mode. GDI uses the foreground mix mode to combine pens and
  // interiors of filled objects with the colors already on the screen. The foreground mix mode
  // defines how colors from the brush or pen and the colors in the existing image are to be combined.

  if (ColorPalette[0] == RGB(0xFF, 0xFF, 0xFF)) {
    if (iDrawMode == R2_XORPEN) iDrawMode = R2_NOTXORPEN;
  }
  return (deviceContext->SetROP2(iDrawMode));
}
void CPrimState::SetTxtAlign(CDC* deviceContext, EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment) {
  //if (horizontalAlignment == 1)
  //	nFlgs = TA_LEFT;
  //else if (horizontalAlignment == 2)
  //	nFlgs = TA_CENTER;
  //else
  //	nFlgs = TA_RIGHT;

  //if (verticalAlignment == 2)
  //	nFlgs |= TA_TOP;
  //else
  //	nFlgs |= TA_BASELINE;

  m_fontDefinition.HorizontalAlignment(horizontalAlignment);
  m_fontDefinition.VerticalAlignment(verticalAlignment);

  deviceContext->SetTextAlign(TA_LEFT | TA_BASELINE);
}
void CPrimState::SetFontDef(CDC* deviceContext, const EoDbFontDefinition& fd) {
  m_fontDefinition = fd;
  SetTxtAlign(deviceContext, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());
}
