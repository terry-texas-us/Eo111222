#include "Stdafx.h"

#include <afxstr.h>
#include <climits>
#include <cstdlib>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "PrimState.h"
#include "drw_Entities.h"

EoInt16 EoDbPrimitive::sm_LayerPenColor = 1;
EoInt16 EoDbPrimitive::sm_LayerLineType = 1;
EoInt16 EoDbPrimitive::sm_SpecialLineTypeIndex = 0;
EoInt16 EoDbPrimitive::sm_SpecialPenColorIndex = 0;

EoUInt16 EoDbPrimitive::sm_ControlPointIndex = USHRT_MAX;
double EoDbPrimitive::sm_RelationshipOfPoint = 0.;
double EoDbPrimitive::sm_SelectApertureSize = 0.02;

EoDbPrimitive::EoDbPrimitive() : m_PenColor(PENCOLOR_BYLAYER), m_LineType(LINETYPE_BYLAYER) {}
EoDbPrimitive::EoDbPrimitive(EoInt16 penColor, EoInt16 lineType) : m_PenColor(penColor), m_LineType(lineType) {}

EoDbPrimitive::~EoDbPrimitive() {}

void EoDbPrimitive::CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*) {}
void EoDbPrimitive::CutAtPt(EoGePoint3d&, EoDbGroup*) {}
int EoDbPrimitive::IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) { return 0; }
bool EoDbPrimitive::PvtOnCtrlPt(AeSysView*, const EoGePoint4d&) { return false; }

void EoDbPrimitive::SetBaseProperties(const DRW_Entity* entity, AeSysDoc* document) {
  m_PenColor = static_cast<EoInt16>(entity->color);
  m_linetypeName = Eo::MultiByteToWString(entity->lineType.c_str());
  m_layerName = Eo::MultiByteToWString(entity->layer.c_str());

  // Determine actual PenColor and LineType and where to do substitution
  // for PenColor ByLayer
  // get color index from parent Layer
  // for PenColor ByBlock
  // get color index from parent Block;

  // for lineType ByLayer
  // get linetype from parent Layer
  // for linetypeName ByBlock
  // get lineType from parent Block;

  auto* linetypeTable = document->LineTypeTable();
  m_LineType = linetypeTable->LegacyLineTypeIndex(m_linetypeName);
}

CString EoDbPrimitive::FormatPenColor() const {
  CString str;
  if (m_PenColor == PENCOLOR_BYLAYER) {
    str = L"ByLayer";
  } else if (m_PenColor == PENCOLOR_BYBLOCK) {
    str = L"ByBlock";
  } else {
    wchar_t szBuf[16]{};
    _itow_s(m_PenColor, szBuf, 16, 10);
    str = szBuf;
  }
  return str;
}
CString EoDbPrimitive::FormatLineType() const {
  CString str;
  if (m_LineType == LINETYPE_BYLAYER) {
    str = L"ByLayer";
  } else if (m_LineType == LINETYPE_BYBLOCK) {
    str = L"ByBlock";
  } else {
    wchar_t szBuf[16]{};
    _itow_s(m_LineType, szBuf, 16, 10);
    str = szBuf;
  }
  return str;
}
EoInt16 EoDbPrimitive::LogicalPenColor() const {
  EoInt16 nPenColor = sm_SpecialPenColorIndex == 0 ? m_PenColor : sm_SpecialPenColorIndex;

  if (nPenColor == PENCOLOR_BYLAYER)
    nPenColor = sm_LayerPenColor;
  else if (nPenColor == PENCOLOR_BYBLOCK)
    nPenColor = 7;

  return (nPenColor);
}
EoInt16 EoDbPrimitive::LogicalLineType() const {
  EoInt16 LineType = sm_SpecialLineTypeIndex == 0 ? m_LineType : sm_SpecialLineTypeIndex;

  if (LineType == LINETYPE_BYLAYER)
    LineType = sm_LayerLineType;
  else if (LineType == LINETYPE_BYBLOCK)
    LineType = 1;

  return (LineType);
}
void EoDbPrimitive::ModifyState() {
  m_PenColor = pstate.PenColor();
  m_LineType = pstate.LineType();
}
EoInt16 EoDbPrimitive::PenColor() const { return m_PenColor; }
EoInt16 EoDbPrimitive::LineType() const { return m_LineType; }
void EoDbPrimitive::PenColor(EoInt16 penColor) { m_PenColor = penColor; }
void EoDbPrimitive::LineType(EoInt16 lineType) { m_LineType = lineType; }

EoUInt16 EoDbPrimitive::ControlPointIndex() { return sm_ControlPointIndex; }
bool EoDbPrimitive::IsSupportedTyp(int iTyp) { return (iTyp <= 7 && iTyp != 4 && iTyp != 5); }
EoInt16 EoDbPrimitive::LayerPenColorIndex() { return sm_LayerPenColor; }
void EoDbPrimitive::SetLayerPenColorIndex(EoInt16 colorIndex) { sm_LayerPenColor = colorIndex; }
EoInt16 EoDbPrimitive::LayerLineTypeIndex() { return sm_LayerLineType; }
void EoDbPrimitive::SetLayerLineTypeIndex(EoInt16 lineTypeIndex) { sm_LayerLineType = lineTypeIndex; }
double& EoDbPrimitive::Rel() { return sm_RelationshipOfPoint; }
EoInt16 EoDbPrimitive::SpecialLineTypeIndex() { return sm_SpecialLineTypeIndex; }
EoInt16 EoDbPrimitive::SpecialPenColorIndex() { return sm_SpecialPenColorIndex; }
void EoDbPrimitive::SetSpecialPenColorIndex(EoInt16 colorIndex) { sm_SpecialPenColorIndex = colorIndex; }
