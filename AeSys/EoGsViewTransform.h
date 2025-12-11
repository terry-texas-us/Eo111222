#pragma once

#include "EoGsAbstractView.h"

class EoGsViewTransform : public EoGsAbstractView {
  float m_UMin;
  float m_VMin;
  float m_UMax;
  float m_VMax;

  EoGeTransformMatrix m_Matrix;
  EoGeTransformMatrix m_ProjectionMatrix;

  EoGeTransformMatrix m_InverseMatrix;

 public:  // Constructors and destructor
  EoGsViewTransform();
  EoGsViewTransform(EoGsViewTransform& src);

  ~EoGsViewTransform() {}

 public:  // Operators
  EoGsViewTransform& operator=(const EoGsViewTransform& src);

 public:  // Methods
  void AdjustWindow(const float aspectRatio);
  void BuildTransformMatrix();
  EoGeTransformMatrix& GetMatrix();
  EoGeTransformMatrix& GetMatrixInverse();
  void Initialize(const EoGsViewport& viewport);
  void LoadIdentity();
  void ZAxisRotation(double dSinAng, double dCosAng);
  void Scale(EoGeVector3d scale);
  /// <summary> Sets a window which is centered on the view target after adjusting for viewport aspect ratio</summary>
  void SetCenteredWindow(const EoGsViewport& viewport, float uExtent, float vExtent);
  void SetMatrix(EoGeTransformMatrix& tm);
  void SetWindow(const float uMin, const float vMin, const float uMax, const float vMax);
  void TransformPoint(EoGePoint4d& point);
  void TransformPoints(EoGePoint4dArray& pointsArray);
  void TransformPoints(int numberOfPoints, EoGePoint4d* points);
  void TransformVector(EoGeVector3d& vector);
  void Translate(EoGeVector3d translate);
  float UExtent() const;
  float UMax() const;
  float UMin() const;
  float VExtent() const;
  float VMax() const;
  float VMin() const;
};

typedef CList<EoGsViewTransform> EoGsViewTransforms;
