#pragma once
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>

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
#include "drw_Base.h"

class EoDbLine : public EoDbPrimitive {
 private:
  EoGeLine m_ln;

 public:  // Constructors and destructor
  EoDbLine() {}
  EoDbLine(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint);
  EoDbLine(EoGeLine& line);
  EoDbLine(EoInt16 penColor, EoInt16 lineType, EoGeLine line);
  EoDbLine(EoInt16 penColor, EoInt16 lineType, const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint);
  EoDbLine(const DRW_Coord& beginPoint, const DRW_Coord& endPoint);

  EoDbLine(const EoDbLine& other);

  ~EoDbLine() override {};

 public:
  const EoDbLine& operator=(const EoDbLine& other);

 public:
  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbLine*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override { return m_ln.Midpoint(); }
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive* primitive) override;
  bool Is(EoUInt16 wType) override { return wType == EoDb::kLinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix& transformMatrix) override;
  void Translate(EoGeVector3d v) override { m_ln += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoUInt8*) override;

 public:  // Methods - virtuals
  void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*) override;
  /// <summary>Cuts a line a point.</summary>
  void CutAtPt(EoGePoint3d&, EoDbGroup*) override;
  int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) override;

 public:  // Methods
  void GetLine(EoGeLine& line) const { line = m_ln; }
  void GetPts(EoGePoint3d& beginPoint, EoGePoint3d& endPoint) const {
    beginPoint = m_ln.begin;
    endPoint = m_ln.end;
  }

  void SetBeginPoint(double x, double y, double z) {
    m_ln.begin.x = x;
    m_ln.begin.y = y;
    m_ln.begin.z = z;
  }

  void SetEndPoint(double x, double y, double z) {
    m_ln.end.x = x;
    m_ln.end.y = y;
    m_ln.end.z = z;
  }

  EoGePoint3d& BeginPoint() { return m_ln.begin; }
  EoGePoint3d& EndPoint() { return m_ln.end; }
  EoGeLine& Ln() { return m_ln; }
  double Length() { return (m_ln.Length()); }
  EoGePoint3d ProjPt(const EoGePoint3d& pt) { return (m_ln.ProjPt(pt)); }
  double RelOfPt(EoGePoint3d pt);
  void BeginPoint(EoGePoint3d pt) { m_ln.begin = pt; }
  void EndPoint(EoGePoint3d pt) { m_ln.end = pt; }
  void Square(AeSysView* view);
};
