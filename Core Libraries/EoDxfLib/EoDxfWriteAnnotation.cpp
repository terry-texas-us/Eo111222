#include <string>

#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfMLeader.h"
#include "EoDxfWriter.h"

bool EoDxfWrite::WriteDimension(EoDxfDimension* dimension) {
  m_writer->WriteString(0, "DIMENSION");
  WriteEntity(dimension);
  m_writer->WriteString(100, "AcDbDimension");
  if (!dimension->getName().empty()) { m_writer->WriteString(2, dimension->getName()); }
  m_writer->WriteDouble(10, dimension->getDefPoint().x);
  m_writer->WriteDouble(20, dimension->getDefPoint().y);
  m_writer->WriteDouble(30, dimension->getDefPoint().z);
  m_writer->WriteDouble(11, dimension->getTextPoint().x);
  m_writer->WriteDouble(21, dimension->getTextPoint().y);
  m_writer->WriteDouble(31, dimension->getTextPoint().z);
  if (!(dimension->type & 32)) { dimension->type = dimension->type + 32; }
  m_writer->WriteInt16(70, dimension->type);
  if (!(dimension->getText().empty())) { m_writer->WriteUtf8String(1, dimension->getText()); }
  m_writer->WriteInt16(71, dimension->getAlign());
  if (dimension->getTextLineStyle() != 1) { m_writer->WriteInt16(72, dimension->getTextLineStyle()); }
  if (dimension->getTextLineFactor() != 1) { m_writer->WriteDouble(41, dimension->getTextLineFactor()); }
  m_writer->WriteUtf8String(3, dimension->getStyle());
  if (dimension->getTextLineFactor() != 0) { m_writer->WriteDouble(53, dimension->getDir()); }
  m_writer->WriteDouble(210, dimension->getExtrusion().x);
  m_writer->WriteDouble(220, dimension->getExtrusion().y);
  m_writer->WriteDouble(230, dimension->getExtrusion().z);

  switch (dimension->m_entityType) {
    case EoDxf::DIMALIGNED:
    case EoDxf::DIMLINEAR: {
      auto* alignedDimension = dynamic_cast<EoDxfAlignedDimension*>(dimension);
      m_writer->WriteString(100, "AcDbAlignedDimension");
      auto crd = alignedDimension->getClonepoint();
      if (crd.x != 0 || crd.y != 0 || crd.z != 0) {
        m_writer->WriteDouble(12, crd.x);
        m_writer->WriteDouble(22, crd.y);
        m_writer->WriteDouble(32, crd.z);
      }
      m_writer->WriteDouble(13, alignedDimension->getDef1Point().x);
      m_writer->WriteDouble(23, alignedDimension->getDef1Point().y);
      m_writer->WriteDouble(33, alignedDimension->getDef1Point().z);
      m_writer->WriteDouble(14, alignedDimension->getDef2Point().x);
      m_writer->WriteDouble(24, alignedDimension->getDef2Point().y);
      m_writer->WriteDouble(34, alignedDimension->getDef2Point().z);
      if (dimension->m_entityType == EoDxf::DIMLINEAR) {
        auto* dl = dynamic_cast<EoDxfDimLinear*>(dimension);
        if (dl->getAngle() != 0) { m_writer->WriteDouble(50, dl->getAngle()); }
        if (dl->getOblique() != 0) { m_writer->WriteDouble(52, dl->getOblique()); }
        m_writer->WriteString(100, "AcDbRotatedDimension");
      }
      break;
    }
    case EoDxf::DIMRADIAL: {
      auto* radialDimension = dynamic_cast<EoDxfRadialDimension*>(dimension);
      m_writer->WriteString(100, "AcDbRadialDimension");
      m_writer->WriteDouble(15, radialDimension->getDiameterPoint().x);
      m_writer->WriteDouble(25, radialDimension->getDiameterPoint().y);
      m_writer->WriteDouble(35, radialDimension->getDiameterPoint().z);
      m_writer->WriteDouble(40, radialDimension->getLeaderLength());
      break;
    }
    case EoDxf::DIMDIAMETRIC: {
      auto* diametricDimension = dynamic_cast<EoDxfDiametricDimension*>(dimension);
      m_writer->WriteString(100, "AcDbDiametricDimension");
      m_writer->WriteDouble(15, diametricDimension->getDiameter1Point().x);
      m_writer->WriteDouble(25, diametricDimension->getDiameter1Point().y);
      m_writer->WriteDouble(35, diametricDimension->getDiameter1Point().z);
      m_writer->WriteDouble(40, diametricDimension->getLeaderLength());
      break;
    }
    case EoDxf::DIMANGULAR: {
      auto* _2LineAngularDimension = dynamic_cast<EoDxf2LineAngularDimension*>(dimension);
      m_writer->WriteString(100, "AcDb2LineAngularDimension");
      m_writer->WriteDouble(13, _2LineAngularDimension->getFirstLine1().x);
      m_writer->WriteDouble(23, _2LineAngularDimension->getFirstLine1().y);
      m_writer->WriteDouble(33, _2LineAngularDimension->getFirstLine1().z);
      m_writer->WriteDouble(14, _2LineAngularDimension->getFirstLine2().x);
      m_writer->WriteDouble(24, _2LineAngularDimension->getFirstLine2().y);
      m_writer->WriteDouble(34, _2LineAngularDimension->getFirstLine2().z);
      m_writer->WriteDouble(15, _2LineAngularDimension->getSecondLine1().x);
      m_writer->WriteDouble(25, _2LineAngularDimension->getSecondLine1().y);
      m_writer->WriteDouble(35, _2LineAngularDimension->getSecondLine1().z);
      m_writer->WriteDouble(16, _2LineAngularDimension->getDimPoint().x);
      m_writer->WriteDouble(26, _2LineAngularDimension->getDimPoint().y);
      m_writer->WriteDouble(36, _2LineAngularDimension->getDimPoint().z);
      break;
    }
    case EoDxf::DIMANGULAR3P: {
      auto* _3PointAngularDimension = dynamic_cast<EoDxf3PointAngularDimension*>(dimension);
      m_writer->WriteString(100, "AcDb3PointAngularDimension");
      m_writer->WriteDouble(13, _3PointAngularDimension->getFirstLine().x);
      m_writer->WriteDouble(23, _3PointAngularDimension->getFirstLine().y);
      m_writer->WriteDouble(33, _3PointAngularDimension->getFirstLine().z);
      m_writer->WriteDouble(14, _3PointAngularDimension->getSecondLine().x);
      m_writer->WriteDouble(24, _3PointAngularDimension->getSecondLine().y);
      m_writer->WriteDouble(34, _3PointAngularDimension->getSecondLine().z);
      m_writer->WriteDouble(15, _3PointAngularDimension->getVertexPoint().x);
      m_writer->WriteDouble(25, _3PointAngularDimension->getVertexPoint().y);
      m_writer->WriteDouble(35, _3PointAngularDimension->getVertexPoint().z);
      break;
    }
    case EoDxf::DIMORDINATE: {
      auto* ordinateDimension = dynamic_cast<EoDxfOrdinateDimension*>(dimension);
      m_writer->WriteString(100, "AcDbOrdinateDimension");
      m_writer->WriteDouble(13, ordinateDimension->getFirstLine().x);
      m_writer->WriteDouble(23, ordinateDimension->getFirstLine().y);
      m_writer->WriteDouble(33, ordinateDimension->getFirstLine().z);
      m_writer->WriteDouble(14, ordinateDimension->getSecondLine().x);
      m_writer->WriteDouble(24, ordinateDimension->getSecondLine().y);
      m_writer->WriteDouble(34, ordinateDimension->getSecondLine().z);
      break;
    }
    default:
      break;
  }

  return true;
}

bool EoDxfWrite::WriteLeader(EoDxfLeader* leader) {
  m_writer->WriteString(0, "LEADER");
  WriteEntity(leader);
  m_writer->WriteString(100, "AcDbLeader");
  m_writer->WriteUtf8String(3, leader->m_dimensionStyleName);

  if (leader->m_arrowheadFlag != 1) { m_writer->WriteInt16(71, leader->m_arrowheadFlag); }
  if (leader->m_leaderPathType != 0) { m_writer->WriteInt16(72, leader->m_leaderPathType); }
  if (leader->m_leaderCreationFlag != 3) { m_writer->WriteInt16(73, leader->m_leaderCreationFlag); }
  if (leader->m_hookLineDirection != 1) { m_writer->WriteInt16(74, leader->m_hookLineDirection); }
  if (leader->m_hookLineFlag != 0) { m_writer->WriteInt16(75, leader->m_hookLineFlag); }

  if (std::abs(leader->m_textAnnotationHeight) > EoDxf::numericEpsilon) {
    m_writer->WriteDouble(40, leader->m_textAnnotationHeight);
  }
  if (std::abs(leader->m_textAnnotationWidth) > EoDxf::numericEpsilon) {
    m_writer->WriteDouble(41, leader->m_textAnnotationWidth);
  }

  m_writer->WriteInt16(76, static_cast<int>(leader->m_vertexList.size()));

  for (const auto& vertex : leader->m_vertexList) {
    m_writer->WriteDouble(10, vertex.x);
    m_writer->WriteDouble(20, vertex.y);
    m_writer->WriteDouble(30, vertex.z);
  }
  if (leader->m_colorToUse != 0) { m_writer->WriteInt16(77, leader->m_colorToUse); }

  if (leader->m_associatedAnnotationHandle != EoDxf::HandleCodes::NoHandle) {
    m_writer->WriteString(340, ToHexString(leader->m_associatedAnnotationHandle));
  }

  if (!leader->m_normalVector.IsDefaultNormal()) {
    m_writer->WriteDouble(210, leader->m_normalVector.x);
    m_writer->WriteDouble(220, leader->m_normalVector.y);
    m_writer->WriteDouble(230, leader->m_normalVector.z);
  }
  if (!leader->m_horizontalDirectionForLeader.IsEqualTo({1.0, 0.0, 0.0})) {
    m_writer->WriteDouble(211, leader->m_horizontalDirectionForLeader.x);
    m_writer->WriteDouble(221, leader->m_horizontalDirectionForLeader.y);
    m_writer->WriteDouble(231, leader->m_horizontalDirectionForLeader.z);
  }
  if (!leader->m_offsetFromBlockInsertionPoint.IsZero()) {
    m_writer->WriteDouble(212, leader->m_offsetFromBlockInsertionPoint.x);
    m_writer->WriteDouble(222, leader->m_offsetFromBlockInsertionPoint.y);
    m_writer->WriteDouble(232, leader->m_offsetFromBlockInsertionPoint.z);
  }
  if (!leader->m_offsetFromAnnotationPlacementPoint.IsZero()) {
    m_writer->WriteDouble(213, leader->m_offsetFromAnnotationPlacementPoint.x);
    m_writer->WriteDouble(223, leader->m_offsetFromAnnotationPlacementPoint.y);
    m_writer->WriteDouble(233, leader->m_offsetFromAnnotationPlacementPoint.z);
  }
  return true;
}

bool EoDxfWrite::WriteMLeader(EoDxfMLeader* mLeader) {
  m_writer->WriteString(0, "MULTILEADER");
  WriteEntity(mLeader);
  m_writer->WriteString(100, "AcDbMLeader");

  // --- Top-level properties ---
  m_writer->WriteInt16(170, mLeader->m_leaderType);
  m_writer->WriteInt16(91, mLeader->m_leaderLineColor);

  if (mLeader->m_leaderLineTypeHandle != EoDxf::HandleCodes::NoHandle) {
    m_writer->WriteString(341, ToHexString(mLeader->m_leaderLineTypeHandle));
  }
  m_writer->WriteInt16(171, mLeader->m_leaderLineWeight);
  m_writer->WriteBool(290, mLeader->m_enableLanding);
  m_writer->WriteBool(291, mLeader->m_enableDogleg);
  m_writer->WriteDouble(41, mLeader->m_doglegLength);
  m_writer->WriteDouble(42, mLeader->m_arrowheadSize);
  m_writer->WriteInt16(172, mLeader->m_contentType);

  if (mLeader->m_textStyleHandle != EoDxf::HandleCodes::NoHandle) {
    m_writer->WriteString(343, ToHexString(mLeader->m_textStyleHandle));
  }
  m_writer->WriteInt16(173, mLeader->m_textLeftAttachmentType);
  m_writer->WriteInt16(95, mLeader->m_textRightAttachmentType);
  m_writer->WriteInt16(174, mLeader->m_textAngleType);
  m_writer->WriteInt16(175, mLeader->m_textAlignmentType);
  m_writer->WriteInt32(92, mLeader->m_textColor);
  m_writer->WriteBool(292, mLeader->m_enableFrameText);

  if (mLeader->m_blockContentHandle != EoDxf::HandleCodes::NoHandle) {
    m_writer->WriteString(344, ToHexString(mLeader->m_blockContentHandle));
  }
  m_writer->WriteInt32(93, mLeader->m_blockContentColor);
  m_writer->WriteDouble(10, mLeader->m_blockContentScale.x);
  m_writer->WriteDouble(20, mLeader->m_blockContentScale.y);
  m_writer->WriteDouble(30, mLeader->m_blockContentScale.z);
  m_writer->WriteDouble(43, mLeader->m_blockContentRotation);
  m_writer->WriteInt16(176, mLeader->m_blockContentConnectionType);
  m_writer->WriteBool(293, mLeader->m_enableAnnotationScale);

  if (mLeader->m_leaderStyleHandle != EoDxf::HandleCodes::NoHandle) {
    m_writer->WriteString(340, ToHexString(mLeader->m_leaderStyleHandle));
  }
  m_writer->WriteInt32(90, mLeader->m_propertyOverrideFlag);
  m_writer->WriteDouble(45, mLeader->m_overallScale);
  m_writer->WriteInt16(294, mLeader->m_textDirectionNegative);
  m_writer->WriteInt16(271, mLeader->m_textTopAttachmentType);
  m_writer->WriteInt16(272, mLeader->m_textBottomAttachmentType);

  // --- CONTEXT_DATA ---
  const auto& contextData = mLeader->m_contextData;
  m_writer->WriteString(300, "CONTEXT_DATA{");
  m_writer->WriteDouble(40, contextData.m_contentScale);
  m_writer->WriteDouble(10, contextData.m_contentBasePoint.x);
  m_writer->WriteDouble(20, contextData.m_contentBasePoint.y);
  m_writer->WriteDouble(30, contextData.m_contentBasePoint.z);
  m_writer->WriteDouble(41, contextData.m_textHeight);
  m_writer->WriteDouble(140, contextData.m_arrowheadSize);
  m_writer->WriteDouble(145, contextData.m_landingGap);
  m_writer->WriteInt16(174, contextData.m_textLeftAttachment);
  m_writer->WriteInt16(175, contextData.m_textRightAttachment);
  m_writer->WriteInt16(176, contextData.m_textAlignmentType);
  m_writer->WriteInt16(177, contextData.m_blockContentConnectionType);
  m_writer->WriteBool(290, contextData.m_hasMText);
  m_writer->WriteBool(296, contextData.m_hasContent);
  // --- Leader branches ---
  for (const auto& branch : contextData.m_leaders) {
    m_writer->WriteString(302, "LEADER{");
    m_writer->WriteBool(290, branch.m_hasSetLastLeaderLinePoint);
    m_writer->WriteBool(291, branch.m_hasSetDoglegVector);
    m_writer->WriteDouble(10, branch.m_lastLeaderLinePoint.x);
    m_writer->WriteDouble(20, branch.m_lastLeaderLinePoint.y);
    m_writer->WriteDouble(30, branch.m_lastLeaderLinePoint.z);
    m_writer->WriteDouble(11, branch.m_doglegVector.x);
    m_writer->WriteDouble(21, branch.m_doglegVector.y);
    m_writer->WriteDouble(31, branch.m_doglegVector.z);
    m_writer->WriteInt32(90, branch.m_leaderBranchIndex);
    m_writer->WriteDouble(40, branch.m_doglegLength);

    // --- Leader lines ---
    for (const auto& line : branch.m_leaderLines) {
      m_writer->WriteString(304, "LEADER_LINE{");
      for (const auto& vertex : line.m_vertices) {
        m_writer->WriteDouble(10, vertex.x);
        m_writer->WriteDouble(20, vertex.y);
        m_writer->WriteDouble(30, vertex.z);
      }
      m_writer->WriteInt32(91, line.m_leaderLineIndex);
      if (line.m_leaderLineColorOverride != EoDxf::ColorCodes::ColorByLayer) {
        m_writer->WriteInt32(92, line.m_leaderLineColorOverride);
      }
      if (line.m_leaderLineWeightOverride >= 0) { m_writer->WriteInt16(171, line.m_leaderLineWeightOverride); }
      if (std::abs(line.m_arrowheadSize) > EoDxf::numericEpsilon) { m_writer->WriteDouble(40, line.m_arrowheadSize); }
      if (line.m_arrowheadHandle != EoDxf::HandleCodes::NoHandle) {
        m_writer->WriteString(341, ToHexString(line.m_arrowheadHandle));
      }
      m_writer->WriteString(305, "}");
    }
    m_writer->WriteString(303, "}");
  }

  // --- MText content ---
  if (contextData.m_hasMText) {
    m_writer->WriteString(304, "{");
    m_writer->WriteDouble(12, contextData.m_textLocation.x);
    m_writer->WriteDouble(22, contextData.m_textLocation.y);
    m_writer->WriteDouble(32, contextData.m_textLocation.z);
    m_writer->WriteDouble(13, contextData.m_textDirection.x);
    m_writer->WriteDouble(23, contextData.m_textDirection.y);
    m_writer->WriteDouble(33, contextData.m_textDirection.z);
    m_writer->WriteDouble(42, contextData.m_textRotation);
    m_writer->WriteDouble(43, contextData.m_textWidth);
    m_writer->WriteDouble(44, contextData.m_textDefinedWidth);
    m_writer->WriteDouble(45, contextData.m_textDefinedHeight);
    m_writer->WriteInt16(170, contextData.m_textAttachment);
    m_writer->WriteInt32(90, contextData.m_textFlowDirection);
    m_writer->WriteInt32(91, contextData.m_textColor);
    m_writer->WriteDouble(141, contextData.m_textLineSpacingFactor);
    m_writer->WriteInt16(171, contextData.m_textLineSpacingStyle);
    m_writer->WriteInt16(172, contextData.m_textBackgroundFill);
    if (contextData.m_textStyleHandle != EoDxf::HandleCodes::NoHandle) {
      m_writer->WriteString(340, ToHexString(contextData.m_textStyleHandle));
    }
    if (!contextData.m_textString.empty()) {
      auto text = m_writer->FromUtf8String(contextData.m_textString);
      size_t chunkOffset{};
      for (; (text.size() - chunkOffset) > EoDxf::StringGroupCodeMaxChunk; chunkOffset += EoDxf::StringGroupCodeMaxChunk) {
        m_writer->WriteString(3, text.substr(chunkOffset, EoDxf::StringGroupCodeMaxChunk));
      }
      m_writer->WriteString(1, text.substr(chunkOffset));
    }
    m_writer->WriteString(301, "}");
  }

  // --- Block content (non-MText) ---
  if (contextData.m_blockContentHandle != EoDxf::HandleCodes::NoHandle) {
    m_writer->WriteString(341, ToHexString(contextData.m_blockContentHandle));
    m_writer->WriteDouble(14, contextData.m_blockContentNormalDirection.x);
    m_writer->WriteDouble(24, contextData.m_blockContentNormalDirection.y);
    m_writer->WriteDouble(34, contextData.m_blockContentNormalDirection.z);
    m_writer->WriteDouble(15, contextData.m_blockContentScale.x);
    m_writer->WriteDouble(25, contextData.m_blockContentScale.y);
    m_writer->WriteDouble(35, contextData.m_blockContentScale.z);
    m_writer->WriteDouble(46, contextData.m_blockContentRotation);
    m_writer->WriteInt32(93, contextData.m_blockContentColor);
  }

  m_writer->WriteString(301, "}");  // close CONTEXT_DATA
  return true;
}

bool EoDxfWrite::WriteMText(EoDxfMText* mText) {
  m_writer->WriteString(0, "MTEXT");
  WriteEntity(mText);
  m_writer->WriteString(100, "AcDbMText");
  m_writer->WriteDouble(10, mText->m_firstPoint.x);
  m_writer->WriteDouble(20, mText->m_firstPoint.y);
  m_writer->WriteDouble(30, mText->m_firstPoint.z);
  m_writer->WriteDouble(40, mText->m_textHeight);
  m_writer->WriteDouble(41, mText->m_scaleFactorWidth);
  m_writer->WriteInt16(71, mText->m_textGenerationFlags);
  m_writer->WriteInt16(72, mText->m_horizontalAlignment);
  std::string text = m_writer->FromUtf8String(mText->m_string);

  size_t chunkOffset{};
  for (; (text.size() - chunkOffset) > EoDxf::StringGroupCodeMaxChunk; chunkOffset += EoDxf::StringGroupCodeMaxChunk) {
    m_writer->WriteString(3, text.substr(chunkOffset, EoDxf::StringGroupCodeMaxChunk));
  }
  m_writer->WriteString(1, text.substr(chunkOffset));
  m_writer->WriteString(7, mText->m_textStyleName);
  m_writer->WriteDouble(210, mText->m_extrusionDirection.x);
  m_writer->WriteDouble(220, mText->m_extrusionDirection.y);
  m_writer->WriteDouble(230, mText->m_extrusionDirection.z);
  m_writer->WriteDouble(50, mText->m_textRotation);
  m_writer->WriteInt16(73, mText->m_verticalAlignment);
  m_writer->WriteDouble(44, mText->m_lineSpacingFactor);

  return true;
}

bool EoDxfWrite::WriteText(EoDxfText* text) {
  m_writer->WriteString(0, "TEXT");
  WriteEntity(text);
  m_writer->WriteString(100, "AcDbText");

  m_writer->WriteDouble(10, text->m_firstPoint.x);
  m_writer->WriteDouble(20, text->m_firstPoint.y);
  m_writer->WriteDouble(30, text->m_firstPoint.z);
  m_writer->WriteDouble(40, text->m_textHeight);
  m_writer->WriteUtf8String(1, text->m_string);
  m_writer->WriteDouble(50, text->m_textRotation);
  m_writer->WriteDouble(41, text->m_scaleFactorWidth);
  m_writer->WriteDouble(51, text->m_obliqueAngle);

  m_writer->WriteUtf8String(7, text->m_textStyleName);

  m_writer->WriteInt16(71, text->m_textGenerationFlags);
  if (text->m_horizontalAlignment != EoDxfText::HAlign::Left) { m_writer->WriteInt16(72, text->m_horizontalAlignment); }
  if (text->m_horizontalAlignment != EoDxfText::HAlign::Left ||
      text->m_verticalAlignment != EoDxfText::VAlign::BaseLine) {
    m_writer->WriteDouble(11, text->m_secondPoint.x);
    m_writer->WriteDouble(21, text->m_secondPoint.y);
    m_writer->WriteDouble(31, text->m_secondPoint.z);
  }
  m_writer->WriteDouble(210, text->m_extrusionDirection.x);
  m_writer->WriteDouble(220, text->m_extrusionDirection.y);
  m_writer->WriteDouble(230, text->m_extrusionDirection.z);
  m_writer->WriteString(100, "AcDbText");
  if (text->m_verticalAlignment != EoDxfText::VAlign::BaseLine) { m_writer->WriteInt16(73, text->m_verticalAlignment); }
  return true;
}
