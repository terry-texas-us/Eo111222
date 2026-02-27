#include "Stdafx.h"

#include <algorithm>
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
#include "EoGsRenderState.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif  // USING_DDE

namespace {

/** @brief Computes a point on an ellipse arc at a specified angle.
 *
 * This function calculates the coordinates of a point on an ellipse defined by its center, major axis, and minor axis,
 * at a given angle in radians. The angle is measured from the major axis.
 *
 * @param center The center point of the ellipse.
 * @param majorAxis The major axis vector of the ellipse.
 * @param minorAxis The minor axis vector of the ellipse.
 * @param angle The angle in radians at which to compute the point on the ellipse.
 * @return The computed point on the ellipse at the specified angle.
 */
EoGePoint3d PointOnArcAtAngle(EoGePoint3d center, EoGeVector3d majorAxis, EoGeVector3d minorAxis, double angle) {
  EoGeTransformMatrix transformMatrix(center, majorAxis, minorAxis);
  transformMatrix.Inverse();

  EoGePoint3d point(std::cos(angle), std::sin(angle), 0.0);

  point = transformMatrix * point;
  return point;
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
  if (angle >= Eo::TwoPi - Eo::geometricTolerance) { angle = 0.0; }

  return angle;
}

/**
 * @brief Constructs an ellipse segment defined by a center point, major and minor axes, and a sweep angle.
 *
 * This constructor initializes an ellipse segment using the specified center point, major axis, minor axis, and sweep angle.
 * The pen color and line type are set based on the current primitive state.
 *
 * @param center The center point of the ellipse.
 * @param majorAxis The major axis vector of the ellipse.
 * @param minorAxis The minor axis vector of the ellipse.
 * @param sweepAngle The sweep angle of the ellipse segment in radians.
 */
EoDbEllipse::EoDbEllipse(
    const EoGePoint3d& center, const EoGeVector3d& majorAxis, const EoGeVector3d& minorAxis, double sweepAngle)
    : EoDbPrimitive(renderState.Color(), renderState.LineTypeIndex()),
      m_center(center),
      m_majorAxis(majorAxis),
      m_minorAxis(minorAxis),
      m_sweepAngle(sweepAngle) {}

/**
 * @brief Constructs a circle (as ellipse) primitive defined by a center point and radius in the current view.
 *
 * @param color The color.
 * @param lineType The line type index.
 * @param center The center point of the circle.
 * @param radius The radius of the circle.
 */
EoDbEllipse::EoDbEllipse(EoGePoint3d& center, double radius, std::int16_t color, std::int16_t lineType)
    : EoDbPrimitive(color, lineType), m_center(center) {
  auto* activeView = AeSysView::GetActiveView();

  auto normal = activeView->CameraDirection();

  m_minorAxis = activeView->ViewUp() * radius;
  m_majorAxis = m_minorAxis;
  m_majorAxis.RotateAboutArbitraryAxis(normal, -Eo::HalfPi);
  m_sweepAngle = Eo::TwoPi;
}

/**
 * @brief Constructs a circle (as ellipse) primitive defined by a center point, plane normal, and radius.
 *
 * This constructor initializes a circle primitive using the specified center point, plane normal, and radius.
 * The major and minor axes are calculated based on the provided normal vector.
 * The pen color and line type are set based on the provided parameters.
 *
 * @param color The pen color for the ellipse.
 * @param lineType The line type index for the ellipse.
 * @param center The center point of the circle.
 * @param normal The normal vector defining the plane of the circle.
 * @param radius The radius of the circle.
 */
EoDbEllipse::EoDbEllipse(
    EoGePoint3d& center, EoGeVector3d& normal, double radius, std::int16_t color, std::int16_t lineType)
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

  m_color = renderState.Color();
  m_lineTypeIndex = renderState.LineTypeIndex();

  auto cameraDirection = activeView->CameraDirection();

  m_center = center;
  m_majorAxis = EoGeVector3d(center, start);
  m_minorAxis = m_majorAxis;
  m_minorAxis.RotateAboutArbitraryAxis(-cameraDirection, Eo::HalfPi);
  m_sweepAngle = Eo::TwoPi;
}

/*EoDbEllipse::EoDbEllipse(EoGePoint3d& center, EoGeVector3d& extrusion, double radius)
    : EoDbPrimitive(), m_center(center) {

  m_minorAxis = ComputeArbitraryAxis(extrusion) * radius;
  m_majorAxis = m_minorAxis;
  m_majorAxis.RotateAboutArbitraryAxis(extrusion, Eo::HalfPi);
  m_sweepAngle = Eo::TwoPi;
}
*/
/** 
 * @brief Constructs a radial arc (as ellipse) primitive from three points that define an elliptical arc.
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
  m_color = renderState.Color();
  m_lineTypeIndex = renderState.LineTypeIndex();

  m_sweepAngle = 0.0;

  EoGeVector3d startToIntermediate(start, intermediate);
  EoGeVector3d startToEnd(start, end);
  auto normal = CrossProduct(startToIntermediate, startToEnd);
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

  if (std::abs(determinant) > Eo::geometricTolerance) {  // Three points are not colinear
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

    if (std::abs(dAng[1] - dMax) > Eo::geometricTolerance &&
        std::abs(dAng[1] - dMin) > Eo::geometricTolerance) {  // Inside line is not colinear with outside lines
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
 * @param center The center point of the ellipse.
 * @param radius The radius of the ellipse.
 * @param startAngle The starting angle of the arc in radians.
 * @param endAngle The ending angle of the arc in radians.
 * @note The sweep angle is computed to ensure a counter-clockwise direction.
 */
/*
EoDbEllipse::EoDbEllipse(const EoGePoint3d& center, double radius, double startAngle, double endAngle) {
  m_center = center;

  double normalizedStartAngle = NormalizeTo2Pi(startAngle);
  double normalizedEndAngle = NormalizeTo2Pi(endAngle);

  // Compute CCW sweep from start to end in range (0, 2*pi] (canonical representation: positive = CCW sweep)
  double sweepAngle = normalizedEndAngle - normalizedStartAngle;
  if (sweepAngle < 0.0) sweepAngle += Eo::TwoPi;
  if (std::abs(sweepAngle) < Eo::geometricTolerance) sweepAngle = Eo::TwoPi;  // identical => full circle
  m_sweepAngle = sweepAngle;

  // Build major axis vector from center to start point
  EoGePoint3d startPoint(m_center.x + radius * std::cos(normalizedStartAngle), m_center.y + radius * std::sin(normalizedStartAngle), m_center.z);
  m_majorAxis = EoGeVector3d(m_center, startPoint);

  // Minor axis is major rotated +90 degrees about Z (plane normal)
  m_minorAxis = m_majorAxis;
  m_minorAxis.RotateAboutArbitraryAxis(EoGeVector3d::positiveUnitZ, Eo::HalfPi);
}
*/
/** 
 * @brief Constructs an ellipse segment defined by a center point, major and minor axes, and a sweep angle.
 *
 * This constructor initializes an ellipse segment using the specified pen color, line type, center point,
 * major axis, minor axis, and sweep angle.
 *
 * @param color The pen color for the ellipse.
 * @param lineType The line type index for the ellipse.
 * @param center The center point of the ellipse.
 * @param majorAxis The major axis vector of the ellipse.
 * @param minorAxis The minor axis vector of the ellipse.
 * @param sweepAngle The sweep angle of the ellipse segment in radians.
 */
EoDbEllipse::EoDbEllipse(EoGePoint3d& center, EoGeVector3d& majorAxis, EoGeVector3d& minorAxis, double sweepAngle,
    std::int16_t color, std::int16_t lineType)
    : EoDbPrimitive(color, lineType), m_center(center), m_majorAxis(majorAxis), m_minorAxis(minorAxis) {
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
  return primitive;
}

void EoDbEllipse::CutAt2Points(
    const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  EoDbEllipse* pArc;

  double dRel[2]{};

  dRel[0] = SweepAngleToPoint(firstPoint) / m_sweepAngle;
  dRel[1] = SweepAngleToPoint(secondPoint) / m_sweepAngle;

  if (dRel[0] < Eo::geometricTolerance && dRel[1] >= 1.0 - Eo::geometricTolerance) {  // Put entire arc in trap
    pArc = this;
  } else {  // Something gets cut
    auto vPlnNorm = CrossProduct(m_majorAxis, m_minorAxis);
    vPlnNorm.Normalize();

    if (std::abs(m_sweepAngle - Eo::TwoPi) < Eo::geometricTolerance) {  // Closed arc
      m_sweepAngle = (dRel[1] - dRel[0]) * Eo::TwoPi;

      m_majorAxis.RotateAboutArbitraryAxis(vPlnNorm, dRel[0] * Eo::TwoPi);
      m_minorAxis.RotateAboutArbitraryAxis(vPlnNorm, dRel[0] * Eo::TwoPi);

      pArc = new EoDbEllipse(*this);

      m_majorAxis.RotateAboutArbitraryAxis(vPlnNorm, m_sweepAngle);
      m_minorAxis.RotateAboutArbitraryAxis(vPlnNorm, m_sweepAngle);

      m_sweepAngle = Eo::TwoPi - m_sweepAngle;
    } else {  // Arc section with a cut
      pArc = new EoDbEllipse(*this);
      double dSwpAng = m_sweepAngle;

      double dAng1 = dRel[0] * m_sweepAngle;
      double dAng2 = dRel[1] * m_sweepAngle;

      if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
        pArc->SetSweepAngle(dAng1);
        groups->AddTail(new EoDbGroup(pArc));

        m_majorAxis.RotateAboutArbitraryAxis(vPlnNorm, dAng1);
        m_minorAxis.RotateAboutArbitraryAxis(vPlnNorm, dAng1);
        m_sweepAngle = dAng2 - dAng1;

        pArc = new EoDbEllipse(*this);

        m_majorAxis.RotateAboutArbitraryAxis(vPlnNorm, m_sweepAngle);
        m_minorAxis.RotateAboutArbitraryAxis(vPlnNorm, m_sweepAngle);
        m_sweepAngle = dSwpAng - dAng2;
      } else if (dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section in two and place begin section in trap
        pArc->SetSweepAngle(dAng2);

        m_majorAxis.RotateAboutArbitraryAxis(vPlnNorm, dAng2);
        m_minorAxis.RotateAboutArbitraryAxis(vPlnNorm, dAng2);
        m_sweepAngle = dSwpAng - dAng2;
      } else {  // Cut section in two and place end section in trap
        m_sweepAngle = dAng1;

        EoGeVector3d v = m_majorAxis;
        v.RotateAboutArbitraryAxis(vPlnNorm, dAng1);
        pArc->SetMajorAxis(v);
        v = m_minorAxis;
        v.RotateAboutArbitraryAxis(vPlnNorm, dAng1);
        pArc->SetMinorAxis(v);
        pArc->SetSweepAngle(dSwpAng - dAng1);
      }
    }
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(pArc));
}

void EoDbEllipse::CutAtPoint(const EoGePoint3d& point, EoDbGroup* group) {
  if (std::abs(m_sweepAngle - Eo::TwoPi) < Eo::geometricTolerance)
    // Do not fragment a circle
    return;

  double dRel = SweepAngleToPoint(point) / m_sweepAngle;

  if (dRel < Eo::geometricTolerance || dRel >= 1.0 - Eo::geometricTolerance)
    // Nothing to cut
    return;

  double dSwpAng = m_sweepAngle * dRel;

  EoDbEllipse* pArc = new EoDbEllipse(*this);
  pArc->SetSweepAngle(dSwpAng);
  group->AddTail(pArc);

  auto vPlnNorm = CrossProduct(m_majorAxis, m_minorAxis);
  vPlnNorm.Normalize();

  m_majorAxis.RotateAboutArbitraryAxis(vPlnNorm, dSwpAng);
  m_minorAxis.RotateAboutArbitraryAxis(vPlnNorm, dSwpAng);
  m_sweepAngle -= dSwpAng;
}

void EoDbEllipse::Display(AeSysView* view, CDC* deviceContext) {
  if (std::abs(m_sweepAngle) < Eo::geometricTolerance) { return; }

  auto color = LogicalColor();
  auto lineType = LogicalLineType();

  renderState.SetPen(view, deviceContext, color, lineType);

  polyline::BeginLineStrip();
  GenPts(m_center, m_majorAxis, m_minorAxis, m_sweepAngle);
  polyline::__End(view, deviceContext, lineType);
}

void EoDbEllipse::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_center);
}

void EoDbEllipse::AddReportToMessageList(const EoGePoint3d&) {
  CString str;
  str.Format(L"<Ellipse> Color: %s Line Type: %s SweepAngle %f MajorAxisLength: %f", FormatPenColor().GetString(),
      FormatLineType().GetString(), m_sweepAngle, m_majorAxis.Length());
  app.AddStringToMessageList(str);
}

void EoDbEllipse::GenPts(
    EoGePoint3d centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, double sweepAngle) const {
  // Number of points based on angle and a smothness coefficient

  double maxAxisLength = std::max(majorAxis.Length(), minorAxis.Length());
  int numberOfPoints = std::max(2, abs(Eo::Round(sweepAngle / Eo::TwoPi * 32.0)));
  numberOfPoints = std::min(128, std::max(numberOfPoints, abs(Eo::Round(sweepAngle * maxAxisLength / 0.250))));

  EoGeTransformMatrix transformMatrix(centerPoint, majorAxis, minorAxis);
  transformMatrix.Inverse();

  double segmentAngle = sweepAngle / (numberOfPoints - 1);
  double angleCosine = std::cos(segmentAngle);
  double angleSine = std::sin(segmentAngle);

  EoGePoint3d point(1.0, 0.0, 0.0);

  for (int i = 0; i < numberOfPoints; i++) {
    polyline::SetVertex(transformMatrix * point);
    point.Set(point.x * angleCosine - point.y * angleSine, point.y * angleCosine + point.x * angleSine, 0.0);
  }
}

void EoDbEllipse::FormatGeometry(CString& str) {
  str += L"Center Point;" + m_center.ToString();
  str += L"Major Axis;" + m_majorAxis.ToString();
  str += L"Minor Axis;" + m_minorAxis.ToString();
  str += L"Plane Normal;" + (CrossProduct(m_majorAxis, m_minorAxis)).ToString();
}

void EoDbEllipse::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tSweep Angle;%f\tMajor Axis Length;%f", FormatPenColor().GetString(),
      FormatLineType().GetString(), m_sweepAngle, m_majorAxis.Length());
}

EoGePoint3d EoDbEllipse::PointAtStartAngle() { return (m_center + m_majorAxis); }

EoGePoint3d EoDbEllipse::PointAtEndAngle() {
  EoGeTransformMatrix transformMatrix(m_center, m_majorAxis, m_minorAxis);
  transformMatrix.Inverse();

  EoGePoint3d pt(std::cos(m_sweepAngle), std::sin(m_sweepAngle), 0.0);

  pt = transformMatrix * pt;
  return pt;
}

void EoDbEllipse::GetXYExtents(EoGePoint3d arBeg, EoGePoint3d arEnd, EoGePoint3d* arMin, EoGePoint3d* arMax) const {
  double dx = double(m_center.x - arBeg.x);
  double dy = double(m_center.y - arBeg.y);

  double dRad = std::sqrt(dx * dx + dy * dy);

  arMin->x = m_center.x - dRad;
  arMin->y = m_center.y - dRad;
  arMax->x = m_center.x + dRad;
  arMax->y = m_center.y + dRad;

  if (arBeg.x >= m_center.x) {
    if (arBeg.y >= m_center.y) {  // Arc begins in quadrant one
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant one
          if (arBeg.x > arEnd.x) {    // Arc in qraudrant one only
            arMin->x = arEnd.x;
            arMin->y = arBeg.y;
            arMax->x = arBeg.x;
            arMax->y = arEnd.y;
          }
        } else {  // Arc ends in quadrant four
          arMax->x = std::max(arBeg.x, arEnd.x);
        }
      } else {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant two
          arMin->x = arEnd.x;
          arMin->y = std::min(arBeg.y, arEnd.y);
        } else {  // Arc ends in quadrant three
          arMin->y = arEnd.y;
        }
        arMax->x = arBeg.x;
      }
    } else {  // Arc begins in quadrant four
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant one
          arMin->x = std::min(arBeg.x, arEnd.x);
          arMin->y = arBeg.y;
          arMax->y = arEnd.y;
        } else {                    // Arc ends in quadrant four
          if (arBeg.x < arEnd.x) {  // Arc in qraudrant one only
            arMin->x = arBeg.x;
            arMin->y = arBeg.y;
            arMax->x = arEnd.x;
            arMax->y = arEnd.y;
          }
        }
      } else {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant two
          arMin->x = arEnd.x;
          arMin->y = arBeg.y;
        } else {  // Arc ends in quadrant three
          arMin->y = std::min(arBeg.y, arEnd.y);
        }
      }
    }
  } else {
    if (arBeg.y >= m_center.y) {  // Arc begins in quadrant two
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant one
          arMax->y = std::max(arBeg.y, arEnd.y);
        } else {  // Arc ends in quadrant four
          arMax->x = arEnd.x;
          arMax->y = arBeg.y;
        }
      } else {
        if (arEnd.y >= m_center.y) {  // Arc ends in quadrant two
          if (arBeg.x > arEnd.x) {    // Arc in qraudrant two only
            arMin->x = arEnd.x;
            arMin->y = arEnd.y;
            arMax->x = arBeg.x;
            arMax->y = arBeg.y;
          }
        } else {  // Arc ends in quadrant three
          arMin->y = arEnd.y;
          arMax->x = std::max(arBeg.x, arEnd.x);
          arMax->y = arBeg.y;
        }
      }
    } else {  // Arc begins in quadrant three
      if (arEnd.x >= m_center.x) {
        if (arEnd.y >= m_center.y)  // Arc ends in quadrant one
          arMax->y = arEnd.y;
        else {  // Arc ends in quadrant four
          arMax->x = arEnd.x;
          arMax->y = std::max(arBeg.y, arEnd.y);
        }
        arMin->x = arBeg.x;
      } else {
        if (arEnd.y >= m_center.y)  // Arc ends in quadrant two
          arMin->x = std::min(arBeg.x, arEnd.x);
        else {                      // Arc ends in quadrant three
          if (arBeg.x < arEnd.x) {  // Arc in qraudrant three only
            arMin->x = arBeg.x;
            arMin->y = arEnd.y;
            arMax->x = arEnd.x;
            arMax->y = arBeg.y;
          }
        }
      }
    }
  }
}

void EoDbEllipse::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3dArray ptsRegion;
  GetBoundingBox(ptsRegion);

  EoGePoint3d pt;

  for (auto i = 0; i < 4; i++) {
    pt = ptsRegion[i];
    view->ModelTransformPoint(pt);
    pt = transformMatrix * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}

bool EoDbEllipse::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  // Determines if a point is on a control point of the arc.

  EoGePoint4d pt[] = {EoGePoint4d(PointAtStartAngle()), EoGePoint4d(PointAtEndAngle())};

  for (auto i = 0; i < 2; i++) {
    view->ModelViewTransformPoint(pt[i]);

    if (point.DistanceToPointXY(pt[i]) < sm_SelectApertureSize) { return true; }
  }
  return false;
}

int EoDbEllipse::IsWithinArea(const EoGePoint3d& lowerLeft, const EoGePoint3d& upperRight, EoGePoint3d* ptInt) {
  auto vPlnNorm = CrossProduct(m_majorAxis, m_minorAxis);
  vPlnNorm.Normalize();

  if (!(CrossProduct(EoGeVector3d::positiveUnitZ, vPlnNorm)).IsNearNull())
    // not on plane normal to z-axis
    return 0;

  if (std::abs(m_majorAxis.Length() - m_minorAxis.Length()) > Eo::geometricTolerance)
    // not radial
    return 0;

  EoGePoint3d ptMin, ptMax;

  EoGePoint3d ptBeg = PointAtStartAngle();
  EoGePoint3d ptEnd = PointAtEndAngle();

  if (vPlnNorm.z < 0.0) {
    EoGePoint3d pt = ptBeg;
    ptBeg = ptEnd;
    ptEnd = pt;

    vPlnNorm = -vPlnNorm;
    m_majorAxis = EoGeVector3d(m_center, ptBeg);
    m_minorAxis = CrossProduct(vPlnNorm, m_majorAxis);
  }

  GetXYExtents(ptBeg, ptEnd, &ptMin, &ptMax);

  if (ptMin.x >= lowerLeft.x && ptMax.x <= upperRight.x && ptMin.y >= lowerLeft.y &&
      ptMax.y <= upperRight.y) {  // Totally within window boundaries
    ptInt[0] = ptBeg;
    ptInt[1] = ptEnd;
    return (2);
  }
  if (ptMin.x >= upperRight.x || ptMax.x <= lowerLeft.x || ptMin.y >= upperRight.y || ptMax.y <= lowerLeft.y)
    // No extent overlap
    return 0;

  EoGePoint3d ptWrk[8]{};

  double dDis;
  double dOff;
  int iSecs = 0;

  double dRad = EoGeVector3d(m_center, ptBeg).Length();
  if (ptMax.x > upperRight.x) {  // Arc may intersect with right window boundary
    dDis = upperRight.x - m_center.x;
    dOff = std::sqrt(dRad * dRad - dDis * dDis);
    if (m_center.y - dOff >= lowerLeft.y && m_center.y - dOff <= upperRight.y) {
      ptWrk[iSecs].x = upperRight.x;
      ptWrk[iSecs++].y = m_center.y - dOff;
    }
    if (m_center.y + dOff <= upperRight.y && m_center.y + dOff >= lowerLeft.y) {
      ptWrk[iSecs].x = upperRight.x;
      ptWrk[iSecs++].y = m_center.y + dOff;
    }
  }
  if (ptMax.y > upperRight.y) {  // Arc may intersect with top window boundary
    dDis = upperRight.y - m_center.y;
    dOff = std::sqrt(dRad * dRad - dDis * dDis);
    if (m_center.x + dOff <= upperRight.x && m_center.x + dOff >= lowerLeft.x) {
      ptWrk[iSecs].x = m_center.x + dOff;
      ptWrk[iSecs++].y = upperRight.y;
    }
    if (m_center.x - dOff >= lowerLeft.x && m_center.x - dOff <= upperRight.x) {
      ptWrk[iSecs].x = m_center.x - dOff;
      ptWrk[iSecs++].y = upperRight.y;
    }
  }
  if (ptMin.x < lowerLeft.x) {  // Arc may intersect with left window boundary
    dDis = m_center.x - lowerLeft.x;
    dOff = std::sqrt(dRad * dRad - dDis * dDis);
    if (m_center.y + dOff <= upperRight.y && m_center.y + dOff >= lowerLeft.y) {
      ptWrk[iSecs].x = lowerLeft.x;
      ptWrk[iSecs++].y = m_center.y + dOff;
    }
    if (m_center.y - dOff >= lowerLeft.y && m_center.y - dOff <= upperRight.y) {
      ptWrk[iSecs].x = lowerLeft.x;
      ptWrk[iSecs++].y = m_center.y - dOff;
    }
  }
  if (ptMin.y < lowerLeft.y) {  // Arc may intersect with bottom window boundary
    dDis = m_center.y - lowerLeft.y;
    dOff = std::sqrt(dRad * dRad - dDis * dDis);
    if (m_center.x - dOff >= lowerLeft.x && m_center.x - dOff <= upperRight.x) {
      ptWrk[iSecs].x = m_center.x - dOff;
      ptWrk[iSecs++].y = lowerLeft.y;
    }
    if (m_center.x + dOff <= upperRight.x && m_center.x + dOff >= lowerLeft.x) {
      ptWrk[iSecs].x = m_center.x + dOff;
      ptWrk[iSecs++].y = lowerLeft.y;
    }
  }
  if (iSecs == 0) return 0;

  double dBegAng = atan2(ptBeg.y - m_center.y, ptBeg.x - m_center.x);  // Arc begin angle (-π to π)

  double dIntAng[8]{};
  double dWrkAng;
  int iInts = 0;
  for (int i2 = 0; i2 < iSecs; i2++) {                                    // Loop thru possible intersections
    dWrkAng = atan2(ptWrk[i2].y - m_center.y, ptWrk[i2].x - m_center.x);  // Current intersection angle (-π to π)
    dIntAng[iInts] = dWrkAng - dBegAng;                                   // Sweep from begin to intersection
    if (dIntAng[iInts] < 0.0) dIntAng[iInts] += Eo::TwoPi;
    if (std::abs(dIntAng[iInts]) - m_sweepAngle < 0.0) {  // Intersection lies on arc
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
      if (std::abs(dIntAng[i2]) < std::abs(dIntAng[i2 - 1])) {
        double dAng = dIntAng[i2 - 1];
        dIntAng[i2 - 1] = dIntAng[i2];
        dIntAng[i2] = dAng;
        EoGePoint3d pt = ptInt[i2 - 1];
        ptInt[i2 - 1] = ptInt[i2];
        ptInt[i2] = pt;
      }
    }
  }
  if (std::abs(m_sweepAngle - Eo::TwoPi) < Eo::geometricTolerance) {  // Arc is a circle in disuise

  } else {
    if (ptBeg.x >= lowerLeft.x && ptBeg.x <= upperRight.x && ptBeg.y >= lowerLeft.y &&
        ptBeg.y <= upperRight.y) {  // Add beg point to int set
      for (int i = iInts; i > 0; i--) ptInt[i] = ptInt[i - 1];
      ptInt[0] = ptBeg;
      iInts++;
    }
    if (ptEnd.x >= lowerLeft.x && ptEnd.x <= upperRight.x && ptEnd.y >= lowerLeft.y &&
        ptEnd.y <= upperRight.y) {  // Add end point to int set
      ptInt[iInts] = ptEnd;
      iInts++;
    }
  }
  return iInts;
}

EoGePoint3d EoDbEllipse::GoToNextControlPoint() {
  double angle = (sm_RelationshipOfPoint < Eo::geometricTolerance) ? m_sweepAngle : 0.0;
  return (PointOnArcAtAngle(m_center, m_majorAxis, m_minorAxis, angle));
}

bool EoDbEllipse::IsInView(AeSysView* view) {
  EoGePoint3dArray pts;

  GetBoundingBox(pts);

  EoGePoint4d ptBeg(pts[0]);
  view->ModelViewTransformPoint(ptBeg);

  for (std::uint16_t w = 1; w < 4; w++) {
    EoGePoint4d ptEnd(pts[w]);
    view->ModelViewTransformPoint(ptEnd);

    if (EoGePoint4d::ClipLine(ptBeg, ptEnd)) { return true; }

    ptBeg = ptEnd;
  }
  return false;
}

EoGePoint3d EoDbEllipse::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;

  double dAPert = sm_SelectApertureSize;

  EoGePoint3d ptCtrl[] = {PointAtStartAngle(), PointAtEndAngle()};

  for (auto i = 0; i < 2; i++) {
    EoGePoint4d pt(ptCtrl[i]);

    view->ModelViewTransformPoint(pt);

    double dDis = point.DistanceToPointXY(pt);

    if (dDis < dAPert) {
      sm_controlPointIndex = i;
      dAPert = dDis;
    }
  }
  return (sm_controlPointIndex == SHRT_MAX) ? EoGePoint3d::kOrigin : ptCtrl[sm_controlPointIndex];
}

bool EoDbEllipse::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineStrip();
  GenPts(m_center, m_majorAxis, m_minorAxis, m_sweepAngle);
  return polyline::SelectUsingLine(view, line, intersections);
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

void EoDbEllipse::Transform(const EoGeTransformMatrix& transformMatrix) {
  m_center = transformMatrix * m_center;
  m_majorAxis = transformMatrix * m_majorAxis;
  m_minorAxis = transformMatrix * m_minorAxis;
}

void EoDbEllipse::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) m_center += v;
}

bool EoDbEllipse::Write(CFile& file) {
  EoDb::Write(file, std::uint16_t(EoDb::kEllipsePrimitive));
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
    double dEndX = std::cos(m_sweepAngle);
    double dEndY = std::sin(m_sweepAngle);

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
  EoGeTransformMatrix transformMatrix(m_center, m_majorAxis, m_minorAxis);
  transformMatrix.Inverse();

  for (auto i = 0; i < 4; i++) { ptsBox[i] = transformMatrix * ptsBox[i]; }
}

double EoDbEllipse::SweepAngleToPoint(EoGePoint3d point) {
  auto normal = CrossProduct(m_majorAxis, m_minorAxis);
  normal.Normalize();

  EoGeTransformMatrix transformMatrix(m_center, normal);

  EoGePoint3d beginPoint = m_center + m_majorAxis;
  EoGePoint3d endPoint = point;

  // Translate points into z=0 plane
  beginPoint = transformMatrix * beginPoint;
  endPoint = transformMatrix * endPoint;

  return (EoGeLine::AngleBetweenLn_xy(
      EoGeLine(EoGePoint3d::kOrigin, beginPoint), EoGeLine(EoGePoint3d::kOrigin, endPoint)));
}

// The arc's end point and intermediate point (for a 3-point representation) can be computed as:

//EoGePoint3d endPoint(m_centerPoint.x + radius * std::cos(normalizedStartAngle + m_sweepAngle), m_centerPoint.y + radius * std::sin(normalizedStartAngle + m_sweepAngle),
//                     m_centerPoint.z);
//EoGePoint3d middlePoint(m_centerPoint.x + radius * std::cos(normalizedStartAngle + m_sweepAngle * 0.5),
//                        m_centerPoint.y + radius * std::sin(normalizedStartAngle + m_sweepAngle * 0.5), m_centerPoint.z);