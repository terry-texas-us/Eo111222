#include "Stdafx.h"

#include <climits>
#include <vector>

#include "AeSys.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoDbSpline.h"
#include "EoDxfInterface.h"
#include "EoDxfSpline.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderState.h"

EoDbSpline::EoDbSpline(std::uint16_t wPts, EoGePoint3d* pt) {
  m_color = renderState.Color();
  m_lineTypeIndex = renderState.LineTypeIndex();

  for (auto i = 0; i < wPts; i++) { m_pts.Add(pt[i]); }
}
EoDbSpline::EoDbSpline(EoGePoint3dArray& points) {
  m_color = renderState.Color();
  m_lineTypeIndex = renderState.LineTypeIndex();
  m_pts.Copy(points);
}
EoDbSpline::EoDbSpline(std::int16_t penColor, std::int16_t lineType, EoGePoint3dArray& points) {
  m_color = penColor;
  m_lineTypeIndex = lineType;
  m_pts.Copy(points);
}
EoDbSpline::EoDbSpline(const EoDbSpline& src) : EoDbPrimitive(src) {
  m_degree = src.m_degree;
  m_flags = src.m_flags;
  m_pts.Copy(src.m_pts);
  m_knots = src.m_knots;
  m_weights = src.m_weights;
}

EoDbSpline& EoDbSpline::operator=(const EoDbSpline& src) {
  if (this != &src) {
    EoDbPrimitive::operator=(src);
    m_degree = src.m_degree;
    m_flags = src.m_flags;
    m_pts.Copy(src.m_pts);
    m_knots = src.m_knots;
    m_weights = src.m_weights;
  }
  return *this;
}

void EoDbSpline::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<BSpline>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbSpline::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbSpline(*this);
  return primitive;
}

void EoDbSpline::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  std::int16_t color = LogicalColor();
  std::int16_t lineType = LogicalLineType();
  const auto& lineTypeName = LogicalLineTypeName();

  renderState.SetPen(view, renderDevice, color, lineType, lineTypeName, m_lineWeight, m_lineTypeScale);

  polyline::BeginLineStrip();
  GenPts(static_cast<std::int16_t>(m_degree + 1), m_pts);
  polyline::End(view, renderDevice, lineType, lineTypeName);
}

void EoDbSpline::ExportToDxf(EoDxfInterface* writer) const {
  const auto numberOfControlPoints = static_cast<std::int16_t>(m_pts.GetSize());
  if (numberOfControlPoints == 0) { return; }

  EoDxfSpline spline;
  PopulateDxfBaseProperties(&spline);

  const std::int16_t degree = m_degree;
  const std::int16_t order = static_cast<std::int16_t>(degree + 1);
  spline.m_degreeOfTheSplineCurve = degree;
  spline.m_splineFlag = m_flags != 0 ? m_flags : static_cast<std::int16_t>(0x08);  // preserve flags; default planar
  spline.m_numberOfControlPoints = numberOfControlPoints;

  // Use stored knot vector when available; otherwise generate a uniform clamped knot vector.
  if (!m_knots.empty()) {
    spline.m_knotValues = m_knots;
    spline.m_numberOfKnots = static_cast<std::int16_t>(m_knots.size());
  } else {
    const std::int16_t numberOfKnots = static_cast<std::int16_t>(numberOfControlPoints + order);
    spline.m_numberOfKnots = numberOfKnots;
    spline.m_knotValues.reserve(static_cast<size_t>(numberOfKnots));
    for (std::int16_t i = 0; i < numberOfKnots; ++i) {
      if (i < order) {
        spline.m_knotValues.push_back(0.0);
      } else if (i > numberOfControlPoints) {
        spline.m_knotValues.push_back(static_cast<double>(numberOfControlPoints - degree));
      } else {
        spline.m_knotValues.push_back(static_cast<double>(i - degree));
      }
    }
  }

  // Export weights for rational splines (NURBS).
  if (!m_weights.empty()) {
    spline.m_weightValues = m_weights;
  }

  spline.m_numberOfFitPoints = 0;

  for (INT_PTR i = 0; i < numberOfControlPoints; ++i) {
    auto* controlPoint = new EoDxfGeometryBase3d();
    controlPoint->x = m_pts[i].x;
    controlPoint->y = m_pts[i].y;
    controlPoint->z = m_pts[i].z;
    spline.m_controlPoints.push_back(controlPoint);
  }

  writer->AddSpline(spline);
}

void EoDbSpline::AddReportToMessageList(const EoGePoint3d& point) {
  app.AddStringToMessageList(CString(L"<Spline>"));
  EoDbPrimitive::AddReportToMessageList(point);

  CString detailLine;
  detailLine.Format(L"  Degree: %d  Flags: 0x%04X  Control Points: %d  Knots: %zu  Weights: %zu",
      static_cast<int>(m_degree), static_cast<int>(m_flags), static_cast<int>(m_pts.GetSize()), m_knots.size(),
      m_weights.size());
  app.AddStringToMessageList(detailLine);
}

void EoDbSpline::FormatGeometry(CString& str) {
  for (auto i = 0; i < m_pts.GetSize(); i++) { str += L"Control Point;" + m_pts[i].ToString(); }
}

void EoDbSpline::FormatExtra(CString& str) {
  EoDbPrimitive::FormatExtra(str);
  str.AppendFormat(L"\tDegree;%d\tFlags;0x%04X\tControl Points;%d\tKnots;%zu\tWeights;%zu",
      static_cast<int>(m_degree), static_cast<int>(m_flags), static_cast<int>(m_pts.GetSize()), m_knots.size(),
      m_weights.size());
  str += L'\t';
}

void EoDbSpline::GetAllPoints(EoGePoint3dArray& pts) {
  pts.SetSize(0);
  pts.Copy(m_pts);
}

EoGePoint3d EoDbSpline::GetControlPoint() {
  EoGePoint3d point;
  point = m_pts[m_pts.GetSize() / 2];
  return point;
}

void EoDbSpline::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3d pt;

  for (auto i = 0; i < m_pts.GetSize(); i++) {
    pt = m_pts[i];
    view->ModelTransformPoint(pt);
    pt = transformMatrix * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}
EoGePoint3d EoDbSpline::GoToNextControlPoint() {
  EoGePoint3d point;

  auto i = m_pts.GetSize() - 1;

  if (sm_RelationshipOfPoint < Eo::geometricTolerance) {
    point = m_pts[i];
  } else if (sm_RelationshipOfPoint >= 1.0 - Eo::geometricTolerance) {
    point = m_pts[0];
  } else if (m_pts[i].x > m_pts[0].x) {
    point = m_pts[0];
  } else if (m_pts[i].x < m_pts[0].x) {
    point = m_pts[i];
  } else if (m_pts[i].y > m_pts[0].y) {
    point = m_pts[0];
  } else {
    point = m_pts[i];
  }
  return point;
}

bool EoDbSpline::IsInView(AeSysView* view) {
  EoGePoint4d ndcPoints[2]{};

  ndcPoints[0] = EoGePoint4d{m_pts[0]};

  view->ModelViewTransformPoint(ndcPoints[0]);
  for (std::uint16_t w = 1; w < m_pts.GetSize(); w++) {
    ndcPoints[1] = EoGePoint4d{m_pts[w]};

    view->ModelViewTransformPoint(ndcPoints[1]);
    if (EoGePoint4d::ClipLine(ndcPoints[0], ndcPoints[1])) { return true; }

    ndcPoints[0] = ndcPoints[1];
  }
  return false;
}

bool EoDbSpline::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  (void)view;
  (void)point;
  return false;
}

EoGePoint3d EoDbSpline::SelectAtControlPoint(AeSysView*, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;
  return EoGePoint3d{point};
}

bool EoDbSpline::SelectUsingLine(
    [[maybe_unused]] AeSysView* view, [[maybe_unused]] EoGeLine line, [[maybe_unused]] EoGePoint3dArray&) {
  return false;
}

bool EoDbSpline::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  polyline::BeginLineStrip();

  GenPts(static_cast<std::int16_t>(m_degree + 1), m_pts);

  return (polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, ptProj));
}
bool EoDbSpline::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  return polyline::SelectUsingRectangle(view, pt1, pt2, m_pts);
}
void EoDbSpline::Transform(const EoGeTransformMatrix& transformMatrix) {
  for (auto i = 0; i < m_pts.GetSize(); i++) { m_pts[i] = transformMatrix * m_pts[i]; }
}

void EoDbSpline::Translate(const EoGeVector3d& v) {
  for (auto i = 0; i < m_pts.GetSize(); i++) { m_pts[i] += v; }
}

void EoDbSpline::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  for (auto i = 0; i < m_pts.GetSize(); i++) {
    if (((mask >> i) & 1UL) == 1) { m_pts[i] += v; }
  }
}

EoDbSpline* EoDbSpline::ReadFromPeg(CFile& file) {
  auto penColor = EoDb::ReadInt16(file);
  auto lineType = EoDb::ReadInt16(file);
  auto numberOfPoints = EoDb::ReadUInt16(file);

  EoGePoint3dArray points;
  points.SetSize(numberOfPoints);
  for (std::uint16_t n = 0; n < numberOfPoints; n++) { points[n] = EoDb::ReadPoint3d(file); }
  return new EoDbSpline(penColor, lineType, points);
}

bool EoDbSpline::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kSplinePrimitive));
  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, m_lineTypeIndex);
  EoDb::WriteUInt16(file, std::uint16_t(m_pts.GetSize()));

  for (auto i = 0; i < m_pts.GetSize(); i++) { m_pts[i].Write(file); }

  return true;
}

int EoDbSpline::GenPts(const std::int16_t order, const EoGePoint3dArray& controlPoints) {
  const auto numberOfControlPoints = static_cast<int>(controlPoints.GetSize());
  if (numberOfControlPoints < 2) {
    if (numberOfControlPoints == 1) {
      polyline::SetVertex(controlPoints[0]);
      polyline::SetVertex(controlPoints[0]);
    }
    return numberOfControlPoints < 1 ? 0 : 2;
  }

  int iPts = 8 * numberOfControlPoints;

  int i, i2, i4;

  int iTMax = (numberOfControlPoints - 1) - order + 2;
  int iKnotVecMax = (numberOfControlPoints - 1) + order;  // Maximum number of knot vectors

  // Dynamic allocation sized to actual control point count.
  // Original used fixed 65×66 array which overflowed for > ~64 control points.
  const int stride = order + 1;
  std::vector<double> knotStorage(static_cast<std::size_t>(iKnotVecMax) + 1, 0.0);
  std::vector<double> weightStorage(
      static_cast<std::size_t>(stride) * (static_cast<std::size_t>(iKnotVecMax) + 1), 0.0);
  double* dKnot = knotStorage.data();
  double* dWght = weightStorage.data();

  for (i = 0; i <= iKnotVecMax; i++) {  // Determine knot vectors
    if (i <= order - 1) {  // Beginning of curve
      dKnot[i] = 0.;
    } else if (i >= iTMax + order) {  // End of curve
      dKnot[i] = dKnot[i - 1];
    } else {
      i2 = i - order;
      if (controlPoints[i2] == controlPoints[i2 + 1]) {  // Repeating vertices
        dKnot[i] = dKnot[i - 1];
      } else {  // Successive internal vectors
        dKnot[i] = dKnot[i - 1] + 1.;
      }
    }
  }
  if (dKnot[iKnotVecMax] != 0.0) {
    double G = 0.;
    double H = 0.;
    double Z = 0.;
    double T, W1, W2;
    double dStep = dKnot[iKnotVecMax] / (double)(iPts - 1);
    int iPts2 = 0;
    for (i4 = order - 1; i4 <= order + iTMax; i4++) {
      for (i = 0; i <= iKnotVecMax - 1; i++) {  // Calculate values for weighting value
        if (i != i4 || dKnot[i] == dKnot[i + 1]) {
          dWght[stride * i + 1] = 0.;
        } else {
          dWght[stride * i + 1] = 1.;
        }
      }
      for (T = dKnot[i4]; T <= dKnot[i4 + 1] - dStep; T += dStep) {
        iPts2++;
        for (i2 = 2; i2 <= order; i2++) {
          for (i = 0; i <= numberOfControlPoints - 1; i++) {  // Determine first term of weighting function equation
            if (dWght[stride * i + i2 - 1] == 0.0) {
              W1 = 0.;
            } else {
              W1 = ((T - dKnot[i]) * dWght[stride * i + i2 - 1]) / (dKnot[i + i2 - 1] - dKnot[i]);
            }

            if (dWght[stride * (i + 1) + i2 - 1] == 0.0) {  // Determine second term of weighting function equation
              W2 = 0.;
            } else {
              W2 = ((dKnot[i + i2] - T) * dWght[stride * (i + 1) + i2 - 1]) / (dKnot[i + i2] - dKnot[i + 1]);
            }

            dWght[stride * i + i2] = W1 + W2;
            G = controlPoints[i].x * dWght[stride * i + i2] + G;
            H = controlPoints[i].y * dWght[stride * i + i2] + H;
            Z = controlPoints[i].z * dWght[stride * i + i2] + Z;
          }
          if (i2 == order) { break; }
          G = 0.;
          H = 0.;
          Z = 0.;
        }
        polyline::SetVertex(EoGePoint3d(G, H, Z));
        G = 0.;
        H = 0.;
        Z = 0.;
      }
    }
    iPts = iPts2 + 1;
  } else {  // either order greater than number of control points or all control points coincidental
    iPts = 2;
    polyline::SetVertex(controlPoints[0]);
  }
  polyline::SetVertex(controlPoints[controlPoints.GetUpperBound()]);

  return iPts;
}
