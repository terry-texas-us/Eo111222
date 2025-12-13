#pragma once

class EoDbEllipse : public EoDbPrimitive {
 private:
  EoGePoint3d m_ptCenter;
  EoGeVector3d m_vMajAx;
  EoGeVector3d m_vMinAx;
  double m_dSwpAng;

 public:
  EoDbEllipse() : m_ptCenter {}, m_vMajAx {}, m_vMinAx {}, m_dSwpAng {0.0} {}
  
  /// <summary>Ellipse segment is constructed using a center point, a major and minor vector and a sweep ang.</summary>
  EoDbEllipse(const EoGePoint3d& centerPoint, const EoGeVector3d& majorAxis, const EoGeVector3d& minorAxis,
              double sweepAngle);
  
  /// <summary>Ellipse segment is constructed using a center point, a major and minor vector and a sweep ang.</summary>
  EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, EoGeVector3d& majorAxis,
              EoGeVector3d& minorAxis, double sweepAngle);
  
  /// <summary>Ellipse is constructed using a center point and a radius about view plane normal</summary>
  EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, double radius);
  
  /// <summary>Ellipse is constructed using a center point, plane normal and a radius</summary>
  EoDbEllipse(EoInt16 penColor, EoInt16 lineType, EoGePoint3d& centerPoint, EoGeVector3d& planeNormal, double radius);
  
  /// <summary>Circle is constructed using a center point and a begin point about view plane normal.</summary>
  EoDbEllipse(EoGePoint3d& centerPoint, EoGePoint3d& beginPoint);
  
  /// @brief Constructs an ellipse from three points that define an elliptical arc.
  /// @param beginPoint The starting point of the elliptical arc.
  /// @param intermediatePoint A point on the elliptical arc between the start and end points.
  /// @param endPoint The ending point of the elliptical arc.
  EoDbEllipse(EoGePoint3d beginPoint, EoGePoint3d intermediatePoint, EoGePoint3d endPoint);

  EoDbEllipse(const EoDbEllipse&);

  ~EoDbEllipse() override {}

 public:  // Operators
  const EoDbEllipse& operator=(const EoDbEllipse&);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbEllipse*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void GetAllPts(EoGePoint3dArray& pts) override {
    pts.SetSize(0);
    pts.Add(m_ptCenter);
  }
  EoGePoint3d GetCtrlPt() override { return (m_ptCenter); }
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
  void Translate(EoGeVector3d v) override { m_ptCenter += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoByte* buffer) override;

 public:  // Methods
  void CutAtPt(EoGePoint3d&, EoDbGroup*) override;
  void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*) override;
  /// <summary>Generates a set of points which may be used to represent a arc using a double angle algorithm.</summary>
  void GenPts(EoGePoint3d ptCent, EoGeVector3d vMajAx, EoGeVector3d vMinAx, double dSwpAng);
  EoGePoint3d GetBegPt();
  /// <summary>Determines the bounding region. This is always a quad, but it may not be xy oriented.</summary>
  void GetBoundingBox(EoGePoint3dArray&);
  EoGePoint3d GetEndPt();
  EoGeVector3d GetMajAx() { return (m_vMajAx); }
  EoGeVector3d GetMinAx() { return (m_vMinAx); }
  const EoGePoint3d& Center() { return (m_ptCenter); }
  double GetSwpAng() { return (m_dSwpAng); }
  void GetXYExtents(EoGePoint3d, EoGePoint3d, EoGePoint3d*, EoGePoint3d*);
  int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*) override;
  void SetCP(EoGePoint3d pt) { m_ptCenter = pt; }
  void SetMajAx(EoGeVector3d v) { m_vMajAx = v; }
  void SetMinAx(EoGeVector3d v) { m_vMinAx = v; }
  void SetSwpAng(double dAng) { m_dSwpAng = dAng; }
  double SwpAngToPt(EoGePoint3d);
};

EoGePoint3d pFndPtOnArc(EoGePoint3d, EoGeVector3d, EoGeVector3d, const double);
int pFndSwpAngGivPlnAnd3Lns(EoGeVector3d, EoGePoint3d, EoGePoint3d, EoGePoint3d, EoGePoint3d&, double*);
/// <summary>Finds center point of a circle given radius and two tangent vectors.</summary>

// Notes:	A radius and two lines define four center points.  The center point
//			selected is on the concave side of the angle formed by the two vectors
//			defined by the line endpoints.	These two vectors are oriented with
//			the tail of the second vector at the head of the first.

// Returns: TRUE	center point determined
//			FALSE	endpoints of first line coincide or endpoints of second line coincide or
//					two lines are parallel or four points are not coplanar
bool pFndCPGivRadAnd4Pts(double, EoGePoint3d, EoGePoint3d, EoGePoint3d, EoGePoint3d, EoGePoint3d*);
