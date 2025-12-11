#pragma once

typedef CArray<CPnt4, CPnt4&> CPnt4s;

class CPnt4 : public CObject
{
private:
	double m_d[4];

public:
	static bool ClipLine(CPnt4& ptA, CPnt4& ptB);
	/// <summary>Sutherland-hodgman-like polygon clip by view volume.</summary>
	static void	ClipPolygon(CPnt4s& pta);
	
	static CPnt4 Max(CPnt4& ptA, CPnt4& ptB);
	static CPnt4 Min(CPnt4& ptA, CPnt4& ptB);

public: // Constructors and destructor
	CPnt4(); 
	CPnt4(const double* values);
	CPnt4(const double x, const double y, const double z, const double w);
	CPnt4(const CPnt4& pt);
	CPnt4(const EoGePoint3d& pt);
	~CPnt4()
	{
	}
	operator double*();
	operator const double*() const;
	
public: // Operators

	CPnt4& operator=(const CPnt4& pt);

	void operator+=(EoGeVector3d& v);
	void operator-=(EoGeVector3d& v);
	void operator*=(const double t);
	void operator/=(const double t);

	CPnt4 operator+(EoGeVector3d vector);
	CPnt4 operator-(EoGeVector3d vector);
	CPnt4 operator*(const double t);
	CPnt4 operator/(const double t);
	
	bool operator==(const CPnt4& pt);
	bool operator!=(const CPnt4& pt);

	double& operator[](int i);
	const double& operator[](int i) const;
	void operator()(const double x, const double y, const double z, const double w);
	
	EoGeVector3d operator-(const CPnt4& ptQ);

public: // Methods
	
	/// <summary>Determines the xy distance between two points.</summary>
	double DistanceToPointXY(const CPnt4& ptQ) const;
	bool Identical(const CPnt4& point, double tolerance);
	/// <summary>Performs a containment test on a point.</summary>
	bool IsInView();
};