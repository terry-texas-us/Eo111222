#include <string>

#include "EoDxfAttributes.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

void EoDxfAttDef::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_insertionPoint);
  }
}

void EoDxfAttDef::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_insertionPoint.x = reader.GetDouble();
      break;
    case 20:
      m_insertionPoint.y = reader.GetDouble();
      break;
    case 30:
      m_insertionPoint.z = reader.GetDouble();
      break;
    case 11:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.x = reader.GetDouble();
      break;
    case 21:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.y = reader.GetDouble();
      break;
    case 31:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.z = reader.GetDouble();
      break;
    case 40:
      m_textHeight = reader.GetDouble();
      break;
    case 41:
      m_relativeXScaleFactor = reader.GetDouble();
      break;
    case 50:
      m_textRotation = reader.GetDouble();
      break;
    case 51:
      m_obliqueAngle = reader.GetDouble();
      break;
    case 1:
      m_defaultValue = reader.GetWideString();
      break;
    case 2:
      m_tagString = reader.GetWideString();
      break;
    case 3:
      m_promptString = reader.GetWideString();
      break;
    case 7:
      m_textStyleName = reader.GetWideString();
      break;
    case 70:
      m_attributeFlags = reader.GetInt16();
      break;
    case 71:
      m_textGenerationFlags = reader.GetInt16();
      break;
    case 72:
      m_horizontalTextJustification = reader.GetInt16();
      break;
    case 73:
      m_fieldLength = reader.GetInt16();
      break;
    case 74:
      m_verticalTextJustification = reader.GetInt16();
      break;
    case 280:
      m_versionNumber = reader.GetInt16();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfAttrib::ApplyExtrusion() {
  if (m_haveExtrusion) {
    CalculateArbitraryAxis(m_extrusionDirection);
    ExtrudePointInPlace(m_extrusionDirection, m_firstAlignmentPoint);
  }
}

void EoDxfAttrib::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 10:
      m_firstAlignmentPoint.x = reader.GetDouble();
      break;
    case 20:
      m_firstAlignmentPoint.y = reader.GetDouble();
      break;
    case 30:
      m_firstAlignmentPoint.z = reader.GetDouble();
      break;
    case 11:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.x = reader.GetDouble();
      break;
    case 21:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.y = reader.GetDouble();
      break;
    case 31:
      m_hasSecondAlignmentPoint = true;
      m_secondAlignmentPoint.z = reader.GetDouble();
      break;
    case 40:
      m_textHeight = reader.GetDouble();
      break;
    case 41:
      m_relativeXScaleFactor = reader.GetDouble();
      break;
    case 50:
      m_textRotation = reader.GetDouble();
      break;
    case 51:
      m_obliqueAngle = reader.GetDouble();
      break;
    case 1:
      m_attributeValue = reader.GetWideString();
      break;
    case 2:
      m_tagString = reader.GetWideString();
      break;
    case 7:
      m_textStyleName = reader.GetWideString();
      break;
    case 70:
      m_attributeFlags = reader.GetInt16();
      break;
    case 71:
      m_textGenerationFlags = reader.GetInt16();
      break;
    case 72:
      m_horizontalTextJustification = reader.GetInt16();
      break;
    case 73:
      m_fieldLength = reader.GetInt16();
      break;
    case 74:
      m_verticalTextJustification = reader.GetInt16();
      break;
    case 280:
      m_versionNumber = reader.GetInt16();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}
