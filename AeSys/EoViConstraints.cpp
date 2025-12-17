#include "stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "PrimState.h"

void AeSysView::InitializeConstraints() {
  m_AxisConstraintInfluenceAngle = 5.;
  m_AxisConstraintOffsetAngle = 0.;

  m_XGridSnapSpacing = 1.;
  m_YGridSnapSpacing = 1.;
  m_ZGridSnapSpacing = 1.;
  m_XGridLineSpacing = 12.;
  m_YGridLineSpacing = 12.;
  m_ZGridLineSpacing = 12.;
  m_XGridPointSpacing = 3.;
  m_YGridPointSpacing = 3.;
  m_ZGridPointSpacing = 0.;
  m_MaximumDotsPerLine = 64;

  m_DisplayGridWithLines = false;
  m_DisplayGridWithPoints = false;
  m_GridSnap = false;
}
double AeSysView::AxisConstraintInfluenceAngle() const { return m_AxisConstraintInfluenceAngle; }
void AeSysView::SetAxisConstraintInfluenceAngle(const double angle) { m_AxisConstraintInfluenceAngle = angle; }
double AeSysView::AxisConstraintOffsetAngle() const { return m_AxisConstraintOffsetAngle; }
void AeSysView::SetAxisConstraintOffsetAngle(const double angle) { m_AxisConstraintOffsetAngle = angle; }
EoGePoint3d AeSysView::GridOrign() const { return m_GridOrigin; }
void AeSysView::GridOrign(const EoGePoint3d& origin) { m_GridOrigin = origin; }
bool AeSysView::DisplayGridWithLines() const { return m_DisplayGridWithLines; }
void AeSysView::EnableDisplayGridWithLines(bool display) { m_DisplayGridWithLines = display; }
void AeSysView::EnableDisplayGridWithPoints(bool display) { m_DisplayGridWithPoints = display; }
bool AeSysView::DisplayGridWithPoints() const { return m_DisplayGridWithPoints; }
bool AeSysView::GridSnap() const { return m_GridSnap; }
void AeSysView::EnableGridSnap(bool snap) { m_GridSnap = snap; }
void AeSysView::GetGridLineSpacing(double& x, double& y, double& z) {
  x = m_XGridLineSpacing;
  y = m_YGridLineSpacing;
  z = m_ZGridLineSpacing;
}
void AeSysView::SetGridLineSpacing(double x, double y, double z) {
  m_XGridLineSpacing = x;
  m_YGridLineSpacing = y;
  m_ZGridLineSpacing = z;
}
void AeSysView::GetGridPointSpacing(double& x, double& y, double& z) {
  x = m_XGridPointSpacing;
  y = m_YGridPointSpacing;
  z = m_ZGridPointSpacing;
}
void AeSysView::SetGridPointSpacing(double x, double y, double z) {
  m_XGridPointSpacing = x;
  m_YGridPointSpacing = y;
  m_ZGridPointSpacing = z;
}
void AeSysView::GetGridSnapSpacing(double& x, double& y, double& z) {
  x = m_XGridSnapSpacing;
  y = m_YGridSnapSpacing;
  z = m_ZGridSnapSpacing;
}
void AeSysView::SetGridSnapSpacing(double x, double y, double z) {
  m_XGridSnapSpacing = x;
  m_YGridSnapSpacing = y;
  m_ZGridSnapSpacing = z;
}
void AeSysView::DisplayGrid(CDC* deviceContext) {
  double dHalfPts = m_MaximumDotsPerLine * 0.5;

  if (DisplayGridWithPoints()) {
    EoGePoint3d pt;

    if (fabs(m_YGridPointSpacing) > DBL_EPSILON && fabs(m_ZGridPointSpacing) > DBL_EPSILON) {
      COLORREF Color = app.PenColorsGetHot(1);

      pt.x = m_GridOrigin.x;
      pt.z = m_GridOrigin.z - dHalfPts * m_ZGridPointSpacing;
      for (int i = 0; i < m_MaximumDotsPerLine; i++) {
        pt.y = m_GridOrigin.y - dHalfPts * m_YGridPointSpacing;
        for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++) {
          DisplayPixel(deviceContext, Color, pt);
          pt.y += m_YGridPointSpacing;
        }
        pt.z += m_ZGridPointSpacing;
      }
    }
    if (fabs(m_XGridPointSpacing) > DBL_EPSILON && fabs(m_ZGridPointSpacing) > DBL_EPSILON) {
      COLORREF Color = app.PenColorsGetHot(2);

      pt.x = m_GridOrigin.x - dHalfPts * m_XGridPointSpacing;
      pt.y = m_GridOrigin.y;
      for (int i = 0; i < m_MaximumDotsPerLine; i++) {
        pt.z = m_GridOrigin.z - dHalfPts * m_ZGridPointSpacing;
        for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++) {
          DisplayPixel(deviceContext, Color, pt);
          pt.z += m_ZGridPointSpacing;
        }
        pt.x += m_XGridPointSpacing;
      }
    }
    if (fabs(m_XGridPointSpacing) > DBL_EPSILON && fabs(m_YGridPointSpacing) > DBL_EPSILON) {
      COLORREF Color = app.PenColorsGetHot(3);

      pt.y = m_GridOrigin.y - dHalfPts * m_YGridPointSpacing;
      pt.z = m_GridOrigin.z;
      for (int i = 0; i < m_MaximumDotsPerLine; i++) {
        pt.x = m_GridOrigin.x - dHalfPts * m_XGridPointSpacing;
        for (int i2 = 0; i2 < m_MaximumDotsPerLine; i2++) {
          DisplayPixel(deviceContext, Color, pt);
          pt.x += m_XGridPointSpacing;
        }
        pt.y += m_YGridPointSpacing;
      }
    }
  }
  if (DisplayGridWithLines()) {
    if (fabs(m_XGridLineSpacing) > DBL_EPSILON && fabs(m_YGridLineSpacing) > DBL_EPSILON) {
      EoGeLine ln;

      int i;
      EoInt16 nPenColor = pstate.PenColor();
      EoInt16 LineType = pstate.LineType();
      pstate.SetPen(this, deviceContext, 250, 1);

      ln.begin.x = m_GridOrigin.x - dHalfPts * m_XGridLineSpacing;
      ln.end.x = m_GridOrigin.x + dHalfPts * m_XGridLineSpacing;
      ln.begin.y = m_GridOrigin.y - dHalfPts * m_YGridLineSpacing;
      ln.begin.z = m_GridOrigin.z;
      ln.end.z = m_GridOrigin.z;
      for (i = 0; i < m_MaximumDotsPerLine; i++) {
        ln.end.y = ln.begin.y;
        ln.Display(this, deviceContext);
        ln.begin.y += m_YGridLineSpacing;
      }
      ln.begin.y = m_GridOrigin.y - dHalfPts * m_YGridLineSpacing;
      ln.end.y = m_GridOrigin.y + dHalfPts * m_YGridLineSpacing;
      for (i = 0; i < m_MaximumDotsPerLine; i++) {
        ln.end.x = ln.begin.x;
        ln.Display(this, deviceContext);
        ln.begin.x += m_XGridLineSpacing;
      }
      pstate.SetPen(this, deviceContext, nPenColor, LineType);
    }
  }
}
EoGePoint3d AeSysView::SnapPointToAxis(EoGePoint3d& beginPoint, EoGePoint3d& endPoint) {
  EoGeLine Line(beginPoint, endPoint);

  return (Line.ConstrainToAxis(m_AxisConstraintInfluenceAngle, m_AxisConstraintOffsetAngle));
}
EoGePoint3d AeSysView::SnapPointToGrid(EoGePoint3d& arPt) {
  EoGePoint3d pt = arPt;

  if (GridSnap()) {
    if (fabs(m_XGridSnapSpacing) > DBL_EPSILON) {
      pt.x -= fmod((arPt.x - m_GridOrigin.x), m_XGridSnapSpacing);
      if (fabs(pt.x - arPt.x) > m_XGridSnapSpacing * 0.5)
        pt.x += Eo::CopySign(m_XGridSnapSpacing, arPt.x - m_GridOrigin.x);
    }
    if (fabs(m_YGridSnapSpacing) > DBL_EPSILON) {
      pt.y -= fmod((arPt.y - m_GridOrigin.y), m_YGridSnapSpacing);
      if (fabs(pt.y - arPt.y) > m_YGridSnapSpacing * 0.5)
        pt.y += Eo::CopySign(m_YGridSnapSpacing, arPt.y - m_GridOrigin.y);
    }
    if (fabs(m_ZGridSnapSpacing) > DBL_EPSILON) {
      pt.z -= fmod((arPt.z - m_GridOrigin.z), m_ZGridSnapSpacing);
      if (fabs(pt.z - arPt.z) > m_ZGridSnapSpacing * 0.5)
        pt.z += Eo::CopySign(m_ZGridSnapSpacing, arPt.z - m_GridOrigin.z);
    }
  }
  return (pt);
}
