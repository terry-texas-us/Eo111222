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
#include "EoGsRenderState.h"
#include "drw_Entities.h"

std::int16_t EoDbPrimitive::sm_layerColor{COLOR_BYLAYER};
std::int16_t EoDbPrimitive::sm_layerLineTypeIndex{LINETYPE_BYLAYER};
std::int16_t EoDbPrimitive::sm_specialColor{};

int EoDbPrimitive::sm_controlPointIndex{SHRT_MAX};
double EoDbPrimitive::sm_RelationshipOfPoint{};
double EoDbPrimitive::sm_SelectApertureSize{0.02};

EoDbPrimitive::EoDbPrimitive(std::int16_t color, std::int16_t lineTypeIndex)
    : m_color(color), m_lineTypeIndex(lineTypeIndex) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"EoDbPrimitive(color, lineTypeIndex) CTOR: this=%p, vtable=%p\n",
      this, *(void**)this);
}

EoDbPrimitive::~EoDbPrimitive() {}

void EoDbPrimitive::CutAt2Points([[maybe_unused]] const EoGePoint3d& firstPoint,
    [[maybe_unused]] const EoGePoint3d& secondPoint, [[maybe_unused]] EoDbGroupList*, [[maybe_unused]] EoDbGroupList*) {
}
void EoDbPrimitive::CutAtPoint([[maybe_unused]] const EoGePoint3d& point, [[maybe_unused]] EoDbGroup* group) {}
int EoDbPrimitive::IsWithinArea(const EoGePoint3d&, const EoGePoint3d&, EoGePoint3d*) { return 0; }
bool EoDbPrimitive::PivotOnControlPoint(AeSysView*, const EoGePoint4d&) { return false; }

void EoDbPrimitive::SetBaseProperties(const DRW_Entity* entity, AeSysDoc* document) {
  m_color = static_cast<std::int16_t>(entity->color);
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

std::int16_t EoDbPrimitive::LogicalColor() const noexcept {
  std::int16_t color = sm_specialColor == 0 ? m_color : sm_specialColor;

  if (color == COLOR_BYLAYER)
    color = sm_layerColor;
  else if (color == COLOR_BYBLOCK)
    color = 7;

  return color;
}

std::int16_t EoDbPrimitive::LogicalLineType() const noexcept {
  std::int16_t lineTypeIndex = m_lineTypeIndex;

  if (lineTypeIndex == LINETYPE_BYLAYER)
    lineTypeIndex = sm_layerLineTypeIndex;
  else if (lineTypeIndex == LINETYPE_BYBLOCK)
    lineTypeIndex = 1;

  return lineTypeIndex;
}

void EoDbPrimitive::ModifyState() {
  m_color = renderState.Color();
  m_lineTypeIndex = renderState.LineTypeIndex();
}

int EoDbPrimitive::ControlPointIndex() noexcept { return sm_controlPointIndex; }
bool EoDbPrimitive::IsSupportedTyp(int type) noexcept { return (type <= 7 && type != 4 && type != 5); }
std::int16_t EoDbPrimitive::LayerColor() noexcept { return sm_layerColor; }
void EoDbPrimitive::SetLayerColor(std::int16_t layerColor) noexcept { sm_layerColor = layerColor; }
std::int16_t EoDbPrimitive::LayerLineTypeIndex() noexcept { return sm_layerLineTypeIndex; }
void EoDbPrimitive::SetLayerLineTypeIndex(std::int16_t lineTypeIndex) noexcept { sm_layerLineTypeIndex = lineTypeIndex; }
double& EoDbPrimitive::Rel() noexcept { return sm_RelationshipOfPoint; }
std::int16_t EoDbPrimitive::SpecialColor() noexcept { return sm_specialColor; }
void EoDbPrimitive::SetSpecialColor(std::int16_t specialColor) noexcept { sm_specialColor = specialColor; }