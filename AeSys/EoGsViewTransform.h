#pragma once

#include "EoGsAbstractView.h"

class EoGsViewport;

class EoGsViewTransform : public EoGsAbstractView {
  double m_UMin;
  double m_VMin;
  double m_UMax;
  double m_VMax;

  EoGeTransformMatrix m_Matrix;
  EoGeTransformMatrix m_ProjectionMatrix;

  EoGeTransformMatrix m_InverseMatrix;

 public:  // Constructors and destructor
  EoGsViewTransform();
  EoGsViewTransform(EoGsViewTransform& src);

  ~EoGsViewTransform() override {}

 public:  // Operators
  EoGsViewTransform& operator=(const EoGsViewTransform& src);

 public:  // Methods
  void AdjustWindow(const double aspectRatio);
  void BuildTransformMatrix();
  EoGeTransformMatrix& GetMatrix();
  EoGeTransformMatrix& GetMatrixInverse();
  void Initialize(const EoGsViewport& viewport);
  void LoadIdentity();
  void ZAxisRotation(double dSinAng, double dCosAng);
  void Scale(EoGeVector3d scale);
  /// <summary> Sets a window which is centered on the view target after adjusting for viewport aspect ratio</summary>
  void SetCenteredWindow(const EoGsViewport& viewport, double uExtent, double vExtent);
  void SetMatrix(EoGeTransformMatrix& tm);
  void SetWindow(const double uMin, const double vMin, const double uMax, const double vMax);
  void TransformPoint(EoGePoint4d& point);
  void TransformPoints(EoGePoint4dArray& pointsArray);
  void TransformPoints(int numberOfPoints, EoGePoint4d* points);
  void TransformVector(EoGeVector3d& vector);
  void Translate(EoGeVector3d translate);
  double UExtent() const;
  double UMax() const;
  double UMin() const;
  double VExtent() const;
  double VMax() const;
  double VMin() const;
};

typedef CList<EoGsViewTransform> EoGsViewTransforms;
