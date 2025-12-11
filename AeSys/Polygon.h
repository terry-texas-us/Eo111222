#pragma once

/// <summary>A fill area set primative with interior style hatch is generated using ines.</summary>
// Parameters:	deviceContext
//				iSets		number of point lists
//				iPtLstsId	starting indicies for point lists
void DisplayFilAreaHatch(AeSysView* view, CDC* deviceContext, EoGeTransformMatrix& tm, const int iSets, const int* iPtLstsId, EoGePoint3d*);
/// <summary>Generates polygon.</summary>
// The polygon is closed automatically by drawing a line from the last vertex to the first.
// Arrays of vertices are previously modelview transformed and clipped to view volume.
void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray);
/// <summary>Sutherland-hodgman-like polygon clip by clip plane.</summary>
/// <remarks>Visibility determined using dot product.</remarks>
// ptaIn	coordinates of vetices of input polygon
// ptQ 	coordinates of any point on clip plane
// planeNormal	normal vector of clip plane
// ptaOut	coordinates of vertices of output polygon
void Polygon_IntersectionWithPln(EoGePoint4dArray& pointsArrayIn, const EoGePoint4d& ptQ, EoGeVector3d& planeNormal, EoGePoint4dArray& pointsArrayOut);
