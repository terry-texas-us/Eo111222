#include "stdafx.h"

MatrixRow::MatrixRow(EoGeVector3d v, const double d) 
{
	m_d[0] = v.x; m_d[1] = v.y; m_d[2] = v.z; m_d[3] = d;
}
MatrixRow& MatrixRow::operator=(const MatrixRow& row) 
{
	m_d[0] = row.m_d[0]; m_d[1] = row.m_d[1]; m_d[2] = row.m_d[2]; m_d[3] = row.m_d[3]; 
	return *this;
}
MatrixRow& MatrixRow::operator+=(MatrixRow& row) 
{
	m_d[0] += row.m_d[0]; m_d[1] += row.m_d[1]; m_d[2] += row.m_d[2]; m_d[3] += row.m_d[3]; 
	return *this;
}
MatrixRow& MatrixRow::operator-=(const MatrixRow& row) 
{
	m_d[0] -= row.m_d[0]; m_d[1] -= row.m_d[1]; m_d[2] -= row.m_d[2]; m_d[3] -= row.m_d[3]; 
	return *this;
}
MatrixRow& MatrixRow::operator*=(const double d) 
{
	m_d[0] *= d; m_d[1] *= d; m_d[2] *= d; m_d[3] *= d;
	return *this;
}
MatrixRow& MatrixRow::operator/=(const double d) 
{
	m_d[0] /= d; m_d[1] /= d; m_d[2] /= d; m_d[3] /= d;
	return *this;
}
void MatrixRow::operator()(const double c0, const double c1, const double c2, const double c3) 
{
	m_d[0] = c0; m_d[1] = c1; m_d[2] = c2; m_d[3] = c3;
}
MatrixRow& MatrixRow::operator+(MatrixRow& row)
{ 
	MatrixRow Row = *this;
	return Row += row; 
}
MatrixRow MatrixRow::operator-(const MatrixRow& row)
{  
	MatrixRow Row = *this;
	return Row -= row;
}
double MatrixRow::operator*(const MatrixRow& row)
{ 
	return (m_d[0] * row.m_d[0] + m_d[1] * row.m_d[1] + m_d[2] * row.m_d[2] + m_d[3] * row.m_d[3]); 
}

MatrixRow MatrixRow::operator*(const double d)
{
	return MatrixRow(m_d[0] * d, m_d[1] * d, m_d[2] * d, m_d[3] * d);
}
MatrixRow MatrixRow::operator/(const double d)
{ 
	double d_inv = 1. / d; 
	return MatrixRow(m_d[0] * d_inv, m_d[1] * d_inv, m_d[2] * d_inv, m_d[3] * d_inv); 
}
bool MatrixRow::Identical(const MatrixRow& row, double tolerance)
{ 
    return 
	(
		fabs(m_d[0] - row.m_d[0]) > tolerance ||
		fabs(m_d[1] - row.m_d[1]) > tolerance ||
		fabs(m_d[2] - row.m_d[2]) > tolerance ||
		fabs(m_d[3] - row.m_d[3]) > tolerance ? false : true
	);
}

Matrix::Matrix(const Matrix& m)
{
	m_row[0] = m.m_row[0];
	m_row[1] = m.m_row[1];
	m_row[2] = m.m_row[2];
	m_row[3] = m.m_row[3];
}
Matrix::Matrix(const MatrixRow& v0, const MatrixRow& v1, const MatrixRow& v2, const MatrixRow& v3) 
{
	m_row[0] = v0; 
	m_row[1] = v1; 
	m_row[2] = v2; 
	m_row[3] = v3;
}
Matrix& Matrix::operator=(const Matrix& m) 
{
	m_row[0] = m[0]; 
	m_row[1] = m[1]; 
	m_row[2] = m[2]; 
	m_row[3] = m[3];

	return *this;
}
Matrix& Matrix::operator+=(Matrix& m) 
{
	m_row[0] += m[0];
	m_row[1] += m[1];
	m_row[2] += m[2];
	m_row[3] += m[3];
	
	return *this;
}
Matrix& Matrix::operator-=(Matrix& m) 
{
	m_row[0] -= m[0]; 
	m_row[1] -= m[1]; 
	m_row[2] -= m[2]; 
	m_row[3] -= m[3];
	
	return *this;
}
Matrix& Matrix::operator*=(double d)
{
	m_row[0] *= d; 
	m_row[1] *= d; 
	m_row[2] *= d; 
	m_row[3] *= d;
	
	return *this;
}
Matrix& Matrix::operator*=(const Matrix& mB) 
{
	Matrix mA(*this);
	
	for (int iRow = 0; iRow < 4; iRow++)
	{
		MatrixRow vRowA(mA[0][iRow], mA[1][iRow], mA[2][iRow], mA[3][iRow]);
		
		for (int iCol = 0; iCol < 4; iCol++)
			(*this)[iCol][iRow] = vRowA * mB[iCol];
	}
	return *this;
}
Matrix& Matrix::operator/=(double d) 
{
	m_row[0] /= d; 
	m_row[1] /= d; 
	m_row[2] /= d; 
	m_row[3] /= d;
	
	return *this;
}
Matrix Matrix::operator+(Matrix& mB)
{ 
	return Matrix(m_row[0] + mB[0], m_row[1] + mB[1], m_row[2] + mB[2], m_row[3] + mB[3]);
}

Matrix Matrix::operator-(Matrix& mB)
{ 
	return Matrix(m_row[0] - mB[0], m_row[1] - mB[1], m_row[2] - mB[2], m_row[3] - mB[3]); 
}

Matrix Matrix::operator*(const Matrix& mB)
{
	Matrix mC;
	
	for (int iRow = 0; iRow < 4; iRow++)
	{
		MatrixRow vRowA(m_row[0][iRow], m_row[1][iRow], m_row[2][iRow], m_row[3][iRow]);
		
		for (int iCol = 0; iCol < 4; iCol++)
			mC[iCol][iRow] = vRowA * mB[iCol];
	}
	return mC;
}

Matrix Matrix::operator*(const double d)
{ 
	return Matrix(m_row[0] * d, m_row[1] * d, m_row[2] * d, m_row[3] * d); 
}

Matrix Matrix::operator/(const double d)
{ 
	return Matrix(m_row[0] / d, m_row[1] / d, m_row[2] / d, m_row[3] / d); 
}

// Methods

bool Matrix::Identical(Matrix& mB)
{ 
	return ((m_row[0] == mB[0]) && (m_row[1] == mB[1]) && (m_row[2] == mB[2]) && (m_row[3] == mB[3])); 
}
Matrix& Matrix::Identity()
{
	m_row[0](1., 0., 0., 0.);
	m_row[1](0., 1., 0., 0.);
	m_row[2](0., 0., 1., 0.);
	m_row[3](0., 0., 0., 1.); 
	
	return (*this);
}

Matrix& Matrix::Inverse()		// Gauss-Jordan elimination with partial pivoting
{
	Matrix mA(*this);				// As a evolves from original mat into identity
	this->Identity();					// mB evolves from identity into inverse(a)
	
	int i, i1;

	// Loop over cols of mA from left to right, eliminating above and below diag
	for (int iCol = 0; iCol < 4; iCol++) 
	{	// Find largest pivot in column iCol among rows iCol..3
		i1 = iCol; 		// Row with largest pivot candidate
		for (i = iCol + 1; i < 4; i++)
			if (fabs(mA[i][iCol]) > fabs(mA[i1][iCol]))
				i1 = i;

		// Swap rows i1 and iCol in mA and mB to put pivot on diagonal
		MatrixRow::Exchange(mA[i1], mA[iCol]);
		MatrixRow::Exchange((*this)[i1], (*this)[iCol]);

		// Scale row iCol to have mA unit diagonal
		if (mA[iCol][iCol] == 0.)
		{
			ATLTRACE2(atlTraceGeneral, 1, L"Matrix::Inverse: singular matrix, can't invert\n");
		}
		(*this)[iCol] /= mA[iCol][iCol];
		mA[iCol] /= mA[iCol][iCol];

		// Eliminate off-diagonal elems in col iCol of mA, doing identical ops to mB
		for (i = 0; i < 4; i++)
			if (i != iCol) 
			{
				(*this)[i] -= (*this)[iCol] * mA[i][iCol];
				mA[i] -=  mA[iCol] * mA[i][iCol];
			}
	}
	return *this;
}

Matrix Matrix::Transpose() 
{
	return Matrix(
		MatrixRow(m_row[0][0], m_row[1][0], m_row[2][0], m_row[3][0]),
		MatrixRow(m_row[0][1], m_row[1][1], m_row[2][1], m_row[3][1]),
		MatrixRow(m_row[0][2], m_row[1][2], m_row[2][2], m_row[3][2]),
		MatrixRow(m_row[0][3], m_row[1][3], m_row[2][3], m_row[3][3]));
}

