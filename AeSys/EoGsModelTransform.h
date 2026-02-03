#pragma once

#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoGsModelTransform {
  EoUInt16 m_depth;

  EoGeTransformMatrix m_CompositeTransformMatrix;
  EoGeTransformMatrixList m_TransformMatrixList;

 public:
  EoGsModelTransform();
  EoGsModelTransform(const EoGsModelTransform&) = delete;
  EoGsModelTransform& operator=(const EoGsModelTransform&) = delete;

  ~EoGsModelTransform();

  /** Places an identity transform on the top of the current transformation stack.
   */
  void InvokeNew();

  /** Removes the top transformation off the current transformation stack.
   */
  [[nodiscard]] void Return();

  void TransformPoint(EoGePoint3d& point) noexcept;
  void TransformPoint(EoGePoint4d& point) noexcept;
  void TransformPoints(EoGePoint4dArray& pointsArray);
  void TransformPoints(int numberOfPoints, EoGePoint4d* points);
  void TransformVector(EoGeVector3d& vector) noexcept;

  /** The specified transformation is concatenated to the current model transformation (which is initially the identity transform).
   */
  void SetLocalTM(const EoGeTransformMatrix& transformation);
};
