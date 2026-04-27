#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfLineWeights.h"
#include "EoDxfReader.h"
#include "EoDxfTables.h"

void EoDxfTableEntry::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 5:
      m_handle = reader.GetHandleString();
      break;
    case 330:
      m_ownerHandle = reader.GetHandleString();
      break;
    case 2:
      m_tableName = reader.GetWideString();
      break;
    case 70:
      m_flagValues = reader.GetInt16();
      break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
      m_extensionData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetWideString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013: {
      m_currentVariant = nullptr;
      auto variant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, EoDxfGeometryBase3d(reader.GetDouble(), 0.0, 0.0));
      m_currentVariant = variant.get();
      m_extensionData.push_back(std::move(variant));
      break;
    }
    case 1020:
    case 1021:
    case 1022:
    case 1023:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseY(reader.GetDouble()); }
      break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
      if (m_currentVariant) { m_currentVariant->SetGeometryBaseZ(reader.GetDouble()); }
      m_currentVariant = nullptr;
      break;
    case 1040:
    case 1041:
    case 1042:
      m_extensionData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetDouble()));
      break;
    case 1070:
      m_extensionData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetInt16()));
      break;
    case 1071:
      m_extensionData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(code, reader.GetInt32()));
      break;
    default:
      break;
  }
}

void EoDxfTableEntry::Reset() {
  m_flagValues = 0;
  m_extensionData.clear();
  m_currentVariant = nullptr;
}

void EoDxfBlockRecord::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 70:
      m_blockInsertionUnits = reader.GetInt16();
      m_flagValues = m_blockInsertionUnits;  // Block records only flag: block insertion units
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfBlockRecord::Reset() {
  m_blockInsertionUnits = 0;   // Block records only flag: block insertion units
  EoDxfTableEntry::Reset();
}

void EoDxfDimensionStyle::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 105:
      m_handle = reader.GetHandleString();
      break;
    case 3:
      dimpost = reader.GetWideString();
      break;
    case 4:
      dimapost = reader.GetWideString();
      break;
    case 5:
      dimblk = reader.GetWideString();
      break;
    case 6:
      dimblk1 = reader.GetWideString();
      break;
    case 7:
      dimblk2 = reader.GetWideString();
      break;
    case 40:
      dimscale = reader.GetDouble();
      break;
    case 41:
      dimasz = reader.GetDouble();
      break;
    case 42:
      dimexo = reader.GetDouble();
      break;
    case 43:
      dimdli = reader.GetDouble();
      break;
    case 44:
      dimexe = reader.GetDouble();
      break;
    case 45:
      dimrnd = reader.GetDouble();
      break;
    case 46:
      dimdle = reader.GetDouble();
      break;
    case 47:
      dimtp = reader.GetDouble();
      break;
    case 48:
      dimtm = reader.GetDouble();
      break;
    case 49:
      dimfxl = reader.GetDouble();
      break;
    case 140:
      dimtxt = reader.GetDouble();
      break;
    case 141:
      dimcen = reader.GetDouble();
      break;
    case 142:
      dimtsz = reader.GetDouble();
      break;
    case 143:
      dimaltf = reader.GetDouble();
      break;
    case 144:
      dimlfac = reader.GetDouble();
      break;
    case 145:
      dimtvp = reader.GetDouble();
      break;
    case 146:
      dimtfac = reader.GetDouble();
      break;
    case 147:
      dimgap = reader.GetDouble();
      break;
    case 148:
      dimaltrnd = reader.GetDouble();
      break;
    case 71:
      dimtol = reader.GetInt16();
      break;
    case 72:
      dimlim = reader.GetInt16();
      break;
    case 73:
      dimtih = reader.GetInt16();
      break;
    case 74:
      dimtoh = reader.GetInt16();
      break;
    case 75:
      dimse1 = reader.GetInt16();
      break;
    case 76:
      dimse2 = reader.GetInt16();
      break;
    case 77:
      dimtad = reader.GetInt16();
      break;
    case 78:
      dimzin = reader.GetInt16();
      break;
    case 79:
      dimazin = reader.GetInt16();
      break;
    case 170:
      dimalt = reader.GetInt16();
      break;
    case 171:
      dimaltd = reader.GetInt16();
      break;
    case 172:
      dimtofl = reader.GetInt16();
      break;
    case 173:
      dimsah = reader.GetInt16();
      break;
    case 174:
      dimtix = reader.GetInt16();
      break;
    case 175:
      dimsoxd = reader.GetInt16();
      break;
    case 176:
      dimclrd = reader.GetInt16();
      break;
    case 177:
      dimclre = reader.GetInt16();
      break;
    case 178:
      dimclrt = reader.GetInt16();
      break;
    case 179:
      dimadec = reader.GetInt16();
      break;
    case 270:
      dimunit = reader.GetInt16();
      break;
    case 271:
      dimdec = reader.GetInt16();
      break;
    case 272:
      dimtdec = reader.GetInt16();
      break;
    case 273:
      dimaltu = reader.GetInt16();
      break;
    case 274:
      dimalttd = reader.GetInt16();
      break;
    case 275:
      dimaunit = reader.GetInt16();
      break;
    case 276:
      dimfrac = reader.GetInt16();
      break;
    case 277:
      dimlunit = reader.GetInt16();
      break;
    case 278:
      dimdsep = reader.GetInt16();
      break;
    case 279:
      dimtmove = reader.GetInt16();
      break;
    case 280:
      dimjust = reader.GetInt16();
      break;
    case 281:
      dimsd1 = reader.GetInt16();
      break;
    case 282:
      dimsd2 = reader.GetInt16();
      break;
    case 283:
      dimtolj = reader.GetInt16();
      break;
    case 284:
      dimtzin = reader.GetInt16();
      break;
    case 285:
      dimaltz = reader.GetInt16();
      break;
    case 286:
      dimaltttz = reader.GetInt16();
      break;
    case 287:
      dimfit = reader.GetInt16();
      break;
    case 288:
      dimupt = reader.GetInt16();
      break;
    case 289:
      dimatfit = reader.GetInt16();
      break;
    case 290:
      dimfxlon = reader.GetBool();
      break;
    case 340:
      dimtxsty = reader.GetWideString();
      break;
    case 341:
      dimldrblk = reader.GetWideString();
      break;
    case 342:
      dimblk = reader.GetWideString();
      break;
    case 343:
      dimblk1 = reader.GetWideString();
      break;
    case 344:
      dimblk2 = reader.GetWideString();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfDimensionStyle::Reset() noexcept {
  dimasz = 0.18;
  dimtxt = 0.18;
  dimexe = 0.18;
  dimexo = 0.0625;
  dimgap = dimcen = 0.09;
  dimtxsty = L"Standard";
  dimscale = 1.0;
  dimlfac = 1.0;
  dimtfac = 1.0;
  dimfxl = 1.0;
  dimdli = 0.38;
  dimrnd = 0.0;
  dimdle = 0.0;
  dimtp = 0.0;
  dimtm = 0.0;
  dimtsz = 0.0;
  dimtvp = 0.0;
  dimaltf = 25.4;
  dimtol = 0;
  dimlim = 0;
  dimse1 = 0;
  dimse2 = 0;
  dimtad = 0;
  dimzin = 0;
  dimtoh = 1;
  dimtolj = 1;
  dimalt = 0;
  dimtofl = 0;
  dimsah = 0;
  dimtix = 0;
  dimsoxd = 0;
  dimfxlon = false;
  dimaltd = 2;
  dimunit = 2;
  dimaltu = 2;
  dimalttd = 2;
  dimlunit = 2;
  dimclrd = 0;
  dimclre = 0;
  dimclrt = 0;
  dimjust = 0;
  dimupt = 0;
  dimazin = 0;
  dimaltz = 0;
  dimaltttz = 0;
  dimtzin = 0;
  dimfrac = 0;
  dimtih = 0;
  dimadec = 0;
  dimaunit = 0;
  dimsd1 = 0;
  dimsd2 = 0;
  dimtmove = 0;
  dimaltrnd = 0.0;
  dimdec = 4;
  dimtdec = 4;
  dimfit = 3;
  dimatfit = 3;
  dimdsep = '.';
  dimlwd = -2;
  dimlwe = -2;
  EoDxfTableEntry::Reset();
}

void EoDxfLinetype::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 3:
      desc = reader.GetWideString();
      break;
    case 73:
      m_numberOfLinetypeElements = reader.GetInt16();
      path.reserve(m_numberOfLinetypeElements);
      break;
    case 40:
      length = reader.GetDouble();
      break;
    case 49:
      path.push_back(reader.GetDouble());
      pathIdx++;
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfLinetype::Reset() {
  desc = L"";
  m_numberOfLinetypeElements = 0;
  length = 0.0;
  path.clear();
  pathIdx = 0;
  EoDxfTableEntry::Reset();
}

void EoDxfLinetype::Update() {
  double d{};
  m_numberOfLinetypeElements = static_cast<std::int16_t>(path.size());
  for (int i = 0; i < m_numberOfLinetypeElements; i++) { d += fabs(path.at(i)); }
  length = d;
}

void EoDxfLayer::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 6:
      m_linetypeName = reader.GetWideString();
      break;
    case 62:
      m_colorNumber = reader.GetInt16();
      break;
    case 290:
      m_plottingFlag = reader.GetBool();
      break;
    case 370:
      m_lineweightEnumValue = EoDxfLineWeights::DxfIndexToLineWeight(reader.GetInt16());
      break;
    case 390:
      m_handleOfPlotStyleName = reader.GetWideString();
      break;
    case 347:
      m_handleOfMaterialStyleName = reader.GetWideString();
      break;
    case 420:
      color24 = reader.GetInt32();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfLayer::Reset() {
  m_linetypeName = L"CONTINUOUS";
  m_colorNumber = 7;
  m_plottingFlag = true;
  m_lineweightEnumValue = EoDxfLineWeights::LineWeight::kLnWtByLwDefault;
  color24 = -1;
  EoDxfTableEntry::Reset();
}


void EoDxfTextStyle::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 3:
      font = reader.GetWideString();
      break;
    case 4:
      bigFont = reader.GetWideString();
      break;
    case 40:
      height = reader.GetDouble();
      break;
    case 41:
      width = reader.GetDouble();
      break;
    case 50:
      oblique = reader.GetDouble();
      break;
    case 42:
      lastHeight = reader.GetDouble();
      break;
    case 71:
      m_textGenerationFlag = reader.GetInt16();
      break;
    case 1071:
      fontFamily = reader.GetInt32();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfVPort::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_lowerLeftCorner.x = reader.GetDouble();
      break;
    case 20:
      m_lowerLeftCorner.y = reader.GetDouble();
      break;
    case 11:
      m_upperRightCorner.x = reader.GetDouble();
      break;
    case 21:
      m_upperRightCorner.y = reader.GetDouble();
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
    case 26:
      m_viewDirection.y = reader.GetDouble();
      break;
    case 36:
      m_viewDirection.z = reader.GetDouble();
      break;
    case 17:
      m_viewTargetPoint.x = reader.GetDouble();
      break;
    case 27:
      m_viewTargetPoint.y = reader.GetDouble();
      break;
    case 37:
      m_viewTargetPoint.z = reader.GetDouble();
      break;
    case 40:
      m_viewHeight = reader.GetDouble();
      break;
    case 41:
      m_viewAspectRatio = reader.GetDouble();
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
    case 50:
      m_snapRotationAngle = reader.GetDouble();
      break;
    case 51:
      m_viewTwistAngle = reader.GetDouble();
      break;
    case 60:
      m_gridBehavior = reader.GetInt16();
      break;
    case 71:
      m_viewMode = reader.GetInt16();
      break;
    case 72:
      m_circleZoomPercent = reader.GetInt16();
      break;
    case 73:
      m_fastZoom = reader.GetInt16();
      break;
    case 74:
      m_ucsIcon = reader.GetInt16();
      break;
    case 75:
      m_snapOn = reader.GetInt16();
      break;
    case 76:
      m_gridOn = reader.GetInt16();
      break;
    case 77:
      m_snapStyle = reader.GetInt16();
      break;
    case 78:
      m_snapIsopair = reader.GetInt16();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfVPort::Reset() {
  m_lowerLeftCorner = {};
  m_upperRightCorner = {1.0, 1.0};
  m_viewCenter = {0.651828, -0.16};
  m_snapBasePoint = {};
  m_snapSpacing = {10.0, 10.0};
  m_gridSpacing = {10.0, 10.0};
  m_viewDirection = {0.0, 0.0, 1.0};
  m_viewTargetPoint = {};
  m_viewHeight = 5.13732;
  m_viewAspectRatio = 2.4426877;
  m_lensLength = 50.0;
  m_frontClipPlane = 0.0;
  m_backClipPlane = 0.0;
  m_snapRotationAngle = 0.0;
  m_viewTwistAngle = 0.0;
  m_viewMode = 0;
  m_circleZoomPercent = 100;
  m_fastZoom = 1;
  m_ucsIcon = 3;
  m_snapOn = 0;
  m_gridOn = 0;
  m_snapStyle = 0;
  m_snapIsopair = 0;
  m_gridBehavior = 7;
  EoDxfTableEntry::Reset();
}
