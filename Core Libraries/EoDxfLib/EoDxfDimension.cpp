#include <string>
#include <vector>

#include "EoDxfDimension.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"

void EoDxfDimension::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 1:
      m_explicitDimensionText = reader.GetWideString();
      break;
    case 2:
      m_nameOfBlockContainer = reader.GetWideString();
      break;
    case 3:
      m_dimensionStyleName = reader.GetWideString();
      break;
    case 70:
      m_dimensionType = reader.GetInt16();
      break;
    case 71:
      m_attachmentPoint = reader.GetInt16();
      break;
    case 72:
      m_dimensionTextLineSpacingStyle = reader.GetInt16();
      break;
    case 10:
      m_definitionPoint.x = reader.GetDouble();
      break;
    case 20:
      m_definitionPoint.y = reader.GetDouble();
      break;
    case 30:
      m_definitionPoint.z = reader.GetDouble();
      break;
    case 11:
      m_middlePointOfDimensionText.x = reader.GetDouble();
      break;
    case 21:
      m_middlePointOfDimensionText.y = reader.GetDouble();
      break;
    case 31:
      m_middlePointOfDimensionText.z = reader.GetDouble();
      break;
    case 12:
      m_clonePoint.x = reader.GetDouble();
      break;
    case 22:
      m_clonePoint.y = reader.GetDouble();
      break;
    case 32:
      m_clonePoint.z = reader.GetDouble();
      break;
    // Subtype-specific accumulator codes — parsed here for all subtypes.
    // Only the codes relevant to the actual subtype (determined later by code 70)
    // will contain meaningful values.
    case 13:
      m_extensionLinePoint1.x = reader.GetDouble();
      break;
    case 23:
      m_extensionLinePoint1.y = reader.GetDouble();
      break;
    case 33:
      m_extensionLinePoint1.z = reader.GetDouble();
      break;
    case 14:
      m_extensionLinePoint2.x = reader.GetDouble();
      break;
    case 24:
      m_extensionLinePoint2.y = reader.GetDouble();
      break;
    case 34:
      m_extensionLinePoint2.z = reader.GetDouble();
      break;
    case 15:
      m_radiusDiameterPoint.x = reader.GetDouble();
      break;
    case 25:
      m_radiusDiameterPoint.y = reader.GetDouble();
      break;
    case 35:
      m_radiusDiameterPoint.z = reader.GetDouble();
      break;
    case 16:
      m_arcDefinitionPoint.x = reader.GetDouble();
      break;
    case 26:
      m_arcDefinitionPoint.y = reader.GetDouble();
      break;
    case 36:
      m_arcDefinitionPoint.z = reader.GetDouble();
      break;
    case 40:
      m_leaderLength = reader.GetDouble();
      break;
    case 50:
      m_rotationAngle = reader.GetDouble();
      break;
    case 52:
      m_obliqueAngle = reader.GetDouble();
      break;
    case 41:
      m_dimensionTextLineSpacingFactor = reader.GetDouble();
      break;
    case 53:
      m_rotationAngleAwayFromDefault = reader.GetDouble();
      break;
    case 51:
      m_horizontalDirection = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}

void EoDxfLeader::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 3:
      m_dimensionStyleName = reader.GetWideString();
      break;
    case 71:
      m_arrowheadFlag = reader.GetInt16();
      break;
    case 72:
      m_leaderPathType = reader.GetInt16();
      break;
    case 73:
      m_leaderCreationFlag = reader.GetInt16();
      break;
    case 74:
      m_hookLineDirection = reader.GetInt16();
      break;
    case 75:
      m_hookLineFlag = reader.GetInt16();
      break;
    case 76:
      m_numberOfVertices = reader.GetInt16();
      m_vertexList.reserve(m_numberOfVertices);
      break;
    case 77:
      m_colorToUse = reader.GetInt16();
      break;
    case 40:
      m_textAnnotationHeight = reader.GetDouble();
      break;
    case 41:
      m_textAnnotationWidth = reader.GetDouble();
      break;
    case 10: {
      m_vertexList.emplace_back();  // value semantics
      m_currentVertexIndex = static_cast<int>(m_vertexList.size()) - 1;
      m_vertexList.back().x = reader.GetDouble();
      break;
    }
    case 20:
      if (m_currentVertexIndex >= 0) { m_vertexList[m_currentVertexIndex].y = reader.GetDouble(); }
      break;
    case 30:
      if (m_currentVertexIndex >= 0) { m_vertexList[m_currentVertexIndex].z = reader.GetDouble(); }
      break;
    case 340:
      m_associatedAnnotationHandle = reader.GetHandleString();
      break;
    case 210:
      m_normalVector.x = reader.GetDouble();
      break;
    case 220:
      m_normalVector.y = reader.GetDouble();
      break;
    case 230:
      m_normalVector.z = reader.GetDouble();
      break;
    case 211:
      m_horizontalDirectionForLeader.x = reader.GetDouble();
      break;
    case 221:
      m_horizontalDirectionForLeader.y = reader.GetDouble();
      break;
    case 231:
      m_horizontalDirectionForLeader.z = reader.GetDouble();
      break;
    case 212:
      m_offsetFromBlockInsertionPoint.x = reader.GetDouble();
      break;
    case 222:
      m_offsetFromBlockInsertionPoint.y = reader.GetDouble();
      break;
    case 232:
      m_offsetFromBlockInsertionPoint.z = reader.GetDouble();
      break;
    case 213:
      m_offsetFromAnnotationPlacementPoint.x = reader.GetDouble();
      break;
    case 223:
      m_offsetFromAnnotationPlacementPoint.y = reader.GetDouble();
      break;
    case 233:
      m_offsetFromAnnotationPlacementPoint.z = reader.GetDouble();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}
