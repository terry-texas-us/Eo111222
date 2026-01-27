#include "Stdafx.h"

#include "EoGeMatrix.h"

EoGeMatrixRow EoGeMatrixRow::operator-() {
  EoGeMatrixRow Row{};

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
  EoGeMatrixRow Row{};

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
  double InverseScaleFactor = 1.0 / scaleFactor;

  m_row[0] *= InverseScaleFactor;
  m_row[1] *= InverseScaleFactor;
  m_row[2] *= InverseScaleFactor;
  m_row[3] *= InverseScaleFactor;

  return *this;
}
EoGeMatrix EoGeMatrix::operator*(const EoGeMatrix& mB) { return Multiply(mB, *this); }
EoGeMatrixRow& EoGeMatrix::operator[](int i) { return m_row[i]; }
const EoGeMatrixRow& EoGeMatrix::operator[](int i) const { return m_row[i]; }

[[nodiscard]] double EoGeMatrix::Determinant() const noexcept {
  return m_4X4[0][0] * (m_4X4[1][1] * m_4X4[2][2] - m_4X4[1][2] * m_4X4[2][1]) -
         m_4X4[0][1] * (m_4X4[1][0] * m_4X4[2][2] - m_4X4[1][2] * m_4X4[2][0]) +
         m_4X4[0][2] * (m_4X4[1][0] * m_4X4[2][1] - m_4X4[1][1] * m_4X4[2][0]);
}

EoGeMatrix& EoGeMatrix::Identity() {
  std::memset(m_4X4, 0, sizeof(m_4X4));
  m_4X4[0][0] = 1.0;
  m_4X4[1][1] = 1.0;
  m_4X4[2][2] = 1.0;
  m_4X4[3][3] = 1.0;

  return *this;
}

bool EoGeMatrix::IsIdentity(double tolerance) const {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      double expected = (i == j) ? 1.0 : 0.0;
      if (fabs(m_4X4[i][j] - expected) > tolerance) { return false; }
    }
  }
  return true;
}

/**
 * Inverts the matrix using Gauss-Jordan elimination with partial pivoting.
 *
 * The Gauss-Jordan elimination method inverts an n×n matrix A by augmenting it with the identity matrix I,
 * forming [A|I]. Apply row operations (swap, scale, add multiples) to transform A into I; the right side becomes A⁻¹.
 * It combines forward elimination (to upper triangular) and backward substitution (to diagonal), 
 * ensuring pivot non-zero for stability.
 *
 * @return A reference to the inverted matrix (the current instance).
 * @note  Fails if A is singular (det=0); a warning is logged.
 */
EoGeMatrix& EoGeMatrix::Inverse() {
  EoGeMatrix mA(*this);
  this->Identity();

  int i, i1;

  for (int iCol = 0; iCol < 4; iCol++) {
    i1 = iCol;
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

[[nodiscard]] EoGeMatrix EoGeMatrix::Lerp(const EoGeMatrix& other, double t) const noexcept {
  EoGeMatrix result(*this);

  // Clamp t to [0, 1] range for safety
  if (t <= 0.0) { return result; }
  if (t >= 1.0) { return other; }

  // Linear interpolation: result = this * (1 - t) + other * t
  double oneMinusT = 1.0 - t;

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) { result.m_4X4[i][j] = m_4X4[i][j] * oneMinusT + other.m_4X4[i][j] * t; }
  }
  return result;
}

EoGeMatrix EoGeMatrix::Transpose() const noexcept {
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
