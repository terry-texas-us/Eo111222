#include <cmath>
#include <new>
#include <string>

#include "drw_base.h"
#include "drw_tables.h"
#include "intern/dxfreader.h"

void EoDxfTableEntry::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 5:
      m_handle = reader->GetHandleString();
      break;
    case 330:
      m_ownerHandle = reader->GetHandleString();
      break;
    case 2:
      m_tableName = reader->GetUtf8String();
      break;
    case 70:
      m_flagValues = reader->GetInt32();
      break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
      m_currentVariant = new EoDxfGroupCodeValuesVariant(code, EoDxfGeometryBase3d(reader->GetDouble(), 0.0, 0.0));
      m_extensionData.push_back(m_currentVariant);
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
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetDouble()));
      break;
    case 1070:
    case 1071:
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(code, reader->GetInt32()));
      break;
    default:
      break;
  }
}

void EoDxfTableEntry::Reset() {
  m_flagValues = 0;
  for (auto* variant : m_extensionData) { delete variant; }
  m_extensionData.clear();
  m_currentVariant = nullptr;
}

void EoDxfBlockRecord::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 70:
      m_blockInsertionUnits = reader->GetInt32();
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

void EoDxfDimensionStyle::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 105:
      m_handle = reader->GetHandleString();
      break;
    case 3:
      dimpost = reader->GetUtf8String();
      break;
    case 4:
      dimapost = reader->GetUtf8String();
      break;
    case 5:
      dimblk = reader->GetUtf8String();
      break;
    case 6:
      dimblk1 = reader->GetUtf8String();
      break;
    case 7:
      dimblk2 = reader->GetUtf8String();
      break;
    case 40:
      dimscale = reader->GetDouble();
      break;
    case 41:
      dimasz = reader->GetDouble();
      break;
    case 42:
      dimexo = reader->GetDouble();
      break;
    case 43:
      dimdli = reader->GetDouble();
      break;
    case 44:
      dimexe = reader->GetDouble();
      break;
    case 45:
      dimrnd = reader->GetDouble();
      break;
    case 46:
      dimdle = reader->GetDouble();
      break;
    case 47:
      dimtp = reader->GetDouble();
      break;
    case 48:
      dimtm = reader->GetDouble();
      break;
    case 49:
      dimfxl = reader->GetDouble();
      break;
    case 140:
      dimtxt = reader->GetDouble();
      break;
    case 141:
      dimcen = reader->GetDouble();
      break;
    case 142:
      dimtsz = reader->GetDouble();
      break;
    case 143:
      dimaltf = reader->GetDouble();
      break;
    case 144:
      dimlfac = reader->GetDouble();
      break;
    case 145:
      dimtvp = reader->GetDouble();
      break;
    case 146:
      dimtfac = reader->GetDouble();
      break;
    case 147:
      dimgap = reader->GetDouble();
      break;
    case 148:
      dimaltrnd = reader->GetDouble();
      break;
    case 71:
      dimtol = reader->GetInt32();
      break;
    case 72:
      dimlim = reader->GetInt32();
      break;
    case 73:
      dimtih = reader->GetInt32();
      break;
    case 74:
      dimtoh = reader->GetInt32();
      break;
    case 75:
      dimse1 = reader->GetInt32();
      break;
    case 76:
      dimse2 = reader->GetInt32();
      break;
    case 77:
      dimtad = reader->GetInt32();
      break;
    case 78:
      dimzin = reader->GetInt32();
      break;
    case 79:
      dimazin = reader->GetInt32();
      break;
    case 170:
      dimalt = reader->GetInt32();
      break;
    case 171:
      dimaltd = reader->GetInt32();
      break;
    case 172:
      dimtofl = reader->GetInt32();
      break;
    case 173:
      dimsah = reader->GetInt32();
      break;
    case 174:
      dimtix = reader->GetInt32();
      break;
    case 175:
      dimsoxd = reader->GetInt32();
      break;
    case 176:
      dimclrd = reader->GetInt32();
      break;
    case 177:
      dimclre = reader->GetInt32();
      break;
    case 178:
      dimclrt = reader->GetInt32();
      break;
    case 179:
      dimadec = reader->GetInt32();
      break;
    case 270:
      dimunit = reader->GetInt32();
      break;
    case 271:
      dimdec = reader->GetInt32();
      break;
    case 272:
      dimtdec = reader->GetInt32();
      break;
    case 273:
      dimaltu = reader->GetInt32();
      break;
    case 274:
      dimalttd = reader->GetInt32();
      break;
    case 275:
      dimaunit = reader->GetInt32();
      break;
    case 276:
      dimfrac = reader->GetInt32();
      break;
    case 277:
      dimlunit = reader->GetInt32();
      break;
    case 278:
      dimdsep = reader->GetInt32();
      break;
    case 279:
      dimtmove = reader->GetInt32();
      break;
    case 280:
      dimjust = reader->GetInt32();
      break;
    case 281:
      dimsd1 = reader->GetInt32();
      break;
    case 282:
      dimsd2 = reader->GetInt32();
      break;
    case 283:
      dimtolj = reader->GetInt32();
      break;
    case 284:
      dimtzin = reader->GetInt32();
      break;
    case 285:
      dimaltz = reader->GetInt32();
      break;
    case 286:
      dimaltttz = reader->GetInt32();
      break;
    case 287:
      dimfit = reader->GetInt32();
      break;
    case 288:
      dimupt = reader->GetInt32();
      break;
    case 289:
      dimatfit = reader->GetInt32();
      break;
    case 290:
      dimfxlon = reader->GetInt32();
      break;
    case 340:
      dimtxsty = reader->GetUtf8String();
      break;
    case 341:
      dimldrblk = reader->GetUtf8String();
      break;
    case 342:
      dimblk = reader->GetUtf8String();
      break;
    case 343:
      dimblk1 = reader->GetUtf8String();
      break;
    case 344:
      dimblk2 = reader->GetUtf8String();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfDimensionStyle::Reset() {
  dimasz = 0.18;
  dimtxt = 0.18;
  dimexe = 0.18;
  dimexo = 0.0625;
  dimgap = dimcen = 0.09;
  dimtxsty = "Standard";
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
  dimfxlon = 0;
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

void EoDxfLinetype::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 3:
      desc = reader->GetUtf8String();
      break;
    case 73:
      size = reader->GetInt32();
      path.reserve(size);
      break;
    case 40:
      length = reader->GetDouble();
      break;
    case 49:
      path.push_back(reader->GetDouble());
      pathIdx++;
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfLinetype::Reset() {
  desc = "";
  size = 0;
  length = 0.0;
  pathIdx = 0;
  EoDxfTableEntry::Reset();
}

void EoDxfLinetype::Update() {
  double d = 0;
  size = static_cast<int>(path.size());
  for (int i = 0; i < size; i++) { d += fabs(path.at(i)); }
  length = d;
}

void EoDxfLayer::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 6:
      m_linetypeName = reader->GetUtf8String();
      break;
    case 62:
      m_colorNumber = reader->GetInt32();
      break;
    case 290:
      m_plottingFlag = reader->GetBool();
      break;
    case 370:
      m_lineweightEnumValue = DRW_LW_Conv::dxfInt2lineWidth(reader->GetInt32());
      break;
    case 390:
      m_handleOfPlotStyleName = reader->GetString();
      break;
    case 347:
      m_handleOfMaterialStyleName = reader->GetString();
      break;
    case 420:
      color24 = reader->GetInt32();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfLayer::Reset() {
  m_linetypeName = "CONTINUOUS";
  m_colorNumber = 7;
  m_plottingFlag = true;
  m_lineweightEnumValue = DRW_LW_Conv::widthDefault;
  color24 = -1;
  EoDxfTableEntry::Reset();
}


void EoDxfTextStyle::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 3:
      font = reader->GetUtf8String();
      break;
    case 4:
      bigFont = reader->GetUtf8String();
      break;
    case 40:
      height = reader->GetDouble();
      break;
    case 41:
      width = reader->GetDouble();
      break;
    case 50:
      oblique = reader->GetDouble();
      break;
    case 42:
      lastHeight = reader->GetDouble();
      break;
    case 71:
      genFlag = reader->GetInt32();
      break;
    case 1071:
      fontFamily = reader->GetInt32();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfViewport::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 10:
      lowerLeft.x = reader->GetDouble();
      break;
    case 20:
      lowerLeft.y = reader->GetDouble();
      break;
    case 11:
      upperRight.x = reader->GetDouble();
      break;
    case 21:
      upperRight.y = reader->GetDouble();
      break;
    case 12:
      center.x = reader->GetDouble();
      break;
    case 22:
      center.y = reader->GetDouble();
      break;
    case 13:
      snapBase.x = reader->GetDouble();
      break;
    case 23:
      snapBase.y = reader->GetDouble();
      break;
    case 14:
      snapSpacing.x = reader->GetDouble();
      break;
    case 24:
      snapSpacing.y = reader->GetDouble();
      break;
    case 15:
      gridSpacing.x = reader->GetDouble();
      break;
    case 25:
      gridSpacing.y = reader->GetDouble();
      break;
    case 16:
      viewDir.x = reader->GetDouble();
      break;
    case 26:
      viewDir.y = reader->GetDouble();
      break;
    case 36:
      viewDir.z = reader->GetDouble();
      break;
    case 17:
      viewTarget.x = reader->GetDouble();
      break;
    case 27:
      viewTarget.y = reader->GetDouble();
      break;
    case 37:
      viewTarget.z = reader->GetDouble();
      break;
    case 40:
      height = reader->GetDouble();
      break;
    case 41:
      ratio = reader->GetDouble();
      break;
    case 42:
      lensHeight = reader->GetDouble();
      break;
    case 43:
      frontClip = reader->GetDouble();
      break;
    case 44:
      backClip = reader->GetDouble();
      break;
    case 50:
      snapAngle = reader->GetDouble();
      break;
    case 51:
      twistAngle = reader->GetDouble();
      break;
    case 71:
      viewMode = reader->GetInt32();
      break;
    case 72:
      circleZoom = reader->GetInt32();
      break;
    case 73:
      fastZoom = reader->GetInt32();
      break;
    case 74:
      ucsIcon = reader->GetInt32();
      break;
    case 75:
      snap = reader->GetInt32();
      break;
    case 76:
      grid = reader->GetInt32();
      break;
    case 77:
      snapStyle = reader->GetInt32();
      break;
    case 78:
      snapIsopair = reader->GetInt32();
      break;
    default:
      EoDxfTableEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfViewport::Reset() {
  upperRight.x = 1.0;
  upperRight.y = 1.0;
  snapSpacing.x = 10.0;
  snapSpacing.y = 10.0;
  gridSpacing = snapSpacing;
  center.x = 0.651828;
  center.y = -0.16;
  viewDir.z = 1;
  height = 5.13732;
  ratio = 2.4426877;
  lensHeight = 50;
  frontClip = 0.0;
  backClip = 0.0;
  snapAngle = 0.0;
  twistAngle = 0.0;
  viewMode = 0;
  snap = 0;
  grid = 0;
  snapStyle = 0;
  snapIsopair = 0;
  fastZoom = 1;
  circleZoom = 100;
  ucsIcon = 3;
  gridBehavior = 7;
  EoDxfTableEntry::Reset();
}
