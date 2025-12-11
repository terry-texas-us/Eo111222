#pragma once

class EoDbPolygon : public EoDbPrimitive {
	static EoUInt16 sm_EdgeToEvaluate;
	static EoUInt16 sm_Edge;
	static EoUInt16 sm_PivotVertex;

	static EoInt16 sm_SpecialPolygonStyle;

	EoInt16	m_InteriorStyle;
	EoInt16	m_InteriorStyleIndex;
	EoUInt16 m_NumberOfPoints;
	EoGePoint3d	m_HatchOrigin;
	EoGeVector3d m_vPosXAx;
	EoGeVector3d m_vPosYAx;
	EoGePoint3d* m_Pt;

public:	// Constructors and destructor
	EoDbPolygon();
	EoDbPolygon(EoByte* buffer, int version);
	EoDbPolygon(EoGePoint3dArray& pts);
	EoDbPolygon(EoUInt16, EoGePoint3d*);
	EoDbPolygon(EoGePoint3d& origin, EoGeVector3d& xAxis, EoGeVector3d& yAxis, EoGePoint3dArray& pts);
	EoDbPolygon(EoInt16 penColor, EoInt16 style, EoInt16 styleIndex, EoGePoint3d& origin, EoGeVector3d& xAxis, EoGeVector3d& yAxis, EoGePoint3dArray& points);
	EoDbPolygon(EoUInt16, EoGePoint3d, EoGeVector3d, EoGeVector3d, const EoGePoint3d*);

	EoDbPolygon(const EoDbPolygon& src);
public: // Operators
	const EoDbPolygon& operator=(const EoDbPolygon&);

	EoGePoint3d& operator[](int i) {return m_Pt[i];}
	const EoGePoint3d& operator[](int i) const {return m_Pt[i];}

	~EoDbPolygon();

public: // Methods - absolute virtuals
	void	AddToTreeViewControl(HWND hTree, HTREEITEM hParent);
	void	Assign(EoDbPrimitive* primitive) {
		*this = *static_cast<EoDbPolygon*>(primitive);
	}
#if defined(USING_ODA)
	OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif // USING_ODA
	EoDbPrimitive*& Copy(EoDbPrimitive*&);
	void Display(AeSysView* view, CDC* deviceContext);
	void AddReportToMessageList(EoGePoint3d);
	void FormatExtra(CString& str);
	void FormatGeometry(CString& str);
	void GetAllPts(EoGePoint3dArray& pts);
	EoGePoint3d	GetCtrlPt();
	void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
	EoGePoint3d	GoToNxtCtrlPt();
	bool Identical(EoDbPrimitive*) {return false;}
	bool Is(EoUInt16 wType) {
		return wType == EoDb::kPolygonPrimitive;
	}
	bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point);
	bool IsInView(AeSysView* view);
	EoGePoint3d	SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point);
	bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) {
		return false;
	}
	bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&);
	bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d);
	void Transform(EoGeTransformMatrix&);
	void Translate(EoGeVector3d translate);
	void TranslateUsingMask(EoGeVector3d, const DWORD);
	bool Write(CFile& file);
	void Write(CFile& file, EoByte* buffer);

	CString FormatIntStyle();
	const EoInt16& IntStyle() {return (m_InteriorStyle);}
	const EoInt16& IntStyleId() {return (m_InteriorStyleIndex);}
	EoGePoint3d GetPt(int i) {return (m_Pt[i]);}
	int GetPts() {return (m_NumberOfPoints);}
	void ModifyState();
	bool PvtOnCtrlPt(AeSysView* view, const EoGePoint4d&);
	void SetIntStyle(const EoInt16 n) {m_InteriorStyle = n;}
	void SetIntStyleId(const EoInt16 n) {m_InteriorStyleIndex = n;}
	void SetHatRefVecs(double, double, double);

private:
	EoUInt16 SwingVertex();

public:
	static void SetSpecialPolygonStyle(EoInt16 polygonStyle) {
		sm_SpecialPolygonStyle = polygonStyle;
	}
	static EoUInt16& EdgeToEvaluate() {return sm_EdgeToEvaluate;}
	static EoUInt16& Edge() {return sm_Edge;}
};
/// <summary>A fill area set primative with interior style hatch is generated using ines.</summary>
// Parameters:	deviceContext
//				iSets		number of point lists
//				iPtLstsId	starting indicies for point lists
void DisplayFilAreaHatch(AeSysView* view, CDC* deviceContext, EoGeTransformMatrix& tm, const int iSets, const int* iPtLstsId, EoGePoint3d*);
/// <summary>Generates polygon.</summary>
// The polygon is closed automatically by drawing a line from the last vertex to the first.
// Arrays of vertices are previously modelview transformed and clipped to view volume.
void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray);
