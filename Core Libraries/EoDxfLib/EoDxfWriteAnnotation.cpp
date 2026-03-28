#include <string>

#include "EoDxfWrite.h"

#include "EoDxfAttributes.h"
#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfMLeader.h"

bool EoDxfWrite::WriteDimension(const EoDxfDimension& dimension) {
  WriteCodeString(0, L"DIMENSION");
  WriteEntity(dimension);
  WriteCodeString(100, L"AcDbDimension");
  if (!dimension.GetBlockName().empty()) { WriteCodeWideString(2, dimension.GetBlockName()); }
  WriteCodeDouble(10, dimension.GetDefinitionPoint().x);
  WriteCodeDouble(20, dimension.GetDefinitionPoint().y);
  WriteCodeDouble(30, dimension.GetDefinitionPoint().z);
  WriteCodeDouble(11, dimension.GetTextPoint().x);
  WriteCodeDouble(21, dimension.GetTextPoint().y);
  WriteCodeDouble(31, dimension.GetTextPoint().z);

  auto dimensionType = dimension.m_dimensionType;
  if (!(dimensionType & 32)) { dimensionType = dimensionType + 32; }
  WriteCodeInt16(70, dimensionType);

  WriteCodeInt16(71, dimension.GetAttachmentPoint());
  if (dimension.GetTextLineSpacingStyle() != 1) { WriteCodeInt16(72, dimension.GetTextLineSpacingStyle()); }

  if (dimension.GetDimensionTextLineSpacingFactor() != 1) {
    WriteCodeDouble(41, dimension.GetDimensionTextLineSpacingFactor());
  }
  // Group code 42 - Actual measurement (optional; read-only value) - is not written by AeSys, as it is a read-only
  // value that is calculated by AutoCAD based on the dimension's geometry and properties. It is not a property that can
  // be set or modified directly by the user, and therefore does not need to be included in the DXF output when writing
  // dimension entities.

  if (!(dimension.GetExplicitDimensionText().empty())) {
    WriteCodeWideString(1, dimension.GetExplicitDimensionText());
  }
  if (std::abs(dimension.GetRotationAngleAwayFromDefault()) > EoDxf::geometricTolerance) {
    WriteCodeDouble(53, dimension.GetRotationAngleAwayFromDefault());
  }
  // Group code 51 - All dimension types have an optional 51 group code, which indicates the horizontal direction for
  // the dimension entity. The dimension entity determines the orientation of dimension text and lines for horizontal,
  // vertical, and rotated linear dimensions. This group value is the negative of the angle between the OCS X axis and
  // the UCS X axis.It is always in the XY plane of the OCS - not written byAeSys.

  WriteExtrusionDirection(dimension);
  WriteCodeWideString(3, dimension.GetDimensionStyleName());

  // End of AcDbDimension group, start of specific dimension type group

  switch (dimension.m_entityType) {
    case EoDxf::DIMALIGNED:
    case EoDxf::DIMLINEAR: {
      WriteCodeString(100, L"AcDbAlignedDimension");
      auto clonePoint = dimension.GetClonePoint();
      if (clonePoint.x != 0 || clonePoint.y != 0 || clonePoint.z != 0) {
        WriteCodeDouble(12, clonePoint.x);
        WriteCodeDouble(22, clonePoint.y);
        WriteCodeDouble(32, clonePoint.z);
      }
      WriteCodeDouble(13, dimension.GetExtensionLinePoint1().x);
      WriteCodeDouble(23, dimension.GetExtensionLinePoint1().y);
      WriteCodeDouble(33, dimension.GetExtensionLinePoint1().z);
      WriteCodeDouble(14, dimension.GetExtensionLinePoint2().x);
      WriteCodeDouble(24, dimension.GetExtensionLinePoint2().y);
      WriteCodeDouble(34, dimension.GetExtensionLinePoint2().z);
      if (dimension.m_entityType == EoDxf::DIMLINEAR) {
        if (dimension.GetRotationAngle() != 0) { WriteCodeDouble(50, dimension.GetRotationAngle()); }
        if (dimension.GetObliqueAngle() != 0) { WriteCodeDouble(52, dimension.GetObliqueAngle()); }
        WriteCodeString(100, L"AcDbRotatedDimension");
      }
      break;
    }
    case EoDxf::DIMRADIAL: {
      WriteCodeString(100, L"AcDbRadialDimension");
      WriteCodeDouble(15, dimension.GetRadiusDiameterPoint().x);
      WriteCodeDouble(25, dimension.GetRadiusDiameterPoint().y);
      WriteCodeDouble(35, dimension.GetRadiusDiameterPoint().z);
      WriteCodeDouble(40, dimension.GetLeaderLength());
      break;
    }
    case EoDxf::DIMDIAMETRIC: {
      WriteCodeString(100, L"AcDbDiametricDimension");
      WriteCodeDouble(15, dimension.GetRadiusDiameterPoint().x);
      WriteCodeDouble(25, dimension.GetRadiusDiameterPoint().y);
      WriteCodeDouble(35, dimension.GetRadiusDiameterPoint().z);
      WriteCodeDouble(40, dimension.GetLeaderLength());
      break;
    }
    case EoDxf::DIMANGULAR: {
      WriteCodeString(100, L"AcDb2LineAngularDimension");
      WriteCodeDouble(13, dimension.GetExtensionLinePoint1().x);
      WriteCodeDouble(23, dimension.GetExtensionLinePoint1().y);
      WriteCodeDouble(33, dimension.GetExtensionLinePoint1().z);
      WriteCodeDouble(14, dimension.GetExtensionLinePoint2().x);
      WriteCodeDouble(24, dimension.GetExtensionLinePoint2().y);
      WriteCodeDouble(34, dimension.GetExtensionLinePoint2().z);
      WriteCodeDouble(15, dimension.GetRadiusDiameterPoint().x);
      WriteCodeDouble(25, dimension.GetRadiusDiameterPoint().y);
      WriteCodeDouble(35, dimension.GetRadiusDiameterPoint().z);
      WriteCodeDouble(16, dimension.GetArcDefinitionPoint().x);
      WriteCodeDouble(26, dimension.GetArcDefinitionPoint().y);
      WriteCodeDouble(36, dimension.GetArcDefinitionPoint().z);
      break;
    }
    case EoDxf::DIMANGULAR3P: {
      WriteCodeString(100, L"AcDb3PointAngularDimension");
      WriteCodeDouble(13, dimension.GetExtensionLinePoint1().x);
      WriteCodeDouble(23, dimension.GetExtensionLinePoint1().y);
      WriteCodeDouble(33, dimension.GetExtensionLinePoint1().z);
      WriteCodeDouble(14, dimension.GetExtensionLinePoint2().x);
      WriteCodeDouble(24, dimension.GetExtensionLinePoint2().y);
      WriteCodeDouble(34, dimension.GetExtensionLinePoint2().z);
      WriteCodeDouble(15, dimension.GetRadiusDiameterPoint().x);
      WriteCodeDouble(25, dimension.GetRadiusDiameterPoint().y);
      WriteCodeDouble(35, dimension.GetRadiusDiameterPoint().z);
      break;
    }
    case EoDxf::DIMORDINATE: {
      WriteCodeString(100, L"AcDbOrdinateDimension");
      WriteCodeDouble(13, dimension.GetExtensionLinePoint1().x);
      WriteCodeDouble(23, dimension.GetExtensionLinePoint1().y);
      WriteCodeDouble(33, dimension.GetExtensionLinePoint1().z);
      WriteCodeDouble(14, dimension.GetExtensionLinePoint2().x);
      WriteCodeDouble(24, dimension.GetExtensionLinePoint2().y);
      WriteCodeDouble(34, dimension.GetExtensionLinePoint2().z);
      break;
    }
    default:
      break;
  }

  return m_writeOk;
}

bool EoDxfWrite::WriteLeader(const EoDxfLeader& leader) {
  WriteCodeString(0, L"LEADER");
  WriteEntity(leader);
  WriteCodeString(100, L"AcDbLeader");
  WriteCodeWideString(3, leader.m_dimensionStyleName);

  if (leader.m_arrowheadFlag != 1) { WriteCodeInt16(71, leader.m_arrowheadFlag); }
  if (leader.m_leaderPathType != 0) { WriteCodeInt16(72, leader.m_leaderPathType); }
  if (leader.m_leaderCreationFlag != 3) { WriteCodeInt16(73, leader.m_leaderCreationFlag); }
  if (leader.m_hookLineDirection != 1) { WriteCodeInt16(74, leader.m_hookLineDirection); }
  if (leader.m_hookLineFlag != 0) { WriteCodeInt16(75, leader.m_hookLineFlag); }

  if (std::abs(leader.m_textAnnotationHeight) > EoDxf::numericEpsilon) {
    WriteCodeDouble(40, leader.m_textAnnotationHeight);
  }
  if (std::abs(leader.m_textAnnotationWidth) > EoDxf::numericEpsilon) {
    WriteCodeDouble(41, leader.m_textAnnotationWidth);
  }

  WriteCodeInt16(76, static_cast<std::int16_t>(leader.m_vertexList.size()));

  for (const auto& vertex : leader.m_vertexList) {
    WriteCodeDouble(10, vertex.x);
    WriteCodeDouble(20, vertex.y);
    WriteCodeDouble(30, vertex.z);
  }
  if (leader.m_colorToUse != 0) { WriteCodeInt16(77, leader.m_colorToUse); }

  if (leader.m_associatedAnnotationHandle != EoDxf::NoHandle) {
    WriteCodeString(340, ToHexString(leader.m_associatedAnnotationHandle));
  }

  if (!leader.m_normalVector.IsDefaultNormal()) {
    WriteCodeDouble(210, leader.m_normalVector.x);
    WriteCodeDouble(220, leader.m_normalVector.y);
    WriteCodeDouble(230, leader.m_normalVector.z);
  }
  if (!leader.m_horizontalDirectionForLeader.IsEqualTo({1.0, 0.0, 0.0})) {
    WriteCodeDouble(211, leader.m_horizontalDirectionForLeader.x);
    WriteCodeDouble(221, leader.m_horizontalDirectionForLeader.y);
    WriteCodeDouble(231, leader.m_horizontalDirectionForLeader.z);
  }
  if (!leader.m_offsetFromBlockInsertionPoint.IsZero()) {
    WriteCodeDouble(212, leader.m_offsetFromBlockInsertionPoint.x);
    WriteCodeDouble(222, leader.m_offsetFromBlockInsertionPoint.y);
    WriteCodeDouble(232, leader.m_offsetFromBlockInsertionPoint.z);
  }
  if (!leader.m_offsetFromAnnotationPlacementPoint.IsZero()) {
    WriteCodeDouble(213, leader.m_offsetFromAnnotationPlacementPoint.x);
    WriteCodeDouble(223, leader.m_offsetFromAnnotationPlacementPoint.y);
    WriteCodeDouble(233, leader.m_offsetFromAnnotationPlacementPoint.z);
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteMLeader(const EoDxfMLeader& mLeader) {
  WriteCodeString(0, L"MULTILEADER");
  WriteEntity(mLeader);
  WriteCodeString(100, L"AcDbMLeader");

  // --- Top-level properties ---
  WriteCodeInt16(170, mLeader.m_leaderType);
  WriteCodeInt32(91, mLeader.m_leaderLineColor);

  if (mLeader.m_leaderLineTypeHandle != EoDxf::NoHandle) {
    WriteCodeString(341, ToHexString(mLeader.m_leaderLineTypeHandle));
  }
  WriteCodeInt16(171, mLeader.m_leaderLineWeight);
  WriteCodeBool(290, mLeader.m_enableLanding);
  WriteCodeBool(291, mLeader.m_enableDogleg);
  WriteCodeDouble(41, mLeader.m_doglegLength);
  WriteCodeDouble(42, mLeader.m_arrowheadSize);
  WriteCodeInt16(172, mLeader.m_contentType);

  if (mLeader.m_textStyleHandle != EoDxf::NoHandle) { WriteCodeString(343, ToHexString(mLeader.m_textStyleHandle)); }
  WriteCodeInt16(173, mLeader.m_textLeftAttachmentType);
  WriteCodeInt32(95, mLeader.m_textRightAttachmentType);
  WriteCodeInt16(174, mLeader.m_textAngleType);
  WriteCodeInt16(175, mLeader.m_textAlignmentType);
  WriteCodeInt32(92, mLeader.m_textColor);
  WriteCodeBool(292, mLeader.m_enableFrameText);

  if (mLeader.m_blockContentHandle != EoDxf::NoHandle) {
    WriteCodeString(344, ToHexString(mLeader.m_blockContentHandle));
  }
  WriteCodeInt32(93, mLeader.m_blockContentColor);
  WriteCodeDouble(10, mLeader.m_blockContentScale.x);
  WriteCodeDouble(20, mLeader.m_blockContentScale.y);
  WriteCodeDouble(30, mLeader.m_blockContentScale.z);
  WriteCodeDouble(43, mLeader.m_blockContentRotation);
  WriteCodeInt16(176, mLeader.m_blockContentConnectionType);
  WriteCodeBool(293, mLeader.m_enableAnnotationScale);

  if (mLeader.m_leaderStyleHandle != EoDxf::NoHandle) {
    WriteCodeString(340, ToHexString(mLeader.m_leaderStyleHandle));
  }
  WriteCodeInt32(90, mLeader.m_propertyOverrideFlag);
  WriteCodeDouble(45, mLeader.m_overallScale);
  WriteCodeBool(294, mLeader.m_textDirectionNegative);
  WriteCodeInt16(271, mLeader.m_textTopAttachmentType);
  WriteCodeInt16(272, mLeader.m_textBottomAttachmentType);

  // --- CONTEXT_DATA ---
  const auto& contextData = mLeader.m_contextData;
  WriteCodeString(300, L"CONTEXT_DATA{");
  WriteCodeDouble(40, contextData.m_contentScale);
  WriteCodeDouble(10, contextData.m_contentBasePoint.x);
  WriteCodeDouble(20, contextData.m_contentBasePoint.y);
  WriteCodeDouble(30, contextData.m_contentBasePoint.z);
  WriteCodeDouble(41, contextData.m_textHeight);
  WriteCodeDouble(140, contextData.m_arrowheadSize);
  WriteCodeDouble(145, contextData.m_landingGap);
  WriteCodeInt16(174, contextData.m_textLeftAttachment);
  WriteCodeInt16(175, contextData.m_textRightAttachment);
  WriteCodeInt16(176, contextData.m_textAlignmentType);
  WriteCodeInt16(177, contextData.m_blockContentConnectionType);
  WriteCodeBool(290, contextData.m_hasMText);
  WriteCodeBool(296, contextData.m_hasContent);
  // --- Leader branches ---
  for (const auto& branch : contextData.m_leaders) {
    WriteCodeString(302, L"LEADER{");
    WriteCodeBool(290, branch.m_hasSetLastLeaderLinePoint);
    WriteCodeBool(291, branch.m_hasSetDoglegVector);
    WriteCodeDouble(10, branch.m_lastLeaderLinePoint.x);
    WriteCodeDouble(20, branch.m_lastLeaderLinePoint.y);
    WriteCodeDouble(30, branch.m_lastLeaderLinePoint.z);
    WriteCodeDouble(11, branch.m_doglegVector.x);
    WriteCodeDouble(21, branch.m_doglegVector.y);
    WriteCodeDouble(31, branch.m_doglegVector.z);
    WriteCodeInt32(90, branch.m_leaderBranchIndex);
    WriteCodeDouble(40, branch.m_doglegLength);

    // --- Leader lines ---
    for (const auto& line : branch.m_leaderLines) {
      WriteCodeString(304, L"LEADER_LINE{");
      for (const auto& vertex : line.m_vertices) {
        WriteCodeDouble(10, vertex.x);
        WriteCodeDouble(20, vertex.y);
        WriteCodeDouble(30, vertex.z);
      }
      WriteCodeInt32(91, line.m_leaderLineIndex);
      if (line.m_leaderLineColorOverride != EoDxf::colorByLayer) { WriteCodeInt32(92, line.m_leaderLineColorOverride); }
      if (line.m_leaderLineWeightOverride >= 0) { WriteCodeInt16(171, line.m_leaderLineWeightOverride); }
      if (std::abs(line.m_arrowheadSize) > EoDxf::numericEpsilon) { WriteCodeDouble(40, line.m_arrowheadSize); }
      if (line.m_arrowheadHandle != EoDxf::NoHandle) { WriteCodeString(341, ToHexString(line.m_arrowheadHandle)); }
      WriteCodeString(305, L"}");
    }
    WriteCodeString(303, L"}");
  }

  // --- MText content ---
  if (contextData.m_hasMText) {
    WriteCodeString(304, L"{");
    WriteCodeDouble(12, contextData.m_textLocation.x);
    WriteCodeDouble(22, contextData.m_textLocation.y);
    WriteCodeDouble(32, contextData.m_textLocation.z);
    WriteCodeDouble(13, contextData.m_textDirection.x);
    WriteCodeDouble(23, contextData.m_textDirection.y);
    WriteCodeDouble(33, contextData.m_textDirection.z);
    WriteCodeDouble(42, contextData.m_textRotation);
    WriteCodeDouble(43, contextData.m_textWidth);
    WriteCodeDouble(44, contextData.m_textDefinedWidth);
    WriteCodeDouble(45, contextData.m_textDefinedHeight);
    WriteCodeInt16(170, contextData.m_textAttachment);
    WriteCodeInt32(90, contextData.m_textFlowDirection);
    WriteCodeInt32(91, contextData.m_textColor);
    WriteCodeDouble(141, contextData.m_textLineSpacingFactor);
    WriteCodeInt16(171, contextData.m_textLineSpacingStyle);
    WriteCodeInt16(172, contextData.m_textBackgroundFill);
    if (contextData.m_textStyleHandle != EoDxf::NoHandle) {
      WriteCodeString(340, ToHexString(contextData.m_textStyleHandle));
    }
    if (!contextData.m_textString.empty()) {
      size_t chunkOffset{};
      for (; (contextData.m_textString.size() - chunkOffset) > EoDxf::StringGroupCodeMaxChunk;
          chunkOffset += EoDxf::StringGroupCodeMaxChunk) {
        WriteCodeWideString(3, contextData.m_textString.substr(chunkOffset, EoDxf::StringGroupCodeMaxChunk));
      }
      WriteCodeWideString(1, contextData.m_textString.substr(chunkOffset));
    }
    WriteCodeString(301, L"}");
  }

  // --- Block content (non-MText) ---
  if (contextData.m_blockContentHandle != EoDxf::NoHandle) {
    WriteCodeString(341, ToHexString(contextData.m_blockContentHandle));
    WriteCodeDouble(14, contextData.m_blockContentNormalDirection.x);
    WriteCodeDouble(24, contextData.m_blockContentNormalDirection.y);
    WriteCodeDouble(34, contextData.m_blockContentNormalDirection.z);
    WriteCodeDouble(15, contextData.m_blockContentScale.x);
    WriteCodeDouble(25, contextData.m_blockContentScale.y);
    WriteCodeDouble(35, contextData.m_blockContentScale.z);
    WriteCodeDouble(46, contextData.m_blockContentRotation);
    WriteCodeInt32(93, contextData.m_blockContentColor);
  }

  WriteCodeString(301, L"}");  // close CONTEXT_DATA
  return m_writeOk;
}

bool EoDxfWrite::WriteMText(const EoDxfMText& mText) {
  WriteCodeString(0, L"MTEXT");
  WriteEntity(mText);
  WriteCodeString(100, L"AcDbMText");
  WriteCodeDouble(10, mText.m_insertionPoint.x);
  WriteCodeDouble(20, mText.m_insertionPoint.y);
  WriteCodeDouble(30, mText.m_insertionPoint.z);
  WriteCodeDouble(40, mText.m_nominalTextHeight);
  WriteCodeDouble(41, mText.m_referenceRectangleWidth);
  WriteCodeWideString(7, mText.m_textStyleName);
  WriteCodeDouble(50, mText.m_rotationAngle);

  if (mText.m_haveXAxisDirection) {
    WriteCodeDouble(11, mText.m_xAxisDirectionVector.x);
    WriteCodeDouble(21, mText.m_xAxisDirectionVector.y);
    WriteCodeDouble(31, mText.m_xAxisDirectionVector.z);
  }
  WriteCodeInt16(71, static_cast<std::int16_t>(mText.m_attachmentPoint));
  WriteCodeInt16(72, static_cast<std::int16_t>(mText.m_drawingDirection));
  WriteCodeInt16(73, static_cast<std::int16_t>(mText.m_lineSpacingStyle));
  WriteCodeDouble(44, mText.m_lineSpacingFactor);

  size_t chunkOffset{};
  for (; (mText.m_textString.size() - chunkOffset) > EoDxf::StringGroupCodeMaxChunk;
      chunkOffset += EoDxf::StringGroupCodeMaxChunk) {
    WriteCodeWideString(3, mText.m_textString.substr(chunkOffset, EoDxf::StringGroupCodeMaxChunk));
  }
  WriteCodeWideString(1, mText.m_textString.substr(chunkOffset));

  WriteExtrusionDirection(mText);

  return m_writeOk;
}

bool EoDxfWrite::WriteText(const EoDxfText& text) {
  WriteCodeString(0, L"TEXT");
  WriteEntity(text);
  WriteCodeString(100, L"AcDbText");
  WriteThickness(text);
  WriteCodeDouble(10, text.m_firstAlignmentPoint.x);
  WriteCodeDouble(20, text.m_firstAlignmentPoint.y);
  WriteCodeDouble(30, text.m_firstAlignmentPoint.z);
  WriteCodeDouble(40, text.m_textHeight);
  WriteCodeWideString(1, text.m_string);
  WriteCodeDouble(50, text.m_textRotation);
  WriteCodeDouble(41, text.m_scaleFactorWidth);
  WriteCodeDouble(51, text.m_obliqueAngle);

  WriteCodeWideString(7, text.m_textStyleName);

  WriteCodeInt16(71, text.m_textGenerationFlags);
  if (text.m_horizontalAlignment != EoDxfText::HorizontalAlignment::Left) {
    WriteCodeInt16(72, static_cast<std::int16_t>(text.m_horizontalAlignment));
  }
  if (text.m_horizontalAlignment != EoDxfText::HorizontalAlignment::Left ||
      text.m_verticalAlignment != EoDxfText::VerticalAlignment::BaseLine) {
    WriteCodeDouble(11, text.m_secondAlignmentPoint.x);
    WriteCodeDouble(21, text.m_secondAlignmentPoint.y);
    WriteCodeDouble(31, text.m_secondAlignmentPoint.z);
  }
  WriteExtrusionDirection(text);

  WriteCodeString(100, L"AcDbText");
  if (text.m_verticalAlignment != EoDxfText::VerticalAlignment::BaseLine) {
    WriteCodeInt16(73, static_cast<std::int16_t>(text.m_verticalAlignment));
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteAttrib(const EoDxfAttrib& attrib) {
  WriteCodeString(0, L"ATTRIB");
  WriteEntity(attrib);

  // AcDbText subclass — text properties
  WriteCodeString(100, L"AcDbText");
  WriteThickness(attrib);
  WriteCodeDouble(10, attrib.m_firstAlignmentPoint.x);
  WriteCodeDouble(20, attrib.m_firstAlignmentPoint.y);
  WriteCodeDouble(30, attrib.m_firstAlignmentPoint.z);
  WriteCodeDouble(40, attrib.m_textHeight);
  WriteCodeWideString(1, attrib.m_attributeValue);
  if (std::abs(attrib.m_textRotation) > EoDxf::geometricTolerance) {
    WriteCodeDouble(50, attrib.m_textRotation);
  }
  if (std::abs(attrib.m_relativeXScaleFactor - 1.0) > EoDxf::numericEpsilon) {
    WriteCodeDouble(41, attrib.m_relativeXScaleFactor);
  }
  if (std::abs(attrib.m_obliqueAngle) > EoDxf::geometricTolerance) {
    WriteCodeDouble(51, attrib.m_obliqueAngle);
  }
  WriteCodeWideString(7, attrib.m_textStyleName);
  if (attrib.m_textGenerationFlags != 0) {
    WriteCodeInt16(71, attrib.m_textGenerationFlags);
  }
  if (attrib.m_horizontalTextJustification != 0) {
    WriteCodeInt16(72, attrib.m_horizontalTextJustification);
  }
  if (attrib.m_horizontalTextJustification != 0 || attrib.m_verticalTextJustification != 0) {
    WriteCodeDouble(11, attrib.m_secondAlignmentPoint.x);
    WriteCodeDouble(21, attrib.m_secondAlignmentPoint.y);
    WriteCodeDouble(31, attrib.m_secondAlignmentPoint.z);
  }

  // AcDbAttribute subclass — attribute-specific fields
  WriteCodeString(100, L"AcDbAttribute");
  if (attrib.m_versionNumber != 0) {
    WriteCodeInt16(280, attrib.m_versionNumber);
  }
  WriteCodeWideString(2, attrib.m_tagString);
  if (attrib.m_attributeFlags != 0) {
    WriteCodeInt16(70, attrib.m_attributeFlags);
  }
  if (attrib.m_verticalTextJustification != 0) {
    WriteCodeInt16(74, attrib.m_verticalTextJustification);
  }

  WriteExtrusionDirection(attrib);

  return m_writeOk;
}
