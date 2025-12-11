#include "stdafx.h"

#include "EoGsModelTransform.h"

EoGsModelTransform::EoGsModelTransform() { m_Depth = 0; }
EoGsModelTransform::~EoGsModelTransform() {}
void EoGsModelTransform::InvokeNew() {
  m_Depth++;
  m_TransformMatrixList.AddTail(m_CompositeTransformMatrix);
}
void EoGsModelTransform::Return() {
  m_Depth--;
  m_CompositeTransformMatrix = m_TransformMatrixList.RemoveTail();
}
void EoGsModelTransform::SetLocalTM(EoGeTransformMatrix& transformation) {
  m_CompositeTransformMatrix = (EoGeMatrix)transformation * m_CompositeTransformMatrix;
}
void EoGsModelTransform::TransformPoint(EoGePoint3d& pt) {
  if (m_Depth > 0) { pt = m_CompositeTransformMatrix * pt; }
}
void EoGsModelTransform::TransformPoint(EoGePoint4d& point) {
  if (m_Depth > 0) { point = m_CompositeTransformMatrix * point; }
}
void EoGsModelTransform::TransformPoints(int numberOfPoints, EoGePoint4d* points) {
  if (m_Depth > 0) {
    for (int i = 0; i < numberOfPoints; i++) { points[i] = m_CompositeTransformMatrix * points[i]; }
  }
}
void EoGsModelTransform::TransformPoints(EoGePoint4dArray& pointsArray) {
  if (m_Depth > 0) {
    int NumberOfPoints = (int)pointsArray.GetSize();
    for (int i = 0; i < NumberOfPoints; i++) { pointsArray[i] = m_CompositeTransformMatrix * pointsArray[i]; }
  }
}

void EoGsModelTransform::TransformVector(EoGeVector3d& vector) {
  if (m_Depth > 0) { vector = m_CompositeTransformMatrix * vector; }
}
