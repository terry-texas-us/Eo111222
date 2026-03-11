#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "EoDxfBase.h"
#include "EoDxfHeader.h"
#include "EoDxfReader.h"
#include "EoDxfWriter.h"

EoDxfHeader::EoDxfHeader(const EoDxfHeader& other)
    : m_version{other.m_version}, m_comments{other.m_comments}, m_name{other.m_name}, m_currentVariant{nullptr} {
  for (const auto& [key, variant] : other.m_variants) {
    m_variants.emplace(key, std::make_unique<EoDxfGroupCodeValuesVariant>(*variant));
  }
}

EoDxfHeader& EoDxfHeader::operator=(const EoDxfHeader& other) {
  if (this == &other) { return *this; }
  ClearVariants();
  m_version = other.m_version;
  m_comments = other.m_comments;
  m_name = other.m_name;
  m_currentVariant = nullptr;

  for (const auto& [key, variant] : other.m_variants) {
    m_variants.emplace(key, std::make_unique<EoDxfGroupCodeValuesVariant>(*variant));
  }
  return *this;
}

void EoDxfHeader::AddComment(const std::string& comment) {
  if (!m_comments.empty()) { m_comments += '\n'; }
  m_comments += comment;
}

void EoDxfHeader::ParseCode(int code, EoDxfReader* reader) {
  if (code != 9 && m_currentVariant == nullptr) { return; }
  // The header section is structured as a series of variables, each starting with a group code of 9 followed by the
  // variable name as a string. Subsequent group codes and values belong to that variable until the next group code of 9
  // is encountered, which indicates the start of a new variable. Therefore, when group code 9 is encountered, create a
  // new EoDxfGroupCodeValuesVariant for the new variable and store it in the m_variants map using the variable
  // name as the key. For other group codes, add the corresponding value to the current variant being processed.

  switch (code) {
    case 1:
      m_currentVariant->AddString(code, reader->GetUtf8String());
      if (m_name == "$ACADVER") {
        reader->SetVersion(m_currentVariant->GetString(), true);
        m_version = reader->GetVersion();
      }
      break;
    case 2:
      m_currentVariant->AddString(code, reader->GetUtf8String());
      break;
    case 3:
      m_currentVariant->AddString(code, reader->GetUtf8String());
      if (m_name == "$DWGCODEPAGE") {
        reader->SetCodePage(m_currentVariant->GetString());
        m_currentVariant->AddString(code, reader->GetCodePage());
      }
      break;
    case 5: {
      // Group code 5 is a handle — a hexadecimal string representing a unique identifier for an object in the
      // DXF file. Convert the hex string to a uint64_t and store as a Handle variant for proper numeric access.
      const auto& hexStr = reader->GetString();
      std::uint64_t handle{};
      std::istringstream iss(hexStr);
      iss >> std::hex >> handle;
      m_currentVariant->AddHandle(code, handle);
    } break;
    case 6:
      m_currentVariant->AddString(code, reader->GetUtf8String());
      break;
    case 7:
      m_currentVariant->AddString(code, reader->GetUtf8String());
      break;
    case 8:
      m_currentVariant->AddString(code, reader->GetUtf8String());
      break;
    case 9: {
      auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>();
      m_currentVariant = newVariant.get();
      m_name = reader->GetString();
      if (m_version < static_cast<int>(EoDxf::Version::AC1015) && m_name == "$DIMUNIT") { m_name = "$DIMLUNIT"; }
      m_variants[m_name] = std::move(newVariant);
    } break;
    case 10:
      m_currentVariant->AddGeometryBase(code, EoDxfGeometryBase3d(reader->GetDouble(), 0.0, 0.0));
      break;
    case 20:
      if (m_currentVariant->GetIf<EoDxfGeometryBase3d>() != nullptr) { m_currentVariant->SetGeometryBaseY(reader->GetDouble()); }
      break;
    case 30:
      if (m_currentVariant->GetIf<EoDxfGeometryBase3d>() != nullptr) { m_currentVariant->SetGeometryBaseZ(reader->GetDouble()); }
      break;
    case 40:  // distance, scale and factors are stored as doubles in drawing units
      m_currentVariant->AddDouble(code, reader->GetDouble());
      break;
    case 50:  // angle is stored as doubles in radians
      m_currentVariant->AddDouble(code, reader->GetDouble());
      break;
    case 62:
      m_currentVariant->AddInt16(code, reader->GetInt16());
      break;
    case 70:  // 16-bit integer / bit flags — stored as Int16 in the variant to preserve the DXF-specified type.
      m_currentVariant->AddInt16(code, reader->GetInt16());
      break;
    case 280:  // 8-bit flag / small int — DXF spec groups 280-289 as 16-bit integer range.
      m_currentVariant->AddInt16(code, reader->GetInt16());
      break;
    case 290:  // boolean (0/1) — DXF spec groups 290-299 as a dedicated boolean type.
      m_currentVariant->AddBoolean(code, reader->GetBool());
      break;
    case 345:  // hard-pointer handle reference — convert hex string to uint64_t Handle variant.
      [[fallthrough]];
    case 346:
      [[fallthrough]];
    case 349: {
      const auto& hexStr = reader->GetString();
      std::uint64_t handle{};
      std::istringstream iss(hexStr);
      iss >> std::hex >> handle;
      m_currentVariant->AddHandle(code, handle);
    } break;
    case 370:
      m_currentVariant->AddInt16(code, reader->GetInt16());
      break;
    case 380:
      m_currentVariant->AddInt16(code, reader->GetInt16());
      break;
    case 390:
      m_currentVariant->AddString(code, reader->GetUtf8String());
      break;
    default:
      // Unrecognized group code for header variable; ignore (and/or log) it and continue parsing.
      break;
  }
}

void EoDxfHeader::WriteBase(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  std::string variantString;
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteString(9, "$INSBASE");
  if (GetGeometryBase("$INSBASE", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$EXTMIN");
  if (GetGeometryBase("$EXTMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 1.0000000000000000E+020);
    writer->WriteDouble(20, 1.0000000000000000E+020);
    writer->WriteDouble(30, 1.0000000000000000E+020);
  }

  writer->WriteString(9, "$EXTMAX");
  if (GetGeometryBase("$EXTMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, -1.0000000000000000E+020);
    writer->WriteDouble(20, -1.0000000000000000E+020);
    writer->WriteDouble(30, -1.0000000000000000E+020);
  }

  writer->WriteString(9, "$LIMMIN");
  if (GetGeometryBase("$LIMMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }

  writer->WriteString(9, "$LIMMAX");
  if (GetGeometryBase("$LIMMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 420.0);
    writer->WriteDouble(20, 297.0);
  }

  writer->WriteString(9, "$ORTHOMODE");
  writer->WriteInt16(70, GetInt16("$ORTHOMODE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$REGENMODE");
  writer->WriteInt16(70, GetInt16("$REGENMODE", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$FILLMODE");
  writer->WriteInt16(70, GetInt16("$FILLMODE", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$QTEXTMODE");
  writer->WriteInt16(70, GetInt16("$QTEXTMODE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$MIRRTEXT");
  writer->WriteInt16(70, GetInt16("$MIRRTEXT", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$LTSCALE");
  writer->WriteDouble(40, GetDouble("$LTSCALE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$ATTMODE");
  writer->WriteInt16(70, GetInt16("$ATTMODE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$TEXTSIZE");
  writer->WriteDouble(40, GetDouble("$TEXTSIZE", &variantDouble) ? variantDouble : 2.5);

  writer->WriteString(9, "$TRACEWID");
  writer->WriteDouble(40, GetDouble("$TRACEWID", &variantDouble) ? variantDouble : 15.68);

  writer->WriteString(9, "$TEXTSTYLE");
  if (GetString("$TEXTSTYLE", &variantString)) {
    writer->WriteUtf8String(7, variantString);
  } else {
    writer->WriteString(7, "STANDARD");
  }

  writer->WriteString(9, "$CLAYER");
  if (GetString("$CLAYER", &variantString)) {
    writer->WriteUtf8String(8, variantString);
  } else {
    writer->WriteString(8, "0");
  }

  writer->WriteString(9, "$CELTYPE");
  if (GetString("$CELTYPE", &variantString)) {
    writer->WriteUtf8String(6, variantString);
  } else {
    writer->WriteString(6, "BYLAYER");
  }

  writer->WriteString(9, "$CECOLOR");
  writer->WriteInt16(62, GetInt16("$CECOLOR", &variantInt16) ? variantInt16 : 256);

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
  writer->WriteInt16(70, GetInt16("$DIMTOL", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMLIM");
  writer->WriteInt16(70, GetInt16("$DIMLIM", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMTIH");
  writer->WriteInt16(70, GetInt16("$DIMTIH", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMTOH");
  writer->WriteInt16(70, GetInt16("$DIMTOH", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMSE1");
  writer->WriteInt16(70, GetInt16("$DIMSE1", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMSE2");
  writer->WriteInt16(70, GetInt16("$DIMSE2", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMTAD");
  writer->WriteInt16(70, GetInt16("$DIMTAD", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$DIMZIN");
  writer->WriteInt16(70, GetInt16("$DIMZIN", &variantInt16) ? variantInt16 : 8);

  writer->WriteString(9, "$DIMBLK");
  if (GetString("$DIMBLK", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMASO");
  writer->WriteInt16(70, GetInt16("$DIMASO", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$DIMSHO");
  writer->WriteInt16(70, GetInt16("$DIMSHO", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$DIMPOST");
  if (GetString("$DIMPOST", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMAPOST");
  if (GetString("$DIMAPOST", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMALT");
  writer->WriteInt16(70, GetInt16("$DIMALT", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMALTD");
  writer->WriteInt16(70, GetInt16("$DIMALTD", &variantInt16) ? variantInt16 : 3);

  writer->WriteString(9, "$DIMALTF");
  writer->WriteDouble(40, GetDouble("$DIMALTF", &variantDouble) ? variantDouble : 0.03937);

  writer->WriteString(9, "$DIMLFAC");
  writer->WriteDouble(40, GetDouble("$DIMLFAC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DIMTOFL");
  writer->WriteInt16(70, GetInt16("$DIMTOFL", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$DIMTVP");
  writer->WriteDouble(40, GetDouble("$DIMTVP", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMTIX");
  writer->WriteInt16(70, GetInt16("$DIMTIX", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMSOXD");
  writer->WriteInt16(70, GetInt16("$DIMSOXD", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMSAH");
  writer->WriteInt16(70, GetInt16("$DIMSAH", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMBLK1");
  if (GetString("$DIMBLK1", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$DIMBLK2");
  if (GetString("$DIMBLK2", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$LUNITS");
  writer->WriteInt16(70, GetInt16("$LUNITS", &variantInt16) ? variantInt16 : 2);

  writer->WriteString(9, "$LUPREC");
  writer->WriteInt16(70, GetInt16("$LUPREC", &variantInt16) ? variantInt16 : 4);

  writer->WriteString(9, "$SKETCHINC");
  writer->WriteDouble(40, GetDouble("$SKETCHINC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$FILLETRAD");
  writer->WriteDouble(40, GetDouble("$FILLETRAD", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$AUNITS");
  writer->WriteInt16(70, GetInt16("$AUNITS", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$AUPREC");
  writer->WriteInt16(70, GetInt16("$AUPREC", &variantInt16) ? variantInt16 : 2);

  writer->WriteString(9, "$MENU");
  if (GetString("$MENU", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, ".");
  }

  writer->WriteString(9, "$ELEVATION");
  writer->WriteDouble(40, GetDouble("$ELEVATION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$THICKNESS");
  writer->WriteDouble(40, GetDouble("$THICKNESS", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$LIMCHECK");
  writer->WriteInt16(70, GetInt16("$LIMCHECK", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$CHAMFERA");
  writer->WriteDouble(40, GetDouble("$CHAMFERA", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$CHAMFERB");
  writer->WriteDouble(40, GetDouble("$CHAMFERB", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$SKPOLY");
  writer->WriteInt16(70, GetInt16("$SKPOLY", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$USRTIMER");
  writer->WriteInt16(70, GetInt16("$USRTIMER", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$ANGBASE");
  writer->WriteDouble(50, GetDouble("$ANGBASE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$ANGDIR");
  writer->WriteInt16(70, GetInt16("$ANGDIR", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$PDMODE");
  writer->WriteInt16(70, GetInt16("$PDMODE", &variantInt16) ? variantInt16 : 34);

  writer->WriteString(9, "$PDSIZE");
  writer->WriteDouble(40, GetDouble("$PDSIZE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$PLINEWID");
  writer->WriteDouble(40, GetDouble("$PLINEWID", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$SPLFRAME");
  writer->WriteInt16(70, GetInt16("$SPLFRAME", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$SPLINETYPE");
  writer->WriteInt16(70, GetInt16("$SPLINETYPE", &variantInt16) ? variantInt16 : 2);

  writer->WriteString(9, "$SPLINESEGS");
  writer->WriteInt16(70, GetInt16("$SPLINESEGS", &variantInt16) ? variantInt16 : 8);

  writer->WriteString(9, "$SURFTAB1");
  writer->WriteInt16(70, GetInt16("$SURFTAB1", &variantInt16) ? variantInt16 : 6);

  writer->WriteString(9, "$SURFTAB2");
  writer->WriteInt16(70, GetInt16("$SURFTAB2", &variantInt16) ? variantInt16 : 6);

  writer->WriteString(9, "$SURFTYPE");
  writer->WriteInt16(70, GetInt16("$SURFTYPE", &variantInt16) ? variantInt16 : 6);

  writer->WriteString(9, "$SURFU");
  writer->WriteInt16(70, GetInt16("$SURFU", &variantInt16) ? variantInt16 : 6);

  writer->WriteString(9, "$SURFV");
  writer->WriteInt16(70, GetInt16("$SURFV", &variantInt16) ? variantInt16 : 6);

  writer->WriteString(9, "$UCSNAME");
  if (GetString("$UCSNAME", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$UCSORG");
  if (GetGeometryBase("$UCSORG", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSXDIR");
  if (GetGeometryBase("$UCSXDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSYDIR");
  if (GetGeometryBase("$UCSYDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$USERI1");
  writer->WriteInt16(70, GetInt16("$USERI1", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$USERI2");
  writer->WriteInt16(70, GetInt16("$USERI2", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$USERI3");
  writer->WriteInt16(70, GetInt16("$USERI3", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$USERI4");
  writer->WriteInt16(70, GetInt16("$USERI4", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$USERI5");
  writer->WriteInt16(70, GetInt16("$USERI5", &variantInt16) ? variantInt16 : 0);

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
  writer->WriteInt16(70, GetInt16("$WORLDVIEW", &variantInt16) ? variantInt16 : 1);
}

void EoDxfHeader::WriteAC1009Additions(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  std::string variantString;
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteString(9, "$DIMSTYLE");  // not used before AC1009
  if (GetString("$DIMSTYLE", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "STANDARD");
  }

  writer->WriteString(9, "$DIMCLRD");
  writer->WriteInt16(70, GetInt16("$DIMCLRD", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMCLRE");
  writer->WriteInt16(70, GetInt16("$DIMCLRE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMCLRT");
  writer->WriteInt16(70, GetInt16("$DIMCLRT", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMTFAC");
  writer->WriteDouble(40, GetDouble("$DIMTFAC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DIMGAP");
  writer->WriteDouble(40, GetDouble("$DIMGAP", &variantDouble) ? variantDouble : 0.625);

  writer->WriteString(9, "$PELEVATION");
  writer->WriteDouble(40, GetDouble("$PELEVATION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$PUCSNAME");
  if (GetString("$PUCSNAME", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSORG");
  if (GetGeometryBase("$PUCSORG", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSXDIR");
  if (GetGeometryBase("$PUCSXDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSYDIR");
  if (GetGeometryBase("$PUCSYDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$SHADEDGE");
  writer->WriteInt16(70, GetInt16("$SHADEDGE", &variantInt16) ? variantInt16 : 3);

  writer->WriteString(9, "$SHADEDIF");
  writer->WriteInt16(70, GetInt16("$SHADEDIF", &variantInt16) ? variantInt16 : 70);

  writer->WriteString(9, "$TILEMODE");
  writer->WriteInt16(70, GetInt16("$TILEMODE", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$MAXACTVP");
  writer->WriteInt16(70, GetInt16("$MAXACTVP", &variantInt16) ? variantInt16 : 64);

  writer->WriteString(9, "$PLIMCHECK");
  writer->WriteInt16(70, GetInt16("$PLIMCHECK", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$PEXTMIN");
  if (GetGeometryBase("$PEXTMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PEXTMAX");
  if (GetGeometryBase("$PEXTMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PLIMMIN");
  if (GetGeometryBase("$PLIMMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }

  writer->WriteString(9, "$PLIMMAX");
  if (GetGeometryBase("$PLIMMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 297.0);
    writer->WriteDouble(20, 210.0);
  }

  writer->WriteString(9, "$UNITMODE");
  if (GetInt16("$UNITMODE", &variantInt16)) {
    writer->WriteInt16(70, variantInt16);
  } else {
    writer->WriteInt16(70, 0);
  }

  writer->WriteString(9, "$VISRETAIN");
  writer->WriteInt16(70, GetInt16("$VISRETAIN", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$PLINEGEN");
  writer->WriteInt16(70, GetInt16("$PLINEGEN", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$PSLTSCALE");
  writer->WriteInt16(70, GetInt16("$PSLTSCALE", &variantInt16) ? variantInt16 : 1);
}

void EoDxfHeader::WriteAC1012Additions(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  std::string variantString;
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteString(9, "$CELTSCALE");
  writer->WriteDouble(40, GetDouble("$CELTSCALE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DISPSILH");
  writer->WriteInt16(70, GetInt16("$DISPSILH", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMJUST");
  writer->WriteInt16(70, GetInt16("$DIMJUST", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMSD1");
  writer->WriteInt16(70, GetInt16("$DIMSD1", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMSD2");
  writer->WriteInt16(70, GetInt16("$DIMSD2", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMTOLJ");
  writer->WriteInt16(70, GetInt16("$DIMTOLJ", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMTZIN");
  writer->WriteInt16(70, GetInt16("$DIMTZIN", &variantInt16) ? variantInt16 : 8);

  writer->WriteString(9, "$DIMALTZ");
  writer->WriteInt16(70, GetInt16("$DIMALTZ", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMALTTZ");
  writer->WriteInt16(70, GetInt16("$DIMALTTZ", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMUPT");
  writer->WriteInt16(70, GetInt16("$DIMUPT", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMDEC");
  writer->WriteInt16(70, GetInt16("$DIMDEC", &variantInt16) ? variantInt16 : 2);

  writer->WriteString(9, "$DIMTDEC");
  writer->WriteInt16(70, GetInt16("$DIMTDEC", &variantInt16) ? variantInt16 : 2);

  writer->WriteString(9, "$DIMALTU");
  writer->WriteInt16(70, GetInt16("$DIMALTU", &variantInt16) ? variantInt16 : 2);

  writer->WriteString(9, "$DIMALTTD");
  writer->WriteInt16(70, GetInt16("$DIMALTTD", &variantInt16) ? variantInt16 : 3);

  writer->WriteString(9, "$DIMTXSTY");
  if (GetString("$DIMTXSTY", &variantString)) {
    writer->WriteUtf8String(7, variantString);
  } else {
    writer->WriteString(7, "STANDARD");
  }

  writer->WriteString(9, "$DIMAUNIT");
  writer->WriteInt16(70, GetInt16("$DIMAUNIT", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$CHAMFERC");
  writer->WriteDouble(40, GetDouble("$CHAMFERC", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$CHAMFERD");
  writer->WriteDouble(40, GetDouble("$CHAMFERD", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$PINSBASE");
  if (GetGeometryBase("$PINSBASE", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$TREEDEPTH");
  writer->WriteInt16(70, GetInt16("$TREEDEPTH", &variantInt16) ? variantInt16 : 3020);

  writer->WriteString(9, "$CMLSTYLE");
  if (GetString("$CMLSTYLE", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "Standard");
  }

  writer->WriteString(9, "$CMLJUST");
  writer->WriteInt16(70, GetInt16("$CMLJUST", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$CMLSCALE");
  writer->WriteDouble(40, GetDouble("$CMLSCALE", &variantDouble) ? variantDouble : 20.0);
}

void EoDxfHeader::WriteAC1014Additions(EoDxfWriter* writer) {
  std::int16_t variantInt16;

  writer->WriteString(9, "$PROXYGRAPHICS");
  writer->WriteInt16(70, GetInt16("$PROXYGRAPHICS", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$MEASUREMENT");
  writer->WriteInt16(70, GetInt16("$MEASUREMENT", &variantInt16) ? variantInt16 : 1);
}

void EoDxfHeader::WriteAC1015Additions(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  std::string variantString;
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteString(9, "$DIMADEC");
  writer->WriteInt16(70, GetInt16("$DIMADEC", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMALTRND");
  writer->WriteDouble(40, GetDouble("$DIMALTRND", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$DIMAZIN");
  writer->WriteInt16(70, GetInt16("$DIMAZIN", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMDSEP");
  writer->WriteInt16(70, GetInt16("$DIMDSEP", &variantInt16) ? variantInt16 : 44);

  writer->WriteString(9, "$DIMATFIT");
  writer->WriteInt16(70, GetInt16("$DIMATFIT", &variantInt16) ? variantInt16 : 3);

  writer->WriteString(9, "$DIMFRAC");
  writer->WriteInt16(70, GetInt16("$DIMFRAC", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMLDRBLK");
  if (GetString("$DIMLDRBLK", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "STANDARD");
  }

  // `$DIMLUNIT` replaced `$DIMUNIT` in AC1015, but `$DIMUNIT` may still be present in AC1012 and AC1014 files,
  // so check for both and default to 2 (decimal) if neither is found or if the value is out of range
  if (!GetInt16("$DIMLUNIT", &variantInt16)) {
    if (!GetInt16("$DIMUNIT", &variantInt16)) { variantInt16 = 2; }
  }
  if (variantInt16 < 1 || variantInt16 > 6) { variantInt16 = 2; }

  writer->WriteString(9, "$DIMLUNIT");
  writer->WriteInt16(70, variantInt16);

  writer->WriteString(9, "$DIMLWD");
  writer->WriteInt16(70, GetInt16("$DIMLWD", &variantInt16) ? variantInt16 : -2);

  writer->WriteString(9, "$DIMLWE");
  writer->WriteInt16(70, GetInt16("$DIMLWE", &variantInt16) ? variantInt16 : -2);

  writer->WriteString(9, "$DIMTMOVE");
  writer->WriteInt16(70, GetInt16("$DIMTMOVE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$UCSORTHOREF");
  if (GetString("$UCSORTHOREF", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$UCSORTHOVIEW");
  writer->WriteInt16(70, GetInt16("$UCSORTHOVIEW", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$UCSORGTOP");
  if (GetGeometryBase("$UCSORGTOP", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGBOTTOM");
  if (GetGeometryBase("$UCSORGBOTTOM", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGLEFT");
  if (GetGeometryBase("$UCSORGLEFT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGRIGHT");
  if (GetGeometryBase("$UCSORGRIGHT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGFRONT");
  if (GetGeometryBase("$UCSORGFRONT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSORGBACK");
  if (GetGeometryBase("$UCSORGBACK", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$UCSBASE");
  if (GetString("$UCSBASE", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSBASE");
  if (GetString("$PUCSBASE", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSORTHOREF");
  if (GetString("$PUCSORTHOREF", &variantString)) {
    writer->WriteUtf8String(2, variantString);
  } else {
    writer->WriteString(2, "");
  }

  writer->WriteString(9, "$PUCSORTHOVIEW");
  writer->WriteInt16(70, GetInt16("$PUCSORTHOVIEW", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$PUCSORGTOP");
  if (GetGeometryBase("$PUCSORGTOP", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGBOTTOM");
  if (GetGeometryBase("$PUCSORGBOTTOM", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGLEFT");
  if (GetGeometryBase("$PUCSORGLEFT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGRIGHT");
  if (GetGeometryBase("$PUCSORGRIGHT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGFRONT");
  if (GetGeometryBase("$PUCSORGFRONT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$PUCSORGBACK");
  if (GetGeometryBase("$PUCSORGBACK", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteString(9, "$CELWEIGHT");
  writer->WriteInt16(370, GetInt16("$CELWEIGHT", &variantInt16) ? variantInt16 : -1);

  writer->WriteString(9, "$ENDCAPS");
  writer->WriteInt16(280, GetInt16("$ENDCAPS", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$JOINSTYLE");
  writer->WriteInt16(280, GetInt16("$JOINSTYLE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$LWDISPLAY");
  writer->WriteInt16(290, GetInt16("$LWDISPLAY", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$INSUNITS");
  writer->WriteInt16(70, GetInt16("$INSUNITS", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$HYPERLINKBASE");
  if (GetString("$HYPERLINKBASE", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$STYLESHEET");
  if (GetString("$STYLESHEET", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }

  writer->WriteString(9, "$XEDIT");
  writer->WriteInt16(290, GetInt16("$XEDIT", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$CEPSNTYPE");
  writer->WriteInt16(380, GetInt16("$CEPSNTYPE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$PSTYLEMODE");
  writer->WriteInt16(290, GetInt16("$PSTYLEMODE", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$EXTNAMES");
  writer->WriteInt16(290, GetInt16("$EXTNAMES", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$PSVPSCALE");
  writer->WriteDouble(40, GetDouble("$PSVPSCALE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$OLESTARTUP");
  writer->WriteInt16(290, GetInt16("$OLESTARTUP", &variantInt16) ? variantInt16 : 0);
}

void EoDxfHeader::WriteAC1018Additions(EoDxfWriter* writer, EoDxf::Version version) {
  std::int16_t variantInt16;
  std::string variantString;

  writer->WriteString(9, "$SORTENTS");
  writer->WriteInt16(280, GetInt16("$SORTENTS", &variantInt16) ? variantInt16 : 127);

  writer->WriteString(9, "$INDEXCTL");
  writer->WriteInt16(280, GetInt16("$INDEXCTL", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$HIDETEXT");
  writer->WriteInt16(280, GetInt16("$HIDETEXT", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$XCLIPFRAME");
  if (version > EoDxf::Version::AC1021) {
    if (GetInt16("$XCLIPFRAME", &variantInt16)) {
      writer->WriteInt16(280, variantInt16);
    } else {
      writer->WriteInt16(280, 0);
    }
  } else {
    if (GetInt16("$XCLIPFRAME", &variantInt16)) {
      writer->WriteInt16(290, variantInt16);
    } else {
      writer->WriteInt16(290, 0);
    }
  }

  writer->WriteString(9, "$HALOGAP");
  writer->WriteInt16(280, GetInt16("$HALOGAP", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$OBSCOLOR");
  writer->WriteInt16(70, GetInt16("$OBSCOLOR", &variantInt16) ? variantInt16 : 257);

  writer->WriteString(9, "$OBSLTYPE");
  writer->WriteInt16(280, GetInt16("$OBSLTYPE", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$INTERSECTIONDISPLAY");
  writer->WriteInt16(280, GetInt16("$INTERSECTIONDISPLAY", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$INTERSECTIONCOLOR");
  writer->WriteInt16(70, GetInt16("$INTERSECTIONCOLOR", &variantInt16) ? variantInt16 : 257);

  writer->WriteString(9, "$DIMASSOC");
  writer->WriteInt16(280, GetInt16("$DIMASSOC", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$PROJECTNAME");
  if (GetString("$PROJECTNAME", &variantString)) {
    writer->WriteUtf8String(1, variantString);
  } else {
    writer->WriteString(1, "");
  }
}

void EoDxfHeader::WriteAC1021Additions(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  std::string variantString;

  writer->WriteString(9, "$DIMFXL");
  writer->WriteDouble(40, GetDouble("$DIMFXL", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$DIMFXLON");
  writer->WriteInt16(70, GetInt16("$DIMFXLON", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMJOGANG");
  writer->WriteDouble(40, GetDouble("$DIMJOGANG", &variantDouble) ? variantDouble : 0.7854);

  writer->WriteString(9, "$DIMTFILL");
  writer->WriteInt16(70, GetInt16("$DIMTFILL", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMTFILLCLR");
  writer->WriteInt16(70, GetInt16("$DIMTFILLCLR", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$DIMARCSYM");
  writer->WriteInt16(70, GetInt16("$DIMARCSYM", &variantInt16) ? variantInt16 : 0);

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
  writer->WriteInt16(290, GetInt16("$CAMERADISPLAY", &variantInt16) ? variantInt16 : 0);

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
  writer->WriteDouble(40, GetDouble("$LOFTANG1", &variantDouble) ? variantDouble : EoDxf::HalfPi);

  writer->WriteString(9, "$LOFTANG2");
  writer->WriteDouble(40, GetDouble("$LOFTANG2", &variantDouble) ? variantDouble : EoDxf::HalfPi);

  writer->WriteString(9, "$LOFTMAG1");
  writer->WriteDouble(40, GetDouble("$LOFTMAG1", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$LOFTMAG2");
  writer->WriteDouble(40, GetDouble("$LOFTMAG2", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$LOFTPARAM");
  writer->WriteInt16(70, GetInt16("$LOFTPARAM", &variantInt16) ? variantInt16 : 7);

  writer->WriteString(9, "$LOFTNORMALS");
  writer->WriteInt16(280, GetInt16("$LOFTNORMALS", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$LATITUDE");
  writer->WriteDouble(40, GetDouble("$LATITUDE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$LONGITUDE");
  writer->WriteDouble(40, GetDouble("$LONGITUDE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteString(9, "$NORTHDIRECTION");
  writer->WriteDouble(40, GetDouble("$NORTHDIRECTION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteString(9, "$TIMEZONE");
  writer->WriteInt16(70, GetInt16("$TIMEZONE", &variantInt16) ? variantInt16 : -8000);

  writer->WriteString(9, "$LIGHTGLYPHDISPLAY");
  writer->WriteInt16(280, GetInt16("$LIGHTGLYPHDISPLAY", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$TILEMODELIGHTSYNCH");
  writer->WriteInt16(280, GetInt16("$TILEMODELIGHTSYNCH", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$SOLIDHIST");
  writer->WriteInt16(280, GetInt16("$SOLIDHIST", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$SHOWHIST");
  writer->WriteInt16(280, GetInt16("$SHOWHIST", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$DWFFRAME");
  writer->WriteInt16(280, GetInt16("$DWFFRAME", &variantInt16) ? variantInt16 : 2);

  writer->WriteString(9, "$DGNFRAME");
  writer->WriteInt16(280, GetInt16("$DGNFRAME", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$REALWORLDSCALE");
  writer->WriteInt16(290, GetInt16("$REALWORLDSCALE", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$INTERFERECOLOR");
  writer->WriteInt16(62, GetInt16("$INTERFERECOLOR", &variantInt16) ? variantInt16 : 1);

  writer->WriteString(9, "$CSHADOW");
  writer->WriteInt16(280, GetInt16("$CSHADOW", &variantInt16) ? variantInt16 : 0);

  writer->WriteString(9, "$SHADOWPLANELOCATION");
  writer->WriteDouble(40, GetDouble("$SHADOWPLANELOCATION", &variantDouble) ? variantDouble : 0.0);
}

void EoDxfHeader::WriteAC1024Additions(EoDxfWriter* writer) {
  std::int16_t variantInt16;

  writer->WriteString(9, "$DIMTXTDIRECTION");
  writer->WriteInt16(70, GetInt16("$DIMTXTDIRECTION", &variantInt16) ? variantInt16 : 0);
}

void EoDxfHeader::Write(EoDxfWriter* writer, EoDxf::Version version) {
  std::int16_t variantInt16;
  std::string variantString;
  EoDxfGeometryBase3d variantGeometryBase;
  writer->WriteString(2, "HEADER");
  writer->WriteString(9, "$ACADVER");
  switch (version) {
    case EoDxf::Version::AC1006:  // R10 (not supported) [1988]
    case EoDxf::Version::AC1009:  // R11 [1990] & R12 [1992]
      variantString = "AC1009";
      break;
    case EoDxf::Version::AC1012:  // R13 (not supported)
    case EoDxf::Version::AC1014:  // R14
      variantString = "AC1014";
      break;
    case EoDxf::Version::AC1015:  // AutoCAD 2000 / 2000i / 2002
      variantString = "AC1015";
      break;
    case EoDxf::Version::AC1018:  // AutoCAD 2004 / 2005 / 2006
      variantString = "AC1018";
      break;
    case EoDxf::Version::AC1024:  // AutoCAD 2010 / 2011 / 2012
      variantString = "AC1024";
      break;
    case EoDxf::Version::AC1027:  // AutoCAD 2013 / 2014 / 2015 / 2016 / 2017
      variantString = "AC1027";
      break;
    case EoDxf::Version::AC1032:  // AutoCAD 2018 / 2019 / 2020 / 2021 / 2022 / 2023 / 2024 / 2025 / 2026
      variantString = "AC1032";
      break;
    case EoDxf::Version::AC1021:  // AutoCAD 2007 / 2008 / 2009
      [[fallthrough]];  // intentional fallthrough to default case
    default:
      variantString = "AC1021";
      break;
  }
  writer->WriteString(1, variantString);
  writer->SetVersion(variantString, true);

  (void)GetString("$ACADVER", &variantString);
  (void)GetString("$ACADMAINTVER", &variantString);

  if (!GetString("$DWGCODEPAGE", &variantString)) { variantString = "ANSI_1252"; }
  writer->WriteString(9, "$DWGCODEPAGE");
  writer->SetCodePage(variantString);
  writer->WriteString(3, writer->GetCodePage());

  writer->WriteString(9, "$HANDSEED");
  std::uint64_t variantHandle;
  if (GetHandle("$HANDSEED", &variantHandle)) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << variantHandle;
    writer->WriteString(5, oss.str());
  } else {
    writer->WriteString(5, "20000");
  }

  if (GetInt16("$GRIDMODE", &variantInt16)) {
    writer->WriteString(9, "$GRIDMODE");
    writer->WriteInt16(70, variantInt16);
  }

  if (GetInt16("$SNAPSTYLE", &variantInt16)) {
    writer->WriteString(9, "$SNAPSTYLE");
    writer->WriteInt16(70, variantInt16);
  }

  if (GetGeometryBase("$GRIDUNIT", &variantGeometryBase)) {
    writer->WriteString(9, "$GRIDUNIT");
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  }

  if (GetGeometryBase("$VIEWCTR", &variantGeometryBase)) {
    writer->WriteString(9, "$VIEWCTR");
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  }

  WriteBase(writer);

  if (version >= EoDxf::Version::AC1009) { WriteAC1009Additions(writer); }
  if (version >= EoDxf::Version::AC1012) { WriteAC1012Additions(writer); }
  if (version >= EoDxf::Version::AC1014) { WriteAC1014Additions(writer); }
  if (version >= EoDxf::Version::AC1015) { WriteAC1015Additions(writer); }
  if (version >= EoDxf::Version::AC1018) { WriteAC1018Additions(writer, version); }
  if (version >= EoDxf::Version::AC1021) { WriteAC1021Additions(writer); }
  if (version >= EoDxf::Version::AC1024) { WriteAC1024Additions(writer); }

  if (version < EoDxf::Version::AC1015) {  // so only write them if the version is AC1014 or earlier
    writer->WriteString(9, "$DRAGMODE");
    writer->WriteInt16(70, GetInt16("$DRAGMODE", &variantInt16) ? variantInt16 : 2);

    writer->WriteString(9, "$OSMODE");
    writer->WriteInt16(70, GetInt16("$OSMODE", &variantInt16) ? variantInt16 : 0);

    writer->WriteString(9, "$COORDS");
    writer->WriteInt16(70, GetInt16("$COORDS", &variantInt16) ? variantInt16 : 2);

    writer->WriteString(9, "$ATTDIA");
    writer->WriteInt16(70, GetInt16("$ATTDIA", &variantInt16) ? variantInt16 : 1);

    writer->WriteString(9, "$ATTREQ");
    writer->WriteInt16(70, GetInt16("$ATTREQ", &variantInt16) ? variantInt16 : 1);

    writer->WriteString(9, "$BLIPMODE");
    writer->WriteInt16(70, GetInt16("$BLIPMODE", &variantInt16) ? variantInt16 : 0);

    // but not written in AC1004 (which is not supported)
    writer->WriteString(9, "$HANDLING");
    writer->WriteInt16(70, GetInt16("$HANDLING", &variantInt16) ? variantInt16 : 1);
  }
}

void EoDxfHeader::AddDouble(const std::string& key, double value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[key] = std::move(newVariant);
}

void EoDxfHeader::AddInt32(const std::string& key, std::int32_t value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[key] = std::move(newVariant);
}

void EoDxfHeader::AddString(const std::string& key, std::string value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, std::move(value));
  m_currentVariant = newVariant.get();
  m_variants[key] = std::move(newVariant);
}

void EoDxfHeader::AddGeometryBase(const std::string& key, EoDxfGeometryBase3d value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[key] = std::move(newVariant);
}

void EoDxfHeader::AddInt16(const std::string& key, std::int16_t value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[key] = std::move(newVariant);
}

void EoDxfHeader::AddHandle(const std::string& key, std::uint64_t value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[key] = std::move(newVariant);
}

bool EoDxfHeader::GetDouble(const std::string& key, double* variantDouble) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<double>()) {
      *variantDouble = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  return false;
}

bool EoDxfHeader::GetInt16(const std::string& key, std::int16_t* variantInt16) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<std::int16_t>()) {
      *variantInt16 = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  return false;
}

bool EoDxfHeader::GetInt32(const std::string& key, std::int32_t* variantInteger) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<std::int32_t>()) {
      *variantInteger = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
    // Int16 is widened to int for backward compatibility with callers that
    // do not yet distinguish 16-bit vs 32-bit integer group codes.
    if (const auto* value = variant->GetIf<std::int16_t>()) {
      *variantInteger = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
    if (const auto* value = variant->GetIf<bool>()) {
      *variantInteger = *value ? 1 : 0;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  // Non-Integer/Int16 variants deliberately left in the map
  return false;
}

bool EoDxfHeader::GetString(const std::string& key, std::string* variantString) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<std::string>()) {
      *variantString = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  // Non-String variants deliberately left in the map
  return false;
}

bool EoDxfHeader::GetGeometryBase(const std::string& key, EoDxfGeometryBase3d* variantGeometryBase) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<EoDxfGeometryBase3d>()) {
      *variantGeometryBase = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  // Non-GeometryBase variants deliberately left in the map
  return false;
}

bool EoDxfHeader::GetHandle(const std::string& key, std::uint64_t* varHandle) {
  if (auto it = m_variants.find(key); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<std::uint64_t>()) {
      *varHandle = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  // Non-Handle variants deliberately left in the map
  return false;
}

void EoDxfHeader::ClearVariants() {
  m_variants.clear();
}
