#include "Stdafx.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

#include "AeSys.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbAttrib.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDxfAttributes.h"
#include "EoDxfInterface.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"

EoDbAttrib::EoDbAttrib(const EoDbFontDefinition& fontDefinition, EoGeReferenceSystem& referenceSystem,
    const std::wstring& text, std::wstring tagString, std::int16_t attributeFlags)
    : EoDbText(fontDefinition, referenceSystem, text),
      m_tagString(std::move(tagString)),
      m_attributeFlags(attributeFlags) {}

EoDbAttrib::EoDbAttrib(const EoDbAttrib& other)
    : EoDbText(other),
      m_tagString(other.m_tagString),
      m_attributeFlags(other.m_attributeFlags),
      m_insertHandle(other.m_insertHandle) {}

const EoDbAttrib& EoDbAttrib::operator=(const EoDbAttrib& other) {
  EoDbText::operator=(other);
  m_tagString = other.m_tagString;
  m_attributeFlags = other.m_attributeFlags;
  m_insertHandle = other.m_insertHandle;
  return *this;
}

void EoDbAttrib::AddReportToMessageList(const EoGePoint3d& point) {
  CString label;
  label.Format(L"<Attrib tag='%s' flags=%d insertHandle=%I64X>", m_tagString.c_str(), m_attributeFlags, m_insertHandle);
  app.AddStringToMessageList(label);
  EoDbText::AddReportToMessageList(point);
}

void EoDbAttrib::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label;
  label.Format(L"<Attrib: %s>", m_tagString.c_str());
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbAttrib::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbAttrib(*this);
  return primitive;
}

void EoDbAttrib::FormatExtra(CString& extra) {
  EoDbText::FormatExtra(extra);
  // FormatExtra Terminator Rule: EoDbText::FormatExtra already ends with '\t'.
  // Append attribute-specific Name;Value pairs. Each ends with '\t'.
  extra.AppendFormat(L"Tag;%s\tAttribFlags;%d\tInsertHandle;%I64X\t", m_tagString.c_str(), m_attributeFlags,
      m_insertHandle);
}

void EoDbAttrib::FormatGeometry(CString& str) {
  // Attribute geometry is identical to text geometry; delegate completely.
  EoDbText::FormatGeometry(str);
}

void EoDbAttrib::ExportToDxf(EoDxfInterface* writer) const {
  // When owned by an INSERT, the parent EoDbBlockReference::ExportToDxf handles
  // the INSERT → ATTRIB → SEQEND sequence. Skip to prevent double-export.
  if (m_insertHandle != 0) {
    return;
  }
  // Build an EoDxfAttrib and export it via the writer.
  // This mirrors ConvertAttribEntity in reverse — recovering DXF properties from the reference system
  // using the same logic as EoDbText::ExportToDxf for TEXT entities.
  EoDxfAttrib attrib;
  PopulateDxfBaseProperties(&attrib);

  // Attribute-specific fields
  attrib.m_attributeValue = std::wstring(Text().GetString());
  attrib.m_tagString = m_tagString;
  attrib.m_attributeFlags = m_attributeFlags;

  EoDbFontDefinition fontDef;
  GetFontDef(fontDef);
  attrib.m_textStyleName = std::wstring(fontDef.FontName().GetString());

  // Recover DXF properties from the reference system (same as EoDbText::ExportToDxf)
  EoGeReferenceSystem referenceSystem;
  GetRefSys(referenceSystem);
  const double textHeight = referenceSystem.YDirection().Length();
  attrib.m_textHeight = textHeight;

  const double xDirLength = referenceSystem.XDirection().Length();
  if (textHeight > Eo::geometricTolerance) {
    attrib.m_relativeXScaleFactor = xDirLength / (textHeight * Eo::defaultCharacterCellAspectRatio);
  }

  // Rotation angle from xDirection — DXF ATTRIB group 50 is in degrees
  const auto& xDir = referenceSystem.XDirection();
  attrib.m_textRotation = Eo::RadianToDegree(std::atan2(xDir.y, xDir.x));

  // Recover oblique angle from Y-direction shear
  if (textHeight > Eo::geometricTolerance && xDirLength > Eo::geometricTolerance) {
    const auto xUnit = xDir * (1.0 / xDirLength);
    const auto yUnit = referenceSystem.YDirection() * (1.0 / textHeight);
    const double sinOblique = DotProduct(xUnit, yUnit);
    attrib.m_obliqueAngle = Eo::RadianToDegree(std::asin(std::clamp(sinOblique, -1.0, 1.0)));
  }

  // Map AeSys alignment to DXF alignment — reuse fontDef already populated above

  switch (fontDef.HorizontalAlignment()) {
    case EoDb::HorizontalAlignment::Center:
      attrib.m_horizontalTextJustification = 1;
      break;
    case EoDb::HorizontalAlignment::Right:
      attrib.m_horizontalTextJustification = 2;
      break;
    case EoDb::HorizontalAlignment::Left:
      [[fallthrough]];
    default:
      attrib.m_horizontalTextJustification = 0;
      break;
  }

  switch (fontDef.VerticalAlignment()) {
    case EoDb::VerticalAlignment::Top:
      attrib.m_verticalTextJustification = 3;
      break;
    case EoDb::VerticalAlignment::Middle:
      attrib.m_verticalTextJustification = 2;
      break;
    case EoDb::VerticalAlignment::Bottom:
      [[fallthrough]];
    default:
      attrib.m_verticalTextJustification = 0;
      break;
  }

  const auto origin = referenceSystem.Origin();

  // DXF ATTRIB: Left+Baseline uses first alignment point; other alignments use second alignment point
  const bool isDefaultAlignment = (attrib.m_horizontalTextJustification == 0 && attrib.m_verticalTextJustification == 0);
  if (isDefaultAlignment) {
    attrib.m_firstAlignmentPoint = {origin.x, origin.y, origin.z};
  } else {
    attrib.m_firstAlignmentPoint = {origin.x, origin.y, origin.z};
    attrib.m_secondAlignmentPoint = {origin.x, origin.y, origin.z};
  }

  attrib.m_textGenerationFlags = TextGenerationFlags();
  const auto& extrusion = Extrusion();
  attrib.m_extrusionDirection = {extrusion.x, extrusion.y, extrusion.z};

  writer->AddAttrib(attrib);
}

bool EoDbAttrib::Write(CFile& file) {
  // V1 write: write kAttribPrimitive type code followed by V1 text data.
  // When read by a V1 reader that doesn't recognize kAttribPrimitive, the primitive is skipped.
  // When read by a V2 reader, it dispatches to EoDbAttrib::ReadFromPeg.
  //
  // The text data following the type code is identical to kTextPrimitive layout so that
  // the ReadFromPeg can reuse the same field parsing.
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kAttribPrimitive));
  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, m_lineTypeIndex);

  // Font definition and reference system — delegating to the same serialization as EoDbText
  EoDbFontDefinition fontDefinition;
  GetFontDef(fontDefinition);
  fontDefinition.Write(file);

  EoGeReferenceSystem referenceSystem;
  GetRefSys(referenceSystem);
  referenceSystem.Write(file);

  EoDb::Write(file, Text());

  // V2 attribute extension — tag, flags, insertHandle
  EoDb::Write(file, CString(m_tagString.c_str()));
  EoDb::WriteInt16(file, m_attributeFlags);
  EoDb::WriteUInt64(file, m_insertHandle);

  return true;
}

void EoDbAttrib::Write(CFile& file, std::uint8_t* buffer) {
  // Legacy buffer-based write: delegate to EoDbText (writes kTextPrimitive).
  // Attribute identity is lost in this path — used only by AE2011 job files.
  EoDbText::Write(file, buffer);
}

EoDbAttrib* EoDbAttrib::ReadFromPeg(CFile& file) {
  // Read V1 text data (same layout as kTextPrimitive, minus the type code which was already consumed)
  auto color = EoDb::ReadInt16(file);
  (void)color;
  auto lineType = EoDb::ReadInt16(file);
  (void)lineType;
  EoDbFontDefinition fontDefinition;
  fontDefinition.Read(file);
  EoGeReferenceSystem referenceSystem;
  referenceSystem.Read(file);
  CString text;
  EoDb::Read(file, text);

  // Read V2 attribute extension
  CString tagString;
  EoDb::Read(file, tagString);
  auto attributeFlags = EoDb::ReadInt16(file);
  auto insertHandle = EoDb::ReadUInt64(file);

  auto* attribPrimitive =
      new EoDbAttrib(fontDefinition, referenceSystem, std::wstring(text.GetString()),
          std::wstring(tagString.GetString()), attributeFlags);
  attribPrimitive->SetInsertHandle(insertHandle);
  attribPrimitive->ConvertFormattingCharacters();
  return attribPrimitive;
}
