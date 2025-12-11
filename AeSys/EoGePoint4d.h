#pragma once

typedef CArray<EoGePoint4d, EoGePoint4d&> EoGePoint4dArray;

class EoGePoint4d {
public:
	double x;
	double y;
	double z;
	double w;

public: // Constructors and destructor
	EoGePoint4d();
	EoGePoint4d(const double initialX, const double initialY, const double initialZ, const double initialW);
	EoGePoint4d(const EoGePoint3d& initialPoint);

public: // Operators
	void operator+=(EoGeVector3d& v);
	void operator-=(EoGeVector3d& v);
	void operator*=(const double t);
	void operator/=(const double t);

	EoGePoint4d operator+(EoGeVector3d vector);
	EoGePoint4d operator-(EoGeVector3d vector);
	EoGePoint4d operator*(const double t);
	EoGePoint4d operator/(const double t);

	void operator()(const double x, const double y, const double z, const double w);

	EoGeVector3d operator-(const EoGePoint4d& ptQ);

public: // Methods
	/// <summary>Determines the xy distance between two points.</summary>
	double DistanceToPointXY(const EoGePoint4d& ptQ) const;
	/// <summary>Performs a containment test on a point.</summary>
	bool IsInView();

public:
	static bool ClipLine(EoGePoint4d& ptA, EoGePoint4d& ptB);
	/// <summary>Sutherland-hodgman-like polygon clip by view volume.</summary>
	static void	ClipPolygon(EoGePoint4dArray& pointsArray);
	/// <summary>Sutherland-hodgman-like polygon clip by clip plane.</summary>
	/// <remarks>Visibility determined using dot product.</remarks>
	static void IntersectionWithPln(EoGePoint4dArray& pointsArrayIn, const EoGePoint4d& pointOnClipPlane, EoGeVector3d& clipPlaneNormal, EoGePoint4dArray& pointsArrayOut);
	static EoGePoint4d Max(EoGePoint4d& ptA, EoGePoint4d& ptB);
	static EoGePoint4d Min(EoGePoint4d& ptA, EoGePoint4d& ptB);
};