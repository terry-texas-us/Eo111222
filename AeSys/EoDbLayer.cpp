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
    : m_name{name}, m_state{static_cast<std::uint16_t>(state)}, m_tracingState{}, m_color{1}, m_lineType{} {}

EoDbLayer::EoDbLayer(const CString& name, std::uint16_t state)
    : m_name{name}, m_state{state}, m_tracingState{}, m_color{1}, m_lineType{} {}

[[nodiscard]] COLORREF EoDbLayer::ColorValue() const { return ColorPalette[m_color]; }

void EoDbLayer::Display(AeSysView* view, CDC* deviceContext) {
  EoDbPrimitive::SetLayerColor(ColorIndex());
  EoDbPrimitive::SetLayerLineTypeIndex(LineTypeIndex());

  COLORREF* pCurColTbl = pColTbl;

  pColTbl = (IsOpened() || IsWork() || IsActive()) ? ColorPalette : GreyPalette;

  EoDbGroupList::Display(view, deviceContext);
  pColTbl = pCurColTbl;
}
void EoDbLayer::Display(AeSysView* view, CDC* deviceContext, bool identifyTrap) {
  ATLTRACE2(traceGeneral, 3, L"EoDbLayer<%p>::Display(%p, %p, %i) + Name: %ls\n", this, view, deviceContext,
      identifyTrap, static_cast<LPCWSTR>(this->Name()));

  auto* document = AeSysDoc::GetDoc();

  try {
    if (!IsOff()) {
      EoDbPrimitive::SetLayerColor(ColorIndex());
      EoDbPrimitive::SetLayerLineTypeIndex(LineTypeIndex());

      COLORREF* pCurColTbl = pColTbl;

      bool LayerIsDetectable = IsOpened() || IsWork() || IsActive();

      pColTbl = LayerIsDetectable ? ColorPalette : GreyPalette;

      auto position = GetHeadPosition();
      while (position != nullptr) {
        auto* group = GetNext(position);

        if (group->IsInView(view)) {
          if (LayerIsDetectable) { document->AddGroupToAllViews(group); }
          if (identifyTrap && document->FindTrappedGroup(group) != 0) {
            EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor());
            group->Display(view, deviceContext);
            EoDbPrimitive::SetSpecialColor(0);
          } else {
            group->Display(view, deviceContext);
          }
        }
      }
      pColTbl = pCurColTbl;
    }
  } catch (CException* e) { e->Delete(); }
}

[[nodiscard]] std::int16_t EoDbLayer::LineTypeIndex() const {
  std::int16_t index = (m_lineType == nullptr ? 0 : m_lineType->Index());
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
