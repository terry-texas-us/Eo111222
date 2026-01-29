#include "Stdafx.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbConic.h"
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
EoGePoint3d PointOnArcAtAngle(EoGePoint3d center, EoGeVector3d majorAxis, EoGeVector3d minorAxis, const double angle) {
  EoGeTransformMatrix transformMatrix(center, majorAxis, minorAxis);
  transformMatrix.Inverse();

  EoGePoint3d point(cos(angle), sin(angle), 0.0);

  point = transformMatrix * point;
  return (point);
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
double EoDbConic::NormalizeTo2Pi(double angle) {
  if (!std::isfinite(angle)) { return 0.0; }

  // Reduce large values, keep result in [0, two_pi)
  angle = std::fmod(angle, Eo::TwoPi);
  if (angle < 0.0) angle += Eo::TwoPi;

  // Snap values very close to two_pi back to 0.0 to produce canonical result
  if (angle >= Eo::TwoPi - Eo::geometricTolerance) { angle = 0.0; }

  return angle;
}

CString EoDbConic::SubClassName(double ratio, double startAngle, double endAngle) {
  bool isCircular = fabs(1.0 - ratio) <= Eo::geometricTolerance;

  double sweep = NormalizeTo2Pi(endAngle) - NormalizeTo2Pi(startAngle);
  if (sweep <= 0.0) sweep += Eo::TwoPi;
  bool isFull = fabs(sweep - Eo::TwoPi) <= Eo::geometricTolerance;

  if (isCircular) {
    return isFull ? L"Circle" : L"Radial Arc";
  } else {
    return isFull ? L"Ellipse" : L"Elliptical Arc";
  }
}

EoDbConic* EoDbConic::CreateCircle(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius) {
  auto* circle = new EoDbConic(center, extrusion, radius, 0.0, Eo::TwoPi);
  circle->SetColor(pstate.PenColor());
  circle->SetLineTypeIndex(pstate.LineType());
  return circle;
}

EoDbConic* EoDbConic::CreateCircleInView(const EoGePoint3d& center, double radius) {
  auto* activeView = AeSysView::GetActiveView();
  auto cameraDirection = activeView->CameraDirection();
  cameraDirection.Normalize();

  return CreateCircle(center, cameraDirection, radius);
}

EoDbConic::EoDbConic(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius, double startAngle,
                     double endAngle)
    : EoDbPrimitive(),
      m_center(center),
      m_majorAxis(ComputeArbitraryAxis(extrusion) * radius),
      m_extrusion(extrusion),
      m_ratio(1.0),
      m_startAngle(startAngle),
      m_endAngle(endAngle) {}

///////////

/**
 * @brief Constructs a circle (as ellipse) primitive defined by a center point and radius in the current view.
 *
 * @param color The color.
 * @param lineType The line type index.
 * @param center The center point of the circle.
 * @param radius The radius of the circle.
 */
/*
EoDbConic::EoDbConic(EoGePoint3d& center, double radius, EoInt16 color, EoInt16 lineType)
    : EoDbPrimitive(color, lineType), m_center(center) {
  auto* activeView = AeSysView::GetActiveView();

  auto normal = activeView->CameraDirection();

  m_minorAxis = activeView->ViewUp() * radius;
  m_majorAxis = m_minorAxis;
  m_majorAxis.RotAboutArbAx(normal, -Eo::HalfPi);
  m_sweepAngle = Eo::TwoPi;
}
*/
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

/*
EoDbConic::EoDbConic(EoGePoint3d& center, EoGeVector3d& normal, double radius, EoInt16 color, EoInt16 lineType)
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
*/
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

/*
EoDbConic::EoDbConic(EoGePoint3d& center, EoGePoint3d& start) {
  auto* activeView = AeSysView::GetActiveView();

  m_color = pstate.PenColor();
  m_lineTypeIndex = pstate.LineType();

  auto cameraDirection = activeView->CameraDirection();

  m_center = center;
  m_majorAxis = EoGeVector3d(center, start);
  m_sweepAngle = Eo::TwoPi;
}
*/
EoDbConic::EoDbConic(const EoGePoint3d& center, const EoGeVector3d& extrusion, const EoGeVector3d& majorAxis,
                     double ratio)
    : EoDbPrimitive(),
      m_center(center),
      m_majorAxis(majorAxis),
      m_extrusion(extrusion),
      m_ratio(ratio),
      m_startAngle(0.0),
      m_endAngle(Eo::TwoPi) {}

EoDbConic::EoDbConic(EoGePoint3d start, EoGePoint3d intermediate, EoGePoint3d end)
    : EoDbPrimitive(pstate.PenColor(), pstate.LineType()) {
  EoGeVector3d startToIntermediate(start, intermediate);
  EoGeVector3d startToEnd(start, end);
  auto normal = CrossProduct(startToIntermediate, startToEnd);
  normal.Normalize();

  // Ensure extrusion points in positive Z direction
  // If normal.z < 0, the arc is CW when viewed from +Z, so swap start/end to make it CCW
  if (normal.z < 0.0) {
    normal = -normal;
    std::swap(start, end);  // Reverse arc direction to maintain CCW sweep
  }

  m_extrusion = normal;
  m_ratio = 1.0;

  // Build transformation matrix which will get int and end points to z=0 plane with beg point as origin
  EoGeTransformMatrix transformMatrix(start, normal);

  EoGePoint3d pt[3]{start, intermediate, end};
  pt[1] = transformMatrix * pt[1];
  pt[2] = transformMatrix * pt[2];

  double determinant = (pt[1].x * pt[2].y - pt[2].x * pt[1].y);

  if (fabs(determinant) > Eo::geometricTolerance) {  // Three points are not colinear
    double dT = ((pt[2].x - pt[1].x) * pt[2].x + pt[2].y * (pt[2].y - pt[1].y)) / determinant;

    m_center.x = (pt[1].x - pt[1].y * dT) * 0.5;
    m_center.y = (pt[1].y + pt[1].x * dT) * 0.5;
    m_center.z = 0.0;
    transformMatrix = transformMatrix.Inverse();
    m_center = transformMatrix * m_center;

    // Recalculate in z=0 plane with center point at origin
    transformMatrix = EoGeTransformMatrix(m_center, normal);

    pt[0] = start;
    pt[1] = intermediate;
    pt[2] = end;

    for (int i = 0; i < 3; i++) { pt[i] = transformMatrix * pt[i]; }

    double radius = EoGeVector3d(EoGePoint3d::kOrigin, pt[0]).Length();

    // Compute angles
    double angles[3]{};
    for (int i = 0; i < 3; i++) {
      angles[i] = atan2(pt[i].y, pt[i].x);
      if (angles[i] < 0.0) { angles[i] += Eo::TwoPi; }
    }
    m_startAngle = angles[0];

    // Calculate CCW sweep from start through intermediate to end
    double sweepToIntermediate = angles[1] - angles[0];
    if (sweepToIntermediate < 0.0) { sweepToIntermediate += Eo::TwoPi; }

    double sweepToEnd = angles[2] - angles[0];
    if (sweepToEnd < 0.0) { sweepToEnd += Eo::TwoPi; }

    // Verify intermediate is between start and end in CCW direction
    double totalSweep;
    if (sweepToIntermediate < sweepToEnd) {
      // Intermediate is correctly between start and end (CCW)
      totalSweep = sweepToEnd;
    } else {
      // Intermediate is on the "long way" around, use the other direction
      totalSweep = Eo::TwoPi - sweepToEnd;
      totalSweep = -totalSweep;  // Make negative to indicate CW (which shouldn't happen after our flip)
    }

    m_endAngle = m_startAngle + totalSweep;
    m_majorAxis = ComputeArbitraryAxis(m_extrusion) * radius;
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
EoDbConic::EoDbConic(const EoGePoint3d& center, double radius, double startAngle, double endAngle) {
  m_center = center;

  double normalizedStartAngle = NormalizeTo2Pi(startAngle);
  double normalizedEndAngle = NormalizeTo2Pi(endAngle);

  // Compute CCW sweep from start to end in range (0, 2*pi] (canonical representation: positive = CCW sweep)
  double sweepAngle = normalizedEndAngle - normalizedStartAngle;
  if (sweepAngle < 0.0) sweepAngle += Eo::TwoPi;
  if (fabs(sweepAngle) < Eo::geometricTolerance) { sweepAngle = Eo::TwoPi; }  // identical => full circle
  m_sweepAngle = sweepAngle;

  // Build major axis vector from center to start point
  EoGePoint3d startPoint(m_center.x + radius * cos(normalizedStartAngle),
                         m_center.y + radius * sin(normalizedStartAngle), m_center.z);
  m_majorAxis = EoGeVector3d(m_center, startPoint);
}

/** 
 * @brief Constructs an conic primitive defined by a center point, major and minor axes, and a sweep angle.
 *
 * This constructor initializes a conic primitive using the specified pen color, line type, center point,
 * major axis, minor axis, and sweep angle from legacy peg ellipse primitive.
 *
 * @param center The center point of the ellipse.
 * @param majorAxis The major axis vector of the ellipse.
 * @param minorAxis The minor axis vector of the ellipse.
 * @param sweepAngle The sweep angle of the ellipse segment in radians.
 * @param color The pen color for the ellipse.
 * @param lineType The line type index for the ellipse.
 */
EoDbConic::EoDbConic(EoGePoint3d& center, EoGeVector3d& majorAxis, EoGeVector3d& minorAxis, double sweepAngle)
    : m_center(center),
      m_majorAxis(majorAxis),
      m_extrusion(CrossProduct(majorAxis, minorAxis)),
      m_ratio(minorAxis.Length() / majorAxis.Length()),
      m_startAngle(0.0),
      m_endAngle(sweepAngle) {
  m_extrusion.Normalize();
}

EoDbConic::EoDbConic(const EoDbConic& other)
    : EoDbPrimitive(other),
      m_center(other.m_center),
      m_majorAxis(other.m_majorAxis),
      m_extrusion(other.m_extrusion),
      m_ratio(other.m_ratio),
      m_startAngle(other.m_startAngle),
      m_endAngle(other.m_endAngle) {
  m_sweepAngle = other.m_sweepAngle;
}

const EoDbConic& EoDbConic::operator=(const EoDbConic& other) {
  if (this != &other) {
    EoDbPrimitive::operator=(other);
    m_center = other.m_center;
    m_majorAxis = other.m_majorAxis;
    m_extrusion = other.m_extrusion;
    m_ratio = other.m_ratio;
    m_startAngle = other.m_startAngle;
    m_endAngle = other.m_endAngle;

    m_sweepAngle = other.m_sweepAngle;
  }
  return (*this);
}

void EoDbConic::AddReportToMessageList(EoGePoint3d) {
  CString color = FormatPenColor();
  CString lineType = FormatLineType();

  auto subClassName = SubClassName(m_ratio, m_startAngle, m_endAngle);
  app.AddStringToMessageList(subClassName);

  CString message;
  message.Format(L"  Color: %s, Line Type: %s", color.GetBuffer(), lineType.GetBuffer());
  app.AddStringToMessageList(message);

  double radius = m_majorAxis.Length();

  auto conicType = Subclass();
  switch (conicType) {
    case ConicType::Circle:
      message.Format(L"  Radius: %.4f", radius);
      break;

    case ConicType::RadialArc:
      message.Format(L"  Radius: %.4f Start Angle: %.2f° End Angle: %.2f°", radius, Eo::RadianToDegree(m_startAngle),
                     Eo::RadianToDegree(m_endAngle));
      break;
    case ConicType::Ellipse:
      message.Format(L"  Major Radius: %.4f Radius Ratio: %.4f", radius, m_ratio);
      break;

    case ConicType::EllipticalArc:
      message.Format(L"  Major Radius: %.4f Radius Ratio: %.4f Start Angle: %.2f° End Angle: %.2f°", radius, m_ratio,
                     Eo::RadianToDegree(m_startAngle), Eo::RadianToDegree(m_endAngle));
      break;
  }
  app.AddStringToMessageList(message);
}

void EoDbConic::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label = SubClassName(m_ratio, m_startAngle, m_endAngle);
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbConic::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbConic(*this);
  return (primitive);
}

void EoDbConic::CutAt2Pts(EoGePoint3d* pt, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  EoDbConic* pArc;

  double dRel[2]{};

  dRel[0] = SweepAngleToPoint(pt[0]) / m_sweepAngle;
  dRel[1] = SweepAngleToPoint(pt[1]) / m_sweepAngle;

  if (dRel[0] <= Eo::geometricTolerance && dRel[1] >= 1.0 - Eo::geometricTolerance) {  // Put entire arc in trap
    pArc = this;
  } else {  // Something gets cut
    auto normal = CrossProduct(m_majorAxis, MinorAxis());
    normal.Normalize();

    if (fabs(m_sweepAngle - Eo::TwoPi) <= Eo::geometricTolerance) {  // Closed arc
      m_sweepAngle = (dRel[1] - dRel[0]) * Eo::TwoPi;

      m_majorAxis.RotAboutArbAx(normal, dRel[0] * Eo::TwoPi);

      pArc = new EoDbConic(*this);

      m_majorAxis.RotAboutArbAx(normal, m_sweepAngle);

      m_sweepAngle = Eo::TwoPi - m_sweepAngle;
    } else {  // Arc section with a cut
      pArc = new EoDbConic(*this);
      double dSwpAng = m_sweepAngle;

      double dAng1 = dRel[0] * m_sweepAngle;
      double dAng2 = dRel[1] * m_sweepAngle;

      if (dRel[0] > Eo::geometricTolerance && dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section out of middle
        pArc->SetSweepAngle(dAng1);
        groups->AddTail(new EoDbGroup(pArc));

        m_majorAxis.RotAboutArbAx(normal, dAng1);
        m_sweepAngle = dAng2 - dAng1;

        pArc = new EoDbConic(*this);

        m_majorAxis.RotAboutArbAx(normal, m_sweepAngle);
        m_sweepAngle = dSwpAng - dAng2;
      } else if (dRel[1] < 1.0 - Eo::geometricTolerance) {  // Cut section in two and place begin section in trap
        pArc->SetSweepAngle(dAng2);

        m_majorAxis.RotAboutArbAx(normal, dAng2);
        m_sweepAngle = dSwpAng - dAng2;
      } else {  // Cut section in two and place end section in trap
        m_sweepAngle = dAng1;

        EoGeVector3d v = m_majorAxis;
        v.RotAboutArbAx(normal, dAng1);
        pArc->SetMajorAxis(v);
        pArc->SetSweepAngle(dSwpAng - dAng1);
      }
    }
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(pArc));
}

void EoDbConic::CutAtPt(EoGePoint3d& point, EoDbGroup* group) {
  if (group == nullptr) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Warning: Null group in CutAtPt\n");
    return;
  }
  if (IsFullConic()) { return; }  // Do not fragment a circle or full ellipse

  double sweepAngle = SweepAngle();
  if (fabs(sweepAngle) <= Eo::geometricTolerance) { return; }  // Nothing to cut

  double parameterAtPoint = SweepAngleToPoint(point) / sweepAngle;

  if (parameterAtPoint <= Eo::geometricTolerance || parameterAtPoint >= 1.0 - Eo::geometricTolerance) { return; }
  // Cut point is not on or beyond endpoints

  double absoluteAngleAtPoint = m_startAngle + sweepAngle * parameterAtPoint;

  EoDbConic* newConic = new EoDbConic(*this);

  newConic->SetStartAngle(absoluteAngleAtPoint);
  newConic->SetSweepAngle(m_endAngle - absoluteAngleAtPoint);  // Deprecated, but kept for temporary continuity

  group->AddTail(newConic);

  m_endAngle = absoluteAngleAtPoint;
  m_sweepAngle = m_endAngle - m_startAngle;  // Deprecated, but kept for temporary continuity
}

void EoDbConic::Display(AeSysView* view, CDC* deviceContext) {
  double sweepAngle = m_endAngle - m_startAngle;
  if (fabs(sweepAngle) <= Eo::geometricTolerance) { return; }

  auto color = LogicalColor();
  auto lineType = LogicalLineType();

  pstate.SetPen(view, deviceContext, color, lineType);

  polyline::BeginLineStrip();
  GenerateApproximationVertices(m_center, m_majorAxis);
  polyline::__End(view, deviceContext, lineType);
}

void EoDbConic::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_center);
}

void EoDbConic::GenerateApproximationVertices(EoGePoint3d center, EoGeVector3d majorAxis) const {
  auto minorAxis = MinorAxis();

  // Normalize angles to [0, 2π) and ensure CCW sweep
  double normalizedStartAngle = NormalizeTo2Pi(m_startAngle);
  double normalizedEndAngle = NormalizeTo2Pi(m_endAngle);

  // Compute CCW sweep angle from start to end
  double sweepAngle = normalizedEndAngle - normalizedStartAngle;
  if (sweepAngle <= 0.0) { sweepAngle += Eo::TwoPi; }

  // For a full circle/ellipse (start == end), use full sweep
  if (fabs(m_endAngle - m_startAngle) <= Eo::geometricTolerance ||
      fabs(sweepAngle - Eo::TwoPi) <= Eo::geometricTolerance) {
    sweepAngle = Eo::TwoPi;
  }

  // Calculate number of vertices based on arc length and curvature
  double maxAxisLength = std::max(majorAxis.Length(), minorAxis.Length());
  int numberOfPoints = std::max(2, abs(Eo::Round(sweepAngle / Eo::TwoPi * 32.0)));
  numberOfPoints = std::min(128, std::max(numberOfPoints, abs(Eo::Round(sweepAngle * maxAxisLength / 0.250))));

  // Build transformation matrix: local ellipse coordinates → world coordinates
  // majorAxis and minorAxis define the ellipse orientation (NOT rotated by startAngle)
  // For ellipses, startAngle is a parametric t-parameter, not a geometric angle
  EoGeTransformMatrix transformMatrix(center, majorAxis, minorAxis);
  transformMatrix.Inverse();

  // Calculate segment angle and pre-compute sin/cos for rotation
  double segmentAngle = sweepAngle / (numberOfPoints - 1);
  double angleCosine = cos(segmentAngle);
  double angleSine = sin(segmentAngle);

  // Start at the parametric position defined by normalizedStartAngle
  // For ellipses: P(t) = center + cos(t)*majorAxis + sin(t)*minorAxis
  // In local coords: (cos(startAngle), sin(startAngle), 0)
  EoGePoint3d point(cos(normalizedStartAngle), sin(normalizedStartAngle), 0.0);

  // Generate vertices by incrementally rotating the point
  for (int i = 0; i < numberOfPoints; i++) {
    polyline::SetVertex(transformMatrix * point);

    // Rotate point by segmentAngle in parametric space
    // This works for both circles (where parametric = geometric)
    // and ellipses (where we work in parametric space)
    // [cos(θ) -sin(θ)] [x]   [x*cos(θ) - y*sin(θ)]
    // [sin(θ)  cos(θ)] [y] = [x*sin(θ) + y*cos(θ)]
    double newX = point.x * angleCosine - point.y * angleSine;
    double newY = point.x * angleSine + point.y * angleCosine;
    point(newX, newY, 0.0);
  }
}

void EoDbConic::FormatGeometry(CString& geometry) {
  CString subClassName = SubClassName(m_ratio, m_startAngle, m_endAngle);

  geometry += L"Center Point;" + m_center.ToString();
  if (subClassName == L"<Ellipse>") { geometry += L"Major Axis;" + m_majorAxis.ToString(); }
  geometry += L"Extrusion;" + m_extrusion.ToString();
}

void EoDbConic::FormatExtra(CString& extra) {
  auto conicType = Subclass();
  CString format{L"Color;%s\tLine Type;%s\t"};

  switch (conicType) {
    case ConicType::Circle:
      format += L"Radius;%.4f\t";
      extra.Format(format, FormatPenColor().GetString(), FormatLineType().GetString(), m_majorAxis.Length());
      break;

    case ConicType::RadialArc:
      format += L"Radius;%.4f\tStart Angle;%.2f°\tEnd Angle;%.2f°\tSweep;%.2f°\t";
      extra.Format(format, FormatPenColor().GetString(), FormatLineType().GetString(), m_majorAxis.Length(),
                   Eo::RadianToDegree(m_startAngle), Eo::RadianToDegree(m_endAngle),
                   Eo::RadianToDegree(m_endAngle - m_startAngle));
      break;

    case ConicType::Ellipse:
      format += L"Major Radius;%.4f\tMinor Radius;%.4f\tRatio;%.4f\t";
      extra.Format(format, FormatPenColor().GetString(), FormatLineType().GetString(), m_majorAxis.Length(),
                   m_majorAxis.Length() * m_ratio, m_ratio);
      break;

    case ConicType::EllipticalArc:
      format += L"Major Radius;%.4f\tRatio;%.4f\tStart Param;%.2f°\tEnd Param;%.2f°\t";
      extra.Format(format, FormatPenColor().GetString(), FormatLineType().GetString(), m_majorAxis.Length(), m_ratio,
                   Eo::RadianToDegree(m_startAngle), Eo::RadianToDegree(m_endAngle));
      break;
  }
}

EoGePoint3d EoDbConic::PointAtStartAngle() {
  EoGeTransformMatrix transformMatrix(m_center, m_majorAxis, MinorAxis());
  transformMatrix.Inverse();

  // Use parametric start angle to compute the beginning point
  EoGePoint3d point(cos(m_startAngle), sin(m_startAngle), 0.0);

  point = transformMatrix * point;
  return point;
}

EoGePoint3d EoDbConic::PointAtEndAngle() {
  EoGeTransformMatrix transformMatrix(m_center, m_majorAxis, MinorAxis());
  transformMatrix.Inverse();

  // Use parametric end angle to compute the ending point
  EoGePoint3d point(cos(m_endAngle), sin(m_endAngle), 0.0);

  point = transformMatrix * point;
  return point;
}

void EoDbConic::GetXYExtents(EoGePoint3d arBeg, EoGePoint3d arEnd, EoGePoint3d* arMin, EoGePoint3d* arMax) const {
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
          if (arBeg.x > arEnd.x) {    // Arc in qraudrant one only
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
          if (arBeg.x > arEnd.x) {    // Arc in qraudrant two only
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

void EoDbConic::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
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

bool EoDbConic::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  // Determines if a point is on a control point of the arc.

  EoGePoint4d pt[] = {EoGePoint4d(PointAtStartAngle()), EoGePoint4d(PointAtEndAngle())};

  for (EoUInt16 w = 0; w < 2; w++) {
    view->ModelViewTransformPoint(pt[w]);

    if (point.DistanceToPointXY(pt[w]) < sm_SelectApertureSize) return true;
  }
  return false;
}

int EoDbConic::IsWithinArea(EoGePoint3d ptLL, EoGePoint3d ptUR, EoGePoint3d* ptInt) {
  auto normal = CrossProduct(m_majorAxis, MinorAxis());
  normal.Normalize();

  if (!(CrossProduct(EoGeVector3d::positiveUnitZ, normal)).IsNearNull()) { return 0; }

  if (fabs(m_majorAxis.Length() - MinorAxis().Length()) > Eo::geometricTolerance) { return 0; }

  EoGePoint3d ptMin, ptMax;

  auto ptBeg = PointAtStartAngle();
  auto ptEnd = PointAtEndAngle();

  if (normal.z < 0.0) {
    EoGePoint3d pt = ptBeg;
    ptBeg = ptEnd;
    ptEnd = pt;

    normal = -normal;
    m_majorAxis = EoGeVector3d(m_center, ptBeg);
  }

  GetXYExtents(ptBeg, ptEnd, &ptMin, &ptMax);

  if (ptMin.x >= ptLL.x && ptMax.x <= ptUR.x && ptMin.y >= ptLL.y && ptMax.y <= ptUR.y) {
    // Totally within window boundaries
    ptInt[0] = ptBeg;
    ptInt[1] = ptEnd;
    return (2);
  }
  if (ptMin.x >= ptUR.x || ptMax.x <= ptLL.x || ptMin.y >= ptUR.y || ptMax.y <= ptLL.y) { return 0; }

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
  for (int i2 = 0; i2 < iSecs; i2++) {                                    // Loop thru possible intersections
    dWrkAng = atan2(ptWrk[i2].y - m_center.y, ptWrk[i2].x - m_center.x);  // Current intersection angle (- pi to
    dIntAng[iInts] = dWrkAng - dBegAng;                                   // Sweep from begin to intersection
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
  if (fabs(m_sweepAngle - Eo::TwoPi) <= Eo::geometricTolerance) {  // Arc is a circle in disuise

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

/**
 * @brief Computes the next control point on the conic section.
 *
 * This method determines the next control point based on the current relationship of the point
 * to the conic section. It uses the parametric angle to find the appropriate point on the arc.
 *
 * @note If close to start (sm_RelationshipOfPoint near 0), return end point; otherwise return start point

 * @return The next control point on the conic section.
 */
EoGePoint3d EoDbConic::GoToNextControlPoint() {
  double parametricAngle = (sm_RelationshipOfPoint <= Eo::geometricTolerance) ? m_endAngle : m_startAngle;
  return PointOnArcAtAngle(m_center, m_majorAxis, MinorAxis(), parametricAngle);
}

bool EoDbConic::IsInView(AeSysView* view) {
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

EoGePoint3d EoDbConic::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;

  double dAPert = sm_SelectApertureSize;

  EoGePoint3d ptCtrl[] = {PointAtStartAngle(), PointAtEndAngle()};

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

bool EoDbConic::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineStrip();
  GenerateApproximationVertices(m_center, m_majorAxis);
  return polyline::SelectUsingLine(view, line, intersections);
}

bool EoDbConic::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  polyline::BeginLineStrip();
  GenerateApproximationVertices(m_center, m_majorAxis);
  return (polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, ptProj));
}

bool EoDbConic::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  polyline::BeginLineStrip();
  GenerateApproximationVertices(m_center, m_majorAxis);
  return polyline::SelectUsingRectangle(view, pt1, pt2);
}

void EoDbConic::Transform(EoGeTransformMatrix& transformationMatrix) {
  m_center = transformationMatrix * m_center;
  m_majorAxis = transformationMatrix * m_majorAxis;
  m_extrusion = transformationMatrix * m_extrusion;
  EoGeVector3d minorAxis = MinorAxis();
  m_ratio = minorAxis.Length() / m_majorAxis.Length();
  // if ratio is greater than zero, then major and minor axes need to be reversed. Are the start and end angles still valid?
  if (m_ratio > 0.0) { m_majorAxis = minorAxis; }
  m_extrusion.Normalize();
}

void EoDbConic::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) m_center += v;
}

bool EoDbConic::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kConicPrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_center.Write(file);
  m_majorAxis.Write(file);
  m_extrusion.Write(file);
  EoDb::Write(file, m_ratio);
  EoDb::Write(file, m_startAngle);
  EoDb::Write(file, m_endAngle);

  return true;
}

void EoDbConic::GetBoundingBox(EoGePoint3dArray& ptsBox) {
  ptsBox.SetSize(4);
  ptsBox[0] = EoGePoint3d(-1.0, -1.0, 0.0);
  ptsBox[1] = EoGePoint3d(1.0, -1.0, 0.0);
  ptsBox[2] = EoGePoint3d(1.0, 1.0, 0.0);
  ptsBox[3] = EoGePoint3d(-1.0, 1.0, 0.0);

  // Compute sweep angle from start/end angles
  double normalizedStartAngle = NormalizeTo2Pi(m_startAngle);
  double normalizedEndAngle = NormalizeTo2Pi(m_endAngle);
  double sweepAngle = normalizedEndAngle - normalizedStartAngle;
  if (sweepAngle <= 0.0) { sweepAngle += Eo::TwoPi; }

  // For full conics, use standard bounding box
  if (!IsFullConic() && sweepAngle < 3.0 * Eo::TwoPi / 4.0) {
    // For arcs less than 270°, optimize the bounding box
    // Compute end point position in parametric space
    double dEndX = cos(sweepAngle);
    double dEndY = sin(sweepAngle);

    if (dEndX >= 0.0) {
      if (dEndY >= 0.0) {  // Arc ends in quadrant one
        ptsBox[0].x = dEndX;
        ptsBox[0].y = 0.0;
        ptsBox[1].y = 0.0;
        ptsBox[2].y = dEndY;
        ptsBox[3].x = dEndX;
        ptsBox[3].y = dEndY;
      }
    } else {
      if (dEndY >= 0.0) {  // Arc ends in quadrant two
        ptsBox[0].x = dEndX;
        ptsBox[0].y = 0.0;
        ptsBox[1].y = 0.0;
        ptsBox[3].x = dEndX;
      } else {  // Arc ends in quadrant three
        ptsBox[0].y = dEndY;
        ptsBox[1].y = dEndY;
      }
    }
  }

  // Transform from parametric unit circle to actual conic in world space
  // This works for both circles (ratio=1.0) and ellipses (ratio<1.0)
  EoGeVector3d minorAxis = MinorAxis();

  // Rotate axes to align with start angle in parametric space
  EoGeVector3d rotatedMajorAxis = m_majorAxis;
  EoGeVector3d rotatedMinorAxis = minorAxis;
  rotatedMajorAxis.RotAboutArbAx(m_extrusion, normalizedStartAngle);
  rotatedMinorAxis.RotAboutArbAx(m_extrusion, normalizedStartAngle);

  EoGeTransformMatrix tm(m_center, rotatedMajorAxis, rotatedMinorAxis);
  tm.Inverse();

  for (EoUInt16 w = 0; w < 4; w++) { ptsBox[w] = tm * ptsBox[w]; }
}

double EoDbConic::SweepAngleToPoint(EoGePoint3d point) {
  auto normal = CrossProduct(m_majorAxis, MinorAxis());
  normal.Normalize();

  EoGeTransformMatrix transformMatrix(m_center, normal);

  auto startPoint = PointAtStartAngle();

  auto endPoint = point;

  // Translate points into z=0 plane
  startPoint = transformMatrix * startPoint;
  endPoint = transformMatrix * endPoint;

  return (EoGeLine::AngleBetweenLn_xy(EoGeLine(EoGePoint3d::kOrigin, startPoint),
                                      EoGeLine(EoGePoint3d::kOrigin, endPoint)));
}

/** @brief Given a plane normal and three points (two outside and one inside), find the sweep angle defined by the three points about the center point.
 * @param planeNormal Normal vector of the plane containing the points
 * @param arP1 First outside point
 * @param arP2 Inside point
 * @param arP3 Second outside point
 * @param center Center point about which to measure the sweep angle
 * @param adTheta Sweep angle result
 * @return TRUE if successful, FALSE if not.
*/
int SweepAngleFromNormalAnd3Points(EoGeVector3d normal, EoGePoint3d arP1, EoGePoint3d arP2, EoGePoint3d arP3,
                                   EoGePoint3d& center, double* adTheta) {
  double dT[3]{};
  EoGePoint3d rR[3]{};

  if (arP1 == center || arP2 == center || arP3 == center) { return (FALSE); }

  // None of the points coincide with center point
  EoGeTransformMatrix tm(center, normal);
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
  if (fabs(dT[1] - dTMax) > Eo::geometricTolerance &&
      fabs(dT[1] - dTMin) > Eo::geometricTolerance) {  // Inside line is not colinear with outside lines
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
