#pragma once

#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGeTransformMatrix.h"
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
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbLine*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPts(EoGePoint3dArray& pts) override;
  EoGePoint3d GetCtrlPt() override { return m_ln.Midpoint(); }
  /// <summary>Determines the extent.</summary>
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override;
  bool Identical(EoDbPrimitive* primitive) override;
  bool Is(EoUInt16 wType) override { return wType == EoDb::kLinePrimitive; }
  /// <summary>Tests whether a line is wholly or partially within the current view volume.</summary>
  bool IsInView(AeSysView* view) override;
  /// <summary>Determines if a line is identified by a point.</summary>
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  /// <summary>Evaluates whether a line intersects line.</summary>
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt) override;
  /// <summary>Evaluates whether a point lies within tolerance specified of line.</summary>
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  /// <summary>Determines whether a line is partially or wholly within the area defined by the two points passed.</summary>
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix& tm) override {
    BeginPoint(tm * BeginPoint());
    EndPoint(tm * EndPoint());
  }
  void Translate(EoGeVector3d v) override { m_ln += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoByte*) override;

 public:  // Methods - virtuals
  /// <summary>Cuts a line at two points.</summary>
  // Notes:	Line segment between to points goes in groups.
  void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*) override;
  /// <summary>Cuts a line a point.</summary>
  void CutAtPt(EoGePoint3d&, EoDbGroup*) override;
  int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) override;

 public:  // Methods
  void GetLine(EoGeLine& ln) { ln = m_ln; }
  void GetPts(EoGePoint3d& ptBeg, EoGePoint3d& ptEnd) {
    ptBeg = m_ln.begin;
    ptEnd = m_ln.end;
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
