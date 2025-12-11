#include "stdafx.h"

/// <summary>Cleans up the corners of two sets of parallel lines.</summary>
// Returns: false	parallel lines
//			true otherwise (success)
bool ParLines_CleanCorners(CLine* ln1, CLine* ln2)
{	
	EoGePoint3d ptInt;
	
	for (EoUInt16 w = 0; w < 2; w++)
	{
		if (ln1[w].ParallelTo(ln2[w]))
			return false;
		
		CLine::Intersection_xy(ln1[w], ln2[w], ptInt);

		ln1[w].end = ptInt;
		ln2[w].begin = ptInt;
	}
	return true;
}
/// <summary>
///Generates coordinate sets for parallel lines and modifies previously generated coordinate
///sets to properly close corners.
/// </summary>
// Notes:	The first of the two parallel lines lies to the left of lead
//			line, and the second to the right.
// Returns: 2 null length lead line
//			4 lead line parallel to line set being continued
//			1 otherwise (success)
// Parameters:	dEcc		eccentricity of lines to lead line
//				dWid		distance between lines
//				lnLead	lead line
//				pLns, 
//				abCont		sequence continuation status
int ParLines_GenPtsAndClean(double dEcc, double dWid, CLine lnLead, CLine* pLns, bool abCont)
{
	EoGeVector3d vLead(lnLead.begin, lnLead.end);
	
	double dLen = vLead.Length();
	
	if (dLen <= DBL_EPSILON)	// Null length lead line
		return (2);

	if (abCont) 
	{
		pLns[0] = pLns[2];
		pLns[1] = pLns[3];
	}
	
	double dX = vLead.y * dWid / dLen;
	double dY = vLead.x * dWid / dLen;

	pLns[2] = lnLead + EoGeVector3d(- dX * dEcc, dY * dEcc, 0.);
	pLns[3] = lnLead + EoGeVector3d(dX * (1. - dEcc), - dY * (1. - dEcc), 0.);

	if (abCont) 
	{
		EoGePoint3d ptInt;
		
		for (int i = 0; i < 2; i++)
		{
			if (CLine::Intersection_xy(pLns[i], pLns[i + 2], ptInt)) // Two lines are not parallel
			{
				pLns[i].end = ptInt;
				pLns[i + 2].begin = ptInt;
			}
			else
				return (4);
		}
	}
	return (1);
}