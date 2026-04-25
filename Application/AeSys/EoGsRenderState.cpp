#include "Stdafx.h"

#include <algorithm>
#include <cstdint>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbFontDefinition.h"
#include "EoDbPrimitive.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderState.h"
#include "Resource.h"

// State list maintenance
EoGsRenderState* psSav[] = {nullptr, nullptr, nullptr, nullptr};

void EoGsRenderState::Restore(CDC* deviceContext, int saveIndex) {
  if (saveIndex >= static_cast<int>(sizeof(psSav) / sizeof(psSav[0]))) { return; }

  if (psSav[saveIndex] != nullptr) {
    SetPen(nullptr, deviceContext, psSav[saveIndex]->Color(), psSav[saveIndex]->LineTypeIndex(),
        psSav[saveIndex]->LineTypeName(), psSav[saveIndex]->LineWeight(), psSav[saveIndex]->LineTypeScale());

    m_fontDefinition = psSav[saveIndex]->m_fontDefinition;

    SetAlignment(deviceContext, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());

    SetPolygonIntStyle(psSav[saveIndex]->PolygonIntStyle());
    SetPolygonIntStyleId(psSav[saveIndex]->PolygonIntStyleId());

    delete psSav[saveIndex];
    psSav[saveIndex] = nullptr;
  }
}

void EoGsRenderState::Restore(EoGsRenderDevice* renderDevice, int saveIndex) {
  if (saveIndex >= static_cast<int>(sizeof(psSav) / sizeof(psSav[0]))) { return; }

  if (psSav[saveIndex] != nullptr) {
    SetPen(nullptr, renderDevice, psSav[saveIndex]->Color(), psSav[saveIndex]->LineTypeIndex(),
        psSav[saveIndex]->LineTypeName(), psSav[saveIndex]->LineWeight(), psSav[saveIndex]->LineTypeScale());

    m_fontDefinition = psSav[saveIndex]->m_fontDefinition;

    SetAlignment(renderDevice, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());

    SetPolygonIntStyle(psSav[saveIndex]->PolygonIntStyle());
    SetPolygonIntStyleId(psSav[saveIndex]->PolygonIntStyleId());

    delete psSav[saveIndex];
    psSav[saveIndex] = nullptr;
  }
}

int EoGsRenderState::Save() {
  int iSaveId = sizeof(psSav) / sizeof(psSav[0]) - 1;

  while (iSaveId >= 0 && psSav[iSaveId] != nullptr) { iSaveId--; }

  if (iSaveId < 0) {
    app.WarningMessageBox(IDS_MSG_SAVE_STATE_LIST_ERROR);
  } else {
    SetPolygonIntStyle(Gs::renderState.PolygonIntStyle());
    psSav[iSaveId] = new EoGsRenderState;
    *psSav[iSaveId] = Gs::renderState;
  }
  // return id to use for restore reference
  return iSaveId;
}

void EoGsRenderState::SetPen(AeSysView* view, CDC* deviceContext, std::int16_t color, std::int16_t lineTypeIndex) {
  SetPen(view, deviceContext, color, lineTypeIndex, std::wstring{});
}

void EoGsRenderState::SetPen(AeSysView* view, CDC* deviceContext, std::int16_t color, std::int16_t lineTypeIndex,
    const std::wstring& lineTypeName) {
  SetPen(view, deviceContext, color, lineTypeIndex, lineTypeName, EoDxfLineWeights::LineWeight::kLnWtByLwDefault, 1.0);
}

void EoGsRenderState::SetPen(AeSysView* view, CDC* deviceContext, std::int16_t color, std::int16_t lineTypeIndex,
    const std::wstring& lineTypeName, EoDxfLineWeights::LineWeight lineWeight, double lineTypeScale) {
  m_lineWeight = lineWeight;  // Store unresolved for UI readback
  if (EoDbPrimitive::SpecialColor() != 0) { color = EoDbPrimitive::SpecialColor(); }
  if (color == EoDbPrimitive::COLOR_BYLAYER) { color = EoDbPrimitive::LayerColor(); }
  if (lineTypeIndex == EoDbPrimitive::LINETYPE_BYLAYER) {
    lineTypeIndex = EoDbPrimitive::LayerLineTypeIndex();
    m_lineTypeName = EoDbPrimitive::LayerLineTypeName();
  } else {
    m_lineTypeName = lineTypeName;
  }

  // Resolve ByLayer line weight to the current layer's line weight
  if (lineWeight == EoDxfLineWeights::LineWeight::kLnWtByLayer) { lineWeight = EoDbPrimitive::LayerLineWeight(); }

  // Resolve ByLwDefault to the system default (0.25 mm)
  if (lineWeight == EoDxfLineWeights::LineWeight::kLnWtByLwDefault) {
    lineWeight = EoDxfLineWeights::LineWeight::kLnWt025;
  }

  // Resolve ByLayer linetype scale to the current layer's linetype scale
  if (lineTypeScale <= 0.0) { lineTypeScale = EoDbPrimitive::LayerLineTypeScale(); }

  m_color = color;
  m_LineTypeIndex = lineTypeIndex;
  m_lineTypeScale = lineTypeScale;

  double logicalWidth = 0.;

  if (view && view->PenWidthsOn()) {
    auto const logicalPixelsX = static_cast<double>(deviceContext->GetDeviceCaps(LOGPIXELSX));

    // Convert resolved line weight from DXF 0.01 mm units to screen pixels.
    // Zoom-independent — matches AutoCAD "Display Lineweight" model-space behavior.
    auto dxfCode = EoDxfLineWeights::LineWeightToDxfIndex(lineWeight);
    if (dxfCode > 0) { logicalWidth = dxfCode * (0.01 / 25.4) * logicalPixelsX; }
    logicalWidth = Eo::Round(logicalWidth);
  }
  if (deviceContext) { ManagePenResources(deviceContext, color, int(logicalWidth), lineTypeIndex); }
}

void EoGsRenderState::SetPen(
    AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t color, std::int16_t lineTypeIndex) {
  SetPen(view, renderDevice, color, lineTypeIndex, std::wstring{});
}

void EoGsRenderState::SetPen(AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t color,
    std::int16_t lineTypeIndex, const std::wstring& lineTypeName) {
  SetPen(view, renderDevice, color, lineTypeIndex, lineTypeName, EoDxfLineWeights::LineWeight::kLnWtByLwDefault, 1.0);
}

void EoGsRenderState::SetPen(AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t color,
    std::int16_t lineTypeIndex, const std::wstring& lineTypeName, EoDxfLineWeights::LineWeight lineWeight,
    double lineTypeScale) {
  m_lineWeight = lineWeight;  // Store unresolved for UI readback
  if (EoDbPrimitive::SpecialColor() != 0) { color = EoDbPrimitive::SpecialColor(); }
  if (color == EoDbPrimitive::COLOR_BYLAYER) { color = EoDbPrimitive::LayerColor(); }
  if (lineTypeIndex == EoDbPrimitive::LINETYPE_BYLAYER) {
    lineTypeIndex = EoDbPrimitive::LayerLineTypeIndex();
    m_lineTypeName = EoDbPrimitive::LayerLineTypeName();
  } else {
    m_lineTypeName = lineTypeName;
  }

  // Resolve ByLayer line weight to the current layer's line weight
  if (lineWeight == EoDxfLineWeights::LineWeight::kLnWtByLayer) { lineWeight = EoDbPrimitive::LayerLineWeight(); }

  // Resolve ByLwDefault to the system default (0.25 mm)
  if (lineWeight == EoDxfLineWeights::LineWeight::kLnWtByLwDefault) {
    lineWeight = EoDxfLineWeights::LineWeight::kLnWt025;
  }

  // Resolve ByLayer linetype scale to the current layer's linetype scale
  if (lineTypeScale <= 0.0) { lineTypeScale = EoDbPrimitive::LayerLineTypeScale(); }

  m_color = color;
  m_LineTypeIndex = lineTypeIndex;
  m_lineTypeScale = lineTypeScale;

  double logicalWidth = 0.;

  if (view && view->PenWidthsOn()) {
    auto const logicalPixelsX = static_cast<double>(renderDevice->GetDeviceCaps(LOGPIXELSX));

    auto dxfCode = EoDxfLineWeights::LineWeightToDxfIndex(lineWeight);
    if (dxfCode > 0) { logicalWidth = dxfCode * (0.01 / 25.4) * logicalPixelsX; }
    logicalWidth = Eo::Round(logicalWidth);
  }
  if (renderDevice) { ManagePenResources(renderDevice, color, int(logicalWidth), lineTypeIndex); }
}

void EoGsRenderState::ManagePenResources(
    CDC* deviceContext, std::int16_t penColor, int penWidth, std::int16_t lineType) {
  static const int numberOfPens = 8;
  static HPEN hPen[numberOfPens] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
  static COLORREF crColRef[numberOfPens];
  static std::int16_t lineTypes[numberOfPens];
  static int penWidths[numberOfPens];
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
  for (int i = 0; i < numberOfPens; i++) {
    if (hPen[i] && lineTypes[i] == lineType && penWidths[i] == penWidth && crColRef[i] == pColTbl[penColor]) {
      hPenCur = hPen[i];
      if (deviceContext) { deviceContext->SelectObject(CPen::FromHandle(hPenCur)); }
      return;
    }
    if (hPen[i] == nullptr) { iPen = i; }
  }
  HPEN newPenHandle = ::CreatePen(lineType, penWidth, pColTbl[penColor]);

  if (newPenHandle) {
    hPenCur = newPenHandle;
    if (deviceContext) { deviceContext->SelectObject(CPen::FromHandle(newPenHandle)); }
    if (hPen[iPen]) { ::DeleteObject(hPen[iPen]); }
    hPen[iPen] = newPenHandle;
    lineTypes[iPen] = lineType;
    penWidths[iPen] = penWidth;
    crColRef[iPen] = pColTbl[penColor];
  }
}

void EoGsRenderState::ManagePenResources(
    EoGsRenderDevice* renderDevice, std::int16_t penColor, int penWidth, std::int16_t lineType) {
  // Map internal lineType index to GDI pen style constant
  int penStyle;
  switch (lineType) {
    case 0:
      penStyle = PS_NULL;
      break;
    case 2:
      penStyle = PS_DOT;
      break;
    case 3:
      penStyle = PS_DASH;
      break;
    case 6:
      penStyle = PS_DASHDOT;
      break;
    case 7:
      penStyle = PS_DASHDOTDOT;
      break;
    default:
      penStyle = PS_SOLID;
  }
  if (renderDevice) {
    renderDevice->SetTextColor(pColTbl[penColor]);
    renderDevice->SelectPen(penStyle, penWidth, pColTbl[penColor]);
  }
}

void EoGsRenderState::SetColor(CDC* deviceContext, std::int16_t color) {
  m_color = color;
  if (deviceContext) { ManagePenResources(deviceContext, color, 0, m_LineTypeIndex); }
}

void EoGsRenderState::SetColor(EoGsRenderDevice* renderDevice, std::int16_t color) {
  m_color = color;
  if (renderDevice) { ManagePenResources(renderDevice, color, 0, m_LineTypeIndex); }
}

void EoGsRenderState::SetLineType(CDC* deviceContext, std::int16_t lineTypeIndex) {
  m_LineTypeIndex = lineTypeIndex;
  if (deviceContext) { ManagePenResources(deviceContext, m_color, 0, lineTypeIndex); }
}

void EoGsRenderState::SetLineType(EoGsRenderDevice* renderDevice, std::int16_t lineTypeIndex) {
  m_LineTypeIndex = lineTypeIndex;
  if (renderDevice) { ManagePenResources(renderDevice, m_color, 0, lineTypeIndex); }
}

/** @brief Sets the current foreground mix mode. GDI uses the foreground mix mode to combine pens and
 * interiors of filled objects with the colors already on the screen. The foreground mix mode
 * defines how colors from the brush or pen and the colors in the existing image are to be combined.
 *
 * @param deviceContext The device context to set the ROP2 mode for.
 * @param drawMode The drawing mode to set, such as R2_COPYPEN, R2_XORPEN, etc.
 * @return The previous ROP2 mode before setting the new one.
 * @note The behavior of ROP2 modes can be affected by the background color.
 * For example, XOR modes may not work properly with non black or white background colors, and this function includes
 * logic to handle such cases accordingly.
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

int EoGsRenderState::SetROP2(EoGsRenderDevice* renderDevice, int drawMode) {
  if (Eo::ColorPalette[0] == Eo::colorBlack) {
    return renderDevice->SetROP2(drawMode);
  } else if (Eo::ColorPalette[0] == Eo::colorWhite) {
    if (drawMode == R2_XORPEN) { drawMode = R2_NOTXORPEN; }
  } else {
    if (drawMode == R2_XORPEN || drawMode == R2_NOTXORPEN) { drawMode = R2_COPYPEN; }
  }
  return renderDevice->SetROP2(drawMode);
}

void EoGsRenderState::SetAlignment(
    CDC* deviceContext, EoDb::HorizontalAlignment horizontalAlignment, EoDb::VerticalAlignment verticalAlignment) {
  m_fontDefinition.SetAlignment(horizontalAlignment, verticalAlignment);

  deviceContext->SetTextAlign(TA_LEFT | TA_BASELINE);
}

void EoGsRenderState::SetAlignment(EoGsRenderDevice* renderDevice, EoDb::HorizontalAlignment horizontalAlignment,
    EoDb::VerticalAlignment verticalAlignment) {
  m_fontDefinition.SetAlignment(horizontalAlignment, verticalAlignment);

  renderDevice->SetTextAlign(TA_LEFT | TA_BASELINE);
}

void EoGsRenderState::SetFontDefinition(CDC* deviceContext, const EoDbFontDefinition& fontDefinition) {
  m_fontDefinition = fontDefinition;
  SetAlignment(deviceContext, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());
}

void EoGsRenderState::SetFontDefinition(EoGsRenderDevice* renderDevice, const EoDbFontDefinition& fontDefinition) {
  m_fontDefinition = fontDefinition;
  SetAlignment(renderDevice, m_fontDefinition.HorizontalAlignment(), m_fontDefinition.VerticalAlignment());
}
