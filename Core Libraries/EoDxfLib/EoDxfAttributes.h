#pragma once

#include <cstdint>
#include <string>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"


/** @brief Class representing the ATTDEF entity in a DXF file, which defines an attribute definition for block references.
 *
 *  The EoDxfAttDef class inherits from EoDxfGraphic and contains properties specific to the ATTDEF entity, such as
 *  insertion point, text height, default value, text rotation, and other attributes related to text formatting and
 *  attribute definition. It also includes methods for parsing DXF group codes to populate these properties and applying
 *  extrusion transformations if necessary.
 */
class EoDxfAttDef : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfAttDef(EoDxf::ETYPE entityType = EoDxf::ATTDEF) noexcept : EoDxfGraphic{entityType} {}
  void ApplyExtrusion() override;

  [[nodiscard]] bool HasSecondAlignmentPoint() const noexcept { return m_hasSecondAlignmentPoint; }

 protected:
  /** @brief Parses dxf code and value to read attribute definition entity data
   *  @param code dxf code
   *  @param reader reference to EoDxfReader to read value
   */
  void ParseCode(int code, EoDxfReader& reader);

 public:
  // --- Subclass AcDbText ---
  EoDxfGeometryBase3d m_insertionPoint{};  // Group codes 10, 20 & 30 (first alignment point in OCS)
  EoDxfGeometryBase3d m_secondAlignmentPoint{};  // Group codes 11, 21 & 31 (second alignment point in OCS)
  double m_textHeight{};  // Group code 40
  std::wstring m_defaultValue;  // Group code 1 (default attribute value string)
  double m_textRotation{};  // Group code 50 (in degrees)
  double m_relativeXScaleFactor{1.0};  // Group code 41 (width factor, default 1.0)
  double m_obliqueAngle{};  // Group code 51 (in degrees)
  std::wstring m_textStyleName{L"STANDARD"};  // Group code 7
  std::int16_t m_textGenerationFlags{};  // Group code 71 (optional; 2=backward, 4=upside down)
  std::int16_t
      m_horizontalTextJustification{};  // Group code 72 (0=Left, 1=Center, 2=Right, 3=Aligned, 4=Middle, 5=Fit)

  // --- Subclass AcDbAttributeDefinition ---
  std::int16_t m_versionNumber{};  // Group code 280 (0 = 2010)
  std::wstring m_promptString;  // Group code 3 (prompt string displayed during attribute insertion)
  std::wstring m_tagString;  // Group code 2 (attribute tag, cannot contain spaces)
  std::int16_t m_attributeFlags{};  // Group code 70 (1=Invisible, 2=Constant, 4=Verification required, 8=Preset)
  std::int16_t m_fieldLength{};  // Group code 73 (optional, not currently used)
  std::int16_t m_verticalTextJustification{};  // Group code 74 (0=Baseline, 1=Bottom, 2=Middle, 3=Top)

 private:
  bool m_hasSecondAlignmentPoint{};
};

/** @brief Class representing the ATTRIB entity in a DXF file, which defines an attribute instance for block references.
 *
 *  The EoDxfAttrib class inherits from EoDxfGraphic and contains properties specific to the ATTRIB entity, such as
 *  alignment points, text height, attribute value, text rotation, and other attributes related to text formatting and
 *  attribute instance. It also includes methods for parsing DXF group codes to populate these properties and applying
 *  extrusion transformations if necessary.
 */
class EoDxfAttrib : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  explicit EoDxfAttrib(EoDxf::ETYPE entityType = EoDxf::ATTRIB) noexcept : EoDxfGraphic{entityType} {}
  void ApplyExtrusion() override;

  [[nodiscard]] bool HasSecondAlignmentPoint() const noexcept { return m_hasSecondAlignmentPoint; }

 protected:
  /** @brief Parses dxf code and value to read attribute entity data
   *  @param code dxf code
   *  @param reader reference to EoDxfReader to read value
   */
  void ParseCode(int code, EoDxfReader& reader);

 public:
  // --- Subclass AcDbText ---
  EoDxfGeometryBase3d m_firstAlignmentPoint{};  // Group codes 10, 20 & 30 (text start point in OCS)
  double m_textHeight{};  // Group code 40
  std::wstring m_attributeValue;  // Group code 1 (attribute value string)
  double m_textRotation{};  // Group code 50 (in degrees)
  double m_relativeXScaleFactor{1.0};  // Group code 41 (width factor, default 1.0)
  double m_obliqueAngle{};  // Group code 51 (in degrees)
  std::wstring m_textStyleName{L"STANDARD"};  // Group code 7
  std::int16_t m_textGenerationFlags{};  // Group code 71 (optional; 2=backward, 4=upside down)
  std::int16_t
      m_horizontalTextJustification{};  // Group code 72 (0=Left, 1=Center, 2=Right, 3=Aligned, 4=Middle, 5=Fit)

  // --- Subclass AcDbAttribute ---
  std::int16_t m_versionNumber{};  // Group code 280 (0 = 2010)
  std::wstring m_tagString;  // Group code 2 (attribute tag)
  std::int16_t m_attributeFlags{};  // Group code 70 (1=Invisible, 2=Constant, 4=Verification required, 8=Preset)
  std::int16_t m_fieldLength{};  // Group code 73 (optional, not currently used)
  std::int16_t m_verticalTextJustification{};  // Group code 74 (0=Baseline, 1=Bottom, 2=Middle, 3=Top)
  EoDxfGeometryBase3d m_secondAlignmentPoint{};  // Group codes 11, 21 & 31 (alignment/fit point in OCS)

 private:
  bool m_hasSecondAlignmentPoint{};
};
