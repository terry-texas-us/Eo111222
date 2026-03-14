#include <algorithm>
#include <cmath>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfHatch.h"
#include "EoDxfReader.h"

namespace {

/** @brief Helper function to parse application-defined group (code 102) and its associated data until the closing tag
 * is reached.
 *  @param reader pointer to EoDxfReader to read value
 *  @return true if group is successfully parsed, false if group is not recognized or an error occurs
 *
 *  This function reads the application-defined group code (102) and its associated data,
 *  which can include various types of values based on the DXF specification.
 *  It continues reading until it encounters the closing tag for the group.
 *  The parsed values are stored in an EoDxfGroupCodeValuesVariant, which can hold different types of data
 *  based on the group code ranges defined in the DXF format.
 */
bool AddAppDataValue(EoDxfGroupCodeValuesVariant& variant, int code, EoDxfReader* reader) {
  if (code == 330 || code == 360) {
    variant.AddHandle(code, reader->GetHandleString());
    return true;
  }
  if (code <= 9 || code == 100 || code == 102 || code == 105 || (code >= 300 && code < 370) ||
      (code >= 390 && code < 400) || (code >= 410 && code < 420) || (code >= 430 && code < 440) ||
      (code >= 470 && code < 481) || code == 999 || (code >= 1000 && code <= 1009)) {
    variant.AddWideString(code, reader->GetWideString());
    return true;
  }
  if ((code >= 10 && code < 60) || (code >= 110 && code < 150) || (code >= 210 && code < 240) ||
      (code >= 460 && code < 470) || (code >= 1010 && code <= 1059)) {
    variant.AddDouble(code, reader->GetDouble());
    return true;
  }
  if ((code >= 60 && code < 80) || (code >= 170 && code < 180) || (code >= 270 && code < 290) ||
      (code >= 370 && code < 390) || (code >= 400 && code < 410) || (code >= 1060 && code <= 1070)) {
    variant.AddInt16(code, reader->GetInt16());
    return true;
  }
  if ((code >= 90 && code < 100) || (code >= 420 && code < 430) || (code >= 440 && code < 460) || code == 1071) {
    variant.AddInt32(code, reader->GetInt32());
    return true;
  }
  if (code >= 160 && code < 170) {
    variant.AddInt64(code, reader->GetInt64());
    return true;
  }
  if (code >= 290 && code < 300) {
    variant.AddBoolean(code, reader->GetBool());
    return true;
  }
  return false;
}
}  // namespace

EoDxfGraphic::EoDxfGraphic(const EoDxfGraphic& other)
    : EoDxfEntity{other},
      m_layer{other.m_layer},
      m_lineType{other.m_lineType},
      m_proxyEntityGraphicsData{other.m_proxyEntityGraphicsData},
      m_colorName{other.m_colorName},
      m_lineTypeScale{other.m_lineTypeScale},
      m_handle{other.m_handle},
      m_ownerHandle{other.m_ownerHandle},
      m_materialHandle{other.m_materialHandle},
      m_color{other.m_color},
      m_lineWeight{other.m_lineWeight},
      m_numberOfBytesInProxyGraphics{other.m_numberOfBytesInProxyGraphics},
      m_color24{other.m_color24},
      m_transparency{other.m_transparency},
      m_plotStyleHandle{other.m_plotStyleHandle},
      m_shadowMode{other.m_shadowMode},
      m_space{other.m_space},
      m_visibilityFlag{other.m_visibilityFlag},
      m_haveExtrusion{other.m_haveExtrusion},
      m_currentVariant{nullptr} {
  m_extendedData.reserve(other.m_extendedData.size());
  for (const auto* v : other.m_extendedData) { m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(*v)); }
}

EoDxfGraphic& EoDxfGraphic::operator=(const EoDxfGraphic& other) {
  if (this != &other) {
    EoDxfEntity::operator=(other);
    clearExtendedData();

    m_layer = other.m_layer;
    m_lineType = other.m_lineType;
    m_proxyEntityGraphicsData = other.m_proxyEntityGraphicsData;
    m_colorName = other.m_colorName;
    m_lineTypeScale = other.m_lineTypeScale;
    m_handle = other.m_handle;
    m_ownerHandle = other.m_ownerHandle;
    m_materialHandle = other.m_materialHandle;
    m_color = other.m_color;
    m_lineWeight = other.m_lineWeight;
    m_numberOfBytesInProxyGraphics = other.m_numberOfBytesInProxyGraphics;
    m_color24 = other.m_color24;
    m_transparency = other.m_transparency;
    m_plotStyleHandle = other.m_plotStyleHandle;
    m_shadowMode = other.m_shadowMode;
    m_space = other.m_space;
    m_visibilityFlag = other.m_visibilityFlag;
    m_haveExtrusion = other.m_haveExtrusion;
    m_currentVariant = nullptr;

    m_extendedData.reserve(other.m_extendedData.size());
    for (const auto* v : other.m_extendedData) { m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(*v)); }
  }
  return *this;
}

EoDxfGraphic::~EoDxfGraphic() { clearExtendedData(); }

void EoDxfGraphic::Clear() {
  clearExtendedData();
  // extend this later for more state reset if needed
}

void EoDxfGraphic::clearExtendedData() noexcept {
  for (auto* v : m_extendedData) { delete v; }
  m_extendedData.clear();
}

void EoDxfGraphic::CalculateArbitraryAxis(const EoDxfGeometryBase3d& extrusionDirection) {
  // Follow the arbitrary DXF definitions for extrusion axes.
  if (fabs(extrusionDirection.x) < 0.015625 && fabs(extrusionDirection.y) < 0.015625) {
    // If we get here, implement Ax = Wy x N where Wy is [0,1,0] per the DXF spec.
    // The cross product works out to Wy.y*N.z-Wy.z*N.y, Wy.z*N.x-Wy.x*N.z, Wy.x*N.y-Wy.y*N.x
    // Factoring in the fixed values for Wy gives N.z,0,-N.x
    extAxisX.x = extrusionDirection.z;
    extAxisX.y = 0;
    extAxisX.z = -extrusionDirection.x;
  } else {
    // Otherwise, implement Ax = Wz x N where Wz is [0,0,1] per the DXF spec.
    // The cross product works out to Wz.y*N.z-Wz.z*N.y, Wz.z*N.x-Wz.x*N.z, Wz.x*N.y-Wz.y*N.x
    // Factoring in the fixed values for Wz gives -N.y,N.x,0.
    extAxisX.x = -extrusionDirection.y;
    extAxisX.y = extrusionDirection.x;
    extAxisX.z = 0;
  }

  extAxisX.Unitize();

  // Ay = N x Ax
  extAxisY.x = (extrusionDirection.y * extAxisX.z) - (extAxisX.y * extrusionDirection.z);
  extAxisY.y = (extrusionDirection.z * extAxisX.x) - (extAxisX.z * extrusionDirection.x);
  extAxisY.z = (extrusionDirection.x * extAxisX.y) - (extAxisX.x * extrusionDirection.y);

  extAxisY.Unitize();
}

void EoDxfGraphic::ExtrudePointInPlace(
    const EoDxfGeometryBase3d& extrusionDirection, EoDxfGeometryBase3d& point) const noexcept {
  double px = (extAxisX.x * point.x) + (extAxisY.x * point.y) + (extrusionDirection.x * point.z);
  double py = (extAxisX.y * point.x) + (extAxisY.y * point.y) + (extrusionDirection.y * point.z);
  double pz = (extAxisX.z * point.x) + (extAxisY.z * point.y) + (extrusionDirection.z * point.z);

  point.x = px;
  point.y = py;
  point.z = pz;
}

void EoDxfGraphic::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 5:
      m_handle = reader->GetHandleString();
      break;
    case 330:
      m_ownerHandle = reader->GetHandleString();
      break;
    case 8:
      m_layer = reader->GetWideString();
      break;
    case 6:
      m_lineType = reader->GetWideString();
      break;
    case 62:
      m_color = reader->GetInt16();
      break;
    case 370:
      m_lineWeight = EoDxfLineWidths::dxfInt2lineWidth(reader->GetInt16());
      break;
    case 48:
      m_lineTypeScale = reader->GetDouble();
      break;
    case 60:
      m_visibilityFlag = reader->GetInt16();
      break;
    case 39:
      m_thickness = reader->GetDouble();
      break;
    case 210:
      m_haveExtrusion = true;
      m_extrusionDirection.x = reader->GetDouble();
      break;
    case 220:
      m_extrusionDirection.y = reader->GetDouble();
      break;
    case 230:
      m_extrusionDirection.z = reader->GetDouble();
      break;
    case 284:
      // @bug possible: 284 is a bitmask using 8-bit integer values, reading as int32 for simplicity
      m_shadowMode = static_cast<EoDxf::ShadowMode>(reader->GetInt16());
      break;
    case 390:
      m_plotStyleHandle = reader->GetHandleString();
      break;
    case 420:
      m_color24 = reader->GetInt32();
      break;
    case 430:
      m_colorName = reader->GetWideString();
      break;
    case 440:
      m_transparency = static_cast<EoDxf::TransparencyCodes>(reader->GetInt32());
      break;
    case 67:
      m_space = static_cast<EoDxf::Space>(reader->GetInt16());
      break;
    case 102:
      ParseAppDataGroup(reader);
      break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
      m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetWideString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
      m_currentVariant = new EoDxfGroupCodeValuesVariant(code, EoDxfGeometryBase3d(reader->GetDouble(), 0.0, 0.0));
      m_extendedData.push_back(m_currentVariant);
      break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseY(reader->GetDouble()); }
      break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseZ(reader->GetDouble()); }
      m_currentVariant = nullptr;
      break;
    case 1040:
    case 1041:
    case 1042:
      m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetDouble()));
      break;
    case 1070:
      m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetInt16()));
      break;
    case 1071:
      m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetInt32()));
      break;
    default:
      break;
  }
}

bool EoDxfEntity::ParseAppDataGroup(EoDxfReader* reader) {
  std::list<EoDxfGroupCodeValuesVariant> groupList;

  EoDxfGroupCodeValuesVariant currentVariant;

  std::wstring appName = reader->GetWideString();
  if (appName.empty() || appName[0] != L'{') { return false; }

  // opening line: store without the leading '{'
  currentVariant.AddWideString(102, appName.substr(1));
  groupList.push_back(currentVariant);

  while (true) {
    int nextCode{};
    if (!reader->ReadRec(&nextCode)) { break; }  // EOF or read error
    bool hasValue{};

    if (nextCode == 102) {
      std::wstring val = reader->GetWideString();
      if (!val.empty() && val[0] == L'}') { break; }  // closing 102 } — do not store the closing tag

      // rare nested control string
      currentVariant = EoDxfGroupCodeValuesVariant{};
      currentVariant.AddWideString(102, val);
      hasValue = true;
    } else {
      currentVariant = EoDxfGroupCodeValuesVariant{};
      hasValue = AddAppDataValue(currentVariant, nextCode, reader);
    }
    if (hasValue) { groupList.push_back(currentVariant); }
  }

  m_appData.push_back(std::move(groupList));  // avoid copy
  return true;
}

void EoDxfPoint::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10:
      m_firstPoint.x = reader->GetDouble();
      break;
    case 20:
      m_firstPoint.y = reader->GetDouble();
      break;
    case 30:
      m_firstPoint.z = reader->GetDouble();
      break;
    case 50:
      m_angleOfXAxis = reader->GetDouble() * EoDxf::DegreesToRadians;
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfLine::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10:
      m_startPoint.x = reader->GetDouble();
      break;
    case 20:
      m_startPoint.y = reader->GetDouble();
      break;
    case 30:
      m_startPoint.z = reader->GetDouble();
      break;
    case 11:
      m_endPoint.x = reader->GetDouble();
      break;
    case 21:
      m_endPoint.y = reader->GetDouble();
      break;
    case 31:
      m_endPoint.z = reader->GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfCircle::ApplyExtrusion() {
  if (m_haveExtrusion) {
    // NOTE: Commenting these out causes the the arcs being tested to be located
    // on the other side of the y axis (all x dimensions are negated).
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_centerPoint);
  }
}

void EoDxfCircle::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10:
      m_centerPoint.x = reader->GetDouble();
      break;
    case 20:
      m_centerPoint.y = reader->GetDouble();
      break;
    case 30:
      m_centerPoint.z = reader->GetDouble();
      break;
    case 40:
      m_radius = reader->GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfArc::ApplyExtrusion() {
  EoDxfCircle::ApplyExtrusion();

  if (m_haveExtrusion) {
    // If the extrusion vector has a z value less than 0, the angles for the arc
    // have to be mirrored since DXF files use the right hand rule.
    // Note that the following code only handles the special case where there is a 2D
    // drawing with the z axis heading into the paper (or rather screen). An arbitrary
    // extrusion axis (with x and y values greater than 1/64) may still have issues.
    if (fabs(m_extrusionDirection.x) < 0.015625 && fabs(m_extrusionDirection.y) < 0.015625 &&
        m_extrusionDirection.z < 0.0) {
      m_startAngle = EoDxf::Pi - m_startAngle;
      m_endAngle = EoDxf::Pi - m_endAngle;

      double temp = m_startAngle;
      m_startAngle = m_endAngle;
      m_endAngle = temp;
    }
  }
}

void EoDxfArc::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 50:
      m_startAngle = reader->GetDouble() * EoDxf::DegreesToRadians;
      break;
    case 51:
      m_endAngle = reader->GetDouble() * EoDxf::DegreesToRadians;
      break;
    default:
      EoDxfCircle::ParseCode(code, reader);
      break;
  }
}

void EoDxfEllipse::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 40:
      m_ratio = reader->GetDouble();
      break;
    case 41:
      m_startParam = reader->GetDouble();
      break;
    case 42:
      m_endParam = reader->GetDouble();
      break;
    default:
      EoDxfLine::ParseCode(code, reader);
      break;
  }
}

void EoDxfEllipse::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_endPoint);
    double intialparam = m_startParam;
    if (m_extrusionDirection.z < 0.) {
      m_startParam = EoDxf::TwoPi - m_endParam;
      m_endParam = EoDxf::TwoPi - intialparam;
    }
  }
}

// if ratio > 1 minor axis are greather than major axis, correct it
void EoDxfEllipse::CorrectAxis() {
  bool complete{};
  if (fabs(m_startParam - m_endParam) < EoDxf::geometricTolerance) {
    m_startParam = 0.0;
    m_endParam = EoDxf::TwoPi;
    complete = true;
  }
  if (m_ratio > 1.0) {
    if (fabs(m_endParam - m_startParam - EoDxf::TwoPi) < 1.0e-10) { complete = true; }
    double incX = m_endPoint.x;
    m_endPoint.x = -(m_endPoint.y * m_ratio);
    m_endPoint.y = incX * m_ratio;
    m_ratio = 1.0 / m_ratio;
    if (!complete) {
      if (m_startParam < EoDxf::HalfPi) { m_startParam += EoDxf::TwoPi; }
      if (m_endParam < EoDxf::HalfPi) { m_endParam += EoDxf::TwoPi; }
      m_endParam -= EoDxf::HalfPi;
      m_startParam -= EoDxf::HalfPi;
    }
  }
}

// parts are the number of vertex to split polyline, default 128
void EoDxfEllipse::ToPolyline(EoDxfPolyline* polyline, int parts) {
  double radMajor, radMinor, cosRot, sinRot, incAngle, curAngle;
  double cosCurr, sinCurr;
  radMajor = sqrt(m_endPoint.x * m_endPoint.x + m_endPoint.y * m_endPoint.y);
  radMinor = radMajor * m_ratio;
  // calculate sin & cos of included angle
  incAngle = atan2(m_endPoint.y, m_endPoint.x);
  cosRot = cos(incAngle);
  sinRot = sin(incAngle);
  incAngle = EoDxf::TwoPi / parts;
  curAngle = m_startParam;
  int i = static_cast<int>(curAngle / incAngle);
  do {
    if (curAngle > m_endParam) {
      curAngle = m_endParam;
      i = parts + 2;
    }
    cosCurr = cos(curAngle);
    sinCurr = sin(curAngle);
    double x = m_startPoint.x + (cosCurr * cosRot * radMajor) - (sinCurr * sinRot * radMinor);
    double y = m_startPoint.y + (cosCurr * sinRot * radMajor) + (sinCurr * cosRot * radMinor);
    polyline->addVertex(EoDxfVertex(x, y, 0.0, 0.0));
    curAngle = (++i) * incAngle;
  } while (i < parts);
  if (fabs(m_endParam - m_startParam - EoDxf::TwoPi) < 1.0e-10) { polyline->m_polylineFlag = 1; }
  polyline->m_layer = this->m_layer;
  polyline->m_lineType = this->m_lineType;
  polyline->m_color = this->m_color;
  polyline->m_lineWeight = this->m_lineWeight;
  polyline->m_extrusionDirection = this->m_extrusionDirection;
}

void EoDxfTrace::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_startPoint);
    ExtrudePointInPlace(m_extrusionDirection, m_endPoint);
    ExtrudePointInPlace(m_extrusionDirection, m_thirdPoint);
    ExtrudePointInPlace(m_extrusionDirection, m_fourthPoint);
  }
}

void EoDxfTrace::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 12:
      m_thirdPoint.x = reader->GetDouble();
      break;
    case 22:
      m_thirdPoint.y = reader->GetDouble();
      break;
    case 32:
      m_thirdPoint.z = reader->GetDouble();
      break;
    case 13:
      m_fourthPoint.x = reader->GetDouble();
      break;
    case 23:
      m_fourthPoint.y = reader->GetDouble();
      break;
    case 33:
      m_fourthPoint.z = reader->GetDouble();
      break;
    default:
      EoDxfLine::ParseCode(code, reader);
      break;
  }
}

void EoDxfSolid::ParseCode(int code, EoDxfReader* reader) { EoDxfTrace::ParseCode(code, reader); }

void EoDxf3dFace::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 70:
      m_invisibleFlag = reader->GetInt16();
      break;
    default:
      EoDxfTrace::ParseCode(code, reader);
      break;
  }
}

void EoDxfBlock::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 2:
      [[fallthrough]];  // They are always identical in every valid DXF file (regular blocks, anonymous blocks, XREFs,
                        // overlays, everything). Both groups 2 and 3 contain the block name, and the DXF spec confirms
                        // this is intentional for backward compatibility. It seems that group 3 may be missing on read
                        // but it should be written out with both groups 2 and 3.
    case 3:
      m_blockName = reader->GetWideString();
      break;
    case 70:
      m_blockTypeFlags = reader->GetInt16();
      break;
    case 10:
      m_basePoint.x = reader->GetDouble();
      break;
    case 20:
      m_basePoint.y = reader->GetDouble();
      break;
    case 30:
      m_basePoint.z = reader->GetDouble();
      break;
    case 1:
      m_xrefPathName = reader->GetWideString();
      break;
    case 4:
      m_blockDescription = reader->GetWideString();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfInsert::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 2:
      m_blockName = reader->GetWideString();
      break;
    case 41:
      m_xScaleFactor = reader->GetDouble();
      break;
    case 42:
      m_yScaleFactor = reader->GetDouble();
      break;
    case 43:
      m_zScaleFactor = reader->GetDouble();
      break;
    case 50:
      m_rotationAngle = reader->GetDouble();
      m_rotationAngle = m_rotationAngle * EoDxf::DegreesToRadians;
      break;
    case 70:
      m_columnCount = reader->GetInt16();
      break;
    case 71:
      m_rowCount = reader->GetInt16();
      break;
    case 44:
      m_columnSpacing = reader->GetDouble();
      break;
    case 45:
      m_rowSpacing = reader->GetDouble();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}

void EoDxfLwPolyline::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    for (auto& vert : m_vertices) {  // range-based, value semantics – no raw pointer
      EoDxfGeometryBase3d v(vert.x, vert.y, m_elevation);
      ExtrudePointInPlace(m_extrusionDirection, v);
      vert.x = v.x;
      vert.y = v.y;
    }
  }
}

void EoDxfLwPolyline::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10: {
      m_vertices.emplace_back();
      m_currentVertexIndex = static_cast<int>(m_vertices.size()) - 1;
      m_vertices.back().x = reader->GetDouble();
      break;
    }
    case 20:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].y = reader->GetDouble(); }
      break;
    case 40:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].stawidth = reader->GetDouble(); }
      break;
    case 41:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].endwidth = reader->GetDouble(); }
      break;
    case 42:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].bulge = reader->GetDouble(); }
      break;
    case 38:
      m_elevation = reader->GetDouble();
      break;
    case 39:
      m_thickness = reader->GetDouble();
      break;
    case 43:
      m_constantWidth = reader->GetDouble();
      break;
    case 70:
      m_polylineFlag = reader->GetInt16();
      break;
    case 90:
      m_numberOfVertices = reader->GetInt32();
      m_vertices.reserve(m_numberOfVertices);  // now reserves the correct container
      break;
    case 210:
      m_haveExtrusion = true;
      m_extrusionDirection.x = reader->GetDouble();
      break;
    case 220:
      m_extrusionDirection.y = reader->GetDouble();
      break;
    case 230:
      m_extrusionDirection.z = reader->GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfText::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10:
      m_firstAlignmentPoint.x = reader->GetDouble();
      break;
    case 20:
      m_firstAlignmentPoint.y = reader->GetDouble();
      break;
    case 30:
      m_firstAlignmentPoint.z = reader->GetDouble();
      break;
    case 11:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.x = reader->GetDouble();
      break;
    case 21:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.y = reader->GetDouble();
      break;
    case 31:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.z = reader->GetDouble();
      break;
    case 40:
      m_textHeight = reader->GetDouble();
      break;
    case 41:
      m_scaleFactorWidth = reader->GetDouble();
      break;
    case 50:
      m_textRotation = reader->GetDouble();
      break;
    case 51:
      m_obliqueAngle = reader->GetDouble();
      break;
    case 71:
      m_textGenerationFlags = reader->GetInt16();
      break;
    case 72:
      m_horizontalAlignment = static_cast<HorizontalAlignment>(
          std::clamp(reader->GetInt16(), static_cast<std::int16_t>(HorizontalAlignment::Left),
              static_cast<std::int16_t>(HorizontalAlignment::FitIfBaseLine)));
      break;
    case 73:
      m_verticalAlignment = static_cast<VerticalAlignment>(std::clamp(reader->GetInt16(),
          static_cast<std::int16_t>(VerticalAlignment::Bottom), static_cast<std::int16_t>(VerticalAlignment::Top)));
      break;
    case 1:
      m_string = reader->GetWideString();
      break;
    case 7:
      m_textStyleName = reader->GetWideString();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfMText::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10:
      m_insertionPoint.x = reader->GetDouble();
      break;
    case 20:
      m_insertionPoint.y = reader->GetDouble();
      break;
    case 30:
      m_insertionPoint.z = reader->GetDouble();
      break;
    case 40:
      m_nominalTextHeight = reader->GetDouble();
      break;
    case 41:
      m_referenceRectangleWidth = reader->GetDouble();
      break;
    case 7:
      m_textStyleName = reader->GetWideString();
      break;
    case 50:
      m_rotationAngle = reader->GetDouble();
      break;

    case 11:
      m_haveXAxisDirection = true;
      m_xAxisDirectionVector.x = reader->GetDouble();
      break;
    case 21:
      m_xAxisDirectionVector.y = reader->GetDouble();
      break;
    case 31:
      m_xAxisDirectionVector.z = reader->GetDouble();
      break;

    case 71:
      m_attachmentPoint = static_cast<AttachmentPoint>(reader->GetInt16());
      break;
    case 72:
      m_drawingDirection = static_cast<DrawingDirection>(reader->GetInt16());
      break;
    case 73:
      m_lineSpacingStyle = static_cast<LineSpacingStyle>(reader->GetInt16());
      break;
    case 44:
      m_lineSpacingFactor = reader->GetDouble();
      break;

    case 1:  // final chunk (or only chunk if no continuation chunks) of text string
      m_textString += reader->GetWideString();
      break;
    case 3:  // continuation chunk (multiple allowed)
      m_textString += reader->GetWideString();
      break;

    // (AC1021+) ? must appear together
    case 90:
      m_backgroundFillSetting = reader->GetInt32();
      break;
    case 45:
      m_fillBoxScale = reader->GetDouble();
      break;
    case 63:
      m_backgroundColor = reader->GetInt16();
      break;
    case 421:
      m_backgroundColor = reader->GetInt32();
      break;
    case 431:
      m_backgroundColorName = reader->GetWideString();
      break;

    // calculated values (can ignore)
    case 42:
      m_horizontalWidth = reader->GetDouble();
      break;
    case 43:
      m_verticalHeight = reader->GetDouble();
      break;

    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfMText::UpdateAngle() {
  if (m_haveXAxisDirection) { m_rotationAngle = atan2(m_xAxisDirectionVector.y, m_xAxisDirectionVector.x); }
}

void EoDxfPolyline::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 70:
      m_polylineFlag = reader->GetInt16();
      break;
    case 40:
      m_defaultStartWidth = reader->GetDouble();
      break;
    case 41:
      m_defaultEndWidth = reader->GetDouble();
      break;
    case 71:
      m_polygonMeshVertexCountM = reader->GetInt16();
      break;
    case 72:
      m_polygonMeshVertexCountN = reader->GetInt16();
      break;
    case 73:
      m_smoothSurfaceDensityM = reader->GetInt16();
      break;
    case 74:
      m_smoothSurfaceDensityN = reader->GetInt16();
      break;
    case 75:
      m_curvesAndSmoothSurfaceType = reader->GetInt16();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}

void EoDxfVertex::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10:
      m_locationPoint.x = reader->GetDouble();
      break;
    case 20:
      m_locationPoint.y = reader->GetDouble();
      break;
    case 30:
      m_locationPoint.z = reader->GetDouble();
      break;
    case 70:
      m_vertexFlags = reader->GetInt16();
      break;
    case 40:
      m_startingWidth = reader->GetDouble();
      break;
    case 41:
      m_endingWidth = reader->GetDouble();
      break;
    case 42:
      m_bulge = reader->GetDouble();
      break;
    case 50:
      m_curveFitTangentDirection = reader->GetDouble();
      break;
    case 71:
      m_polyfaceMeshVertexIndex1 = reader->GetInt16();
      break;
    case 72:
      m_polyfaceMeshVertexIndex2 = reader->GetInt16();
      break;
    case 73:
      m_polyfaceMeshVertexIndex3 = reader->GetInt16();
      break;
    case 74:
      m_polyfaceMeshVertexIndex4 = reader->GetInt16();
      break;
    case 91:
      m_identifier = reader->GetInt32();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}


void EoDxfSpline::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 210:
      m_normalVector.x = reader->GetDouble();
      break;
    case 220:
      m_normalVector.y = reader->GetDouble();
      break;
    case 230:
      m_normalVector.z = reader->GetDouble();
      break;
    case 12:
      m_startTangent.x = reader->GetDouble();
      break;
    case 22:
      m_startTangent.y = reader->GetDouble();
      break;
    case 32:
      m_startTangent.z = reader->GetDouble();
      break;
    case 13:
      m_endTangent.x = reader->GetDouble();
      break;
    case 23:
      m_endTangent.y = reader->GetDouble();
      break;
    case 33:
      m_endTangent.z = reader->GetDouble();
      break;
    case 70:
      m_splineFlag = reader->GetInt16();
      break;
    case 71:
      m_degreeOfTheSplineCurve = reader->GetInt16();
      break;
    case 72:
      m_numberOfKnots = reader->GetInt16();
      break;
    case 73:
      m_numberOfControlPoints = reader->GetInt16();
      break;
    case 74:
      m_numberOfFitPoints = reader->GetInt16();
      break;
    case 42:
      m_knotTolerance = reader->GetDouble();
      break;
    case 43:
      m_controlPointTolerance = reader->GetDouble();
      break;
    case 44:
      m_fitTolerance = reader->GetDouble();
      break;
    case 10:
      m_controlPoint = new EoDxfGeometryBase3d();
      m_controlPoints.push_back(m_controlPoint);
      m_controlPoint->x = reader->GetDouble();
      break;
    case 20:
      if (m_controlPoint != nullptr) { m_controlPoint->y = reader->GetDouble(); }
      break;
    case 30:
      if (m_controlPoint != nullptr) { m_controlPoint->z = reader->GetDouble(); }
      break;
    case 11:
      m_fitPoint = new EoDxfGeometryBase3d();
      m_fitPoints.push_back(m_fitPoint);
      m_fitPoint->x = reader->GetDouble();
      break;
    case 21:
      if (m_fitPoint != nullptr) { m_fitPoint->y = reader->GetDouble(); }
      break;
    case 31:
      if (m_fitPoint != nullptr) { m_fitPoint->z = reader->GetDouble(); }
      break;
    case 40:
      m_knotValues.push_back(reader->GetDouble());
      break;

    // case 41:
    //   break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfImage::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 12:
      vVector.x = reader->GetDouble();
      break;
    case 22:
      vVector.y = reader->GetDouble();
      break;
    case 32:
      vVector.z = reader->GetDouble();
      break;
    case 13:
      sizeu = reader->GetDouble();
      break;
    case 23:
      sizev = reader->GetDouble();
      break;
    case 340:
      m_imageDefinitionHandle = reader->GetHandleString();
      break;
    case 280:
      m_clippingState = reader->GetInt16();
      break;
    case 281:
      m_brightnessValue = reader->GetInt16();
      break;
    case 282:
      m_contrastValue = reader->GetInt16();
      break;
    case 283:
      m_fadeValue = reader->GetInt16();
      break;
    default:
      EoDxfLine::ParseCode(code, reader);
      break;
  }
}

void EoDxfDimension::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 1:
      text = reader->GetWideString();
      break;
    case 2:
      name = reader->GetWideString();
      break;
    case 3:
      style = reader->GetWideString();
      break;
    case 70:
      m_dimensionType = reader->GetInt16();
      break;
    case 71:
      m_attachmentPoint = reader->GetInt16();
      break;
    case 72:
      m_dimensionTextLineSpacingStyle = reader->GetInt16();
      break;
    case 10:
      defPoint.x = reader->GetDouble();
      break;
    case 20:
      defPoint.y = reader->GetDouble();
      break;
    case 30:
      defPoint.z = reader->GetDouble();
      break;
    case 11:
      textPoint.x = reader->GetDouble();
      break;
    case 21:
      textPoint.y = reader->GetDouble();
      break;
    case 31:
      textPoint.z = reader->GetDouble();
      break;
    case 12:
      clonePoint.x = reader->GetDouble();
      break;
    case 22:
      clonePoint.y = reader->GetDouble();
      break;
    case 32:
      clonePoint.z = reader->GetDouble();
      break;
    case 13:
      def1.x = reader->GetDouble();
      break;
    case 23:
      def1.y = reader->GetDouble();
      break;
    case 33:
      def1.z = reader->GetDouble();
      break;
    case 14:
      def2.x = reader->GetDouble();
      break;
    case 24:
      def2.y = reader->GetDouble();
      break;
    case 34:
      def2.z = reader->GetDouble();
      break;
    case 15:
      circlePoint.x = reader->GetDouble();
      break;
    case 25:
      circlePoint.y = reader->GetDouble();
      break;
    case 35:
      circlePoint.z = reader->GetDouble();
      break;
    case 16:
      arcPoint.x = reader->GetDouble();
      break;
    case 26:
      arcPoint.y = reader->GetDouble();
      break;
    case 36:
      arcPoint.z = reader->GetDouble();
      break;
    case 41:
      linefactor = reader->GetDouble();
      break;
    case 53:
      rot = reader->GetDouble();
      break;
    case 50:
      angle = reader->GetDouble();
      break;
    case 52:
      oblique = reader->GetDouble();
      break;
    case 40:
      length = reader->GetDouble();
      break;
    case 51:
      hdir = reader->GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfLeader::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 3:
      m_dimensionStyleName = reader->GetWideString();
      break;
    case 71:
      m_arrowheadFlag = reader->GetInt16();
      break;
    case 72:
      m_leaderPathType = reader->GetInt16();
      break;
    case 73:
      m_leaderCreationFlag = reader->GetInt16();
      break;
    case 74:
      m_hookLineDirection = reader->GetInt16();
      break;
    case 75:
      m_hookLineFlag = reader->GetInt16();
      break;
    case 76:
      m_numberOfVertices = reader->GetInt16();
      m_vertexList.reserve(m_numberOfVertices);
      break;
    case 77:
      m_colorToUse = reader->GetInt16();
      break;
    case 40:
      m_textAnnotationHeight = reader->GetDouble();
      break;
    case 41:
      m_textAnnotationWidth = reader->GetDouble();
      break;
    case 10: {
      m_vertexList.emplace_back();  // value semantics
      m_currentVertexIndex = static_cast<int>(m_vertexList.size()) - 1;
      m_vertexList.back().x = reader->GetDouble();
      break;
    }
    case 20:
      if (m_currentVertexIndex >= 0) { m_vertexList[m_currentVertexIndex].y = reader->GetDouble(); }
      break;
    case 30:
      if (m_currentVertexIndex >= 0) { m_vertexList[m_currentVertexIndex].z = reader->GetDouble(); }
      break;
    case 340:
      m_associatedAnnotationHandle = reader->GetHandleString();
      break;
    case 210:
      m_normalVector.x = reader->GetDouble();
      break;
    case 220:
      m_normalVector.y = reader->GetDouble();
      break;
    case 230:
      m_normalVector.z = reader->GetDouble();
      break;
    case 211:
      m_horizontalDirectionForLeader.x = reader->GetDouble();
      break;
    case 221:
      m_horizontalDirectionForLeader.y = reader->GetDouble();
      break;
    case 231:
      m_horizontalDirectionForLeader.z = reader->GetDouble();
      break;
    case 212:
      m_offsetFromBlockInsertionPoint.x = reader->GetDouble();
      break;
    case 222:
      m_offsetFromBlockInsertionPoint.y = reader->GetDouble();
      break;
    case 232:
      m_offsetFromBlockInsertionPoint.z = reader->GetDouble();
      break;
    case 213:
      m_offsetFromAnnotationPlacementPoint.x = reader->GetDouble();
      break;
    case 223:
      m_offsetFromAnnotationPlacementPoint.y = reader->GetDouble();
      break;
    case 233:
      m_offsetFromAnnotationPlacementPoint.z = reader->GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfViewport::ParseCode(int code, EoDxfReader* reader) {
  switch (code) {
    case 10:
      m_centerPoint.x = reader->GetDouble();
      break;
    case 20:
      m_centerPoint.y = reader->GetDouble();
      break;
    case 30:
      m_centerPoint.z = reader->GetDouble();
      break;
    case 12:
      m_viewCenter.x = reader->GetDouble();
      break;
    case 22:
      m_viewCenter.y = reader->GetDouble();
      break;
    case 13:
      m_snapBasePoint.x = reader->GetDouble();
      break;
    case 23:
      m_snapBasePoint.y = reader->GetDouble();
      break;
    case 14:
      m_snapSpacing.x = reader->GetDouble();
      break;
    case 24:
      m_snapSpacing.y = reader->GetDouble();
      break;
    case 15:
      m_gridSpacing.x = reader->GetDouble();
      break;
    case 25:
      m_gridSpacing.y = reader->GetDouble();
      break;
    case 16:
      m_viewDirection.x = reader->GetDouble();
      break;
    case 17:
      m_viewTargetPoint.x = reader->GetDouble();
      break;
    case 26:
      m_viewDirection.y = reader->GetDouble();
      break;
    case 27:
      m_viewTargetPoint.y = reader->GetDouble();
      break;
    case 36:
      m_viewDirection.z = reader->GetDouble();
      break;
    case 37:
      m_viewTargetPoint.z = reader->GetDouble();
      break;
    case 40:
      m_width = reader->GetDouble();
      break;
    case 41:
      m_height = reader->GetDouble();
      break;
    case 42:
      m_lensLength = reader->GetDouble();
      break;
    case 43:
      m_frontClipPlane = reader->GetDouble();
      break;
    case 44:
      m_backClipPlane = reader->GetDouble();
      break;
    case 45:
      m_viewHeight = reader->GetDouble();
      break;
    case 50:
      m_snapAngle = reader->GetDouble();
      break;
    case 51:
      m_twistAngle = reader->GetDouble();
      break;
    case 68:
      m_viewportStatus = reader->GetInt16();
      break;
    case 69:
      m_viewportId = reader->GetInt16();
      break;
    case 90:
      m_viewportStatusBitCodedFlags = reader->GetInt32();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}
