#pragma once

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

  ~EoDbSpline() {}

 public:  // Operators
  const EoDbSpline& operator=(const EoDbSpline&);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent);
  void Assign(EoDbPrimitive* primitive) { *this = *static_cast<EoDbSpline*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&);
  void Display(AeSysView* view, CDC* deviceContext);
  void AddReportToMessageList(EoGePoint3d);
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
  void GetAllPts(EoGePoint3dArray& pts) {
    pts.SetSize(0);
    pts.Copy(m_pts);
  }
  void FormatExtra(CString& str);
  void FormatGeometry(CString& str);
  EoGePoint3d GetCtrlPt();
  /// <summary>Determines the extent.</summary>
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
  EoGePoint3d GoToNxtCtrlPt();
  bool Identical(EoDbPrimitive*) { return false; }
  bool Is(EoUInt16 type) { return type == EoDb::kSplinePrimitive; }
  bool IsInView(AeSysView* view);
  bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) { return false; }
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point);
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) { return false; }
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&);
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d);
  void SetPt(EoUInt16 w, EoGePoint3d pt) { m_pts[w] = pt; }
  void Transform(EoGeTransformMatrix&);
  void Translate(EoGeVector3d translate);
  void TranslateUsingMask(EoGeVector3d, const DWORD);
  bool Write(CFile& file);
  void Write(CFile& file, EoByte* buffer);
};
