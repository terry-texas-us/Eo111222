#pragma once

#include "EoGeMatrix.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"

class EoGeLine;
class EoGeTransformMatrix : public EoGeMatrix {
 public:
  EoGeTransformMatrix() : EoGeMatrix() { EoGeMatrix::Identity(); }

  EoGeTransformMatrix(const EoGeMatrix& matrix) : EoGeMatrix(matrix) {}

  EoGeTransformMatrix(EoGeMatrixRow& row0, EoGeMatrixRow& row1, EoGeMatrixRow& row2, EoGeMatrixRow& row3) : EoGeMatrix(row0, row1, row2, row3) {}

  EoGeTransformMatrix(EoGePoint3d point, EoGeVector3d normal);

  EoGeTransformMatrix(const EoGePoint3d& referencePoint, EoGeVector3d referenceAxis, const double angle);

  EoGeTransformMatrix(EoGePoint3d pt, EoGeVector3d xReference, EoGeVector3d yReference);

  EoGeLine operator*(const EoGeLine& line);

  EoGePoint3d operator*(const EoGePoint3d& point);

  EoGePoint4d operator*(const EoGePoint4d& point);

  EoGeVector3d operator*(const EoGeVector3d& vector);

  EoGeTransformMatrix BuildRotationTransformMatrix(const EoGeVector3d& rotationAngles) const;

  void AppendXAxisRotation(double xAxisAngle);

  void AppendYAxisRotation(double yAxisAngle);

  void AppendZAxisRotation(double zAxisAngle);

  void ConstructUsingReferencePointAndNormal(EoGePoint3d ptP, EoGeVector3d vN);

  EoGeTransformMatrix XAxisRotation(const double dSinAng, const double dCosAng);

  EoGeTransformMatrix YAxisRotation(const double dSinAng, const double dCosAng);

  EoGeTransformMatrix ZAxisRotation(const double dSinAng, const double dCosAng);

  void Scale(EoGeVector3d scaleFactors);

  inline void Translate(EoGeVector3d translate) {
    m_4X4[0][3] += translate.x;
    m_4X4[1][3] += translate.y;
    m_4X4[2][3] += translate.z;
  }
};

typedef CList<EoGeTransformMatrix> EoGeTransformMatrixList;
