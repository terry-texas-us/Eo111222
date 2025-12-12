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

  ~EoDbPolyline() override {}

 public:  // Operators
  const EoDbPolyline& operator=(const EoDbPolyline& polyline);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbPolyline*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
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
  bool Is(EoUInt16 type) override { return type == EoDb::kPolylinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) override { return false; }
  bool PvtOnCtrlPt(AeSysView* view, const EoGePoint4d& ptView) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) override { return false; }
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void SetFlag(const EoUInt16 w) { m_wFlags = w; }
  void SetPt(int index, const EoGePoint3d& pt) { m_pts[index] = pt; }
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& /* file */, EoByte* /* buffer */) override {};

 private:
  EoUInt16 SwingVertex();

 public:
  static EoUInt16& EdgeToEvaluate() { return sm_EdgeToEvaluate; }
  static EoUInt16& Edge() { return sm_Edge; }
  bool IsLooped() { return (m_wFlags != 0); }
};
