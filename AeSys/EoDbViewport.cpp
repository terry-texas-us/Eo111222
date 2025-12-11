#include "stdafx.h"

#include "EoDbViewport.h"

void EoDbViewport::Display(AeSysView* view, CDC* deviceContext)
{
	EoInt16 nPenColor = LogicalPenColor();
	EoInt16 LineType = LogicalLineType();
		
	pstate.SetPen(view, deviceContext, nPenColor, LineType);
	EoGePoint3d Target = m_AbstractView.Target();

	polyline::BeginLineStrip();
	polyline::SetVertex(Target);
	//polyline::SetVertex();
	polyline::__End(view, deviceContext, LineType);
}

