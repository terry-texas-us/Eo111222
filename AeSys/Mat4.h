#pragma once

class MatrixRow
{
private:
	
	double m_d[4];

public: // Constructors and destructor
	
	MatrixRow() {m_d[0] = m_d[1] = m_d[2] = m_d[3] = 0.;}
	MatrixRow(double c0, double c1, double c2, double c3) {m_d[0] = c0; m_d[1] = c1; m_d[2] = c2; m_d[3] = c3;}
	MatrixRow(const MatrixRow& row)
	{
		m_d[0] = row.m_d[0]; m_d[1] = row.m_d[1]; m_d[2] = row.m_d[2]; m_d[3] = row.m_d[3];
	}
	MatrixRow(EoGeVector3d v, const double d);
		
	~MatrixRow() 
	{
	}
public: // Operators

	MatrixRow& operator=(const MatrixRow& row);
	MatrixRow& operator+=(MatrixRow& v);
	MatrixRow& operator-=(const MatrixRow& row);
	MatrixRow& operator*=(const double d);
	MatrixRow& operator/=(const double d);

	bool operator==(MatrixRow& row) {return (Identical(row, DBL_EPSILON));}
	bool operator!=(MatrixRow& row) {return (!Identical(row, DBL_EPSILON));}

	MatrixRow& operator+(MatrixRow& row);
	MatrixRow operator-(const MatrixRow& row);
	MatrixRow operator*(const double d);
	double operator*(const MatrixRow& row);
	MatrixRow operator/(const double d);

	double& operator[](int i) {return m_d[i];}
	const double& operator[](int i) const {return m_d[i];}

	void operator()(const double c0, const double c1, const double c2, const double c3);

	MatrixRow operator-() {return MatrixRow(- m_d[0], - m_d[1], - m_d[2], - m_d[3]);}

	bool Identical(const MatrixRow& row, double tolerance);
	void Get(double* d) const {d[0] = m_d[0]; d[1] = m_d[1]; d[2] = m_d[2]; d[3] = m_d[3];}

	static void Exchange(MatrixRow& rowA, MatrixRow& rowB)
	{ 
		MatrixRow Row(rowA); 
		rowA = rowB; 
		rowB = Row; 
	}
};
class Matrix : public CObject
{
protected:

	MatrixRow m_row[4];

public: // Constructors and destructor

	Matrix() 
	{
	}
	Matrix(const Matrix& m);
	Matrix(const MatrixRow& row0, const MatrixRow& row1, const MatrixRow& row2, const MatrixRow& row3);
	
	virtual ~Matrix() 
	{
	}
public: // Operators

	Matrix& operator=(const Matrix& m);

	Matrix& operator+=(Matrix& m);
	Matrix& operator-=(Matrix& m);
	Matrix& operator*=(const Matrix& m);
	Matrix& operator*=(double d);
	Matrix& operator/=(double d);

	Matrix operator+(Matrix& mB);
	Matrix operator-(Matrix& mB);
	Matrix operator*(const Matrix& mB);
	Matrix operator*(const double d);
	Matrix operator/(const double d);
	
	bool operator==(Matrix& mB) {return (Identical(mB));}
	bool operator!=(Matrix& mB) {return (!Identical(mB));}

	MatrixRow& operator[](int i) {return m_row[i];}
	const MatrixRow& operator[](int i) const {return m_row[i];}	

public: // Methods

	bool	Identical(Matrix& m);
	Matrix&	Identity();
	Matrix&	Inverse();
	Matrix	Transpose();
};
