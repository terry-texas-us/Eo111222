#pragma once

class EoDbEllipse : public EoDbPrimitive {
private:
	EoGePoint3d	m_ptCenter;
	EoGeVector3d m_vMajAx;
	EoGeVector3d m_vMinAx;
	double m_dSwpAng;

public: // Constructors and destructor
	EoDbEllipse() {
	}
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
	/// <summary>Arc is constructed using three points on arc.</summary>
	EoDbEllipse(EoGePoint3d, EoGePoint3d, EoGePoint3d);

	EoDbEllipse(const EoDbEllipse&);

	~EoDbEllipse() {
	}
public: // Operators
	const EoDbEllipse& operator=(const EoDbEllipse&);

public: // Methods - absolute virtuals
	void AddToTreeViewControl(HWND hTree, HTREEITEM hParent);
	void Assign(EoDbPrimitive* primitive) {
		*this = *static_cast<EoDbEllipse*>(primitive);
	}
#if defined(USING_ODA)
	OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif // USING_ODA
	EoDbPrimitive*& Copy(EoDbPrimitive*&);
	void Display(AeSysView* view, CDC* deviceContext);
	void AddReportToMessageList(EoGePoint3d);
	void GetAllPts(EoGePoint3dArray& pts) {
		pts.SetSize(0); pts.Add(m_ptCenter);
	}
	EoGePoint3d GetCtrlPt() {
		return (m_ptCenter);
	}
	void FormatExtra(CString& str);
	void FormatGeometry(CString& str);
	/// <summary>Determines the extent. Actually the extents of the bounding region of the arc.</summary>
	void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
	EoGePoint3d GoToNxtCtrlPt();
	bool Identical(EoDbPrimitive*) {
		return false;
	}
	bool Is(EoUInt16 wType) {
		return wType == EoDb::kEllipsePrimitive;
	}
	bool IsInView(AeSysView* view);
	bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point);
	/// <summary>Determines the best control point on arc within specified tolerance. Control points for arcs are the points at start and end of the sweep.</summary>
	EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point);
	/// <summary>Determines if a line crosses arc.</summary>
	bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt);
	bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&);
	bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d);
	void Transform(EoGeTransformMatrix&);
	void Translate(EoGeVector3d v) {
		m_ptCenter += v;
	}
	void TranslateUsingMask(EoGeVector3d, const DWORD);
	bool Write(CFile& file);
	void Write(CFile& file, EoByte* buffer);

public: // Methods
	void CutAtPt(EoGePoint3d&, EoDbGroup*);
	void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*);
	/// <summary>Generates a set of points which may be used to represent a arc using a double angle algorithm.</summary>
	void GenPts(EoGePoint3d ptCent, EoGeVector3d vMajAx, EoGeVector3d vMinAx, double dSwpAng);
	EoGePoint3d GetBegPt();
	/// <summary>Determines the bounding region. This is always a quad, but it may not be xy oriented.</summary>
	void GetBoundingBox(EoGePoint3dArray&);
	EoGePoint3d GetEndPt();
	EoGeVector3d GetMajAx() {
		return (m_vMajAx);
	}
	EoGeVector3d GetMinAx() {
		return (m_vMinAx);
	}
	const EoGePoint3d& Center() {
		return (m_ptCenter);
	}
	double GetSwpAng() {
		return (m_dSwpAng);
	}
	void GetXYExtents(EoGePoint3d, EoGePoint3d, EoGePoint3d*, EoGePoint3d*);
	int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*);
	void SetCP(EoGePoint3d pt) {
		m_ptCenter = pt;
	}
	void SetMajAx(EoGeVector3d v) {
		m_vMajAx = v;
	}
	void SetMinAx(EoGeVector3d v) {
		m_vMinAx = v;
	}
	void SetSwpAng(double dAng) {
		m_dSwpAng = dAng;
	}
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
