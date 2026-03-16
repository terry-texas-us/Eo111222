#include <string>

#include "EoDxfAttributes.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

void EoDxfAttributeBase::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_firstAlignmentPoint);
    if (m_hasSecondAlignmentPoint) {
      ExtrudePointInPlace(m_extrusionDirection, m_secondAlignmentPoint);
    }
  }
}

bool EoDxfAttributeBase::ParseBaseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_firstAlignmentPoint.x = reader.GetDouble();
      return true;
    case 20:
      m_firstAlignmentPoint.y = reader.GetDouble();
      return true;
    case 30:
      m_firstAlignmentPoint.z = reader.GetDouble();
      return true;
    case 11:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.x = reader.GetDouble();
      return true;
    case 21:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.y = reader.GetDouble();
      return true;
    case 31:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.z = reader.GetDouble();
      return true;
    case 40:
      m_textHeight = reader.GetDouble();
      return true;
    case 41:
      m_relativeXScaleFactor = reader.GetDouble();
      return true;
    case 50:
      m_textRotation = reader.GetDouble();
      return true;
    case 51:
      m_obliqueAngle = reader.GetDouble();
      return true;
    case 2:
      m_tagString = reader.GetWideString();
      return true;
    case 7:
      m_textStyleName = reader.GetWideString();
      return true;
    case 70:
      m_attributeFlags = reader.GetInt16();
      return true;
    case 71:
      m_textGenerationFlags = reader.GetInt16();
      return true;
    case 72:
      m_horizontalTextJustification = reader.GetInt16();
      return true;
    case 73:
      m_fieldLength = reader.GetInt16();
      return true;
    case 74:
      m_verticalTextJustification = reader.GetInt16();
      return true;
    case 280:
      m_versionNumber = reader.GetInt16();
      return true;
    default:
      return false;
  }
}

void EoDxfAttDef::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 1:
      m_defaultValue = reader.GetWideString();
      break;
    case 3:
      m_promptString = reader.GetWideString();
      break;
    default:
      if (!ParseBaseCode(code, reader)) {
        EoDxfGraphic::ParseCode(code, reader);
      }
      break;
  }
}

void EoDxfAttrib::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 1:
      m_attributeValue = reader.GetWideString();
      break;
    default:
      if (!ParseBaseCode(code, reader)) {
        EoDxfGraphic::ParseCode(code, reader);
      }
      break;
  }
}
