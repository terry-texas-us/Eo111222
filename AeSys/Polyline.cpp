#include "stdafx.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

namespace polyline
{
	EoGePoint4dArray	pts_;
	bool LoopLine;

void BeginLineLoop() 
{
	pts_.SetSize(0);
	LoopLine = true;
}
void BeginLineStrip() 
{
	pts_.SetSize(0);
	LoopLine = false;
}
bool AnyPointsInView(EoGePoint4dArray& pointsArray)
{
	for (int i = 0; i < pointsArray.GetSize(); i++)
	{
		if (pointsArray[i].IsInView()) return true;
	}
	return false;
}
void __Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray, EoDbLineType* lineType)	
{
	EoUInt16 wDefLen = lineType->GetNumberOfDashes();
	if (wDefLen == 0) return;

	EoGePoint4d ln[2];
	CPoint pnt[2];
	EoGePoint3d pt[2];

	int iDashDefId = 0;

	double* dDash = new double[wDefLen];

	lineType->GetDashLen(dDash);

	double dSecLen = EoMax(.025/* * 96.*/, fabs(dDash[iDashDefId]));
	
	for (int i = 0; i < pointsArray.GetSize() - 1; i++) 
	{
		EoGeVector3d vLn(pointsArray[i], pointsArray[i + 1]);
		pt[0] = pointsArray[i];
		
		double dVecLen = vLn.Length();
		double dRemDisToEnd = dVecLen;
		
		while (dSecLen <= dRemDisToEnd + DBL_EPSILON) 
		{
			EoGeVector3d vDash(vLn);
			vDash *= dSecLen / dVecLen;
			pt[1] = pt[0] + vDash;
			dRemDisToEnd -= dSecLen;
			if (dDash[iDashDefId] >= 0.0)
			{
				ln[0] = pt[0];
				ln[1] = pt[1];

				view->ModelViewTransformPoints(2, ln);

				if (EoGePoint4d::ClipLine(ln[0], ln[1]))
				{
					view->DoProjection(pnt, 2, &ln[0]);
					deviceContext->Polyline(pnt, 2);
				}
			}
			pt[0] = pt[1];
			iDashDefId = (iDashDefId + 1) % wDefLen;
			dSecLen = EoMax(.025/* * 96.*/, fabs(dDash[iDashDefId]));
		}
		if (dRemDisToEnd > DBL_EPSILON) 							
		{	// Partial component of dash section must produced
			if (dDash[iDashDefId] >= 0.0) 
			{
				pt[1] = pointsArray[i + 1];
			
				ln[0] = pt[0];
				ln[1] = pt[1];

				view->ModelViewTransformPoints(2, ln);

				if (EoGePoint4d::ClipLine(ln[0], ln[1]))
				{
					view->DoProjection(pnt, 2, &ln[0]);
					deviceContext->Polyline(pnt, 2);
				}
			}
		}
		// Length of dash remaining
		dSecLen -= dRemDisToEnd;							
	}
	delete [] dDash;
}
void __End(AeSysView* view, CDC* deviceContext, EoInt16 lineTypeIndex) 
{
	if (EoDbPrimitive::IsSupportedTyp(lineTypeIndex))
	{
		int Size = pts_.GetSize();
		if (Size > 1)
		{
			view->ModelViewTransformPoints(pts_);
		
			if (AnyPointsInView(pts_)) 
			{
				CPoint pnt;
				pnt = view->DoProjection(pts_[0]);
				deviceContext->MoveTo(pnt);

				for (int i = 1; i < Size; i++) 
				{
					pnt = view->DoProjection(pts_[i]);
					deviceContext->LineTo(pnt);
				}
				if (LoopLine)
				{
					pnt = view->DoProjection(pts_[0]);
					deviceContext->LineTo(pnt);
				}
				return;
			}
		}
	}
	else
	{
		EoDbLineTypeTable* LineTypeTable = AeSysDoc::GetDoc()->LineTypeTable();
		
		EoDbLineType* LineType;
		if (!LineTypeTable->__Lookup(lineTypeIndex, LineType))
		{
			return;
		}
		pstate.SetLineType(deviceContext, 1);
		__Display(view, deviceContext, pts_, LineType);
		pstate.SetLineType(deviceContext, lineTypeIndex);
	}
}
void GeneratePointsForNPoly(EoGePoint3d& centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, int numberOfPoints, CPnts& pts) 		
{
	EoGeTransformMatrix	tm(centerPoint, majorAxis, minorAxis);
	
	tm.Inverse();
	
	// Determine the parameter (angular increment)
	double AngleIncrement = TWOPI / double(numberOfPoints);
	double CosIncrement = cos(AngleIncrement);
	double SinIncrement = sin(AngleIncrement);
	pts.SetSize(numberOfPoints);

	pts[0](1.0, 0.0, 0.0);
	
	for (int i = 0; i < numberOfPoints - 1; i++) 
	{
		pts[i + 1](pts[i].x * CosIncrement - pts[i].y * SinIncrement, pts[i].y * CosIncrement + pts[i].x * SinIncrement, 0.0);
	}
	for (int i = 0; i < numberOfPoints; i++)
	{
		pts[i] = tm * pts[i];
	}
}
bool SelectUsingLine(AeSysView* view, EoGeLine line, CPnts& ptsInt)
{	
	EoGePoint4d ptBeg(pts_[0]);
	EoGePoint4d ptEnd;
	
	view->ModelViewTransformPoint(ptBeg);
		
	for (EoUInt16 w = 1; w < pts_.GetSize(); w++)
	{
		ptEnd = EoGePoint4d(pts_[w]);
		view->ModelViewTransformPoint(ptEnd);
		
		EoGePoint3d ptInt;
		if (EoGeLine::Intersection_xy(line, EoGeLine(ptBeg, ptEnd), ptInt)) 
		{
			double dRel;
			line.RelOfPtToEndPts(ptInt, dRel);
			if (dRel >= - DBL_EPSILON && dRel <= 1. + DBL_EPSILON)
			{
				EoGeLine(ptBeg, ptEnd).RelOfPtToEndPts(ptInt, dRel);		
				if (dRel >= - DBL_EPSILON && dRel <= 1. + DBL_EPSILON) 
				{
					ptInt.z = ptBeg.z + dRel * (ptEnd.z - ptBeg.z);
					ptsInt.Add(ptInt);
				}
			}
		}
		ptBeg = ptEnd;
	}
	return (!ptsInt.IsEmpty());
}
bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, double& dRel, EoGePoint3d& ptProj)
{
	bool bResult = false;

	EoGePoint4d ptBeg(pts_[0]);
	view->ModelViewTransformPoint(ptBeg);
	
	for (int i = 1; i < (int) pts_.GetSize(); i++) 
	{
		EoGePoint4d ptEnd = EoGePoint4d(pts_[i]);
		view->ModelViewTransformPoint(ptEnd);
		EoGeLine LineSegment(ptBeg, ptEnd);
		if (LineSegment.IsSelectedByPointXY(point, view->SelectApertureSize(), ptProj, &dRel)) 
		{
			ptProj.z = ptBeg.z + dRel * (ptEnd.z - ptBeg.z);
			bResult = true;
			break;
		}
		ptBeg = ptEnd;
	}
	return (bResult);
}
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint)
{
	EoGePoint4d ptBeg(pts_[0]);
	view->ModelViewTransformPoint(ptBeg);
	
	for (EoUInt16 w = 1; w < pts_.GetSize(); w++) 
	{
		EoGePoint4d ptEnd(pts_[w]);
		view->ModelViewTransformPoint(ptEnd);
	
		EoGeLine LineSegment(ptBeg, ptEnd);
		if (LineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint))
			return true;

		ptBeg = ptEnd;
	}
	return false;
}
// Not considering possible closure
bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeftPoint, EoGePoint3d upperRightPoint, const CPnts& pts)
{
	EoGePoint4d ptBeg(pts[0]);
	view->ModelViewTransformPoint(ptBeg);
	
	for (EoUInt16 w = 1; w < pts.GetSize(); w++) 
	{
		EoGePoint4d ptEnd(pts[w]);
		view->ModelViewTransformPoint(ptEnd);
	
		EoGeLine LineSegment(ptBeg, ptEnd);
		if (LineSegment.IsContainedXY(lowerLeftPoint, upperRightPoint))
			return true;

		ptBeg = ptEnd;
	}
	return false;
}
void SetVertex(const EoGePoint3d& point) 
{
	EoGePoint4d Point4(point);
	pts_.Add(Point4);
}

}