#include <algorithm>
#include <cmath>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "intern/dxfreader.h"

EoDxfEntity::EoDxfEntity(const EoDxfEntity& other)
    : m_appData{other.m_appData},
      m_layer{other.m_layer},
      m_lineType{other.m_lineType},
      m_proxyEntityGraphicsData{other.m_proxyEntityGraphicsData},
      m_colorName{other.m_colorName},
      m_lineTypeScale{other.m_lineTypeScale},
      m_entityType{other.m_entityType},
      m_handle{other.m_handle},
      m_ownerHandle{other.m_ownerHandle},
      m_material{other.m_material},
      m_color{other.m_color},
      m_lineWeight{other.m_lineWeight},
      m_numberOfBytesInProxyGraphics{other.m_numberOfBytesInProxyGraphics},
      m_color24{other.m_color24},
      m_transparency{other.m_transparency},
      m_plotStyle{other.m_plotStyle},
      m_shadowMode{other.m_shadowMode},
      m_space{other.m_space},
      m_visible{other.m_visible},
      m_haveExtrusion{other.m_haveExtrusion},
      m_currentVariant{nullptr} {
  m_extendedData.reserve(other.m_extendedData.size());
  for (const auto* v : other.m_extendedData) { m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(*v)); }
}

EoDxfEntity& EoDxfEntity::operator=(const EoDxfEntity& other) {
  if (this != &other) {
    clearExtendedData();

    m_appData = other.m_appData;
    m_layer = other.m_layer;
    m_lineType = other.m_lineType;
    m_proxyEntityGraphicsData = other.m_proxyEntityGraphicsData;
    m_colorName = other.m_colorName;
    m_lineTypeScale = other.m_lineTypeScale;
    m_entityType = other.m_entityType;
    m_handle = other.m_handle;
    m_ownerHandle = other.m_ownerHandle;
    m_material = other.m_material;
    m_color = other.m_color;
    m_lineWeight = other.m_lineWeight;
    m_numberOfBytesInProxyGraphics = other.m_numberOfBytesInProxyGraphics;
    m_color24 = other.m_color24;
    m_transparency = other.m_transparency;
    m_plotStyle = other.m_plotStyle;
    m_shadowMode = other.m_shadowMode;
    m_space = other.m_space;
    m_visible = other.m_visible;
    m_haveExtrusion = other.m_haveExtrusion;
    m_currentVariant = nullptr;

    m_extendedData.reserve(other.m_extendedData.size());
    for (const auto* v : other.m_extendedData) { m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(*v)); }
  }
  return *this;
}

EoDxfEntity::~EoDxfEntity() { clearExtendedData(); }

void EoDxfEntity::Clear() {
  clearExtendedData();
  // extend this later for more state reset if needed
}

void EoDxfEntity::clearExtendedData() noexcept {
  for (auto* v : m_extendedData) { delete v; }
  m_extendedData.clear();
}

void EoDxfEntity::CalculateArbitraryAxis(const DRW_Coord& extrusionDirection) {
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

  extAxisX.unitize();

  // Ay = N x Ax
  extAxisY.x = (extrusionDirection.y * extAxisX.z) - (extAxisX.y * extrusionDirection.z);
  extAxisY.y = (extrusionDirection.z * extAxisX.x) - (extAxisX.z * extrusionDirection.x);
  extAxisY.z = (extrusionDirection.x * extAxisX.y) - (extAxisX.x * extrusionDirection.y);

  extAxisY.unitize();
}

void EoDxfEntity::ExtrudePointInPlace(const DRW_Coord& extrusionDirection, DRW_Coord& point) const noexcept {
  double px = (extAxisX.x * point.x) + (extAxisY.x * point.y) + (extrusionDirection.x * point.z);
  double py = (extAxisX.y * point.x) + (extAxisY.y * point.y) + (extrusionDirection.y * point.z);
  double pz = (extAxisX.z * point.x) + (extAxisY.z * point.y) + (extrusionDirection.z * point.z);

  point.x = px;
  point.y = py;
  point.z = pz;
}

void EoDxfEntity::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 5:
      m_handle = reader->GetHandleString();
      break;
    case 330:
      m_ownerHandle = reader->GetHandleString();
      break;
    case 8:
      m_layer = reader->GetUtf8String();
      break;
    case 6:
      m_lineType = reader->GetUtf8String();
      break;
    case 62:
      m_color = reader->GetInt32();
      break;
    case 370:
      m_lineWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->GetInt32());
      break;
    case 48:
      m_lineTypeScale = reader->GetDouble();
      break;
    case 60:
      m_visible = reader->GetBool();
      break;
    case 284:
      // @bug possible: 284 is a bitmask using 8-bit integer values, reading as int32 for simplicity
      m_shadowMode = static_cast<EoDxf::ShadowMode>(reader->GetInt32());
      break;
    case 390:
      m_plotStyle = reader->GetHandleString();
      break;
    case 420:
      m_color24 = reader->GetInt32();
      break;
    case 430:
      m_colorName = reader->GetString();
      break;
    case 440:
      m_transparency = reader->GetInt32();
      break;
    case 67:
      m_space = static_cast<EoDxf::Space>(reader->GetInt32());
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
      m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
      m_currentVariant = new EoDxfGroupCodeValuesVariant(code, DRW_Coord(reader->GetDouble(), 0.0, 0.0));
      m_extendedData.push_back(m_currentVariant);
      break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
      if (m_currentVariant) { m_currentVariant->setCoordY(reader->GetDouble()); }
      break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
      if (m_currentVariant) { m_currentVariant->setCoordZ(reader->GetDouble()); }
      m_currentVariant = nullptr;
      break;
    case 1040:
    case 1041:
    case 1042:
      m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetDouble()));
      break;
    case 1070:
    case 1071:
      m_extendedData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetInt32()));
      break;
    default:
      break;
  }
}

bool EoDxfEntity::ParseAppDataGroup(dxfReader* reader) {
  std::list<EoDxfGroupCodeValuesVariant> groupList;

  EoDxfGroupCodeValuesVariant currentVariant;

  std::string appName = reader->GetString();
  if (appName.empty() || appName[0] != '{') { return false; }

  // opening line: store without the leading '{'
  currentVariant.addString(102, appName.substr(1));
  groupList.push_back(currentVariant);

  while (true) {
    int nextCode{};
    if (!reader->ReadRec(&nextCode)) { break; }  // EOF or read error

    if (nextCode == 102) {
      std::string val = reader->GetString();
      if (!val.empty() && val[0] == '}') { break; }  // closing 102 } — do not store the closing tag

      // rare nested control string
      currentVariant = EoDxfGroupCodeValuesVariant{};
      currentVariant.addString(102, val);
    } else {
      currentVariant = EoDxfGroupCodeValuesVariant{};
      if (nextCode == 330 || nextCode == 360) {
        currentVariant.addInt(nextCode, reader->GetHandleString());
      } else {
        switch (reader->GetType()) {
          case dxfReader::Type::String:
            currentVariant.addString(nextCode, reader->GetString());
            break;
          case dxfReader::Type::Int32:
          case dxfReader::Type::Int64:
            currentVariant.addInt(nextCode, reader->GetInt32());
            break;
          case dxfReader::Type::Double:
            currentVariant.addDouble(nextCode, reader->GetDouble());
            break;
          case dxfReader::Type::Bool:
            currentVariant.addInt(nextCode, reader->GetInt32());
            break;
          default:
            break;
        }
      }
    }
    groupList.push_back(currentVariant);
  }

  m_appData.push_back(std::move(groupList));  // avoid copy
  return true;
}

void EoDxfPoint::ParseCode(int code, dxfReader* reader) {
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
    default:
      EoDxfEntity::ParseCode(code, reader);
      break;
  }
}

void EoDxfLine::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 11:
      m_secondPoint.x = reader->GetDouble();
      break;
    case 21:
      m_secondPoint.y = reader->GetDouble();
      break;
    case 31:
      m_secondPoint.z = reader->GetDouble();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}

void EoDxfCircle::ApplyExtrusion() {
  if (m_haveExtrusion) {
    // NOTE: Commenting these out causes the the arcs being tested to be located
    // on the other side of the y axis (all x dimensions are negated).
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_firstPoint);
  }
}

void EoDxfCircle::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 40:
      m_radius = reader->GetDouble();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
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

void EoDxfArc::ParseCode(int code, dxfReader* reader) {
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

void EoDxfEllipse::ParseCode(int code, dxfReader* reader) {
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
    ExtrudePointInPlace(m_extrusionDirection, m_secondPoint);
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
    double incX = m_secondPoint.x;
    m_secondPoint.x = -(m_secondPoint.y * m_ratio);
    m_secondPoint.y = incX * m_ratio;
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
  radMajor = sqrt(m_secondPoint.x * m_secondPoint.x + m_secondPoint.y * m_secondPoint.y);
  radMinor = radMajor * m_ratio;
  // calculate sin & cos of included angle
  incAngle = atan2(m_secondPoint.y, m_secondPoint.x);
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
    double x = m_firstPoint.x + (cosCurr * cosRot * radMajor) - (sinCurr * sinRot * radMinor);
    double y = m_firstPoint.y + (cosCurr * sinRot * radMajor) + (sinCurr * cosRot * radMinor);
    polyline->addVertex(DRW_Vertex(x, y, 0.0, 0.0));
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
    ExtrudePointInPlace(m_extrusionDirection, m_firstPoint);
    ExtrudePointInPlace(m_extrusionDirection, m_secondPoint);
    ExtrudePointInPlace(m_extrusionDirection, m_thirdPoint);
    ExtrudePointInPlace(m_extrusionDirection, m_fourthPoint);
  }
}

void EoDxfTrace::ParseCode(int code, dxfReader* reader) {
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

void EoDxfSolid::ParseCode(int code, dxfReader* reader) { EoDxfTrace::ParseCode(code, reader); }

void EoDxf3dFace::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      m_invisibleFlag = reader->GetInt32();
      break;
    default:
      EoDxfTrace::ParseCode(code, reader);
      break;
  }
}

void DRW_Block::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      name = reader->GetUtf8String();
      break;
    case 70:
      m_flags = reader->GetInt32();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}

void EoDxfInsert::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      m_blockName = reader->GetUtf8String();
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
      m_columnCount = reader->GetInt32();
      break;
    case 71:
      m_rowCount = reader->GetInt32();
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
      DRW_Coord v(vert.x, vert.y, m_elevation);
      ExtrudePointInPlace(m_extrusionDirection, v);
      vert.x = v.x;
      vert.y = v.y;
    }
  }
}

void EoDxfLwPolyline::ParseCode(int code, dxfReader* reader) {
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
      m_polylineFlag = reader->GetInt32();
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
      EoDxfEntity::ParseCode(code, reader);
      break;
  }
}

void EoDxfText::ParseCode(int code, dxfReader* reader) {
  switch (code) {
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
      m_textGenerationFlags = reader->GetInt32();
      break;
    case 72:
      m_horizontalAlignment = static_cast<HAlign>(
          std::clamp(reader->GetInt32(), static_cast<int>(HAlign::Left), static_cast<int>(HAlign::FitIfBaseLine)));
      break;
    case 73:
      m_verticalAlignment = static_cast<VAlign>(
          std::clamp(reader->GetInt32(), static_cast<int>(VAlign::Top), static_cast<int>(VAlign::Bottom)));
      break;
    case 1:
      m_string = reader->GetUtf8String();
      break;
    case 7:
      m_textStyleName = reader->GetUtf8String();
      break;
    default:
      EoDxfLine::ParseCode(code, reader);
      break;
  }
}

void EoDxfMText::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 1:
      m_string += reader->GetString();
      m_string = reader->ToUtf8String(m_string);
      break;
    case 11:
      m_haveXAxisDirection = true;
      EoDxfText::ParseCode(code, reader);
      break;
    case 3:
      m_string += reader->GetString();
      break;
    case 44:
      m_lineSpacingFactor = reader->GetDouble();
      break;
    default:
      EoDxfText::ParseCode(code, reader);
      break;
  }
}

void EoDxfMText::UpdateAngle() {
  if (m_haveXAxisDirection) { m_textRotation = atan2(m_secondPoint.y, m_secondPoint.x) * EoDxf::RadiansToDegrees; }
}

void EoDxfPolyline::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      m_polylineFlag = reader->GetInt32();
      break;
    case 40:
      m_defaultStartWidth = reader->GetDouble();
      break;
    case 41:
      m_defaultEndWidth = reader->GetDouble();
      break;
    case 71:
      m_polygonMeshVertexCountM = reader->GetInt32();
      break;
    case 72:
      m_polygonMeshVertexCountN = reader->GetInt32();
      break;
    case 73:
      m_smoothSurfaceDensityM = reader->GetInt32();
      break;
    case 74:
      m_smoothSurfaceDensityN = reader->GetInt32();
      break;
    case 75:
      m_curvesAndSmoothSurfaceType = reader->GetInt32();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}

void DRW_Vertex::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      m_vertexFlags = reader->GetInt32();
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
      m_polyfaceMeshVertexIndex1 = reader->GetInt32();
      break;
    case 72:
      m_polyfaceMeshVertexIndex2 = reader->GetInt32();
      break;
    case 73:
      m_polyfaceMeshVertexIndex3 = reader->GetInt32();
      break;
    case 74:
      m_polyfaceMeshVertexIndex4 = reader->GetInt32();
      break;
    case 91:
      m_identifier = reader->GetInt32();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}

void EoDxfHatch::AddLine() {
  ClearEntities();
  if (m_hatchLoop) {
    auto entity = std::make_unique<EoDxfLine>();
    m_point = m_line = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::AddArc() {
  ClearEntities();
  if (m_hatchLoop) {
    auto entity = std::make_unique<EoDxfArc>();
    m_point = m_arc = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::AddEllipse() {
  ClearEntities();
  if (m_hatchLoop) {
    auto entity = std::make_unique<EoDxfEllipse>();
    m_point = m_ellipse = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::AddSpline() {
  ClearEntities();
  if (m_hatchLoop) {
    m_point = nullptr;
    auto entity = std::make_unique<EoDxfSpline>();
    m_spline = entity.get();
    m_hatchLoop->m_entities.push_back(std::move(entity));
  }
}

void EoDxfHatch::ClearEntities() noexcept {
  m_point = nullptr;
  m_line = nullptr;
  m_polyline = nullptr;
  m_arc = nullptr;
  m_ellipse = nullptr;
  m_spline = nullptr;
  m_polylineVertex = nullptr;
}

void EoDxfHatch::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      m_hatchPatternName = reader->GetUtf8String();
      break;
    case 70:
      m_solidFillFlag = reader->GetInt32();
      break;
    case 71:
      m_associativityFlag = reader->GetInt32();
      break;
    case 10:
      if (m_point) {
        m_point->m_firstPoint.x = reader->GetDouble();
      } else if (m_spline) {
        // Add routing for the active m_spline data groups
      } else if (m_polyline) {
        m_polylineVertex = &m_polyline->AddVertex();
        m_polylineVertex->x = reader->GetDouble();
      }
      break;
    case 20:
      if (m_point) {
        m_point->m_firstPoint.y = reader->GetDouble();
      } else if (m_spline) {
        // Add routing for the active m_spline data groups
      } else if (m_polylineVertex) {
        m_polylineVertex->y = reader->GetDouble();
      }
      break;
    case 11:
      if (m_line) {
        m_line->m_secondPoint.x = reader->GetDouble();
      } else if (m_ellipse) {
        m_ellipse->m_secondPoint.x = reader->GetDouble();
      }
      break;
    case 21:
      if (m_line) {
        m_line->m_secondPoint.y = reader->GetDouble();
      } else if (m_ellipse) {
        m_ellipse->m_secondPoint.y = reader->GetDouble();
      }
      break;
    case 40:
      if (m_arc) {
        m_arc->m_radius = reader->GetDouble();
      } else if (m_ellipse) {
        m_ellipse->m_ratio = reader->GetDouble();
      } else if (m_spline) {
        // Add routing for the active m_spline data groups
      }
      break;
    case 41:
      m_hatchPatternScaleOrSpacing = reader->GetDouble();
      break;
    case 42:
      if (m_polylineVertex) { m_polylineVertex->bulge = reader->GetDouble(); }
      break;
    case 50:
      if (m_arc) {
        m_arc->m_startAngle = reader->GetDouble() * EoDxf::DegreesToRadians;
      } else if (m_ellipse) {
        m_ellipse->m_startParam = reader->GetDouble();
      }
      break;
    case 51:
      if (m_arc) {
        m_arc->m_endAngle = reader->GetDouble() * EoDxf::DegreesToRadians;
      } else if (m_ellipse) {
        m_ellipse->m_endParam = reader->GetDouble();
      }
      break;
    case 52:
      m_hatchPatternAngle = reader->GetDouble();
      break;
    case 73:
      if (m_arc) {
        m_arc->m_isCounterClockwise = reader->GetInt32();
      } else if (m_ellipse) {
        m_ellipse->m_isCounterClockwise = reader->GetInt32();
      } else if (m_polyline) {
        m_polyline->m_polylineFlag = reader->GetInt32();
      }
      break;
    case 75:
      m_hatchStyle = reader->GetInt32();
      break;
    case 76:
      m_hatchPatternType = reader->GetInt32();
      break;
    case 77:
      m_hatchPatternDoubleFlag = reader->GetInt32();
      break;
    case 78:
      m_numberOfPatternDefinitionLines = reader->GetInt32();
      break;

    case 91:
      m_numberOfBoundaryPaths = reader->GetInt32();
      m_hatchLoops.reserve(m_numberOfBoundaryPaths);
      break;

    // Hatch boundary path data groups
    case 72:
      if (m_isPolyline) {
        break;
      } else {
        int edgeType = reader->GetInt32();
        if (edgeType == 1) {  // line
          AddLine();
        } else if (edgeType == 2) {  // circular arc
          AddArc();
        } else if (edgeType == 3) {  // elliptic arc
          AddEllipse();
        } else if (edgeType == 4) {  // spline
          AddSpline();
        }
      }
      break;
    case 92: {
      auto boundaryPathType = reader->GetInt32();
      m_hatchLoop = new DRW_HatchLoop(boundaryPathType);
      m_hatchLoops.push_back(m_hatchLoop);
      if (boundaryPathType & 2) {  // polyline
        m_isPolyline = true;
        ClearEntities();
        auto entity = std::make_unique<EoDxfLwPolyline>();
        m_polyline = entity.get();
        m_hatchLoop->m_entities.push_back(std::move(entity));
      } else {
        m_isPolyline = false;
      }
      break;
    }
    case 93:
      if (m_polyline) {
        m_polyline->m_numberOfVertices = reader->GetInt32();
        m_polyline->m_vertices.reserve(m_polyline->m_numberOfVertices);  // modern reserve on new container
      } else if (m_hatchLoop) {
        m_hatchLoop->m_numberOfEdges = reader->GetInt32();
      }
      break;

    // Spline-specific data groups (94, 95, 96) are stored on the active m_spline if it exists, otherwise ignored
    case 94:
      if (m_spline) { m_spline->m_degreeOfTheSplineCurve = reader->GetInt32(); }
      break;
    case 95:
      if (m_spline) {
        m_spline->m_numberOfKnots = reader->GetInt32();
        m_spline->m_knotValues.reserve(m_spline->m_numberOfKnots);
      }
      break;
    case 96:
      if (m_spline) { m_spline->m_numberOfControlPoints = reader->GetInt32(); }
      break;

    case 98:
      ClearEntities();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}

void EoDxfSpline::ParseCode(int code, dxfReader* reader) {
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
      m_splineFlag = reader->GetInt32();
      break;
    case 71:
      m_degreeOfTheSplineCurve = reader->GetInt32();
      break;
    case 72:
      m_numberOfKnots = reader->GetInt32();
      break;
    case 73:
      m_numberOfControlPoints = reader->GetInt32();
      break;
    case 74:
      m_numberOfFitPoints = reader->GetInt32();
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
      m_controlPoint = new DRW_Coord();
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
      m_fitPoint = new DRW_Coord();
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
      EoDxfEntity::ParseCode(code, reader);
      break;
  }
}

void EoDxfImage::ParseCode(int code, dxfReader* reader) {
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
      ref = reader->GetHandleString();
      break;
    case 280:
      clip = reader->GetInt32();
      break;
    case 281:
      brightness = reader->GetInt32();
      break;
    case 282:
      contrast = reader->GetInt32();
      break;
    case 283:
      fade = reader->GetInt32();
      break;
    default:
      EoDxfLine::ParseCode(code, reader);
      break;
  }
}

void EoDxfDimension::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 1:
      text = reader->GetUtf8String();
      break;
    case 2:
      name = reader->GetString();
      break;
    case 3:
      style = reader->GetUtf8String();
      break;
    case 70:
      type = reader->GetInt32();
      break;
    case 71:
      align = reader->GetInt32();
      break;
    case 72:
      linesty = reader->GetInt32();
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
      EoDxfEntity::ParseCode(code, reader);
      break;
  }
}

void DRW_Leader::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 3:
      style = reader->GetUtf8String();
      break;
    case 71:
      arrow = reader->GetInt32();
      break;
    case 72:
      leadertype = reader->GetInt32();
      break;
    case 73:
      flag = reader->GetInt32();
      break;
    case 74:
      hookline = reader->GetInt32();
      break;
    case 75:
      hookflag = reader->GetInt32();
      break;
    case 76:
      vertnum = reader->GetInt32();
      break;
    case 77:
      coloruse = reader->GetInt32();
      break;
    case 40:
      textheight = reader->GetDouble();
      break;
    case 41:
      textwidth = reader->GetDouble();
      break;
    case 10: {
      vertexpoint = new DRW_Coord();
      vertexlist.push_back(vertexpoint);
      vertexpoint->x = reader->GetDouble();
      break;
    }
    case 20:
      if (vertexpoint != nullptr) { vertexpoint->y = reader->GetDouble(); }
      break;
    case 30:
      if (vertexpoint != nullptr) { vertexpoint->z = reader->GetDouble(); }
      break;
    case 340:
      annotHandle = reader->GetHandleString();
      break;
    case 210:
      extrusionPoint.x = reader->GetDouble();
      break;
    case 220:
      extrusionPoint.y = reader->GetDouble();
      break;
    case 230:
      extrusionPoint.z = reader->GetDouble();
      break;
    case 211:
      horizdir.x = reader->GetDouble();
      break;
    case 221:
      horizdir.y = reader->GetDouble();
      break;
    case 231:
      horizdir.z = reader->GetDouble();
      break;
    case 212:
      offsetblock.x = reader->GetDouble();
      break;
    case 222:
      offsetblock.y = reader->GetDouble();
      break;
    case 232:
      offsetblock.z = reader->GetDouble();
      break;
    case 213:
      offsettext.x = reader->GetDouble();
      break;
    case 223:
      offsettext.y = reader->GetDouble();
      break;
    case 233:
      offsettext.z = reader->GetDouble();
      break;
    default:
      EoDxfEntity::ParseCode(code, reader);
      break;
  }
}

void EoDxfViewPort::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 40:
      pswidth = reader->GetDouble();
      break;
    case 41:
      psheight = reader->GetDouble();
      break;
    case 68:
      vpstatus = reader->GetInt32();
      break;
    case 69:
      vpID = reader->GetInt32();
      break;
    case 12: {
      centerPX = reader->GetDouble();
      break;
    }
    case 22:
      centerPY = reader->GetDouble();
      break;
    default:
      EoDxfPoint::ParseCode(code, reader);
      break;
  }
}
