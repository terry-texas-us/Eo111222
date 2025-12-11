#include "stdafx.h"

CModelView::CModelView()
{
	m_UMin = - 1.;
	m_VMin = - 1.;
	m_UMax = 1.;
	m_VMax = 1.;
}

CModelView::CModelView(CModelView& src) : EoDbAbstractView(src)
{
	m_UMin = src.m_UMin;
	m_VMin = src.m_VMin;
	m_UMax = src.m_UMax;
	m_VMax = src.m_VMax;

	m_tmView = src.m_tmView;
	m_tmViewInverse = src.m_tmViewInverse;
}

CModelView& CModelView::operator=(const CModelView& src)
{
	EoDbAbstractView::operator=(src);

	m_UMin = src.m_UMin;
	m_VMin = src.m_VMin;
	m_UMax = src.m_UMax;
	m_VMax = src.m_VMax;

	m_tmView = src.m_tmView;
	m_tmView = src.m_tmViewInverse;

	return *this;
}
void CModelView::AdjustWindow(double aspectRatio)
{
	double UExtent = m_UMax - m_UMin;
	double VExtent = m_VMax - m_VMin;
	
	if (UExtent <= DBL_EPSILON || VExtent / UExtent > aspectRatio) 
	{
		double Adjustment = (VExtent / aspectRatio - UExtent) * .5;
		m_UMin -= Adjustment;
		m_UMax += Adjustment;
	}
	else
	{
		double Adjustment = (UExtent * aspectRatio - VExtent) * .5;
		m_VMin -= Adjustment;
		m_VMax += Adjustment;
	}
	BuildTransformMatrix();
}
void CModelView::BuildTransformMatrix()
{
	m_tmView.Identity();
	
	EoGeVector3d vN = m_Target - m_Position;
	vN.Normalize();

	EoGeVector3d vU = m_ViewUp.CrossProduct(vN);
	vU.Normalize();
	
	EoGeVector3d vV = vN.CrossProduct(vU);
	vV.Normalize();
	
	EoGeVector3d vector = EoGeVector3d(m_Position, EoGePoint3d::kOrigin);

	m_tmView[0] = MatrixRow(vU, vector.DotProduct(vU));
	m_tmView[1] = MatrixRow(vV, vector.DotProduct(vV));
	m_tmView[2] = MatrixRow(vN, vector.DotProduct(vN));
	m_tmView[3] = MatrixRow(0., 0., 0., 1.);

	double UExtent = m_UMax - m_UMin;
	double VExtent = m_VMax - m_VMin;
	double dNExt = m_BackClipDistance - m_FrontClipDistance;

	CTMat tmProj;
	tmProj.Identity();
	
	if (IsPerspectiveOn())
	{
		tmProj[0] = MatrixRow(2. * m_FrontClipDistance / UExtent, 0., (m_UMax + m_UMin) / UExtent, 0.);
		tmProj[1] = MatrixRow(0., (2. * m_FrontClipDistance) / VExtent, (m_VMax + m_VMin) / VExtent, 0.);
		tmProj[2] = MatrixRow(0., 0., - (m_BackClipDistance + m_FrontClipDistance) / dNExt,
						- 2. * m_BackClipDistance * m_FrontClipDistance / dNExt);
		tmProj[3] = MatrixRow(0., 0., - 1., 0.);
	}
	else 
	{
		// if oblique projections needed
		
		tmProj[0] = MatrixRow(2. / UExtent, 0., 0., - (m_UMax + m_UMin) / UExtent);
		tmProj[1] = MatrixRow(0., 2. / VExtent, 0., - (m_VMax + m_VMin) / VExtent);
		tmProj[2] = MatrixRow(0., 0., - 2. / dNExt, - (m_BackClipDistance + m_FrontClipDistance) / dNExt);
		tmProj[3] = MatrixRow(0., 0., 0., 1.);
	}
	m_tmView *= tmProj;

	m_tmViewInverse = m_tmView;
	m_tmViewInverse.Inverse();
}
void CModelView::DoTransform(CPnt4& pt)
{
	mspace.Transform(pt);
	pt = m_tmView * pt;
}
void CModelView::DoTransform(CPnt4s& pta)
{
	int iPts = (int) pta.GetSize();
	for (int i = 0; i < iPts; i++)
	{
		DoTransform(pta[i]);
	}
}
void CModelView::DoTransform(int iPts, CPnt4* pt)
{
	for (int i = 0; i < iPts; i++)
	{
		DoTransform(pt[i]);
	}
}

void CModelView::DoTransform(EoGeVector3d& v)
{
	mspace.Transform(v);
	v = m_tmView * v;
}

void CModelView::Initialize(const CViewport& viewport)
{
	SetCenteredWindow(viewport, 44., 34.);
	
	EoGePoint3d Target = EoGePoint3d(UExtent() / 2., VExtent() / 2., 0.);
	EoGePoint3d Position = Target + (EoGeVector3d::kZAxis * m_LensLength);
	
	SetView(Position, Target, EoGeVector3d::kYAxis);
	
	SetDirection(EoGeVector3d::kZAxis);
	
	SetFrontClipDistance(- 1000.);
	SetBackClipDistance(1000.);

	SetPerspectiveEnabled(FALSE);
	
	BuildTransformMatrix();
}
void CModelView::SetCenteredWindow(const CViewport& viewport, double uExtent, double vExtent)
{	
	if (uExtent == 0.)
	{
		uExtent = UExtent();
	}
	if (vExtent == 0.)
	{
		vExtent = VExtent();
	}
	double AspectRatio = viewport.HeightInInches() / viewport.WidthInInches();

	if (AspectRatio < vExtent / uExtent)
	{
		uExtent = vExtent / AspectRatio;
	}
	else
	{
		vExtent = uExtent * AspectRatio;
	}
	SetWindow(- uExtent * .5, - vExtent * .5, uExtent * .5, vExtent * .5);
}
void CModelView::SetWindow(double uMin, double vMin, double uMax, double vMax)
{
	m_UMin = uMin;
	m_VMin = vMin;
	m_UMax = uMax;
	m_VMax = vMax;

	BuildTransformMatrix();
}
void CModelView::RotateZ(double dSinAng, double dCosAng)
{
	CTMat tm; tm.RotateZ(dSinAng, dCosAng);
	m_tmView *= tm;
}
void CModelView::Scale(EoGeVector3d v)
{
	CTMat tm; tm.Scale(v);
	m_tmView *= tm;
}
void CModelView::Translate(EoGeVector3d v)
{
	m_tmView.Translate(v); 
}