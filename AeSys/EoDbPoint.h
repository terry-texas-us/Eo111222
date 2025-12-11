#pragma once

class EoDbPoint : public EoDbPrimitive {
private:
	EoInt16	m_PointStyle;
	EoGePoint3d	m_Point;
	EoUInt16 m_NumberOfDatums;
	double* m_Data;

public: // Constructors and destructor
	EoDbPoint();
	EoDbPoint(const EoGePoint3d& point);
	EoDbPoint(EoInt16 penColor, EoInt16 pointStyle, const EoGePoint3d& point);
	EoDbPoint(EoInt16 penColor, EoInt16 pointStyle, const EoGePoint3d& point, EoUInt16 numberOfDatums, double* data);

	EoDbPoint(const EoDbPoint& src);

	~EoDbPoint();

public: // Operators
	const EoDbPoint& operator=(const EoDbPoint& src);

public: // Methods - absolute virtuals
	void AddToTreeViewControl(HWND hTree, HTREEITEM hParent);
	void Assign(EoDbPrimitive* primitive) {
		*this = *static_cast<EoDbPoint*>(primitive);
	}
#if defined(USING_ODA)
	OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif // USING_ODA
	EoDbPrimitive*& Copy(EoDbPrimitive*&);
	void Display(AeSysView* view, CDC* deviceContext);
	void AddReportToMessageList(EoGePoint3d);
	void FormatExtra(CString& str);
	void FormatGeometry(CString& str);
	void GetAllPts(EoGePoint3dArray& pts) {pts.SetSize(0); pts.Add(m_Point);}
	EoGePoint3d GetCtrlPt();
	/// <summary>Determines the extent.</summary>
	void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
	EoGePoint3d GoToNxtCtrlPt() {
		return (m_Point);
	}
	bool Identical(EoDbPrimitive* primitive) {
		return m_Point == static_cast<EoDbPoint*>(primitive)->m_Point;
	}
	bool Is(EoUInt16 wType) {
		return wType == EoDb::kPointPrimitive;
	}
	bool IsInView(AeSysView* view);
	bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point);
	EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point);
	bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) {
		return false;
	}
	bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&);
	bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d);
	void Transform(EoGeTransformMatrix&);
	void Translate(EoGeVector3d v) {
		m_Point += v;
	}
	void TranslateUsingMask(EoGeVector3d, const DWORD);
	bool Write(CFile& file);
	void Write(CFile& file, EoByte* buffer);

public: // Methods
	double GetDat(EoUInt16 wDat) {
		return (m_Data[wDat]);
	}
	EoGePoint3d GetPt() {
		return (m_Point);
	}
	EoInt16& PointStyle() {
		return m_PointStyle;
	}
	void ModifyState();
	void SetDat(EoUInt16, double*);
	void SetPt(EoGePoint3d pt) {
		m_Point = pt;
	}
};
