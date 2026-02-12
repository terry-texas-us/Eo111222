#include "Stdafx.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbConic.h"
#include "EoDbDimension.h"
#include "EoDbEllipse.h"
#include "EoDbGroup.h"
#include "EoDbJobFile.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDbSpline.h"
#include "EoDbText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeVector3d.h"

/// <remarks>
/// Vax: the excess 128 exponent .. range is -128 (0x00 - 0x80) to 127 (0xff - 0x80)
/// MS: the excess 127 exponent .. range is -127 (0x00 - 0x7f) to 128 (0xff - 0x7f)
/// </remarks>
class CVaxFloat {
  float m_f;

 public:
  CVaxFloat() { m_f = 0.f; }
  void Convert(const double&);
  double Convert();
};
double CVaxFloat::Convert() {
  float fMS = 0.f;

  BYTE* pvax = (BYTE*)&m_f;
  BYTE* pms = (BYTE*)&fMS;

  BYTE bSign = BYTE(pvax[1] & 0x80);
  BYTE bExp = BYTE((pvax[1] << 1) & 0xff);
  bExp |= pvax[0] >> 7;

  if (bExp == 0) {
    if (bSign != 0) { throw std::runtime_error("CVaxFloat: Conversion to MS - Reserve operand fault"); }
  } else if (bExp == 1) {  // this is a valid vax exponent but because the vax places the hidden
                           // leading 1 to the right of the binary point we have a problem ..
                           // the possible values are 2.94e-39 to 5.88e-39 .. just call it 0.
  } else {                 // - 128 + 127 - 1 (to get hidden 1 to the left of the binary point)
    bExp -= 2;

    pms[3] = BYTE(bExp >> 1);
    pms[3] |= bSign;

    pms[2] = BYTE((bExp << 7) & 0xff);
    pms[2] |= pvax[0] & 0x7f;

    pms[1] = pvax[3];
    pms[0] = pvax[2];
  }
  return (double(fMS));
}
void CVaxFloat::Convert(const double& dMS) {
  float fMS = float(dMS);
  float fVax = 0.f;

  if (fMS != 0.f) {
    BYTE* pMS = (BYTE*)&fMS;
    BYTE* pVax = (BYTE*)&fVax;

    BYTE bSign = BYTE(pMS[3] & 0x80);
    BYTE bExp = BYTE((pMS[3] << 1) & 0xff);
    bExp |= pMS[2] >> 7;

    if (bExp > 0xfd) bExp = 0xfd;

    // - 127 + 128 + 1 (to get hidden 1 to the right of the binary point)
    bExp += 2;

    pVax[1] = BYTE(bExp >> 1);
    pVax[1] |= bSign;

    pVax[0] = BYTE((bExp << 7) & 0xff);
    pVax[0] |= pMS[2] & 0x7f;

    pVax[3] = pMS[1];
    pVax[2] = pMS[0];
  }
  m_f = fVax;
}
class CVaxPnt {
  CVaxFloat x;
  CVaxFloat y;
  CVaxFloat z;

 public:
  CVaxPnt() {}
  void Convert(EoGePoint3d point) {
    x.Convert(point.x);
    y.Convert(point.y);
    z.Convert(point.z);
  }
  EoGePoint3d Convert() { return EoGePoint3d(x.Convert(), y.Convert(), z.Convert()); }
};
class CVaxVec {
  CVaxFloat x;
  CVaxFloat y;
  CVaxFloat z;

 public:
  CVaxVec() {}
  void Convert(EoGeVector3d v) {
    x.Convert(v.x);
    y.Convert(v.y);
    z.Convert(v.z);
  }
  EoGeVector3d Convert() { return EoGeVector3d(x.Convert(), y.Convert(), z.Convert()); }
};

void EoDbJobFile::ReadMemFile(CFile& file, EoGeVector3d translateVector) {
  auto* document = AeSysDoc::GetDoc();

  document->RemoveAllTrappedGroups();

  EoDbGroup* Group;
  while (GetNextVisibleGroup(file, Group)) {
    document->AddWorkLayerGroup(Group);
    document->AddGroupToTrap(Group);
  }
  document->TranslateTrappedGroups(translateVector);
}
void EoDbJobFile::ReadHeader(CFile& file) {
  if (file.Read(m_PrimBuf, 32) == 32) {
    m_Version = Version();

    if (m_Version == 1) {
      file.SeekToBegin();
    } else {
      if (file.GetLength() >= 96) { file.Seek(96, CFile::begin); }
    }
  }
}

void EoDbJobFile::ReadLayer(CFile& file, EoDbLayer* layer) {
  EoDbGroup* group{};

  while (GetNextVisibleGroup(file, group)) {
    if (group != nullptr) { layer->AddTail(group); }
  }
}

bool EoDbJobFile::GetNextVisibleGroup(CFile& file, EoDbGroup*& group) {
  auto Position = file.GetPosition();

  group = 0;
  try {
    EoDbPrimitive* primitive{};
    if (!GetNextPrimitive(file, primitive)) { return false; }
    group = new EoDbGroup(primitive);
    std::uint16_t numberOfPrimitives = *((std::uint16_t*)((m_Version == 1) ? &m_PrimBuf[2] : &m_PrimBuf[1]));
    for (std::uint16_t w = 1; w < numberOfPrimitives; w++) {
      try {
        Position = file.GetPosition();
        if (!GetNextPrimitive(file, primitive)) {
          throw std::runtime_error("Exception.FileJob: Unexpected end of file.");
        }
        group->AddTail(primitive);
      } catch (const std::runtime_error& ex) {
        app.AddStringToMessageList(std::wstring(ex.what(), ex.what() + strlen(ex.what())));
        file.Seek(static_cast<LONGLONG>(Position + 32), CFile::begin);
      }
    }
  } catch (const std::runtime_error& ex) {
    if (Position >= 96) {
      std::wstring errorMessage(ex.what(), ex.what() + strlen(ex.what()));
      if (::MessageBoxW(0, errorMessage.c_str(), 0, MB_ICONERROR | MB_RETRYCANCEL) == IDCANCEL) { return false; }
    }
    file.Seek(static_cast<LONGLONG>(Position + 32), CFile::begin);
  }
  return true;
}

bool EoDbJobFile::GetNextPrimitive(CFile& file, EoDbPrimitive*& primitive) {
  std::int16_t PrimitiveType = 0;
  do {
    if (!ReadNextPrimitive(file, m_PrimBuf, PrimitiveType)) { return false; }
  } while (PrimitiveType <= 0);
  ConstructPrimitive(primitive, PrimitiveType);
  return true;
}
bool EoDbJobFile::ReadNextPrimitive(CFile& file, std::uint8_t* buffer, std::int16_t& primitiveType) const {
  if (file.Read(buffer, 32) < 32) { return false; }
  primitiveType = *((std::int16_t*)&buffer[4]);

  if (!IsValidPrimitive(primitiveType)) { throw std::runtime_error("Exception.FileJob: Invalid primitive type."); }
  int LengthInChunks = (m_Version == 1) ? buffer[6] : buffer[3];
  if (LengthInChunks > 1) {
    UINT BytesRemaining = static_cast<UINT>(LengthInChunks - 1) * 32U;

    if (BytesRemaining >= EoDbPrimitive::BUFFER_SIZE - 32) {
      throw std::runtime_error("Exception.FileJob: Primitive buffer overflow.");
    }
    if (file.Read(&buffer[32], BytesRemaining) < BytesRemaining) {
      throw std::runtime_error("Exception.FileJob: Unexpected end of file.");
    }
  }
  return true;
}
int EoDbJobFile::Version() {
  switch (m_PrimBuf[5]) {
    case 17:   // 0x11 text
    case 24:   // 0x18 bspline
    case 33:   // 0x21 conic
    case 61:   // 0x3D arc
    case 67:   // 0x43 line
    case 70:   // 0x46 point
    case 100:  // 0x64 polygon
      m_Version = 1;
      break;

    default:
      m_Version = 3;
  }
  return m_Version;
}
bool EoDbJobFile::IsValidPrimitive(std::int16_t primitiveType) {
  switch (primitiveType) {
    case EoDb::kPointPrimitive:      // 0x0100
    case EoDb::kLinePrimitive:       // 0x0200
    case EoDb::kPolygonPrimitive:    // 0x0400
    case EoDb::kEllipsePrimitive:    // 0x1003
    case EoDb::kSplinePrimitive:     // 0x2000
    case EoDb::kCSplinePrimitive:    // 0x2001
    case EoDb::kTextPrimitive:       // 0x4000
    case EoDb::kTagPrimitive:        // 0x4100
    case EoDb::kDimensionPrimitive:  // 0x4200
      return true;

    default:
      return IsValidVersion1Primitive(primitiveType);
  }
}
bool EoDbJobFile::IsValidVersion1Primitive(std::int16_t primitiveType) {
  std::uint8_t* PrimitiveType = (std::uint8_t*)&primitiveType;
  switch (PrimitiveType[1]) {
    case 17:   // 0x11 text
    case 24:   // 0x18 bspline
    case 33:   // 0x21 conic
    case 61:   // 0x3d arc
    case 67:   // 0x43 line
    case 70:   // 0x46 point
    case 100:  // 0x64 polygon
      return true;

    default:
      return false;
  }
}
void EoDbJobFile::WriteHeader(CFile& file) {
  ::ZeroMemory(m_PrimBuf, 96);
  m_PrimBuf[4] = 'T';
  m_PrimBuf[5] = 'c';
  file.Write(m_PrimBuf, 96);
}
void EoDbJobFile::WriteLayer(CFile& file, EoDbLayer* layer) {
  layer->BreakSegRefs();
  layer->BreakPolylines();

  auto Position = layer->GetHeadPosition();
  while (Position != nullptr) {
    auto* Group = layer->GetNext(Position);
    WriteGroup(file, Group);
  }
}

void EoDbJobFile::WriteGroup(CFile& file, EoDbGroup* group) {
  m_PrimBuf[0] = 0;
  *((std::uint16_t*)&m_PrimBuf[1]) = std::uint16_t(group->GetCount());

  auto Position = group->GetHeadPosition();
  while (Position != nullptr) {
    auto* primitive = group->GetNext(Position);
    primitive->Write(file, m_PrimBuf);
  }
}

void EoDbJobFile::ConstructPrimitive(EoDbPrimitive*& primitive, std::int16_t PrimitiveType) {
  switch (PrimitiveType) {
    case EoDb::kTagPrimitive:
      ConvertTagToPoint();
      [[fallthrough]];  // fall through and construct a EoDbPoint instead
    case EoDb::kPointPrimitive:
      primitive = ConvertPointPrimitive();
      break;
    case EoDb::kLinePrimitive:
      primitive = ConvertLinePrimitive();
      break;
    case EoDb::kPolygonPrimitive:
      primitive = new EoDbPolygon(m_PrimBuf, 3);
      break;
    case EoDb::kEllipsePrimitive:
      primitive = ConvertEllipsePrimitive();
      break;
    case EoDb::kConicPrimitive:
      break;
    case EoDb::kCSplinePrimitive:
      ConvertCSplineToBSpline();
      [[fallthrough]];  // fall through and construct a EoDbSpline instead
    case EoDb::kSplinePrimitive:
      primitive = new EoDbSpline(m_PrimBuf, 3);
      break;
    case EoDb::kTextPrimitive:
      primitive = new EoDbText(m_PrimBuf, 3);
      static_cast<EoDbText*>(primitive)->ConvertFormattingCharacters();
      break;
    case EoDb::kDimensionPrimitive:
      primitive = new EoDbDimension(m_PrimBuf);
      break;

    default:
      ConstructPrimitiveFromVersion1(primitive);
  }
}
void EoDbJobFile::ConstructPrimitiveFromVersion1(EoDbPrimitive*& primitive) {
  switch (m_PrimBuf[5]) {
    case 17:
      primitive = new EoDbText(m_PrimBuf, 1);
      static_cast<EoDbText*>(primitive)->ConvertFormattingCharacters();
      break;
    case 24:
      primitive = new EoDbSpline(m_PrimBuf, 1);
      break;
    case 33:
      break;
    case 61:
      primitive = ConvertVersion1EllipsePrimitive();
      break;
    case 67:
      primitive = ConvertVersion1LinePrimitive();
      break;
    case 70:
      primitive = ConvertVersion1PointPrimitive();
      break;
    case 100:
      primitive = new EoDbPolygon(m_PrimBuf, 1);
      break;

    default:
      throw std::runtime_error("Exception.FileJob: Invalid primitive type.");
  }
}

EoDbPrimitive* EoDbJobFile::ConvertEllipsePrimitive() {
  auto color = std::int16_t(m_PrimBuf[6]);
  auto lineTypeIndex = std::int16_t(m_PrimBuf[7]);

  auto center = ((CVaxPnt*)&m_PrimBuf[8])->Convert();
  auto majorAxis = ((CVaxVec*)&m_PrimBuf[20])->Convert();
  auto minorAxis = ((CVaxVec*)&m_PrimBuf[32])->Convert();
  auto sweepAngle = ((CVaxFloat*)&m_PrimBuf[44])->Convert();

  if (sweepAngle > Eo::TwoPi || sweepAngle < -Eo::TwoPi) { sweepAngle = Eo::TwoPi; }

  /// @todo for negative z extrusion the positive sweep is incorrect

  auto* conic = EoDbConic::CreateConicFromEllipsePrimitive(center, majorAxis, minorAxis, sweepAngle);
  if (conic == nullptr) { throw std::runtime_error("Error creating Conic."); }
  conic->SetColor(color);
  conic->SetLineTypeIndex(lineTypeIndex);

  return conic;
}

EoDbPrimitive* EoDbJobFile::ConvertLinePrimitive() {
  std::int16_t color = std::int16_t(m_PrimBuf[6]);
  std::int16_t lineTypeIndex = std::int16_t(m_PrimBuf[7]);

  EoGeLine line;
  line.begin = ((CVaxPnt*)&m_PrimBuf[8])->Convert();
  line.end = ((CVaxPnt*)&m_PrimBuf[20])->Convert();

  return EoDbLine::CreateLine(line)->WithProperties(color, lineTypeIndex);
}

EoDbPrimitive* EoDbJobFile::ConvertPointPrimitive() {
  std::int16_t PenColor = std::int16_t(m_PrimBuf[6]);
  std::int16_t PointStyle = std::int16_t(m_PrimBuf[7]);

  EoGePoint3d Point = ((CVaxPnt*)&m_PrimBuf[8])->Convert();

  double Data[3]{};

  Data[0] = ((CVaxFloat*)&m_PrimBuf[20])->Convert();
  Data[1] = ((CVaxFloat*)&m_PrimBuf[24])->Convert();
  Data[2] = ((CVaxFloat*)&m_PrimBuf[28])->Convert();

  return new EoDbPoint(PenColor, PointStyle, Point, 3, Data);
}

EoDbPrimitive* EoDbJobFile::ConvertVersion1EllipsePrimitive() {
  auto color = std::int16_t(m_PrimBuf[4] & 0x000f);
  auto lineTypeIndex = std::int16_t((m_PrimBuf[4] & 0x00ff) >> 4);

  EoGePoint3d begin(((CVaxFloat*)&m_PrimBuf[8])->Convert(), ((CVaxFloat*)&m_PrimBuf[12])->Convert(), 0.0);
  begin *= 1.e-3;

  EoGePoint3d center(((CVaxFloat*)&m_PrimBuf[20])->Convert(), ((CVaxFloat*)&m_PrimBuf[24])->Convert(), 0.0);
  center *= 1.e-3;

  double sweepAngle = ((CVaxFloat*)&m_PrimBuf[28])->Convert();

  EoGeVector3d majorAxis;
  if (sweepAngle < 0.0) {
    EoGePoint3d point;
    point.x = (center.x + ((begin.x - center.x) * cos(sweepAngle) - (begin.y - center.y) * sin(sweepAngle)));
    point.y = (center.y + ((begin.x - center.x) * sin(sweepAngle) + (begin.y - center.y) * cos(sweepAngle)));
    majorAxis = point - center;
  } else {
    majorAxis = begin - center;
  }
  auto minorAxis = CrossProduct(EoGeVector3d::positiveUnitZ, majorAxis);
  sweepAngle = fabs(sweepAngle);

  auto* conic = EoDbConic::CreateConicFromEllipsePrimitive(center, majorAxis, minorAxis, sweepAngle);
  if (conic != nullptr) {
    conic->SetColor(color);
    conic->SetLineTypeIndex(lineTypeIndex);
  }
  return conic;
}

EoDbPrimitive* EoDbJobFile::ConvertVersion1LinePrimitive() {
  std::int16_t color = std::int16_t(m_PrimBuf[4] & 0x000f);
  std::int16_t lineTypeIndex = std::int16_t((m_PrimBuf[4] & 0x00ff) >> 4);

  EoGeLine line;
  line.begin = ((CVaxPnt*)&m_PrimBuf[8])->Convert() * 1.e-3;
  line.end = ((CVaxPnt*)&m_PrimBuf[20])->Convert() * 1.e-3;

  return EoDbLine::CreateLine(line)->WithProperties(color, lineTypeIndex);
}

EoDbPrimitive* EoDbJobFile::ConvertVersion1PointPrimitive() {
  std::int16_t PenColor = std::int16_t(m_PrimBuf[4] & 0x000f);
  std::int16_t PointStyle = std::int16_t((m_PrimBuf[4] & 0x00ff) >> 4);

  EoGePoint3d Point = ((CVaxPnt*)&m_PrimBuf[8])->Convert();
  Point *= 1.e-3;

  double Data[3]{};

  Data[0] = ((CVaxFloat*)&m_PrimBuf[20])->Convert();
  Data[1] = ((CVaxFloat*)&m_PrimBuf[24])->Convert();
  Data[2] = ((CVaxFloat*)&m_PrimBuf[28])->Convert();

  return new EoDbPoint(PenColor, PointStyle, Point, 3, Data);
}
void EoDbJobFile::ConvertCSplineToBSpline() {
  std::uint16_t NumberOfControlPoints = *((std::uint16_t*)&m_PrimBuf[10]);

  m_PrimBuf[3] = static_cast<std::uint8_t>((2 + NumberOfControlPoints * 3) / 8 + 1);
  *((std::uint16_t*)&m_PrimBuf[4]) = std::uint16_t(EoDb::kSplinePrimitive);
  m_PrimBuf[8] = m_PrimBuf[10];
  m_PrimBuf[9] = m_PrimBuf[11];
  ::MoveMemory(&m_PrimBuf[10], &m_PrimBuf[38], NumberOfControlPoints * 3 * sizeof(CVaxFloat));
}
void EoDbJobFile::ConvertTagToPoint() {
  *((std::uint16_t*)&m_PrimBuf[4]) = EoDb::kPointPrimitive;
  ::ZeroMemory(&m_PrimBuf[20], 12);
}
EoDbDimension::EoDbDimension(std::uint8_t* buffer) {
  m_color = std::int16_t(buffer[6]);
  m_lineTypeIndex = std::int16_t(buffer[7]);

  m_line.begin = ((CVaxPnt*)&buffer[8])->Convert();
  m_line.end = ((CVaxPnt*)&buffer[20])->Convert();

  m_color = std::int16_t(buffer[32]);

  m_fontDefinition.SetFontName(Eo::defaultStrokeFont);
  m_fontDefinition.SetPrecision(EoDb::Precision::StrokeType);
  m_fontDefinition.SetCharacterSpacing(((CVaxFloat*)&buffer[36])->Convert());
  m_fontDefinition.SetPath(static_cast<EoDb::Path>(buffer[40]));
  m_fontDefinition.SetAlignment(
      static_cast<EoDb::HorizontalAlignment>(buffer[41]), static_cast<EoDb::VerticalAlignment>(buffer[42]));
  m_ReferenceSystem.SetOrigin(((CVaxPnt*)&buffer[43])->Convert());
  m_ReferenceSystem.SetXDirection(((CVaxVec*)&buffer[55])->Convert());
  m_ReferenceSystem.SetYDirection(((CVaxVec*)&buffer[67])->Convert());

  std::int16_t TextLength = *((std::int16_t*)&buffer[79]);

  buffer[81 + TextLength] = '\0';
  m_text = CString((LPCSTR)&buffer[81]);
}

EoDbPolygon::EoDbPolygon(std::uint8_t* buffer, int version) {
  if (version == 1) {
    m_color = std::int16_t(buffer[4] & 0x000f);
    m_fillStyleIndex = 0;

    double polygonStyle = ((CVaxFloat*)&buffer[12])->Convert();
    m_polygonStyle = static_cast<EoDb::PolygonStyle>(static_cast<int>(polygonStyle) % 16);

    switch (m_polygonStyle) {
      case EoDb::PolygonStyle::Hatch: {
        double dXScal = ((CVaxFloat*)&buffer[16])->Convert();
        double dYScal = ((CVaxFloat*)&buffer[20])->Convert();
        double dAng = ((CVaxFloat*)&buffer[24])->Convert();

        m_positiveX.z = 0.0;
        m_positiveY.z = 0.0;

        if (fabs(dXScal) > Eo::geometricTolerance && fabs(dYScal) > Eo::geometricTolerance) {  // Have 2 hatch lines
          m_fillStyleIndex = 2;
          m_positiveX.x = cos(dAng);
          m_positiveX.y = sin(dAng);
          m_positiveY.x = -m_positiveX.y;
          m_positiveY.y = m_positiveX.x;
          m_positiveX *= dXScal * 1.e-3;
          m_positiveY *= dYScal * 1.e-3;
        } else if (fabs(dXScal) > Eo::geometricTolerance) {  // Vertical hatch lines
          m_fillStyleIndex = 1;
          m_positiveX.x = cos(dAng + Eo::HalfPi);
          m_positiveX.y = sin(dAng + Eo::HalfPi);
          m_positiveY.x = -m_positiveX.y;
          m_positiveY.y = m_positiveX.x;
          m_positiveY *= dXScal * 1.e-3;
        } else {  // Horizontal hatch lines
          m_fillStyleIndex = 1;
          m_positiveX.x = cos(dAng);
          m_positiveX.y = sin(dAng);
          m_positiveY.x = -m_positiveX.y;
          m_positiveY.y = m_positiveX.x;
          m_positiveY *= dYScal * 1.e-3;
        }
        break;
      }
      case EoDb::PolygonStyle::Hollow:
      case EoDb::PolygonStyle::Solid:
      case EoDb::PolygonStyle::Pattern:
        m_positiveX = EoGeVector3d::positiveUnitX;
        m_positiveY = EoGeVector3d::positiveUnitY;
        break;

      case EoDb::PolygonStyle::Special:
        [[fallthrough]];  // fall through and treat as default
      default:
        m_numberOfVertices = 3;
        m_vertices = new EoGePoint3d[m_numberOfVertices];
        m_vertices[0] = EoGePoint3d::kOrigin;
        m_vertices[1] = EoGePoint3d::kOrigin + EoGeVector3d::positiveUnitX;
        m_vertices[2] = EoGePoint3d::kOrigin + EoGeVector3d::positiveUnitY;
        m_hatchOrigin = m_vertices[0];
        return;
    }
    m_numberOfVertices = std::uint16_t(((CVaxFloat*)&buffer[8])->Convert());

    m_vertices = new EoGePoint3d[m_numberOfVertices];

    int bufferIndex{36};

    for (auto i = 0; i < m_numberOfVertices; i++) {
      m_vertices[i] = ((CVaxPnt*)&buffer[bufferIndex])->Convert() * 1.e-3;
      bufferIndex += sizeof(CVaxPnt);
    }
    m_hatchOrigin = m_vertices[0];
  } else {
    m_color = std::int16_t(buffer[6]);
    m_polygonStyle = static_cast<EoDb::PolygonStyle>(static_cast<int8_t>(buffer[7]));
    m_fillStyleIndex = *((std::int16_t*)&buffer[8]);
    m_numberOfVertices = static_cast<std::uint16_t>(*((std::int16_t*)&buffer[10]));
    m_hatchOrigin = ((CVaxPnt*)&buffer[12])->Convert();
    m_positiveX = ((CVaxVec*)&buffer[24])->Convert();
    m_positiveY = ((CVaxVec*)&buffer[36])->Convert();
    m_vertices = new EoGePoint3d[m_numberOfVertices];

    int bufferIndex{48};

    for (auto i = 0; i < m_numberOfVertices; i++) {
      m_vertices[i] = ((CVaxPnt*)&buffer[bufferIndex])->Convert();
      bufferIndex += sizeof(CVaxPnt);
    }
  }
}

EoDbSpline::EoDbSpline(std::uint8_t* buffer, int version) {
  if (version == 1) {
    m_color = std::int16_t(buffer[4] & 0x000f);
    m_lineTypeIndex = std::int16_t((buffer[4] & 0x00ff) >> 4);

    std::uint16_t wPts = std::uint16_t(((CVaxFloat*)&buffer[8])->Convert());

    int bufferIndex{12};

    for (auto i = 0; i < wPts; i++) {
      EoGePoint3d pt = ((CVaxPnt*)&buffer[bufferIndex])->Convert() * 1.e-3;
      m_pts.Add(pt);
      bufferIndex += sizeof(CVaxPnt);
    }
  } else {
    m_color = std::int16_t(buffer[6]);
    m_lineTypeIndex = std::int16_t(buffer[7]);

    std::uint16_t wPts = static_cast<std::uint16_t>(*((std::int16_t*)&buffer[8]));

    int bufferIndex{10};

    for (auto i = 0; i < wPts; i++) {
      EoGePoint3d pt = ((CVaxPnt*)&buffer[bufferIndex])->Convert();
      m_pts.Add(pt);
      bufferIndex += sizeof(CVaxPnt);
    }
  }
}

EoDbText::EoDbText(std::uint8_t* buffer, int version) {
  m_fontDefinition.SetPrecision(EoDb::Precision::StrokeType);
  m_fontDefinition.SetFontName(Eo::defaultStrokeFont);

  if (version == 1) {
    m_color = std::int16_t(buffer[4] & 0x000f);
    m_fontDefinition.SetCharacterSpacing(((CVaxFloat*)&buffer[36])->Convert());
    m_fontDefinition.SetCharacterSpacing(std::min(std::max(m_fontDefinition.CharacterSpacing(), 0.0), 4.0));

    double d = ((CVaxFloat*)&buffer[40])->Convert();

    m_fontDefinition.SetPath(static_cast<EoDb::Path>(fmod(d, 10.0)));
    if (m_fontDefinition.Path() < EoDb::Path::Right || m_fontDefinition.Path() > EoDb::Path::Down) {
      m_fontDefinition.SetPath(EoDb::Path::Right);
    }
    m_fontDefinition.SetHorizontalAlignment(static_cast<EoDb::HorizontalAlignment>(fmod(d / 10.0, 10.0)));
    if (m_fontDefinition.HorizontalAlignment() < EoDb::HorizontalAlignment::Left ||
        m_fontDefinition.HorizontalAlignment() > EoDb::HorizontalAlignment::Right) {
      m_fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
    }
    m_fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment(static_cast<std::uint16_t>((d / 100.0))));
    if (m_fontDefinition.VerticalAlignment() < EoDb::VerticalAlignment::Top ||
        m_fontDefinition.VerticalAlignment() > EoDb::VerticalAlignment::Bottom) {
      m_fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
    }

    m_ReferenceSystem.SetOrigin(((CVaxPnt*)&buffer[8])->Convert() * 1.e-3);

    double height = ((CVaxFloat*)&buffer[20])->Convert();
    height = std::min(std::max(height, 0.01e3), 100.e3);

    double expansionFactor = ((CVaxFloat*)&buffer[24])->Convert();
    expansionFactor = std::min(std::max(expansionFactor, 0.0), 10.0);

    m_ReferenceSystem.SetXDirection(
        EoGeVector3d(Eo::defaultCharacterCellAspectRatio * height * expansionFactor, 0.0, 0.0) * 1.e-3);
    m_ReferenceSystem.SetYDirection(EoGeVector3d(0.0, height, 0.0) * 1.e-3);

    double rotationAngle = ((CVaxFloat*)&buffer[28])->Convert();
    rotationAngle = std::min(std::max(rotationAngle, -Eo::TwoPi), Eo::TwoPi);

    if (fabs(rotationAngle) > Eo::geometricTolerance) {
      EoGeVector3d xDirection(m_ReferenceSystem.XDirection());
      xDirection = RotateVectorAboutZAxis(xDirection, rotationAngle);
      m_ReferenceSystem.SetXDirection(xDirection);
      EoGeVector3d yDirection(m_ReferenceSystem.YDirection());
      yDirection = RotateVectorAboutZAxis(yDirection, rotationAngle);
      m_ReferenceSystem.SetYDirection(yDirection);
    }
    char* NextToken = nullptr;
    char* pChr = strtok_s((char*)&buffer[44], "\\", &NextToken);

    if (pChr == 0)
      m_strText = L"EoDbJobFile.PrimText error: Missing string terminator.";
    else if (strlen(pChr) > 132)
      m_strText = L"EoDbJobFile.PrimText error: Text too long.";
    else {
      while (*pChr != 0) {
        if (!isprint(*pChr)) *pChr = '.';
        pChr++;
      }
      m_strText = &buffer[44];
    }
  } else {
    m_color = std::int16_t(buffer[6]);
    m_fontDefinition.SetCharacterSpacing(((CVaxFloat*)&buffer[10])->Convert());
    m_fontDefinition.SetPath(static_cast<EoDb::Path>(buffer[14]));
    m_fontDefinition.SetAlignment(
        static_cast<EoDb::HorizontalAlignment>(buffer[15]), static_cast<EoDb::VerticalAlignment>(buffer[16]));
    m_ReferenceSystem.SetOrigin(((CVaxPnt*)&buffer[17])->Convert());
    m_ReferenceSystem.SetXDirection(((CVaxVec*)&buffer[29])->Convert());
    m_ReferenceSystem.SetYDirection(((CVaxVec*)&buffer[41])->Convert());

    std::int16_t TextLength = *((std::int16_t*)&buffer[53]);
    buffer[55 + TextLength] = '\0';
    m_strText = CString((LPCSTR)&buffer[55]);
  }
}

void EoDbConic::Write(CFile& file, std::uint8_t* buffer) {
  buffer[3] = 2;
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kConicPrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_lineTypeIndex == LINETYPE_BYLAYER ? sm_layerLineTypeIndex : m_lineTypeIndex);
  if (buffer[7] >= 16) buffer[7] = 2;

  ((CVaxPnt*)&buffer[8])->Convert(m_center);
  ((CVaxVec*)&buffer[20])->Convert(m_majorAxis);
  ((CVaxVec*)&buffer[32])->Convert(MinorAxis());
  ((CVaxFloat*)&buffer[44])->Convert(SweepAngle());

  file.Write(buffer, 64);
}

void EoDbEllipse::Write(CFile& file, std::uint8_t* buffer) {
  buffer[3] = 2;
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kEllipsePrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_lineTypeIndex == LINETYPE_BYLAYER ? sm_layerLineTypeIndex : m_lineTypeIndex);
  if (buffer[7] >= 16) buffer[7] = 2;

  ((CVaxPnt*)&buffer[8])->Convert(m_center);
  ((CVaxVec*)&buffer[20])->Convert(m_majorAxis);
  ((CVaxVec*)&buffer[32])->Convert(m_minorAxis);
  ((CVaxFloat*)&buffer[44])->Convert(m_sweepAngle);

  file.Write(buffer, 64);
}
void EoDbDimension::Write(CFile& file, std::uint8_t* buffer) {
  std::uint16_t TextLength = std::uint16_t(m_text.GetLength());

  buffer[3] = std::uint8_t((118 + TextLength) / 32);
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kDimensionPrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_lineTypeIndex == LINETYPE_BYLAYER ? sm_layerLineTypeIndex : m_lineTypeIndex);
  if (buffer[7] >= 16) buffer[7] = 2;

  ((CVaxPnt*)&buffer[8])->Convert(m_line.begin);
  ((CVaxPnt*)&buffer[20])->Convert(m_line.end);

  buffer[32] = static_cast<std::uint8_t>(m_color);
  buffer[33] = std::int8_t(EoDb::Precision::StrokeType);
  *((std::int16_t*)&buffer[34]) = 0;
  ((CVaxFloat*)&buffer[36])->Convert(m_fontDefinition.CharacterSpacing());
  buffer[40] = static_cast<std::uint8_t>(m_fontDefinition.Path());
  buffer[41] = static_cast<std::uint8_t>(m_fontDefinition.HorizontalAlignment());
  buffer[42] = static_cast<std::uint8_t>(m_fontDefinition.VerticalAlignment());

  EoGeReferenceSystem ReferenceSystem = m_ReferenceSystem;

  ((CVaxPnt*)&buffer[43])->Convert(ReferenceSystem.Origin());
  ((CVaxVec*)&buffer[55])->Convert(ReferenceSystem.XDirection());
  ((CVaxVec*)&buffer[67])->Convert(ReferenceSystem.YDirection());

  *((std::int16_t*)&buffer[79]) = static_cast<std::int16_t>(TextLength);
  memcpy(&buffer[81], (LPCWSTR)m_text, TextLength);

  file.Write(buffer, static_cast<UINT>(buffer[3] * 32));
}
void EoDbLine::Write(CFile& file, std::uint8_t* buffer) {
  buffer[3] = 1;
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kLinePrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_lineTypeIndex == LINETYPE_BYLAYER ? sm_layerLineTypeIndex : m_lineTypeIndex);
  if (buffer[7] >= 16) buffer[7] = 2;

  ((CVaxPnt*)&buffer[8])->Convert(m_line.begin);
  ((CVaxPnt*)&buffer[20])->Convert(m_line.end);

  file.Write(buffer, 32);
}
void EoDbPoint::Write(CFile& file, std::uint8_t* buffer) {
  buffer[3] = 1;
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kPointPrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_pointStyle);

  ((CVaxPnt*)&buffer[8])->Convert(m_Point);

  ::ZeroMemory(&buffer[20], 12);

  int bufferIndex{20};

  for (auto i = 0; i < m_NumberOfDatums; i++) {
    ((CVaxFloat*)&buffer[bufferIndex])->Convert(m_Data[i]);
    bufferIndex += sizeof(CVaxFloat);
  }

  file.Write(buffer, 32);
}

void EoDbPolygon::Write(CFile& file, std::uint8_t* buffer) {
  buffer[3] = static_cast<std::uint8_t>((79 + m_numberOfVertices * 12) / 32);
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kPolygonPrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_polygonStyle);
  *((std::int16_t*)&buffer[8]) = m_fillStyleIndex;
  *((std::int16_t*)&buffer[10]) = static_cast<std::int16_t>(m_numberOfVertices);

  ((CVaxPnt*)&buffer[12])->Convert(m_hatchOrigin);
  ((CVaxVec*)&buffer[24])->Convert(m_positiveX);
  ((CVaxVec*)&buffer[36])->Convert(m_positiveY);

  int bufferIndex{48};

  for (auto i = 0; i < m_numberOfVertices; i++) {
    ((CVaxPnt*)&buffer[bufferIndex])->Convert(m_vertices[i]);
    bufferIndex += sizeof(CVaxPnt);
  }
  file.Write(buffer, static_cast<UINT>(buffer[3] * 32));
}

void EoDbSpline::Write(CFile& file, std::uint8_t* buffer) {
  buffer[3] = static_cast<std::uint8_t>((2 + m_pts.GetSize() * 3) / 8 + 1);
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kSplinePrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_lineTypeIndex == LINETYPE_BYLAYER ? sm_layerLineTypeIndex : m_lineTypeIndex);

  *((std::int16_t*)&buffer[8]) = (std::int16_t)m_pts.GetSize();

  int bufferIndex{10};

  for (auto i = 0; i < m_pts.GetSize(); i++) {
    ((CVaxPnt*)&buffer[bufferIndex])->Convert(m_pts[i]);
    bufferIndex += sizeof(CVaxPnt);
  }
  file.Write(buffer, static_cast<UINT>(buffer[3] * 32));
}

void EoDbText::Write(CFile& file, std::uint8_t* buffer) {
  std::uint16_t TextLength = std::uint16_t(m_strText.GetLength());

  buffer[3] = static_cast<std::uint8_t>((86 + TextLength) / 32);
  *((std::uint16_t*)&buffer[4]) = std::uint16_t(EoDb::kTextPrimitive);
  buffer[6] = static_cast<std::uint8_t>(m_color == COLOR_BYLAYER ? sm_layerColor : m_color);
  buffer[7] = static_cast<std::uint8_t>(m_fontDefinition.Precision());
  *((std::int16_t*)&buffer[8]) = 0;
  ((CVaxFloat*)&buffer[10])->Convert(m_fontDefinition.CharacterSpacing());
  buffer[14] = static_cast<std::uint8_t>(m_fontDefinition.Path());
  buffer[15] = static_cast<std::uint8_t>(m_fontDefinition.HorizontalAlignment());
  buffer[16] = static_cast<std::uint8_t>(m_fontDefinition.VerticalAlignment());

  EoGeReferenceSystem ReferenceSystem = m_ReferenceSystem;
  ((CVaxPnt*)&buffer[17])->Convert(ReferenceSystem.Origin());
  ((CVaxVec*)&buffer[29])->Convert(ReferenceSystem.XDirection());
  ((CVaxVec*)&buffer[41])->Convert(ReferenceSystem.YDirection());

  *((std::uint16_t*)&buffer[53]) = TextLength;
  memcpy(&buffer[55], (LPCWSTR)m_strText, TextLength);

  file.Write(buffer, static_cast<UINT>(buffer[3] * 32));
}
