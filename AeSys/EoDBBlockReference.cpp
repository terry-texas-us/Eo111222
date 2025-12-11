#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"

EoDbBlockReference::EoDbBlockReference() {
	m_pt = EoGePoint3d::kOrigin;
	m_vNormal = EoGeVector3d::kZAxis;

	m_vScaleFactors(1., 1., 1.);
	m_dRotation = 0.;

	m_wColCnt = 1;
	m_wRowCnt = 1;
	m_dColSpac = 0.;
	m_dRowSpac = 0.;
}
EoDbBlockReference::EoDbBlockReference(const CString& strName, const EoGePoint3d& pt)
	: m_strName(strName), m_pt(pt) {
	m_vNormal = EoGeVector3d::kZAxis;
	m_vScaleFactors(1., 1., 1.);
	m_dRotation = 0.;
	m_wColCnt = 1;
	m_wRowCnt = 1;
	m_dColSpac = 0.;
	m_dRowSpac = 0.;
}
EoDbBlockReference::EoDbBlockReference(const EoDbBlockReference& src) {
	m_strName = src.m_strName;
	m_pt = src.m_pt;
	m_vNormal = src.m_vNormal;
	m_vScaleFactors = src.m_vScaleFactors;
	m_dRotation = src.m_dRotation;
	m_wColCnt = src.m_wColCnt;
	m_wRowCnt = src.m_wRowCnt;
	m_dColSpac = src.m_dColSpac;
	m_dRowSpac = src.m_dRowSpac;
}
EoDbBlockReference::EoDbBlockReference(EoUInt16 penColor, EoUInt16 lineType, const CString& name,
	const EoGePoint3d& point, const EoGeVector3d& normal, const EoGeVector3d scaleFactors, double rotation)
	: m_strName(name), m_pt(point), m_vNormal(normal), m_vScaleFactors(scaleFactors) {
	m_PenColor = penColor;
	m_LineType = lineType;
	m_dRotation = rotation;

	m_wColCnt = 1;
	m_wRowCnt = 1;
	m_dColSpac = 0.;
	m_dRowSpac = 0.;

}
const EoDbBlockReference& EoDbBlockReference::operator=(const EoDbBlockReference& src) {
	m_strName = src.m_strName;
	m_pt = src.m_pt;
	m_vNormal = src.m_vNormal;
	m_vScaleFactors = src.m_vScaleFactors;
	m_dRotation = src.m_dRotation;
	m_wColCnt = src.m_wColCnt;
	m_wRowCnt = src.m_wRowCnt;
	m_dColSpac = src.m_dColSpac;
	m_dRowSpac = src.m_dRowSpac;

	return (*this);
}
void EoDbBlockReference::AddToTreeViewControl(HWND hTree, HTREEITEM hParent) {
	EoDbBlock* Block;
	if (AeSysDoc::GetDoc()->LookupBlock(m_strName, Block) == 0) {return;}

	HTREEITEM hti = tvAddItem(hTree, hParent, L"<SegRef>", this);

	((EoDbGroup*) Block)->AddPrimsToTreeViewControl(hTree, hti);
}
EoGeTransformMatrix EoDbBlockReference::BuildTransformMatrix(const EoGePoint3d& ptBase) {
	EoGeTransformMatrix tm1;
	tm1.Translate(EoGeVector3d(ptBase, EoGePoint3d::kOrigin));
	EoGeTransformMatrix tm2;
	tm2.Scale(m_vScaleFactors);
	EoGeTransformMatrix tm3;
	tm3.ZAxisRotation(sin(m_dRotation), cos(m_dRotation));
	EoGeTransformMatrix tm4(EoGePoint3d::kOrigin, m_vNormal);
	EoGeTransformMatrix tm5;
	tm5.Translate(EoGeVector3d(EoGePoint3d::kOrigin, m_pt));

	return ((EoGeMatrix) tm1 * (EoGeMatrix) tm2 * (EoGeMatrix) tm3 * (EoGeMatrix) tm4 * (EoGeMatrix) tm5);
}
EoDbPrimitive*& EoDbBlockReference::Copy(EoDbPrimitive*& primitive) {
	primitive = new EoDbBlockReference(*this);
	return (primitive);
}
void EoDbBlockReference::Display(AeSysView* view, CDC* deviceContext) {
	EoDbBlock* Block;
	if (AeSysDoc::GetDoc()->LookupBlock(m_strName, Block) == 0)
		return;

	EoGePoint3d ptBase = Block->GetBasePt();
	EoGeTransformMatrix tm = BuildTransformMatrix(ptBase);

	view->InvokeNewModelTransform();
	view->SetLocalModelTransform(tm);

	Block->Display(view, deviceContext);

	view->ReturnModelTransform();
}
void EoDbBlockReference::AddReportToMessageList(EoGePoint3d) {
	CString str;
	str.Format(L"<SegRef> Color: %s Line Type: %s SegmentName %s", FormatPenColor(), FormatLineType(), m_strName);
	app.AddStringToMessageList(str);
}
void EoDbBlockReference::FormatExtra(CString& str) {
	str.Format(L"Color;%s\tStyle;%s\tSegment Name;%s\tRotation Angle;%f", FormatPenColor(), FormatLineType(), m_strName, m_dRotation);
}
void EoDbBlockReference::FormatGeometry(CString& str) {
	str += L"Insertion Point;" + m_pt.ToString();
	str += L"Normal;" + m_vNormal.ToString();
	str += L"Scale;" + m_vScaleFactors.ToString();
}
EoGePoint3d EoDbBlockReference::GetCtrlPt() {
	EoGePoint3d pt;
	pt = m_pt;
	return (pt);
}
void EoDbBlockReference::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
	EoDbBlock* Block;

	if (AeSysDoc::GetDoc()->LookupBlock(m_strName, Block) == 0) {return;}

	EoGePoint3d ptBase = Block->GetBasePt();

	EoGeTransformMatrix tmIns = BuildTransformMatrix(ptBase);

	view->InvokeNewModelTransform();
	view->SetLocalModelTransform(tmIns);

	Block->GetExtents(view, ptMin, ptMax, tm);

	view->ReturnModelTransform();
}
bool EoDbBlockReference::IsInView(AeSysView* view) {
	// Test whether an instance of a block is wholly or partially within the current view volume.
	EoDbBlock* Block;

	if (AeSysDoc::GetDoc()->LookupBlock(m_strName, Block) == 0) {return false;}

	EoGePoint3d ptBase = Block->GetBasePt();

	EoGeTransformMatrix tm = BuildTransformMatrix(ptBase);

	view->InvokeNewModelTransform();
	view->SetLocalModelTransform(tm);

	bool bInView = Block->IsInView(view);

	view->ReturnModelTransform();
	return (bInView);
}
EoGePoint3d EoDbBlockReference::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
	sm_ControlPointIndex = USHRT_MAX;
	EoGePoint3d ptCtrl;

	EoDbBlock* Block;

	if (AeSysDoc::GetDoc()->LookupBlock(m_strName, Block) == 0) {return ptCtrl;}

	EoGePoint3d ptBase = Block->GetBasePt();

	EoGeTransformMatrix tm = BuildTransformMatrix(ptBase);

	view->InvokeNewModelTransform();
	view->SetLocalModelTransform(tm);

	POSITION Position = Block->GetHeadPosition();
	while (Position != 0) {
		EoDbPrimitive* Primitive = Block->GetNext(Position);
		ptCtrl = Primitive->SelectAtControlPoint(view, point);
		if (sm_ControlPointIndex != USHRT_MAX) {
			view->ModelTransformPoint(ptCtrl);
			break;
		}
	}
	view->ReturnModelTransform();
	return ptCtrl;
}
bool EoDbBlockReference::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
	EoDbBlock* Block;

	if (AeSysDoc::GetDoc()->LookupBlock(m_strName, Block) == 0) {return false;}

	EoGePoint3d ptBase = Block->GetBasePt();

	EoGeTransformMatrix tm = BuildTransformMatrix(ptBase);

	view->InvokeNewModelTransform();
	view->SetLocalModelTransform(tm);

	bool bResult = Block->SelectUsingRectangle(view, pt1, pt2);

	view->ReturnModelTransform();
	return (bResult);
}
bool EoDbBlockReference::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
	bool bResult = false;

	EoDbBlock* Block;

	if (AeSysDoc::GetDoc()->LookupBlock(m_strName, Block) == 0) {
		return (bResult);
	}
	EoGePoint3d ptBase = Block->GetBasePt();

	EoGeTransformMatrix tm = BuildTransformMatrix(ptBase);

	view->InvokeNewModelTransform();
	view->SetLocalModelTransform(tm);

	POSITION Position = Block->GetHeadPosition();
	while (Position != 0) {
		if ((Block->GetNext(Position))->SelectUsingPoint(view, point, ptProj)) {
			bResult = true; 
			break;
		}
	}
	view->ReturnModelTransform();
	return (bResult);
}
void EoDbBlockReference::Transform(EoGeTransformMatrix& tm) {
	m_pt = tm * m_pt;
	m_vNormal = tm * m_vNormal;

	if (fabs(m_vNormal.x) <= FLT_EPSILON && fabs(m_vNormal.y) <= FLT_EPSILON) {
		m_vScaleFactors = tm * m_vScaleFactors;
	}
}
void EoDbBlockReference::TranslateUsingMask(EoGeVector3d v, DWORD mask) {
	if (mask != 0) {
		m_pt += v;
	}
}
bool EoDbBlockReference::Write(CFile& file) {
	EoDb::Write(file, EoUInt16(EoDb::kGroupReferencePrimitive));
	EoDb::Write(file, m_PenColor);
	EoDb::Write(file, m_LineType);
	EoDb::Write(file, m_strName);
	m_pt.Write(file);
	m_vNormal.Write(file);
	m_vScaleFactors.Write(file);
	EoDb::Write(file, m_dRotation);
	EoDb::Write(file, m_wColCnt);
	EoDb::Write(file, m_wRowCnt);
	EoDb::Write(file, m_dColSpac);
	EoDb::Write(file, m_dRowSpac);

	return true;
}
