#include <map>
#include <string>

#include "drw_base.h"
#include "drw_header.h"
#include "intern/drw_dbg.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

DRW_Header::DRW_Header() { m_version = DRW::Version::AC1021; }

void DRW_Header::addComment(std::string c) {
  if (!m_comments.empty()) { m_comments += '\n'; }
  m_comments += c;
}

/** @brief DRW_Header::parseCode
 *  @param code DXF group code
 *  @param reader Pointer to dxfReader object
 *
 *  Parses the given DXF group code and stores the data in the header variables.
 */
void DRW_Header::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 9:
      curr = new DRW_Variant();
      m_name = reader->GetString();
      if (m_version < DRW::Version::AC1015 && m_name == "$DIMUNIT") { m_name = "$DIMLUNIT"; }
      vars[m_name] = curr;
      break;
    case 1:
      curr->addString(code, reader->GetUtf8String());
      if (m_name == "$ACADVER") {
        reader->SetVersion(curr->content.s, true);
        m_version = reader->GetVersion();
      }
      break;
    case 2:
      curr->addString(code, reader->GetUtf8String());
      break;
    case 3:
      curr->addString(code, reader->GetUtf8String());
      if (m_name == "$DWGCODEPAGE") {
        reader->SetCodePage(curr->content.s);
        curr->addString(code, reader->GetCodePage());
      }
      break;
    case 6:
      curr->addString(code, reader->GetUtf8String());
      break;
    case 7:
      curr->addString(code, reader->GetUtf8String());
      break;
    case 8:
      curr->addString(code, reader->GetUtf8String());
      break;
    case 10:
      curr->addCoord(code, DRW_Coord(reader->GetDouble(), 0.0, 0.0));
      break;
    case 20:
      curr->setCoordY(reader->GetDouble());
      break;
    case 30:
      curr->setCoordZ(reader->GetDouble());
      break;
    case 40:
      curr->addDouble(code, reader->GetDouble());
      break;
    case 50:
      curr->addDouble(code, reader->GetDouble());
      break;
    case 62:
      curr->addInt(code, reader->GetInt32());
      break;
    case 70:
      curr->addInt(code, reader->GetInt32());
      break;
    case 280:
      curr->addInt(code, reader->GetInt32());
      break;
    case 290:
      curr->addInt(code, reader->GetInt32());
      break;
    case 370:
      curr->addInt(code, reader->GetInt32());
      break;
    case 380:
      curr->addInt(code, reader->GetInt32());
      break;
    case 390:
      curr->addString(code, reader->GetUtf8String());
      break;
    default:
      break;
  }
}

void DRW_Header::write(dxfWriter* writer, DRW::Version version) {
  double varDouble;
  int varInt;
  std::string varStr;
  DRW_Coord varCoord;
  writer->WriteString(2, "HEADER");
  writer->WriteString(9, "$ACADVER");
  switch (version) {
    case DRW::Version::AC1006:  // R10 (not supported) [1988]
    case DRW::Version::AC1009:  // R11 [1990] & R12 [1992]
      varStr = "AC1009";
      break;
    case DRW::Version::AC1012:  // R13 (not supported)
    case DRW::Version::AC1014:  // R14
      varStr = "AC1014";
      break;
    case DRW::Version::AC1015:  // AutoCAD 2000 / 2000i / 2002
      varStr = "AC1015";
      break;
    case DRW::Version::AC1018:  // AutoCAD 2004 / 2005 / 2006
      varStr = "AC1018";
      break;
    case DRW::Version::AC1024:  // AutoCAD 2010 / 2011 / 2012
      varStr = "AC1024";
      break;
    case DRW::Version::AC1027:  // AutoCAD 2013 / 2014 / 2015 / 2016 / 2017
      varStr = "AC1027";
      break;
    case DRW::Version::
        AC1032:  // AutoCAD 2018 / 2019 / 2020 / 2021 / 2022 / 2023 / 2024 / 2025 / 2026 (current format – no new code since 2018)
      varStr = "AC1032";
      break;
    case DRW::Version::AC1021:  // AutoCAD 2007 / 2008 / 2009
      [[fallthrough]];          // intentional fallthrough to default case
    default:
      varStr = "AC1021";
      break;
  }
  writer->WriteString(1, varStr);
  writer->SetVersion(&varStr, true);

  (void)getStr("$ACADVER", &varStr);
  (void)getStr("$ACADMAINTVER", &varStr);

  if (!getStr("$DWGCODEPAGE", &varStr)) { varStr = "ANSI_1252"; }
  writer->WriteString(9, "$DWGCODEPAGE");
  writer->SetCodePage(&varStr);
  writer->WriteString(3, writer->GetCodePage());
  writer->WriteString(9, "$INSBASE");
  if (getCoord("$INSBASE", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }
  writer->WriteString(9, "$EXTMIN");
  if (getCoord("$EXTMIN", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 1.0000000000000000E+020);
    writer->WriteDouble(20, 1.0000000000000000E+020);
    writer->WriteDouble(30, 1.0000000000000000E+020);
  }
  writer->WriteString(9, "$EXTMAX");
  if (getCoord("$EXTMAX", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, -1.0000000000000000E+020);
    writer->WriteDouble(20, -1.0000000000000000E+020);
    writer->WriteDouble(30, -1.0000000000000000E+020);
  }
  writer->WriteString(9, "$LIMMIN");
  if (getCoord("$LIMMIN", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }
  writer->WriteString(9, "$LIMMAX");
  if (getCoord("$LIMMAX", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
  } else {
    writer->WriteDouble(10, 420.0);
    writer->WriteDouble(20, 297.0);
  }
  writer->WriteString(9, "$ORTHOMODE");
  if (getInt("$ORTHOMODE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$REGENMODE");
  if (getInt("$REGENMODE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$FILLMODE");
  if (getInt("$FILLMODE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$QTEXTMODE");
  if (getInt("$QTEXTMODE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$MIRRTEXT");
  if (getInt("$MIRRTEXT", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  if (version == DRW::Version::AC1009) {
    writer->WriteString(9, "$DRAGMODE");
    if (getInt("$DRAGMODE", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 2);
  }
  writer->WriteString(9, "$LTSCALE");
  if (GetDouble("$LTSCALE", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 1.0);
  if (version == DRW::Version::AC1009) {
    writer->WriteString(9, "$OSMODE");
    if (getInt("$OSMODE", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
  }
  writer->WriteString(9, "$ATTMODE");
  if (getInt("$ATTMODE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$TEXTSIZE");
  if (GetDouble("$TEXTSIZE", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 2.5);
  writer->WriteString(9, "$TRACEWID");
  if (GetDouble("$TRACEWID", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 15.68);
  writer->WriteString(9, "$TEXTSTYLE");
  if (getStr("$TEXTSTYLE", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(7, varStr);
    else
      writer->WriteUtf8String(7, varStr);
  else
    writer->WriteString(7, "STANDARD");
  writer->WriteString(9, "$CLAYER");
  if (getStr("$CLAYER", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(8, varStr);
    else
      writer->WriteUtf8String(8, varStr);
  else
    writer->WriteString(8, "0");
  writer->WriteString(9, "$CELTYPE");
  if (getStr("$CELTYPE", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(6, varStr);
    else
      writer->WriteUtf8String(6, varStr);
  else
    writer->WriteString(6, "BYLAYER");
  writer->WriteString(9, "$CECOLOR");
  if (getInt("$CECOLOR", &varInt))
    writer->WriteInt16(62, varInt);
  else
    writer->WriteInt16(62, 256);
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$CELTSCALE");
    if (GetDouble("$CELTSCALE", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 1.0);
    writer->WriteString(9, "$DISPSILH");
    if (getInt("$DISPSILH", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
  }

  writer->WriteString(9, "$DIMSCALE");
  if (GetDouble("$DIMSCALE", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 2.5);
  writer->WriteString(9, "$DIMASZ");
  if (GetDouble("$DIMASZ", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 2.5);
  writer->WriteString(9, "$DIMEXO");
  if (GetDouble("$DIMEXO", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.625);
  writer->WriteString(9, "$DIMDLI");
  if (GetDouble("$DIMDLI", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 3.75);
  writer->WriteString(9, "$DIMRND");
  if (GetDouble("$DIMRND", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$DIMDLE");
  if (GetDouble("$DIMDLE", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$DIMEXE");
  if (GetDouble("$DIMEXE", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 1.25);
  writer->WriteString(9, "$DIMTP");
  if (GetDouble("$DIMTP", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$DIMTM");
  if (GetDouble("$DIMTM", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$DIMTXT");
  if (GetDouble("$DIMTXT", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 2.5);
  writer->WriteString(9, "$DIMCEN");
  if (GetDouble("$DIMCEN", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 2.5);
  writer->WriteString(9, "$DIMTSZ");
  if (GetDouble("$DIMTSZ", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$DIMTOL");
  if (getInt("$DIMTOL", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMLIM");
  if (getInt("$DIMLIM", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMTIH");
  if (getInt("$DIMTIH", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMTOH");
  if (getInt("$DIMTOH", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMSE1");
  if (getInt("$DIMSE1", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMSE2");
  if (getInt("$DIMSE2", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMTAD");
  if (getInt("$DIMTAD", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$DIMZIN");
  if (getInt("$DIMZIN", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 8);
  writer->WriteString(9, "$DIMBLK");
  if (getStr("$DIMBLK", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(1, varStr);
    else
      writer->WriteUtf8String(1, varStr);
  else
    writer->WriteString(1, "");
  writer->WriteString(9, "$DIMASO");
  if (getInt("$DIMASO", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$DIMSHO");
  if (getInt("$DIMSHO", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$DIMPOST");
  if (getStr("$DIMPOST", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(1, varStr);
    else
      writer->WriteUtf8String(1, varStr);
  else
    writer->WriteString(1, "");
  writer->WriteString(9, "$DIMAPOST");
  if (getStr("$DIMAPOST", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(1, varStr);
    else
      writer->WriteUtf8String(1, varStr);
  else
    writer->WriteString(1, "");
  writer->WriteString(9, "$DIMALT");
  if (getInt("$DIMALT", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMALTD");
  if (getInt("$DIMALTD", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 3);
  writer->WriteString(9, "$DIMALTF");
  if (GetDouble("$DIMALTF", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.03937);
  writer->WriteString(9, "$DIMLFAC");
  if (GetDouble("$DIMLFAC", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 1.0);
  writer->WriteString(9, "$DIMTOFL");
  if (getInt("$DIMTOFL", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$DIMTVP");
  if (GetDouble("$DIMTVP", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$DIMTIX");
  if (getInt("$DIMTIX", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMSOXD");
  if (getInt("$DIMSOXD", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMSAH");
  if (getInt("$DIMSAH", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMBLK1");
  if (getStr("$DIMBLK1", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(1, varStr);
    else
      writer->WriteUtf8String(1, varStr);
  else
    writer->WriteString(1, "");
  writer->WriteString(9, "$DIMBLK2");
  if (getStr("$DIMBLK2", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(1, varStr);
    else
      writer->WriteUtf8String(1, varStr);
  else
    writer->WriteString(1, "");
  writer->WriteString(9, "$DIMSTYLE");
  if (getStr("$DIMSTYLE", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(2, varStr);
    else
      writer->WriteUtf8String(2, varStr);
  else
    writer->WriteString(2, "STANDARD");
  writer->WriteString(9, "$DIMCLRD");
  if (getInt("$DIMCLRD", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMCLRE");
  if (getInt("$DIMCLRE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMCLRT");
  if (getInt("$DIMCLRT", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$DIMTFAC");
  if (GetDouble("$DIMTFAC", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 1.0);
  writer->WriteString(9, "$DIMGAP");
  if (GetDouble("$DIMGAP", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.625);
  //post r12 dim vars
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$DIMJUST");
    if (getInt("$DIMJUST", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMSD1");
    if (getInt("$DIMSD1", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMSD2");
    if (getInt("$DIMSD2", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMTOLJ");
    if (getInt("$DIMTOLJ", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMTZIN");
    if (getInt("$DIMTZIN", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 8);
    writer->WriteString(9, "$DIMALTZ");
    if (getInt("$DIMALTZ", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMALTTZ");
    if (getInt("$DIMALTTZ", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMUPT");
    if (getInt("$DIMUPT", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMDEC");
    if (getInt("$DIMDEC", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 2);
    writer->WriteString(9, "$DIMTDEC");
    if (getInt("$DIMTDEC", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 2);
    writer->WriteString(9, "$DIMALTU");
    if (getInt("$DIMALTU", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 2);
    writer->WriteString(9, "$DIMALTTD");
    if (getInt("$DIMALTTD", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 3);
    writer->WriteString(9, "$DIMTXSTY");
    if (getStr("$DIMTXSTY", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(7, varStr);
      else
        writer->WriteUtf8String(7, varStr);
    else
      writer->WriteString(7, "STANDARD");
    writer->WriteString(9, "$DIMAUNIT");
    if (getInt("$DIMAUNIT", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMADEC");
    if (getInt("$DIMADEC", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMALTRND");
    if (GetDouble("$DIMALTRND", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
    writer->WriteString(9, "$DIMAZIN");
    if (getInt("$DIMAZIN", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMDSEP");
    if (getInt("$DIMDSEP", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 44);
    writer->WriteString(9, "$DIMATFIT");
    if (getInt("$DIMATFIT", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 3);
    writer->WriteString(9, "$DIMFRAC");
    if (getInt("$DIMFRAC", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$DIMLDRBLK");
    if (getStr("$DIMLDRBLK", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(1, varStr);
      else
        writer->WriteUtf8String(1, varStr);
    else
      writer->WriteString(1, "STANDARD");
    //verify if exist "$DIMLUNIT" or obsolete "$DIMUNIT" (pre v2000)
    if (!getInt("$DIMLUNIT", &varInt)) {
      if (!getInt("$DIMUNIT", &varInt)) varInt = 2;
    }
    //verify valid values from 1 to 6
    if (varInt < 1 || varInt > 6) varInt = 2;
    if (version > DRW::Version::AC1014) {
      writer->WriteString(9, "$DIMLUNIT");
      writer->WriteInt16(70, varInt);
    } else {
      writer->WriteString(9, "$DIMUNIT");
      writer->WriteInt16(70, varInt);
    }
    writer->WriteString(9, "$DIMLWD");
    if (getInt("$DIMLWD", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, -2);
    writer->WriteString(9, "$DIMLWE");
    if (getInt("$DIMLWE", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, -2);
    writer->WriteString(9, "$DIMTMOVE");
    if (getInt("$DIMTMOVE", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);

    if (version > DRW::Version::AC1018) {
      writer->WriteString(9, "$DIMFXL");
      if (GetDouble("$DIMFXL", &varDouble))
        writer->WriteDouble(40, varDouble);
      else
        writer->WriteDouble(40, 1.0);
      writer->WriteString(9, "$DIMFXLON");
      if (getInt("$DIMFXLON", &varInt))
        writer->WriteInt16(70, varInt);
      else
        writer->WriteInt16(70, 0);
      writer->WriteString(9, "$DIMJOGANG");
      if (GetDouble("$DIMJOGANG", &varDouble))
        writer->WriteDouble(40, varDouble);
      else
        writer->WriteDouble(40, 0.7854);
      writer->WriteString(9, "$DIMTFILL");
      if (getInt("$DIMTFILL", &varInt))
        writer->WriteInt16(70, varInt);
      else
        writer->WriteInt16(70, 0);
      writer->WriteString(9, "$DIMTFILLCLR");
      if (getInt("$DIMTFILLCLR", &varInt))
        writer->WriteInt16(70, varInt);
      else
        writer->WriteInt16(70, 0);
      writer->WriteString(9, "$DIMARCSYM");
      if (getInt("$DIMARCSYM", &varInt))
        writer->WriteInt16(70, varInt);
      else
        writer->WriteInt16(70, 0);
      writer->WriteString(9, "$DIMLTYPE");
      if (getStr("$DIMLTYPE", &varStr))
        if (version == DRW::Version::AC1009)
          writer->WriteUtf8Caps(6, varStr);
        else
          writer->WriteUtf8String(6, varStr);
      else
        writer->WriteString(6, "");
      writer->WriteString(9, "$DIMLTEX1");
      if (getStr("$DIMLTEX1", &varStr))
        if (version == DRW::Version::AC1009)
          writer->WriteUtf8Caps(6, varStr);
        else
          writer->WriteUtf8String(6, varStr);
      else
        writer->WriteString(6, "");
      writer->WriteString(9, "$DIMLTEX2");
      if (getStr("$DIMLTEX2", &varStr))
        if (version == DRW::Version::AC1009)
          writer->WriteUtf8Caps(6, varStr);
        else
          writer->WriteUtf8String(6, varStr);
      else
        writer->WriteString(6, "");
      if (version > DRW::Version::AC1021) {
        writer->WriteString(9, "$DIMTXTDIRECTION");
        if (getInt("$DIMTXTDIRECTION", &varInt))
          writer->WriteInt16(70, varInt);
        else
          writer->WriteInt16(70, 0);
      }
    }  // end post v2004 dim vars
  }  //end post r12 dim vars

  writer->WriteString(9, "$LUNITS");
  if (getInt("$LUNITS", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 2);
  writer->WriteString(9, "$LUPREC");
  if (getInt("$LUPREC", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 4);
  writer->WriteString(9, "$SKETCHINC");
  if (GetDouble("$SKETCHINC", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 1.0);
  writer->WriteString(9, "$FILLETRAD");
  if (GetDouble("$FILLETRAD", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$AUNITS");
  if (getInt("$AUNITS", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$AUPREC");
  if (getInt("$AUPREC", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 2);
  writer->WriteString(9, "$MENU");
  if (getStr("$MENU", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(1, varStr);
    else
      writer->WriteUtf8String(1, varStr);
  else
    writer->WriteString(1, ".");
  writer->WriteString(9, "$ELEVATION");
  if (GetDouble("$ELEVATION", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$PELEVATION");
  if (GetDouble("$PELEVATION", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$THICKNESS");
  if (GetDouble("$THICKNESS", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$LIMCHECK");
  if (getInt("$LIMCHECK", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  if (version < DRW::Version::AC1015) {
    writer->WriteString(9, "$BLIPMODE");
    if (getInt("$BLIPMODE", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
  }
  writer->WriteString(9, "$CHAMFERA");
  if (GetDouble("$CHAMFERA", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$CHAMFERB");
  if (GetDouble("$CHAMFERB", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$CHAMFERC");
    if (GetDouble("$CHAMFERC", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
    writer->WriteString(9, "$CHAMFERD");
    if (GetDouble("$CHAMFERD", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
  }
  writer->WriteString(9, "$SKPOLY");
  if (getInt("$SKPOLY", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 0);
  //rlz: todo, times
  writer->WriteString(9, "$USRTIMER");
  if (getInt("$USRTIMER", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$ANGBASE");
  if (GetDouble("$ANGBASE", &varDouble))
    writer->WriteDouble(50, varDouble);
  else
    writer->WriteDouble(50, 0.0);
  writer->WriteString(9, "$ANGDIR");
  if (getInt("$ANGDIR", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$PDMODE");
  if (getInt("$PDMODE", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 34);
  writer->WriteString(9, "$PDSIZE");
  if (GetDouble("$PDSIZE", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$PLINEWID");
  if (GetDouble("$PLINEWID", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  if (version < DRW::Version::AC1012) {
    writer->WriteString(9, "$COORDS");
    if (getInt("$COORDS", &varInt)) {
      writer->WriteInt16(70, varInt);
    } else
      writer->WriteInt16(70, 2);
  }
  writer->WriteString(9, "$SPLFRAME");
  if (getInt("$SPLFRAME", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$SPLINETYPE");
  if (getInt("$SPLINETYPE", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 2);
  writer->WriteString(9, "$SPLINESEGS");
  if (getInt("$SPLINESEGS", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 8);
  if (version < DRW::Version::AC1012) {
    writer->WriteString(9, "$ATTDIA");
    if (getInt("$ATTDIA", &varInt)) {
      writer->WriteInt16(70, varInt);
    } else
      writer->WriteInt16(70, 1);
    writer->WriteString(9, "$ATTREQ");
    if (getInt("$ATTREQ", &varInt)) {
      writer->WriteInt16(70, varInt);
    } else
      writer->WriteInt16(70, 1);
    writer->WriteString(9, "$HANDLING");
    if (getInt("$HANDLING", &varInt)) {
      writer->WriteInt16(70, varInt);
    } else
      writer->WriteInt16(70, 1);
  }
  writer->WriteString(9, "$HANDSEED");
  writer->WriteString(5, "20000");
  writer->WriteString(9, "$SURFTAB1");
  if (getInt("$SURFTAB1", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 6);
  writer->WriteString(9, "$SURFTAB2");
  if (getInt("$SURFTAB2", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 6);
  writer->WriteString(9, "$SURFTYPE");
  if (getInt("$SURFTYPE", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 6);
  writer->WriteString(9, "$SURFU");
  if (getInt("$SURFU", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 6);
  writer->WriteString(9, "$SURFV");
  if (getInt("$SURFV", &varInt)) {
    writer->WriteInt16(70, varInt);
  } else
    writer->WriteInt16(70, 6);
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$UCSBASE");
    if (getStr("$UCSBASE", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(2, varStr);
      else
        writer->WriteUtf8String(2, varStr);
    else
      writer->WriteString(2, "");
  }
  writer->WriteString(9, "$UCSNAME");
  if (getStr("$UCSNAME", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(2, varStr);
    else
      writer->WriteUtf8String(2, varStr);
  else
    writer->WriteString(2, "");
  writer->WriteString(9, "$UCSORG");
  if (getCoord("$UCSORG", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }
  writer->WriteString(9, "$UCSXDIR");
  if (getCoord("$UCSXDIR", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }
  writer->WriteString(9, "$UCSYDIR");
  if (getCoord("$UCSYDIR", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$UCSORTHOREF");
    if (getStr("$UCSORTHOREF", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(2, varStr);
      else
        writer->WriteUtf8String(2, varStr);
    else
      writer->WriteString(2, "");
    writer->WriteString(9, "$UCSORTHOVIEW");
    if (getInt("$UCSORTHOVIEW", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$UCSORGTOP");
    if (getCoord("$UCSORGTOP", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$UCSORGBOTTOM");
    if (getCoord("$UCSORGBOTTOM", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$UCSORGLEFT");
    if (getCoord("$UCSORGLEFT", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$UCSORGRIGHT");
    if (getCoord("$UCSORGRIGHT", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$UCSORGFRONT");
    if (getCoord("$UCSORGFRONT", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$UCSORGBACK");
    if (getCoord("$UCSORGBACK", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$PUCSBASE");
    if (getStr("$PUCSBASE", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(2, varStr);
      else
        writer->WriteUtf8String(2, varStr);
    else
      writer->WriteString(2, "");
  }  //end post r12 UCS vars
  writer->WriteString(9, "$PUCSNAME");
  if (getStr("$PUCSNAME", &varStr))
    if (version == DRW::Version::AC1009)
      writer->WriteUtf8Caps(2, varStr);
    else
      writer->WriteUtf8String(2, varStr);
  else
    writer->WriteString(2, "");
  writer->WriteString(9, "$PUCSORG");
  if (getCoord("$PUCSORG", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }
  writer->WriteString(9, "$PUCSXDIR");
  if (getCoord("$PUCSXDIR", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }
  writer->WriteString(9, "$PUCSYDIR");
  if (getCoord("$PUCSYDIR", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$PUCSORTHOREF");
    if (getStr("$PUCSORTHOREF", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(2, varStr);
      else
        writer->WriteUtf8String(2, varStr);
    else
      writer->WriteString(2, "");
    writer->WriteString(9, "$PUCSORTHOVIEW");
    if (getInt("$PUCSORTHOVIEW", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$PUCSORGTOP");
    if (getCoord("$PUCSORGTOP", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$PUCSORGBOTTOM");
    if (getCoord("$PUCSORGBOTTOM", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$PUCSORGLEFT");
    if (getCoord("$PUCSORGLEFT", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$PUCSORGRIGHT");
    if (getCoord("$PUCSORGRIGHT", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$PUCSORGFRONT");
    if (getCoord("$PUCSORGFRONT", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
    writer->WriteString(9, "$PUCSORGBACK");
    if (getCoord("$PUCSORGBACK", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
  }  //end post r12 PUCS vars

  writer->WriteString(9, "$USERI1");
  if (getInt("$USERI1", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$USERI2");
  if (getInt("$USERI2", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$USERI3");
  if (getInt("$USERI3", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$USERI4");
  if (getInt("$USERI4", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$USERI5");
  if (getInt("$USERI5", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$USERR1");
  if (GetDouble("$USERR1", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$USERR2");
  if (GetDouble("$USERR2", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$USERR3");
  if (GetDouble("$USERR3", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$USERR4");
  if (GetDouble("$USERR4", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$USERR5");
  if (GetDouble("$USERR5", &varDouble))
    writer->WriteDouble(40, varDouble);
  else
    writer->WriteDouble(40, 0.0);
  writer->WriteString(9, "$WORLDVIEW");
  if (getInt("$WORLDVIEW", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$SHADEDGE");
  if (getInt("$SHADEDGE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 3);
  writer->WriteString(9, "$SHADEDIF");
  if (getInt("$SHADEDIF", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 70);
  writer->WriteString(9, "$TILEMODE");
  if (getInt("$TILEMODE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$MAXACTVP");
  if (getInt("$MAXACTVP", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 64);
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$PINSBASE");
    if (getCoord("$PINSBASE", &varCoord)) {
      writer->WriteDouble(10, varCoord.x);
      writer->WriteDouble(20, varCoord.y);
      writer->WriteDouble(30, varCoord.z);
    } else {
      writer->WriteDouble(10, 0.0);
      writer->WriteDouble(20, 0.0);
      writer->WriteDouble(30, 0.0);
    }
  }
  writer->WriteString(9, "$PLIMCHECK");
  if (getInt("$PLIMCHECK", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$PEXTMIN");
  if (getCoord("$PEXTMIN", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }
  writer->WriteString(9, "$PEXTMAX");
  if (getCoord("$PEXTMAX", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
    writer->WriteDouble(30, varCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  if (getInt("$GRIDMODE", &varInt)) {
    writer->WriteString(9, "$GRIDMODE");
    writer->WriteInt16(70, varInt);
  }
  if (getInt("$SNAPSTYLE", &varInt)) {
    writer->WriteString(9, "$SNAPSTYLE");
    writer->WriteInt16(70, varInt);
  }
  if (getCoord("$GRIDUNIT", &varCoord)) {
    writer->WriteString(9, "$GRIDUNIT");
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
  }
  if (getCoord("$VIEWCTR", &varCoord)) {
    writer->WriteString(9, "$VIEWCTR");
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
  }

  writer->WriteString(9, "$PLIMMIN");
  if (getCoord("$PLIMMIN", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }
  writer->WriteString(9, "$PLIMMAX");
  if (getCoord("$PLIMMAX", &varCoord)) {
    writer->WriteDouble(10, varCoord.x);
    writer->WriteDouble(20, varCoord.y);
  } else {
    writer->WriteDouble(10, 297.0);
    writer->WriteDouble(20, 210.0);
  }
  writer->WriteString(9, "$UNITMODE");
  if (getInt("$UNITMODE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$VISRETAIN");
  if (getInt("$VISRETAIN", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  writer->WriteString(9, "$PLINEGEN");
  if (getInt("$PLINEGEN", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 0);
  writer->WriteString(9, "$PSLTSCALE");
  if (getInt("$PSLTSCALE", &varInt))
    writer->WriteInt16(70, varInt);
  else
    writer->WriteInt16(70, 1);
  if (version > DRW::Version::AC1009) {
    writer->WriteString(9, "$TREEDEPTH");
    if (getInt("$TREEDEPTH", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 3020);
    writer->WriteString(9, "$CMLSTYLE");
    if (getStr("$CMLSTYLE", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(2, varStr);
      else
        writer->WriteUtf8String(2, varStr);
    else
      writer->WriteString(2, "Standard");
    writer->WriteString(9, "$CMLJUST");
    if (getInt("$CMLJUST", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 0);
    writer->WriteString(9, "$CMLSCALE");
    if (GetDouble("$CMLSCALE", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 20.0);
    writer->WriteString(9, "$PROXYGRAPHICS");
    if (getInt("$PROXYGRAPHICS", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 1);
    writer->WriteString(9, "$MEASUREMENT");
    if (getInt("$MEASUREMENT", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 1);
    writer->WriteString(9, "$CELWEIGHT");
    if (getInt("$CELWEIGHT", &varInt))
      writer->WriteInt16(370, varInt);
    else
      writer->WriteInt16(370, -1);
    writer->WriteString(9, "$ENDCAPS");
    if (getInt("$ENDCAPS", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$JOINSTYLE");
    if (getInt("$JOINSTYLE", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$LWDISPLAY");
    if (getInt("$LWDISPLAY", &varInt))
      writer->WriteInt16(290, varInt);
    else
      writer->WriteInt16(290, 0);
    if (version > DRW::Version::AC1014) {
      writer->WriteString(9, "$INSUNITS");
      if (getInt("$INSUNITS", &varInt))
        writer->WriteInt16(70, varInt);
      else
        writer->WriteInt16(70, 0);
    }
    writer->WriteString(9, "$HYPERLINKBASE");
    if (getStr("$HYPERLINKBASE", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(1, varStr);
      else
        writer->WriteUtf8String(1, varStr);
    else
      writer->WriteString(1, "");
    writer->WriteString(9, "$STYLESHEET");
    if (getStr("$STYLESHEET", &varStr))
      if (version == DRW::Version::AC1009)
        writer->WriteUtf8Caps(1, varStr);
      else
        writer->WriteUtf8String(1, varStr);
    else
      writer->WriteString(1, "");
    writer->WriteString(9, "$XEDIT");
    if (getInt("$XEDIT", &varInt))
      writer->WriteInt16(290, varInt);
    else
      writer->WriteInt16(290, 1);
    writer->WriteString(9, "$CEPSNTYPE");
    if (getInt("$CEPSNTYPE", &varInt))
      writer->WriteInt16(380, varInt);
    else
      writer->WriteInt16(380, 0);
    writer->WriteString(9, "$PSTYLEMODE");
    if (getInt("$PSTYLEMODE", &varInt)) {
      writer->WriteInt16(290, varInt);
    } else {
      writer->WriteInt16(290, 1);
    }

    writer->WriteString(9, "$EXTNAMES");
    if (getInt("$EXTNAMES", &varInt))
      writer->WriteInt16(290, varInt);
    else
      writer->WriteInt16(290, 1);
    writer->WriteString(9, "$PSVPSCALE");
    if (GetDouble("$PSVPSCALE", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
    writer->WriteString(9, "$OLESTARTUP");
    if (getInt("$OLESTARTUP", &varInt))
      writer->WriteInt16(290, varInt);
    else
      writer->WriteInt16(290, 0);
  }
  if (version > DRW::Version::AC1015) {
    writer->WriteString(9, "$SORTENTS");
    if (getInt("$SORTENTS", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 127);
    writer->WriteString(9, "$INDEXCTL");
    if (getInt("$INDEXCTL", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$HIDETEXT");
    if (getInt("$HIDETEXT", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 1);
    writer->WriteString(9, "$XCLIPFRAME");
    if (version > DRW::Version::AC1021) {
      if (getInt("$XCLIPFRAME", &varInt))
        writer->WriteInt16(280, varInt);
      else
        writer->WriteInt16(280, 0);
    } else {
      if (getInt("$XCLIPFRAME", &varInt))
        writer->WriteInt16(290, varInt);
      else
        writer->WriteInt16(290, 0);
    }
    writer->WriteString(9, "$HALOGAP");
    if (getInt("$HALOGAP", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$OBSCOLOR");
    if (getInt("$OBSCOLOR", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 257);
    writer->WriteString(9, "$OBSLTYPE");
    if (getInt("$OBSLTYPE", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$INTERSECTIONDISPLAY");
    if (getInt("$INTERSECTIONDISPLAY", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$INTERSECTIONCOLOR");
    if (getInt("$INTERSECTIONCOLOR", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 257);
    writer->WriteString(9, "$DIMASSOC");
    if (getInt("$DIMASSOC", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 1);
    writer->WriteString(9, "$PROJECTNAME");
    if (getStr("$PROJECTNAME", &varStr))
      writer->WriteUtf8String(1, varStr);
    else
      writer->WriteString(1, "");
  }
  if (version > DRW::Version::AC1018) {
    writer->WriteString(9, "$CAMERADISPLAY");
    if (getInt("$CAMERADISPLAY", &varInt))
      writer->WriteInt16(290, varInt);
    else
      writer->WriteInt16(290, 0);
    writer->WriteString(9, "$LENSLENGTH");
    if (GetDouble("$LENSLENGTH", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 50.0);
    writer->WriteString(9, "$CAMERAHEIGHT");
    if (GetDouble("$CAMERAHEIGTH", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
    writer->WriteString(9, "$STEPSPERSEC");
    if (GetDouble("$STEPSPERSEC", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 2.0);
    writer->WriteString(9, "$STEPSIZE");
    if (GetDouble("$STEPSIZE", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 50.0);
    writer->WriteString(9, "$3DDWFPREC");
    if (GetDouble("$3DDWFPREC", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 2.0);
    writer->WriteString(9, "$PSOLWIDTH");
    if (GetDouble("$PSOLWIDTH", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 5.0);
    writer->WriteString(9, "$PSOLHEIGHT");
    if (GetDouble("$PSOLHEIGHT", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 80.0);
    writer->WriteString(9, "$LOFTANG1");
    if (GetDouble("$LOFTANG1", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, DRW::HalfPi);
    writer->WriteString(9, "$LOFTANG2");
    if (GetDouble("$LOFTANG2", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, DRW::HalfPi);
    writer->WriteString(9, "$LOFTMAG1");
    if (GetDouble("$LOFTMAG1", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
    writer->WriteString(9, "$LOFTMAG2");
    if (GetDouble("$LOFTMAG2", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
    writer->WriteString(9, "$LOFTPARAM");
    if (getInt("$LOFTPARAM", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, 7);
    writer->WriteString(9, "$LOFTNORMALS");
    if (getInt("$LOFTNORMALS", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 1);
    writer->WriteString(9, "$LATITUDE");
    if (GetDouble("$LATITUDE", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 1.0);
    writer->WriteString(9, "$LONGITUDE");
    if (GetDouble("$LONGITUDE", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 1.0);
    writer->WriteString(9, "$NORTHDIRECTION");
    if (GetDouble("$LONGITUDE", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
    writer->WriteString(9, "$TIMEZONE");
    if (getInt("$TIMEZONE", &varInt))
      writer->WriteInt16(70, varInt);
    else
      writer->WriteInt16(70, -8000);
    writer->WriteString(9, "$LIGHTGLYPHDISPLAY");
    if (getInt("$LIGHTGLYPHDISPLAY", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 1);
    writer->WriteString(9, "$TILEMODELIGHTSYNCH");
    if (getInt("$TILEMODELIGHTSYNCH", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 1);
    //$CMATERIAL is a handle
    writer->WriteString(9, "$SOLIDHIST");
    if (getInt("$SOLIDHIST", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 1);
    writer->WriteString(9, "$SHOWHIST");
    if (getInt("$SHOWHIST", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 1);
    writer->WriteString(9, "$DWFFRAME");
    if (getInt("$DWFFRAME", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 2);
    writer->WriteString(9, "$DGNFRAME");
    if (getInt("$DGNFRAME", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$REALWORLDSCALE");
    if (getInt("$REALWORLDSCALE", &varInt))
      writer->WriteInt16(290, varInt);
    else
      writer->WriteInt16(290, 1);
    writer->WriteString(9, "$INTERFERECOLOR");
    if (getInt("$INTERFERECOLOR", &varInt))
      writer->WriteInt16(62, varInt);
    else
      writer->WriteInt16(62, 1);
    //$INTERFEREOBJVS is a handle
    //$INTERFEREVPVS is a handle
    writer->WriteString(9, "$CSHADOW");
    if (getInt("$CSHADOW", &varInt))
      writer->WriteInt16(280, varInt);
    else
      writer->WriteInt16(280, 0);
    writer->WriteString(9, "$SHADOWPLANELOCATION");
    if (GetDouble("$SHADOWPLANELOCATION", &varDouble))
      writer->WriteDouble(40, varDouble);
    else
      writer->WriteDouble(40, 0.0);
  }

#ifdef DRW_DBG
  std::map<std::string, DRW_Variant*>::const_iterator it;
  for (it = vars.begin(); it != vars.end(); ++it) {
    DRW_DBG((*it).first);
    DRW_DBG("\n");
  }
#endif
}

void DRW_Header::addDouble(std::string key, double value, int code) {
  curr = new DRW_Variant(code, value);
  vars[key] = curr;
}

void DRW_Header::addInt(std::string key, int value, int code) {
  curr = new DRW_Variant(code, value);
  vars[key] = curr;
}

void DRW_Header::addStr(std::string key, std::string value, int code) {
  curr = new DRW_Variant(code, value);
  vars[key] = curr;
}

void DRW_Header::addCoord(std::string key, DRW_Coord value, int code) {
  curr = new DRW_Variant(code, value);
  vars[key] = curr;
}

bool DRW_Header::GetDouble(std::string key, double* varDouble) {
  bool result = false;
  std::map<std::string, DRW_Variant*>::iterator it;
  it = vars.find(key);
  if (it != vars.end()) {
    DRW_Variant* var = (*it).second;
    if (var->type() == DRW_Variant::Type::Double) {
      *varDouble = var->content.d;
      result = true;
    }
    delete var;
    vars.erase(it);
  }
  return result;
}

bool DRW_Header::getInt(std::string key, int* varInt) {
  bool result = false;
  std::map<std::string, DRW_Variant*>::iterator it;
  it = vars.find(key);
  if (it != vars.end()) {
    DRW_Variant* var = (*it).second;
    if (var->type() == DRW_Variant::Type::Integer) {
      *varInt = var->content.i;
      result = true;
    }
    delete var;
    vars.erase(it);
  }
  return result;
}

bool DRW_Header::getStr(std::string key, std::string* varStr) {
  bool result = false;
  std::map<std::string, DRW_Variant*>::iterator it;
  it = vars.find(key);
  if (it != vars.end()) {
    DRW_Variant* var = (*it).second;
    if (var->type() == DRW_Variant::Type::String) {
      *varStr = *var->content.s;
      result = true;
    }
    delete var;
    vars.erase(it);
  }
  return result;
}

bool DRW_Header::getCoord(std::string key, DRW_Coord* varCoord) {
  bool result = false;
  std::map<std::string, DRW_Variant*>::iterator it;
  it = vars.find(key);
  if (it != vars.end()) {
    DRW_Variant* var = (*it).second;
    if (var->type() == DRW_Variant::Type::Coord) {
      *varCoord = *var->content.v;
      result = true;
    }
    delete var;
    vars.erase(it);
  }
  return result;
}
