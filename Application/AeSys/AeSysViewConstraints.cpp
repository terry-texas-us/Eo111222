#include "Stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoGsRenderDeviceGdi.h"
#include "EoGsRenderState.h"

void AeSysView::InitializeConstraints() noexcept {
  m_AxisConstraintInfluenceAngle = 5.0;
  m_AxisConstraintOffsetAngle = 0.0;

  m_XGridSnapSpacing = 1.0;
  m_YGridSnapSpacing = 1.0;
  m_ZGridSnapSpacing = 1.0;
  m_XGridLineSpacing = 12.0;
  m_YGridLineSpacing = 12.0;
  m_ZGridLineSpacing = 12.0;
  m_XGridPointSpacing = 3.0;
  m_YGridPointSpacing = 3.0;
  m_ZGridPointSpacing = 0.0;
  m_MaximumDotsPerLine = 64;

  m_DisplayGridWithLines = false;
  m_DisplayGridWithPoints = false;
  m_GridSnap = false;
}

double AeSysView::AxisConstraintInfluenceAngle() const noexcept {
  return m_AxisConstraintInfluenceAngle;
}
void AeSysView::SetAxisConstraintInfluenceAngle(double angle) noexcept {
  m_AxisConstraintInfluenceAngle = angle;
}
double AeSysView::AxisConstraintOffsetAngle() const noexcept {
  return m_AxisConstraintOffsetAngle;
}
void AeSysView::SetAxisConstraintOffsetAngle(double angle) noexcept {
  m_AxisConstraintOffsetAngle = angle;
}
EoGePoint3d AeSysView::GridOrign() const noexcept {
  return m_GridOrigin;
}
void AeSysView::GridOrign(const EoGePoint3d& origin) noexcept {
  m_GridOrigin = origin;
}
bool AeSysView::DisplayGridWithLines() const noexcept {
  return m_DisplayGridWithLines;
}
void AeSysView::EnableDisplayGridWithLines(bool display) noexcept {
  m_DisplayGridWithLines = display;
}
void AeSysView::EnableDisplayGridWithPoints(bool display) noexcept {
  m_DisplayGridWithPoints = display;
}
bool AeSysView::DisplayGridWithPoints() const noexcept {
  return m_DisplayGridWithPoints;
}
bool AeSysView::GridSnap() const noexcept {
  return m_GridSnap;
}
void AeSysView::EnableGridSnap(bool snap) noexcept {
  m_GridSnap = snap;
}

void AeSysView::GetGridLineSpacing(double& x, double& y, double& z) const noexcept {
  x = m_XGridLineSpacing;
  y = m_YGridLineSpacing;
  z = m_ZGridLineSpacing;
}

void AeSysView::SetGridLineSpacing(double x, double y, double z) noexcept {
  m_XGridLineSpacing = x;
  m_YGridLineSpacing = y;
  m_ZGridLineSpacing = z;
}

void AeSysView::GetGridPointSpacing(double& x, double& y, double& z) const noexcept {
  x = m_XGridPointSpacing;
  y = m_YGridPointSpacing;
  z = m_ZGridPointSpacing;
}

void AeSysView::SetGridPointSpacing(double x, double y, double z) noexcept {
  m_XGridPointSpacing = x;
  m_YGridPointSpacing = y;
  m_ZGridPointSpacing = z;
}

void AeSysView::GetGridSnapSpacing(double& x, double& y, double& z) const noexcept {
  x = m_XGridSnapSpacing;
  y = m_YGridSnapSpacing;
  z = m_ZGridSnapSpacing;
}

void AeSysView::SetGridSnapSpacing(double x, double y, double z) noexcept {
  m_XGridSnapSpacing = x;
  m_YGridSnapSpacing = y;
  m_ZGridSnapSpacing = z;
}

void AeSysView::DisplayGrid(CDC* deviceContext) {
  const double dHalfPts = m_MaximumDotsPerLine * 0.5;

  if (DisplayGridWithPoints()) {
    EoGePoint3d pt;

    if (Eo::IsGeometricallyNonZero(m_YGridPointSpacing) && Eo::IsGeometricallyNonZero(m_ZGridPointSpacing)) {
      const auto color = app.PenColorsGetHot(1);

      pt.x = m_GridOrigin.x;
      pt.z = m_GridOrigin.z - dHalfPts * m_ZGridPointSpacing;
      for (int i = 0; i < m_MaximumDotsPerLine; i++) {
        pt.y = m_GridOrigin.y - dHalfPts * m_YGridPointSpacing;
        for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++) {
          DisplayPixel(deviceContext, color, pt);
          pt.y += m_YGridPointSpacing;
        }
        pt.z += m_ZGridPointSpacing;
      }
    }
    if (Eo::IsGeometricallyNonZero(m_XGridPointSpacing) && Eo::IsGeometricallyNonZero(m_ZGridPointSpacing)) {
      const auto color = app.PenColorsGetHot(2);

      pt.x = m_GridOrigin.x - dHalfPts * m_XGridPointSpacing;
      pt.y = m_GridOrigin.y;
      for (int i = 0; i < m_MaximumDotsPerLine; i++) {
        pt.z = m_GridOrigin.z - dHalfPts * m_ZGridPointSpacing;
        for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++) {
          DisplayPixel(deviceContext, color, pt);
          pt.z += m_ZGridPointSpacing;
        }
        pt.x += m_XGridPointSpacing;
      }
    }
    if (Eo::IsGeometricallyNonZero(m_XGridPointSpacing) && Eo::IsGeometricallyNonZero(m_YGridPointSpacing)) {
      const auto color = app.PenColorsGetHot(3);

      pt.y = m_GridOrigin.y - dHalfPts * m_YGridPointSpacing;
      pt.z = m_GridOrigin.z;
      for (int i = 0; i < m_MaximumDotsPerLine; i++) {
        pt.x = m_GridOrigin.x - dHalfPts * m_XGridPointSpacing;
        for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++) {
          DisplayPixel(deviceContext, color, pt);
          pt.x += m_XGridPointSpacing;
        }
        pt.y += m_YGridPointSpacing;
      }
    }
  }
  if (DisplayGridWithLines()) {
    if (Eo::IsGeometricallyNonZero(m_XGridLineSpacing) && Eo::IsGeometricallyNonZero(m_YGridLineSpacing)) {
      EoGeLine line;
      EoGsRenderDeviceGdi renderDevice(deviceContext);

      int i;
      const auto color = Gs::renderState.Color();
      const auto lineType = Gs::renderState.LineTypeIndex();
      Gs::renderState.SetPen(this, deviceContext, 250, 1);

      line.begin.x = m_GridOrigin.x - dHalfPts * m_XGridLineSpacing;
      line.end.x = m_GridOrigin.x + dHalfPts * m_XGridLineSpacing;
      line.begin.y = m_GridOrigin.y - dHalfPts * m_YGridLineSpacing;
      line.begin.z = m_GridOrigin.z;
      line.end.z = m_GridOrigin.z;
      for (i = 0; i < m_MaximumDotsPerLine; i++) {
        line.end.y = line.begin.y;
        line.Display(this, &renderDevice);
        line.begin.y += m_YGridLineSpacing;
      }
      line.begin.y = m_GridOrigin.y - dHalfPts * m_YGridLineSpacing;
      line.end.y = m_GridOrigin.y + dHalfPts * m_YGridLineSpacing;
      for (i = 0; i < m_MaximumDotsPerLine; i++) {
        line.end.x = line.begin.x;
        line.Display(this, &renderDevice);
        line.begin.x += m_XGridLineSpacing;
      }
      Gs::renderState.SetPen(this, deviceContext, color, lineType);
    }
  }
}
EoGePoint3d AeSysView::SnapPointToAxis(const EoGePoint3d& begin, const EoGePoint3d& end) const {
  const EoGeLine line(begin, end);

  return (line.ConstrainToAxis(m_AxisConstraintInfluenceAngle, m_AxisConstraintOffsetAngle));
}

EoGePoint3d AeSysView::SnapPointToGrid(const EoGePoint3d& point) const {
  EoGePoint3d snappedPoint = point;

  if (GridSnap()) {
    if (Eo::IsGeometricallyNonZero(m_XGridSnapSpacing)) {
      snappedPoint.x -= fmod((point.x - m_GridOrigin.x), m_XGridSnapSpacing);
      if (std::abs(snappedPoint.x - point.x) > m_XGridSnapSpacing * 0.5) {
        snappedPoint.x += Eo::CopySign(m_XGridSnapSpacing, point.x - m_GridOrigin.x);
      }
    }
    if (Eo::IsGeometricallyNonZero(m_YGridSnapSpacing)) {
      snappedPoint.y -= fmod((point.y - m_GridOrigin.y), m_YGridSnapSpacing);
      if (std::abs(snappedPoint.y - point.y) > m_YGridSnapSpacing * 0.5) {
        snappedPoint.y += Eo::CopySign(m_YGridSnapSpacing, point.y - m_GridOrigin.y);
      }
    }
    if (Eo::IsGeometricallyNonZero(m_ZGridSnapSpacing)) {
      snappedPoint.z -= fmod((point.z - m_GridOrigin.z), m_ZGridSnapSpacing);
      if (std::abs(snappedPoint.z - point.z) > m_ZGridSnapSpacing * 0.5) {
        snappedPoint.z += Eo::CopySign(m_ZGridSnapSpacing, point.z - m_GridOrigin.z);
      }
    }
  }
  return snappedPoint;
}
