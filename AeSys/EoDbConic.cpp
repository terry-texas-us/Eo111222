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

EoDbConic::EoDbConic(const EoGePoint3d& center, const EoGeVector3d& extrusion, const EoGeVector3d& majorAxis,
                     double ratio, double startAngle, double endAngle)
    : m_center(center),
      m_majorAxis(majorAxis),
      m_extrusion(extrusion),
      m_ratio(std::clamp(ratio, Eo::geometricTolerance, 1.0)),
      m_startAngle(startAngle),
      m_endAngle(endAngle) {
  // Validate invariants - these should never fail if factory methods are used correctly
  ASSERT(m_majorAxis.Length() > Eo::geometricTolerance && "Major axis must have non-zero length");
  ASSERT(m_ratio > 0.0 && m_ratio <= 1.0 + Eo::numericEpsilon && "Ratio must be in (0, 1]");
  ASSERT(m_extrusion.Length() > Eo::geometricTolerance && "Extrusion must have non-zero length");

  // Ensure extrusion is normalized (release - defensive)
  if (m_extrusion.Length() > Eo::geometricTolerance) { m_extrusion.Normalize(); }
}

EoDbConic* EoDbConic::CreateCircle(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius) {
  auto majorAxis = ComputeArbitraryAxis(extrusion) * radius;
  return new EoDbConic(center, extrusion, majorAxis, 1.0, 0.0, Eo::TwoPi);
}

EoDbConic* EoDbConic::CreateCircleInView(const EoGePoint3d& center, double radius) {
  auto* activeView = AeSysView::GetActiveView();
  auto cameraDirection = activeView->CameraDirection();
  cameraDirection.Normalize();

  return CreateCircle(center, cameraDirection, radius);
}

EoDbConic* EoDbConic::CreateConic(EoGePoint3d& center, EoGeVector3d& extrusion, EoGeVector3d& majorAxis, double ratio,
                                  double startAngle, double endAngle) {
  return new EoDbConic(center, extrusion, majorAxis, ratio, startAngle, endAngle);
}

EoDbConic* EoDbConic::CreateConicFromEllipsePrimitive(EoGePoint3d& center, EoGeVector3d& majorAxis,
                                                      EoGeVector3d& minorAxis, double sweepAngle) {
  EoGeVector3d extrusion{CrossProduct(majorAxis, minorAxis)};
  extrusion.Normalize();

  double ratio{minorAxis.Length() / majorAxis.Length()};

  return new EoDbConic(center, extrusion, majorAxis, ratio, 0.0, sweepAngle);
}

EoDbConic* EoDbConic::CreateEllipse(const EoGePoint3d& center, const EoGeVector3d& extrusion,
                                    const EoGeVector3d& majorAxis, double ratio) {
  return new EoDbConic(center, extrusion, majorAxis, ratio, 0.0, Eo::TwoPi);
}

EoDbConic* EoDbConic::CreateRadialArc(const EoGePoint3d& center, const EoGeVector3d& extrusion, double radius,
                                      double startAngle, double endAngle) {
  if (radius <= Eo::geometricTolerance) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"CreateRadialArc: Invalid radius (%.6f)\n", radius);
    return nullptr;
  }
  if (extrusion.IsNearNull()) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"CreateRadialArc: Null extrusion vector\n");
    return nullptr;
  }
  // Compute OCS X-axis (major axis direction) using arbitrary axis algorithm
  EoGeVector3d normalizedExtrusion = extrusion;
  normalizedExtrusion.Normalize();

  auto majorAxis = ComputeArbitraryAxis(normalizedExtrusion) * radius;
  return new EoDbConic(center, normalizedExtrusion, majorAxis, 1.0, startAngle, endAngle);
}

EoDbConic* EoDbConic::CreateRadialArcFrom3Points(EoGePoint3d start, EoGePoint3d intermediate, EoGePoint3d end) {
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

  EoGeVector3d extrusion = normal;
  double ratio{1.0};

  // Build transformation matrix which will get int and end points to z=0 plane with beg point as origin
  EoGeTransformMatrix transformMatrix(start, normal);

  EoGePoint3d pt[3]{start, intermediate, end};
  pt[1] = transformMatrix * pt[1];
  pt[2] = transformMatrix * pt[2];

  double determinant = (pt[1].x * pt[2].y - pt[2].x * pt[1].y);

  if (fabs(determinant) > Eo::geometricTolerance) {  // Three points are not colinear
    double dT = ((pt[2].x - pt[1].x) * pt[2].x + pt[2].y * (pt[2].y - pt[1].y)) / determinant;

    EoGePoint3d center{(pt[1].x - pt[1].y * dT) * 0.5, (pt[1].y + pt[1].x * dT) * 0.5, 0.0};

    transformMatrix = transformMatrix.Inverse();
    center = transformMatrix * center;

    // Recalculate in z=0 plane with center point at origin
    transformMatrix = EoGeTransformMatrix(center, normal);

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
    double startAngle = angles[0];

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

    double endAngle = startAngle + totalSweep;
    EoGeVector3d majorAxis = ComputeArbitraryAxis(extrusion) * radius;
    return new EoDbConic(center, extrusion, majorAxis, ratio, startAngle, endAngle);
  }
  return nullptr;
}

EoDbConic::EoDbConic(const EoDbConic& other)
    : EoDbPrimitive(other),
      m_center(other.m_center),
      m_majorAxis(other.m_majorAxis),
      m_extrusion(other.m_extrusion),
      m_ratio(other.m_ratio),
      m_startAngle(other.m_startAngle),
      m_endAngle(other.m_endAngle) {}

const EoDbConic& EoDbConic::operator=(const EoDbConic& other) {
  if (this != &other) {
    EoDbPrimitive::operator=(other);
    m_center = other.m_center;
    m_majorAxis = other.m_majorAxis;
    m_extrusion = other.m_extrusion;
    m_ratio = other.m_ratio;
    m_startAngle = other.m_startAngle;
    m_endAngle = other.m_endAngle;
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
  double totalSweep = SweepAngle();
  double rel0 = SweepAngleToPoint(pt[0]) / totalSweep;
  double rel1 = SweepAngleToPoint(pt[1]) / totalSweep;

  // Clamp to valid range
  rel0 = std::clamp(rel0, 0.0, 1.0);
  rel1 = std::clamp(rel1, 0.0, 1.0);

  // Ensure rel0 < rel1
  if (rel0 > rel1) { std::swap(rel0, rel1); }

  EoDbConic* trappedArc{};

  if (rel0 <= Eo::geometricTolerance && rel1 >= 1.0 - Eo::geometricTolerance) {
    // Entire arc in trap - nothing gets cut
    trappedArc = this;
  } else {  // Something gets cut
    double originalStart = m_startAngle;
    double originalEnd = m_endAngle;

    double angle0 = originalStart + totalSweep * rel0;
    double angle1 = originalStart + totalSweep * rel1;

    if (IsFullConic()) {  // Closed conic (full circle or ellipse) - split into trapped and remaining portions
      trappedArc = new EoDbConic(*this);
      trappedArc->SetAngles(angle0, angle1);

      // Remaining portion wraps around
      m_startAngle = angle1;
      m_endAngle = angle0 + Eo::TwoPi;  // Wrap to complete the circle

      groups->AddTail(new EoDbGroup(this));
    } else {  // Arc section - may produce 1, 2, or 3 pieces
      if (rel0 > Eo::geometricTolerance && rel1 < 1.0 - Eo::geometricTolerance) {
        // Cut section out of middle - produces 3 pieces
        // Piece 1: original start to first cut point
        EoDbConic* piece1 = new EoDbConic(*this);
        piece1->SetAngles(originalStart, angle0);
        groups->AddTail(new EoDbGroup(piece1));

        // Piece 2 (trapped): between cut points
        trappedArc = new EoDbConic(*this);
        trappedArc->SetAngles(angle0, angle1);

        // Piece 3 (this): second cut point to original end
        m_startAngle = angle1;
        m_endAngle = originalEnd;
        groups->AddTail(new EoDbGroup(this));
      } else if (rel1 < 1.0 - Eo::geometricTolerance) {
        // Trap beginning section (rel0 near 0)
        trappedArc = new EoDbConic(*this);
        trappedArc->SetAngles(originalStart, angle1);

        // Remaining portion
        m_startAngle = angle1;
        m_endAngle = originalEnd;
        groups->AddTail(new EoDbGroup(this));
      } else {
        // Trap ending section (rel1 near 1)
        // Keep beginning section
        m_endAngle = angle0;
        groups->AddTail(new EoDbGroup(this));

        // Trapped portion
        trappedArc = new EoDbConic(*this);
        trappedArc->SetAngles(angle0, originalEnd);
      }
    }
  }
  newGroups->AddTail(new EoDbGroup(trappedArc));
}

void EoDbConic::CutAtPt(EoGePoint3d& point, EoDbGroup* group) {
  if (group == nullptr) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"Warning: Null group in CutAtPt\n");
    return;
  }
  if (IsFullConic()) { return; }  // @todo Consider moving start angle to point, but no cutting needed

  double sweepAngle = SweepAngle();
  if (fabs(sweepAngle) <= Eo::geometricTolerance) { return; }  // Nothing to cut

  double parameterAtPoint = SweepAngleToPoint(point) / sweepAngle;

  if (parameterAtPoint <= Eo::geometricTolerance || parameterAtPoint >= 1.0 - Eo::geometricTolerance) { return; }
  // Cut point is not on or beyond endpoints

  double absoluteAngleAtPoint = m_startAngle + sweepAngle * parameterAtPoint;

  EoDbConic* newConic = new EoDbConic(*this);

  newConic->SetStartAngle(absoluteAngleAtPoint);

  group->AddTail(newConic);

  m_endAngle = absoluteAngleAtPoint;
}

void EoDbConic::Display(AeSysView* view, CDC* deviceContext) {
  if (view == nullptr || deviceContext == nullptr) { return; }

  // Skip degenerate arcs
  const double sweepAngle = SweepAngle();
  if (sweepAngle <= Eo::geometricTolerance) { return; }

  // Skip if major axis is degenerate
  if (m_majorAxis.Length() <= Eo::geometricTolerance) { return; }

  pstate.SetPen(view, deviceContext, LogicalColor(), LogicalLineType());

  polyline::BeginLineStrip();
  GenerateApproximationVertices(m_center, m_majorAxis);
  polyline::__End(view, deviceContext, LogicalLineType());
}

void EoDbConic::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_center);
}

void EoDbConic::GenerateApproximationVertices(EoGePoint3d center, EoGeVector3d majorAxis) const {
  auto minorAxis = MinorAxis();

  // Guard against degenerate geometry
  if (majorAxis.Length() <= Eo::geometricTolerance || minorAxis.Length() <= Eo::geometricTolerance) { return; }

  double sweepAngle = m_endAngle - m_startAngle;
  if (sweepAngle <= 0.0) { sweepAngle += Eo::TwoPi; }
  if (fabs(sweepAngle - Eo::TwoPi) <= Eo::geometricTolerance) { sweepAngle = Eo::TwoPi; }

  // For negative Z extrusion, OCS CCW appears as WCS CW when viewed from +Z.
  bool isFlippedOcs = m_extrusion.z < -Eo::geometricTolerance;
  double effectiveSweep = isFlippedOcs ? -sweepAngle : sweepAngle;

  // Calculate adaptive tessellation based on arc length and curvature
  double maxAxisLength = std::max(majorAxis.Length(), minorAxis.Length());
  int numberOfPoints = std::max(2, abs(Eo::Round(sweepAngle / Eo::TwoPi * 32.0)));
  numberOfPoints = std::min(128, std::max(numberOfPoints, abs(Eo::Round(sweepAngle * maxAxisLength / 0.250))));

  // Build OCS to WCS transformation
  EoGeTransformMatrix transformMatrix(center, majorAxis, minorAxis);
  transformMatrix.Inverse();

  // Pre-compute incremental rotation
  double segmentAngle = effectiveSweep / (numberOfPoints - 1);
  double angleCosine = cos(segmentAngle);
  double angleSine = sin(segmentAngle);

  // Generate vertices starting at parametric start angle
  EoGePoint3d point(cos(m_startAngle), sin(m_startAngle), 0.0);

  for (int i = 0; i < numberOfPoints; ++i) {
    polyline::SetVertex(transformMatrix * point);

    // Incremental rotation: [cos -sin; sin cos] * point
    double newX = point.x * angleCosine - point.y * angleSine;
    double newY = point.x * angleSine + point.y * angleCosine;
    point(newX, newY, 0.0);
  }
}

void EoDbConic::FormatGeometry(CString& geometry) {
  auto conicType = Subclass();
  geometry += L"Center;" + m_center.ToString();
  if (conicType == ConicType::Ellipse) { geometry += L"Major Axis;" + m_majorAxis.ToString(); }
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
  const auto minorAxis = MinorAxis();

  if (m_majorAxis.Length() <= Eo::geometricTolerance) { return m_center; }

  EoGeTransformMatrix transformMatrix(m_center, m_majorAxis, minorAxis);
  transformMatrix.Inverse();

  const EoGePoint3d point(cos(m_startAngle), sin(m_startAngle), 0.0);
  return transformMatrix * point;
}

EoGePoint3d EoDbConic::PointAtEndAngle() {
  const auto minorAxis = MinorAxis();

  if (m_majorAxis.Length() <= Eo::geometricTolerance) { return m_center; }

  EoGeTransformMatrix transformMatrix(m_center, m_majorAxis, minorAxis);
  transformMatrix.Inverse();

  const EoGePoint3d point(cos(m_endAngle), sin(m_endAngle), 0.0);

  return transformMatrix * point;
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
    if (fabs(dIntAng[iInts]) - SweepAngle() < 0.0) {  // Intersection lies on arc
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
  if (IsFullConic()) {
    // @todo handle full circle or ellipse
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
  if (m_ratio > 1.0) {
    // Minor axis is longer than major axis, swap them
    std::swap(m_majorAxis, minorAxis);
    m_ratio = 1.0 / m_ratio;
    // Angles may need to be adjusted, but this depends on transformation type
  }
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

  // Guard against degenerate case where point is at center
  if (EoGeVector3d(EoGePoint3d::kOrigin, endPoint).Length() < Eo::geometricTolerance) {
    return 0.0;  // Point at center, return start angle
  }
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
