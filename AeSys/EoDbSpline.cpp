#include "Stdafx.h"

#include <climits>

#include "AeSys.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoDbSpline.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"
#include "Resource.h"

EoDbSpline::EoDbSpline(EoUInt16 wPts, EoGePoint3d* pt) {
  m_color = pstate.PenColor();
  m_lineTypeIndex = pstate.LineType();

  for (EoUInt16 w = 0; w < wPts; w++) m_pts.Add(pt[w]);
}
EoDbSpline::EoDbSpline(EoGePoint3dArray& points) {
  m_color = pstate.PenColor();
  m_lineTypeIndex = pstate.LineType();
  m_pts.Copy(points);
}
EoDbSpline::EoDbSpline(EoInt16 penColor, EoInt16 lineType, EoGePoint3dArray& points) {
  m_color = penColor;
  m_lineTypeIndex = lineType;
  m_pts.Copy(points);
}
EoDbSpline::EoDbSpline(const EoDbSpline& src) {
  m_color = src.m_color;
  m_lineTypeIndex = src.m_lineTypeIndex;
  m_pts.Copy(src.m_pts);
}

const EoDbSpline& EoDbSpline::operator=(const EoDbSpline& src) {
  m_color = src.m_color;
  m_lineTypeIndex = src.m_lineTypeIndex;
  m_pts.Copy(src.m_pts);

  return (*this);
}

void EoDbSpline::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<BSpline>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbSpline::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbSpline(*this);
  return (primitive);
}

void EoDbSpline::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 color = LogicalColor();
  EoInt16 lineType = LogicalLineType();

  pstate.SetPen(view, deviceContext, color, lineType);

  polyline::BeginLineStrip();
  GenPts(3, m_pts);
  polyline::__End(view, deviceContext, lineType);
}
void EoDbSpline::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str.Format(L"<BSpline> Color: %s Line Type: %s", FormatPenColor().GetString(), FormatLineType().GetString());
  app.AddStringToMessageList(str);
}
void EoDbSpline::FormatGeometry(CString& str) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) { str += L"Control Point;" + m_pts[w].ToString(); }
}

void EoDbSpline::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tControl Points;%d", FormatPenColor().GetString(), FormatLineType().GetString(),
             static_cast<int>(m_pts.GetSize()));
}

void EoDbSpline::GetAllPoints(EoGePoint3dArray& pts) {
  pts.SetSize(0);
  pts.Copy(m_pts);
}

EoGePoint3d EoDbSpline::GetControlPoint() {
  EoGePoint3d point;
  point = m_pts[m_pts.GetSize() / 2];
  return (point);
}

void EoDbSpline::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3d pt;

  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) {
    pt = m_pts[w];
    view->ModelTransformPoint(pt);
    pt = tm * pt;
    ptMin = EoGePoint3d::Min(ptMin, pt);
    ptMax = EoGePoint3d::Max(ptMax, pt);
  }
}
EoGePoint3d EoDbSpline::GoToNextControlPoint() {
  EoGePoint3d pt;

  INT_PTR i = m_pts.GetSize() - 1;

  if (sm_RelationshipOfPoint < Eo::geometricTolerance)
    pt = m_pts[i];
  else if (sm_RelationshipOfPoint >= 1.0 - Eo::geometricTolerance)
    pt = m_pts[0];
  else if (m_pts[i].x > m_pts[0].x)
    pt = m_pts[0];
  else if (m_pts[i].x < m_pts[0].x)
    pt = m_pts[i];
  else if (m_pts[i].y > m_pts[0].y)
    pt = m_pts[0];
  else
    pt = m_pts[i];
  return (pt);
}
bool EoDbSpline::IsInView(AeSysView* view) {
  EoGePoint4d pt[2]{};

  pt[0] = m_pts[0];

  view->ModelViewTransformPoint(pt[0]);

  for (EoUInt16 w = 1; w < m_pts.GetSize(); w++) {
    pt[1] = m_pts[w];

    view->ModelViewTransformPoint(pt[1]);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) return true;

    pt[0] = pt[1];
  }
  return false;
}

bool EoDbSpline::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  (void)view;
  (void)point;
  return false;
}

EoGePoint3d EoDbSpline::SelectAtControlPoint(AeSysView*, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;
  return (point);
}

bool EoDbSpline::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) {
  (void)view;
  (void)line;
  return false;
}

bool EoDbSpline::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  polyline::BeginLineStrip();

  GenPts(3, m_pts);

  return (polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, ptProj));
}
bool EoDbSpline::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  return polyline::SelectUsingRectangle(view, pt1, pt2, m_pts);
}
void EoDbSpline::Transform(EoGeTransformMatrix& tm) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) m_pts[w] = tm * m_pts[w];
}

void EoDbSpline::Translate(EoGeVector3d v) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) m_pts[w] += v;
}

void EoDbSpline::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++)
    if (((mask >> w) & 1UL) == 1) m_pts[w] += v;
}

bool EoDbSpline::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kSplinePrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  EoDb::Write(file, EoUInt16(m_pts.GetSize()));

  for (EoUInt16 w = 0; w < m_pts.GetSize(); w++) m_pts[w].Write(file);

  return true;
}

int EoDbSpline::GenPts(const int iOrder, EoGePoint3dArray& pts) {
  double* dKnot = new double[65 * 66];
  if (dKnot == 0) {
    app.WarningMessageBox(IDS_MSG_MEM_ALLOC_ERR);
    return 0;
  }
  int iPts = 8 * static_cast<int>(pts.GetSize());
  double* dWght = &dKnot[65];

  int i, i2, i4;

  int iTMax = (static_cast<int>(pts.GetSize()) - 1) - iOrder + 2;
  int iKnotVecMax = (static_cast<int>(pts.GetSize()) - 1) + iOrder;  // Maximum number of dKnot vectors

  for (i = 0; i < 65 * 65; i++)  // Set weighting value array with zeros
    dWght[i] = 0.;

  for (i = 0; i <= iKnotVecMax; i++) {  // Determine dKnot vectors
    if (i <= iOrder - 1)                // Beginning of curve
      dKnot[i] = 0.;
    else if (i >= iTMax + iOrder)  // End of curve
      dKnot[i] = dKnot[i - 1];
    else {
      i2 = i - iOrder;
      if (pts[i2] == pts[i2 + 1])  // Repeating vertices
        dKnot[i] = dKnot[i - 1];
      else  // Successive internal vectors
        dKnot[i] = dKnot[i - 1] + 1.;
    }
  }
  if (dKnot[iKnotVecMax] != 0.0) {
    double G = 0.;
    double H = 0.;
    double Z = 0.;
    double T, W1, W2;
    double dStep = dKnot[iKnotVecMax] / (double)(iPts - 1);
    int iPts2 = 0;
    for (i4 = iOrder - 1; i4 <= iOrder + iTMax; i4++) {
      for (i = 0; i <= iKnotVecMax - 1; i++) {  // Calculate values for weighting value
        if (i != i4 || dKnot[i] == dKnot[i + 1])
          dWght[65 * i + 1] = 0.;
        else
          dWght[65 * i + 1] = 1.;
      }
      for (T = dKnot[i4]; T <= dKnot[i4 + 1] - dStep; T += dStep) {
        iPts2++;
        for (i2 = 2; i2 <= iOrder; i2++) {
          for (i = 0; i <= pts.GetSize() - 1; i++) {  // Determine first term of weighting function equation
            if (dWght[65 * i + i2 - 1] == 0.0)
              W1 = 0.;
            else
              W1 = ((T - dKnot[i]) * dWght[65 * i + i2 - 1]) / (dKnot[i + i2 - 1] - dKnot[i]);

            if (dWght[65 * (i + 1) + i2 - 1] == 0.0)  // Determine second term of weighting function equation
              W2 = 0.;
            else
              W2 = ((dKnot[i + i2] - T) * dWght[65 * (i + 1) + i2 - 1]) / (dKnot[i + i2] - dKnot[i + 1]);

            dWght[65 * i + i2] = W1 + W2;
            G = pts[i].x * dWght[65 * i + i2] + G;
            H = pts[i].y * dWght[65 * i + i2] + H;
            Z = pts[i].z * dWght[65 * i + i2] + Z;
          }
          if (i2 == iOrder) break;
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
  } else {  // either iOrder greater than number of control points or all control points coincidental
    iPts = 2;
    polyline::SetVertex(pts[0]);
  }
  polyline::SetVertex(pts[pts.GetUpperBound()]);

  delete[] dKnot;

  return (iPts);
}
