#include "Stdafx.h"

#include <cmath>

#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoDbPolyline.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoPowerGeometry.h"
#include "Resource.h"

namespace Power {

void FillHomeRunArrow(EoDbGroup* group, EoGePoint3d& pointOnCircuit, const EoGePoint3d& endPoint) {
  EoGePoint3dArray points;
  points.SetSize(3);

  points[0] = pointOnCircuit.ProjectToward(endPoint, 0.05);

  const EoGeLine circuit(points[0], endPoint);

  circuit.ProjPtFrom_xy(0.0, -0.075, &points[0]);
  points[1] = pointOnCircuit;
  circuit.ProjPtFrom_xy(0.0, 0.075, &points[2]);

  group->AddTail(new EoDbPolyline(2, 1, points));
}

bool FillConductorSymbol(int conductorType, EoDbGroup* group,
    const EoGePoint3d& pointOnCircuit,
    const EoGePoint3d& endPoint) {
  EoGePoint3d points[5]{};
  const EoGeLine circuit(pointOnCircuit, endPoint);

  switch (conductorType) {
    case ID_OP4: {
      circuit.ProjPtFrom_xy(0.0, -0.1, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.075, &points[1]);
      circuit.ProjPtFrom_xy(0.0, 0.0875, &points[2]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));

      auto* circle = EoDbConic::CreateCircleInView(points[2], 0.0125);
      if (circle != nullptr) {
        circle->SetColor(1);
        circle->SetLineTypeName(L"CONTINUOUS");
        group->AddTail(circle);
      }
    } break;

    case ID_OP5:
      circuit.ProjPtFrom_xy(0.0, -0.1, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.1, &points[1]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));
      break;

    case ID_OP6:
      circuit.ProjPtFrom_xy(0.0, -0.1, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.05, &points[1]);

      points[2] = pointOnCircuit.ProjectToward(endPoint, 0.025);

      EoGeLine(points[2], endPoint).ProjPtFrom_xy(0.0, 0.075, &points[3]);
      EoGeLine(pointOnCircuit, endPoint).ProjPtFrom_xy(0.0, 0.1, &points[4]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));
      group->AddTail(EoDbLine::CreateLine(points[1], points[3])->WithProperties(1, L"CONTINUOUS"));
      group->AddTail(EoDbLine::CreateLine(points[3], points[4])->WithProperties(1, L"CONTINUOUS"));
      break;

    case ID_OP7:
      circuit.ProjPtFrom_xy(0.0, -0.05, &points[0]);
      circuit.ProjPtFrom_xy(0.0, 0.05, &points[1]);
      group->AddTail(EoDbLine::CreateLine(points[0], points[1])->WithProperties(1, L"CONTINUOUS"));
      break;

    default:
      return false;
  }
  return true;
}

}  // namespace Power
