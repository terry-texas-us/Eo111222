#pragma once

class EoGsModelTransform {
  EoUInt16 m_Depth;

  EoGeTransformMatrix m_CompositeTransformMatrix;
  EoGeTransformMatrixList m_TransformMatrixList;

 public:
  EoGsModelTransform();
  EoGsModelTransform(const EoGsModelTransform&) = delete;
  EoGsModelTransform& operator=(const EoGsModelTransform&) = delete;


  ~EoGsModelTransform();
  /// <summary> Places an identity transform on the top of the current transformation stack.</summary>
  void InvokeNew();
  /// <summary> Removes the top transformation off the current transformation stack.</summary>
  void Return();
  void TransformPoint(EoGePoint3d& pt);
  void TransformPoint(EoGePoint4d& point);
  void TransformPoints(EoGePoint4dArray& pointsArray);
  void TransformPoints(int numberOfPoints, EoGePoint4d* points);
  void TransformVector(EoGeVector3d& vector);
  /// <summary> The specified transformation is concatenated to the current model transformation (which is initially the identity transform).</summary>
  void SetLocalTM(EoGeTransformMatrix& transformation);
};
