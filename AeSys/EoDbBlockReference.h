#pragma once

class EoDbBlockReference : public EoDbPrimitive {
private:
	CString m_strName;
	EoGePoint3d m_pt;
	EoGeVector3d m_vNormal;
	EoGeVector3d m_vScaleFactors;
	double m_dRotation;

	EoUInt16 m_wColCnt;
	EoUInt16 m_wRowCnt;
	double m_dColSpac;
	double m_dRowSpac;

public: // Constructors and destructor
	EoDbBlockReference();
	EoDbBlockReference(const CString& strName, const EoGePoint3d& pt);
	EoDbBlockReference(const EoDbBlockReference&);
	EoDbBlockReference(EoUInt16 penColor, EoUInt16 lineType, const CString& name,
		const EoGePoint3d& point, const EoGeVector3d& normal, const EoGeVector3d scaleFactors, double rotation);
	virtual ~EoDbBlockReference() {
	};
public: // Operators
	const EoDbBlockReference& operator=(const EoDbBlockReference&);

public: // Methods - absolute virtuals
	void AddToTreeViewControl(HWND hTree, HTREEITEM hParent);
	void Assign(EoDbPrimitive* primitive) {
		*this = *static_cast<EoDbBlockReference*>(primitive);
	}
	EoGeTransformMatrix	BuildTransformMatrix(const EoGePoint3d& ptBase);
#if defined(USING_ODA)
	OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif // USING_ODA
	EoDbPrimitive*& Copy(EoDbPrimitive*&);
	void Display(AeSysView* view, CDC* deviceContext);
	void AddReportToMessageList(EoGePoint3d);
	void GetAllPts(EoGePoint3dArray& pts) {pts.SetSize(0); pts.Add(m_pt);}
	void FormatExtra(CString& str);
	void FormatGeometry(CString& str);
	EoGePoint3d	GetCtrlPt();
	void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
	EoGePoint3d	GoToNxtCtrlPt() {return m_pt;}
	bool Identical(EoDbPrimitive*) {return false;}
	bool Is(EoUInt16 type) {
		return type == EoDb::kGroupReferencePrimitive;
	}
	bool IsInView(AeSysView* view);
	bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) {
		return false;
	}
	void Read(CFile&);
	EoGePoint3d	SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point);
	bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) {
		return false;
	}
	/// <summary>Evaluates whether a point lies within tolerance specified of block.</summary>
	bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&);
	bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d);
	void Transform(EoGeTransformMatrix&);
	void Translate(EoGeVector3d v) {m_pt += v;}
	void TranslateUsingMask(EoGeVector3d v, const DWORD mask);
	bool Write(CFile& file);
	void Write(CFile& /* file */, EoByte* /* buffer */) {
	};
public: // Methods
	EoUInt16& ColCnt() {return m_wColCnt;}
	double& ColSpacing() {return m_dColSpac;}
	CString GetName() {return m_strName;}
	double GetRotation() {return m_dRotation;}
	EoGeVector3d GetScaleFactors() {return m_vScaleFactors;}
	EoGePoint3d& InsPt() {return m_pt;}
	EoGeVector3d Normal() {return m_vNormal;}
	EoUInt16& RowCnt() {return m_wRowCnt;}
	double& RowSpacing() {return m_dRowSpac;}

	void SetNormal(const EoGeVector3d& normal) {m_vNormal = normal;}
	void SetPosition(const EoGePoint3d& position) {m_pt = position;}
	void SetScaleFactors(const EoGeVector3d& scaleFactors) {m_vScaleFactors = scaleFactors;}
	void SetRotation(double rotation) {m_dRotation = rotation;}
	void SetRows(EoUInt16 rows) {m_wRowCnt = rows;}
	void SetRowSpacing(double rowSpacing) {m_dRowSpac = rowSpacing;}
	void SetColumns(EoUInt16 columns) {m_wColCnt = columns;}
	void SetColumnSpacing(double columnSpacing) {m_dColSpac = columnSpacing;}
};
