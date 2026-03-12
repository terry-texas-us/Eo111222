#include <string>

#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfMLeader.h"
#include "EoDxfWriter.h"

bool EoDxfWrite::WriteDimension(EoDxfDimension* dimension) {
  WriteCodeString(0, L"DIMENSION");
  WriteEntity(dimension);
  WriteCodeString(100, L"AcDbDimension");
  if (!dimension->getName().empty()) { WriteCodeWideString(2, dimension->getName()); }
  WriteCodeDouble(10, dimension->getDefPoint().x);
  WriteCodeDouble(20, dimension->getDefPoint().y);
  WriteCodeDouble(30, dimension->getDefPoint().z);
  WriteCodeDouble(11, dimension->getTextPoint().x);
  WriteCodeDouble(21, dimension->getTextPoint().y);
  WriteCodeDouble(31, dimension->getTextPoint().z);
  if (!(dimension->m_dimensionType & 32)) { dimension->m_dimensionType = dimension->m_dimensionType + 32; }
  WriteCodeInt16(70, dimension->m_dimensionType);
  if (!(dimension->getText().empty())) { WriteCodeWideString(1, dimension->getText()); }
  WriteCodeInt16(71, dimension->GetAttachmentPoint());
  if (dimension->getTextLineStyle() != 1) { WriteCodeInt16(72, dimension->getTextLineStyle()); }
  if (dimension->getTextLineFactor() != 1) { WriteCodeDouble(41, dimension->getTextLineFactor()); }
  WriteCodeWideString(3, dimension->getStyle());
  if (dimension->getTextLineFactor() != 0) { WriteCodeDouble(53, dimension->getDir()); }
  WriteCodeDouble(210, dimension->getExtrusion().x);
  WriteCodeDouble(220, dimension->getExtrusion().y);
  WriteCodeDouble(230, dimension->getExtrusion().z);

  switch (dimension->m_entityType) {
    case EoDxf::DIMALIGNED:
    case EoDxf::DIMLINEAR: {
      auto* alignedDimension = dynamic_cast<EoDxfAlignedDimension*>(dimension);
      WriteCodeString(100, L"AcDbAlignedDimension");
      auto crd = alignedDimension->getClonepoint();
      if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
        WriteCodeDouble(12, crd.x);
        WriteCodeDouble(22, crd.y);
        WriteCodeDouble(32, crd.z);
      }
      WriteCodeDouble(13, alignedDimension->getDef1Point().x);
      WriteCodeDouble(23, alignedDimension->getDef1Point().y);
      WriteCodeDouble(33, alignedDimension->getDef1Point().z);
      WriteCodeDouble(14, alignedDimension->getDef2Point().x);
      WriteCodeDouble(24, alignedDimension->getDef2Point().y);
      WriteCodeDouble(34, alignedDimension->getDef2Point().z);
      if (dimension->m_entityType == EoDxf::DIMLINEAR) {
        auto* dl = dynamic_cast<EoDxfDimLinear*>(dimension);
        if (dl->getAngle() != 0) { WriteCodeDouble(50, dl->getAngle()); }
        if (dl->getOblique() != 0) { WriteCodeDouble(52, dl->getOblique()); }
        WriteCodeString(100, L"AcDbRotatedDimension");
      }
      break;
    }
    case EoDxf::DIMRADIAL: {
      auto* radialDimension = dynamic_cast<EoDxfRadialDimension*>(dimension);
      WriteCodeString(100, L"AcDbRadialDimension");
      WriteCodeDouble(15, radialDimension->getDiameterPoint().x);
      WriteCodeDouble(25, radialDimension->getDiameterPoint().y);
      WriteCodeDouble(35, radialDimension->getDiameterPoint().z);
      WriteCodeDouble(40, radialDimension->getLeaderLength());
      break;
    }
    case EoDxf::DIMDIAMETRIC: {
      auto* diametricDimension = dynamic_cast<EoDxfDiametricDimension*>(dimension);
      WriteCodeString(100, L"AcDbDiametricDimension");
      WriteCodeDouble(15, diametricDimension->getDiameter1Point().x);
      WriteCodeDouble(25, diametricDimension->getDiameter1Point().y);
      WriteCodeDouble(35, diametricDimension->getDiameter1Point().z);
      WriteCodeDouble(40, diametricDimension->getLeaderLength());
      break;
    }
    case EoDxf::DIMANGULAR: {
      auto* _2LineAngularDimension = dynamic_cast<EoDxf2LineAngularDimension*>(dimension);
      WriteCodeString(100, L"AcDb2LineAngularDimension");
      WriteCodeDouble(13, _2LineAngularDimension->getFirstLine1().x);
      WriteCodeDouble(23, _2LineAngularDimension->getFirstLine1().y);
      WriteCodeDouble(33, _2LineAngularDimension->getFirstLine1().z);
      WriteCodeDouble(14, _2LineAngularDimension->getFirstLine2().x);
      WriteCodeDouble(24, _2LineAngularDimension->getFirstLine2().y);
      WriteCodeDouble(34, _2LineAngularDimension->getFirstLine2().z);
      WriteCodeDouble(15, _2LineAngularDimension->getSecondLine1().x);
      WriteCodeDouble(25, _2LineAngularDimension->getSecondLine1().y);
      WriteCodeDouble(35, _2LineAngularDimension->getSecondLine1().z);
      WriteCodeDouble(16, _2LineAngularDimension->getDimPoint().x);
      WriteCodeDouble(26, _2LineAngularDimension->getDimPoint().y);
      WriteCodeDouble(36, _2LineAngularDimension->getDimPoint().z);
      break;
    }
    case EoDxf::DIMANGULAR3P: {
      auto* _3PointAngularDimension = dynamic_cast<EoDxf3PointAngularDimension*>(dimension);
      WriteCodeString(100, L"AcDb3PointAngularDimension");
      WriteCodeDouble(13, _3PointAngularDimension->getFirstLine().x);
      WriteCodeDouble(23, _3PointAngularDimension->getFirstLine().y);
      WriteCodeDouble(33, _3PointAngularDimension->getFirstLine().z);
      WriteCodeDouble(14, _3PointAngularDimension->getSecondLine().x);
      WriteCodeDouble(24, _3PointAngularDimension->getSecondLine().y);
      WriteCodeDouble(34, _3PointAngularDimension->getSecondLine().z);
      WriteCodeDouble(15, _3PointAngularDimension->getVertexPoint().x);
      WriteCodeDouble(25, _3PointAngularDimension->getVertexPoint().y);
      WriteCodeDouble(35, _3PointAngularDimension->getVertexPoint().z);
      break;
    }
    case EoDxf::DIMORDINATE: {
      auto* ordinateDimension = dynamic_cast<EoDxfOrdinateDimension*>(dimension);
      WriteCodeString(100, L"AcDbOrdinateDimension");
      WriteCodeDouble(13, ordinateDimension->getFirstLine().x);
      WriteCodeDouble(23, ordinateDimension->getFirstLine().y);
      WriteCodeDouble(33, ordinateDimension->getFirstLine().z);
      WriteCodeDouble(14, ordinateDimension->getSecondLine().x);
      WriteCodeDouble(24, ordinateDimension->getSecondLine().y);
      WriteCodeDouble(34, ordinateDimension->getSecondLine().z);
      break;
    }
    default:
      break;
  }

  return m_writeOk;
}

bool EoDxfWrite::WriteLeader(EoDxfLeader* leader) {
  WriteCodeString(0, L"LEADER");
  WriteEntity(leader);
  WriteCodeString(100, L"AcDbLeader");
  WriteCodeWideString(3, leader->m_dimensionStyleName);

  if (leader->m_arrowheadFlag != 1) { WriteCodeInt16(71, leader->m_arrowheadFlag); }
  if (leader->m_leaderPathType != 0) { WriteCodeInt16(72, leader->m_leaderPathType); }
  if (leader->m_leaderCreationFlag != 3) { WriteCodeInt16(73, leader->m_leaderCreationFlag); }
  if (leader->m_hookLineDirection != 1) { WriteCodeInt16(74, leader->m_hookLineDirection); }
  if (leader->m_hookLineFlag != 0) { WriteCodeInt16(75, leader->m_hookLineFlag); }

  if (std::abs(leader->m_textAnnotationHeight) > EoDxf::numericEpsilon) {
    WriteCodeDouble(40, leader->m_textAnnotationHeight);
  }
  if (std::abs(leader->m_textAnnotationWidth) > EoDxf::numericEpsilon) {
    WriteCodeDouble(41, leader->m_textAnnotationWidth);
  }

  WriteCodeInt16(76, static_cast<std::int16_t>(leader->m_vertexList.size()));

  for (const auto& vertex : leader->m_vertexList) {
    WriteCodeDouble(10, vertex.x);
    WriteCodeDouble(20, vertex.y);
    WriteCodeDouble(30, vertex.z);
  }
  if (leader->m_colorToUse != 0) { WriteCodeInt16(77, leader->m_colorToUse); }

  if (leader->m_associatedAnnotationHandle != EoDxf::NoHandle) {
    WriteCodeString(340, ToHexString(leader->m_associatedAnnotationHandle));
  }

  if (!leader->m_normalVector.IsDefaultNormal()) {
    WriteCodeDouble(210, leader->m_normalVector.x);
    WriteCodeDouble(220, leader->m_normalVector.y);
    WriteCodeDouble(230, leader->m_normalVector.z);
  }
  if (!leader->m_horizontalDirectionForLeader.IsEqualTo({1.0, 0.0, 0.0})) {
    WriteCodeDouble(211, leader->m_horizontalDirectionForLeader.x);
    WriteCodeDouble(221, leader->m_horizontalDirectionForLeader.y);
    WriteCodeDouble(231, leader->m_horizontalDirectionForLeader.z);
  }
  if (!leader->m_offsetFromBlockInsertionPoint.IsZero()) {
    WriteCodeDouble(212, leader->m_offsetFromBlockInsertionPoint.x);
    WriteCodeDouble(222, leader->m_offsetFromBlockInsertionPoint.y);
    WriteCodeDouble(232, leader->m_offsetFromBlockInsertionPoint.z);
  }
  if (!leader->m_offsetFromAnnotationPlacementPoint.IsZero()) {
    WriteCodeDouble(213, leader->m_offsetFromAnnotationPlacementPoint.x);
    WriteCodeDouble(223, leader->m_offsetFromAnnotationPlacementPoint.y);
    WriteCodeDouble(233, leader->m_offsetFromAnnotationPlacementPoint.z);
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteMLeader(EoDxfMLeader* mLeader) {
  WriteCodeString(0, L"MULTILEADER");
  WriteEntity(mLeader);
  WriteCodeString(100, L"AcDbMLeader");

  // --- Top-level properties ---
  WriteCodeInt16(170, mLeader->m_leaderType);
  WriteCodeInt32(91, mLeader->m_leaderLineColor);

  if (mLeader->m_leaderLineTypeHandle != EoDxf::NoHandle) {
    WriteCodeString(341, ToHexString(mLeader->m_leaderLineTypeHandle));
  }
  WriteCodeInt16(171, mLeader->m_leaderLineWeight);
  WriteCodeBool(290, mLeader->m_enableLanding);
  WriteCodeBool(291, mLeader->m_enableDogleg);
  WriteCodeDouble(41, mLeader->m_doglegLength);
  WriteCodeDouble(42, mLeader->m_arrowheadSize);
  WriteCodeInt16(172, mLeader->m_contentType);

  if (mLeader->m_textStyleHandle != EoDxf::NoHandle) { WriteCodeString(343, ToHexString(mLeader->m_textStyleHandle)); }
  WriteCodeInt16(173, mLeader->m_textLeftAttachmentType);
  WriteCodeInt32(95, mLeader->m_textRightAttachmentType);
  WriteCodeInt16(174, mLeader->m_textAngleType);
  WriteCodeInt16(175, mLeader->m_textAlignmentType);
  WriteCodeInt32(92, mLeader->m_textColor);
  WriteCodeBool(292, mLeader->m_enableFrameText);

  if (mLeader->m_blockContentHandle != EoDxf::NoHandle) {
    WriteCodeString(344, ToHexString(mLeader->m_blockContentHandle));
  }
  WriteCodeInt32(93, mLeader->m_blockContentColor);
  WriteCodeDouble(10, mLeader->m_blockContentScale.x);
  WriteCodeDouble(20, mLeader->m_blockContentScale.y);
  WriteCodeDouble(30, mLeader->m_blockContentScale.z);
  WriteCodeDouble(43, mLeader->m_blockContentRotation);
  WriteCodeInt16(176, mLeader->m_blockContentConnectionType);
  WriteCodeBool(293, mLeader->m_enableAnnotationScale);

  if (mLeader->m_leaderStyleHandle != EoDxf::NoHandle) {
    WriteCodeString(340, ToHexString(mLeader->m_leaderStyleHandle));
  }
  WriteCodeInt32(90, mLeader->m_propertyOverrideFlag);
  WriteCodeDouble(45, mLeader->m_overallScale);
  WriteCodeBool(294, mLeader->m_textDirectionNegative);
  WriteCodeInt16(271, mLeader->m_textTopAttachmentType);
  WriteCodeInt16(272, mLeader->m_textBottomAttachmentType);

  // --- CONTEXT_DATA ---
  const auto& contextData = mLeader->m_contextData;
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

bool EoDxfWrite::WriteMText(EoDxfMText* mText) {
  WriteCodeString(0, L"MTEXT");
  WriteEntity(mText);
  WriteCodeString(100, L"AcDbMText");
  WriteCodeDouble(10, mText->m_firstPoint.x);
  WriteCodeDouble(20, mText->m_firstPoint.y);
  WriteCodeDouble(30, mText->m_firstPoint.z);
  WriteCodeDouble(40, mText->m_textHeight);
  WriteCodeDouble(41, mText->m_scaleFactorWidth);
  WriteCodeInt16(71, mText->m_textGenerationFlags);
  WriteCodeInt16(72, static_cast<std::int16_t>(mText->m_horizontalAlignment));

  size_t chunkOffset{};
  for (; (mText->m_string.size() - chunkOffset) > EoDxf::StringGroupCodeMaxChunk;
      chunkOffset += EoDxf::StringGroupCodeMaxChunk) {
    WriteCodeWideString(3, mText->m_string.substr(chunkOffset, EoDxf::StringGroupCodeMaxChunk));
  }
  WriteCodeWideString(1, mText->m_string.substr(chunkOffset));
  WriteCodeWideString(7, mText->m_textStyleName);
  WriteCodeDouble(210, mText->m_extrusionDirection.x);
  WriteCodeDouble(220, mText->m_extrusionDirection.y);
  WriteCodeDouble(230, mText->m_extrusionDirection.z);
  WriteCodeDouble(50, mText->m_textRotation);
  WriteCodeInt16(73, static_cast<std::int16_t>(mText->m_verticalAlignment));
  WriteCodeDouble(44, mText->m_lineSpacingFactor);

  return m_writeOk;
}

bool EoDxfWrite::WriteText(EoDxfText* text) {
  WriteCodeString(0, L"TEXT");
  WriteEntity(text);
  WriteCodeString(100, L"AcDbText");

  WriteCodeDouble(10, text->m_firstPoint.x);
  WriteCodeDouble(20, text->m_firstPoint.y);
  WriteCodeDouble(30, text->m_firstPoint.z);
  WriteCodeDouble(40, text->m_textHeight);
  WriteCodeWideString(1, text->m_string);
  WriteCodeDouble(50, text->m_textRotation);
  WriteCodeDouble(41, text->m_scaleFactorWidth);
  WriteCodeDouble(51, text->m_obliqueAngle);

  WriteCodeWideString(7, text->m_textStyleName);

  WriteCodeInt16(71, text->m_textGenerationFlags);
  if (text->m_horizontalAlignment != EoDxfText::HAlign::Left) {
    WriteCodeInt16(72, static_cast<std::int16_t>(text->m_horizontalAlignment));
  }
  if (text->m_horizontalAlignment != EoDxfText::HAlign::Left ||
      text->m_verticalAlignment != EoDxfText::VAlign::BaseLine) {
    WriteCodeDouble(11, text->m_secondPoint.x);
    WriteCodeDouble(21, text->m_secondPoint.y);
    WriteCodeDouble(31, text->m_secondPoint.z);
  }
  WriteCodeDouble(210, text->m_extrusionDirection.x);
  WriteCodeDouble(220, text->m_extrusionDirection.y);
  WriteCodeDouble(230, text->m_extrusionDirection.z);
  WriteCodeString(100, L"AcDbText");
  if (text->m_verticalAlignment != EoDxfText::VAlign::BaseLine) {
    WriteCodeInt16(73, static_cast<std::int16_t>(text->m_verticalAlignment));
  }
  return m_writeOk;
}
