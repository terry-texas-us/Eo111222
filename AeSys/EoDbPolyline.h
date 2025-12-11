#pragma once

class EoDbPolyline : public EoDbPrimitive {
  static EoUInt16 sm_EdgeToEvaluate;
  static EoUInt16 sm_Edge;
  static EoUInt16 sm_PivotVertex;

 public:
  static const EoUInt16 sm_Closed = 0x0010;

 private:
  EoUInt16 m_wFlags;
  EoGePoint3dArray m_pts;

 public:  // Constructors and destructor
  EoDbPolyline();
  EoDbPolyline(EoByte* buffer);
  EoDbPolyline(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, double radius, int numberOfSides);
  EoDbPolyline(EoInt16 penColor, EoInt16 lineType, EoGePoint3dArray& pts);
  EoDbPolyline(EoGePoint3dArray& pts);
  EoDbPolyline(const EoDbPolyline& polyline);

  ~EoDbPolyline() {}

 public:  // Operators
  const EoDbPolyline& operator=(const EoDbPolyline& polyline);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent);
  void Assign(EoDbPrimitive* primitive) { *this = *static_cast<EoDbPolyline*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&);
  void Display(AeSysView* view, CDC* deviceContext);
  void AddReportToMessageList(EoGePoint3d);
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
  bool Is(EoUInt16 type) { return type == EoDb::kPolylinePrimitive; }
  bool IsInView(AeSysView* view);
  bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) { return false; }
  bool PvtOnCtrlPt(AeSysView* view, const EoGePoint4d& ptView);
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point);
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) { return false; }
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&);
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d);
  void SetFlag(const EoUInt16 w) { m_wFlags = w; }
  void SetPt(int index, const EoGePoint3d& pt) { m_pts[index] = pt; }
  void Transform(EoGeTransformMatrix&);
  void Translate(EoGeVector3d translate);
  void TranslateUsingMask(EoGeVector3d, const DWORD);
  bool Write(CFile& file);
  void Write(CFile& /* file */, EoByte* /* buffer */) {};

 private:
  EoUInt16 SwingVertex();

 public:
  static EoUInt16& EdgeToEvaluate() { return sm_EdgeToEvaluate; }
  static EoUInt16& Edge() { return sm_Edge; }
  bool IsLooped() { return (m_wFlags != 0); }
};
