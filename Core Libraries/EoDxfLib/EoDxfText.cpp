#include <algorithm>
#include <string>

#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"
#include "EoDxfText.h"

/** @brief Parses dxf code and value to read text entity data
 *  @param code dxf code
 *  @param reader pointer to EoDxfReader to read value
 */
void EoDxfText::ParseCode(int code, EoDxfReader& reader) {
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
      m_scaleFactorWidth = reader.GetDouble();
      break;
    case 50:
      m_textRotation = reader.GetDouble();
      break;
    case 51:
      m_obliqueAngle = reader.GetDouble();
      break;
    case 71:
      m_textGenerationFlags = reader.GetInt16();
      break;
    case 72:
      m_horizontalAlignment = static_cast<HorizontalAlignment>(
          std::clamp(reader.GetInt16(), static_cast<std::int16_t>(HorizontalAlignment::Left),
              static_cast<std::int16_t>(HorizontalAlignment::FitIfBaseLine)));
      break;
    case 73:
      m_verticalAlignment = static_cast<VerticalAlignment>(std::clamp(reader.GetInt16(),
          static_cast<std::int16_t>(VerticalAlignment::BaseLine), static_cast<std::int16_t>(VerticalAlignment::Top)));
      break;
    case 1:
      m_string = reader.GetWideString();
      break;
    case 7:
      m_textStyleName = reader.GetWideString();
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}
