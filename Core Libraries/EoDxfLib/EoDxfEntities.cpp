#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

EoDxfGraphic::EoDxfGraphic(const EoDxfGraphic& other)
    : EoDxfEntity{other},
      m_layer{other.m_layer},
      m_lineType{other.m_lineType},
      m_proxyEntityGraphicsData{other.m_proxyEntityGraphicsData},
      m_colorName{other.m_colorName},
      m_lineTypeScale{other.m_lineTypeScale},
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
      m_extrusionDirection{other.m_extrusionDirection},
      m_thickness{other.m_thickness},
      m_haveExtrusion{other.m_haveExtrusion} {}

EoDxfGraphic& EoDxfGraphic::operator=(const EoDxfGraphic& other) {
  if (this != &other) {
    EoDxfEntity::operator=(other);

    m_layer = other.m_layer;
    m_lineType = other.m_lineType;
    m_proxyEntityGraphicsData = other.m_proxyEntityGraphicsData;
    m_colorName = other.m_colorName;
    m_lineTypeScale = other.m_lineTypeScale;
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
    m_extrusionDirection = other.m_extrusionDirection;
    m_thickness = other.m_thickness;
    m_haveExtrusion = other.m_haveExtrusion;
  }
  return *this;
}

void EoDxfGraphic::Clear() {
  m_extendedData.clear();
  m_currentVariant = nullptr;
  // extend this later for more state reset if needed
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

void EoDxfGraphic::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 8:
      m_layer = reader.GetWideString();
      break;
    case 6:
      m_lineType = reader.GetWideString();
      break;
    case 62:
      m_color = reader.GetInt16();
      break;
    case 370:
      m_lineWeight = EoDxfLineWidths::dxfInt2lineWidth(reader.GetInt16());
      break;
    case 48:
      m_lineTypeScale = reader.GetDouble();
      break;
    case 60:
      m_visibilityFlag = reader.GetInt16();
      break;
    case 39:
      m_thickness = reader.GetDouble();
      break;
    case 210:
      m_haveExtrusion = true;
      m_extrusionDirection.x = reader.GetDouble();
      break;
    case 220:
      m_extrusionDirection.y = reader.GetDouble();
      break;
    case 230:
      m_extrusionDirection.z = reader.GetDouble();
      break;
    case 284:
      m_shadowMode = static_cast<EoDxf::ShadowMode>(reader.GetInt16());
      break;
    case 390:
      m_plotStyleHandle = reader.GetHandleString();
      break;
    case 420:
      m_color24 = reader.GetInt32();
      break;
    case 430:
      m_colorName = reader.GetWideString();
      break;
    case 440:
      m_transparency = static_cast<EoDxf::TransparencyCodes>(reader.GetInt32());
      break;
    case 67:
      m_space = static_cast<EoDxf::Space>(reader.GetInt16());
      break;
    default:
      EoDxfEntity::ParseCode(code, reader);
      break;
  }
}

void EoDxfPoint::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_pointLocation.x = reader.GetDouble();
      break;
    case 20:
      m_pointLocation.y = reader.GetDouble();
      break;
    case 30:
      m_pointLocation.z = reader.GetDouble();
      break;
    case 50:
      m_angleOfXAxis = reader.GetDouble() * EoDxf::DegreesToRadians;
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfLine::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_startPoint.x = reader.GetDouble();
      break;
    case 20:
      m_startPoint.y = reader.GetDouble();
      break;
    case 30:
      m_startPoint.z = reader.GetDouble();
      break;
    case 11:
      m_endPoint.x = reader.GetDouble();
      break;
    case 21:
      m_endPoint.y = reader.GetDouble();
      break;
    case 31:
      m_endPoint.z = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfRay::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_startPoint.x = reader.GetDouble();
      break;
    case 20:
      m_startPoint.y = reader.GetDouble();
      break;
    case 30:
      m_startPoint.z = reader.GetDouble();
      break;
    case 11:
      m_unitDirectionVector.x = reader.GetDouble();
      break;
    case 21:
      m_unitDirectionVector.y = reader.GetDouble();
      break;
    case 31:
      m_unitDirectionVector.z = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfXline::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_startPoint.x = reader.GetDouble();
      break;
    case 20:
      m_startPoint.y = reader.GetDouble();
      break;
    case 30:
      m_startPoint.z = reader.GetDouble();
      break;
    case 11:
      m_unitDirectionVector.x = reader.GetDouble();
      break;
    case 21:
      m_unitDirectionVector.y = reader.GetDouble();
      break;
    case 31:
      m_unitDirectionVector.z = reader.GetDouble();
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

void EoDxfCircle::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_centerPoint.x = reader.GetDouble();
      break;
    case 20:
      m_centerPoint.y = reader.GetDouble();
      break;
    case 30:
      m_centerPoint.z = reader.GetDouble();
      break;
    case 40:
      m_radius = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfArc::ApplyExtrusion() {
  if (!m_haveExtrusion) { return; }

  CalculateArbitraryAxis(m_extrusionDirection);
  ExtrudePointInPlace(m_extrusionDirection, m_centerPoint);

  /* If the extrusion vector has a z value less than 0, the angles for the arc have to be mirrored since DXF files use
   the right hand rule. Note that the following code only handles the special case where there is a 2D drawing with the
   z axis heading into the paper (or rather screen). An arbitrary extrusion axis (with x and y values greater than 1/64)
   may still have issues.*/

  if (fabs(m_extrusionDirection.x) >= 0.015625 || fabs(m_extrusionDirection.y) >= 0.015625 ||
      m_extrusionDirection.z >= 0.0) {
    return;
  }

  m_startAngle = EoDxf::Pi - m_startAngle;
  m_endAngle = EoDxf::Pi - m_endAngle;

  double temp = m_startAngle;
  m_startAngle = m_endAngle;
  m_endAngle = temp;
}

void EoDxfArc::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_centerPoint.x = reader.GetDouble();
      break;
    case 20:
      m_centerPoint.y = reader.GetDouble();
      break;
    case 30:
      m_centerPoint.z = reader.GetDouble();
      break;
    case 40:
      m_radius = reader.GetDouble();
      break;
    case 50:
      m_startAngle = reader.GetDouble() * EoDxf::DegreesToRadians;
      break;
    case 51:
      m_endAngle = reader.GetDouble() * EoDxf::DegreesToRadians;
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfEllipse::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_centerPoint.x = reader.GetDouble();
      break;
    case 20:
      m_centerPoint.y = reader.GetDouble();
      break;
    case 30:
      m_centerPoint.z = reader.GetDouble();
      break;
    case 11:
      m_endPointOfMajorAxis.x = reader.GetDouble();
      break;
    case 21:
      m_endPointOfMajorAxis.y = reader.GetDouble();
      break;
    case 31:
      m_endPointOfMajorAxis.z = reader.GetDouble();
      break;
    case 40:
      m_ratio = reader.GetDouble();
      break;
    case 41:
      m_startParam = reader.GetDouble();
      break;
    case 42:
      m_endParam = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfEllipse::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_endPointOfMajorAxis);
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
    double incX = m_endPointOfMajorAxis.x;
    m_endPointOfMajorAxis.x = -(m_endPointOfMajorAxis.y * m_ratio);
    m_endPointOfMajorAxis.y = incX * m_ratio;
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
  radMajor =
      sqrt(m_endPointOfMajorAxis.x * m_endPointOfMajorAxis.x + m_endPointOfMajorAxis.y * m_endPointOfMajorAxis.y);
  radMinor = radMajor * m_ratio;
  // calculate sin & cos of included angle
  incAngle = atan2(m_endPointOfMajorAxis.y, m_endPointOfMajorAxis.x);
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
    double x = m_centerPoint.x + (cosCurr * cosRot * radMajor) - (sinCurr * sinRot * radMinor);
    double y = m_centerPoint.y + (cosCurr * sinRot * radMajor) + (sinCurr * cosRot * radMinor);
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
    ExtrudePointInPlace(m_extrusionDirection, m_firstCorner);
    ExtrudePointInPlace(m_extrusionDirection, m_secondCorner);
    ExtrudePointInPlace(m_extrusionDirection, m_thirdCorner);
    ExtrudePointInPlace(m_extrusionDirection, m_fourthCorner);
  }
}

void EoDxfTrace::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_firstCorner.x = reader.GetDouble();
      break;
    case 20:
      m_firstCorner.y = reader.GetDouble();
      break;
    case 30:
      m_firstCorner.z = reader.GetDouble();
      break;
    case 11:
      m_secondCorner.x = reader.GetDouble();
      break;
    case 21:
      m_secondCorner.y = reader.GetDouble();
      break;
    case 31:
      m_secondCorner.z = reader.GetDouble();
      break;
    case 12:
      m_thirdCorner.x = reader.GetDouble();
      break;
    case 22:
      m_thirdCorner.y = reader.GetDouble();
      break;
    case 32:
      m_thirdCorner.z = reader.GetDouble();
      break;
    case 13:
      m_fourthCorner.x = reader.GetDouble();
      break;
    case 23:
      m_fourthCorner.y = reader.GetDouble();
      break;
    case 33:
      m_fourthCorner.z = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfSolid::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_firstCorner);
    ExtrudePointInPlace(m_extrusionDirection, m_secondCorner);
    ExtrudePointInPlace(m_extrusionDirection, m_thirdCorner);
    ExtrudePointInPlace(m_extrusionDirection, m_fourthCorner);
  }
}

void EoDxfSolid::ParseCode(int code, EoDxfReader& reader) { 
  switch (code) {
    case 10:
      m_firstCorner.x = reader.GetDouble();
      break;
    case 20:
      m_firstCorner.y = reader.GetDouble();
      break;
    case 30:
      m_firstCorner.z = reader.GetDouble();
      break;
    case 11:
      m_secondCorner.x = reader.GetDouble();
      break;
    case 21:
      m_secondCorner.y = reader.GetDouble();
      break;
    case 31:
      m_secondCorner.z = reader.GetDouble();
      break;
    case 12:
      m_thirdCorner.x = reader.GetDouble();
      break;
    case 22:
      m_thirdCorner.y = reader.GetDouble();
      break;
    case 32:
      m_thirdCorner.z = reader.GetDouble();
      break;
    case 13:
      m_fourthCorner.x = reader.GetDouble();
      break;
    case 23:
      m_fourthCorner.y = reader.GetDouble();
      break;
    case 33:
      m_fourthCorner.z = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxf3dFace::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_firstCorner.x = reader.GetDouble();
      break;
    case 20:
      m_firstCorner.y = reader.GetDouble();
      break;
    case 30:
      m_firstCorner.z = reader.GetDouble();
      break;
    case 11:
      m_secondCorner.x = reader.GetDouble();
      break;
    case 21:
      m_secondCorner.y = reader.GetDouble();
      break;
    case 31:
      m_secondCorner.z = reader.GetDouble();
      break;
    case 12:
      m_thirdCorner.x = reader.GetDouble();
      break;
    case 22:
      m_thirdCorner.y = reader.GetDouble();
      break;
    case 32:
      m_thirdCorner.z = reader.GetDouble();
      break;
    case 13:
      m_fourthCorner.x = reader.GetDouble();
      break;
    case 23:
      m_fourthCorner.y = reader.GetDouble();
      break;
    case 33:
      m_fourthCorner.z = reader.GetDouble();
      break;
    case 70:
      m_invisibleFlag = reader.GetInt16();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfBlock::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 2:
      [[fallthrough]];  // They are always identical in every valid DXF file (regular blocks, anonymous blocks, XREFs,
                        // overlays, everything). Both groups 2 and 3 contain the block name, and the DXF spec confirms
                        // this is intentional for backward compatibility. It seems that group 3 may be missing on read
                        // but it should be written out with both groups 2 and 3.
    case 3:
      m_blockName = reader.GetWideString();
      break;
    case 70:
      m_blockTypeFlags = reader.GetInt16();
      break;
    case 10:
      m_basePoint.x = reader.GetDouble();
      break;
    case 20:
      m_basePoint.y = reader.GetDouble();
      break;
    case 30:
      m_basePoint.z = reader.GetDouble();
      break;
    case 1:
      m_xrefPathName = reader.GetWideString();
      break;
    case 4:
      m_blockDescription = reader.GetWideString();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfInsert::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 2:
      m_blockName = reader.GetWideString();
      break;
    case 10:
      m_insertionPoint.x = reader.GetDouble();
      break;
    case 20:
      m_insertionPoint.y = reader.GetDouble();
      break;
    case 30:
      m_insertionPoint.z = reader.GetDouble();
      break;
    case 41:
      m_xScaleFactor = reader.GetDouble();
      break;
    case 42:
      m_yScaleFactor = reader.GetDouble();
      break;
    case 43:
      m_zScaleFactor = reader.GetDouble();
      break;
    case 50:
      m_rotationAngle = reader.GetDouble();
      m_rotationAngle = m_rotationAngle * EoDxf::DegreesToRadians;
      break;
    case 70:
      m_columnCount = reader.GetInt16();
      break;
    case 71:
      m_rowCount = reader.GetInt16();
      break;
    case 44:
      m_columnSpacing = reader.GetDouble();
      break;
    case 45:
      m_rowSpacing = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
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

void EoDxfLwPolyline::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10: {
      m_vertices.emplace_back();
      m_currentVertexIndex = static_cast<int>(m_vertices.size()) - 1;
      m_vertices.back().x = reader.GetDouble();
      break;
    }
    case 20:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].y = reader.GetDouble(); }
      break;
    case 40:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].stawidth = reader.GetDouble(); }
      break;
    case 41:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].endwidth = reader.GetDouble(); }
      break;
    case 42:
      if (m_currentVertexIndex >= 0) { m_vertices[m_currentVertexIndex].bulge = reader.GetDouble(); }
      break;
    case 38:
      m_elevation = reader.GetDouble();
      break;
    case 43:
      m_constantWidth = reader.GetDouble();
      break;
    case 70:
      m_polylineFlag = reader.GetInt16();
      break;
    case 90:
      m_numberOfVertices = reader.GetInt32();
      m_vertices.reserve(m_numberOfVertices);  // now reserves the correct container
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfMText::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_insertionPoint.x = reader.GetDouble();
      break;
    case 20:
      m_insertionPoint.y = reader.GetDouble();
      break;
    case 30:
      m_insertionPoint.z = reader.GetDouble();
      break;
    case 40:
      m_nominalTextHeight = reader.GetDouble();
      break;
    case 41:
      m_referenceRectangleWidth = reader.GetDouble();
      break;
    case 7:
      m_textStyleName = reader.GetWideString();
      break;
    case 50:
      m_rotationAngle = reader.GetDouble();
      break;

    case 11:
      m_haveXAxisDirection = true;
      m_xAxisDirectionVector.x = reader.GetDouble();
      break;
    case 21:
      m_xAxisDirectionVector.y = reader.GetDouble();
      break;
    case 31:
      m_xAxisDirectionVector.z = reader.GetDouble();
      break;

    case 71:
      m_attachmentPoint = static_cast<AttachmentPoint>(reader.GetInt16());
      break;
    case 72:
      m_drawingDirection = static_cast<DrawingDirection>(reader.GetInt16());
      break;
    case 73:
      m_lineSpacingStyle = static_cast<LineSpacingStyle>(reader.GetInt16());
      break;
    case 44:
      m_lineSpacingFactor = reader.GetDouble();
      break;

    case 1:  // final chunk (or only chunk if no continuation chunks) of text string
      m_textString += reader.GetWideString();
      break;
    case 3:  // continuation chunk (multiple allowed)
      m_textString += reader.GetWideString();
      break;

    // (AC1021+) ? must appear together
    case 90:
      m_backgroundFillSetting = reader.GetInt32();
      break;
    case 45:
      m_fillBoxScale = reader.GetDouble();
      break;
    case 63:
      m_backgroundFillColor = reader.GetInt16();
      break;
    case 421:
      m_backgroundColor = reader.GetInt32();
      break;
    case 431:
      m_backgroundColorName = reader.GetWideString();
      break;

    // calculated values (can ignore)
    case 42:
      m_horizontalWidth = reader.GetDouble();
      break;
    case 43:
      m_verticalHeight = reader.GetDouble();
      break;

    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfMText::UpdateAngle() {
  if (m_haveXAxisDirection) { m_rotationAngle = atan2(m_xAxisDirectionVector.y, m_xAxisDirectionVector.x); }
}

void EoDxfPolyline::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_polylineElevation.x = reader.GetDouble();  // always 0.0
      break;
    case 20:
      m_polylineElevation.y = reader.GetDouble();  // always 0.0
      break;
    case 30:
      m_polylineElevation.z = reader.GetDouble();
      break;
    case 70:
      m_polylineFlag = reader.GetInt16();
      break;
    case 40:
      m_defaultStartWidth = reader.GetDouble();
      break;
    case 41:
      m_defaultEndWidth = reader.GetDouble();
      break;
    case 71:
      m_polygonMeshVertexCountM = reader.GetInt16();
      break;
    case 72:
      m_polygonMeshVertexCountN = reader.GetInt16();
      break;
    case 73:
      m_smoothSurfaceDensityM = reader.GetInt16();
      break;
    case 74:
      m_smoothSurfaceDensityN = reader.GetInt16();
      break;
    case 75:
      m_curvesAndSmoothSurfaceType = reader.GetInt16();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfVertex::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_locationPoint.x = reader.GetDouble();
      break;
    case 20:
      m_locationPoint.y = reader.GetDouble();
      break;
    case 30:
      m_locationPoint.z = reader.GetDouble();
      break;
    case 70:
      m_vertexFlags = reader.GetInt16();
      break;
    case 40:
      m_startingWidth = reader.GetDouble();
      break;
    case 41:
      m_endingWidth = reader.GetDouble();
      break;
    case 42:
      m_bulge = reader.GetDouble();
      break;
    case 50:
      m_curveFitTangentDirection = reader.GetDouble();
      break;
    case 71:
      m_polyfaceMeshVertexIndex1 = reader.GetInt16();
      break;
    case 72:
      m_polyfaceMeshVertexIndex2 = reader.GetInt16();
      break;
    case 73:
      m_polyfaceMeshVertexIndex3 = reader.GetInt16();
      break;
    case 74:
      m_polyfaceMeshVertexIndex4 = reader.GetInt16();
      break;
    case 91:
      m_identifier = reader.GetInt32();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfSpline::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 210:
      m_normalVector.x = reader.GetDouble();
      break;
    case 220:
      m_normalVector.y = reader.GetDouble();
      break;
    case 230:
      m_normalVector.z = reader.GetDouble();
      break;
    case 12:
      m_startTangent.x = reader.GetDouble();
      break;
    case 22:
      m_startTangent.y = reader.GetDouble();
      break;
    case 32:
      m_startTangent.z = reader.GetDouble();
      break;
    case 13:
      m_endTangent.x = reader.GetDouble();
      break;
    case 23:
      m_endTangent.y = reader.GetDouble();
      break;
    case 33:
      m_endTangent.z = reader.GetDouble();
      break;
    case 70:
      m_splineFlag = reader.GetInt16();
      break;
    case 71:
      m_degreeOfTheSplineCurve = reader.GetInt16();
      break;
    case 72:
      m_numberOfKnots = reader.GetInt16();
      break;
    case 73:
      m_numberOfControlPoints = reader.GetInt16();
      break;
    case 74:
      m_numberOfFitPoints = reader.GetInt16();
      break;
    case 42:
      m_knotTolerance = reader.GetDouble();
      break;
    case 43:
      m_controlPointTolerance = reader.GetDouble();
      break;
    case 44:
      m_fitTolerance = reader.GetDouble();
      break;
    case 10:
      m_controlPoint = new EoDxfGeometryBase3d();
      m_controlPoints.push_back(m_controlPoint);
      m_controlPoint->x = reader.GetDouble();
      break;
    case 20:
      if (m_controlPoint != nullptr) { m_controlPoint->y = reader.GetDouble(); }
      break;
    case 30:
      if (m_controlPoint != nullptr) { m_controlPoint->z = reader.GetDouble(); }
      break;
    case 11:
      m_fitPoint = new EoDxfGeometryBase3d();
      m_fitPoints.push_back(m_fitPoint);
      m_fitPoint->x = reader.GetDouble();
      break;
    case 21:
      if (m_fitPoint != nullptr) { m_fitPoint->y = reader.GetDouble(); }
      break;
    case 31:
      if (m_fitPoint != nullptr) { m_fitPoint->z = reader.GetDouble(); }
      break;
    case 40:
      m_knotValues.push_back(reader.GetDouble());
      break;

    // case 41:
    //   break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfImage::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_insertionPoint.x = reader.GetDouble();
      break;
    case 20:
      m_insertionPoint.y = reader.GetDouble();
      break;
    case 30:
      m_insertionPoint.z = reader.GetDouble();
      break;
    case 11:
      m_uVector.x = reader.GetDouble();
      break;
    case 21:
      m_uVector.y = reader.GetDouble();
      break;
    case 31:
      m_uVector.z = reader.GetDouble();
      break;
    case 12:
      m_vVector.x = reader.GetDouble();
      break;
    case 22:
      m_vVector.y = reader.GetDouble();
      break;
    case 32:
      m_vVector.z = reader.GetDouble();
      break;
    case 13:
      m_uImageSizeInPixels = reader.GetDouble();
      break;
    case 23:
      m_vImageSizeInPixels = reader.GetDouble();
      break;
    case 340:
      m_imageDefinitionHandle = reader.GetHandleString();
      break;
    case 280:
      m_clippingState = reader.GetInt16();
      break;
    case 281:
      m_brightnessValue = reader.GetInt16();
      break;
    case 282:
      m_contrastValue = reader.GetInt16();
      break;
    case 283:
      m_fadeValue = reader.GetInt16();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfViewport::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_centerPoint.x = reader.GetDouble();
      break;
    case 20:
      m_centerPoint.y = reader.GetDouble();
      break;
    case 30:
      m_centerPoint.z = reader.GetDouble();
      break;
    case 12:
      m_viewCenter.x = reader.GetDouble();
      break;
    case 22:
      m_viewCenter.y = reader.GetDouble();
      break;
    case 13:
      m_snapBasePoint.x = reader.GetDouble();
      break;
    case 23:
      m_snapBasePoint.y = reader.GetDouble();
      break;
    case 14:
      m_snapSpacing.x = reader.GetDouble();
      break;
    case 24:
      m_snapSpacing.y = reader.GetDouble();
      break;
    case 15:
      m_gridSpacing.x = reader.GetDouble();
      break;
    case 25:
      m_gridSpacing.y = reader.GetDouble();
      break;
    case 16:
      m_viewDirection.x = reader.GetDouble();
      break;
    case 17:
      m_viewTargetPoint.x = reader.GetDouble();
      break;
    case 26:
      m_viewDirection.y = reader.GetDouble();
      break;
    case 27:
      m_viewTargetPoint.y = reader.GetDouble();
      break;
    case 36:
      m_viewDirection.z = reader.GetDouble();
      break;
    case 37:
      m_viewTargetPoint.z = reader.GetDouble();
      break;
    case 40:
      m_width = reader.GetDouble();
      break;
    case 41:
      m_height = reader.GetDouble();
      break;
    case 42:
      m_lensLength = reader.GetDouble();
      break;
    case 43:
      m_frontClipPlane = reader.GetDouble();
      break;
    case 44:
      m_backClipPlane = reader.GetDouble();
      break;
    case 45:
      m_viewHeight = reader.GetDouble();
      break;
    case 50:
      m_snapAngle = reader.GetDouble();
      break;
    case 51:
      m_twistAngle = reader.GetDouble();
      break;
    case 68:
      m_viewportStatus = reader.GetInt16();
      break;
    case 69:
      m_viewportId = reader.GetInt16();
      break;
    case 90:
      m_viewportStatusBitCodedFlags = reader.GetInt32();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}
