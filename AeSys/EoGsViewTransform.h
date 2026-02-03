#pragma once

#include "EoGeTransformMatrix.h"
#include "EoGsAbstractView.h"

class EoGsViewport;

/** @class  Manages view transformations including scaling, rotation, and translation.
 * 
 * The EoGsViewTransform class is responsible for handling view transformations in a 3D graphics context.
 * It provides methods to adjust the view window, build transformation matrices, and apply various transformations
 * such as scaling, rotation, and translation to points and vectors in 3D space.
 *
 * @note View space, sometimes called camera space, is similar to world space in that it is typically used for the entire scene.
 * However, in view space, the origin is at the viewer or camera. The view direction (where the viewer is looking) defines the positive Z axis.
 * An "up" direction defined by the application becomes the positive Y axis.
 */
class EoGsViewTransform : public EoGsAbstractView {
  double m_UMin;
  double m_VMin;
  double m_UMax;
  double m_VMax;

  EoGeTransformMatrix m_Matrix;
  EoGeTransformMatrix m_ProjectionMatrix;

  EoGeTransformMatrix m_InverseMatrix;

 public:
  EoGsViewTransform();
  EoGsViewTransform(const EoGsViewTransform& other);

  ~EoGsViewTransform() override {}

 public:  // Operators
  EoGsViewTransform& operator=(const EoGsViewTransform& src);

 public:  // Methods
  void AdjustWindow(const double aspectRatio);
  void BuildTransformMatrix();
  [[nodiscard]] EoGeTransformMatrix& GetMatrix();
  [[nodiscard]] EoGeTransformMatrix& GetMatrixInverse();
  void Initialize(const EoGsViewport& viewport);
  void LoadIdentity();
  void ZAxisRotation(double dSinAng, double dCosAng);
  void Scale(EoGeVector3d scale);

  /** Sets a window which is centered on the view target after adjusting for viewport aspect ratio
   */
  void SetCenteredWindow(const EoGsViewport& viewport, double uExtent, double vExtent);

  void SetMatrix(EoGeTransformMatrix& tm);
  void SetWindow(const double uMin, const double vMin, const double uMax, const double vMax);
  void TransformPoint(EoGePoint4d& point);
  void TransformPoints(EoGePoint4dArray& pointsArray);
  void TransformPoints(int numberOfPoints, EoGePoint4d* points);
  void TransformVector(EoGeVector3d& vector);
  void Translate(EoGeVector3d translate);
  [[nodiscard]] double UExtent() const;
  [[nodiscard]] double UMax() const;
  [[nodiscard]] double UMin() const;
  [[nodiscard]] double VExtent() const;
  [[nodiscard]] double VMax() const;
  [[nodiscard]] double VMin() const;
};

typedef CList<EoGsViewTransform> EoGsViewTransforms;
