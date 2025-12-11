#include "stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

#include "Polygon.h"

void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray) 
{	
	int iPts = (int) pointsArray.GetSize();
	if (iPts >= 2)
	{
		CPoint* pnt = new CPoint[iPts];
		
		view->DoProjection(pnt, pointsArray);
	
		if (pstate.PolygonIntStyle() == EoDb::kSolid)
		{
			CBrush brush(pColTbl[pstate.PenColor()]);
			CBrush* pBrushOld = deviceContext->SelectObject(&brush);
			deviceContext->Polygon(pnt, iPts);
			deviceContext->SelectObject(pBrushOld);
		}
		else if (pstate.PolygonIntStyle() == EoDb::kHollow)
		{
			CBrush* pBrushOld = (CBrush*) deviceContext->SelectStockObject(NULL_BRUSH);
			deviceContext->Polygon(pnt, iPts);
			deviceContext->SelectObject(pBrushOld);
		}
		else
		{
			deviceContext->Polygon(pnt, iPts);
		}
		delete [] pnt;
	}
}
void Polygon_IntersectionWithPln(EoGePoint4dArray& pointsArrayIn, const EoGePoint4d& ptQ, EoGeVector3d& planeNormal, EoGePoint4dArray& pointsArrayOut)
{
	if (pointsArrayIn.IsEmpty()) return;
	
	EoGePoint4d pt;
	EoGePoint4d ptEdge[2];
	bool bEdgeVis[2];
	
	bool bVisVer0 = EoGeDotProduct(EoGeVector3d(ptQ, pointsArrayIn[0]), planeNormal) >= - DBL_EPSILON ? true : false;
	
	ptEdge[0] = pointsArrayIn[0];
	bEdgeVis[0] = bVisVer0;
	
	if (bVisVer0)
	{
		pointsArrayOut.Add(pointsArrayIn[0]);
	}
	int iPtsIn = (int) pointsArrayIn.GetSize();
	for (int i = 1; i < iPtsIn; i++)
	{	
		ptEdge[1] = pointsArrayIn[i];
		bEdgeVis[1] = EoGeDotProduct(EoGeVector3d(ptQ, ptEdge[1]), planeNormal) >= - DBL_EPSILON ? true : false;
		
		if (bEdgeVis[0] != bEdgeVis[1])
		{	// Vetices of edge on opposite sides of clip plane
			pt = EoGeLine::IntersectionWithPln4(ptEdge[0], ptEdge[1], ptQ, planeNormal);
			pointsArrayOut.Add(pt);
		}
		if (bEdgeVis[1])
		{
			pointsArrayOut.Add(pointsArrayIn[i]);
		}
		ptEdge[0] = ptEdge[1];
		bEdgeVis[0] = bEdgeVis[1];
	}
	if (pointsArrayOut.GetSize() != 0 && bEdgeVis[0] != bVisVer0)
	{	// first and last vertices on opposite sides of clip plane
		pt = EoGeLine::IntersectionWithPln4(ptEdge[0], pointsArrayIn[0], ptQ, planeNormal);		
		pointsArrayOut.Add(pt);
	}
}
