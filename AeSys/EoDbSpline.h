#pragma once

#include "EoDbPrimitive.h"

class EoDbSpline : public EoDbPrimitive {
 private:
  EoGePoint3dArray m_pts;

 public:  // Constructors and destructor
  EoDbSpline() {}
  EoDbSpline(EoByte* buffer, int version);
  EoDbSpline(EoUInt16, EoGePoint3d*);
  EoDbSpline(EoGePoint3dArray& points);
  EoDbSpline(EoInt16 penColor, EoInt16 lineType, EoGePoint3dArray& points);
  EoDbSpline(const EoDbSpline&);

  ~EoDbSpline() override {}

 public:  // Operators
  const EoDbSpline& operator=(const EoDbSpline&);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbSpline*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  /// <summary>
  ///Generates the required B-spline basis dKnot vectors and B-spline curves of various iOrder
  ///using the Cox and de Boor algorithm.
  /// </summary>
  /// <remarks>
  ///	If the iOrder equals the number of vertices, and there are no multiple vertices, a Bezier curve will
  ///	be generated. As the iOrder decreases the curve produced will lie closer to the defining polygon.
  ///	When the iOrder is two the generated curve is a series of straight lines which are identical to the
  ///	defining polygon.  Increasing the iOrder "tightens" the curve.	Additional shape control can be obtained by
  ///	use of repeating vertices.
  /// </remarks>
  /// <param name="iOrder">iOrder of B-spline basis</param>
  /// <param name="pts">array of points generated</param>
  int GenPts(const int iOrder, EoGePoint3dArray& pts);
  void GetAllPts(EoGePoint3dArray& pts) override {
    pts.SetSize(0);
    pts.Copy(m_pts);
  }
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetCtrlPt() override;
  /// <summary>Determines the extent.</summary>
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 type) override { return type == EoDb::kSplinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) override { return false; }
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) override { return false; }
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void SetPt(EoUInt16 w, EoGePoint3d pt) { m_pts[w] = pt; }
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoByte* buffer) override;
};
