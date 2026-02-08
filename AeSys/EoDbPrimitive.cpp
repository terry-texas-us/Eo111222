#include "Stdafx.h"

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

EoInt16 EoDbPrimitive::sm_layerColor{COLOR_BYLAYER};
EoInt16 EoDbPrimitive::sm_layerLineTypeIndex{LINETYPE_BYLAYER};
EoInt16 EoDbPrimitive::sm_specialColor{};

EoUInt16 EoDbPrimitive::sm_ControlPointIndex = USHRT_MAX;
double EoDbPrimitive::sm_RelationshipOfPoint = 0.0;
double EoDbPrimitive::sm_SelectApertureSize = 0.02;

EoDbPrimitive::EoDbPrimitive() : m_color(COLOR_BYLAYER), m_lineTypeIndex(LINETYPE_BYLAYER) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbPrimitive() CTOR: this=%p, vtable=%p\n", this, *(void**)this);
}
EoDbPrimitive::EoDbPrimitive(EoInt16 color, EoInt16 lineTypeIndex) : m_color(color), m_lineTypeIndex(lineTypeIndex) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbPrimitive(color,lt) CTOR: this=%p, vtable=%p\n", this,
            *(void**)this);
}

EoDbPrimitive::~EoDbPrimitive() {}

void EoDbPrimitive::CutAt2Points(const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*,
                              EoDbGroupList*) {
  (void)firstPoint;
  (void)secondPoint;
}
void EoDbPrimitive::CutAtPoint(EoGePoint3d& point, EoDbGroup* group) {(void) point; (void) group;}
int EoDbPrimitive::IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) { return 0; }
bool EoDbPrimitive::PivotOnControlPoint(AeSysView*, const EoGePoint4d&) { return false; }

void EoDbPrimitive::SetBaseProperties(const DRW_Entity* entity, AeSysDoc* document) {
  m_color = static_cast<EoInt16>(entity->color);
  m_lineTypeName = Eo::MultiByteToWString(entity->lineType.c_str());
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
  m_lineTypeIndex = linetypeTable->LegacyLineTypeIndex(m_lineTypeName);
}

CString EoDbPrimitive::FormatPenColor() const {
  CString str;
  if (m_color == COLOR_BYLAYER) {
    str = L"ByLayer";
  } else if (m_color == COLOR_BYBLOCK) {
    str = L"ByBlock";
  } else {
    wchar_t szBuf[16]{};
    _itow_s(m_color, szBuf, 16, 10);
    str = szBuf;
  }
  return str;
}
CString EoDbPrimitive::FormatLineType() const {
  CString str;
  if (m_lineTypeIndex == LINETYPE_BYLAYER) {
    str = L"ByLayer";
  } else if (m_lineTypeIndex == LINETYPE_BYBLOCK) {
    str = L"ByBlock";
  } else {
    wchar_t szBuf[16]{};
    _itow_s(m_lineTypeIndex, szBuf, 16, 10);
    str = szBuf;
  }
  return str;
}

EoInt16 EoDbPrimitive::LogicalColor() const {
  EoInt16 color = sm_specialColor == 0 ? m_color : sm_specialColor;

  if (color == COLOR_BYLAYER)
    color = sm_layerColor;
  else if (color == COLOR_BYBLOCK)
    color = 7;

  return color;
}

EoInt16 EoDbPrimitive::LogicalLineType() const {
  EoInt16 lineTypeIndex = m_lineTypeIndex;

  if (lineTypeIndex == LINETYPE_BYLAYER)
    lineTypeIndex = sm_layerLineTypeIndex;
  else if (lineTypeIndex == LINETYPE_BYBLOCK)
    lineTypeIndex = 1;

  return lineTypeIndex;
}

void EoDbPrimitive::ModifyState() {
  m_color = pstate.Color();
  m_lineTypeIndex = pstate.LineType();
}

EoUInt16 EoDbPrimitive::ControlPointIndex() { return sm_ControlPointIndex; }
bool EoDbPrimitive::IsSupportedTyp(int type) { return (type <= 7 && type != 4 && type != 5); }
EoInt16 EoDbPrimitive::LayerColor() { return sm_layerColor; }
void EoDbPrimitive::SetLayerColor(EoInt16 layerColor) { sm_layerColor = layerColor; }
EoInt16 EoDbPrimitive::LayerLineTypeIndex() { return sm_layerLineTypeIndex; }
void EoDbPrimitive::SetLayerLineTypeIndex(EoInt16 lineTypeIndex) { sm_layerLineTypeIndex = lineTypeIndex; }
double& EoDbPrimitive::Rel() { return sm_RelationshipOfPoint; }
EoInt16 EoDbPrimitive::SpecialColor() { return sm_specialColor; }
void EoDbPrimitive::SetSpecialColor(EoInt16 specialColor) { sm_specialColor = specialColor; }
