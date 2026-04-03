#include "Stdafx.h"

#include <climits>
#include <cstdlib>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbHandleManager.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGsRenderState.h"

std::int16_t EoDbPrimitive::sm_layerColor{COLOR_BYLAYER};
std::int16_t EoDbPrimitive::sm_layerLineTypeIndex{LINETYPE_BYLAYER};
std::wstring EoDbPrimitive::sm_layerLineType{};
EoDxfLineWeights::LineWeight EoDbPrimitive::sm_layerLineWeight{EoDxfLineWeights::LineWeight::kLnWtByLwDefault};
double EoDbPrimitive::sm_layerLineTypeScale{1.0};
std::int16_t EoDbPrimitive::sm_specialColor{};

int EoDbPrimitive::sm_controlPointIndex{SHRT_MAX};
double EoDbPrimitive::sm_RelationshipOfPoint{};
double EoDbPrimitive::sm_SelectApertureSize{0.02};
EoDbHandleManager* EoDbPrimitive::sm_handleManager{nullptr};

EoDbPrimitive::EoDbPrimitive() {
  if (sm_handleManager != nullptr) { m_handle = sm_handleManager->AssignHandle(); }
}

EoDbPrimitive::EoDbPrimitive(const EoDbPrimitive& other)
    : m_color(other.m_color),
      m_lineType(other.m_lineType),
      m_layerName(other.m_layerName),
      m_ownerHandle(other.m_ownerHandle),
      m_thickness(other.m_thickness),
      m_lineWeight(other.m_lineWeight),
      m_lineTypeScale(other.m_lineTypeScale) {
  if (sm_handleManager != nullptr) { m_handle = sm_handleManager->AssignHandle(); }
}

  EoDbPrimitive& EoDbPrimitive::operator=(const EoDbPrimitive& other) {
  if (this != &other) {
    m_color = other.m_color;
    m_lineType = other.m_lineType;
    m_layerName = other.m_layerName;
    // m_handle is intentionally NOT copied — entity identity is preserved
    m_ownerHandle = other.m_ownerHandle;
    m_thickness = other.m_thickness;
    m_lineWeight = other.m_lineWeight;
    m_lineTypeScale = other.m_lineTypeScale;
  }
  return *this;
}

EoDbPrimitive::EoDbPrimitive(std::int16_t color, std::int16_t lineTypeIndex)
    : m_color(color), m_lineType(EoDbLineTypeTable::LegacyLineTypeName(lineTypeIndex)) {
  if (sm_handleManager != nullptr) { m_handle = sm_handleManager->AssignHandle(); }
  ATLTRACE2(traceGeneral, 3, L"EoDbPrimitive(color, lineTypeIndex) CTOR: this=%p, vtable=%p\n", this, *(void**)this);
}

EoDbPrimitive::~EoDbPrimitive() {}

EoDbPrimitive* EoDbPrimitive::WithProperties(const EoGsRenderState& renderState) {
  m_color = renderState.Color();
  SetLineTypeName(renderState.LineTypeName());
  m_lineWeight = renderState.LineWeight();
  return this;
}

EoDbPrimitive* EoDbPrimitive::WithProperties(
    std::int16_t color, const std::wstring& lineTypeName, EoDxfLineWeights::LineWeight lineWeight) {
  m_color = color;
  SetLineTypeName(lineTypeName);
  m_lineWeight = lineWeight;
  return this;
}

void EoDbPrimitive::SetLineTypeName(std::wstring name) {
  m_lineType = std::move(name);
}

void EoDbPrimitive::CutAt2Points([[maybe_unused]] const EoGePoint3d& firstPoint,
    [[maybe_unused]] const EoGePoint3d& secondPoint, [[maybe_unused]] EoDbGroupList*, [[maybe_unused]] EoDbGroupList*) {
}
void EoDbPrimitive::CutAtPoint([[maybe_unused]] const EoGePoint3d& point, [[maybe_unused]] EoDbGroup* group) {}
int EoDbPrimitive::IsWithinArea(const EoGePoint3d&, const EoGePoint3d&, EoGePoint3d*) { return 0; }
bool EoDbPrimitive::PivotOnControlPoint(AeSysView*, const EoGePoint4d&) { return false; }

void EoDbPrimitive::SetBaseProperties(const EoDxfGraphic* entity, [[maybe_unused]] AeSysDoc* document) {
  m_color = static_cast<std::int16_t>(entity->m_color);
  m_lineType = entity->m_lineType.c_str();
  m_layerName = entity->m_layer.c_str();

  // Determine actual PenColor and LineType and where to do substitution
  // for PenColor ByLayer
  // get color index from parent Layer
  // for PenColor ByBlock
  // get color index from parent Block;

  // for lineType ByLayer
  // get linetype from parent Layer
  // for linetypeName ByBlock
  // get lineType from parent Block;

  m_handle = entity->m_handle;
  if (sm_handleManager != nullptr) { sm_handleManager->AccommodateHandle(m_handle); }
  m_ownerHandle = entity->m_ownerHandle;
  m_thickness = entity->m_thickness;
  m_lineWeight = entity->m_lineWeight;
  m_lineTypeScale = entity->m_lineTypeScale;
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
  if (IsLineTypeByLayer()) {
    str = L"ByLayer";
  } else if (IsLineTypeByBlock()) {
    str = L"ByBlock";
  } else {
    str = m_lineType.c_str();
  }
  return str;
}

std::int16_t EoDbPrimitive::LogicalColor() const noexcept {
  std::int16_t color = sm_specialColor == 0 ? m_color : sm_specialColor;

  if (color == COLOR_BYLAYER) {
    color = sm_layerColor;
  } else if (color == COLOR_BYBLOCK) {
    color = 7;
  }

  return color;
}

std::int16_t EoDbPrimitive::LogicalLineType() const {
  if (IsLineTypeByLayer()) {
    return sm_layerLineTypeIndex;
  }
  if (IsLineTypeByBlock()) {
    return 1;  // CONTINUOUS
  }
  return EoDbLineTypeTable::LegacyLineTypeIndex(m_lineType);
}

const std::wstring& EoDbPrimitive::LogicalLineTypeName() const {
  if (IsLineTypeByLayer()) { return sm_layerLineType; }
  return m_lineType;
}

void EoDbPrimitive::PopulateDxfBaseProperties(EoDxfGraphic* entity) const {
  entity->m_layer = m_layerName;
  entity->m_color = m_color;
  entity->m_handle = m_handle;
  entity->m_ownerHandle = m_ownerHandle;

  if (IsLineTypeByLayer()) {
    entity->m_lineType = L"BYLAYER";
  } else if (IsLineTypeByBlock()) {
    entity->m_lineType = L"BYBLOCK";
  } else if (!m_lineType.empty()) {
    entity->m_lineType = m_lineType;
  }
  entity->m_thickness = m_thickness;
  entity->m_lineWeight = m_lineWeight;
  entity->m_lineTypeScale = m_lineTypeScale;
}

void EoDbPrimitive::ExportToDxf([[maybe_unused]] EoDxfInterface* writer) const {}

void EoDbPrimitive::WriteV2Extension([[maybe_unused]] CFile& file) const {}
void EoDbPrimitive::ReadV2Extension([[maybe_unused]] CFile& file) {}

void EoDbPrimitive::ModifyState() {
  m_color = Gs::renderState.Color();
  SetLineTypeName(Gs::renderState.LineTypeName());
  m_lineWeight = Gs::renderState.LineWeight();
}

void EoDbPrimitive::FormatExtra(CString& extra) {
  extra.Format(L"Handle;%I64X\tOwner;%I64X\tLayer;%s\tColor;%s\tLineType;%s\tLineWeight;%hd\tLineTypeScale;%g", m_handle, m_ownerHandle,
      m_layerName.empty() ? L"" : m_layerName.c_str(), FormatPenColor().GetString(), FormatLineType().GetString(),
      static_cast<std::int16_t>(m_lineWeight), m_lineTypeScale);
}

void EoDbPrimitive::AddReportToMessageList(const EoGePoint3d&) {
  CString message;
  message.Format(L"Handle: %I64X  Owner: %I64X  Layer: %s  Color: %s  LineType: %s  LineWeight: %hd  LineTypeScale: %g", m_handle,
      m_ownerHandle, m_layerName.empty() ? L"" : m_layerName.c_str(), FormatPenColor().GetString(),
      FormatLineType().GetString(), static_cast<std::int16_t>(m_lineWeight), m_lineTypeScale);
  app.AddStringToMessageList(message);
}

int EoDbPrimitive::ControlPointIndex() noexcept { return sm_controlPointIndex; }
bool EoDbPrimitive::IsSupportedTyp(int type) noexcept { return (type <= 7 && type != 4 && type != 5); }
std::int16_t EoDbPrimitive::LayerColor() noexcept { return sm_layerColor; }
void EoDbPrimitive::SetLayerColor(std::int16_t layerColor) noexcept { sm_layerColor = layerColor; }
std::int16_t EoDbPrimitive::LayerLineTypeIndex() noexcept { return sm_layerLineTypeIndex; }
void EoDbPrimitive::SetLayerLineTypeIndex(std::int16_t lineTypeIndex) noexcept {
  sm_layerLineTypeIndex = lineTypeIndex;
}
const std::wstring& EoDbPrimitive::LayerLineTypeName() noexcept { return sm_layerLineType; }
void EoDbPrimitive::SetLayerLineTypeName(const std::wstring& lineTypeName) { sm_layerLineType = lineTypeName; }
EoDxfLineWeights::LineWeight EoDbPrimitive::LayerLineWeight() noexcept { return sm_layerLineWeight; }
void EoDbPrimitive::SetLayerLineWeight(EoDxfLineWeights::LineWeight lineWeight) noexcept {
  sm_layerLineWeight = lineWeight;
}
double EoDbPrimitive::LayerLineTypeScale() noexcept { return sm_layerLineTypeScale; }
void EoDbPrimitive::SetLayerLineTypeScale(double lineTypeScale) noexcept { sm_layerLineTypeScale = lineTypeScale; }
double& EoDbPrimitive::Rel() noexcept { return sm_RelationshipOfPoint; }
std::int16_t EoDbPrimitive::SpecialColor() noexcept { return sm_specialColor; }
void EoDbPrimitive::SetSpecialColor(std::int16_t specialColor) noexcept { sm_specialColor = specialColor; }
void EoDbPrimitive::SetHandleManager(EoDbHandleManager* handleManager) noexcept { sm_handleManager = handleManager; }

EoDbHandleManager* EoDbPrimitive::SuspendHandleAssignment() noexcept {
  auto* saved = sm_handleManager;
  sm_handleManager = nullptr;
  return saved;
}

void EoDbPrimitive::ResumeHandleAssignment(EoDbHandleManager* saved) noexcept { sm_handleManager = saved; }