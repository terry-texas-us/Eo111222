#pragma once

class CModelView : public EoDbAbstractView
{
private:
	double			m_UMin;
	double			m_VMin;
	double			m_UMax;
	double			m_VMax; 
	CTMat			m_tmView;
	CTMat			m_tmViewInverse;

public: // Constructors and destructor
	CModelView();
	CModelView(CModelView& src);

	~CModelView() 
	{
	}
public: // Operators
	CModelView& operator=(const CModelView& src);

public: // Methods
	void	AdjustWindow(double aspectRatio);
	void	BuildTransformMatrix();
	void	DoTransform(CPnt4s& pta);
	void	DoTransform(CPnt4& pt);
	void	DoTransform(int iPts, CPnt4* pt);
	void	DoTransform(EoGeVector3d& v);
	
	CTMat&	GetMatrix() {return m_tmView;}
	CTMat&	GetMatrixInverse() 
	{
		return m_tmViewInverse;
	}
	double	UExtent() 
	{
		return m_UMax - m_UMin;
	}
	double	UMax() 
	{
		return m_UMax;
	}
	double	UMin() 
	{
		return m_UMin;
	}
	double	VExtent() 
	{
		return m_VMax - m_VMin;
	}
	double	VMax() 
	{
		return m_VMax;
	}
	double	VMin() 
	{
		return m_VMin;
	}
	void Initialize(const CViewport& viewport);
	void	LoadIdentity() 
	{
		m_tmView.Identity();
	}
	/// <summary> Sets a window which is centered on the view target after adjusting for viewport aspect ratio</summary>
	void	SetCenteredWindow(const CViewport& viewport, double uExtent, double vExtent);
	void	SetMatrix(CTMat& tm) 
	{
		m_tmView = tm;
	}
	void	SetWindow(double uMin, double vMin, double uMax, double vMax);

	void	RotateZ(double dSinAng, double dCosAng);
	void	Scale(EoGeVector3d scale);
	void	Translate(EoGeVector3d translate);
};

typedef CList<CModelView> CModelViews;
