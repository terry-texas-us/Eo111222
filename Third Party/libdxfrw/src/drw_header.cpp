#include <map>
#include <string>

#include "drw_base.h"
#include "drw_header.h"
#include "intern/drw_dbg.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include <algorithm>

namespace {
/** Helper function to write a UTF-8 string to the dxfWriter, handling differences in how strings are written based on the DXF version.
 * For AC1009 (R11/R12), strings are written in uppercase, while for later versions, they are written as-is.
 *
 * @param writer Pointer to the dxfWriter object used to write the string.
 * @param code The group code associated with the string being written.
 * @param str The UTF-8 string to be written to the dxfWriter.
 * @param ver The DXF version being targeted for writing, which determines how the string is formatted.
 * @return true if the string was successfully written, false otherwise.
 */
bool WriteUtf8String(dxfWriter* writer, int code, const std::string& str, DRW::Version ver) {
  if (ver == DRW::Version::AC1009) { return writer->WriteUtf8Caps(code, str); }
  return writer->WriteUtf8String(code, str);
}
}  // namespace

DRW_Header::DRW_Header(const DRW_Header& other)
    : m_version{other.m_version}, m_comments{other.m_comments}, m_currentVariant{nullptr} {
  for (const auto& [key, variant] : other.m_variants) { m_variants.emplace(key, new DRW_Variant(*variant)); }
}

DRW_Header& DRW_Header::operator=(const DRW_Header& other) {
  if (this == &other) { return *this; }
  ClearVariants();
  m_version = other.m_version;
  m_comments = other.m_comments;
  m_currentVariant = nullptr;

  for (const auto& [key, variant] : other.m_variants) { m_variants.emplace(key, new DRW_Variant(*variant)); }
  return *this;
}

void DRW_Header::AddComment(std::string comment) {
  if (!m_comments.empty()) { m_comments += '\n'; }
  m_comments += comment;
}

void DRW_Header::ParseCode(int code, dxfReader* reader) {
  switch (code) {
    case 9:
      m_currentVariant = new DRW_Variant();
      m_name = reader->GetString();
      if (m_version < DRW::Version::AC1015 && m_name == "$DIMUNIT") { m_name = "$DIMLUNIT"; }
      m_variants[m_name] = m_currentVariant;
      break;
    case 1:
      m_currentVariant->addString(code, reader->GetUtf8String());
      if (m_name == "$ACADVER") {
        reader->SetVersion(m_currentVariant->content.s, true);
        m_version = reader->GetVersion();
      }
      break;
    case 2:
      m_currentVariant->addString(code, reader->GetUtf8String());
      break;
    case 3:
      m_currentVariant->addString(code, reader->GetUtf8String());
      if (m_name == "$DWGCODEPAGE") {
        reader->SetCodePage(m_currentVariant->content.s);
        m_currentVariant->addString(code, reader->GetCodePage());
      }
      break;
    case 6:
      m_currentVariant->addString(code, reader->GetUtf8String());
      break;
    case 7:
      m_currentVariant->addString(code, reader->GetUtf8String());
      break;
    case 8:
      m_currentVariant->addString(code, reader->GetUtf8String());
      break;
    case 10:
      m_currentVariant->addCoord(code, DRW_Coord(reader->GetDouble(), 0.0, 0.0));
      break;
    case 20:
      m_currentVariant->setCoordY(reader->GetDouble());
      break;
    case 30:
      m_currentVariant->setCoordZ(reader->GetDouble());
      break;
    case 40:
      m_currentVariant->addDouble(code, reader->GetDouble());
      break;
    case 50:
      m_currentVariant->addDouble(code, reader->GetDouble());
      break;
    case 62:
      m_currentVariant->addInt(code, reader->GetInt32());
      break;
    case 70:
      m_currentVariant->addInt(code, reader->GetInt32());
      break;
    case 280:
      m_currentVariant->addInt(code, reader->GetInt32());
      break;
    case 290:
      m_currentVariant->addInt(code, reader->GetInt32());
      break;
    case 370:
      m_currentVariant->addInt(code, reader->GetInt32());
      break;
    case 380:
      m_currentVariant->addInt(code, reader->GetInt32());
      break;
    case 390:
      m_currentVariant->addString(code, reader->GetUtf8String());
      break;
    default:
      break;
  }
}

void DRW_Header::WriteBase(dxfWriter* writer, DRW::Version version) {
  double variantDouble;
  int variantInteger;
  std::string variantString;
  DRW_Coord variantCoord;

  writer->WriteString(9, "$INSBASE");
  if (GetCoord("$INSBASE", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$EXTMIN");
  if (GetCoord("$EXTMIN", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 1.0000000000000000E+020);
    writer->WriteDouble(20, 1.0000000000000000E+020);
    writer->WriteDouble(30, 1.0000000000000000E+020);
  }

  writer->WriteString(9, "$EXTMAX");
  if (GetCoord("$EXTMAX", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, -1.0000000000000000E+020);
    writer->WriteDouble(20, -1.0000000000000000E+020);
    writer->WriteDouble(30, -1.0000000000000000E+020);
  }

  writer->WriteString(9, "$LIMMIN");
  if (GetCoord("$LIMMIN", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }

  writer->WriteString(9, "$LIMMAX");
  if (GetCoord("$LIMMAX", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
  } else {
    writer->WriteDouble(10, 420.0);
    writer->WriteDouble(20, 297.0);
  }

  writer->WriteString(9, "$ORTHOMODE");
  writer->WriteInt16(70, GetInteger("$ORTHOMODE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$REGENMODE");
  writer->WriteInt16(70, GetInteger("$REGENMODE", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$FILLMODE");
  writer->WriteInt16(70, GetInteger("$FILLMODE", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$QTEXTMODE");
  writer->WriteInt16(70, GetInteger("$QTEXTMODE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$MIRRTEXT");
  writer->WriteInt16(70, GetInteger("$MIRRTEXT", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$LTSCALE");
  writer->WriteDouble(40, GetDouble("$LTSCALE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$ATTMODE");
  writer->WriteInt16(70, GetInteger("$ATTMODE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$TEXTSIZE");
  writer->WriteDouble(40, GetDouble("$TEXTSIZE", &variantDouble) ? variantDouble : 2.5);

  writer->WriteString(9, "$TRACEWID");
  writer->WriteDouble(40, GetDouble("$TRACEWID", &variantDouble) ? variantDouble : 15.68);

  writer->WriteString(9, "$TEXTSTYLE");
  if (GetString("$TEXTSTYLE", &variantString)) {
    WriteUtf8String(writer, 7, variantString, version);
  } else {
    writer->WriteString(7, "STANDARD");
  }

  writer->WriteString(9, "$CLAYER");
  if (GetString("$CLAYER", &variantString)) {
    WriteUtf8String(writer, 8, variantString, version);
  } else {
    writer->WriteString(8, "0");
  }

  writer->WriteString(9, "$CELTYPE");
  if (GetString("$CELTYPE", &variantString)) {
    WriteUtf8String(writer, 6, variantString, version);
  } else {
    writer->WriteString(6, "BYLAYER");
  }

  writer->WriteString(9, "$CECOLOR");
  writer->WriteInt16(62, GetInteger("$CECOLOR", &variantInteger) ? variantInteger : 256);

  writer->WriteString(9, "$DIMSCALE");
  writer->WriteDouble(40, GetDouble("$DIMSCALE", &variantDouble) ? variantDouble : 2.5);

  writer->WriteString(9, "$DIMASZ");
  writer->WriteDouble(40, GetDouble("$DIMASZ", &variantDouble) ? variantDouble : 2.5);

  writer->WriteString(9, "$DIMEXO");
  writer->WriteDouble(40, GetDouble("$DIMEXO", &variantDouble) ? variantDouble : 0.625);

  writer->WriteString(9, "$DIMDLI");
  writer->WriteDouble(40, GetDouble("$DIMDLI", &variantDouble) ? variantDouble : 3.75);

  writer->WriteString(9, "$DIMRND");
  writer->WriteDouble(40, GetDouble("$DIMRND", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMDLE");
  writer->WriteDouble(40, GetDouble("$DIMDLE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMEXE");
  writer->WriteDouble(40, GetDouble("$DIMEXE", &variantDouble) ? variantDouble : 1.25);

  writer->WriteString(9, "$DIMTP");
  writer->WriteDouble(40, GetDouble("$DIMTP", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMTM");
  writer->WriteDouble(40, GetDouble("$DIMTM", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMTXT");
  writer->WriteDouble(40, GetDouble("$DIMTXT", &variantDouble) ? variantDouble : 2.5);

  writer->WriteString(9, "$DIMCEN");
  writer->WriteDouble(40, GetDouble("$DIMCEN", &variantDouble) ? variantDouble : 2.5);

  writer->WriteString(9, "$DIMTSZ");
  writer->WriteDouble(40, GetDouble("$DIMTSZ", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMTOL");
  writer->WriteInt16(70, GetInteger("$DIMTOL", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMLIM");
  writer->WriteInt16(70, GetInteger("$DIMLIM", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMTIH");
  writer->WriteInt16(70, GetInteger("$DIMTIH", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMTOH");
  writer->WriteInt16(70, GetInteger("$DIMTOH", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMSE1");
  writer->WriteInt16(70, GetInteger("$DIMSE1", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMSE2");
  writer->WriteInt16(70, GetInteger("$DIMSE2", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMTAD");
  writer->WriteInt16(70, GetInteger("$DIMTAD", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$DIMZIN");
  writer->WriteInt16(70, GetInteger("$DIMZIN", &variantInteger) ? variantInteger : 8);

  writer->WriteString(9, "$DIMBLK");
  if (GetString("$DIMBLK", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMASO");
  writer->WriteInt16(70, GetInteger("$DIMASO", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$DIMSHO");
  writer->WriteInt16(70, GetInteger("$DIMSHO", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$DIMPOST");
  if (GetString("$DIMPOST", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMAPOST");
  if (GetString("$DIMAPOST", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMALT");
  writer->WriteInt16(70, GetInteger("$DIMALT", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMALTD");
  writer->WriteInt16(70, GetInteger("$DIMALTD", &variantInteger) ? variantInteger : 3);

  writer->WriteString(9, "$DIMALTF");
  writer->WriteDouble(40, GetDouble("$DIMALTF", &variantDouble) ? variantDouble : 0.03937);

  writer->WriteString(9, "$DIMLFAC");
  writer->WriteDouble(40, GetDouble("$DIMLFAC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DIMTOFL");
  writer->WriteInt16(70, GetInteger("$DIMTOFL", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$DIMTVP");
  writer->WriteDouble(40, GetDouble("$DIMTVP", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMTIX");
  writer->WriteInt16(70, GetInteger("$DIMTIX", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMSOXD");
  writer->WriteInt16(70, GetInteger("$DIMSOXD", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMSAH");
  writer->WriteInt16(70, GetInteger("$DIMSAH", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMBLK1");
  if (GetString("$DIMBLK1", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMBLK2");
  if (GetString("$DIMBLK2", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$LUNITS");
  writer->WriteInt16(70, GetInteger("$LUNITS", &variantInteger) ? variantInteger : 2);

  writer->WriteString(9, "$LUPREC");
  writer->WriteInt16(70, GetInteger("$LUPREC", &variantInteger) ? variantInteger : 4);

  writer->WriteString(9, "$SKETCHINC");
  writer->WriteDouble(40, GetDouble("$SKETCHINC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$FILLETRAD");
  writer->WriteDouble(40, GetDouble("$FILLETRAD", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$AUNITS");
  writer->WriteInt16(70, GetInteger("$AUNITS", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$AUPREC");
  writer->WriteInt16(70, GetInteger("$AUPREC", &variantInteger) ? variantInteger : 2);

  writer->WriteString(9, "$MENU");
  if (GetString("$MENU", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, ".");
  }

  writer->WriteString(9, "$ELEVATION");
  writer->WriteDouble(40, GetDouble("$ELEVATION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$THICKNESS");
  writer->WriteDouble(40, GetDouble("$THICKNESS", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$LIMCHECK");
  writer->WriteInt16(70, GetInteger("$LIMCHECK", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$CHAMFERA");
  writer->WriteDouble(40, GetDouble("$CHAMFERA", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$CHAMFERB");
  writer->WriteDouble(40, GetDouble("$CHAMFERB", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$SKPOLY");
  writer->WriteInt16(70, GetInteger("$SKPOLY", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$USRTIMER");
  writer->WriteInt16(70, GetInteger("$USRTIMER", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$ANGBASE");
  writer->WriteDouble(50, GetDouble("$ANGBASE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$ANGDIR");
  writer->WriteInt16(70, GetInteger("$ANGDIR", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$PDMODE");
  writer->WriteInt16(70, GetInteger("$PDMODE", &variantInteger) ? variantInteger : 34);

  writer->WriteString(9, "$PDSIZE");
  writer->WriteDouble(40, GetDouble("$PDSIZE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$PLINEWID");
  writer->WriteDouble(40, GetDouble("$PLINEWID", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$SPLFRAME");
  writer->WriteInt16(70, GetInteger("$SPLFRAME", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$SPLINETYPE");
  writer->WriteInt16(70, GetInteger("$SPLINETYPE", &variantInteger) ? variantInteger : 2);

  writer->WriteString(9, "$SPLINESEGS");
  writer->WriteInt16(70, GetInteger("$SPLINESEGS", &variantInteger) ? variantInteger : 8);

  writer->WriteString(9, "$SURFTAB1");
  writer->WriteInt16(70, GetInteger("$SURFTAB1", &variantInteger) ? variantInteger : 6);

  writer->WriteString(9, "$SURFTAB2");
  writer->WriteInt16(70, GetInteger("$SURFTAB2", &variantInteger) ? variantInteger : 6);

  writer->WriteString(9, "$SURFTYPE");
  writer->WriteInt16(70, GetInteger("$SURFTYPE", &variantInteger) ? variantInteger : 6);

  writer->WriteString(9, "$SURFU");
  writer->WriteInt16(70, GetInteger("$SURFU", &variantInteger) ? variantInteger : 6);

  writer->WriteString(9, "$SURFV");
  writer->WriteInt16(70, GetInteger("$SURFV", &variantInteger) ? variantInteger : 6);

  writer->WriteString(9, "$UCSNAME");
  if (GetString("$UCSNAME", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$UCSORG");
  if (GetCoord("$UCSORG", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSXDIR");
  if (GetCoord("$UCSXDIR", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSYDIR");
  if (GetCoord("$UCSYDIR", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$USERI1");
  writer->WriteInt16(70, GetInteger("$USERI1", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$USERI2");
  writer->WriteInt16(70, GetInteger("$USERI2", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$USERI3");
  writer->WriteInt16(70, GetInteger("$USERI3", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$USERI4");
  writer->WriteInt16(70, GetInteger("$USERI4", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$USERI5");
  writer->WriteInt16(70, GetInteger("$USERI5", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$USERR1");
  writer->WriteDouble(40, GetDouble("$USERR1", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$USERR2");
  writer->WriteDouble(40, GetDouble("$USERR2", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$USERR3");
  writer->WriteDouble(40, GetDouble("$USERR3", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$USERR4");
  writer->WriteDouble(40, GetDouble("$USERR4", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$USERR5");
  writer->WriteDouble(40, GetDouble("$USERR5", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$WORLDVIEW");
  writer->WriteInt16(70, GetInteger("$WORLDVIEW", &variantInteger) ? variantInteger : 1);
}

void DRW_Header::WriteAC1009Additions(dxfWriter* writer, DRW::Version version) {
  double variantDouble;
  int variantInteger;
  std::string variantString;
  DRW_Coord variantCoord;

  writer->WriteString(9, "$DIMSTYLE");  // not used before AC1009
  if (GetString("$DIMSTYLE", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "STANDARD");
  }

  writer->WriteString(9, "$DIMCLRD");
  writer->WriteInt16(70, GetInteger("$DIMCLRD", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMCLRE");
  writer->WriteInt16(70, GetInteger("$DIMCLRE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMCLRT");
  writer->WriteInt16(70, GetInteger("$DIMCLRT", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMTFAC");
  writer->WriteDouble(40, GetDouble("$DIMTFAC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DIMGAP");
  writer->WriteDouble(40, GetDouble("$DIMGAP", &variantDouble) ? variantDouble : 0.625);

  writer->WriteString(9, "$PELEVATION");
  writer->WriteDouble(40, GetDouble("$PELEVATION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$PUCSNAME");
  if (GetString("$PUCSNAME", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSORG");
  if (GetCoord("$PUCSORG", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSXDIR");
  if (GetCoord("$PUCSXDIR", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSYDIR");
  if (GetCoord("$PUCSYDIR", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$SHADEDGE");
  writer->WriteInt16(70, GetInteger("$SHADEDGE", &variantInteger) ? variantInteger : 3);

  writer->WriteString(9, "$SHADEDIF");
  writer->WriteInt16(70, GetInteger("$SHADEDIF", &variantInteger) ? variantInteger : 70);

  writer->WriteString(9, "$TILEMODE");
  writer->WriteInt16(70, GetInteger("$TILEMODE", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$MAXACTVP");
  writer->WriteInt16(70, GetInteger("$MAXACTVP", &variantInteger) ? variantInteger : 64);

  writer->WriteString(9, "$PLIMCHECK");
  writer->WriteInt16(70, GetInteger("$PLIMCHECK", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$PEXTMIN");
  if (GetCoord("$PEXTMIN", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PEXTMAX");
  if (GetCoord("$PEXTMAX", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PLIMMIN");
  if (GetCoord("$PLIMMIN", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }

  writer->WriteString(9, "$PLIMMAX");
  if (GetCoord("$PLIMMAX", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
  } else {
    writer->WriteDouble(10, 297.0);
    writer->WriteDouble(20, 210.0);
  }

  writer->WriteString(9, "$UNITMODE");
  if (GetInteger("$UNITMODE", &variantInteger))
    writer->WriteInt16(70, variantInteger);
  else
    writer->WriteInt16(70, 0);

  writer->WriteString(9, "$VISRETAIN");
  writer->WriteInt16(70, GetInteger("$VISRETAIN", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$PLINEGEN");
  writer->WriteInt16(70, GetInteger("$PLINEGEN", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$PSLTSCALE");
  writer->WriteInt16(70, GetInteger("$PSLTSCALE", &variantInteger) ? variantInteger : 1);
}

void DRW_Header::WriteAC1012Additions(dxfWriter* writer, DRW::Version version) {
  double variantDouble;
  int variantInteger;
  std::string variantString;
  DRW_Coord variantCoord;

  writer->WriteString(9, "$CELTSCALE");
  writer->WriteDouble(40, GetDouble("$CELTSCALE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DISPSILH");
  writer->WriteInt16(70, GetInteger("$DISPSILH", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMJUST");
  writer->WriteInt16(70, GetInteger("$DIMJUST", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMSD1");
  writer->WriteInt16(70, GetInteger("$DIMSD1", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMSD2");
  writer->WriteInt16(70, GetInteger("$DIMSD2", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMTOLJ");
  writer->WriteInt16(70, GetInteger("$DIMTOLJ", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMTZIN");
  writer->WriteInt16(70, GetInteger("$DIMTZIN", &variantInteger) ? variantInteger : 8);

  writer->WriteString(9, "$DIMALTZ");
  writer->WriteInt16(70, GetInteger("$DIMALTZ", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMALTTZ");
  writer->WriteInt16(70, GetInteger("$DIMALTTZ", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMUPT");
  writer->WriteInt16(70, GetInteger("$DIMUPT", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMDEC");
  writer->WriteInt16(70, GetInteger("$DIMDEC", &variantInteger) ? variantInteger : 2);

  writer->WriteString(9, "$DIMTDEC");
  writer->WriteInt16(70, GetInteger("$DIMTDEC", &variantInteger) ? variantInteger : 2);

  writer->WriteString(9, "$DIMALTU");
  writer->WriteInt16(70, GetInteger("$DIMALTU", &variantInteger) ? variantInteger : 2);

  writer->WriteString(9, "$DIMALTTD");
  writer->WriteInt16(70, GetInteger("$DIMALTTD", &variantInteger) ? variantInteger : 3);

  writer->WriteString(9, "$DIMTXSTY");
  if (GetString("$DIMTXSTY", &variantString)) {
    WriteUtf8String(writer, 7, variantString, version);
  } else {
    writer->WriteString(7, "STANDARD");
  }

  writer->WriteString(9, "$DIMAUNIT");
  writer->WriteInt16(70, GetInteger("$DIMAUNIT", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$CHAMFERC");
  writer->WriteDouble(40, GetDouble("$CHAMFERC", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$CHAMFERD");
  writer->WriteDouble(40, GetDouble("$CHAMFERD", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$PINSBASE");
  if (GetCoord("$PINSBASE", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$TREEDEPTH");
  writer->WriteInt16(70, GetInteger("$TREEDEPTH", &variantInteger) ? variantInteger : 3020);

  writer->WriteString(9, "$CMLSTYLE");
  if (GetString("$CMLSTYLE", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "Standard");
  }

  writer->WriteString(9, "$CMLJUST");
  writer->WriteInt16(70, GetInteger("$CMLJUST", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$CMLSCALE");
  writer->WriteDouble(40, GetDouble("$CMLSCALE", &variantDouble) ? variantDouble : 20.0);
}

void DRW_Header::WriteAC1014Additions(dxfWriter* writer, DRW::Version version) {
  int variantInteger;

  writer->WriteString(9, "$PROXYGRAPHICS");
  writer->WriteInt16(70, GetInteger("$PROXYGRAPHICS", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$MEASUREMENT");
  writer->WriteInt16(70, GetInteger("$MEASUREMENT", &variantInteger) ? variantInteger : 1);
}

void DRW_Header::WriteAC1015Additions(dxfWriter* writer, DRW::Version version) {
  double variantDouble;
  int variantInteger;
  std::string variantString;
  DRW_Coord variantCoord;

  writer->WriteString(9, "$DIMADEC");
  writer->WriteInt16(70, GetInteger("$DIMADEC", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMALTRND");
  writer->WriteDouble(40, GetDouble("$DIMALTRND", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMAZIN");
  writer->WriteInt16(70, GetInteger("$DIMAZIN", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMDSEP");
  writer->WriteInt16(70, GetInteger("$DIMDSEP", &variantInteger) ? variantInteger : 44);

  writer->WriteString(9, "$DIMATFIT");
  writer->WriteInt16(70, GetInteger("$DIMATFIT", &variantInteger) ? variantInteger : 3);

  writer->WriteString(9, "$DIMFRAC");
  writer->WriteInt16(70, GetInteger("$DIMFRAC", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMLDRBLK");
  if (GetString("$DIMLDRBLK", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "STANDARD");
  }

  //----------------------------
  // `$DIMLUNIT` replaced `$DIMUNIT` in AC1015, but `$DIMUNIT` may still be present in AC1012 and AC1014 files,
  // so check for both and default to 2 (decimal) if neither is found or if the value is out of range
  if (!GetInteger("$DIMLUNIT", &variantInteger)) {
    if (!GetInteger("$DIMUNIT", &variantInteger)) { variantInteger = 2; }
  }
  if (variantInteger < 1 || variantInteger > 6) { variantInteger = 2; }

  if (version > DRW::Version::AC1014) {
    writer->WriteString(9, "$DIMLUNIT");
  } else {
    writer->WriteString(9, "$DIMUNIT");
  }
  writer->WriteInt16(70, variantInteger);
  //----------------------------

  writer->WriteString(9, "$DIMLWD");
  writer->WriteInt16(70, GetInteger("$DIMLWD", &variantInteger) ? variantInteger : -2);

  writer->WriteString(9, "$DIMLWE");
  writer->WriteInt16(70, GetInteger("$DIMLWE", &variantInteger) ? variantInteger : -2);

  writer->WriteString(9, "$DIMTMOVE");
  writer->WriteInt16(70, GetInteger("$DIMTMOVE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$UCSORTHOREF");
  if (GetString("$UCSORTHOREF", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$UCSORTHOVIEW");
  writer->WriteInt16(70, GetInteger("$UCSORTHOVIEW", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$UCSORGTOP");
  if (GetCoord("$UCSORGTOP", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGBOTTOM");
  if (GetCoord("$UCSORGBOTTOM", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGLEFT");
  if (GetCoord("$UCSORGLEFT", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGRIGHT");
  if (GetCoord("$UCSORGRIGHT", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGFRONT");
  if (GetCoord("$UCSORGFRONT", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGBACK");
  if (GetCoord("$UCSORGBACK", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSBASE");
  if (GetString("$UCSBASE", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSBASE");
  if (GetString("$PUCSBASE", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSORTHOREF");
  if (GetString("$PUCSORTHOREF", &variantString)) {
    WriteUtf8String(writer, 2, variantString, version);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSORTHOVIEW");
  writer->WriteInt16(70, GetInteger("$PUCSORTHOVIEW", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$PUCSORGTOP");
  if (GetCoord("$PUCSORGTOP", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGBOTTOM");
  if (GetCoord("$PUCSORGBOTTOM", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGLEFT");
  if (GetCoord("$PUCSORGLEFT", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGRIGHT");
  if (GetCoord("$PUCSORGRIGHT", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGFRONT");
  if (GetCoord("$PUCSORGFRONT", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGBACK");
  if (GetCoord("$PUCSORGBACK", &variantCoord)) {
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
    writer->WriteDouble(30, variantCoord.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$CELWEIGHT");
  writer->WriteInt16(370, GetInteger("$CELWEIGHT", &variantInteger) ? variantInteger : -1);

  writer->WriteString(9, "$ENDCAPS");
  writer->WriteInt16(280, GetInteger("$ENDCAPS", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$JOINSTYLE");
  writer->WriteInt16(280, GetInteger("$JOINSTYLE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$LWDISPLAY");
  writer->WriteInt16(290, GetInteger("$LWDISPLAY", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$INSUNITS");
  writer->WriteInt16(70, GetInteger("$INSUNITS", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$HYPERLINKBASE");
  if (GetString("$HYPERLINKBASE", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$STYLESHEET");
  if (GetString("$STYLESHEET", &variantString)) {
    WriteUtf8String(writer, 1, variantString, version);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$XEDIT");
  writer->WriteInt16(290, GetInteger("$XEDIT", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$CEPSNTYPE");
  writer->WriteInt16(380, GetInteger("$CEPSNTYPE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$PSTYLEMODE");
  writer->WriteInt16(290, GetInteger("$PSTYLEMODE", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$EXTNAMES");
  writer->WriteInt16(290, GetInteger("$EXTNAMES", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$PSVPSCALE");
  writer->WriteDouble(40, GetDouble("$PSVPSCALE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$OLESTARTUP");
  writer->WriteInt16(290, GetInteger("$OLESTARTUP", &variantInteger) ? variantInteger : 0);
}

void DRW_Header::WriteAC1018Additions(dxfWriter* writer, DRW::Version version) {
  int variantInteger;
  std::string variantString;

  writer->WriteString(9, "$SORTENTS");
  writer->WriteInt16(280, GetInteger("$SORTENTS", &variantInteger) ? variantInteger : 127);

  writer->WriteString(9, "$INDEXCTL");
  writer->WriteInt16(280, GetInteger("$INDEXCTL", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$HIDETEXT");
  writer->WriteInt16(280, GetInteger("$HIDETEXT", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$XCLIPFRAME");
  if (version > DRW::Version::AC1021) {
    if (GetInteger("$XCLIPFRAME", &variantInteger)) {
      writer->WriteInt16(280, variantInteger);
    } else {
      writer->WriteInt16(280, 0);
    }
  } else {
    if (GetInteger("$XCLIPFRAME", &variantInteger)) {
      writer->WriteInt16(290, variantInteger);
    } else {
      writer->WriteInt16(290, 0);
    }
  }

  writer->WriteString(9, "$HALOGAP");
  writer->WriteInt16(280, GetInteger("$HALOGAP", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$OBSCOLOR");
  writer->WriteInt16(70, GetInteger("$OBSCOLOR", &variantInteger) ? variantInteger : 257);

  writer->WriteString(9, "$OBSLTYPE");
  writer->WriteInt16(280, GetInteger("$OBSLTYPE", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$INTERSECTIONDISPLAY");
  writer->WriteInt16(280, GetInteger("$INTERSECTIONDISPLAY", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$INTERSECTIONCOLOR");
  writer->WriteInt16(70, GetInteger("$INTERSECTIONCOLOR", &variantInteger) ? variantInteger : 257);

  writer->WriteString(9, "$DIMASSOC");
  writer->WriteInt16(280, GetInteger("$DIMASSOC", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$PROJECTNAME");
  if (GetString("$PROJECTNAME", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }
}

void DRW_Header::WriteAC1021Additions(dxfWriter* writer, DRW::Version version) {
  double variantDouble;
  int variantInteger;
  std::string variantString;

  writer->WriteString(9, "$DIMFXL");
  writer->WriteDouble(40, GetDouble("$DIMFXL", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DIMFXLON");
  writer->WriteInt16(70, GetInteger("$DIMFXLON", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMJOGANG");
  writer->WriteDouble(40, GetDouble("$DIMJOGANG", &variantDouble) ? variantDouble : 0.7854);

  writer->WriteString(9, "$DIMTFILL");
  writer->WriteInt16(70, GetInteger("$DIMTFILL", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMTFILLCLR");
  writer->WriteInt16(70, GetInteger("$DIMTFILLCLR", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMARCSYM");
  writer->WriteInt16(70, GetInteger("$DIMARCSYM", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$DIMLTYPE");
  if (GetString("$DIMLTYPE", &variantString)) {
    writer->WriteUtf8String(6, variantString);
  } else {
    writer->WriteString(6, "");
  }

  writer->WriteString(9, "$DIMLTEX1");
  if (GetString("$DIMLTEX1", &variantString)) {
    writer->WriteUtf8String(6, variantString);
  } else {
    writer->WriteString(6, "");
  }

  writer->WriteString(9, "$DIMLTEX2");
  if (GetString("$DIMLTEX2", &variantString)) {
    writer->WriteUtf8String(6, variantString);
  } else {
    writer->WriteString(6, "");
  }

  writer->WriteString(9, "$CAMERADISPLAY");
  writer->WriteInt16(290, GetInteger("$CAMERADISPLAY", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$LENSLENGTH");
  writer->WriteDouble(40, GetDouble("$LENSLENGTH", &variantDouble) ? variantDouble : 50.0);

  writer->WriteString(9, "$CAMERAHEIGHT");
  writer->WriteDouble(40, GetDouble("$CAMERAHEIGHT", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$STEPSPERSEC");
  writer->WriteDouble(40, GetDouble("$STEPSPERSEC", &variantDouble) ? variantDouble : 2.0);

  writer->WriteString(9, "$STEPSIZE");
  writer->WriteDouble(40, GetDouble("$STEPSIZE", &variantDouble) ? variantDouble : 50.0);

  writer->WriteString(9, "$3DDWFPREC");
  writer->WriteDouble(40, GetDouble("$3DDWFPREC", &variantDouble) ? variantDouble : 2.0);

  writer->WriteString(9, "$PSOLWIDTH");
  writer->WriteDouble(40, GetDouble("$PSOLWIDTH", &variantDouble) ? variantDouble : 5.0);

  writer->WriteString(9, "$PSOLHEIGHT");
  writer->WriteDouble(40, GetDouble("$PSOLHEIGHT", &variantDouble) ? variantDouble : 80.0);

  writer->WriteString(9, "$LOFTANG1");
  writer->WriteDouble(40, GetDouble("$LOFTANG1", &variantDouble) ? variantDouble : DRW::HalfPi);

  writer->WriteString(9, "$LOFTANG2");
  writer->WriteDouble(40, GetDouble("$LOFTANG2", &variantDouble) ? variantDouble : DRW::HalfPi);

  writer->WriteString(9, "$LOFTMAG1");
  writer->WriteDouble(40, GetDouble("$LOFTMAG1", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$LOFTMAG2");
  writer->WriteDouble(40, GetDouble("$LOFTMAG2", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$LOFTPARAM");
  writer->WriteInt16(70, GetInteger("$LOFTPARAM", &variantInteger) ? variantInteger : 7);

  writer->WriteString(9, "$LOFTNORMALS");
  writer->WriteInt16(280, GetInteger("$LOFTNORMALS", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$LATITUDE");
  writer->WriteDouble(40, GetDouble("$LATITUDE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$LONGITUDE");
  writer->WriteDouble(40, GetDouble("$LONGITUDE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$NORTHDIRECTION");
  writer->WriteDouble(40, GetDouble("$NORTHDIRECTION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$TIMEZONE");
  writer->WriteInt16(70, GetInteger("$TIMEZONE", &variantInteger) ? variantInteger : -8000);

  writer->WriteString(9, "$LIGHTGLYPHDISPLAY");
  writer->WriteInt16(280, GetInteger("$LIGHTGLYPHDISPLAY", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$TILEMODELIGHTSYNCH");
  writer->WriteInt16(280, GetInteger("$TILEMODELIGHTSYNCH", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$SOLIDHIST");
  writer->WriteInt16(280, GetInteger("$SOLIDHIST", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$SHOWHIST");
  writer->WriteInt16(280, GetInteger("$SHOWHIST", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$DWFFRAME");
  writer->WriteInt16(280, GetInteger("$DWFFRAME", &variantInteger) ? variantInteger : 2);

  writer->WriteString(9, "$DGNFRAME");
  writer->WriteInt16(280, GetInteger("$DGNFRAME", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$REALWORLDSCALE");
  writer->WriteInt16(290, GetInteger("$REALWORLDSCALE", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$INTERFERECOLOR");
  writer->WriteInt16(62, GetInteger("$INTERFERECOLOR", &variantInteger) ? variantInteger : 1);

  writer->WriteString(9, "$CSHADOW");
  writer->WriteInt16(280, GetInteger("$CSHADOW", &variantInteger) ? variantInteger : 0);

  writer->WriteString(9, "$SHADOWPLANELOCATION");
  writer->WriteDouble(40, GetDouble("$SHADOWPLANELOCATION", &variantDouble) ? variantDouble : 0.0);
}

void DRW_Header::WriteAC1024Additions(dxfWriter* writer, DRW::Version version) {
  int variantInteger;

  writer->WriteString(9, "$DIMTXTDIRECTION");
  writer->WriteInt16(70, GetInteger("$DIMTXTDIRECTION", &variantInteger) ? variantInteger : 0);
}

void DRW_Header::Write(dxfWriter* writer, DRW::Version version) {
  int variantInteger;
  std::string variantString;
  DRW_Coord variantCoord;
  writer->WriteString(2, "HEADER");
  writer->WriteString(9, "$ACADVER");
  switch (version) {
    case DRW::Version::AC1006:  // R10 (not supported) [1988]
    case DRW::Version::AC1009:  // R11 [1990] & R12 [1992]
      variantString = "AC1009";
      break;
    case DRW::Version::AC1012:  // R13 (not supported)
    case DRW::Version::AC1014:  // R14
      variantString = "AC1014";
      break;
    case DRW::Version::AC1015:  // AutoCAD 2000 / 2000i / 2002
      variantString = "AC1015";
      break;
    case DRW::Version::AC1018:  // AutoCAD 2004 / 2005 / 2006
      variantString = "AC1018";
      break;
    case DRW::Version::AC1024:  // AutoCAD 2010 / 2011 / 2012
      variantString = "AC1024";
      break;
    case DRW::Version::AC1027:  // AutoCAD 2013 / 2014 / 2015 / 2016 / 2017
      variantString = "AC1027";
      break;
    case DRW::Version::AC1032:  // AutoCAD 2018 / 2019 / 2020 / 2021 / 2022 / 2023 / 2024 / 2025 / 2026
      variantString = "AC1032";
      break;
    case DRW::Version::AC1021:  // AutoCAD 2007 / 2008 / 2009
      [[fallthrough]];          // intentional fallthrough to default case
    default:
      variantString = "AC1021";
      break;
  }
  writer->WriteString(1, variantString);
  writer->SetVersion(&variantString, true);

  (void)GetString("$ACADVER", &variantString);
  (void)GetString("$ACADMAINTVER", &variantString);

  if (!GetString("$DWGCODEPAGE", &variantString)) { variantString = "ANSI_1252"; }
  writer->WriteString(9, "$DWGCODEPAGE");
  writer->SetCodePage(&variantString);
  writer->WriteString(3, writer->GetCodePage());

  writer->WriteString(9, "$HANDSEED");
  writer->WriteString(5, "20000");

  if (GetInteger("$GRIDMODE", &variantInteger)) {
    writer->WriteString(9, "$GRIDMODE");
    writer->WriteInt16(70, variantInteger);
  }

  if (GetInteger("$SNAPSTYLE", &variantInteger)) {
    writer->WriteString(9, "$SNAPSTYLE");
    writer->WriteInt16(70, variantInteger);
  }

  if (GetCoord("$GRIDUNIT", &variantCoord)) {
    writer->WriteString(9, "$GRIDUNIT");
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
  }

  if (GetCoord("$VIEWCTR", &variantCoord)) {
    writer->WriteString(9, "$VIEWCTR");
    writer->WriteDouble(10, variantCoord.x);
    writer->WriteDouble(20, variantCoord.y);
  }

  WriteBase(writer, version);
  WriteAC1009Additions(writer, version);
  WriteAC1012Additions(writer, version);
  WriteAC1014Additions(writer, version);
  WriteAC1015Additions(writer, version);
  WriteAC1018Additions(writer, version);
  WriteAC1021Additions(writer, version);
  WriteAC1024Additions(writer, version);

  if (version < DRW::Version::AC1015) {  // so only write them if the version is AC1014 or earlier
    writer->WriteString(9, "$DRAGMODE");
    writer->WriteInt16(70, GetInteger("$DRAGMODE", &variantInteger) ? variantInteger : 2);

    writer->WriteString(9, "$OSMODE");
    writer->WriteInt16(70, GetInteger("$OSMODE", &variantInteger) ? variantInteger : 0);

    writer->WriteString(9, "$COORDS");
    writer->WriteInt16(70, GetInteger("$COORDS", &variantInteger) ? variantInteger : 2);

    writer->WriteString(9, "$ATTDIA");
    writer->WriteInt16(70, GetInteger("$ATTDIA", &variantInteger) ? variantInteger : 1);

    writer->WriteString(9, "$ATTREQ");
    writer->WriteInt16(70, GetInteger("$ATTREQ", &variantInteger) ? variantInteger : 1);

    writer->WriteString(9, "$BLIPMODE");
    writer->WriteInt16(70, GetInteger("$BLIPMODE", &variantInteger) ? variantInteger : 0);

    // but not written in AC1004 (which is not supported)
    writer->WriteString(9, "$HANDLING");
    writer->WriteInt16(70, GetInteger("$HANDLING", &variantInteger) ? variantInteger : 1);
  }

#ifdef DRW_DBG
  std::map<std::string, DRW_Variant*>::const_iterator it;
  for (it = m_variants.begin(); it != m_variants.end(); ++it) {
    DRW_DBG(it->first);
    DRW_DBG("\n");
  }
#endif
}

void DRW_Header::AddDouble(const std::string& key, double value, int code) {
  m_currentVariant = new DRW_Variant(code, value);
  m_variants[key] = m_currentVariant;
}

void DRW_Header::AddInteger(const std::string& key, int value, int code) {
  m_currentVariant = new DRW_Variant(code, value);
  m_variants[key] = m_currentVariant;
}

void DRW_Header::AddString(const std::string& key, std::string value, int code) {
  m_currentVariant = new DRW_Variant(code, value);
  m_variants[key] = m_currentVariant;
}

void DRW_Header::AddCoord(const std::string& key, DRW_Coord value, int code) {
  m_currentVariant = new DRW_Variant(code, value);
  m_variants[key] = m_currentVariant;
}

bool DRW_Header::GetDouble(const std::string& key, double* variantDouble) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second;
    if (variant->type() == DRW_Variant::Type::Double) {
      *variantDouble = variant->content.d;
      delete variant;
      m_variants.erase(it);
      return true;
    }
  }
  // Non-matching type is deliberately left in the map
  return false;
}

bool DRW_Header::GetInteger(const std::string& key, int* variantInteger) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second;
    if (variant->type() == DRW_Variant::Type::Integer) {
      *variantInteger = variant->content.i;
      delete variant;
      m_variants.erase(it);
      return true;
    }
  }
  // Non-Integer variants deliberately left in the map
  return false;
}

bool DRW_Header::GetString(const std::string& key, std::string* variantString) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second;
    if (variant->type() == DRW_Variant::Type::String) {
      if (variant->content.s != nullptr) {
        *variantString = *variant->content.s;
      } else {
        variantString->clear();  // null → empty string (safe default)
      }
      delete variant;
      m_variants.erase(it);
      return true;
    }
  }
  // Non-String variants deliberately left in the map
  return false;
}

bool DRW_Header::GetCoord(const std::string& key, DRW_Coord* variantCoord) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second;
    if (variant->type() == DRW_Variant::Type::Coord) {
      *variantCoord = *variant->content.v;
      delete variant;
      m_variants.erase(it);
      return true;
    }
  }
  // Non-Coord variants deliberately left in the map
  return false;
}

void DRW_Header::ClearVariants() {
  for (std::map<std::string, DRW_Variant*>::iterator it = m_variants.begin(); it != m_variants.end(); ++it)
    delete it->second;

  m_variants.clear();
}
