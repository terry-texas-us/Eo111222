#include "Stdafx.h"

#include <cmath>
#include <cstdint>
#include <string>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbAttrib.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbDxfInterface.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDxfEntities.h"
#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

void EoDbDxfInterface::AddAttrib(const EoDxfAttrib& attrib) {
  if (m_dxfWriter) {
    m_dxfWriter->WriteAttrib(attrib);
    return;
  }
  countOfAttrib++;
  ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAttrib - entities section\n");
  auto* attribPrimitive = ConvertAttribEntity(attrib, m_document);
  if (attribPrimitive == nullptr) { return; }

  if (m_currentInsertPrimitive != nullptr) {
    const auto insertHandle = m_currentInsertPrimitive->Handle();
    attribPrimitive->SetInsertHandle(insertHandle);
    attribPrimitive->SetOwnerHandle(insertHandle);
    m_currentInsertPrimitive->AddAttributeHandle(attribPrimitive->Handle());
  }

  // Add to the same group as the parent INSERT when available
  if (m_currentInsertGroup != nullptr) {
    m_document->RegisterHandle(attribPrimitive);
    m_currentInsertGroup->AddTail(attribPrimitive);
  } else {
    // Orphan ATTRIB or block-definition context — fall back to AddToDocument
    AddToDocument(attribPrimitive, m_document, attrib.m_space, attrib.m_ownerHandle);
  }
}

void EoDbDxfInterface::ConvertMTextEntity(const EoDxfMText& mtext, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"MText entity conversion\n");

  if (mtext.m_nominalTextHeight < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 1, L"MText entity skipped: zero or near-zero text height\n");
    return;
  }
  if (mtext.m_textString.empty()) {
    ATLTRACE2(traceGeneral, 1, L"MText entity skipped: empty text string\n");
    return;
  }

  auto textHeight = mtext.m_nominalTextHeight;  // Group code 40

  // MTEXT rotation (group code 50) is already in radians; UpdateAngle() has resolved xAxisDirection if present
  auto textRotation = mtext.m_rotationAngle;

  std::wstring textStyleName = mtext.m_textStyleName;  // Group code 7

  auto insertionPointInOcs = EoGePoint3d{mtext.m_insertionPoint.x, mtext.m_insertionPoint.y, mtext.m_insertionPoint.z};
  EoGeVector3d extrusionDirection{
      mtext.m_extrusionDirection.x, mtext.m_extrusionDirection.y, mtext.m_extrusionDirection.z};
  if (!mtext.m_haveExtrusion || extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }

  // Transform insertion point OCS → WCS
  EoGeOcsTransform transformOcs{extrusionDirection};
  auto insertionPointInWcs = transformOcs * insertionPointInOcs;

  // Build baseline direction from rotation angle
  auto baselineDirection = EoGeVector3d::positiveUnitX;
  baselineDirection.RotateAboutArbitraryAxis(extrusionDirection, textRotation);

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(extrusionDirection, xAxisDirection);

  yAxisDirection *= textHeight;
  xAxisDirection *= textHeight * Eo::defaultCharacterCellAspectRatio;

  // Map MTEXT AttachmentPoint (9-point grid) to AeSys horizontal and vertical alignment
  EoDb::HorizontalAlignment horizontalAlignment = EoDb::HorizontalAlignment::Left;
  EoDb::VerticalAlignment verticalAlignment = EoDb::VerticalAlignment::Top;

  switch (mtext.m_attachmentPoint) {
    case EoDxfMText::AttachmentPoint::TopLeft:
      horizontalAlignment = EoDb::HorizontalAlignment::Left;
      verticalAlignment = EoDb::VerticalAlignment::Top;
      break;
    case EoDxfMText::AttachmentPoint::TopCenter:
      horizontalAlignment = EoDb::HorizontalAlignment::Center;
      verticalAlignment = EoDb::VerticalAlignment::Top;
      break;
    case EoDxfMText::AttachmentPoint::TopRight:
      horizontalAlignment = EoDb::HorizontalAlignment::Right;
      verticalAlignment = EoDb::VerticalAlignment::Top;
      break;
    case EoDxfMText::AttachmentPoint::MiddleLeft:
      horizontalAlignment = EoDb::HorizontalAlignment::Left;
      verticalAlignment = EoDb::VerticalAlignment::Middle;
      break;
    case EoDxfMText::AttachmentPoint::MiddleCenter:
      horizontalAlignment = EoDb::HorizontalAlignment::Center;
      verticalAlignment = EoDb::VerticalAlignment::Middle;
      break;
    case EoDxfMText::AttachmentPoint::MiddleRight:
      horizontalAlignment = EoDb::HorizontalAlignment::Right;
      verticalAlignment = EoDb::VerticalAlignment::Middle;
      break;
    case EoDxfMText::AttachmentPoint::BottomLeft:
      horizontalAlignment = EoDb::HorizontalAlignment::Left;
      verticalAlignment = EoDb::VerticalAlignment::Bottom;
      break;
    case EoDxfMText::AttachmentPoint::BottomCenter:
      horizontalAlignment = EoDb::HorizontalAlignment::Center;
      verticalAlignment = EoDb::VerticalAlignment::Bottom;
      break;
    case EoDxfMText::AttachmentPoint::BottomRight:
      horizontalAlignment = EoDb::HorizontalAlignment::Right;
      verticalAlignment = EoDb::VerticalAlignment::Bottom;
      break;
    default:
      break;
  }

  /** @brief Strip only MTEXT formatting codes that AeSys cannot render; preserve codes the renderer handles.
   *
   * AeSys DisplayTextWithFormattingCharacters() handles at render time:
   *  - \P  → hard line break (paragraph)
   *  - \A  → alignment change (bottom/center/top)
   *  - \S  → stacked fractions (numerator/denominator with / or ^ delimiter)
   *
   * Everything else is stripped here at import time:
   *  - \fFontName|...; \HValue; \CValue; \TValue; \QValue; \WValue; → skip to semicolon
   *  - \L \l \O \o → underline/overline start/stop (dropped)
   *  - \~ → non-breaking space (converted to regular space)
   *  - \\ \{ \} → escaped literals (resolved to the literal character)
   *  - { } → brace grouping (braces stripped, content preserved)
   */
  const auto& rawText = mtext.m_textString;
  std::wstring cleanedText;
  cleanedText.reserve(rawText.size());

  for (std::size_t i = 0; i < rawText.size(); ++i) {
    const wchar_t currentCharacter = rawText[i];

    if (currentCharacter == L'\\' && i + 1 < rawText.size()) {
      const wchar_t nextCharacter = rawText[i + 1];
      switch (nextCharacter) {
        case L'P':
        case L'S':
          // Renderer handles \P and \S — pass through verbatim
          cleanedText += L'\\';
          cleanedText += nextCharacter;
          ++i;
          break;
        case L'A': {
          // Renderer handles \A — pass through the complete \Avalue; sequence
          auto semicolonPosition = rawText.find(L';', i + 2);
          if (semicolonPosition != std::wstring::npos) {
            cleanedText += rawText.substr(i, semicolonPosition - i + 1);
            i = semicolonPosition;
          } else {
            cleanedText += L'\\';
            cleanedText += nextCharacter;
            ++i;
          }
          break;
        }
        case L'\\':
        case L'{':
        case L'}':
          // Escaped literal characters — resolve to the literal
          cleanedText += nextCharacter;
          ++i;
          break;
        case L'~':
          // Non-breaking space → regular space
          cleanedText += L' ';
          ++i;
          break;
        case L'L':
        case L'l':
        case L'O':
        case L'o':
          // Underline/overline start/stop → strip
          ++i;
          break;
        case L'f':
        case L'F':
        case L'H':
        case L'C':
        case L'T':
        case L'Q':
        case L'W':
        case L'p': {
          // Unsupported formatting commands with value ending in semicolon → skip to semicolon
          auto semicolonPosition = rawText.find(L';', i + 2);
          if (semicolonPosition != std::wstring::npos) {
            i = semicolonPosition;
          } else {
            ++i;
          }
          break;
        }
        default:
          // Unknown escape → keep as-is
          cleanedText += currentCharacter;
          cleanedText += nextCharacter;
          ++i;
          break;
      }
    } else if (currentCharacter == L'{' || currentCharacter == L'}') {
      // Brace grouping → strip braces, content preserved
      continue;
    } else {
      cleanedText += currentCharacter;
    }
  }

  if (cleanedText.empty()) {
    ATLTRACE2(traceGeneral, 1, L"MText entity skipped: no text content after formatting strip\n");
    return;
  }

  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(textStyleName);
  fontDefinition.SetHorizontalAlignment(horizontalAlignment);
  fontDefinition.SetVerticalAlignment(verticalAlignment);

  // Map MTEXT drawing direction (group 72) to AeSys font path
  if (mtext.m_drawingDirection == EoDxfMText::DrawingDirection::TopToBottom) {
    fontDefinition.SetPath(EoDb::Path::Down);
  }

  EoGeReferenceSystem referenceSystem(insertionPointInWcs, xAxisDirection, yAxisDirection);

  auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, cleanedText);
  textPrimitive->SetBaseProperties(&mtext, document);
  textPrimitive->SetExtrusion(extrusionDirection);

  // Preserve MTEXT-specific DXF properties for round-trip export
  EoDbMTextProperties mtextProperties;
  mtextProperties.attachmentPoint = static_cast<std::int16_t>(mtext.m_attachmentPoint);
  mtextProperties.drawingDirection = static_cast<std::int16_t>(mtext.m_drawingDirection);
  mtextProperties.lineSpacingStyle = static_cast<std::int16_t>(mtext.m_lineSpacingStyle);
  mtextProperties.lineSpacingFactor = mtext.m_lineSpacingFactor;
  mtextProperties.referenceRectangleWidth = mtext.m_referenceRectangleWidth;
  textPrimitive->SetMTextProperties(mtextProperties);

  AddToDocument(textPrimitive, document, mtext.m_space, mtext.m_ownerHandle);
}

void EoDbDxfInterface::ConvertTextEntity(const EoDxfText& text, [[maybe_unused]] AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Text entity conversion\n");

  // Guard: skip degenerate entities with zero height or empty string
  if (text.m_textHeight < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 1, L"Text entity skipped: zero or near-zero text height\n");
    return;
  }
  if (text.m_string.empty()) {
    ATLTRACE2(traceGeneral, 1, L"Text entity skipped: empty text string\n");
    return;
  }

  [[maybe_unused]] auto thickness = text.m_thickness;  // Group code 39 (not supported in AeSys)
  auto firstAlignmentPointInOcs =
      EoGePoint3d{text.m_firstAlignmentPoint.x, text.m_firstAlignmentPoint.y, text.m_firstAlignmentPoint.z};
  auto textHeight = text.m_textHeight;  // Group code 40

  std::wstring string{text.m_string};  // Group code 1

  auto textRotation = Eo::DegreeToRadian(text.m_textRotation);  // Group code 50 (degrees → radians)
  auto xScaleFactorWidth = text.m_scaleFactorWidth;  // Group code 41
  auto obliqueAngle = Eo::DegreeToRadian(text.m_obliqueAngle);  // Group code 51 (degrees → radians)

  std::wstring textStyleName = text.m_textStyleName;  // Group code 7

  auto textGenerationFlags = text.m_textGenerationFlags;  // Group code 71 (2=backward, 4=upside-down)
  auto horizontalAlignment = text.m_horizontalAlignment;  // Group code 72

  auto secondAlignmentPointInOcs =
      EoGePoint3d{text.m_secondAlignmentPoint.x, text.m_secondAlignmentPoint.y, text.m_secondAlignmentPoint.z};
  EoGeVector3d extrusionDirection{
      text.m_extrusionDirection.x, text.m_extrusionDirection.y, text.m_extrusionDirection.z};
  if (!text.m_haveExtrusion || extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }

  auto verticalAlignment = text.m_verticalAlignment;  // Group code 73

  const bool hasSecondAlignmentPoint = text.HasSecondAlignmentPoint();

  EoGePoint3d firstAlignmentPointInWcs;
  EoGePoint3d secondAlignmentPointInWcs;

  // Always transform points to WCS (simplifies branches)
  EoGeOcsTransform transformOcs{extrusionDirection};
  firstAlignmentPointInWcs = transformOcs * firstAlignmentPointInOcs;
  secondAlignmentPointInWcs = transformOcs * secondAlignmentPointInOcs;

  // Determine if this is the default alignment (Left + Baseline)
  const bool isDefaultAlignment = (horizontalAlignment == EoDxfText::HorizontalAlignment::Left &&
      verticalAlignment == EoDxfText::VerticalAlignment::BaseLine);
  const bool isAlignedOrFit = (horizontalAlignment == EoDxfText::HorizontalAlignment::AlignedIfBaseLine ||
      horizontalAlignment == EoDxfText::HorizontalAlignment::FitIfBaseLine);

  // Compute baseline direction – respect DXF rules for Aligned/Fit
  auto baselineDirection = EoGeVector3d::positiveUnitX;

  if (hasSecondAlignmentPoint && isAlignedOrFit) {
    // Spec: for Aligned/Fit, ignore textRotation; baseline direction is defined by both points
    auto alignedDirection = secondAlignmentPointInWcs - firstAlignmentPointInWcs;
    if (!alignedDirection.IsNearNull()) {
      baselineDirection = alignedDirection;
      baselineDirection.Unitize();
    }
  } else {
    // Normal case: rotation defines direction
    baselineDirection.RotateAboutArbitraryAxis(extrusionDirection, textRotation);
  }

  /** DXF origin selection rules:
   *  - Default alignment (Left + Baseline): first alignment point is the insertion point
   *  - Aligned/Fit: first alignment point is the baseline start
   *  - All other non-default alignments: second alignment point is the reference point
   */
  EoGePoint3d referenceOrigin;
  if (isDefaultAlignment || isAlignedOrFit) {
    referenceOrigin = firstAlignmentPointInWcs;
  } else if (hasSecondAlignmentPoint) {
    referenceOrigin = secondAlignmentPointInWcs;
  } else {
    // Fallback: if second point was not parsed, use first (malformed DXF data)
    ATLTRACE2(traceGeneral, 1, L"Text entity: non-default alignment but no second alignment point; using first\n");
    referenceOrigin = firstAlignmentPointInWcs;
  }

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(extrusionDirection, xAxisDirection);

  // Apply oblique angle: shear the Y-axis by rotating it toward the baseline
  if (Eo::IsGeometricallyNonZero(obliqueAngle)) {
    yAxisDirection.RotateAboutArbitraryAxis(extrusionDirection, -obliqueAngle);
  }

  yAxisDirection *= textHeight;
  xAxisDirection *= xScaleFactorWidth * textHeight * Eo::defaultCharacterCellAspectRatio;
  EoGeReferenceSystem referenceSystem(referenceOrigin, xAxisDirection, yAxisDirection);

  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(textStyleName);

  /** Only Left(0), Center(1) and Right(2) horizontal alignment options are supported in AeSys so options
   * Aligned(3) → Left, Middle(4) → Center, and Fit(5) → Right; (should only pair with Baseline(0) vertical alignment
   * and when the second alignment point is defined).
   * @todo AeSys does not support the stretching behavior of Aligned/Fit options.
   */

  /** DXF horizontal alignment 4 ("Middle") is a composite alignment: horizontally centered AND
   *  vertically at the middle of the text height. It is only valid paired with vertical alignment 0
   *  (Baseline). Override the vertical to Middle when this special case is detected.
   */
  const bool isMiddleComposite = (horizontalAlignment == EoDxfText::HorizontalAlignment::MiddleIfBaseLine &&
      verticalAlignment == EoDxfText::VerticalAlignment::BaseLine);

  switch (horizontalAlignment) {
    case EoDxfText::HorizontalAlignment::Center:
      [[fallthrough]];
    case EoDxfText::HorizontalAlignment::MiddleIfBaseLine:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
      break;
    case EoDxfText::HorizontalAlignment::Right:
      [[fallthrough]];
    case EoDxfText::HorizontalAlignment::FitIfBaseLine:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Right);
      break;
    case EoDxfText::HorizontalAlignment::Left:
      [[fallthrough]];
    case EoDxfText::HorizontalAlignment::AlignedIfBaseLine:
      [[fallthrough]];
    default:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Left);
      break;
  }

  if (isMiddleComposite) {
    fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
  } else {
    switch (verticalAlignment) {
      case EoDxfText::VerticalAlignment::Middle:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
        break;
      case EoDxfText::VerticalAlignment::Top:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Top);
        break;
      case EoDxfText::VerticalAlignment::BaseLine:
        [[fallthrough]];
      case EoDxfText::VerticalAlignment::Bottom:
        [[fallthrough]];
      default:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Bottom);
        break;
    }
  }

  auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, string);
  textPrimitive->SetBaseProperties(&text, document);
  textPrimitive->SetTextGenerationFlags(textGenerationFlags);
  textPrimitive->SetExtrusion(extrusionDirection);

  AddToDocument(textPrimitive, document, text.m_space, text.m_ownerHandle);
}

void EoDbDxfInterface::ConvertAttDefEntity(const EoDxfAttDef& attdef, [[maybe_unused]] AeSysDoc* document) {
  // ATTDEFs define attribute templates inside block definitions. In AutoCAD they are NOT rendered
  // in entity references — only ATTRIBs following INSERT entities are rendered. Skipping conversion
  // to EoDbPrimitive prevents the default value from overlapping with the actual ATTRIB text at the
  // same position.
  //
  // When inside a block definition, store the parsed EoDxfAttDef directly in the block so that
  // the full entity property set (handle, owner, layer, color, linetype, etc.) is preserved for
  // DXF round-trip export and future interactive attribute prompting.
  if (m_currentOpenBlockDefinition != nullptr) {
    m_currentOpenBlockDefinition->AddAttributeDefinition(attdef);
    ATLTRACE2(traceGeneral, 2, L"AttDef stored in block (tag='%s', default='%s', prompt='%s')\n",
        attdef.m_tagString.c_str(), attdef.m_defaultValue.c_str(), attdef.m_promptString.c_str());
  } else {
    ATLTRACE2(traceGeneral, 2, L"AttDef entity skipped — not inside block (tag='%s', default='%s')\n",
        attdef.m_tagString.c_str(), attdef.m_defaultValue.c_str());
  }
}

EoDbAttrib* EoDbDxfInterface::ConvertAttribEntity(const EoDxfAttrib& attrib, AeSysDoc* document) {
  ATLTRACE2(traceGeneral, 2, L"Attrib entity conversion (tag='%s', value='%s')\n", attrib.m_tagString.c_str(),
      attrib.m_attributeValue.c_str());

  // Skip invisible attributes (flag bit 0)
  if (attrib.m_attributeFlags & 1) {
    ATLTRACE2(traceGeneral, 2, L"Attrib entity skipped: invisible flag set\n");
    return nullptr;
  }

  if (attrib.m_textHeight < Eo::geometricTolerance) {
    ATLTRACE2(traceGeneral, 1, L"Attrib entity skipped: zero or near-zero text height\n");
    return nullptr;
  }
  if (attrib.m_attributeValue.empty()) {
    ATLTRACE2(traceGeneral, 1, L"Attrib entity skipped: empty attribute value\n");
    return nullptr;
  }

  auto firstAlignmentPointInOcs =
      EoGePoint3d{attrib.m_firstAlignmentPoint.x, attrib.m_firstAlignmentPoint.y, attrib.m_firstAlignmentPoint.z};
  auto textHeight = attrib.m_textHeight;
  std::wstring string{attrib.m_attributeValue};
  auto textRotation = Eo::DegreeToRadian(attrib.m_textRotation);
  auto xScaleFactorWidth = attrib.m_relativeXScaleFactor;
  auto obliqueAngle = Eo::DegreeToRadian(attrib.m_obliqueAngle);
  std::wstring textStyleName = attrib.m_textStyleName;

  auto horizontalAlignment = attrib.m_horizontalTextJustification;
  auto verticalAlignment = attrib.m_verticalTextJustification;

  ATLTRACE2(traceGeneral, 2, L"  Attrib alignment: h=%d, v=%d, hasSecondPt=%d\n", horizontalAlignment,
      verticalAlignment, attrib.HasSecondAlignmentPoint() ? 1 : 0);

  auto secondAlignmentPointInOcs =
      EoGePoint3d{attrib.m_secondAlignmentPoint.x, attrib.m_secondAlignmentPoint.y, attrib.m_secondAlignmentPoint.z};
  EoGeVector3d extrusionDirection{
      attrib.m_extrusionDirection.x, attrib.m_extrusionDirection.y, attrib.m_extrusionDirection.z};
  if (!attrib.m_haveExtrusion || extrusionDirection.IsNearNull()) {
    extrusionDirection = EoGeVector3d::positiveUnitZ;
  } else {
    extrusionDirection.Unitize();
  }

  const bool hasSecondAlignmentPoint = attrib.HasSecondAlignmentPoint();

  EoGeOcsTransform transformOcs{extrusionDirection};
  auto firstAlignmentPointInWcs = transformOcs * firstAlignmentPointInOcs;
  auto secondAlignmentPointInWcs = transformOcs * secondAlignmentPointInOcs;

  const bool isDefaultAlignment = (horizontalAlignment == 0 && verticalAlignment == 0);
  const bool isAlignedOrFit = (horizontalAlignment == 3 || horizontalAlignment == 5);

  auto baselineDirection = EoGeVector3d::positiveUnitX;

  if (hasSecondAlignmentPoint && isAlignedOrFit) {
    auto alignedDirection = secondAlignmentPointInWcs - firstAlignmentPointInWcs;
    if (!alignedDirection.IsNearNull()) {
      baselineDirection = alignedDirection;
      baselineDirection.Unitize();
    }
  } else {
    baselineDirection.RotateAboutArbitraryAxis(extrusionDirection, textRotation);
  }

  EoGePoint3d referenceOrigin;
  if (isDefaultAlignment || isAlignedOrFit) {
    referenceOrigin = firstAlignmentPointInWcs;
  } else if (hasSecondAlignmentPoint) {
    referenceOrigin = secondAlignmentPointInWcs;
  } else {
    ATLTRACE2(traceGeneral, 1, L"Attrib entity: non-default alignment but no second alignment point; using first\n");
    referenceOrigin = firstAlignmentPointInWcs;
  }

  auto xAxisDirection = baselineDirection;
  auto yAxisDirection = CrossProduct(extrusionDirection, xAxisDirection);

  if (Eo::IsGeometricallyNonZero(obliqueAngle)) {
    yAxisDirection.RotateAboutArbitraryAxis(extrusionDirection, -obliqueAngle);
  }

  yAxisDirection *= textHeight;
  xAxisDirection *= xScaleFactorWidth * textHeight * Eo::defaultCharacterCellAspectRatio;
  EoGeReferenceSystem referenceSystem(referenceOrigin, xAxisDirection, yAxisDirection);

  EoDbFontDefinition fontDefinition{};
  fontDefinition.SetFontName(textStyleName);

  /** DXF horizontal alignment 4 ("Middle") is a composite alignment: horizontally centered AND
   *  vertically at the middle of the text height. It is only valid paired with vertical alignment 0
   *  (Baseline). Override the vertical to Middle when this special case is detected.
   */
  const bool isMiddleComposite = (horizontalAlignment == 4 && verticalAlignment == 0);

  switch (horizontalAlignment) {
    case 1:  // Center
    case 4:  // Middle (paired with Baseline)
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Center);
      break;
    case 2:  // Right
    case 5:  // Fit (paired with Baseline)
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Right);
      break;
    case 0:  // Left
    case 3:  // Aligned (paired with Baseline)
    default:
      fontDefinition.SetHorizontalAlignment(EoDb::HorizontalAlignment::Left);
      break;
  }

  if (isMiddleComposite) {
    fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
  } else {
    switch (verticalAlignment) {
      case 2:  // Middle
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Middle);
        break;
      case 3:  // Top
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Top);
        break;
      case 0:  // Baseline
      case 1:  // Bottom
      default:
        fontDefinition.SetVerticalAlignment(EoDb::VerticalAlignment::Bottom);
        break;
    }
  }

  auto* attribPrimitive = new EoDbAttrib(fontDefinition, referenceSystem, string, attrib.m_tagString, attrib.m_attributeFlags);
  attribPrimitive->SetBaseProperties(&attrib, document);
  attribPrimitive->SetTextGenerationFlags(attrib.m_textGenerationFlags);
  attribPrimitive->SetExtrusion(extrusionDirection);

  return attribPrimitive;
}
