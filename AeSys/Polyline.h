#pragma once

class AeSysView;

namespace polyline
{
	void BeginLineStrip();
	void BeginLineLoop();
	void __Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray, EoDbLineType* lineType);
	void __End(AeSysView* view, CDC* deviceContext, EoInt16 lineType);
	/// <summary>Determines points necessary to represent an N-Polygon with line segments.</summary>
	/// centerPoint		polygon center point
	/// majorAxis 	x-axis reference vector
	/// minorAxis 	y-axis reference vector
	/// numberOfPoints		number of points defining polygon
	/// pts			points defining polygon
	void GeneratePointsForNPoly(EoGePoint3d& centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, int numberOfPoints, CPnts& pts);
	/// <summary>Determines how many times (if any), a line segment intersects with polyline.</summary>
	bool SelectUsingLine(AeSysView* view, EoGeLine line, CPnts& ptsInt);
	bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj);
	bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2);
	bool SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2, const CPnts& pts);
	void SetVertex(const EoGePoint3d& point);
}