#pragma once

class AeSysView;
class EoDbLineType;

namespace polyline {
void BeginLineStrip();
void BeginLineLoop();
void __Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray, EoDbLineType* lineType);
void __End(AeSysView* view, CDC* deviceContext, std::int16_t lineType);
/// <summary>Determines points necessary to represent an N-Polygon with line segments.</summary>
void GeneratePointsForNPoly(EoGePoint3d& centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis,
                            int numberOfPoints, EoGePoint3dArray& pts);
/// <summary>Determines how many times (if any), a line segment intersects with polyline.</summary>
bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections);
bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj);
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2);
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2, const EoGePoint3dArray& pts);
void SetVertex(const EoGePoint3d& point);
}  // namespace polyline
