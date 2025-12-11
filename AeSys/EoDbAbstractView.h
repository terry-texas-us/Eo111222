#pragma once

/*
The view target and the direction create a display coordinate system (DCS).
The z-direction equals the direction from target to camera (points toward viewer)
The DCS x-direction is calculated as cross product of (0,0,1) and the view direction.
If the view direction is parallel to (0,0,1), the x-direction is (1,0,0).
The DCS origin is the target point.

Parallel projections:
	The view may be offset from the line of sight (target point may be off the screen)
	A view need additional data to define DCS rectangle to be displayed. The rectangle's
	center is a 2d point (located on the DCS xy plane). Its size is given by its height and width;
	its rotation around its center is given be the twist angle.

Perspective projections:
	The DCS rectangle is always centered around the target point (DCS origin).
	Its size is taken from the views length. Start with a rectangle of 42 units diagonal length
	located at lens length distance from camera point. 
	By dividing 42 times the view distance by the lens length, getting the diagonal length of the DCS rectangle.
	Next, select a rectangle with the smae proportions as the views width and height, and rotate this by view twist angle.
*/

class EoDbAbstractView
{
public:
	static const EoInt16 AV_PERSPECTIVE = 0x01;
	static const EoInt16 AV_FRONTCLIPPING = 0x02;
	static const EoInt16 AV_BACKCLIPPING = 0x04;
	static const EoInt16 AV_FRONTCLIPPINGATEYE = 0x10;

protected:
	EoInt16 m_ViewMode;// bit 1 Perspective mode flag for this view
	// bit 2 Front clipping plane status for this view
	// bit 3 Back clipping plane status for this view
	// bit 16 Front clipping plane is located at the camera
	EoInt16 m_RenderMode; // render mode for this view (not used)
	EoInt16 m_UcsOrthoViewType;// Orthographic type of UCS (not used)
	// top 1, bottom 2, front 3, back 4, left 5, right 6

	EoGePoint3d m_UCSOrigin; // UCS origin
	EoGeVector3d m_UCSXAxis; // UCS X axis
	EoGeVector3d m_UCSYAxis; // UCS Y axis

	double m_Elevation; // elevation of the UCS plane for this view

	EoGePoint3d m_Position;
	EoGePoint3d m_Target; // view target in WCS
	EoGeVector3d m_Direction; // view direction in WCS.
	EoGeVector3d m_ViewUp;
	
	// View-Specific coordinate systems
	double m_Center[2]; // center point of the view in DCS
	double m_Height; // view height in DCS
	double m_Width; // view width in DCS
	double m_TwistAngle; // twist angle of this view in radians

	double m_LensLength; // lens length used for perspective mode in this view
	double m_FrontClipDistance;// distance from the target to the front (near) clipping plane along the target-camera line.
	double m_BackClipDistance;// distance from the target to the back (far) clipping plane along the target-camera line

public: // Constructors and destructor

	EoDbAbstractView();
	EoDbAbstractView(const EoDbAbstractView& av);
	virtual ~EoDbAbstractView()
	{
	}

public: // Operators

	EoDbAbstractView& operator=(const EoDbAbstractView& av);

public: // Methods

	void GetCenter(double& dX, double& dY) 
	{
		dX = m_Center[0]; 
		dY = m_Center[1];
	}
	EoGeVector3d Direction(void);
	EoGePoint3d Position(void);
	double BackClipDistance();
	double Height() 
	{
		return m_Height;
	}
	double LensLength() 
	{
		return m_LensLength;
	}
	double FrontClipDistance();
	EoGePoint3d Target(void);
	double _GetUMax()
	{
		return m_Center[0] + m_Width / 2.;
	}
	double _GetUMin()
	{
		return m_Center[0] - m_Width / 2.;
	}
	double _GetVMax() 
	{
		return m_Center[1] + m_Height / 2.;
	}
	double _GetVMin() 
	{
		return m_Center[1] - m_Height / 2.;
	}
	EoGeVector3d ViewUp() 
	{
		return m_ViewUp;
	}

	bool IsFrontClipOn() 
	{
		return ((m_ViewMode & AV_FRONTCLIPPING) == AV_FRONTCLIPPING);
	}
	bool IsFrontClipAtEyeOn() 
	{
		return ((m_ViewMode & AV_FRONTCLIPPINGATEYE) == AV_FRONTCLIPPINGATEYE);
	}
	bool IsBackClipOn() 
	{
		return ((m_ViewMode & AV_BACKCLIPPING) == AV_BACKCLIPPING);
	}
	bool IsPerspectiveOn()
	{
		return ((m_ViewMode & AV_PERSPECTIVE) == AV_PERSPECTIVE);
	}
	void SetDirection(const EoGeVector3d& direction);
	void SetPosition(const EoGePoint3d& position);
	void SetPosition(const EoGeVector3d& direction);
	void SetBackClipDistance(double distance);
	void SetBackClippingEnabled(bool enabled);
	void SetLensLength(double length) 
	{
		m_LensLength = length;
	}
	void SetFrontClipDistance(double distance);
	void SetFrontClippingEnabled(bool enabled);
	void SetPerspectiveEnabled(BOOL enabled);
	void SetTarget(const EoGePoint3d& target);
	void SetView(const EoGePoint3d& position, const EoGePoint3d& target, const EoGeVector3d& viewUp);
	void SetViewUp(EoGeVector3d viewUp);
};
