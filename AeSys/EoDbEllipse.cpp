#include "Stdafx.h"
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>
#include <algorithm>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdlib>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbEllipse.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"
#include "drw_base.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

namespace {

EoGePoint3d pFndPtOnArc(EoGePoint3d centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, const double dAng) {
  EoGeTransformMatrix tm(centerPoint, majorAxis, minorAxis);
  tm.Inverse();

  EoGePoint3d pt(cos(dAng), sin(dAng), 0.0);

  pt = tm * pt;
  return (pt);
}

}  // namespace

/**
 * @brief Normalizes an angle to the range [0, 2π).
 *
 * This function takes an input angle in radians and normalizes it to be within the range of 0 (inclusive) to 2π (exclusive).
 * If the input angle is not finite, it returns 0.0.
 *
 * @param angle The angle in radians to be normalized.
 * @return The normalized angle in the range [0, 2π).
 */
double EoDbEllipse::NormalizeTo2Pi(double angle) {
  if (!std::isfinite(angle)) { return 0.0; }

  // Reduce large values, keep result in [0, two_pi)
  angle = std::fmod(angle, Eo::TwoPi);
  if (angle < 0.0) angle += Eo::TwoPi;

  // Snap values very close to two_pi back to 0.0 to produce canonical result
  if (angle >= Eo::TwoPi - Eo::angularEpsilon) { angle = 0.0; }

  return angle;
}

/**
 * @brief Constructs an ellipse segment defined by a center point, major and minor axes, and a sweep angle.
 *
 * This constructor initializes an ellipse segment using the specified center point, major axis, minor axis, and sweep angle.
 * The pen color and line type are set based on the current primitive state.
 *
 * @param centerPoint The center point of the ellipse.
 * @param majorAxis The major axis vector of the ellipse.
 * @param minorAxis The minor axis vector of the ellipse.
 * @param sweepAngle The sweep angle of the ellipse segment in radians.
 */
EoDbEllipse::EoDbEllipse(const EoGePoint3d& center, const EoGeVector3d& majorAxis, const EoGeVector3d& minorAxis, double sweepAngle)
    : EoDbPrimitive(pstate.PenColor(), pstate.LineType()),
      m_center(center),
      m_majorAxis(majorAxis),
      m_minorAxis(minorAxis),
      m_sweepAngle(sweepAngle) {}

/**
 * @brief Constructs a circle (as ellipse)primitive defined by a center point and radius in the current view.
 *
 * @param color The color.
 * @param lineType The line type index.
 * @param centerPoint The center point of the circle.
 * @param radius The radius of the circle.
 */
EoDbEllipse::EoDbEllipse(EoInt16 color, EoInt16 lineType, EoGePoint3d& center, double radius)
    : EoDbPrimitive(color, lineType), m_center(center) {
  auto* activeView = AeSysView::GetActiveView();

  auto normal = activeView->CameraDirection();

  m_minorAxis = activeView->ViewUp() * radius;
  m_majorAxis = m_minorAxis;
  m_majorAxis.RotAboutArbAx(normal, -Eo::HalfPi);
  m_sweepAngle = Eo::TwoPi;
}

EoDbEllipse::EoDbEllipse(EoInt16 color, EoInt16 lineType, EoGePoint3d& center, EoGeVector3d& normal, double radius)
    : EoDbPrimitive(color, lineType), m_center(center) {
  EoGeVector3d PlaneNormal(normal);
  PlaneNormal.Normalize();
  m_majorAxis = ComputeArbitraryAxis(PlaneNormal);
  m_majorAxis.Normalize();
  m_majorAxis *= radius;
  m_minorAxis = m_majorAxis;
  EoGeTransformMatrix transformMatrix(center, PlaneNormal, Eo::HalfPi);
  m_minorAxis = transformMatrix * m_minorAxis;
  m_sweepAngle = Eo::TwoPi;
}

/**
 * @brief Constructs a circle (as ellipse) primitive defined by a center point and a start point in the current view.
 *
 * This constructor initializes a circle primitive using the specified center point and a start point that defines the radius.
 * The major and minor axes are calculated based on the view's camera direction.
 * The pen color and line type are set based on the current primitive state.
 *
 * @param center The center point of the circle.
 * @param start The start point that defines the radius of the circle.
 */
EoDbEllipse::EoDbEllipse(EoGePoint3d& center, EoGePoint3d& start) {
  auto* activeView = AeSysView::GetActiveView();

  m_color = pstate.PenColor();
  m_lineTypeIndex = pstate.LineType();

  auto cameraDirection = activeView->CameraDirection();

  m_center = center;
  m_majorAxis = EoGeVector3d(center, start);
  m_minorAxis = m_majorAxis;
  m_minorAxis.RotAboutArbAx(-cameraDirection, Eo::HalfPi);
  m_sweepAngle = Eo::TwoPi;
}

/** 
 * @brief Constructs a radial arc (as ellipse primitive) from three points that define an elliptical arc.
 *
 * This constructor initializes an ellipse segment using three points: a beginning point, an intermediate point, and an end point.
 * It calculates the center point, major axis, minor axis, and sweep angle of the ellipse based on the provided points.
 * The pen color and line type are set based on the current primitive state.
 *
 * @param beginPoint The starting point of the elliptical arc.
 * @param intermediatePoint A point on the elliptical arc between the start and end points.
 * @param endPoint The ending point of the elliptical arc.
 */
EoDbEllipse::EoDbEllipse(EoGePoint3d start, EoGePoint3d intermediate, EoGePoint3d end) {
  m_color = pstate.PenColor();
  m_lineTypeIndex = pstate.LineType();

  m_sweepAngle = 0.0;

  EoGeVector3d startToIntermediate(start, intermediate);
  EoGeVector3d startToEnd(start, end);
  EoGeVector3d normal = EoGeCrossProduct(startToIntermediate, startToEnd);
  normal.Normalize();

  // Build transformation matrix which will get int and end points to z=0 plane with beg point as origin

  EoGeTransformMatrix transformMatrix(start, normal);

  EoGePoint3d pt[3]{};

  pt[0] = start;
  pt[1] = intermediate;
  pt[2] = end;

  pt[1] = transformMatrix * pt[1];
  pt[2] = transformMatrix * pt[2];

  double determinant = (pt[1].x * pt[2].y - pt[2].x * pt[1].y);

  if (fabs(determinant) > DBL_EPSILON) {  // Three points are not colinear
    double dT = ((pt[2].x - pt[1].x) * pt[2].x + pt[2].y * (pt[2].y - pt[1].y)) / determinant;

    m_center.x = (pt[1].x - pt[1].y * dT) * 0.5;
    m_center.y = (pt[1].y + pt[1].x * dT) * 0.5;
    m_center.z = 0.;
    transformMatrix.Inverse();

    // Transform back to original plane
    m_center = transformMatrix * m_center;

    // None of the points coincide with center point

    transformMatrix = EoGeTransformMatrix(m_center, normal);

    double dAng[3]{};

    pt[1] = intermediate;
    pt[2] = end;

    for (int i = 0; i < 3; i++) {  // Translate points into z=0 plane with center point at origin
      pt[i] = transformMatrix * pt[i];
      dAng[i] = atan2(pt[i].y, pt[i].x);
      if (dAng[i] < 0.0) dAng[i] += Eo::TwoPi;
    }
    double dMin = std::min(dAng[0], dAng[2]);
    double dMax = std::max(dAng[0], dAng[2]);

    if (fabs(dAng[1] - dMax) > DBL_EPSILON && fabs(dAng[1] - dMin) > DBL_EPSILON) {  // Inside line is not colinear with outside lines
      m_sweepAngle = dMax - dMin;
      if (dAng[1] > dMin && dAng[1] < dMax) {
        if (dAng[0] == dMax) m_sweepAngle = -m_sweepAngle;
      } else {
        m_sweepAngle = Eo::TwoPi - m_sweepAngle;
        if (dAng[2] == dMax) m_sweepAngle = -m_sweepAngle;
      }
      EoGePoint3d ptRot = start.RotateAboutAxis(m_center, normal, Eo::HalfPi);

      m_majorAxis = EoGeVector3d(m_center, start);
      m_minorAxis = EoGeVector3d(m_center, ptRot);
    }
  }
}

/**
 * @brief Constructs a radial arc defined by a center point, radius, start angle, and end angle.
 *
 * This constructor initializes a radial arc using the specified center point, radius, start angle, and end angle.
 * The angles are normalized to the range [0, 2π) and the sweep angle is calculated accordingly.
 *
 * @param centerPoint The center point of the ellipse.
 * @param radius The radius of the ellipse.
 * @param startAngle The starting angle of the arc in radians.
 * @param endAngle The ending angle of the arc in radians.
 * @note The sweep angle is computed to ensure a counter-clockwise direction.
 */
EoDbEllipse::EoDbEllipse(const DRW_Coord& center, double radius, double startAngle, double endAngle) {
  m_center = center;

  double normalizedStartAngle = NormalizeTo2Pi(startAngle);
  double normalizedEndAngle = NormalizeTo2Pi(endAngle);

  // Compute CCW sweep from start to end in range (0, 2*pi] (canonical representation: positive = CCW sweep)
  double sweepAngle = normalizedEndAngle - normalizedStartAngle;
  if (sweepAngle < 0.0) sweepAngle += Eo::TwoPi;
  if (fabs(sweepAngle) < DBL_EPSILON) sweepAngle = Eo::TwoPi;  // identical => full circle
  m_sweepAngle = sweepAngle;

  // Build major axis vector from center to start point
  EoGePoint3d startPoint(m_center.x + radius * cos(normalizedStartAngle), m_center.y + radius * sin(normalizedStartAngle), m_center.z);
  m_majorAxis = EoGeVector3d(m_center, startPoint);

  // Minor axis is major rotated +90 degrees about Z (plane normal)
  m_minorAxis = m_majorAxis;
  m_minorAxis.RotAboutArbAx(EoGeVector3d::kZAxis, Eo::HalfPi);
}

EoDbEllipse::EoDbEllipse(EoInt16 color, EoInt16 lineType, EoGePoint3d& centerPoint, EoGeVector3d& majorAxis, EoGeVector3d& minorAxis, double sweepAngle)
    : EoDbPrimitive(color, lineType), m_center(centerPoint), m_majorAxis(majorAxis), m_minorAxis(minorAxis) {
  m_sweepAngle = sweepAngle;
}

EoDbEllipse::EoDbEllipse(const EoDbEllipse& other) {
  m_color = other.m_color;
  m_lineTypeIndex = other.m_lineTypeIndex;
  m_center = other.m_center;
  m_majorAxis = other.m_majorAxis;
  m_minorAxis = other.m_minorAxis;
  m_sweepAngle = other.m_sweepAngle;
}

const EoDbEllipse& EoDbEllipse::operator=(const EoDbEllipse& other) {
  m_color = other.m_color;
  m_lineTypeIndex = other.m_lineTypeIndex;
  m_center = other.m_center;
  m_majorAxis = other.m_majorAxis;
  m_minorAxis = other.m_minorAxis;
  m_sweepAngle = other.m_sweepAngle;
  return (*this);
}

void EoDbEllipse::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Arc>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbEllipse::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbEllipse(*this);
  return (primitive);
}

void EoDbEllipse::CutAt2Pts(EoGePoint3d* pt, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  EoDbEllipse* pArc;

  double dRel[2]{};

  dRel[0] = SweepAngleToPoint(pt[0]) / m_sweepAngle;
  dRel[1] = SweepAngleToPoint(pt[1]) / m_sweepAngle;

  if (dRel[0] <= DBL_EPSILON && dRel[1] >= 1. - DBL_EPSILON) {  // Put entire arc in trap
    pArc = this;
  } else {  // Something gets cut
    EoGeVector3d vPlnNorm = EoGeCrossProduct(m_majorAxis, m_minorAxis);
    vPlnNorm.Normalize();

    if (fabs(m_sweepAngle - Eo::TwoPi) <= DBL_EPSILON) {  // Closed arc
      m_sweepAngle = (dRel[1] - dRel[0]) * Eo::TwoPi;

      m_majorAxis.RotAboutArbAx(vPlnNorm, dRel[0] * Eo::TwoPi);
      m_minorAxis.RotAboutArbAx(vPlnNorm, dRel[0] * Eo::TwoPi);

      pArc = new EoDbEllipse(*this);

      m_majorAxis.RotAboutArbAx(vPlnNorm, m_sweepAngle);
      m_minorAxis.RotAboutArbAx(vPlnNorm, m_sweepAngle);

      m_sweepAngle = Eo::TwoPi - m_sweepAngle;
    } else {  // Arc section with a cut
      pArc = new EoDbEllipse(*this);
      double dSwpAng = m_sweepAngle;

      double dAng1 = dRel[0] * m_sweepAngle;
      double dAng2 = dRel[1] * m_sweepAngle;

      if (dRel[0] > DBL_EPSILON && dRel[1] < 1. - DBL_EPSILON) {  // Cut section out of middle
        pArc->SetSweepAngle(dAng1);
        groups->AddTail(new EoDbGroup(pArc));

        m_majorAxis.RotAboutArbAx(vPlnNorm, dAng1);
        m_minorAxis.RotAboutArbAx(vPlnNorm, dAng1);
        m_sweepAngle = dAng2 - dAng1;

        pArc = new EoDbEllipse(*this);

        m_majorAxis.RotAboutArbAx(vPlnNorm, m_sweepAngle);
        m_minorAxis.RotAboutArbAx(vPlnNorm, m_sweepAngle);
        m_sweepAngle = dSwpAng - dAng2;
      } else if (dRel[1] < 1. - DBL_EPSILON) {  // Cut section in two and place begin section in trap
        pArc->SetSweepAngle(dAng2);

        m_majorAxis.RotAboutArbAx(vPlnNorm, dAng2);
        m_minorAxis.RotAboutArbAx(vPlnNorm, dAng2);
        m_sweepAngle = dSwpAng - dAng2;
      } else {  // Cut section in two and place end section in trap
        m_sweepAngle = dAng1;

        EoGeVector3d v = m_majorAxis;
        v.RotAboutArbAx(vPlnNorm, dAng1);
        pArc->SetMajorAxis(v);
        v = m_minorAxis;
        v.RotAboutArbAx(vPlnNorm, dAng1);
        pArc->SetMinorAxis(v);
        pArc->SetSweepAngle(dSwpAng - dAng1);
      }
    }
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(pArc));
}

void EoDbEllipse::CutAtPt(EoGePoint3d& pt, EoDbGroup* group) {
  if (fabs(m_sweepAngle - Eo::TwoPi) <= DBL_EPSILON)
    // Do not fragment a circle
    return;

  double dRel = SweepAngleToPoint(pt) / m_sweepAngle;

  if (dRel <= DBL_EPSILON || dRel >= 1. - DBL_EPSILON)
    // Nothing to cut
    return;

  double dSwpAng = m_sweepAngle * dRel;

  EoDbEllipse* pArc = new EoDbEllipse(*this);
  pArc->SetSweepAngle(dSwpAng);
  group->AddTail(pArc);

  EoGeVector3d vPlnNorm = EoGeCrossProduct(m_majorAxis, m_minorAxis);
  vPlnNorm.Normalize();

  m_majorAxis.RotAboutArbAx(vPlnNorm, dSwpAng);
  m_minorAxis.RotAboutArbAx(vPlnNorm, dSwpAng);
  m_sweepAngle -= dSwpAng;
}

void EoDbEllipse::Display(AeSysView* view, CDC* deviceContext) {
  if (fabs(m_sweepAngle) <= DBL_EPSILON) { return; }

  auto color = LogicalColor();
  auto lineType = LogicalLineType();

  pstate.SetPen(view, deviceContext, color, lineType);

  polyline::BeginLineStrip();
  GenPts(m_center, m_majorAxis, m_minorAxis, m_sweepAngle);
  polyline::__End(view, deviceContext, lineType);
}

void EoDbEllipse::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str.Format(L"<Ellipse> Color: %s Line Type: %s SweepAngle %f MajorAxisLength: %f", FormatPenColor().GetString(), FormatLineType().GetString(), m_sweepAngle,
             m_majorAxis.Length());
  app.AddStringToMessageList(str);
}

void EoDbEllipse::GenPts(EoGePoint3d centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, double sweepAngle) const {
  // Number of points based on angle and a smothness coefficient

  double maxAxisLength = std::max(majorAxis.Length(), minorAxis.Length());
  int numberOfPoints = std::max(2, abs(Eo::Round(sweepAngle / Eo::TwoPi * 32.0)));
  numberOfPoints = std::min(128, std::max(numberOfPoints, abs(Eo::Round(sweepAngle * maxAxisLength / 0.250))));

  EoGeTransformMatrix tm(centerPoint, majorAxis, minorAxis);
  tm.Inverse();

  double segmentAngle = sweepAngle / (numberOfPoints - 1);
  double angleCosine = cos(segmentAngle);
  double angleSine = sin(segmentAngle);

  EoGePoint3d point(1.0, 0.0, 0.0);

  for (int i = 0; i < numberOfPoints; i++) {
    polyline::SetVertex(tm * point);
    point(point.x * angleCosine - point.y * angleSine, point.y * angleCosine + point.x * angleSine, 0.0);
  }
}

void EoDbEllipse::FormatGeometry(CString& str) {
  str += L"Center Point;" + m_center.ToString();
  str += L"Major Axis;" + m_majorAxis.ToString();
  str += L"Minor Axis;" + m_minorAxis.ToString();
  str += L"Plane Normal;" + (EoGeCrossProduct(m_majorAxis, m_minorAxis)).ToString();
}

void EoDbEllipse::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tSweep Angle;%f\tMajor Axis Length;%f", FormatPenColor().GetString(), FormatLineType().GetString(), m_sweepAngle,
             m_majorAxis.Length());
}

EoGePoint3d EoDbEllipse::GetBegPt() { return (m_center + m_majorAxis); }

EoGePoint3d EoDbEllipse::GetEndPt() {
  EoGeTransformMatrix tm(m_center, m_majorAxis, m_minorAxis);
  tm.Inverse();

  EoGePoint3d pt(cos(m_sweepAngle), sin(m_sweepAngle), 0.0);

  pt = tm * pt;
  return (pt);
}

void EoDbEllipse::GetXYExtents(EoGePoint3d arBeg, EoGePoint3d arEnd, EoGePoint3d* arMin, EoGePoint3d* arMax) const {
  double dx = double(m_center.x - arBeg.x);
  double dy = double(m_center.y - arBeg.y);

  double dRad = sqrt(dx * dx + dy * dy);

  (*arMin).x = m_center.x - dRad;
  (*arMin).y = m_center.y - dRad;
  (*arMax).x = m_center.x + dRad;
  (*arMax).y = m_center.y + dRad;

  if (arBeg.x >= m_center.x) {
    if (arBeg.y >= m_center.y) {  // Arc begins in quadrant one
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant one
          if (arBeg.x > arEnd.x) {         // Arc in qraudrant one only
            (*arMin).x = arEnd.x;
            (*arMin).y = arBeg.y;
            (*arMax).x = arBeg.x;
            (*arMax).y = arEnd.y;
          }
        } else  // Arc ends in quadrant four
          (*arMax).x = std::max(arBeg.x, arEnd.x);
      } else {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant two
          (*arMin).x = arEnd.x;
          (*arMin).y = std::min(arBeg.y, arEnd.y);
        } else  // Arc ends in quadrant three
          (*arMin).y = arEnd.y;
        (*arMax).x = arBeg.x;
      }
    } else {  // Arc begins in quadrant four
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant one
          (*arMin).x = std::min(arBeg.x, arEnd.x);
          (*arMin).y = arBeg.y;
          (*arMax).y = arEnd.y;
        } else {                    // Arc ends in quadrant four
          if (arBeg.x < arEnd.x) {  // Arc in qraudrant one only
            (*arMin).x = arBeg.x;
            (*arMin).y = arBeg.y;
            (*arMax).x = arEnd.x;
            (*arMax).y = arEnd.y;
          }
        }
      } else {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant two
          (*arMin).x = arEnd.x;
          (*arMin).y = arBeg.y;
        } else  // Arc ends in quadrant three
          (*arMin).y = std::min(arBeg.y, arEnd.y);
      }
    }
  } else {
    if (arBeg.y >= m_center.y) {  // Arc begins in quadrant two
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y)  // Arc ends in quadrant one
          (*arMax).y = std::max(arBeg.y, arEnd.y);
        else {  // Arc ends in quadrant four
          (*arMax).x = arEnd.x;
          (*arMax).y = arBeg.y;
        }
      } else {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant two
          if (arBeg.x > arEnd.x) {         // Arc in qraudrant two only
            (*arMin).x = arEnd.x;
            (*arMin).y = arEnd.y;
            (*arMax).x = arBeg.x;
            (*arMax).y = arBeg.y;
          }
        } else {  // Arc ends in quadrant three
          (*arMin).y = arEnd.y;
          (*arMax).x = std::max(arBeg.x, arEnd.x);
          (*arMax).y = arBeg.y;
        }
      }
    } else {  // Arc begins in quadrant three
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y)  // Arc ends in quadrant one
          (*arMax).y = arEnd.y;
        else {  // Arc ends in quadrant four
          (*arMax).x = arEnd.x;
          (*arMax).y = std::max(arBeg.y, arEnd.y);
        }
        (*arMin).x = arBeg.x;
      } else {
        if (arEnd.y >= m_center.y)  // Arc ends in quadrant two
          (*arMin).x = std::min(arBeg.x, arEnd.x);
        else {                      // Arc ends in quadrant three
          if (arBeg.x < arEnd.x) {  // Arc in qraudrant three only
            (*arMin).x = arBeg.x;
            (*arMin).y = arEnd.y;
            (*arMax).x = arEnd.x;
            (*arMax).y = arBeg.y;
          }
        }
      }
    }
  }
}

void EoDbEllipse::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3dArray ptsRegion;
  GetBoundingBox(ptsRegion);

  EoGePoint3d pt;

  for (EoUInt16 w = 0; w < 4; w++) {
    pt = ptsRegion[w];
    view->ModelTransformPoint(pt);
    pt = tm * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}

bool EoDbEllipse::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  // Determines if a point is on a control point of the arc.

  EoGePoint4d pt[] = {EoGePoint4d(GetBegPt()), EoGePoint4d(GetEndPt())};

  for (EoUInt16 w = 0; w < 2; w++) {
    view->ModelViewTransformPoint(pt[w]);

    if (point.DistanceToPointXY(pt[w]) < sm_SelectApertureSize) return true;
  }
  return false;
}

int EoDbEllipse::IsWithinArea(EoGePoint3d ptLL, EoGePoint3d ptUR, EoGePoint3d* ptInt) {
  EoGeVector3d vPlnNorm = EoGeCrossProduct(m_majorAxis, m_minorAxis);
  vPlnNorm.Normalize();

  if (!(EoGeCrossProduct(EoGeVector3d::kZAxis, vPlnNorm)).IsNearNull())
    // not on plane normal to z-axis
    return 0;

  if (fabs(m_majorAxis.Length() - m_minorAxis.Length()) > FLT_EPSILON)
    // not radial
    return 0;

  EoGePoint3d ptMin, ptMax;

  EoGePoint3d ptBeg = GetBegPt();
  EoGePoint3d ptEnd = GetEndPt();

  if (vPlnNorm.z < 0.0) {
    EoGePoint3d pt = ptBeg;
    ptBeg = ptEnd;
    ptEnd = pt;

    vPlnNorm = -vPlnNorm;
    m_majorAxis = EoGeVector3d(m_center, ptBeg);
    m_minorAxis = EoGeCrossProduct(vPlnNorm, m_majorAxis);
  }

  GetXYExtents(ptBeg, ptEnd, &ptMin, &ptMax);

  if (ptMin.x >= ptLL.x && ptMax.x <= ptUR.x && ptMin.y >= ptLL.y && ptMax.y <= ptUR.y) {  // Totally within window boundaries
    ptInt[0] = ptBeg;
    ptInt[1] = ptEnd;
    return (2);
  }
  if (ptMin.x >= ptUR.x || ptMax.x <= ptLL.x || ptMin.y >= ptUR.y || ptMax.y <= ptLL.y)
    // No extent overlap
    return 0;

  EoGePoint3d ptWrk[8]{};

  double dDis;
  double dOff;
  int iSecs = 0;

  double dRad = EoGeVector3d(m_center, ptBeg).Length();
  if (ptMax.x > ptUR.x) {  // Arc may intersect with right window boundary
    dDis = ptUR.x - m_center.x;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_center.y - dOff >= ptLL.y && m_center.y - dOff <= ptUR.y) {
      ptWrk[iSecs].x = ptUR.x;
      ptWrk[iSecs++].y = m_center.y - dOff;
    }
    if (m_center.y + dOff <= ptUR.y && m_center.y + dOff >= ptLL.y) {
      ptWrk[iSecs].x = ptUR.x;
      ptWrk[iSecs++].y = m_center.y + dOff;
    }
  }
  if (ptMax.y > ptUR.y) {  // Arc may intersect with top window boundary
    dDis = ptUR.y - m_center.y;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_center.x + dOff <= ptUR.x && m_center.x + dOff >= ptLL.x) {
      ptWrk[iSecs].x = m_center.x + dOff;
      ptWrk[iSecs++].y = ptUR.y;
    }
    if (m_center.x - dOff >= ptLL.x && m_center.x - dOff <= ptUR.x) {
      ptWrk[iSecs].x = m_center.x - dOff;
      ptWrk[iSecs++].y = ptUR.y;
    }
  }
  if (ptMin.x < ptLL.x) {  // Arc may intersect with left window boundary
    dDis = m_center.x - ptLL.x;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_center.y + dOff <= ptUR.y && m_center.y + dOff >= ptLL.y) {
      ptWrk[iSecs].x = ptLL.x;
      ptWrk[iSecs++].y = m_center.y + dOff;
    }
    if (m_center.y - dOff >= ptLL.y && m_center.y - dOff <= ptUR.y) {
      ptWrk[iSecs].x = ptLL.x;
      ptWrk[iSecs++].y = m_center.y - dOff;
    }
  }
  if (ptMin.y < ptLL.y) {  // Arc may intersect with bottom window boundary
    dDis = m_center.y - ptLL.y;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_center.x - dOff >= ptLL.x && m_center.x - dOff <= ptUR.x) {
      ptWrk[iSecs].x = m_center.x - dOff;
      ptWrk[iSecs++].y = ptLL.y;
    }
    if (m_center.x + dOff <= ptUR.x && m_center.x + dOff >= ptLL.x) {
      ptWrk[iSecs].x = m_center.x + dOff;
      ptWrk[iSecs++].y = ptLL.y;
    }
  }
  if (iSecs == 0) return 0;

  double dBegAng = atan2(ptBeg.y - m_center.y, ptBeg.x - m_center.x);  // Arc begin angle (- pi to pi)

  double dIntAng[8]{};
  double dWrkAng;
  int iInts = 0;
  for (int i2 = 0; i2 < iSecs; i2++) {                                              // Loop thru possible intersections
    dWrkAng = atan2(ptWrk[i2].y - m_center.y, ptWrk[i2].x - m_center.x);  // Current intersection angle (- pi to
    dIntAng[iInts] = dWrkAng - dBegAng;                                             // Sweep from begin to intersection
    if (dIntAng[iInts] < 0.0) dIntAng[iInts] += Eo::TwoPi;
    if (fabs(dIntAng[iInts]) - m_sweepAngle < 0.0) {  // Intersection lies on arc
      int i;
      for (i = 0; i < iInts && ptWrk[i2] != ptInt[i]; i++);
      if (i == iInts)  // Unique intersection
        ptInt[iInts++] = ptWrk[i2];
    }
  }
  if (iInts == 0)
    // None of the intersections are on sweep of arc
    return 0;

  for (int i1 = 0; i1 < iInts; i1++) {  // Sort intersections from begin to end of sweep
    for (int i2 = 1; i2 < iInts - i1; i2++) {
      if (fabs(dIntAng[i2]) < fabs(dIntAng[i2 - 1])) {
        double dAng = dIntAng[i2 - 1];
        dIntAng[i2 - 1] = dIntAng[i2];
        dIntAng[i2] = dAng;
        EoGePoint3d pt = ptInt[i2 - 1];
        ptInt[i2 - 1] = ptInt[i2];
        ptInt[i2] = pt;
      }
    }
  }
  if (fabs(m_sweepAngle - Eo::TwoPi) <= DBL_EPSILON) {  // Arc is a circle in disuise

  } else {
    if (ptBeg.x >= ptLL.x && ptBeg.x <= ptUR.x && ptBeg.y >= ptLL.y && ptBeg.y <= ptUR.y) {  // Add beg point to int set
      for (int i = iInts; i > 0; i--) ptInt[i] = ptInt[i - 1];
      ptInt[0] = ptBeg;
      iInts++;
    }
    if (ptEnd.x >= ptLL.x && ptEnd.x <= ptUR.x && ptEnd.y >= ptLL.y && ptEnd.y <= ptUR.y) {  // Add end point to int set
      ptInt[iInts] = ptEnd;
      iInts++;
    }
  }
  return (iInts);
}

EoGePoint3d EoDbEllipse::GoToNxtCtrlPt() {
  double dAng = (sm_RelationshipOfPoint <= DBL_EPSILON) ? m_sweepAngle : 0.;
  return (pFndPtOnArc(m_center, m_majorAxis, m_minorAxis, dAng));
}

bool EoDbEllipse::IsInView(AeSysView* view) {
  EoGePoint3dArray pts;

  GetBoundingBox(pts);

  EoGePoint4d ptBeg(pts[0]);
  view->ModelViewTransformPoint(ptBeg);

  for (EoUInt16 w = 1; w < 4; w++) {
    EoGePoint4d ptEnd(pts[w]);
    view->ModelViewTransformPoint(ptEnd);

    if (EoGePoint4d::ClipLine(ptBeg, ptEnd)) return true;

    ptBeg = ptEnd;
  }
  return false;
}

EoGePoint3d EoDbEllipse::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;

  double dAPert = sm_SelectApertureSize;

  EoGePoint3d ptCtrl[] = {GetBegPt(), GetEndPt()};

  for (EoUInt16 w = 0; w < 2; w++) {
    EoGePoint4d pt(ptCtrl[w]);

    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dAPert) {
      sm_ControlPointIndex = w;
      dAPert = dDis;
    }
  }
  return (sm_ControlPointIndex == USHRT_MAX) ? EoGePoint3d::kOrigin : ptCtrl[sm_ControlPointIndex];
}

bool EoDbEllipse::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt) {
  polyline::BeginLineStrip();
  GenPts(m_center, m_majorAxis, m_minorAxis, m_sweepAngle);
  return polyline::SelectUsingLine(view, line, ptsInt);
}

bool EoDbEllipse::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  polyline::BeginLineStrip();
  GenPts(m_center, m_majorAxis, m_minorAxis, m_sweepAngle);
  return (polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, ptProj));
}

bool EoDbEllipse::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  polyline::BeginLineStrip();
  GenPts(m_center, m_majorAxis, m_minorAxis, m_sweepAngle);
  return polyline::SelectUsingRectangle(view, pt1, pt2);
}

void EoDbEllipse::Transform(EoGeTransformMatrix& tm) {
  m_center = tm * m_center;
  m_majorAxis = tm * m_majorAxis;
  m_minorAxis = tm * m_minorAxis;
}

void EoDbEllipse::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) m_center += v;
}

bool EoDbEllipse::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kEllipsePrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_center.Write(file);
  m_majorAxis.Write(file);
  m_minorAxis.Write(file);
  EoDb::Write(file, m_sweepAngle);

  return true;
}


void EoDbEllipse::GetBoundingBox(EoGePoint3dArray& ptsBox) {
  ptsBox.SetSize(4);
  ptsBox[0] = EoGePoint3d(-1.0, -1.0, 0.0);
  ptsBox[1] = EoGePoint3d(1.0, -1.0, 0.0);
  ptsBox[2] = EoGePoint3d(1.0, 1.0, 0.0);
  ptsBox[3] = EoGePoint3d(-1.0, 1.0, 0.0);

  if (m_sweepAngle < 3. * Eo::TwoPi / 4.0) {
    double dEndX = cos(m_sweepAngle);
    double dEndY = sin(m_sweepAngle);

    if (dEndX >= 0.0) {
      if (dEndY >= 0.0) {  // Arc ends in quadrant one
        ptsBox[0].x = dEndX;
        ptsBox[0].y = 0.;
        ptsBox[1].y = 0.;
        ptsBox[2].y = dEndY;
        ptsBox[3].x = dEndX;
        ptsBox[3].y = dEndY;
      }
    } else {
      if (dEndY >= 0.0) {  // Arc ends in quadrant two
        ptsBox[0].x = dEndX;
        ptsBox[0].y = 0.;
        ptsBox[1].y = 0.;
        ptsBox[3].x = dEndX;
      } else {  // Arc ends in quadrant three
        ptsBox[0].y = dEndY;
        ptsBox[1].y = dEndY;
      }
    }
  }
  EoGeTransformMatrix tm(m_center, m_majorAxis, m_minorAxis);
  tm.Inverse();

  for (EoUInt16 w = 0; w < 4; w++) { ptsBox[w] = tm * ptsBox[w]; }
}

double EoDbEllipse::SweepAngleToPoint(EoGePoint3d point) {
  EoGeVector3d planeNormal = EoGeCrossProduct(m_majorAxis, m_minorAxis);
  planeNormal.Normalize();

  EoGeTransformMatrix transformMatrix(m_center, planeNormal);

  EoGePoint3d beginPoint = m_center + m_majorAxis;
  EoGePoint3d endPoint = point;

  // Translate points into z=0 plane
  beginPoint = transformMatrix * beginPoint;
  endPoint = transformMatrix * endPoint;

  return (EoGeLine::AngleBetweenLn_xy(EoGeLine(EoGePoint3d::kOrigin, beginPoint), EoGeLine(EoGePoint3d::kOrigin, endPoint)));
}

/** @brief Given a plane normal and three points (two outside and one inside), find the sweep angle defined by the three points about the center point.
 * @param planeNormal Normal vector of the plane containing the points
 * @param arP1 First outside point
 * @param arP2 Inside point
 * @param arP3 Second outside point
 * @param centerPoint Center point about which to measure the sweep angle
 * @param adTheta Sweep angle result
 * @return TRUE if successful, FALSE if not.
*/
int SweepAngleFromNormalAnd3Points(EoGeVector3d planeNormal, EoGePoint3d arP1, EoGePoint3d arP2, EoGePoint3d arP3, EoGePoint3d& centerPoint, double* adTheta) {
  double dT[3]{};
  EoGePoint3d rR[3]{};

  if (arP1 == centerPoint || arP2 == centerPoint || arP3 == centerPoint) { return (FALSE); }

  // None of the points coincide with center point
  EoGeTransformMatrix tm(centerPoint, planeNormal);
  rR[0] = arP1;
  rR[1] = arP2;
  rR[2] = arP3;
  for (int i = 0; i < 3; i++) {  // Translate points into z=0 plane with center point at origin
    rR[i] = tm * rR[i];
    dT[i] = atan2(rR[i].y, rR[i].x);
    if (dT[i] < 0.0) dT[i] += Eo::TwoPi;
  }
  double dTMin = std::min(dT[0], dT[2]);
  double dTMax = std::max(dT[0], dT[2]);
  if (fabs(dT[1] - dTMax) > DBL_EPSILON && fabs(dT[1] - dTMin) > DBL_EPSILON) {  // Inside line is not colinear with outside lines
    double dTheta = dTMax - dTMin;
    if (dT[1] > dTMin && dT[1] < dTMax) {
      if (dT[0] == dTMax) dTheta = -dTheta;
    } else {
      dTheta = Eo::TwoPi - dTheta;
      if (dT[2] == dTMax) dTheta = -dTheta;
    }
    *adTheta = dTheta;

    return (TRUE);
  }
  return (FALSE);
}

// The arc's end point and intermediate point (for a 3-point representation) can be computed as:

//EoGePoint3d endPoint(m_centerPoint.x + radius * cos(normalizedStartAngle + m_sweepAngle), m_centerPoint.y + radius * sin(normalizedStartAngle + m_sweepAngle),
//                     m_centerPoint.z);
//EoGePoint3d middlePoint(m_centerPoint.x + radius * cos(normalizedStartAngle + m_sweepAngle * 0.5),
//                        m_centerPoint.y + radius * sin(normalizedStartAngle + m_sweepAngle * 0.5), m_centerPoint.z);
