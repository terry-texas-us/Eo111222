#include "stdafx.h"

#include "AeSys.h"
#include "AeSysView.h"
#include "EoDbEllipse.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGePolyline.h"
#include "PrimState.h"

#if defined(USING_ODA)
#include "ddeGItms.h"
#endif  // USING_ODA

EoDbEllipse::EoDbEllipse(const EoGePoint3d& centerPoint, const EoGeVector3d& majorAxis, const EoGeVector3d& minorAxis,
                         double sweepAngle)
    : m_ptCenter(centerPoint), m_vMajAx(majorAxis), m_vMinAx(minorAxis) {
  m_PenColor = pstate.PenColor();
  m_LineType = pstate.LineType();
  m_dSwpAng = sweepAngle;
}

EoDbEllipse::EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, double radius)
    : EoDbPrimitive(penColor, lineType), m_ptCenter(centerPoint) {
  AeSysView* ActiveView = AeSysView::GetActiveView();

  EoGeVector3d PlaneNormal = ActiveView->CameraDirection();

  m_vMinAx = ActiveView->ViewUp() * radius;
  m_vMajAx = m_vMinAx;
  m_vMajAx.RotAboutArbAx(PlaneNormal, -Eo::HalfPi);
  m_dSwpAng = Eo::TwoPi;
}
EoDbEllipse::EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, EoGeVector3d& planeNormal,
                         double radius)
    : EoDbPrimitive(penColor, lineType), m_ptCenter(centerPoint) {
  EoGeVector3d PlaneNormal(planeNormal);
  PlaneNormal.Normalize();
  m_vMajAx = ComputeArbitraryAxis(PlaneNormal);
  m_vMajAx.Normalize();
  m_vMajAx *= radius;
  m_vMinAx = m_vMajAx;
  EoGeTransformMatrix tm(centerPoint, PlaneNormal, Eo::HalfPi);
  m_vMinAx = tm * m_vMinAx;
  m_dSwpAng = Eo::TwoPi;
}
EoDbEllipse::EoDbEllipse(EoGePoint3d& centerPoint, EoGePoint3d& beginPoint) {
  AeSysView* ActiveView = AeSysView::GetActiveView();

  m_PenColor = pstate.PenColor();
  m_LineType = pstate.LineType();

  EoGeVector3d vPlnNorm = -ActiveView->CameraDirection();

  m_ptCenter = centerPoint;
  m_vMajAx = EoGeVector3d(centerPoint, beginPoint);
  m_vMinAx = m_vMajAx;
  m_vMinAx.RotAboutArbAx(vPlnNorm, Eo::HalfPi);
  m_dSwpAng = Eo::TwoPi;
}

EoDbEllipse::EoDbEllipse(EoGePoint3d beginPoint, EoGePoint3d intermediatePoint, EoGePoint3d endPoint) {
  m_PenColor = pstate.PenColor();
  m_LineType = pstate.LineType();

  m_dSwpAng = 0.;

  EoGeVector3d beginToIntermediate(beginPoint, intermediatePoint);
  EoGeVector3d beginToEnd(beginPoint, endPoint);
  EoGeVector3d planeNormal = EoGeCrossProduct(beginToIntermediate, beginToEnd);
  planeNormal.Normalize();

  // Build transformation matrix which will get int and end points to
  // z=0 plane with beg point as origin

  EoGeTransformMatrix transformMatrix(beginPoint, planeNormal);

  EoGePoint3d pt[3];

  pt[0] = beginPoint;
  pt[1] = intermediatePoint;
  pt[2] = endPoint;

  pt[1] = transformMatrix * pt[1];
  pt[2] = transformMatrix * pt[2];

  double determinant = (pt[1].x * pt[2].y - pt[2].x * pt[1].y);

  if (fabs(determinant) > DBL_EPSILON) {  // Three points are not colinear
    double dT = ((pt[2].x - pt[1].x) * pt[2].x + pt[2].y * (pt[2].y - pt[1].y)) / determinant;

    m_ptCenter.x = (pt[1].x - pt[1].y * dT) * 0.5;
    m_ptCenter.y = (pt[1].y + pt[1].x * dT) * 0.5;
    m_ptCenter.z = 0.;
    transformMatrix.Inverse();

    // Transform back to original plane
    m_ptCenter = transformMatrix * m_ptCenter;

    // None of the points coincide with center point

    transformMatrix = EoGeTransformMatrix(m_ptCenter, planeNormal);

    double dAng[3]{};

    pt[1] = intermediatePoint;
    pt[2] = endPoint;

    for (int i = 0; i < 3; i++) {  // Translate points into z=0 plane with center point at origin
      pt[i] = transformMatrix * pt[i];
      dAng[i] = atan2(pt[i].y, pt[i].x);
      if (dAng[i] < 0.0) dAng[i] += Eo::TwoPi;
    }
    double dMin = std::min(dAng[0], dAng[2]);
    double dMax = std::max(dAng[0], dAng[2]);

    if (fabs(dAng[1] - dMax) > DBL_EPSILON &&
        fabs(dAng[1] - dMin) > DBL_EPSILON) {  // Inside line is not colinear with outside lines
      m_dSwpAng = dMax - dMin;
      if (dAng[1] > dMin && dAng[1] < dMax) {
        if (dAng[0] == dMax) m_dSwpAng = -m_dSwpAng;
      } else {
        m_dSwpAng = Eo::TwoPi - m_dSwpAng;
        if (dAng[2] == dMax) m_dSwpAng = -m_dSwpAng;
      }
      EoGePoint3d ptRot = beginPoint.RotateAboutAxis(m_ptCenter, planeNormal, Eo::HalfPi);

      m_vMajAx = EoGeVector3d(m_ptCenter, beginPoint);
      m_vMinAx = EoGeVector3d(m_ptCenter, ptRot);
    }
  }
}
EoDbEllipse::EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, EoGeVector3d& majorAxis,
                         EoGeVector3d& minorAxis, double sweepAngle)
    : EoDbPrimitive(penColor, lineType), m_ptCenter(centerPoint), m_vMajAx(majorAxis), m_vMinAx(minorAxis) {
  m_dSwpAng = sweepAngle;
}
EoDbEllipse::EoDbEllipse(const EoDbEllipse& src) {
  m_PenColor = src.m_PenColor;
  m_LineType = src.m_LineType;
  m_ptCenter = src.m_ptCenter;
  m_vMajAx = src.m_vMajAx;
  m_vMinAx = src.m_vMinAx;
  m_dSwpAng = src.m_dSwpAng;
}
const EoDbEllipse& EoDbEllipse::operator=(const EoDbEllipse& src) {
  m_PenColor = src.m_PenColor;
  m_LineType = src.m_LineType;
  m_ptCenter = src.m_ptCenter;
  m_vMajAx = src.m_vMajAx;
  m_vMinAx = src.m_vMinAx;
  m_dSwpAng = src.m_dSwpAng;
  return (*this);
}
void EoDbEllipse::AddToTreeViewControl(HWND hTree, HTREEITEM hParent) {
  tvAddItem(hTree, hParent, const_cast<LPWSTR>(L"<Arc>"), this);
}
EoDbPrimitive*& EoDbEllipse::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbEllipse(*this);
  return (primitive);
}
void EoDbEllipse::CutAt2Pts(EoGePoint3d* pt, EoDbGroupList* groups, EoDbGroupList* newGroups) {
  EoDbEllipse* pArc;

  double dRel[2];

  dRel[0] = SwpAngToPt(pt[0]) / m_dSwpAng;
  dRel[1] = SwpAngToPt(pt[1]) / m_dSwpAng;

  if (dRel[0] <= DBL_EPSILON && dRel[1] >= 1. - DBL_EPSILON) {  // Put entire arc in trap
    pArc = this;
  } else {  // Something gets cut
    EoGeVector3d vPlnNorm = EoGeCrossProduct(m_vMajAx, m_vMinAx);
    vPlnNorm.Normalize();

    if (fabs(m_dSwpAng - Eo::TwoPi) <= DBL_EPSILON) {  // Closed arc
      m_dSwpAng = (dRel[1] - dRel[0]) * Eo::TwoPi;

      m_vMajAx.RotAboutArbAx(vPlnNorm, dRel[0] * Eo::TwoPi);
      m_vMinAx.RotAboutArbAx(vPlnNorm, dRel[0] * Eo::TwoPi);

      pArc = new EoDbEllipse(*this);

      m_vMajAx.RotAboutArbAx(vPlnNorm, m_dSwpAng);
      m_vMinAx.RotAboutArbAx(vPlnNorm, m_dSwpAng);

      m_dSwpAng = Eo::TwoPi - m_dSwpAng;
    } else {  // Arc section with a cut
      pArc = new EoDbEllipse(*this);
      double dSwpAng = m_dSwpAng;

      double dAng1 = dRel[0] * m_dSwpAng;
      double dAng2 = dRel[1] * m_dSwpAng;

      if (dRel[0] > DBL_EPSILON && dRel[1] < 1. - DBL_EPSILON) {  // Cut section out of middle
        pArc->SetSwpAng(dAng1);
        groups->AddTail(new EoDbGroup(pArc));

        m_vMajAx.RotAboutArbAx(vPlnNorm, dAng1);
        m_vMinAx.RotAboutArbAx(vPlnNorm, dAng1);
        m_dSwpAng = dAng2 - dAng1;

        pArc = new EoDbEllipse(*this);

        m_vMajAx.RotAboutArbAx(vPlnNorm, m_dSwpAng);
        m_vMinAx.RotAboutArbAx(vPlnNorm, m_dSwpAng);
        m_dSwpAng = dSwpAng - dAng2;
      } else if (dRel[1] < 1. - DBL_EPSILON) {  // Cut section in two and place begin section in trap
        pArc->SetSwpAng(dAng2);

        m_vMajAx.RotAboutArbAx(vPlnNorm, dAng2);
        m_vMinAx.RotAboutArbAx(vPlnNorm, dAng2);
        m_dSwpAng = dSwpAng - dAng2;
      } else {  // Cut section in two and place end section in trap
        m_dSwpAng = dAng1;

        EoGeVector3d v = m_vMajAx;
        v.RotAboutArbAx(vPlnNorm, dAng1);
        pArc->SetMajAx(v);
        v = m_vMinAx;
        v.RotAboutArbAx(vPlnNorm, dAng1);
        pArc->SetMinAx(v);
        pArc->SetSwpAng(dSwpAng - dAng1);
      }
    }
    groups->AddTail(new EoDbGroup(this));
  }
  newGroups->AddTail(new EoDbGroup(pArc));
}
void EoDbEllipse::CutAtPt(EoGePoint3d& pt, EoDbGroup* group) {
  if (fabs(m_dSwpAng - Eo::TwoPi) <= DBL_EPSILON)
    // Do not fragment a circle
    return;

  double dRel = SwpAngToPt(pt) / m_dSwpAng;

  if (dRel <= DBL_EPSILON || dRel >= 1. - DBL_EPSILON)
    // Nothing to cut
    return;

  double dSwpAng = m_dSwpAng * dRel;

  EoDbEllipse* pArc = new EoDbEllipse(*this);
  pArc->SetSwpAng(dSwpAng);
  group->AddTail(pArc);

  EoGeVector3d vPlnNorm = EoGeCrossProduct(m_vMajAx, m_vMinAx);
  vPlnNorm.Normalize();

  m_vMajAx.RotAboutArbAx(vPlnNorm, dSwpAng);
  m_vMinAx.RotAboutArbAx(vPlnNorm, dSwpAng);
  m_dSwpAng -= dSwpAng;
}
void EoDbEllipse::Display(AeSysView* view, CDC* deviceContext) {
  if (fabs(m_dSwpAng) <= DBL_EPSILON) return;

  EoInt16 nPenColor = LogicalPenColor();
  EoInt16 LineType = LogicalLineType();

  pstate.SetPen(view, deviceContext, nPenColor, LineType);

  polyline::BeginLineStrip();
  GenPts(m_ptCenter, m_vMajAx, m_vMinAx, m_dSwpAng);
  polyline::__End(view, deviceContext, LineType);
}
void EoDbEllipse::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str.Format(L"<Ellipse> Color: %s Line Type: %s SweepAngle %f MajorAxisLength: %f", FormatPenColor().GetString(),
             FormatLineType().GetString(), m_dSwpAng, m_vMajAx.Length());
  app.AddStringToMessageList(str);
}
void EoDbEllipse::GenPts(EoGePoint3d centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, double sweepAngle) {
  // Number of points based on angle and a smothness coefficient
  double dLen = std::max(majorAxis.Length(), minorAxis.Length());
  int iPts = std::max(2, abs(Eo::Round(sweepAngle / Eo::TwoPi * 32.0)));
  iPts = std::min(128, std::max(iPts, abs(Eo::Round(sweepAngle * dLen / 0.250))));

  EoGeTransformMatrix tm(centerPoint, majorAxis, minorAxis);
  tm.Inverse();

  double dAng = m_dSwpAng / (iPts - 1);
  double dCos = cos(dAng);
  double dSin = sin(dAng);

  EoGePoint3d pt(1.0, 0.0, 0.0);

  for (int i = 0; i < iPts; i++) {
    polyline::SetVertex(tm * pt);
    pt(pt.x * dCos - pt.y * dSin, pt.y * dCos + pt.x * dSin, 0.0);
  }
}
void EoDbEllipse::FormatGeometry(CString& str) {
  str += L"Center Point;" + m_ptCenter.ToString();
  str += L"Major Axis;" + m_vMajAx.ToString();
  str += L"Minor Axis;" + m_vMinAx.ToString();
  str += L"Plane Normal;" + (EoGeCrossProduct(m_vMajAx, m_vMinAx)).ToString();
}
void EoDbEllipse::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tStyle;%s\tSweep Angle;%f\tMajor Axis Length;%f", FormatPenColor().GetString(),
             FormatLineType().GetString(), m_dSwpAng, m_vMajAx.Length());
}
EoGePoint3d EoDbEllipse::GetBegPt() { return (m_ptCenter + m_vMajAx); }
void EoDbEllipse::GetBoundingBox(EoGePoint3dArray& ptsBox) {
  ptsBox.SetSize(4);
  ptsBox[0] = EoGePoint3d(-1.0, -1.0, 0.0);
  ptsBox[1] = EoGePoint3d(1.0, -1.0, 0.0);
  ptsBox[2] = EoGePoint3d(1.0, 1.0, 0.0);
  ptsBox[3] = EoGePoint3d(-1.0, 1.0, 0.0);

  if (m_dSwpAng < 3. * Eo::TwoPi / 4.0) {
    double dEndX = cos(m_dSwpAng);
    double dEndY = sin(m_dSwpAng);

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
  EoGeTransformMatrix tm(m_ptCenter, m_vMajAx, m_vMinAx);
  tm.Inverse();

  for (EoUInt16 w = 0; w < 4; w++) { ptsBox[w] = tm * ptsBox[w]; }
}
EoGePoint3d EoDbEllipse::GetEndPt() {
  EoGeTransformMatrix tm(m_ptCenter, m_vMajAx, m_vMinAx);
  tm.Inverse();

  EoGePoint3d pt(cos(m_dSwpAng), sin(m_dSwpAng), 0.0);

  pt = tm * pt;
  return (pt);
}
void EoDbEllipse::GetXYExtents(EoGePoint3d arBeg, EoGePoint3d arEnd, EoGePoint3d* arMin, EoGePoint3d* arMax) {
  double dx = double(m_ptCenter.x - arBeg.x);
  double dy = double(m_ptCenter.y - arBeg.y);

  double dRad = sqrt(dx * dx + dy * dy);

  (*arMin).x = m_ptCenter.x - dRad;
  (*arMin).y = m_ptCenter.y - dRad;
  (*arMax).x = m_ptCenter.x + dRad;
  (*arMax).y = m_ptCenter.y + dRad;

  if (arBeg.x >= m_ptCenter.x) {
    if (arBeg.y >= m_ptCenter.y) {  // Arc begins in quadrant one
      if (arEnd.x >= m_ptCenter.x) {
        if (arEnd.y >= m_ptCenter.y) {  // Arc ends in quadrant one
          if (arBeg.x > arEnd.x) {      // Arc in qraudrant one only
            (*arMin).x = arEnd.x;
            (*arMin).y = arBeg.y;
            (*arMax).x = arBeg.x;
            (*arMax).y = arEnd.y;
          }
        } else  // Arc ends in quadrant four
          (*arMax).x = std::max(arBeg.x, arEnd.x);
      } else {
        if (arEnd.y >= m_ptCenter.y) {  // Arc ends in quadrant two
          (*arMin).x = arEnd.x;
          (*arMin).y = std::min(arBeg.y, arEnd.y);
        } else  // Arc ends in quadrant three
          (*arMin).y = arEnd.y;
        (*arMax).x = arBeg.x;
      }
    } else {  // Arc begins in quadrant four
      if (arEnd.x >= m_ptCenter.x) {
        if (arEnd.y >= m_ptCenter.y) {  // Arc ends in quadrant one
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
        if (arEnd.y >= m_ptCenter.y) {  // Arc ends in quadrant two
          (*arMin).x = arEnd.x;
          (*arMin).y = arBeg.y;
        } else  // Arc ends in quadrant three
          (*arMin).y = std::min(arBeg.y, arEnd.y);
      }
    }
  } else {
    if (arBeg.y >= m_ptCenter.y) {  // Arc begins in quadrant two
      if (arEnd.x >= m_ptCenter.x) {
        if (arEnd.y >= m_ptCenter.y)  // Arc ends in quadrant one
          (*arMax).y = std::max(arBeg.y, arEnd.y);
        else {  // Arc ends in quadrant four
          (*arMax).x = arEnd.x;
          (*arMax).y = arBeg.y;
        }
      } else {
        if (arEnd.y >= m_ptCenter.y) {  // Arc ends in quadrant two
          if (arBeg.x > arEnd.x) {      // Arc in qraudrant two only
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
      if (arEnd.x >= m_ptCenter.x) {
        if (arEnd.y >= m_ptCenter.y)  // Arc ends in quadrant one
          (*arMax).y = arEnd.y;
        else {  // Arc ends in quadrant four
          (*arMax).x = arEnd.x;
          (*arMax).y = std::max(arBeg.y, arEnd.y);
        }
        (*arMin).x = arBeg.x;
      } else {
        if (arEnd.y >= m_ptCenter.y)  // Arc ends in quadrant two
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
  EoGeVector3d vPlnNorm = EoGeCrossProduct(m_vMajAx, m_vMinAx);
  vPlnNorm.Normalize();

  if (!(EoGeCrossProduct(EoGeVector3d::kZAxis, vPlnNorm)).IsNearNull())
    // not on plane normal to z-axis
    return 0;

  if (fabs(m_vMajAx.Length() - m_vMinAx.Length()) > FLT_EPSILON)
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
    m_vMajAx = EoGeVector3d(m_ptCenter, ptBeg);
    m_vMinAx = EoGeCrossProduct(vPlnNorm, m_vMajAx);
  }

  GetXYExtents(ptBeg, ptEnd, &ptMin, &ptMax);

  if (ptMin.x >= ptLL.x && ptMax.x <= ptUR.x && ptMin.y >= ptLL.y &&
      ptMax.y <= ptUR.y) {  // Totally within window boundaries
    ptInt[0] = ptBeg;
    ptInt[1] = ptEnd;
    return (2);
  }
  if (ptMin.x >= ptUR.x || ptMax.x <= ptLL.x || ptMin.y >= ptUR.y || ptMax.y <= ptLL.y)
    // No extent overlap
    return 0;

  EoGePoint3d ptWrk[8];

  double dDis;
  double dOff;
  int iSecs = 0;

  double dRad = EoGeVector3d(m_ptCenter, ptBeg).Length();
  if (ptMax.x > ptUR.x) {  // Arc may intersect with right window boundary
    dDis = ptUR.x - m_ptCenter.x;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_ptCenter.y - dOff >= ptLL.y && m_ptCenter.y - dOff <= ptUR.y) {
      ptWrk[iSecs].x = ptUR.x;
      ptWrk[iSecs++].y = m_ptCenter.y - dOff;
    }
    if (m_ptCenter.y + dOff <= ptUR.y && m_ptCenter.y + dOff >= ptLL.y) {
      ptWrk[iSecs].x = ptUR.x;
      ptWrk[iSecs++].y = m_ptCenter.y + dOff;
    }
  }
  if (ptMax.y > ptUR.y) {  // Arc may intersect with top window boundary
    dDis = ptUR.y - m_ptCenter.y;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_ptCenter.x + dOff <= ptUR.x && m_ptCenter.x + dOff >= ptLL.x) {
      ptWrk[iSecs].x = m_ptCenter.x + dOff;
      ptWrk[iSecs++].y = ptUR.y;
    }
    if (m_ptCenter.x - dOff >= ptLL.x && m_ptCenter.x - dOff <= ptUR.x) {
      ptWrk[iSecs].x = m_ptCenter.x - dOff;
      ptWrk[iSecs++].y = ptUR.y;
    }
  }
  if (ptMin.x < ptLL.x) {  // Arc may intersect with left window boundary
    dDis = m_ptCenter.x - ptLL.x;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_ptCenter.y + dOff <= ptUR.y && m_ptCenter.y + dOff >= ptLL.y) {
      ptWrk[iSecs].x = ptLL.x;
      ptWrk[iSecs++].y = m_ptCenter.y + dOff;
    }
    if (m_ptCenter.y - dOff >= ptLL.y && m_ptCenter.y - dOff <= ptUR.y) {
      ptWrk[iSecs].x = ptLL.x;
      ptWrk[iSecs++].y = m_ptCenter.y - dOff;
    }
  }
  if (ptMin.y < ptLL.y) {  // Arc may intersect with bottom window boundary
    dDis = m_ptCenter.y - ptLL.y;
    dOff = sqrt(dRad * dRad - dDis * dDis);
    if (m_ptCenter.x - dOff >= ptLL.x && m_ptCenter.x - dOff <= ptUR.x) {
      ptWrk[iSecs].x = m_ptCenter.x - dOff;
      ptWrk[iSecs++].y = ptLL.y;
    }
    if (m_ptCenter.x + dOff <= ptUR.x && m_ptCenter.x + dOff >= ptLL.x) {
      ptWrk[iSecs].x = m_ptCenter.x + dOff;
      ptWrk[iSecs++].y = ptLL.y;
    }
  }
  if (iSecs == 0) return 0;

  double dBegAng = atan2(ptBeg.y - m_ptCenter.y, ptBeg.x - m_ptCenter.x);  // Arc begin angle (- pi to pi)

  double dIntAng[8];
  double dWrkAng;
  int iInts = 0;
  for (int i2 = 0; i2 < iSecs; i2++) {                                        // Loop thru possible intersections
    dWrkAng = atan2(ptWrk[i2].y - m_ptCenter.y, ptWrk[i2].x - m_ptCenter.x);  // Current intersection angle (- pi to
    dIntAng[iInts] = dWrkAng - dBegAng;                                       // Sweep from begin to intersection
    if (dIntAng[iInts] < 0.0) dIntAng[iInts] += Eo::TwoPi;
    if (fabs(dIntAng[iInts]) - m_dSwpAng < 0.0) {  // Intersection lies on arc
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
  if (fabs(m_dSwpAng - Eo::TwoPi) <= DBL_EPSILON) {  // Arc is a circle in disuise

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
  double dAng = (sm_RelationshipOfPoint <= DBL_EPSILON) ? m_dSwpAng : 0.;
  return (pFndPtOnArc(m_ptCenter, m_vMajAx, m_vMinAx, dAng));
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
  GenPts(m_ptCenter, m_vMajAx, m_vMinAx, m_dSwpAng);
  return polyline::SelectUsingLine(view, line, ptsInt);
}
bool EoDbEllipse::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  polyline::BeginLineStrip();
  GenPts(m_ptCenter, m_vMajAx, m_vMinAx, m_dSwpAng);
  return (polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, ptProj));
}
bool EoDbEllipse::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  polyline::BeginLineStrip();
  GenPts(m_ptCenter, m_vMajAx, m_vMinAx, m_dSwpAng);
  return polyline::SelectUsingRectangle(view, pt1, pt2);
}
double EoDbEllipse::SwpAngToPt(EoGePoint3d pt) {
  EoGeVector3d vPlnNorm = EoGeCrossProduct(m_vMajAx, m_vMinAx);
  vPlnNorm.Normalize();

  EoGeTransformMatrix tm(m_ptCenter, vPlnNorm);

  EoGePoint3d ptBeg = m_ptCenter + m_vMajAx;
  EoGePoint3d ptEnd = pt;

  // Translate points into z=0 plane
  ptBeg = tm * ptBeg;
  ptEnd = tm * ptEnd;

  return (EoGeLine::AngleBetweenLn_xy(EoGeLine(EoGePoint3d::kOrigin, ptBeg), EoGeLine(EoGePoint3d::kOrigin, ptEnd)));
}

void EoDbEllipse::Transform(EoGeTransformMatrix& tm) {
  m_ptCenter = tm * m_ptCenter;
  m_vMajAx = tm * m_vMajAx;
  m_vMinAx = tm * m_vMinAx;
}

void EoDbEllipse::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) m_ptCenter += v;
}

bool EoDbEllipse::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kEllipsePrimitive));
  EoDb::Write(file, m_PenColor);
  EoDb::Write(file, m_LineType);
  m_ptCenter.Write(file);
  m_vMajAx.Write(file);
  m_vMinAx.Write(file);
  EoDb::Write(file, m_dSwpAng);

  return true;
}
EoGePoint3d pFndPtOnArc(EoGePoint3d centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, const double dAng) {
  EoGeTransformMatrix tm(centerPoint, majorAxis, minorAxis);
  tm.Inverse();

  EoGePoint3d pt(cos(dAng), sin(dAng), 0.0);

  pt = tm * pt;
  return (pt);
}
bool pFndCPGivRadAnd4Pts(double radius, EoGePoint3d arLn1Beg, EoGePoint3d arLn1End, EoGePoint3d arLn2Beg,
                         EoGePoint3d arLn2End, EoGePoint3d* centerPoint) {
  double dA1, dA2, dB1, dB2, dC1RAB1, dC2RAB2, dDet, dSgnRad, dV1Mag, dV2Mag;
  EoGeVector3d vPlnNorm;

  EoGeVector3d v1(arLn1Beg, arLn1End);  // Determine vector defined by endpoints of first line
  dV1Mag = v1.Length();
  if (dV1Mag <= DBL_EPSILON) return false;

  EoGeVector3d v2(arLn2Beg, arLn2End);
  dV2Mag = v2.Length();
  if (dV2Mag <= DBL_EPSILON) return false;

  vPlnNorm = EoGeCrossProduct(v1, v2);  // Determine vector normal to tangent vectors
  vPlnNorm.Normalize();
  if (vPlnNorm.IsNearNull()) return false;

  if (fabs((EoGeDotProduct(vPlnNorm, EoGeVector3d(arLn1Beg, arLn2Beg)))) > DBL_EPSILON)  // Four points are not coplanar
    return false;

  EoGeTransformMatrix tm(arLn1Beg, vPlnNorm);

  arLn1End = tm * arLn1End;
  arLn2Beg = tm * arLn2Beg;
  arLn2End = tm * arLn2End;
  dA1 = -arLn1End.y / dV1Mag;
  dB1 = arLn1End.x / dV1Mag;
  v2.x = arLn2End.x - arLn2Beg.x;
  v2.y = arLn2End.y - arLn2Beg.y;
  dA2 = -v2.y / dV2Mag;
  dB2 = v2.x / dV2Mag;
  dDet = dA2 * dB1 - dA1 * dB2;

  dSgnRad = (arLn1End.x * arLn2End.y - arLn2End.x * arLn1End.y) >= 0. ? -fabs(radius) : fabs(radius);

  dC1RAB1 = dSgnRad;
  dC2RAB2 = (arLn2Beg.x * arLn2End.y - arLn2End.x * arLn2Beg.y) / dV2Mag + dSgnRad;
  (*centerPoint).x = (dB2 * dC1RAB1 - dB1 * dC2RAB2) / dDet;
  (*centerPoint).y = (dA1 * dC2RAB2 - dA2 * dC1RAB1) / dDet;
  (*centerPoint).z = 0.;
  tm.Inverse();
  *centerPoint = tm * (*centerPoint);
  return true;
}
int pFndSwpAngGivPlnAnd3Lns(EoGeVector3d planeNormal, EoGePoint3d arP1, EoGePoint3d arP2, EoGePoint3d arP3,
                            EoGePoint3d& centerPoint, double* adTheta) {
  double dT[3];
  EoGePoint3d rR[3];

  if (arP1 == centerPoint || arP2 == centerPoint || arP3 == centerPoint) return (FALSE);

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
  if (fabs(dT[1] - dTMax) > DBL_EPSILON &&
      fabs(dT[1] - dTMin) > DBL_EPSILON) {  // Inside line is not colinear with outside lines
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
