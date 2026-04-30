#include "Stdafx.h"

#include "EoDbGroup.h"
#include "EoDbLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"
#include "EoPipeGeometry.h"
#include "Eo.h"
#include "EoGsRenderState.h"
#include "Resource.h"

namespace Pipe {

bool GenerateTickMark(const EoGePoint3d& begin,
    const EoGePoint3d& end,
    double distance,
    double ticSize,
    EoDbGroup* group) {
  const auto pointOnLine = begin.ProjectToward(end, distance);

  EoGeVector3d projection(pointOnLine, end);

  const double distanceToEndPoint = projection.Length();
  const bool markGenerated = distanceToEndPoint > Eo::geometricTolerance;
  if (markGenerated) {
    projection *= ticSize / distanceToEndPoint;

    EoGePoint3d pt1(pointOnLine);
    pt1 += EoGeVector3d(projection.y, -projection.x, 0.0);

    EoGePoint3d pt2(pointOnLine);
    pt2 += EoGeVector3d(-projection.y, projection.x, 0.0);
    group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(1, L"CONTINUOUS"));
  }
  return markGenerated;
}

void GenerateLineWithFittings(int beginType,
    const EoGePoint3d& begin,
    int endType,
    const EoGePoint3d& end,
    double riseDropRadius,
    double ticSize,
    EoDbGroup* group) {
  EoGePoint3d pt1 = begin;
  EoGePoint3d pt2 = end;

  if (beginType == ID_OP3) {
    // Previous fitting is an elbow or side tee
    GenerateTickMark(begin, end, riseDropRadius, ticSize, group);
  } else if (beginType == ID_OP4) {  // Previous fitting is an elbow down, riser down or bottom tee
    pt1 = begin.ProjectToward(end, riseDropRadius);
    GenerateTickMark(pt1, end, riseDropRadius, ticSize, group);
  } else if (beginType == ID_OP5) {
    // Previous fitting is an elbow up, riser up or top tee
    GenerateTickMark(begin, end, 2.0 * riseDropRadius, ticSize, group);
  }

  if (endType == ID_OP3) {
    // Current fitting is an elbow or side tee
    GenerateTickMark(end, begin, riseDropRadius, ticSize, group);
  } else if (endType == ID_OP4) {
    // Current fitting is an elbow down, riser down or bottom tee
    GenerateTickMark(end, begin, 2.0 * riseDropRadius, ticSize, group);
  } else if (endType == ID_OP5) {  // Current fitting is an elbow up, riser up or top tee
    pt2 = end.ProjectToward(begin, riseDropRadius);
    GenerateTickMark(end, begin, 2.0 * riseDropRadius, ticSize, group);
  }
  group->AddTail(EoDbLine::CreateLine(pt1, pt2)->WithProperties(Gs::renderState));
}

}  // namespace Pipe
