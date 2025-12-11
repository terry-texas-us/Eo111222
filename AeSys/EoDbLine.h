#pragma once


class EoDbLine : public EoDbPrimitive {
private:
	EoGeLine m_ln;

public: // Constructors and destructor
	EoDbLine() {
	}
	EoDbLine(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint);
	EoDbLine(EoGeLine& line);
	EoDbLine(EoInt16 penColor, EoInt16 lineType, EoGeLine line);
	EoDbLine(EoInt16 penColor, EoInt16 lineType, const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint);
#if defined(USING_ODA)
	EoDbLine(const OdGePoint3d& beginPoint, const OdGePoint3d& endPoint);
#endif // USING_ODA

	EoDbLine(const EoDbLine& src);

	~EoDbLine() {
	};

public: // Operators
	const EoDbLine& operator=(const EoDbLine& src);

public: //	Methods - absolute virtuals
	void AddToTreeViewControl(HWND hTree, HTREEITEM hParent);
	void Assign(EoDbPrimitive* primitive) {
		*this = *static_cast<EoDbLine*>(primitive);
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
	EoGePoint3d GetCtrlPt() {
		return m_ln.Midpoint();
	}
	/// <summary>Determines the extent.</summary>
	void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
	EoGePoint3d GoToNxtCtrlPt();
	bool Identical(EoDbPrimitive* primitive);
	bool Is(EoUInt16 wType) {
		return wType == EoDb::kLinePrimitive;
	}
	/// <summary>Tests whether a line is wholly or partially within the current view volume.</summary>
	bool IsInView(AeSysView* view);
	/// <summary>Determines if a line is identified by a point.</summary>
	bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point);
	EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point);
	/// <summary>Evaluates whether a line intersects line.</summary>
	bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt);
	/// <summary>Evaluates whether a point lies within tolerance specified of line.</summary>
	bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&);
	/// <summary>Determines whether a line is partially or wholly within the area defined by the two points passed.</summary>
	bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d);
	void Transform(EoGeTransformMatrix& tm) {
		BeginPoint(tm * BeginPoint()); EndPoint(tm * EndPoint());
	}
	void Translate(EoGeVector3d v) {
		m_ln += v;
	}
	void TranslateUsingMask(EoGeVector3d, const DWORD);
	bool Write(CFile& file);
	void Write(CFile& file, EoByte*);

public: // Methods - virtuals
	/// <summary>Cuts a line at two points.</summary>
	// Notes:	Line segment between to points goes in groups.
	void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*);
	/// <summary>Cuts a line a point.</summary>
	void CutAtPt(EoGePoint3d&, EoDbGroup*);
	int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*);

public: // Methods
	void GetLine(EoGeLine& ln) {
		ln = m_ln;
	}
	void GetPts(EoGePoint3d& ptBeg, EoGePoint3d& ptEnd) {
		ptBeg = m_ln.begin; ptEnd = m_ln.end;
	}
	EoGePoint3d& BeginPoint() {
		return m_ln.begin;
	}
	EoGePoint3d& EndPoint() {
		return m_ln.end;
	}
	EoGeLine& Ln() {
		return m_ln;
	}
	double Length() {
		return (m_ln.Length());
	}
	EoGePoint3d ProjPt(const EoGePoint3d& pt) {
		return (m_ln.ProjPt(pt));
	}
	double RelOfPt(EoGePoint3d pt);
	void BeginPoint(EoGePoint3d pt) {
		m_ln.begin = pt;
	}
	void EndPoint(EoGePoint3d pt) {
		m_ln.end = pt;
	}
	void Square(AeSysView* view);
};
