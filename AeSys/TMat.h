#pragma once

class CLine;
class CTMat : public EoGeMatrix
{
public:	// Constructors amd destructor

	CTMat() : EoGeMatrix() 
	{
		EoGeMatrix::Identity();
	}
	CTMat(const EoGeMatrix& matrix) 
		: EoGeMatrix(matrix) 
	{
	}
	CTMat(EoGeMatrixRow& row0, EoGeMatrixRow& row1, EoGeMatrixRow& row2, EoGeMatrixRow& row3) 
		: EoGeMatrix(row0, row1, row2, row3) 
	{
	}
	CTMat(EoGePoint3d pt, EoGeVector3d v) 
	{
		DefUsingArbPtAndAx(pt, v);
	}
	/// <summary>
	///Constructs transformation matrix required to transform points about a reference point and axis.
	/// </summary>
	/// <remarks>
	///Assumes reference vector is a unit vector. Uses right handed convention.
	///Based on the following equation: [ti] * [r] * [t], where
	///	ti translation of reference point to origin
	///	r  rotation about origin =
	///	| ax*ax+(1-ax*ax)*ca  ax*ay*(1-ca)+az*sa  ax*az*(1-ca)-ay*sa |
	///	| ax*ay*(1-ca)-az*sa  ay*ay+(1-ay*ay)*ca  ay*az*(1-ca)+ax*sa |
	///	| ax*az*(1-ca)+ay*sa  ay*az*(1-ca)-ax*sa  az*az+(1-az*az)*ca |
	///	t	translation of reference point back to intitial position
	/// </remarks>
	/// <param name="refPoint">reference point</param>
	/// <param name="refAxis">reference axis vector (unit)</param>
	/// <param name="angle">angle (radians)</param>	
	CTMat(const EoGePoint3d& refPoint, EoGeVector3d refAxis, const double angle);
	/// <summary>Build matrix which will transform points onto z=0 plane with origin at 0,0</summary>
	/// <remarks>
	///X and y axis vectors do not need to be normalized. If they are not unit vectors, 
	///appropriate scaling is applied in the x and/or y axis.
	/// </remarks>
	/// <param name="pt">origin of reference system</param>
	/// <param name="xReference">x-axis of reference system</param>
	/// <param name="yReference">y-axis of reference system</param>	
	CTMat(EoGePoint3d pt, EoGeVector3d xReference, EoGeVector3d yReference);		
	/// <summary>Builds rotation transformation matrices.</summary>
	/// <remarks>Angles (in degrees) for each axis</remarks>
	CTMat(int*, EoGeVector3d);
	~CTMat() 
	{
	}

public: // Operators	

	CLine operator*(CLine line);

	EoGePoint3d operator*(const EoGePoint3d& point);
	EoGePoint4d operator*(EoGePoint4d& point);

	EoGeVector3d operator*(EoGeVector3d vector);

public: // Methods

	/// <summary>
	///Constructs transformation matrix required to transform points on an arbitrary plane to the z=0 
	///plane with point at origin.
	/// </summary>
	/// <remarks>
	///Assumes plane normal is a unit vector. Uses right handed convention.
	///	See Rodgers, 3-9 Rotation about an arbitrary axis in space
	/// </remarks>
	/// <param name="ptP">point on plane which defines origin</param>
	/// <param name="vN">unit vector defining plane normal</param>	
	void	DefUsingArbPtAndAx(EoGePoint3d ptP, EoGeVector3d vN);
	/// <summary>Initializes a matrix for rotation about the x-axis, the y-axis rotates to the z-axis</summary>
	CTMat	RotateX(const double dSinAng, const double dCosAng);
	/// <summary>Initializes a matrix for rotation about the y-axis, the z-axis rotates to the x-axis</summary>
	CTMat	RotateY(const double dSinAng, const double dCosAng);
	/// <summary>Initializes a matrix for rotation about the z-axis, the x-axis rotates to the y-axis</summary>
	CTMat	RotateZ(const double dSinAng, const double dCosAng);
	void	Scale(EoGeVector3d);
	void	Translate(EoGeVector3d translate);
};
