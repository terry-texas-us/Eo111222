#include <Windows.h>

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include "EoDxfBase.h"
#include "EoDxfHeader.h"
#include "EoDxfReader.h"
#include "EoDxfWriter.h"

namespace {

[[nodiscard]] std::wstring Utf8ToWideText(const std::string_view text) {
  if (text.empty()) { return {}; }

  const auto inputLength = static_cast<int>(text.size());
  const auto requiredLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), inputLength, nullptr, 0);
  if (requiredLength <= 0) { return {}; }

  std::wstring wideText(static_cast<std::size_t>(requiredLength), L'\0');
  const auto convertedLength =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), inputLength, wideText.data(), requiredLength);
  if (convertedLength != requiredLength) { return {}; }

  return wideText;
}

}  // namespace

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

void EoDxfHeader::AddComment(const std::wstring& comment) {
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
      m_currentVariant->AddWideString(code, reader->GetWideString());
      if (m_name == L"$ACADVER") {
        reader->SetVersion(m_currentVariant->GetWideString());
        m_version = reader->GetVersionEnum();
      }
      break;
    case 2:
      m_currentVariant->AddWideString(code, reader->GetWideString());
      break;
    case 3:
      m_currentVariant->AddWideString(code, reader->GetWideString());
      if (m_name == L"$DWGCODEPAGE") {
        reader->SetCodePage(m_currentVariant->GetWideString());
        m_currentVariant->AddWideString(code, reader->GetCodePage());
      }
      break;
    case 5: {
      // Group code 5 is a handle — a hexadecimal string representing a unique identifier for an object in the
      // DXF file. Convert the hex string to a uint64_t and store as a Handle variant for proper numeric access.
      m_currentVariant->AddHandle(code, reader->GetHandleString());
    } break;
    case 6:
      m_currentVariant->AddWideString(code, reader->GetWideString());
      break;
    case 7:
      m_currentVariant->AddWideString(code, reader->GetWideString());
      break;
    case 8:
      m_currentVariant->AddWideString(code, reader->GetWideString());
      break;
    case 9: {
      auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>();
      m_currentVariant = newVariant.get();
      m_name = reader->GetWideString();
      if (m_version < EoDxf::Version::AC1015 && m_name == L"$DIMUNIT") { m_name = L"$DIMLUNIT"; }
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
      m_currentVariant->AddHandle(code, reader->GetHandleString());
    } break;
    case 370:
      m_currentVariant->AddInt16(code, reader->GetInt16());
      break;
    case 380:
      m_currentVariant->AddInt16(code, reader->GetInt16());
      break;
    case 390:
      m_currentVariant->AddWideString(code, reader->GetWideString());
      break;
    default:
      // Unrecognized group code for header variable; ignore (and/or log) it and continue parsing.
      break;
  }
}

void EoDxfHeader::WriteBase(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteWideString(9, L"$INSBASE");
  if (GetGeometryBase(L"$INSBASE", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$EXTMIN");
  if (GetGeometryBase(L"$EXTMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 1.0000000000000000E+020);
    writer->WriteDouble(20, 1.0000000000000000E+020);
    writer->WriteDouble(30, 1.0000000000000000E+020);
  }

  writer->WriteWideString(9, L"$EXTMAX");
  if (GetGeometryBase(L"$EXTMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, -1.0000000000000000E+020);
    writer->WriteDouble(20, -1.0000000000000000E+020);
    writer->WriteDouble(30, -1.0000000000000000E+020);
  }

  writer->WriteWideString(9, L"$LIMMIN");
  if (GetGeometryBase(L"$LIMMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }

  writer->WriteWideString(9, L"$LIMMAX");
  if (GetGeometryBase(L"$LIMMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 420.0);
    writer->WriteDouble(20, 297.0);
  }

  writer->WriteWideString(9, L"$ORTHOMODE");
  writer->WriteInt16(70, GetInt16(L"$ORTHOMODE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$REGENMODE");
  writer->WriteInt16(70, GetInt16(L"$REGENMODE", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$FILLMODE");
  writer->WriteInt16(70, GetInt16(L"$FILLMODE", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$QTEXTMODE");
  writer->WriteInt16(70, GetInt16(L"$QTEXTMODE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$MIRRTEXT");
  writer->WriteInt16(70, GetInt16(L"$MIRRTEXT", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$LTSCALE");
  writer->WriteDouble(40, GetDouble(L"$LTSCALE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$ATTMODE");
  writer->WriteInt16(70, GetInt16(L"$ATTMODE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$TEXTSIZE");
  writer->WriteDouble(40, GetDouble(L"$TEXTSIZE", &variantDouble) ? variantDouble : 2.5);

  writer->WriteWideString(9, L"$TRACEWID");
  writer->WriteDouble(40, GetDouble(L"$TRACEWID", &variantDouble) ? variantDouble : 15.68);

  writer->WriteWideString(9, L"$TEXTSTYLE");
  WriteStoredWideString(writer, L"$TEXTSTYLE", 7, L"STANDARD");

  writer->WriteWideString(9, L"$CLAYER");
  WriteStoredWideString(writer, L"$CLAYER", 8, L"0");

  writer->WriteWideString(9, L"$CELTYPE");
  WriteStoredWideString(writer, L"$CELTYPE", 6, L"BYLAYER");

  writer->WriteWideString(9, L"$CECOLOR");
  writer->WriteInt16(62, GetInt16(L"$CECOLOR", &variantInt16) ? variantInt16 : 256);

  writer->WriteWideString(9, L"$DIMSCALE");
  writer->WriteDouble(40, GetDouble(L"$DIMSCALE", &variantDouble) ? variantDouble : 2.5);

  writer->WriteWideString(9, L"$DIMASZ");
  writer->WriteDouble(40, GetDouble(L"$DIMASZ", &variantDouble) ? variantDouble : 2.5);

  writer->WriteWideString(9, L"$DIMEXO");
  writer->WriteDouble(40, GetDouble(L"$DIMEXO", &variantDouble) ? variantDouble : 0.625);

  writer->WriteWideString(9, L"$DIMDLI");
  writer->WriteDouble(40, GetDouble(L"$DIMDLI", &variantDouble) ? variantDouble : 3.75);

  writer->WriteWideString(9, L"$DIMRND");
  writer->WriteDouble(40, GetDouble(L"$DIMRND", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$DIMDLE");
  writer->WriteDouble(40, GetDouble(L"$DIMDLE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$DIMEXE");
  writer->WriteDouble(40, GetDouble(L"$DIMEXE", &variantDouble) ? variantDouble : 1.25);

  writer->WriteWideString(9, L"$DIMTP");
  writer->WriteDouble(40, GetDouble(L"$DIMTP", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$DIMTM");
  writer->WriteDouble(40, GetDouble(L"$DIMTM", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$DIMTXT");
  writer->WriteDouble(40, GetDouble(L"$DIMTXT", &variantDouble) ? variantDouble : 2.5);

  writer->WriteWideString(9, L"$DIMCEN");
  writer->WriteDouble(40, GetDouble(L"$DIMCEN", &variantDouble) ? variantDouble : 2.5);

  writer->WriteWideString(9, L"$DIMTSZ");
  writer->WriteDouble(40, GetDouble(L"$DIMTSZ", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$DIMTOL");
  writer->WriteInt16(70, GetInt16(L"$DIMTOL", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMLIM");
  writer->WriteInt16(70, GetInt16(L"$DIMLIM", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMTIH");
  writer->WriteInt16(70, GetInt16(L"$DIMTIH", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMTOH");
  writer->WriteInt16(70, GetInt16(L"$DIMTOH", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMSE1");
  writer->WriteInt16(70, GetInt16(L"$DIMSE1", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMSE2");
  writer->WriteInt16(70, GetInt16(L"$DIMSE2", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMTAD");
  writer->WriteInt16(70, GetInt16(L"$DIMTAD", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$DIMZIN");
  writer->WriteInt16(70, GetInt16(L"$DIMZIN", &variantInt16) ? variantInt16 : 8);

  writer->WriteWideString(9, L"$DIMBLK");
  WriteStoredWideString(writer, L"$DIMBLK", 1, L"");

  writer->WriteWideString(9, L"$DIMASO");
  writer->WriteInt16(70, GetInt16(L"$DIMASO", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$DIMSHO");
  writer->WriteInt16(70, GetInt16(L"$DIMSHO", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$DIMPOST");
  WriteStoredWideString(writer, L"$DIMPOST", 1, L"");

  writer->WriteWideString(9, L"$DIMAPOST");
  WriteStoredWideString(writer, L"$DIMAPOST", 1, L"");

  writer->WriteWideString(9, L"$DIMALT");
  writer->WriteInt16(70, GetInt16(L"$DIMALT", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMALTD");
  writer->WriteInt16(70, GetInt16(L"$DIMALTD", &variantInt16) ? variantInt16 : 3);

  writer->WriteWideString(9, L"$DIMALTF");
  writer->WriteDouble(40, GetDouble(L"$DIMALTF", &variantDouble) ? variantDouble : 0.03937);

  writer->WriteWideString(9, L"$DIMLFAC");
  writer->WriteDouble(40, GetDouble(L"$DIMLFAC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$DIMTOFL");
  writer->WriteInt16(70, GetInt16(L"$DIMTOFL", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$DIMTVP");
  writer->WriteDouble(40, GetDouble(L"$DIMTVP", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$DIMTIX");
  writer->WriteInt16(70, GetInt16(L"$DIMTIX", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMSOXD");
  writer->WriteInt16(70, GetInt16(L"$DIMSOXD", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMSAH");
  writer->WriteInt16(70, GetInt16(L"$DIMSAH", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMBLK1");
  WriteStoredWideString(writer, L"$DIMBLK1", 1, L"");

  writer->WriteWideString(9, L"$DIMBLK2");
  WriteStoredWideString(writer, L"$DIMBLK2", 1, L"");

  writer->WriteWideString(9, L"$LUNITS");
  writer->WriteInt16(70, GetInt16(L"$LUNITS", &variantInt16) ? variantInt16 : 2);

  writer->WriteWideString(9, L"$LUPREC");
  writer->WriteInt16(70, GetInt16(L"$LUPREC", &variantInt16) ? variantInt16 : 4);

  writer->WriteWideString(9, L"$SKETCHINC");
  writer->WriteDouble(40, GetDouble(L"$SKETCHINC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$FILLETRAD");
  writer->WriteDouble(40, GetDouble(L"$FILLETRAD", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$AUNITS");
  writer->WriteInt16(70, GetInt16(L"$AUNITS", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$AUPREC");
  writer->WriteInt16(70, GetInt16(L"$AUPREC", &variantInt16) ? variantInt16 : 2);

  writer->WriteWideString(9, L"$MENU");
  WriteStoredWideString(writer, L"$MENU", 1, L".");

  writer->WriteWideString(9, L"$ELEVATION");
  writer->WriteDouble(40, GetDouble(L"$ELEVATION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$THICKNESS");
  writer->WriteDouble(40, GetDouble(L"$THICKNESS", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$LIMCHECK");
  writer->WriteInt16(70, GetInt16(L"$LIMCHECK", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$CHAMFERA");
  writer->WriteDouble(40, GetDouble(L"$CHAMFERA", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$CHAMFERB");
  writer->WriteDouble(40, GetDouble(L"$CHAMFERB", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$SKPOLY");
  writer->WriteInt16(70, GetInt16(L"$SKPOLY", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$USRTIMER");
  writer->WriteInt16(70, GetInt16(L"$USRTIMER", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$ANGBASE");
  writer->WriteDouble(50, GetDouble(L"$ANGBASE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$ANGDIR");
  writer->WriteInt16(70, GetInt16(L"$ANGDIR", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$PDMODE");
  writer->WriteInt16(70, GetInt16(L"$PDMODE", &variantInt16) ? variantInt16 : 34);

  writer->WriteWideString(9, L"$PDSIZE");
  writer->WriteDouble(40, GetDouble(L"$PDSIZE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$PLINEWID");
  writer->WriteDouble(40, GetDouble(L"$PLINEWID", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$SPLFRAME");
  writer->WriteInt16(70, GetInt16(L"$SPLFRAME", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$SPLINETYPE");
  writer->WriteInt16(70, GetInt16(L"$SPLINETYPE", &variantInt16) ? variantInt16 : 2);

  writer->WriteWideString(9, L"$SPLINESEGS");
  writer->WriteInt16(70, GetInt16(L"$SPLINESEGS", &variantInt16) ? variantInt16 : 8);

  writer->WriteWideString(9, L"$SURFTAB1");
  writer->WriteInt16(70, GetInt16(L"$SURFTAB1", &variantInt16) ? variantInt16 : 6);

  writer->WriteWideString(9, L"$SURFTAB2");
  writer->WriteInt16(70, GetInt16(L"$SURFTAB2", &variantInt16) ? variantInt16 : 6);

  writer->WriteWideString(9, L"$SURFTYPE");
  writer->WriteInt16(70, GetInt16(L"$SURFTYPE", &variantInt16) ? variantInt16 : 6);

  writer->WriteWideString(9, L"$SURFU");
  writer->WriteInt16(70, GetInt16(L"$SURFU", &variantInt16) ? variantInt16 : 6);

  writer->WriteWideString(9, L"$SURFV");
  writer->WriteInt16(70, GetInt16(L"$SURFV", &variantInt16) ? variantInt16 : 6);

  writer->WriteWideString(9, L"$UCSNAME");
  WriteStoredWideString(writer, L"$UCSNAME", 2, L"");

  writer->WriteWideString(9, L"$UCSORG");
  if (GetGeometryBase(L"$UCSORG", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSXDIR");
  if (GetGeometryBase(L"$UCSXDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSYDIR");
  if (GetGeometryBase(L"$UCSYDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$USERI1");
  writer->WriteInt16(70, GetInt16(L"$USERI1", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$USERI2");
  writer->WriteInt16(70, GetInt16(L"$USERI2", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$USERI3");
  writer->WriteInt16(70, GetInt16(L"$USERI3", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$USERI4");
  writer->WriteInt16(70, GetInt16(L"$USERI4", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$USERI5");
  writer->WriteInt16(70, GetInt16(L"$USERI5", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$USERR1");
  writer->WriteDouble(40, GetDouble(L"$USERR1", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$USERR2");
  writer->WriteDouble(40, GetDouble(L"$USERR2", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$USERR3");
  writer->WriteDouble(40, GetDouble(L"$USERR3", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$USERR4");
  writer->WriteDouble(40, GetDouble(L"$USERR4", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$USERR5");
  writer->WriteDouble(40, GetDouble(L"$USERR5", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$WORLDVIEW");
  writer->WriteInt16(70, GetInt16(L"$WORLDVIEW", &variantInt16) ? variantInt16 : 1);
}

void EoDxfHeader::WriteAC1009Additions(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteWideString(9, L"$DIMSTYLE");  // not used before AC1009
  WriteStoredWideString(writer, L"$DIMSTYLE", 2, L"STANDARD");

  writer->WriteWideString(9, L"$DIMCLRD");
  writer->WriteInt16(70, GetInt16(L"$DIMCLRD", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMCLRE");
  writer->WriteInt16(70, GetInt16(L"$DIMCLRE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMCLRT");
  writer->WriteInt16(70, GetInt16(L"$DIMCLRT", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMTFAC");
  writer->WriteDouble(40, GetDouble(L"$DIMTFAC", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$DIMGAP");
  writer->WriteDouble(40, GetDouble(L"$DIMGAP", &variantDouble) ? variantDouble : 0.625);

  writer->WriteWideString(9, L"$PELEVATION");
  writer->WriteDouble(40, GetDouble(L"$PELEVATION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$PUCSNAME");
  WriteStoredWideString(writer, L"$PUCSNAME", 2, L"");

  writer->WriteWideString(9, L"$PUCSORG");
  if (GetGeometryBase(L"$PUCSORG", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PUCSXDIR");
  if (GetGeometryBase(L"$PUCSXDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 1.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PUCSYDIR");
  if (GetGeometryBase(L"$PUCSYDIR", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 1.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$SHADEDGE");
  writer->WriteInt16(70, GetInt16(L"$SHADEDGE", &variantInt16) ? variantInt16 : 3);

  writer->WriteWideString(9, L"$SHADEDIF");
  writer->WriteInt16(70, GetInt16(L"$SHADEDIF", &variantInt16) ? variantInt16 : 70);

  writer->WriteWideString(9, L"$TILEMODE");
  writer->WriteInt16(70, GetInt16(L"$TILEMODE", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$MAXACTVP");
  writer->WriteInt16(70, GetInt16(L"$MAXACTVP", &variantInt16) ? variantInt16 : 64);

  writer->WriteWideString(9, L"$PLIMCHECK");
  writer->WriteInt16(70, GetInt16(L"$PLIMCHECK", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$PEXTMIN");
  if (GetGeometryBase(L"$PEXTMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PEXTMAX");
  if (GetGeometryBase(L"$PEXTMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PLIMMIN");
  if (GetGeometryBase(L"$PLIMMIN", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
  }

  writer->WriteWideString(9, L"$PLIMMAX");
  if (GetGeometryBase(L"$PLIMMAX", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  } else {
    writer->WriteDouble(10, 297.0);
    writer->WriteDouble(20, 210.0);
  }

  writer->WriteWideString(9, L"$UNITMODE");
  if (GetInt16(L"$UNITMODE", &variantInt16)) {
    writer->WriteInt16(70, variantInt16);
  } else {
    writer->WriteInt16(70, 0);
  }

  writer->WriteWideString(9, L"$VISRETAIN");
  writer->WriteInt16(70, GetInt16(L"$VISRETAIN", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$PLINEGEN");
  writer->WriteInt16(70, GetInt16(L"$PLINEGEN", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$PSLTSCALE");
  writer->WriteInt16(70, GetInt16(L"$PSLTSCALE", &variantInt16) ? variantInt16 : 1);
}

void EoDxfHeader::WriteAC1012Additions(EoDxfWriter* writer) {
  double variantDouble;
  std::int16_t variantInt16;
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteWideString(9, L"$CELTSCALE");
  writer->WriteDouble(40, GetDouble(L"$CELTSCALE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$DISPSILH");
  writer->WriteInt16(70, GetInt16(L"$DISPSILH", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMJUST");
  writer->WriteInt16(70, GetInt16(L"$DIMJUST", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMSD1");
  writer->WriteInt16(70, GetInt16(L"$DIMSD1", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMSD2");
  writer->WriteInt16(70, GetInt16(L"$DIMSD2", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMTOLJ");
  writer->WriteInt16(70, GetInt16(L"$DIMTOLJ", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMTZIN");
  writer->WriteInt16(70, GetInt16(L"$DIMTZIN", &variantInt16) ? variantInt16 : 8);

  writer->WriteWideString(9, L"$DIMALTZ");
  writer->WriteInt16(70, GetInt16(L"$DIMALTZ", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMALTTZ");
  writer->WriteInt16(70, GetInt16(L"$DIMALTTZ", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMUPT");
  writer->WriteInt16(70, GetInt16(L"$DIMUPT", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMDEC");
  writer->WriteInt16(70, GetInt16(L"$DIMDEC", &variantInt16) ? variantInt16 : 2);

  writer->WriteWideString(9, L"$DIMTDEC");
  writer->WriteInt16(70, GetInt16(L"$DIMTDEC", &variantInt16) ? variantInt16 : 2);

  writer->WriteWideString(9, L"$DIMALTU");
  writer->WriteInt16(70, GetInt16(L"$DIMALTU", &variantInt16) ? variantInt16 : 2);

  writer->WriteWideString(9, L"$DIMALTTD");
  writer->WriteInt16(70, GetInt16(L"$DIMALTTD", &variantInt16) ? variantInt16 : 3);

  writer->WriteWideString(9, L"$DIMTXSTY");
  WriteStoredWideString(writer, L"$DIMTXSTY", 7, L"STANDARD");

  writer->WriteWideString(9, L"$DIMAUNIT");
  writer->WriteInt16(70, GetInt16(L"$DIMAUNIT", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$CHAMFERC");
  writer->WriteDouble(40, GetDouble(L"$CHAMFERC", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$CHAMFERD");
  writer->WriteDouble(40, GetDouble(L"$CHAMFERD", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$PINSBASE");
  if (GetGeometryBase(L"$PINSBASE", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$TREEDEPTH");
  writer->WriteInt16(70, GetInt16(L"$TREEDEPTH", &variantInt16) ? variantInt16 : 3020);

  writer->WriteWideString(9, L"$CMLSTYLE");
  WriteStoredWideString(writer, L"$CMLSTYLE", 2, L"Standard");

  writer->WriteWideString(9, L"$CMLJUST");
  writer->WriteInt16(70, GetInt16(L"$CMLJUST", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$CMLSCALE");
  writer->WriteDouble(40, GetDouble(L"$CMLSCALE", &variantDouble) ? variantDouble : 20.0);
}

void EoDxfHeader::WriteAC1014Additions(EoDxfWriter* writer) {
  std::int16_t variantInt16;

  writer->WriteWideString(9, L"$PROXYGRAPHICS");
  writer->WriteInt16(70, GetInt16(L"$PROXYGRAPHICS", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$MEASUREMENT");
  writer->WriteInt16(70, GetInt16(L"$MEASUREMENT", &variantInt16) ? variantInt16 : 1);
}

void EoDxfHeader::WriteAC1015Additions(EoDxfWriter* writer) {
  bool variantBool{};
  double variantDouble{};
  std::int16_t variantInt16{};
  EoDxfGeometryBase3d variantGeometryBase;

  writer->WriteWideString(9, L"$DIMADEC");
  writer->WriteInt16(70, GetInt16(L"$DIMADEC", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMALTRND");
  writer->WriteDouble(40, GetDouble(L"$DIMALTRND", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$DIMAZIN");
  writer->WriteInt16(70, GetInt16(L"$DIMAZIN", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMDSEP");
  writer->WriteInt16(70, GetInt16(L"$DIMDSEP", &variantInt16) ? variantInt16 : 44);

  writer->WriteWideString(9, L"$DIMATFIT");
  writer->WriteInt16(70, GetInt16(L"$DIMATFIT", &variantInt16) ? variantInt16 : 3);

  writer->WriteWideString(9, L"$DIMFRAC");
  writer->WriteInt16(70, GetInt16(L"$DIMFRAC", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMLDRBLK");
  WriteStoredWideString(writer, L"$DIMLDRBLK", 1, L"STANDARD");

  // `$DIMLUNIT` replaced `$DIMUNIT` in AC1015, but `$DIMUNIT` may still be present in AC1012 and AC1014 files,
  // so check for both and default to 2 (decimal) if neither is found or if the value is out of range
  if (!GetInt16(L"$DIMLUNIT", &variantInt16)) {
    if (!GetInt16(L"$DIMUNIT", &variantInt16)) { variantInt16 = 2; }
  }
  if (variantInt16 < 1 || variantInt16 > 6) { variantInt16 = 2; }

  writer->WriteWideString(9, L"$DIMLUNIT");
  writer->WriteInt16(70, variantInt16);

  writer->WriteWideString(9, L"$DIMLWD");
  writer->WriteInt16(70, GetInt16(L"$DIMLWD", &variantInt16) ? variantInt16 : -2);

  writer->WriteWideString(9, L"$DIMLWE");
  writer->WriteInt16(70, GetInt16(L"$DIMLWE", &variantInt16) ? variantInt16 : -2);

  writer->WriteWideString(9, L"$DIMTMOVE");
  writer->WriteInt16(70, GetInt16(L"$DIMTMOVE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$UCSORTHOREF");
  WriteStoredWideString(writer, L"$UCSORTHOREF", 2, L"");

  writer->WriteWideString(9, L"$UCSORTHOVIEW");
  writer->WriteInt16(70, GetInt16(L"$UCSORTHOVIEW", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$UCSORGTOP");
  if (GetGeometryBase(L"$UCSORGTOP", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSORGBOTTOM");
  if (GetGeometryBase(L"$UCSORGBOTTOM", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSORGLEFT");
  if (GetGeometryBase(L"$UCSORGLEFT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSORGRIGHT");
  if (GetGeometryBase(L"$UCSORGRIGHT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSORGFRONT");
  if (GetGeometryBase(L"$UCSORGFRONT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSORGBACK");
  if (GetGeometryBase(L"$UCSORGBACK", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$UCSBASE");
  WriteStoredWideString(writer, L"$UCSBASE", 2, L"");

  writer->WriteWideString(9, L"$PUCSBASE");
  WriteStoredWideString(writer, L"$PUCSBASE", 2, L"");

  writer->WriteWideString(9, L"$PUCSORTHOREF");
  WriteStoredWideString(writer, L"$PUCSORTHOREF", 2, L"");

  writer->WriteWideString(9, L"$PUCSORTHOVIEW");
  writer->WriteInt16(70, GetInt16(L"$PUCSORTHOVIEW", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$PUCSORGTOP");
  if (GetGeometryBase(L"$PUCSORGTOP", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PUCSORGBOTTOM");
  if (GetGeometryBase(L"$PUCSORGBOTTOM", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PUCSORGLEFT");
  if (GetGeometryBase(L"$PUCSORGLEFT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PUCSORGRIGHT");
  if (GetGeometryBase(L"$PUCSORGRIGHT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PUCSORGFRONT");
  if (GetGeometryBase(L"$PUCSORGFRONT", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$PUCSORGBACK");
  if (GetGeometryBase(L"$PUCSORGBACK", &variantGeometryBase)) {
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
    writer->WriteDouble(30, variantGeometryBase.z);
  } else {
    writer->WriteDouble(10, 0.0);
    writer->WriteDouble(20, 0.0);
    writer->WriteDouble(30, 0.0);
  }

  writer->WriteWideString(9, L"$CELWEIGHT");
  writer->WriteInt16(370, GetInt16(L"$CELWEIGHT", &variantInt16) ? variantInt16 : -1);

  writer->WriteWideString(9, L"$ENDCAPS");
  writer->WriteInt16(280, GetInt16(L"$ENDCAPS", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$JOINSTYLE");
  writer->WriteInt16(280, GetInt16(L"$JOINSTYLE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$LWDISPLAY");
  writer->WriteBool(290, GetBool(L"$LWDISPLAY", &variantBool) ? variantBool : false);

  writer->WriteWideString(9, L"$INSUNITS");
  writer->WriteInt16(70, GetInt16(L"$INSUNITS", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$HYPERLINKBASE");
  WriteStoredWideString(writer, L"$HYPERLINKBASE", 1, L"");

  writer->WriteWideString(9, L"$STYLESHEET");
  WriteStoredWideString(writer, L"$STYLESHEET", 1, L"");

  writer->WriteWideString(9, L"$XEDIT");
  writer->WriteBool(290, GetBool(L"$XEDIT", &variantBool) ? variantBool : true);

  writer->WriteWideString(9, L"$CEPSNTYPE");
  writer->WriteInt16(380, GetInt16(L"$CEPSNTYPE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$PSTYLEMODE");
  writer->WriteBool(290, GetBool(L"$PSTYLEMODE", &variantBool) ? variantBool : true);

  writer->WriteWideString(9, L"$EXTNAMES");
  writer->WriteBool(290, GetBool(L"$EXTNAMES", &variantBool) ? variantBool : true);

  writer->WriteWideString(9, L"$PSVPSCALE");
  writer->WriteDouble(40, GetDouble(L"$PSVPSCALE", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$OLESTARTUP");
  writer->WriteBool(290, GetBool(L"$OLESTARTUP", &variantBool) ? variantBool : false);
}

void EoDxfHeader::WriteAC1018Additions(EoDxfWriter* writer, EoDxf::Version version) {
  bool variantBool{};
  std::int16_t variantInt16{};

  writer->WriteWideString(9, L"$SORTENTS");
  writer->WriteInt16(280, GetInt16(L"$SORTENTS", &variantInt16) ? variantInt16 : 127);

  writer->WriteWideString(9, L"$INDEXCTL");
  writer->WriteInt16(280, GetInt16(L"$INDEXCTL", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$HIDETEXT");
  writer->WriteInt16(280, GetInt16(L"$HIDETEXT", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$XCLIPFRAME");
  if (version > EoDxf::Version::AC1021) {
    if (GetInt16(L"$XCLIPFRAME", &variantInt16)) {
      writer->WriteInt16(280, variantInt16);
    } else {
      writer->WriteInt16(280, 0);
    }
  } else {
    if (GetBool(L"$XCLIPFRAME", &variantBool)) {
      writer->WriteBool(290, variantBool);
    } else {
      writer->WriteBool(290, false);
    }
  }

  writer->WriteWideString(9, L"$HALOGAP");
  writer->WriteInt16(280, GetInt16(L"$HALOGAP", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$OBSCOLOR");
  writer->WriteInt16(70, GetInt16(L"$OBSCOLOR", &variantInt16) ? variantInt16 : 257);

  writer->WriteWideString(9, L"$OBSLTYPE");
  writer->WriteInt16(280, GetInt16(L"$OBSLTYPE", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$INTERSECTIONDISPLAY");
  writer->WriteInt16(280, GetInt16(L"$INTERSECTIONDISPLAY", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$INTERSECTIONCOLOR");
  writer->WriteInt16(70, GetInt16(L"$INTERSECTIONCOLOR", &variantInt16) ? variantInt16 : 257);

  writer->WriteWideString(9, L"$DIMASSOC");
  writer->WriteInt16(280, GetInt16(L"$DIMASSOC", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$PROJECTNAME");
  WriteStoredWideString(writer, L"$PROJECTNAME", 1, L"");
}

void EoDxfHeader::WriteAC1021Additions(EoDxfWriter* writer) {
  bool variantBool{};
  double variantDouble{};
  std::int16_t variantInt16{};

  writer->WriteWideString(9, L"$DIMFXL");
  writer->WriteDouble(40, GetDouble(L"$DIMFXL", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$DIMFXLON");
  writer->WriteInt16(70, GetInt16(L"$DIMFXLON", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMJOGANG");
  writer->WriteDouble(40, GetDouble(L"$DIMJOGANG", &variantDouble) ? variantDouble : 0.7854);

  writer->WriteWideString(9, L"$DIMTFILL");
  writer->WriteInt16(70, GetInt16(L"$DIMTFILL", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMTFILLCLR");
  writer->WriteInt16(70, GetInt16(L"$DIMTFILLCLR", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMARCSYM");
  writer->WriteInt16(70, GetInt16(L"$DIMARCSYM", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$DIMLTYPE");
  WriteStoredWideString(writer, L"$DIMLTYPE", 6, L"");

  writer->WriteWideString(9, L"$DIMLTEX1");
  WriteStoredWideString(writer, L"$DIMLTEX1", 6, L"");

  writer->WriteWideString(9, L"$DIMLTEX2");
  WriteStoredWideString(writer, L"$DIMLTEX2", 6, L"");

  writer->WriteWideString(9, L"$CAMERADISPLAY");
  writer->WriteBool(290, GetBool(L"$CAMERADISPLAY", &variantBool) ? variantBool : false);

  writer->WriteWideString(9, L"$LENSLENGTH");
  writer->WriteDouble(40, GetDouble(L"$LENSLENGTH", &variantDouble) ? variantDouble : 50.0);

  writer->WriteWideString(9, L"$CAMERAHEIGHT");
  writer->WriteDouble(40, GetDouble(L"$CAMERAHEIGHT", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$STEPSPERSEC");
  writer->WriteDouble(40, GetDouble(L"$STEPSPERSEC", &variantDouble) ? variantDouble : 2.0);

  writer->WriteWideString(9, L"$STEPSIZE");
  writer->WriteDouble(40, GetDouble(L"$STEPSIZE", &variantDouble) ? variantDouble : 50.0);

  writer->WriteWideString(9, L"$3DDWFPREC");
  writer->WriteDouble(40, GetDouble(L"$3DDWFPREC", &variantDouble) ? variantDouble : 2.0);

  writer->WriteWideString(9, L"$PSOLWIDTH");
  writer->WriteDouble(40, GetDouble(L"$PSOLWIDTH", &variantDouble) ? variantDouble : 5.0);

  writer->WriteWideString(9, L"$PSOLHEIGHT");
  writer->WriteDouble(40, GetDouble(L"$PSOLHEIGHT", &variantDouble) ? variantDouble : 80.0);

  writer->WriteWideString(9, L"$LOFTANG1");
  writer->WriteDouble(40, GetDouble(L"$LOFTANG1", &variantDouble) ? variantDouble : EoDxf::HalfPi);

  writer->WriteWideString(9, L"$LOFTANG2");
  writer->WriteDouble(40, GetDouble(L"$LOFTANG2", &variantDouble) ? variantDouble : EoDxf::HalfPi);

  writer->WriteWideString(9, L"$LOFTMAG1");
  writer->WriteDouble(40, GetDouble(L"$LOFTMAG1", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$LOFTMAG2");
  writer->WriteDouble(40, GetDouble(L"$LOFTMAG2", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$LOFTPARAM");
  writer->WriteInt16(70, GetInt16(L"$LOFTPARAM", &variantInt16) ? variantInt16 : 7);

  writer->WriteWideString(9, L"$LOFTNORMALS");
  writer->WriteInt16(280, GetInt16(L"$LOFTNORMALS", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$LATITUDE");
  writer->WriteDouble(40, GetDouble(L"$LATITUDE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$LONGITUDE");
  writer->WriteDouble(40, GetDouble(L"$LONGITUDE", &variantDouble) ? variantDouble : 1.0);

  writer->WriteWideString(9, L"$NORTHDIRECTION");
  writer->WriteDouble(40, GetDouble(L"$NORTHDIRECTION", &variantDouble) ? variantDouble : 0.0);

  writer->WriteWideString(9, L"$TIMEZONE");
  writer->WriteInt16(70, GetInt16(L"$TIMEZONE", &variantInt16) ? variantInt16 : -8000);

  writer->WriteWideString(9, L"$LIGHTGLYPHDISPLAY");
  writer->WriteInt16(280, GetInt16(L"$LIGHTGLYPHDISPLAY", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$TILEMODELIGHTSYNCH");
  writer->WriteInt16(280, GetInt16(L"$TILEMODELIGHTSYNCH", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$SOLIDHIST");
  writer->WriteInt16(280, GetInt16(L"$SOLIDHIST", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$SHOWHIST");
  writer->WriteInt16(280, GetInt16(L"$SHOWHIST", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$DWFFRAME");
  writer->WriteInt16(280, GetInt16(L"$DWFFRAME", &variantInt16) ? variantInt16 : 2);

  writer->WriteWideString(9, L"$DGNFRAME");
  writer->WriteInt16(280, GetInt16(L"$DGNFRAME", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$REALWORLDSCALE");
  writer->WriteBool(290, GetBool(L"$REALWORLDSCALE", &variantBool) ? variantBool : true);

  writer->WriteWideString(9, L"$INTERFERECOLOR");
  writer->WriteInt16(62, GetInt16(L"$INTERFERECOLOR", &variantInt16) ? variantInt16 : 1);

  writer->WriteWideString(9, L"$CSHADOW");
  writer->WriteInt16(280, GetInt16(L"$CSHADOW", &variantInt16) ? variantInt16 : 0);

  writer->WriteWideString(9, L"$SHADOWPLANELOCATION");
  writer->WriteDouble(40, GetDouble(L"$SHADOWPLANELOCATION", &variantDouble) ? variantDouble : 0.0);
}

void EoDxfHeader::WriteAC1024Additions(EoDxfWriter* writer) {
  std::int16_t variantInt16;

  writer->WriteWideString(9, L"$DIMTXTDIRECTION");
  writer->WriteInt16(70, GetInt16(L"$DIMTXTDIRECTION", &variantInt16) ? variantInt16 : 0);
}

void EoDxfHeader::Write(EoDxfWriter* writer, EoDxf::Version version) {
  std::int16_t variantInt16;
  std::wstring variantWideString;
  EoDxfGeometryBase3d variantGeometryBase;
  writer->WriteWideString(2, L"HEADER");
  writer->WriteWideString(9, L"$ACADVER");
  switch (version) {
    case EoDxf::Version::AC1006:  // R10 (not supported) [1988]
    case EoDxf::Version::AC1009:  // R11 [1990] & R12 [1992]
      variantWideString = L"AC1009";
      break;
    case EoDxf::Version::AC1012:  // R13 (not supported)
    case EoDxf::Version::AC1014:  // R14
      variantWideString = L"AC1014";
      break;
    case EoDxf::Version::AC1015:  // AutoCAD 2000 / 2000i / 2002
      variantWideString = L"AC1015";
      break;
    case EoDxf::Version::AC1018:  // AutoCAD 2004 / 2005 / 2006
      variantWideString = L"AC1018";
      break;
    case EoDxf::Version::AC1024:  // AutoCAD 2010 / 2011 / 2012
      variantWideString = L"AC1024";
      break;
    case EoDxf::Version::AC1027:  // AutoCAD 2013 / 2014 / 2015 / 2016 / 2017
      variantWideString = L"AC1027";
      break;
    case EoDxf::Version::AC1032:  // AutoCAD 2018 / 2019 / 2020 / 2021 / 2022 / 2023 / 2024 / 2025 / 2026
      variantWideString = L"AC1032";
      break;
    case EoDxf::Version::AC1021:  // AutoCAD 2007 / 2008 / 2009
      [[fallthrough]];  // intentional fallthrough to default case
    default:
      variantWideString = L"AC1021";
      break;
  }
  writer->WriteWideString(1, variantWideString);
  writer->SetVersion(variantWideString);

  if (!GetWideString(L"$DWGCODEPAGE", &variantWideString)) { variantWideString = L"ANSI_1252"; }
  writer->WriteWideString(9, L"$DWGCODEPAGE");
  writer->SetCodePage(variantWideString);
  writer->WriteWideString(3, writer->GetCodePage());

  writer->WriteWideString(9, L"$HANDSEED");
  std::uint64_t variantHandle;
  if (GetHandle(L"$HANDSEED", &variantHandle)) {
    std::wostringstream oss;
    oss << std::uppercase << std::hex << variantHandle;
    writer->WriteWideString(5, oss.str());
  } else {
    writer->WriteWideString(5, L"20000");
  }

  if (GetInt16(L"$GRIDMODE", &variantInt16)) {
    writer->WriteWideString(9, L"$GRIDMODE");
    writer->WriteInt16(70, variantInt16);
  }

  if (GetInt16(L"$SNAPSTYLE", &variantInt16)) {
    writer->WriteWideString(9, L"$SNAPSTYLE");
    writer->WriteInt16(70, variantInt16);
  }

  if (GetGeometryBase(L"$GRIDUNIT", &variantGeometryBase)) {
    writer->WriteWideString(9, L"$GRIDUNIT");
    writer->WriteDouble(10, variantGeometryBase.x);
    writer->WriteDouble(20, variantGeometryBase.y);
  }

  if (GetGeometryBase(L"$VIEWCTR", &variantGeometryBase)) {
    writer->WriteWideString(9, L"$VIEWCTR");
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

  if (version < EoDxf::Version::AC1015) {
    writer->WriteWideString(9, L"$DRAGMODE");
    writer->WriteInt16(70, GetInt16(L"$DRAGMODE", &variantInt16) ? variantInt16 : 2);

    writer->WriteWideString(9, L"$OSMODE");
    writer->WriteInt16(70, GetInt16(L"$OSMODE", &variantInt16) ? variantInt16 : 0);

    writer->WriteWideString(9, L"$COORDS");
    writer->WriteInt16(70, GetInt16(L"$COORDS", &variantInt16) ? variantInt16 : 2);

    writer->WriteWideString(9, L"$ATTDIA");
    writer->WriteInt16(70, GetInt16(L"$ATTDIA", &variantInt16) ? variantInt16 : 1);

    writer->WriteWideString(9, L"$ATTREQ");
    writer->WriteInt16(70, GetInt16(L"$ATTREQ", &variantInt16) ? variantInt16 : 1);

    writer->WriteWideString(9, L"$BLIPMODE");
    writer->WriteInt16(70, GetInt16(L"$BLIPMODE", &variantInt16) ? variantInt16 : 0);

    writer->WriteWideString(9, L"$HANDLING");
    writer->WriteInt16(70, GetInt16(L"$HANDLING", &variantInt16) ? variantInt16 : 1);
  }
}

void EoDxfHeader::AddDouble(std::wstring_view key, double value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[std::wstring{key}] = std::move(newVariant);
}

void EoDxfHeader::AddInt32(std::wstring_view key, std::int32_t value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[std::wstring{key}] = std::move(newVariant);
}

void EoDxfHeader::AddWideString(std::wstring_view key, std::wstring value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, std::move(value));
  m_currentVariant = newVariant.get();
  m_variants[std::wstring{key}] = std::move(newVariant);
}

void EoDxfHeader::AddGeometryBase(std::wstring_view key, EoDxfGeometryBase3d value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[std::wstring{key}] = std::move(newVariant);
}

void EoDxfHeader::AddInt16(std::wstring_view key, std::int16_t value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[std::wstring{key}] = std::move(newVariant);
}

void EoDxfHeader::AddHandle(std::wstring_view key, std::uint64_t value, int code) {
  auto newVariant = std::make_unique<EoDxfGroupCodeValuesVariant>(code, value);
  m_currentVariant = newVariant.get();
  m_variants[std::wstring{key}] = std::move(newVariant);
}

bool EoDxfHeader::GetBool(std::wstring_view key, bool* variantBool) {
  if (auto it = m_variants.find(std::wstring{key}); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<bool>()) {
      *variantBool = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  return false;
}

bool EoDxfHeader::GetDouble(std::wstring_view key, double* variantDouble) {
  if (auto it = m_variants.find(std::wstring{key}); it != m_variants.end()) {
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

bool EoDxfHeader::GetInt16(std::wstring_view key, std::int16_t* variantInt16) {
  if (auto it = m_variants.find(std::wstring{key}); it != m_variants.end()) {
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

bool EoDxfHeader::GetInt32(std::wstring_view key, std::int32_t* variantInteger) {
  if (auto it = m_variants.find(std::wstring{key}); it != m_variants.end()) {
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

bool EoDxfHeader::GetWideString(std::wstring_view key, std::wstring* variantWideString) {
  if (auto it = m_variants.find(std::wstring{key}); it != m_variants.end()) {
    auto* variant = it->second.get();
    if (const auto* value = variant->GetIf<std::wstring>()) {
      *variantWideString = *value;
      if (m_currentVariant == variant) { m_currentVariant = nullptr; }
      m_variants.erase(it);
      return true;
    }
  }
  return false;
}

void EoDxfHeader::WriteStoredWideString(
    EoDxfWriter* writer, std::wstring_view key, const int code, const std::wstring_view defaultValue) {
  std::wstring variantWideString;
  if (GetWideString(key, &variantWideString)) {
    writer->WriteWideString(code, variantWideString);
  } else {
    writer->WriteWideString(code, defaultValue);
  }
}

bool EoDxfHeader::GetGeometryBase(std::wstring_view key, EoDxfGeometryBase3d* variantGeometryBase) {
  if (auto it = m_variants.find(std::wstring{key}); it != m_variants.end()) {
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

bool EoDxfHeader::GetHandle(std::wstring_view key, std::uint64_t* varHandle) {
  if (auto it = m_variants.find(std::wstring{key}); it != m_variants.end()) {
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
