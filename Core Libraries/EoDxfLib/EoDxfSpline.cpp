#include <vector>

#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoDxfReader.h"
#include "EoDxfSpline.h"

void EoDxfSpline::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 210:
      m_normalVector.x = reader.GetDouble();
      break;
    case 220:
      m_normalVector.y = reader.GetDouble();
      break;
    case 230:
      m_normalVector.z = reader.GetDouble();
      break;
    case 12:
      m_startTangent.x = reader.GetDouble();
      break;
    case 22:
      m_startTangent.y = reader.GetDouble();
      break;
    case 32:
      m_startTangent.z = reader.GetDouble();
      break;
    case 13:
      m_endTangent.x = reader.GetDouble();
      break;
    case 23:
      m_endTangent.y = reader.GetDouble();
      break;
    case 33:
      m_endTangent.z = reader.GetDouble();
      break;
    case 70:
      m_splineFlag = reader.GetInt16();
      break;
    case 71:
      m_degreeOfTheSplineCurve = reader.GetInt16();
      break;
    case 72:
      m_numberOfKnots = reader.GetInt16();
      break;
    case 73:
      m_numberOfControlPoints = reader.GetInt16();
      break;
    case 74:
      m_numberOfFitPoints = reader.GetInt16();
      break;
    case 42:
      m_knotTolerance = reader.GetDouble();
      break;
    case 43:
      m_controlPointTolerance = reader.GetDouble();
      break;
    case 44:
      m_fitTolerance = reader.GetDouble();
      break;
    case 10:
      m_controlPoint = new EoDxfGeometryBase3d();
      m_controlPoints.push_back(m_controlPoint);
      m_controlPoint->x = reader.GetDouble();
      break;
    case 20:
      if (m_controlPoint != nullptr) { m_controlPoint->y = reader.GetDouble(); }
      break;
    case 30:
      if (m_controlPoint != nullptr) { m_controlPoint->z = reader.GetDouble(); }
      break;
    case 11:
      m_fitPoint = new EoDxfGeometryBase3d();
      m_fitPoints.push_back(m_fitPoint);
      m_fitPoint->x = reader.GetDouble();
      break;
    case 21:
      if (m_fitPoint != nullptr) { m_fitPoint->y = reader.GetDouble(); }
      break;
    case 31:
      if (m_fitPoint != nullptr) { m_fitPoint->z = reader.GetDouble(); }
      break;
    case 40:
      m_knotValues.push_back(reader.GetDouble());
      break;
    case 41:
      m_weightValues.push_back(reader.GetDouble());
      break;
    default:
      EoDxfGraphic::ParseCode(code, reader);
      break;
  }
}
