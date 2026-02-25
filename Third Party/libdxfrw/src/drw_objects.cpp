#include <cmath>
#include <new>
#include <string>

#include "drw_base.h"
#include "drw_objects.h"
#include "intern/dxfreader.h"

//! Base class for tables entries
/*!
*  Base class for tables entries
*/
void DRW_TableEntry::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 5:
      handle = reader->GetHandleString();
      break;
    case 330:
      parentHandle = reader->GetHandleString();
      break;
    case 2:
      name = reader->GetUtf8String();
      break;
    case 70:
      flags = reader->GetInt32();
      break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
      extData.push_back(new DRW_Variant(code, reader->GetString()));
      break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
      curr = new DRW_Variant(code, DRW_Coord(reader->GetDouble(), 0.0, 0.0));
      extData.push_back(curr);
      break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
      if (curr) curr->setCoordY(reader->GetDouble());
      break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
      if (curr) curr->setCoordZ(reader->GetDouble());
      curr = nullptr;
      break;
    case 1040:
    case 1041:
    case 1042:
      extData.push_back(new DRW_Variant(code, reader->GetDouble()));
      break;
    case 1070:
    case 1071:
      extData.push_back(new DRW_Variant(code, reader->GetInt32()));
      break;
    default:
      break;
  }
}

//! Class to handle dimstyle entries
/*!
*  Class to handle ldim style symbol table entries
*/
void DRW_Dimstyle::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 105:
      handle = reader->GetHandleString();
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
      DRW_TableEntry::parseCode(code, reader);
      break;
  }
}

//! Class to handle line type entries
/*!
*  Class to handle line type symbol table entries
*/
void DRW_LType::parseCode(int code, dxfReader* reader) {
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
      /*    case 74:
            haveShape = reader->GetInt32();
            break;*/
    default:
      DRW_TableEntry::parseCode(code, reader);
      break;
  }
}

//! Update line type
/*!
*  Update the size and length of line type acording to the path
*/
/*TODO: control max length permited */
void DRW_LType::update() {
  double d = 0;
  size = static_cast<int>(path.size());
  for (int i = 0; i < size; i++) { d += fabs(path.at(i)); }
  length = d;
}

//! Class to handle layer entries
/*!
*  Class to handle layer symbol table entries
*/
void DRW_Layer::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 6:
      lineType = reader->GetUtf8String();
      break;
    case 62:
      color = reader->GetInt32();
      break;
    case 290:
      plotF = reader->GetBool();
      break;
    case 370:
      lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->GetInt32());
      break;
    case 390:
      handlePlotS = reader->GetString();
      break;
    case 347:
      handleMaterialS = reader->GetString();
      break;
    case 420:
      color24 = reader->GetInt32();
      break;
    default:
      DRW_TableEntry::parseCode(code, reader);
      break;
  }
}

//! Class to handle text style entries
/*!
*  Class to handle text style symbol table entries
*/
void DRW_Textstyle::parseCode(int code, dxfReader* reader) {
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
      DRW_TableEntry::parseCode(code, reader);
      break;
  }
}

//! Class to handle vport entries
/*!
*  Class to handle vport symbol table entries
*/
void DRW_Vport::parseCode(int code, dxfReader* reader) {
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
      DRW_TableEntry::parseCode(code, reader);
      break;
  }
}

void DRW_ImageDef::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 1:
      name = reader->GetUtf8String();
      break;
    case 5:
      handle = reader->GetHandleString();
      break;
    case 10:
      u = reader->GetDouble();
      break;
    case 20:
      v = reader->GetDouble();
      break;
    case 11:
      up = reader->GetDouble();
      break;
    case 12:
      vp = reader->GetDouble();
      break;
    case 21:
      vp = reader->GetDouble();
      break;
    case 280:
      loaded = reader->GetInt32();
      break;
    case 281:
      resolution = reader->GetInt32();
      break;
    default:
      break;
  }
}
