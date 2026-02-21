#include "Stdafx.h"

#include <algorithm>
#include <cstdint>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbFontDefinition.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderState.h"
#include "Resource.h"

// State list maintenance
EoGsRenderState* psSav[] = {0, 0, 0, 0};

void EoGsRenderState::Restore(CDC* deviceContext, int iSaveId) {
  if (iSaveId >= static_cast<int>(sizeof(psSav) / sizeof(psSav[0]))) return;

  if (psSav[iSaveId] != 0) {
    SetPen(nullptr, deviceContext, psSav[iSaveId]->Color(), psSav[iSaveId]->LineTypeIndex());

    m_fontDefinition = psSav[iSaveId]->m_fontDefinition;

    SetAlignment(deviceContext, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());

    SetPolygonIntStyle(psSav[iSaveId]->PolygonIntStyle());
    SetPolygonIntStyleId(psSav[iSaveId]->PolygonIntStyleId());

    delete psSav[iSaveId];
    psSav[iSaveId] = 0;
  }
}

int EoGsRenderState::Save() {
  int iSaveId = sizeof(psSav) / sizeof(psSav[0]) - 1;

  while (iSaveId >= 0 && psSav[iSaveId] != 0) iSaveId--;

  if (iSaveId < 0) {
    app.WarningMessageBox(IDS_MSG_SAVE_STATE_LIST_ERROR);
  } else {
    SetPolygonIntStyle(renderState.PolygonIntStyle());
    psSav[iSaveId] = new EoGsRenderState;
    *psSav[iSaveId] = renderState;
  }
  // return id to use for restore reference
  return iSaveId;
}

void EoGsRenderState::SetPen(AeSysView* view, CDC* deviceContext, std::int16_t color, std::int16_t lineTypeIndex) {
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

void EoGsRenderState::ManagePenResources(CDC* deviceContext, std::int16_t penColor, int penWidth, std::int16_t lineType) {
  static const int NumberOfPens = 8;
  static HPEN hPen[NumberOfPens] = {0, 0, 0, 0, 0, 0, 0, 0};
  static COLORREF crColRef[NumberOfPens];
  static std::int16_t LineTypes[NumberOfPens];
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

void EoGsRenderState::SetColor(CDC* deviceContext, std::int16_t color) {
  m_color = color;
  if (deviceContext) { ManagePenResources(deviceContext, color, 0, m_LineTypeIndex); }
}

void EoGsRenderState::SetLineType(CDC* deviceContext, std::int16_t lineTypeIndex) {
  m_LineTypeIndex = lineTypeIndex;
  if (deviceContext) { ManagePenResources(deviceContext, m_color, 0, lineTypeIndex); }
}

/** @brief Sets the current foreground mix mode. GDI uses the foreground mix mode to combine pens and
 * interiors of filled objects with the colors already on the screen. The foreground mix mode
 * defines how colors from the brush or pen and the colors in the existing image are to be combined.
 *
 * @param deviceContext The device context to set the ROP2 mode for.
 * @param drawMode The drawing mode to set, such as R2_COPYPEN, R2_XORPEN, etc.
 * @return The previous ROP2 mode before setting the new one.
 * @note The behavior of ROP2 modes can be affected by the background color. 
 * For example, XOR modes may not work properly with non black or white background colors, and this function includes logic to handle such cases accordingly.
 */
int EoGsRenderState::SetROP2(CDC* deviceContext, int drawMode) {
  if (Eo::ColorPalette[0] == Eo::colorBlack) {
    return deviceContext->SetROP2(drawMode);
  } else if (Eo::ColorPalette[0] == Eo::colorWhite) {
    if (drawMode == R2_XORPEN) { drawMode = R2_NOTXORPEN; }
  } else {
    // Gray or other background - XOR modes don't work properly
    // Fallback to normal drawing (caller should use invalidation instead)
    if (drawMode == R2_XORPEN || drawMode == R2_NOTXORPEN) { drawMode = R2_COPYPEN; }
  }
  return deviceContext->SetROP2(drawMode);
}

void EoGsRenderState::SetAlignment(CDC* deviceContext, EoDb::HorizontalAlignment horizontalAlignment, EoDb::VerticalAlignment verticalAlignment) {
  m_fontDefinition.SetAlignment(horizontalAlignment, verticalAlignment);

  deviceContext->SetTextAlign(TA_LEFT | TA_BASELINE);
}

void EoGsRenderState::SetFontDefinition(CDC* deviceContext, const EoDbFontDefinition& fd) {
  m_fontDefinition = fd;
  SetAlignment(deviceContext, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());
}
