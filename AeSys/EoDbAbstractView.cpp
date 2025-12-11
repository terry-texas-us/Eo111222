#include "stdafx.h"


EoDbAbstractView::EoDbAbstractView()
{
	m_ViewMode = 0;
	m_RenderMode = 0;
	m_UcsOrthoViewType = 1;
	
	m_UCSOrigin(0., 0., 0.);
	m_UCSXAxis(1., 0., 0.);
	m_UCSYAxis(0., 1., 0.);
	m_Elevation = 0.;

	m_Position = EoGePoint3d(0., 0., 50.);
	m_Target(0., 0., 0.);
	m_Direction(0., 0., 1.);
	m_ViewUp(0., 1., 0.);
	m_TwistAngle = 0.;
	m_Height = 1.;
	m_Width = 1.;
	m_Center[0] = .5;
	m_Center[1] = .5;
	
	m_LensLength = 50.;

	m_FrontClipDistance = 20.;
	m_BackClipDistance = 100.;
}
EoDbAbstractView::EoDbAbstractView(const EoDbAbstractView& src)
{
	m_ViewMode = src.m_ViewMode; 
	m_RenderMode = src.m_RenderMode;
	
	m_UcsOrthoViewType = src.m_UcsOrthoViewType;
	m_UCSOrigin = src.m_UCSOrigin;
	m_UCSXAxis = src.m_UCSXAxis;
	m_UCSYAxis = src.m_UCSYAxis;
	m_Elevation = src.m_Elevation;
	
	m_Position = src.m_Position;
	m_Target = src.m_Target;
	m_Direction = src.m_Direction;
	m_ViewUp = src.m_ViewUp;
	m_TwistAngle = src.m_TwistAngle;
	m_Height = src.m_Height;
	m_Width = src.m_Width;
	m_Center[0] = src.m_Center[0]; 
	m_Center[1] = src.m_Center[1];
	m_LensLength = src.m_LensLength;
	m_FrontClipDistance = src.m_FrontClipDistance;
	m_BackClipDistance = src.m_BackClipDistance;
}

EoDbAbstractView& EoDbAbstractView::operator=(const EoDbAbstractView& src)
{
	m_ViewMode = src.m_ViewMode; 
	m_RenderMode = src.m_RenderMode;
	
	m_UcsOrthoViewType = src.m_UcsOrthoViewType;
	m_UCSOrigin = src.m_UCSOrigin;
	m_UCSXAxis = src.m_UCSXAxis;
	m_UCSYAxis = src.m_UCSYAxis;
	m_Elevation = src.m_Elevation;
	
	m_Position = src.m_Position;
	m_Target = src.m_Target;
	m_Direction = src.m_Direction;
	m_ViewUp = src.m_ViewUp;
	m_TwistAngle = src.m_TwistAngle;
	m_Height = src.m_Height;
	m_Width = src.m_Width;
	m_Center[0] = src.m_Center[0];
	m_Center[1] = src.m_Center[1];
	m_LensLength = src.m_LensLength;
	m_FrontClipDistance = src.m_FrontClipDistance;
	m_BackClipDistance = src.m_BackClipDistance;

	return *this;
}
double EoDbAbstractView::BackClipDistance(void)
{
	return m_BackClipDistance;
}
EoGePoint3d EoDbAbstractView::Position(void)
{
	return m_Position;
}
EoGePoint3d EoDbAbstractView::Target(void)
{
	return m_Target;
}
double EoDbAbstractView::FrontClipDistance(void)
{
	return m_FrontClipDistance;
}
void EoDbAbstractView::SetBackClipDistance(double distance) 
{
	m_BackClipDistance = distance;
}
void EoDbAbstractView::SetTarget(const EoGePoint3d& target)
{
	m_Target = target;
}
void EoDbAbstractView::SetView(const EoGePoint3d& position, const EoGePoint3d& target, const EoGeVector3d& viewUp)
{
	m_Position = position;
	m_Target = target;
	m_ViewUp = viewUp;
}
void EoDbAbstractView::SetFrontClipDistance(double distance)
{
	m_FrontClipDistance = distance;
}
EoGeVector3d EoDbAbstractView::Direction(void)
{
	return m_Direction;
}
void EoDbAbstractView::SetDirection(const EoGeVector3d& direction)
{
	if (direction.Length() > FLT_EPSILON)
	{
		m_Direction = direction;
		m_Direction.Normalize();
	}
}
void EoDbAbstractView::SetPosition(const EoGePoint3d& position)
{
	m_Position = position;
}
void EoDbAbstractView::SetPosition(const EoGeVector3d& direction) 
{
	m_Position = m_Target + (direction * m_LensLength);
}
void EoDbAbstractView::SetBackClippingEnabled(bool enabled)
{
	if (enabled) 
	{
		m_ViewMode |= AV_BACKCLIPPING;
	}
	else 
	{
		m_ViewMode &= ~AV_BACKCLIPPING;
	}
}
void EoDbAbstractView::SetFrontClippingEnabled(bool enabled)
{
	if (enabled) 
	{
		m_ViewMode |= AV_FRONTCLIPPING;
	}
	else 
	{
		m_ViewMode &= ~AV_FRONTCLIPPING;
	}
}
void EoDbAbstractView::SetPerspectiveEnabled(BOOL enabled)
{
	if (enabled) 
	{
		m_ViewMode |= AV_PERSPECTIVE;
	}
	else 
	{
		m_ViewMode &= ~AV_PERSPECTIVE;
	}
}
void EoDbAbstractView::SetViewUp(EoGeVector3d viewUp)
{
	if (viewUp.Length() > DBL_EPSILON)
	{
		m_ViewUp = viewUp;
		m_ViewUp.Normalize();
	}
}