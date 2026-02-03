#pragma once

#include "EoGeMatrix.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"

class EoGeLine;

/** @class A class representing a 4x4 transformation matrix for 3D geometric transformations.
 *
 *  This class extends the EoGeMatrix class to provide specific functionality for 3D transformations,
 *  including translation, rotation, and scaling of points, vectors, and lines in 3D space.
 */
class EoGeTransformMatrix : public EoGeMatrix {
 public:
  EoGeTransformMatrix() = default;

  EoGeTransformMatrix(const EoGeMatrix& matrix) : EoGeMatrix(matrix) {}

  EoGeTransformMatrix(const EoGeMatrixRow& row0, const EoGeMatrixRow& row1, const EoGeMatrixRow& row2, const EoGeMatrixRow& row3) : EoGeMatrix(row0, row1, row2, row3) {}

  EoGeTransformMatrix(const EoGePoint3d& referencePoint, const EoGeVector3d& normal);

  EoGeTransformMatrix(const EoGePoint3d& referencePoint, const EoGeVector3d& referenceAxis, const double angle);

  EoGeTransformMatrix(const EoGePoint3d& referencePoint, const EoGeVector3d& xAxis, const EoGeVector3d& yAxis);

  [[nodiscard]] EoGeLine operator*(const EoGeLine& line) const;

  [[nodiscard]] EoGePoint3d operator*(const EoGePoint3d& point) const;

  [[nodiscard]] EoGePoint4d operator*(const EoGePoint4d& point) const;

  [[nodiscard]] EoGeVector3d operator*(const EoGeVector3d& vector) const;

  [[nodiscard]] static EoGeTransformMatrix BuildRotationTransformMatrix(const EoGeVector3d& rotationAngles);

  void AppendXAxisRotation(double xAxisAngle);

  void AppendYAxisRotation(double yAxisAngle);

  void AppendZAxisRotation(double zAxisAngle);

  void ConstructUsingReferencePointAndNormal(const EoGePoint3d& referencePoint, const EoGeVector3d& normal);

  [[nodiscard]] static EoGeTransformMatrix XAxisRotation(double sinAngle, double cosAngle) noexcept;

  [[nodiscard]] static EoGeTransformMatrix YAxisRotation(double sinAngle, double cosAngle) noexcept;

  [[nodiscard]] static EoGeTransformMatrix ZAxisRotation(double sinAngle, double cosAngle) noexcept;

  void Scale(const EoGeVector3d& scaleFactors);

  inline void Translate(const EoGeVector3d& translate) {
    m_4X4[0][3] += translate.x;
    m_4X4[1][3] += translate.y;
    m_4X4[2][3] += translate.z;
  }
};

typedef CList<EoGeTransformMatrix> EoGeTransformMatrixList;
