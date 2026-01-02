#include "stdafx.h"

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbPrimitive.h"

EoDbLayer::EoDbLayer(const CString& name, EoUInt16 stateFlags) {
  m_Name = name;
  m_TracingFlgs = 0;
  m_StateFlags = stateFlags;
  m_ColorIndex = 1;
  m_LineType = nullptr;
}
EoDbLayer::EoDbLayer(const CString& name, EoUInt16 stateFlags, EoDbLineType* lineType) {
  m_Name = name;
  m_TracingFlgs = 0;
  m_StateFlags = stateFlags;
  m_ColorIndex = 1;
  m_LineType = lineType;
}
COLORREF EoDbLayer::Color() const { return ColorPalette[m_ColorIndex]; }
void EoDbLayer::Display(AeSysView* view, CDC* deviceContext) {
  EoDbPrimitive::SetLayerPenColorIndex(ColorIndex());
  EoDbPrimitive::SetLayerLineTypeIndex(LineTypeIndex());

  COLORREF* pCurColTbl = pColTbl;

  pColTbl = (IsOpened() || IsWork() || IsActive()) ? ColorPalette : GreyPalette;

  EoDbGroupList::Display(view, deviceContext);
  pColTbl = pCurColTbl;
}
void EoDbLayer::Display(AeSysView* view, CDC* deviceContext, bool identifyTrap) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"EoDbLayer<%p>::Display(%p, %p, %i) + Name: %ls\n", this, view,
            deviceContext, identifyTrap, static_cast<LPCWSTR>(this->Name()));

  auto* document = AeSysDoc::GetDoc();

  try {
    if (!IsOff()) {
      EoDbPrimitive::SetLayerPenColorIndex(ColorIndex());
      EoDbPrimitive::SetLayerLineTypeIndex(LineTypeIndex());

      COLORREF* pCurColTbl = pColTbl;

      bool LayerIsDetectable = IsOpened() || IsWork() || IsActive();

      pColTbl = LayerIsDetectable ? ColorPalette : GreyPalette;

      auto position = GetHeadPosition();
      while (position != 0) {
        EoDbGroup* Group = GetNext(position);

        if (Group->IsInView(view)) {
          if (LayerIsDetectable) { document->AddGroupToAllViews(Group); }
          if (identifyTrap && document->FindTrappedGroup(Group) != 0) {
            EoDbPrimitive::SetSpecialPenColorIndex(app.TrapHighlightColor());
            Group->Display(view, deviceContext);
            EoDbPrimitive::SetSpecialPenColorIndex(0);
          } else {
            Group->Display(view, deviceContext);
          }
        }
      }
      pColTbl = pCurColTbl;
    }
  } catch (CException* e) { e->Delete(); }
}
EoDbLineType* EoDbLayer::LineType() const { return m_LineType; }
EoInt16 EoDbLayer::LineTypeIndex() { return (static_cast<EoInt16>(m_LineType->Index())); }
void EoDbLayer::PenTranslation(EoUInt16 wCols, EoInt16* pColNew, EoInt16* pCol) {
  for (int i = 0; i < wCols; i++) {
    if (m_ColorIndex == pCol[i]) {
      m_ColorIndex = pColNew[i];
      break;
    }
  }
  EoDbGroupList::PenTranslation(wCols, pColNew, pCol);
}
void EoDbLayer::SetLineType(EoDbLineType* lineType) { m_LineType = lineType; }
