#include "stdafx.h"

EoGeMatrixRow EoGeMatrixRow::operator-() {
  EoGeMatrixRow Row;

  Row[0] = -m_d[0];
  Row[1] = -m_d[1];
  Row[2] = -m_d[2];
  Row[3] = -m_d[3];

  return Row;
}
EoGeMatrixRow& EoGeMatrixRow::operator+=(const EoGeMatrixRow& row) {
  m_d[0] += row.m_d[0];
  m_d[1] += row.m_d[1];
  m_d[2] += row.m_d[2];
  m_d[3] += row.m_d[3];
  return *this;
}
EoGeMatrixRow& EoGeMatrixRow::operator-=(const EoGeMatrixRow& row) {
  m_d[0] -= row.m_d[0];
  m_d[1] -= row.m_d[1];
  m_d[2] -= row.m_d[2];
  m_d[3] -= row.m_d[3];
  return *this;
}
EoGeMatrixRow& EoGeMatrixRow::operator*=(const double d) {
  m_d[0] *= d;
  m_d[1] *= d;
  m_d[2] *= d;
  m_d[3] *= d;
  return *this;
}
EoGeMatrixRow& EoGeMatrixRow::operator/=(const double d) {
  m_d[0] /= d;
  m_d[1] /= d;
  m_d[2] /= d;
  m_d[3] /= d;
  return *this;
}
EoGeMatrixRow EoGeMatrixRow::operator-(const EoGeMatrixRow& row) {
  EoGeMatrixRow Row = *this;
  return Row -= row;
}
//bool EoGeMatrixRow::operator==(EoGeMatrixRow& row) {
//	return (Identical(row, DBL_EPSILON));
//}
//bool EoGeMatrixRow::operator!=(EoGeMatrixRow& row) {
//	return (!Identical(row, DBL_EPSILON));
//}
void EoGeMatrixRow::operator()(const double c0, const double c1, const double c2, const double c3) {
  m_d[0] = c0;
  m_d[1] = c1;
  m_d[2] = c2;
  m_d[3] = c3;
}
EoGeMatrixRow EoGeMatrixRow::operator+(const EoGeMatrixRow& row) const {
  EoGeMatrixRow Row = *this;
  return Row += row;
}
EoGeMatrixRow EoGeMatrixRow::operator*(const double scaleFactor) const {
  EoGeMatrixRow Row;

  Row[0] = m_d[0] * scaleFactor;
  Row[1] = m_d[1] * scaleFactor;
  Row[2] = m_d[2] * scaleFactor;
  Row[3] = m_d[3] * scaleFactor;

  return Row;
}
double& EoGeMatrixRow::operator[](int i) { return m_d[i]; }
const double& EoGeMatrixRow::operator[](int i) const { return m_d[i]; }
void EoGeMatrixRow::Exchange(EoGeMatrixRow& rowA, EoGeMatrixRow& rowB) {
  EoGeMatrixRow Row(rowA);
  rowA = rowB;
  rowB = Row;
}
//bool EoGeMatrixRow::Identical(const EoGeMatrixRow& row, double tolerance) {
//	return (fabs(m_d[0] - row.m_d[0]) > tolerance || fabs(m_d[1] - row.m_d[1]) > tolerance || fabs(m_d[2] - row.m_d[2]) > tolerance || fabs(m_d[3] - row.m_d[3]) > tolerance ? false : true);
//}

EoGeMatrix::EoGeMatrix(const EoGeMatrixRow& v0, const EoGeMatrixRow& v1, const EoGeMatrixRow& v2,
                       const EoGeMatrixRow& v3) {
  m_row[0] = v0;
  m_row[1] = v1;
  m_row[2] = v2;
  m_row[3] = v3;
}
EoGeMatrix& EoGeMatrix::operator*=(double scaleFactor) {
  m_row[0] *= scaleFactor;
  m_row[1] *= scaleFactor;
  m_row[2] *= scaleFactor;
  m_row[3] *= scaleFactor;

  return *this;
}
EoGeMatrix& EoGeMatrix::operator*=(const EoGeMatrix& mB) {
  *this = Multiply(mB, *this);

  return *this;
}
EoGeMatrix& EoGeMatrix::operator/=(double scaleFactor) {
  ASSERT(scaleFactor != 0.0);
  double InverseScaleFactor = 1. / scaleFactor;

  m_row[0] *= InverseScaleFactor;
  m_row[1] *= InverseScaleFactor;
  m_row[2] *= InverseScaleFactor;
  m_row[3] *= InverseScaleFactor;

  return *this;
}
EoGeMatrix EoGeMatrix::operator*(const EoGeMatrix& mB) { return Multiply(mB, *this); }
//bool EoGeMatrix::operator==(EoGeMatrix& mB) {
//	return (Identical(mB));
//}
//bool EoGeMatrix::operator!=(EoGeMatrix& mB) {
//	return (!Identical(mB));
//}
EoGeMatrixRow& EoGeMatrix::operator[](int i) { return m_row[i]; }
const EoGeMatrixRow& EoGeMatrix::operator[](int i) const { return m_row[i]; }

// Methods

//bool EoGeMatrix::Identical(EoGeMatrix& mB) {
//	return ((m_row[0] == mB[0]) && (m_row[1] == mB[1]) && (m_row[2] == mB[2]) && (m_row[3] == mB[3]));
//}
EoGeMatrix& EoGeMatrix::Identity() {
  m_row[0](1.0, 0.0, 0.0, 0.0);
  m_row[1](0.0, 1.0, 0.0, 0.0);
  m_row[2](0.0, 0.0, 1.0, 0.0);
  m_row[3](0.0, 0.0, 0.0, 1.0);

  return (*this);
}

EoGeMatrix& EoGeMatrix::Inverse() {  // Gauss-Jordan elimination with partial pivoting
  EoGeMatrix mA(*this);              // As a evolves from original mat into identity
  this->Identity();                  // mB evolves from identity into inverse(a)

  int i, i1;

  // Loop over cols of mA from left to right, eliminating above and below diag
  for (int iCol = 0; iCol < 4; iCol++) {  // Find largest pivot in column iCol among rows iCol..3
    i1 = iCol;                            // Row with largest pivot candidate
    for (i = iCol + 1; i < 4; i++) {
      if (fabs(mA.m_4X4[i][iCol]) > fabs(mA.m_4X4[i1][iCol])) { i1 = i; }
    }
    // Swap rows i1 and iCol in mA and mB to put pivot on diagonal
    EoGeMatrixRow::Exchange(mA[i1], mA[iCol]);
    EoGeMatrixRow::Exchange((*this)[i1], (*this)[iCol]);

    // Scale row iCol to have mA unit diagonal
    if (mA.m_4X4[iCol][iCol] == 0.0) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"EoGeMatrix::Inverse: singular matrix, can't invert\n");
    }
    (*this)[iCol] /= mA.m_4X4[iCol][iCol];
    mA[iCol] /= mA.m_4X4[iCol][iCol];

    // Eliminate off-diagonal elems in col iCol of mA, doing identical ops to mB
    for (i = 0; i < 4; i++) {
      if (i != iCol) {
        (*this)[i] -= (*this)[iCol] * mA.m_4X4[i][iCol];
        mA[i] -= mA[iCol] * mA.m_4X4[i][iCol];
      }
    }
  }
  return *this;
}

EoGeMatrix EoGeMatrix::Transpose() {
  EoGeMatrix TransposeMatrix;

  TransposeMatrix[0][0] = m_4X4[0][0];
  TransposeMatrix[0][1] = m_4X4[1][0];
  TransposeMatrix[0][2] = m_4X4[2][0];
  TransposeMatrix[0][3] = m_4X4[3][0];

  TransposeMatrix[1][0] = m_4X4[0][1];
  TransposeMatrix[1][1] = m_4X4[1][1];
  TransposeMatrix[1][2] = m_4X4[2][1];
  TransposeMatrix[1][3] = m_4X4[3][1];

  TransposeMatrix[2][0] = m_4X4[0][2];
  TransposeMatrix[2][1] = m_4X4[1][2];
  TransposeMatrix[2][2] = m_4X4[2][2];
  TransposeMatrix[2][3] = m_4X4[3][2];

  TransposeMatrix[3][0] = m_4X4[0][3];
  TransposeMatrix[3][1] = m_4X4[1][3];
  TransposeMatrix[3][2] = m_4X4[2][3];
  TransposeMatrix[3][3] = m_4X4[3][3];

  return TransposeMatrix;
}

EoGeMatrix EoGeMatrix::Multiply(const EoGeMatrix& M1, const EoGeMatrix& M2) {
  EoGeMatrix Result;

  double x = M1.m_4X4[0][0];
  double y = M1.m_4X4[0][1];
  double z = M1.m_4X4[0][2];
  double w = M1.m_4X4[0][3];

  Result.m_4X4[0][0] = (M2.m_4X4[0][0] * x) + (M2.m_4X4[1][0] * y) + (M2.m_4X4[2][0] * z) + (M2.m_4X4[3][0] * w);
  Result.m_4X4[0][1] = (M2.m_4X4[0][1] * x) + (M2.m_4X4[1][1] * y) + (M2.m_4X4[2][1] * z) + (M2.m_4X4[3][1] * w);
  Result.m_4X4[0][2] = (M2.m_4X4[0][2] * x) + (M2.m_4X4[1][2] * y) + (M2.m_4X4[2][2] * z) + (M2.m_4X4[3][2] * w);
  Result.m_4X4[0][3] = (M2.m_4X4[0][3] * x) + (M2.m_4X4[1][3] * y) + (M2.m_4X4[2][3] * z) + (M2.m_4X4[3][3] * w);

  x = M1.m_4X4[1][0];
  y = M1.m_4X4[1][1];
  z = M1.m_4X4[1][2];
  w = M1.m_4X4[1][3];

  Result.m_4X4[1][0] = (M2.m_4X4[0][0] * x) + (M2.m_4X4[1][0] * y) + (M2.m_4X4[2][0] * z) + (M2.m_4X4[3][0] * w);
  Result.m_4X4[1][1] = (M2.m_4X4[0][1] * x) + (M2.m_4X4[1][1] * y) + (M2.m_4X4[2][1] * z) + (M2.m_4X4[3][1] * w);
  Result.m_4X4[1][2] = (M2.m_4X4[0][2] * x) + (M2.m_4X4[1][2] * y) + (M2.m_4X4[2][2] * z) + (M2.m_4X4[3][2] * w);
  Result.m_4X4[1][3] = (M2.m_4X4[0][3] * x) + (M2.m_4X4[1][3] * y) + (M2.m_4X4[2][3] * z) + (M2.m_4X4[3][3] * w);

  x = M1.m_4X4[2][0];
  y = M1.m_4X4[2][1];
  z = M1.m_4X4[2][2];
  w = M1.m_4X4[2][3];

  Result.m_4X4[2][0] = (M2.m_4X4[0][0] * x) + (M2.m_4X4[1][0] * y) + (M2.m_4X4[2][0] * z) + (M2.m_4X4[3][0] * w);
  Result.m_4X4[2][1] = (M2.m_4X4[0][1] * x) + (M2.m_4X4[1][1] * y) + (M2.m_4X4[2][1] * z) + (M2.m_4X4[3][1] * w);
  Result.m_4X4[2][2] = (M2.m_4X4[0][2] * x) + (M2.m_4X4[1][2] * y) + (M2.m_4X4[2][2] * z) + (M2.m_4X4[3][2] * w);
  Result.m_4X4[2][3] = (M2.m_4X4[0][3] * x) + (M2.m_4X4[1][3] * y) + (M2.m_4X4[2][3] * z) + (M2.m_4X4[3][3] * w);

  x = M1.m_4X4[3][0];
  y = M1.m_4X4[3][1];
  z = M1.m_4X4[3][2];
  w = M1.m_4X4[3][3];

  Result.m_4X4[3][0] = (M2.m_4X4[0][0] * x) + (M2.m_4X4[1][0] * y) + (M2.m_4X4[2][0] * z) + (M2.m_4X4[3][0] * w);
  Result.m_4X4[3][1] = (M2.m_4X4[0][1] * x) + (M2.m_4X4[1][1] * y) + (M2.m_4X4[2][1] * z) + (M2.m_4X4[3][1] * w);
  Result.m_4X4[3][2] = (M2.m_4X4[0][2] * x) + (M2.m_4X4[1][2] * y) + (M2.m_4X4[2][2] * z) + (M2.m_4X4[3][2] * w);
  Result.m_4X4[3][3] = (M2.m_4X4[0][3] * x) + (M2.m_4X4[1][3] * y) + (M2.m_4X4[2][3] * z) + (M2.m_4X4[3][3] * w);

  return Result;
}
