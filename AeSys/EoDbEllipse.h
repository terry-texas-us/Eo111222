#pragma once
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>
#include <utility>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "drw_base.h"

class EoDbEllipse : public EoDbPrimitive {
 private:
  EoGePoint3d m_centerPoint;
  EoGeVector3d m_majorAxis;
  EoGeVector3d m_minorAxis;
  double m_sweepAngle;

 public:
  EoDbEllipse() : m_centerPoint{}, m_majorAxis{}, m_minorAxis{}, m_sweepAngle{0.0} {}

  /// <summary>Ellipse segment is constructed using a center point, a major and minor vector and a sweep ang.</summary>
  EoDbEllipse(const EoGePoint3d& centerPoint, const EoGeVector3d& majorAxis, const EoGeVector3d& minorAxis, double sweepAngle);

  /// <summary>Ellipse segment is constructed using a center point, a major and minor vector and a sweep ang.</summary>
  EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, EoGeVector3d& majorAxis, EoGeVector3d& minorAxis, double sweepAngle);

  /// <summary>Ellipse is constructed using a center point and a radius about view plane normal</summary>
  EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, double radius);

  /// <summary>Ellipse is constructed using a center point, plane normal and a radius</summary>
  EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, EoGeVector3d& planeNormal, double radius);

  /// <summary>Circle is constructed using a center point and a begin point about view plane normal.</summary>
  EoDbEllipse(EoGePoint3d& centerPoint, EoGePoint3d& beginPoint);

  EoDbEllipse(EoGePoint3d beginPoint, EoGePoint3d intermediatePoint, EoGePoint3d endPoint);

  EoDbEllipse(const DRW_Coord& centerPoint, double radius, double startAngle, double endAngle);

  EoDbEllipse(const EoDbEllipse&);

  ~EoDbEllipse() override {}

 public:
  const EoDbEllipse& operator=(const EoDbEllipse&);

 public: // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbEllipse*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void GetAllPts(EoGePoint3dArray& pts) override {
    pts.SetSize(0);
    pts.Add(m_centerPoint);
  }
  EoGePoint3d GetCtrlPt() override { return (m_centerPoint); }
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  /// <summary>Determines the extent. Actually the extents of the bounding region of the arc.</summary>
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 wType) override { return wType == EoDb::kEllipsePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  /// <summary>Determines the best control point on arc within specified tolerance. Control points for arcs are the points at start and end of the sweep.</summary>
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  /// <summary>Determines if a line crosses arc.</summary>
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d v) override { m_centerPoint += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoByte* buffer) override;

 public:
  void CutAtPt(EoGePoint3d&, EoDbGroup*) override;
  void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*) override;
  
  /// <summary>Generates a set of points which may be used to represent a arc using a double angle algorithm.</summary>
  void GenPts(EoGePoint3d centerPoint, EoGeVector3d majorAxis, EoGeVector3d minorAxis, double sweepAngle) const;
  EoGePoint3d GetBegPt();
  /// <summary>Determines the bounding region. This is always a quad, but it may not be xy oriented.</summary>
  void GetBoundingBox(EoGePoint3dArray&);
  EoGePoint3d GetEndPt();
  const EoGeVector3d& MajorAxis() const noexcept { return (m_majorAxis); }
  const EoGeVector3d& MinorAxis() const noexcept { return (m_minorAxis); }
  const EoGePoint3d& CenterPoint() const noexcept { return (m_centerPoint); }
  double SweepAngle() const noexcept { return (m_sweepAngle); }
  void GetXYExtents(EoGePoint3d, EoGePoint3d, EoGePoint3d*, EoGePoint3d*) const;
  int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) override;
  void SetCenterPoint(EoGePoint3d centerPoint) { m_centerPoint = std::move(centerPoint); }
  void SetMajorAxis(EoGeVector3d majorAxis) { m_majorAxis = std::move(majorAxis); }
  void SetMinorAxis(EoGeVector3d minorAxis) { m_minorAxis = std::move(minorAxis); }
  void SetSweepAngle(double sweepAngle) { m_sweepAngle = sweepAngle; }
  
  double SweepAngleToPoint(EoGePoint3d point);

 private:
  static double NormalizeTo2Pi(double angle);
};

int SweepAngleFromNormalAnd3Points(EoGeVector3d, EoGePoint3d, EoGePoint3d, EoGePoint3d, EoGePoint3d&, double*);
