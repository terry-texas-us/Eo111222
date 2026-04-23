#include "Stdafx.h"

#include <cstdint>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbPrimitive.h"

EoDbLayer::EoDbLayer(const CString& name, State state)
    : m_name{name},
      m_state{static_cast<std::uint16_t>(state)},
      m_tracingState{},
      m_color{Eo::defaultColor},
      m_lineType{} {}

EoDbLayer::EoDbLayer(const CString& name, std::uint16_t state)
    : m_name{name}, m_state{state}, m_tracingState{}, m_color{Eo::defaultColor}, m_lineType{} {}

void EoDbLayer::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  EoDbPrimitive::SetLayerColor(ColorIndex());
  EoDbPrimitive::SetLayerLineTypeIndex(LineTypeIndex());
  EoDbPrimitive::SetLayerLineTypeName(m_lineType != nullptr ? std::wstring(m_lineType->Name()) : std::wstring{});
  EoDbPrimitive::SetLayerLineWeight(m_lineWeight);
  EoDbPrimitive::SetLayerLineTypeScale(m_lineTypeScale);

  COLORREF* pCurColTbl = pColTbl;

  pColTbl = (IsOpened() || IsWork() || IsActive()) ? Eo::ColorPalette : Eo::GrayPalette;

  EoDbGroupList::Display(view, renderDevice);
  pColTbl = pCurColTbl;
}
void EoDbLayer::Display(AeSysView* view, EoGsRenderDevice* renderDevice, bool identifyTrap) {
  ATLTRACE2(traceGeneral, 3, L"EoDbLayer<%p>::Display(%p, %p, %i) + Name: %ls\n", this, view, renderDevice,
      identifyTrap, static_cast<LPCWSTR>(this->Name()));

  auto* document = AeSysDoc::GetDoc();

  try {
    if (!IsOff()) {
      EoDbPrimitive::SetLayerColor(ColorIndex());
      EoDbPrimitive::SetLayerLineTypeIndex(LineTypeIndex());
      EoDbPrimitive::SetLayerLineTypeName(m_lineType != nullptr ? std::wstring(m_lineType->Name()) : std::wstring{});
      EoDbPrimitive::SetLayerLineWeight(m_lineWeight);
      EoDbPrimitive::SetLayerLineTypeScale(m_lineTypeScale);

      COLORREF* pCurColTbl = pColTbl;

      const bool LayerIsDetectable = IsOpened() || IsWork() || IsActive();

      pColTbl = LayerIsDetectable ? Eo::ColorPalette : Eo::GrayPalette;

      auto position = GetHeadPosition();
      while (position != nullptr) {
        auto* group = GetNext(position);

        if (group->IsInView(view)) {
          if (LayerIsDetectable) { document->AddGroupToAllViews(group); }
          if (identifyTrap && document->FindTrappedGroup(group) != nullptr) {
            EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor());
            group->Display(view, renderDevice);
            EoDbPrimitive::SetSpecialColor(0);
          } else {
            group->Display(view, renderDevice);
          }
        }
      }
      pColTbl = pCurColTbl;
    }
  } catch (CException* e) { e->Delete(); }
}

std::int16_t EoDbLayer::LineTypeIndex() const {
  const std::int16_t index = (m_lineType == nullptr ? 0 : m_lineType->Index());
  return index;
}

void EoDbLayer::PenTranslation(std::uint16_t wCols, std::int16_t* pColNew, std::int16_t* pCol) {
  for (int i = 0; i < wCols; i++) {
    if (m_color == pCol[i]) {
      m_color = pColNew[i];
      break;
    }
  }
  EoDbGroupList::PenTranslation(wCols, pColNew, pCol);
}
