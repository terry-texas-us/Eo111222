#pragma once

#include "Eo.h"

class EoGeMatrixRow {
  double m_d[4];

 public:
  EoGeMatrixRow& operator+=(const EoGeMatrixRow& v);
  EoGeMatrixRow& operator-=(const EoGeMatrixRow& row);
  EoGeMatrixRow& operator*=(const double d);
  EoGeMatrixRow& operator/=(const double d);

  EoGeMatrixRow operator+(const EoGeMatrixRow& row) const;
  EoGeMatrixRow operator-(const EoGeMatrixRow& row);
  EoGeMatrixRow operator*(const double d) const;

  double& operator[](int i);
  const double& operator[](int i) const;
  void operator()(const double c0, const double c1, const double c2, const double c3);

  EoGeMatrixRow operator-();

  static void Exchange(EoGeMatrixRow& rowA, EoGeMatrixRow& rowB);
};

class EoGeMatrix {
 protected:
  union {
    double m_4X4[4][4];
    EoGeMatrixRow m_row[4];
  };

 public:
  EoGeMatrix() : m_4X4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} {}

  EoGeMatrix(const EoGeMatrixRow& row0, const EoGeMatrixRow& row1, const EoGeMatrixRow& row2,
             const EoGeMatrixRow& row3);

 public:
  EoGeMatrix& operator*=(const EoGeMatrix& m);
  EoGeMatrix& operator*=(double scaleFactor);

  /** @brief Scales the matrix by the inverse of the given scale factor.
   *  @param scaleFactor The factor by which to scale the matrix.
   *  @return A reference to the scaled matrix (the current instance).
   *  @note If the scale factor is near zero (within numeric epsilon), a warning is logged and no scaling is performed.
   */
  EoGeMatrix& operator/=(double scaleFactor);

  [[nodiscard]] EoGeMatrix operator*(const EoGeMatrix& mB);

  EoGeMatrixRow& operator[](int i);
  
  [[nodiscard]] const EoGeMatrixRow& operator[](int i) const;

 public:
  /** @brief Calculates the determinant of the transformation matrix
   *  @return Determinant value
   *  @note A determinant value of zero indicates that the matrix is singular and does not have an inverse.
   *  For affine transformations, can optimize to 3x3 upper-left submatrix
   */
  [[nodiscard]] double Determinant() const noexcept;

  EoGeMatrix& Identity();

  /** @brief Determines if the matrix is an identity matrix within a specified tolerance.
   * @param tolerance The tolerance value used to compare the matrix elements against the identity matrix.
   * @return true if the matrix is an identity matrix within the specified tolerance; otherwise, false.
   */
  [[nodiscard]] bool IsIdentity(double tolerance = Eo::geometricTolerance) const;

  EoGeMatrix& Inverse();

  /** @brief Creates a new matrix that is a linear interpolation between this matrix and another
   *  @param other Target matrix to interpolate towards
   *  @param t Interpolation parameter [0,1]
   *  @return New interpolated matrix
   */
  [[nodiscard]] EoGeMatrix Lerp(const EoGeMatrix& other, double t) const noexcept;

  [[nodiscard]] EoGeMatrix Transpose() const noexcept;

  [[nodiscard]] static EoGeMatrix Multiply(const EoGeMatrix& M1, const EoGeMatrix& M2);

  /** @brief Gets a specific matrix element
   *  @param row Row index (0-3)
   *  @param column Column index (0-3)
   *  @return Matrix element value
   */
  [[nodiscard]] double GetElement(int row, int column) const noexcept { return m_4X4[row][column]; }

  /** @brief Sets a specific matrix element
   *  @param row Row index (0-3)
   *  @param column Column index (0-3)
   *  @param value Value to set
   */
  void SetElement(int row, int column, double value) { m_4X4[row][column] = value; }
};
