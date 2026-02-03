#include "Stdafx.h"

#include "EoGeMatrix.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsModelTransform.h"

EoGsModelTransform::EoGsModelTransform() { m_depth = 0; }

EoGsModelTransform::~EoGsModelTransform() {}

void EoGsModelTransform::InvokeNew() {
  m_depth++;
  m_TransformMatrixList.AddTail(m_CompositeTransformMatrix);
}

void EoGsModelTransform::Return() {
  if (m_depth == 0) { return; }
  m_depth--;
  m_CompositeTransformMatrix = m_TransformMatrixList.RemoveTail();
}

void EoGsModelTransform::SetLocalTM(const EoGeTransformMatrix& transformation) {
  m_CompositeTransformMatrix = (EoGeMatrix)transformation * m_CompositeTransformMatrix;
}

void EoGsModelTransform::TransformPoint(EoGePoint3d& point) noexcept {
  if (m_depth > 0) { point = m_CompositeTransformMatrix * point; }
}

void EoGsModelTransform::TransformPoint(EoGePoint4d& point) noexcept {
  if (m_depth > 0) { point = m_CompositeTransformMatrix * point; }
}

void EoGsModelTransform::TransformPoints(int numberOfPoints, EoGePoint4d* points) {
  if (m_depth > 0 && points != nullptr) {
    for (int i = 0; i < numberOfPoints; i++) { points[i] = m_CompositeTransformMatrix * points[i]; }
  }
}

void EoGsModelTransform::TransformPoints(EoGePoint4dArray& pointsArray) {
  if (m_depth > 0) {
    int numberOfPoints = (int)pointsArray.GetSize();
    for (int i = 0; i < numberOfPoints; i++) { pointsArray[i] = m_CompositeTransformMatrix * pointsArray[i]; }
  }
}

void EoGsModelTransform::TransformVector(EoGeVector3d& vector) noexcept {
  if (m_depth > 0) { vector = m_CompositeTransformMatrix * vector; }
}
