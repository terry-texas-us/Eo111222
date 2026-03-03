#include <cmath>
#include <list>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_entities.h"
#include "intern/dxfreader.h"

DRW_Entity::DRW_Entity(const DRW_Entity& other)
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
  for (const auto* v : other.m_extendedData) { m_extendedData.push_back(new DRW_Variant(*v)); }
}

DRW_Entity& DRW_Entity::operator=(const DRW_Entity& other) {
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
    for (const auto* v : other.m_extendedData) { m_extendedData.push_back(new DRW_Variant(*v)); }
  }
  return *this;
}

DRW_Entity::~DRW_Entity() { clearExtendedData(); }

void DRW_Entity::Clear() {
  clearExtendedData();
  // extend this later for more state reset if needed
}

void DRW_Entity::clearExtendedData() noexcept {
  for (auto* v : m_extendedData) { delete v; }
  m_extendedData.clear();
}

void DRW_Entity::CalculateArbitraryAxis(const DRW_Coord& extrusionDirection) {
  //Follow the arbitrary DXF definitions for extrusion axes.
  if (fabs(extrusionDirection.x) < 0.015625 && fabs(extrusionDirection.y) < 0.015625) {
    //If we get here, implement Ax = Wy x N where Wy is [0,1,0] per the DXF spec.
    //The cross product works out to Wy.y*N.z-Wy.z*N.y, Wy.z*N.x-Wy.x*N.z, Wy.x*N.y-Wy.y*N.x
    //Factoring in the fixed values for Wy gives N.z,0,-N.x
    extAxisX.x = extrusionDirection.z;
    extAxisX.y = 0;
    extAxisX.z = -extrusionDirection.x;
  } else {
    //Otherwise, implement Ax = Wz x N where Wz is [0,0,1] per the DXF spec.
    //The cross product works out to Wz.y*N.z-Wz.z*N.y, Wz.z*N.x-Wz.x*N.z, Wz.x*N.y-Wz.y*N.x
    //Factoring in the fixed values for Wz gives -N.y,N.x,0.
    extAxisX.x = -extrusionDirection.y;
    extAxisX.y = extrusionDirection.x;
    extAxisX.z = 0;
  }

  extAxisX.unitize();

  //Ay = N x Ax
  extAxisY.x = (extrusionDirection.y * extAxisX.z) - (extAxisX.y * extrusionDirection.z);
  extAxisY.y = (extrusionDirection.z * extAxisX.x) - (extAxisX.z * extrusionDirection.x);
  extAxisY.z = (extrusionDirection.x * extAxisX.y) - (extAxisX.x * extrusionDirection.y);

  extAxisY.unitize();
}

void DRW_Entity::ExtrudePoint(const DRW_Coord& extrusionDirection, DRW_Coord& point) const noexcept {
  double px = (extAxisX.x * point.x) + (extAxisY.x * point.y) + (extrusionDirection.x * point.z);
  double py = (extAxisX.y * point.x) + (extAxisY.y * point.y) + (extrusionDirection.y * point.z);
  double pz = (extAxisX.z * point.x) + (extAxisY.z * point.y) + (extrusionDirection.z * point.z);

  point.x = px;
  point.y = py;
  point.z = pz;
}

void DRW_Entity::ParseCode(int code, dxfReader* reader) {
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
      m_shadowMode = static_cast<DRW::ShadowMode>(reader->GetInt32());
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
      m_space = static_cast<DRW::Space>(reader->GetInt32());
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
      m_extendedData.push_back(new DRW_Variant(code, reader->GetString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
      m_currentVariant = new DRW_Variant(code, DRW_Coord(reader->GetDouble(), 0.0, 0.0));
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
      m_extendedData.push_back(new DRW_Variant(code, reader->GetDouble()));
      break;
    case 1070:
    case 1071:
      m_extendedData.push_back(new DRW_Variant(code, reader->GetInt32()));
      break;
    default:
      break;
  }
}

bool DRW_Entity::ParseAppDataGroup(dxfReader* reader) {
  std::list<DRW_Variant> groupList;

  DRW_Variant currentVariant;

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
      currentVariant = DRW_Variant{};
      currentVariant.addString(102, val);
    } else {
      currentVariant = DRW_Variant{};
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

void DRW_Point::ParseCode(int code, dxfReader* reader) {
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
      DRW_Entity::ParseCode(code, reader);
      break;
  }
}

void DRW_Line::ParseCode(int code, dxfReader* reader) {
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
      DRW_Point::ParseCode(code, reader);
      break;
  }
}

void DRW_Circle::ApplyExtrusion() {
  if (m_haveExtrusion) {
    //NOTE: Commenting these out causes the the arcs being tested to be located
    //on the other side of the y axis (all x dimensions are negated).
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePoint(m_extrusionDirection, m_firstPoint);
  }
}

void DRW_Circle::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 40:
      m_radius = reader->GetDouble();
      break;
    default:
      DRW_Point::ParseCode(code, reader);
      break;
  }
}

void DRW_Arc::ApplyExtrusion() {
  DRW_Circle::ApplyExtrusion();

  if (m_haveExtrusion) {
    // If the extrusion vector has a z value less than 0, the angles for the arc
    // have to be mirrored since DXF files use the right hand rule.
    // Note that the following code only handles the special case where there is a 2D
    // drawing with the z axis heading into the paper (or rather screen). An arbitrary
    // extrusion axis (with x and y values greater than 1/64) may still have issues.
    if (fabs(m_extrusionDirection.x) < 0.015625 && fabs(m_extrusionDirection.y) < 0.015625 &&
        m_extrusionDirection.z < 0.0) {
      m_startAngle = DRW::Pi - m_startAngle;
      m_endAngle = DRW::Pi - m_endAngle;

      double temp = m_startAngle;
      m_startAngle = m_endAngle;
      m_endAngle = temp;
    }
  }
}

void DRW_Arc::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 50:
      m_startAngle = reader->GetDouble() * DRW::DegreesToRadians;
      break;
    case 51:
      m_endAngle = reader->GetDouble() * DRW::DegreesToRadians;
      break;
    default:
      DRW_Circle::ParseCode(code, reader);
      break;
  }
}

void DRW_Ellipse::ParseCode(int code, dxfReader* reader) {
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
      DRW_Line::ParseCode(code, reader);
      break;
  }
}

void DRW_Ellipse::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePoint(m_extrusionDirection, m_secondPoint);
    double intialparam = m_startParam;
    if (m_extrusionDirection.z < 0.) {
      m_startParam = DRW::TwoPi - m_endParam;
      m_endParam = DRW::TwoPi - intialparam;
    }
  }
}

//if ratio > 1 minor axis are greather than major axis, correct it
void DRW_Ellipse::CorrectAxis() {
  bool complete = false;
  if (m_startParam == m_endParam) {
    m_startParam = 0.0;
    m_endParam = DRW::TwoPi;
    complete = true;
  }
  if (m_ratio > 1.0) {
    if (fabs(m_endParam - m_startParam - DRW::TwoPi) < 1.0e-10) { complete = true; }
    double incX = m_secondPoint.x;
    m_secondPoint.x = -(m_secondPoint.y * m_ratio);
    m_secondPoint.y = incX * m_ratio;
    m_ratio = 1.0 / m_ratio;
    if (!complete) {
      if (m_startParam < DRW::HalfPi) { m_startParam += DRW::TwoPi; }
      if (m_endParam < DRW::HalfPi) { m_endParam += DRW::TwoPi; }
      m_endParam -= DRW::HalfPi;
      m_startParam -= DRW::HalfPi;
    }
  }
}

//parts are the number of vertex to split polyline, default 128
void DRW_Ellipse::ToPolyline(DRW_Polyline* pol, int parts) {
  double radMajor, radMinor, cosRot, sinRot, incAngle, curAngle;
  double cosCurr, sinCurr;
  radMajor = sqrt(m_secondPoint.x * m_secondPoint.x + m_secondPoint.y * m_secondPoint.y);
  radMinor = radMajor * m_ratio;
  //calculate sin & cos of included angle
  incAngle = atan2(m_secondPoint.y, m_secondPoint.x);
  cosRot = cos(incAngle);
  sinRot = sin(incAngle);
  incAngle = DRW::TwoPi / parts;
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
    pol->addVertex(DRW_Vertex(x, y, 0.0, 0.0));
    curAngle = (++i) * incAngle;
  } while (i < parts);
  if (fabs(m_endParam - m_startParam - DRW::TwoPi) < 1.0e-10) { pol->flags = 1; }
  pol->m_layer = this->m_layer;
  pol->m_lineType = this->m_lineType;
  pol->m_color = this->m_color;
  pol->m_lineWeight = this->m_lineWeight;
  pol->m_extrusionDirection = this->m_extrusionDirection;
}

void DRW_Trace::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePoint(m_extrusionDirection, m_firstPoint);
    ExtrudePoint(m_extrusionDirection, m_secondPoint);
    ExtrudePoint(m_extrusionDirection, m_thirdPoint);
    ExtrudePoint(m_extrusionDirection, m_fourthPoint);
  }
}

void DRW_Trace::ParseCode(int code, dxfReader* reader) {
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
      DRW_Line::ParseCode(code, reader);
      break;
  }
}

void DRW_Solid::ParseCode(int code, dxfReader* reader) { DRW_Trace::ParseCode(code, reader); }

void DRW_3Dface::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      m_invisibleFlag = reader->GetInt32();
      break;
    default:
      DRW_Trace::ParseCode(code, reader);
      break;
  }
}

void DRW_Block::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      name = reader->GetUtf8String();
      break;
    case 70:
      flags = reader->GetInt32();
      break;
    default:
      DRW_Point::ParseCode(code, reader);
      break;
  }
}

void DRW_Insert::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      name = reader->GetUtf8String();
      break;
    case 41:
      xscale = reader->GetDouble();
      break;
    case 42:
      yscale = reader->GetDouble();
      break;
    case 43:
      zscale = reader->GetDouble();
      break;
    case 50:
      angle = reader->GetDouble();
      angle = angle * DRW::DegreesToRadians;
      break;
    case 70:
      colcount = reader->GetInt32();
      break;
    case 71:
      rowcount = reader->GetInt32();
      break;
    case 44:
      colspace = reader->GetDouble();
      break;
    case 45:
      rowspace = reader->GetDouble();
      break;
    default:
      DRW_Point::ParseCode(code, reader);
      break;
  }
}

void DRW_LWPolyline::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(extPoint);
    for (unsigned int i = 0; i < vertlist.size(); i++) {
      auto* vert = vertlist.at(i);
      DRW_Coord v(vert->x, vert->y, elevation);
      ExtrudePoint(extPoint, v);
      vert->x = v.x;
      vert->y = v.y;
    }
  }
}

void DRW_LWPolyline::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 10: {
      vertex = new DRW_Vertex2D();
      vertlist.push_back(vertex);
      vertex->x = reader->GetDouble();
      break;
    }
    case 20:
      if (vertex != nullptr) { vertex->y = reader->GetDouble(); }
      break;
    case 40:
      if (vertex != nullptr) { vertex->stawidth = reader->GetDouble(); }
      break;
    case 41:
      if (vertex != nullptr) { vertex->endwidth = reader->GetDouble(); }
      break;
    case 42:
      if (vertex != nullptr) { vertex->bulge = reader->GetDouble(); }
      break;
    case 38:
      elevation = reader->GetDouble();
      break;
    case 39:
      thickness = reader->GetDouble();
      break;
    case 43:
      width = reader->GetDouble();
      break;
    case 70:
      flags = reader->GetInt32();
      break;
    case 90:
      vertexnum = reader->GetInt32();
      vertlist.reserve(vertexnum);
      break;
    case 210:
      m_haveExtrusion = true;
      extPoint.x = reader->GetDouble();
      break;
    case 220:
      extPoint.y = reader->GetDouble();
      break;
    case 230:
      extPoint.z = reader->GetDouble();
      break;
    default:
      DRW_Entity::ParseCode(code, reader);
      break;
  }
}

void DRW_Text::ParseCode(int code, dxfReader* reader) {
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
      m_horizontalAlignment = (HAlign)reader->GetInt32();
      break;
    case 73:
      m_verticalAlignment = (VAlign)reader->GetInt32();
      break;
    case 1:
      m_string = reader->GetUtf8String();
      break;
    case 7:
      m_textStyleName = reader->GetUtf8String();
      break;
    default:
      DRW_Line::ParseCode(code, reader);
      break;
  }
}

void DRW_MText::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 1:
      m_string += reader->GetString();
      m_string = reader->ToUtf8String(m_string);
      break;
    case 11:
      m_haveXAxisDirection = true;
      DRW_Text::ParseCode(code, reader);
      break;
    case 3:
      m_string += reader->GetString();
      break;
    case 44:
      m_lineSpacingFactor = reader->GetDouble();
      break;
    default:
      DRW_Text::ParseCode(code, reader);
      break;
  }
}

void DRW_MText::UpdateAngle() {
  if (m_haveXAxisDirection) { m_textRotation = atan2(m_secondPoint.y, m_secondPoint.x) * DRW::RadiansToDegrees; }
}

void DRW_Polyline::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      flags = reader->GetInt32();
      break;
    case 40:
      defstawidth = reader->GetDouble();
      break;
    case 41:
      defendwidth = reader->GetDouble();
      break;
    case 71:
      vertexcount = reader->GetInt32();
      break;
    case 72:
      facecount = reader->GetInt32();
      break;
    case 73:
      smoothM = reader->GetInt32();
      break;
    case 74:
      smoothN = reader->GetInt32();
      break;
    case 75:
      curvetype = reader->GetInt32();
      break;
    default:
      DRW_Point::ParseCode(code, reader);
      break;
  }
}

void DRW_Vertex::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      flags = reader->GetInt32();
      break;
    case 40:
      stawidth = reader->GetDouble();
      break;
    case 41:
      endwidth = reader->GetDouble();
      break;
    case 42:
      bulge = reader->GetDouble();
      break;
    case 50:
      tgdir = reader->GetDouble();
      break;
    case 71:
      vindex1 = reader->GetInt32();
      break;
    case 72:
      vindex2 = reader->GetInt32();
      break;
    case 73:
      vindex3 = reader->GetInt32();
      break;
    case 74:
      vindex4 = reader->GetInt32();
      break;
    case 91:
      identifier = reader->GetInt32();
      break;
    default:
      DRW_Point::ParseCode(code, reader);
      break;
  }
}

void DRW_Hatch::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 2:
      name = reader->GetUtf8String();
      break;
    case 70:
      solid = reader->GetInt32();
      break;
    case 71:
      associative = reader->GetInt32();
      break;
    case 72: /*edge type*/
      if (m_isPolyline) {  // if is polyline is a as_bulge flag
        break;
      } else if (reader->GetInt32() == 1) {  //line
        addLine();
      } else if (reader->GetInt32() == 2) {  //arc
        addArc();
      } else if (reader->GetInt32() == 3) {  //elliptic arc
        addEllipse();
      } else if (reader->GetInt32() == 4) {  //spline
        addSpline();
      }
      break;
    case 10:
      if (pt) {
        pt->m_firstPoint.x = reader->GetDouble();
      } else if (m_polyline) {
        plvert = m_polyline->addVertex();
        plvert->x = reader->GetDouble();
      }
      break;
    case 20:
      if (pt) {
        pt->m_firstPoint.y = reader->GetDouble();
      } else if (plvert) {
        plvert->y = reader->GetDouble();
      }
      break;
    case 11:
      if (line) {
        line->m_secondPoint.x = reader->GetDouble();
      } else if (ellipse) {
        ellipse->m_secondPoint.x = reader->GetDouble();
      }
      break;
    case 21:
      if (line) {
        line->m_secondPoint.y = reader->GetDouble();
      } else if (ellipse) {
        ellipse->m_secondPoint.y = reader->GetDouble();
      }
      break;
    case 40:
      if (arc) {
        arc->m_radius = reader->GetDouble();
      } else if (ellipse) {
        ellipse->m_ratio = reader->GetDouble();
      }
      break;
    case 41:
      scale = reader->GetDouble();
      break;
    case 42:
      if (plvert) { plvert->bulge = reader->GetDouble(); }
      break;
    case 50:
      if (arc) {
        arc->m_startAngle = reader->GetDouble() * DRW::DegreesToRadians;
      } else if (ellipse) {
        ellipse->m_startParam = reader->GetDouble();
      }
      break;
    case 51:
      if (arc) {
        arc->m_endAngle = reader->GetDouble() * DRW::DegreesToRadians;
      } else if (ellipse) {
        ellipse->m_endParam = reader->GetDouble();
      }
      break;
    case 52:
      angle = reader->GetDouble();
      break;
    case 73:
      if (arc) {
        arc->m_isCounterClockwise = reader->GetInt32();
      } else if (m_polyline) {
        m_polyline->flags = reader->GetInt32();
      }
      break;
    case 75:
      hstyle = reader->GetInt32();
      break;
    case 76:
      hpattern = reader->GetInt32();
      break;
    case 77:
      doubleflag = reader->GetInt32();
      break;
    case 78:
      deflines = reader->GetInt32();
      break;
    case 91:
      loopsnum = reader->GetInt32();
      looplist.reserve(loopsnum);
      break;
    case 92: {
      int32_t boundaryPathType = reader->GetInt32();
      loop = new DRW_HatchLoop(boundaryPathType);
      looplist.push_back(loop);
      if (boundaryPathType & 2) {  // Boundary-path type (2 = polyline)
        m_isPolyline = true;
        clearEntities();
        m_polyline = new DRW_LWPolyline;
        loop->objlist.push_back(m_polyline);
      } else {
        m_isPolyline = false;
      }
      break;
    }
    case 93:
      if (m_polyline) {
        m_polyline->vertexnum = reader->GetInt32();
      } else {
        loop->numedges = reader->GetInt32();
      }
      break;
    case 98:  //seed points ??
      clearEntities();
      break;
    default:
      DRW_Point::ParseCode(code, reader);
      break;
  }
}

void DRW_Spline::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 210:
      normalVec.x = reader->GetDouble();
      break;
    case 220:
      normalVec.y = reader->GetDouble();
      break;
    case 230:
      normalVec.z = reader->GetDouble();
      break;
    case 12:
      tgStart.x = reader->GetDouble();
      break;
    case 22:
      tgStart.y = reader->GetDouble();
      break;
    case 32:
      tgStart.z = reader->GetDouble();
      break;
    case 13:
      tgEnd.x = reader->GetDouble();
      break;
    case 23:
      tgEnd.y = reader->GetDouble();
      break;
    case 33:
      tgEnd.z = reader->GetDouble();
      break;
    case 70:
      flags = reader->GetInt32();
      break;
    case 71:
      degree = reader->GetInt32();
      break;
    case 72:
      nknots = reader->GetInt32();
      break;
    case 73:
      ncontrol = reader->GetInt32();
      break;
    case 74:
      nfit = reader->GetInt32();
      break;
    case 42:
      tolknot = reader->GetDouble();
      break;
    case 43:
      tolcontrol = reader->GetDouble();
      break;
    case 44:
      tolfit = reader->GetDouble();
      break;
    case 10: {
      controlpoint = new DRW_Coord();
      controllist.push_back(controlpoint);
      controlpoint->x = reader->GetDouble();
      break;
    }
    case 20:
      if (controlpoint != nullptr) { controlpoint->y = reader->GetDouble(); }
      break;
    case 30:
      if (controlpoint != nullptr) { controlpoint->z = reader->GetDouble(); }
      break;
    case 11: {
      fitpoint = new DRW_Coord();
      fitlist.push_back(fitpoint);
      fitpoint->x = reader->GetDouble();
      break;
    }
    case 21:
      if (fitpoint != nullptr) { fitpoint->y = reader->GetDouble(); }
      break;
    case 31:
      if (fitpoint != nullptr) { fitpoint->z = reader->GetDouble(); }
      break;
    case 40:
      knotslist.push_back(reader->GetDouble());
      break;
      //    case 41:
      //        break;
    default:
      DRW_Entity::ParseCode(code, reader);
      break;
  }
}

void DRW_Image::ParseCode(int code, dxfReader* reader) {
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
      DRW_Line::ParseCode(code, reader);
      break;
  }
}

void DRW_Dimension::ParseCode(int code, dxfReader* reader) {
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
      DRW_Entity::ParseCode(code, reader);
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
      DRW_Entity::ParseCode(code, reader);
      break;
  }
}

void DRW_Viewport::ParseCode(int code, dxfReader* reader) {
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
      DRW_Point::ParseCode(code, reader);
      break;
  }
}
