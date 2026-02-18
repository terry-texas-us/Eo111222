#include "Stdafx.h"

#include "AeSysView.h"
#include "EoGePoint4d.h"

EoGeVector3d AeSysView::CameraDirection() const { return m_ViewTransform.Direction(); }
EoGePoint3d AeSysView::CameraTarget() const { return m_ViewTransform.Target(); }
void AeSysView::CopyActiveModelViewToPreviousModelView() { m_PreviousViewTransform = m_ViewTransform; }
void AeSysView::ExchangeActiveAndPreviousModelViews() {
  EoGsViewTransform ModelView(m_ViewTransform);
  m_ViewTransform = m_PreviousViewTransform;
  m_PreviousViewTransform = ModelView;
}
EoGsViewTransform AeSysView::PreviousModelView() { return m_PreviousViewTransform; }
void AeSysView::SetCameraPosition(const EoGeVector3d& direction) {
  m_ViewTransform.SetPosition(direction);
  m_ViewTransform.BuildTransformMatrix();
}
void AeSysView::SetCameraTarget(const EoGePoint3d& target) {
  m_ViewTransform.SetTarget(target);
  m_ViewTransform.BuildTransformMatrix();
}
/** @brief Sets the view window to be centered in the viewport with specified extents.
 *
 * This method adjusts the view transformation to center the view window
 * within the current viewport, using the provided uExtent and vExtent values.
 *
 * @param uExtent The extent of the view window in the U direction.
 * @param vExtent The extent of the view window in the V direction.
 */
void AeSysView::SetCenteredWindow(double uExtent, double vExtent) {
  m_ViewTransform.SetCenteredWindow(m_Viewport, uExtent, vExtent);
}
void AeSysView::SetViewTransform(EoGsViewTransform& viewTransform) {
  m_ViewTransform = viewTransform;
  m_ViewTransform.BuildTransformMatrix();
}
void AeSysView::SetViewWindow(double uMin, double vMin, double uMax, double vMax) {
  m_ViewTransform.SetWindow(uMin, vMin, uMax, vMax);
}
void AeSysView::ModelViewGetViewport(EoGsViewport& viewport) { viewport = m_Viewport; }
EoGeTransformMatrix& AeSysView::ModelViewGetMatrix() { return m_ViewTransform.GetMatrix(); }
EoGeTransformMatrix& AeSysView::ModelViewGetMatrixInverse() { return m_ViewTransform.GetMatrixInverse(); }
double AeSysView::UExtent() const { return m_ViewTransform.UExtent(); }
double AeSysView::UMax() const { return m_ViewTransform.UMax(); }
double AeSysView::UMin() const { return m_ViewTransform.UMin(); }
EoGeVector3d AeSysView::ViewUp() const { return m_ViewTransform.ViewUp(); }
double AeSysView::VExtent() const { return m_ViewTransform.VExtent(); }
double AeSysView::VMax() const { return m_ViewTransform.VMax(); }
double AeSysView::VMin() const { return m_ViewTransform.VMin(); }
void AeSysView::ModelViewInitialize() { m_ViewTransform.Initialize(m_Viewport); }

void AeSysView::ModelTransformPoint(EoGePoint3d& point) { m_ModelTransform.TransformPoint(point); }
void AeSysView::ModelTransformPoint(EoGePoint4d& point) { m_ModelTransform.TransformPoint(point); }

void AeSysView::ModelTransformVector(EoGeVector3d vector) { m_ModelTransform.TransformVector(vector); }

void AeSysView::ModelViewTransformPoint(EoGePoint4d& ndcPoint) {
  m_ModelTransform.TransformPoint(ndcPoint);
  m_ViewTransform.TransformPoint(ndcPoint);
}
void AeSysView::ModelViewTransformPoints(EoGePoint4dArray& pointsArray) {
  m_ModelTransform.TransformPoints(pointsArray);
  m_ViewTransform.TransformPoints(pointsArray);
}
void AeSysView::ModelViewTransformPoints(int numberOfPoints, EoGePoint4d* points) {
  m_ModelTransform.TransformPoints(numberOfPoints, points);
  m_ViewTransform.TransformPoints(numberOfPoints, points);
}
void AeSysView::ModelViewTransformVector(EoGeVector3d& vector) {
  m_ModelTransform.TransformVector(vector);
  m_ViewTransform.TransformVector(vector);
}
