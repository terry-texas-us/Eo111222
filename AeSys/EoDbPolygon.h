#pragma once

class EoDbPolygon : public EoDbPrimitive {
  static EoUInt16 sm_EdgeToEvaluate;
  static EoUInt16 sm_Edge;
  static EoUInt16 sm_PivotVertex;

  static EoInt16 sm_SpecialPolygonStyle;

  EoInt16 m_InteriorStyle;
  EoInt16 m_InteriorStyleIndex;
  EoUInt16 m_NumberOfPoints;
  EoGePoint3d m_HatchOrigin;
  EoGeVector3d m_vPosXAx;
  EoGeVector3d m_vPosYAx;
  EoGePoint3d* m_Pt;

 public:  // Constructors and destructor
  EoDbPolygon();
  EoDbPolygon(EoByte* buffer, int version);

  /// @brief Constructs an EoDbPolygon object from an array of 3D points.
  /// @param points An array of 3D points that define the vertices of the polygon. Must contain at least 3 points for proper initialization of the plane vectors.
  EoDbPolygon(EoGePoint3dArray& points);
  EoDbPolygon(EoUInt16, EoGePoint3d*);
  EoDbPolygon(EoGePoint3d& origin, EoGeVector3d& xAxis, EoGeVector3d& yAxis, EoGePoint3dArray& pts);
  EoDbPolygon(EoInt16 penColor, EoInt16 style, EoInt16 styleIndex, EoGePoint3d& origin, EoGeVector3d& xAxis,
              EoGeVector3d& yAxis, EoGePoint3dArray& points);
  EoDbPolygon(EoUInt16, EoGePoint3d, EoGeVector3d, EoGeVector3d, const EoGePoint3d*);

  EoDbPolygon(const EoDbPolygon& src);

 public:  // Operators
  const EoDbPolygon& operator=(const EoDbPolygon&);

  EoGePoint3d& operator[](int i) { return m_Pt[i]; }
  const EoGePoint3d& operator[](int i) const { return m_Pt[i]; }

  ~EoDbPolygon() override;

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbPolygon*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPts(EoGePoint3dArray& pts) override;
  EoGePoint3d GetCtrlPt() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 wType) override { return wType == EoDb::kPolygonPrimitive; }
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool IsInView(AeSysView* view) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) override { return false; }
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoByte* buffer) override;

  CString FormatIntStyle();
  const EoInt16& IntStyle() { return (m_InteriorStyle); }
  const EoInt16& IntStyleId() { return (m_InteriorStyleIndex); }
  EoGePoint3d GetPt(int i) { return (m_Pt[i]); }
  int GetPts() { return (m_NumberOfPoints); }
  void ModifyState() override;
  bool PvtOnCtrlPt(AeSysView* view, const EoGePoint4d&) override;
  void SetIntStyle(const EoInt16 n) { m_InteriorStyle = n; }
  void SetIntStyleId(const EoInt16 n) { m_InteriorStyleIndex = n; }
  void SetHatRefVecs(double, double, double);

 private:
  EoUInt16 SwingVertex();

 public:
  static void SetSpecialPolygonStyle(EoInt16 polygonStyle) { sm_SpecialPolygonStyle = polygonStyle; }
  static EoUInt16& EdgeToEvaluate() { return sm_EdgeToEvaluate; }
  static EoUInt16& Edge() { return sm_Edge; }
};
/// <summary>A fill area set primative with interior style hatch is generated using ines.</summary>
// Parameters:	deviceContext
//				iSets		number of point lists
//				iPtLstsId	starting indicies for point lists
void DisplayFilAreaHatch(AeSysView* view, CDC* deviceContext, EoGeTransformMatrix& tm, const int iSets,
                         const int* iPtLstsId, EoGePoint3d*);
/// <summary>Generates polygon.</summary>
// The polygon is closed automatically by drawing a line from the last vertex to the first.
// Arrays of vertices are previously modelview transformed and clipped to view volume.
void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray);
