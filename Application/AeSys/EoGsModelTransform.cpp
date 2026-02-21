#include "Stdafx.h"

#include "EoGeMatrix.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsModelTransform.h"

EoGeTransformMatrix EoGsModelTransform::GetInverseCompositeMatrix() const {
  if (m_depth == 0) { return EoGeTransformMatrix{}; }  // Identity
  EoGeTransformMatrix inverse = m_compositeTransformMatrix;
  inverse.Inverse();
  return inverse;
}

void EoGsModelTransform::InverseTransformPoint(EoGePoint3d& point) const {
  if (m_depth > 0) {
    auto inverse = GetInverseCompositeMatrix();
    point = inverse * point;
  }
}

void EoGsModelTransform::Push() {
  m_depth++;
  m_transformMatrixList.push_back(m_compositeTransformMatrix);
}

bool EoGsModelTransform::Pop() {
  if (m_depth == 0) { return false; }
  m_depth--;
  m_compositeTransformMatrix = m_transformMatrixList.back();
  m_transformMatrixList.pop_back();
  return true;
}

void EoGsModelTransform::Reset() noexcept {
  m_depth = 0;
  m_compositeTransformMatrix.Identity();
  m_transformMatrixList.clear();
}

void EoGsModelTransform::SetLocalTM(const EoGeTransformMatrix& transformation) {
  m_compositeTransformMatrix = static_cast<EoGeMatrix>(transformation) * m_compositeTransformMatrix;
}

void EoGsModelTransform::TransformPoint(EoGePoint3d& point) const noexcept {
  if (m_depth > 0) { point = m_compositeTransformMatrix * point; }
}

void EoGsModelTransform::TransformPoint(EoGePoint4d& point) const noexcept {
  if (m_depth > 0) { point = m_compositeTransformMatrix * point; }
}

void EoGsModelTransform::TransformPoints(int numberOfPoints, EoGePoint4d* points) const noexcept {
  if (m_depth > 0 && points != nullptr) {
    for (int i = 0; i < numberOfPoints; i++) { points[i] = m_compositeTransformMatrix * points[i]; }
  }
}

void EoGsModelTransform::TransformPoints(EoGePoint4dArray& pointsArray) const noexcept {
  if (m_depth > 0) {
    auto numberOfPoints = pointsArray.GetSize();
    for (decltype(numberOfPoints) i = 0; i < numberOfPoints; i++) {
      pointsArray[i] = m_compositeTransformMatrix * pointsArray[i];
    }
  }
}

void EoGsModelTransform::TransformVector(EoGeVector3d& vector) const noexcept {
  if (m_depth > 0) { vector = m_compositeTransformMatrix * vector; }
}