#pragma once
#include <afxstr.h>

#include "EoDbFontDefinition.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGeReferenceSystem.h"

class EoDbDimension : public EoDbPrimitive {
  EoGeLine m_ln;

  EoInt16 m_nTextPenColor{1};
  EoDbFontDefinition m_fd;
  EoGeReferenceSystem m_ReferenceSystem;
  CString m_strText;

 public:  // Constructors and destructor
  EoDbDimension() : EoDbPrimitive(), m_nTextPenColor(EoDbPrimitive::PENCOLOR_BYLAYER), m_fd(), m_ReferenceSystem(), m_strText() {}

  EoDbDimension(EoInt16 penColor, EoInt16 lineType, EoGeLine line);
  EoDbDimension(EoInt16 penColor, EoInt16 lineType, EoGeLine line, EoInt16 textPenColor, const EoDbFontDefinition& fontDefinition,
                const EoGeReferenceSystem& referenceSystem, const CString& text);
  EoDbDimension(EoByte* buffer);

  EoDbDimension(const EoDbDimension& src);

  ~EoDbDimension() override {}

 public:  // Operators
  const EoDbDimension& operator=(const EoDbDimension& src);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbDimension*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPts(EoGePoint3dArray& pts) override;
  EoGePoint3d GetCtrlPt() override { return m_ln.Midpoint(); }
  /// <summary>Determines the extent.</summary>
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 type) override { return type == EoDb::kDimensionPrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  /// <summary>Evaluates whether a line intersects a dimension line.</summary>
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptInt) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoByte* buffer) override;

 public:  // Methods - virtuals
  /// <summary>Cuts a dimension line at two points.</summary>
  // Parameters:	pt
  //				groups	group to place optional line not defined by the cut
  //						points
  //				newGroups group to place line defined by the cut points
  void CutAt2Pts(EoGePoint3d*, EoDbGroupList*, EoDbGroupList*) override;
  void CutAtPt(EoGePoint3d&, EoDbGroup*) override;
  void ModifyState() override;

 public:  // Methods
  void GetBoundingBox(EoGePoint3dArray& ptsBox, double dSpacFac);
  const EoDbFontDefinition& FontDef() { return m_fd; }
  const EoGeLine& Line() { return m_ln; }
  void GetPts(EoGePoint3d& ptBeg, EoGePoint3d& ptEnd) {
    ptBeg = m_ln.begin;
    ptEnd = m_ln.end;
  }
  void GetRefSys(EoGeReferenceSystem& referenceSystem) { referenceSystem = m_ReferenceSystem; }
  double Length() { return m_ln.Length(); }
  double RelOfPt(EoGePoint3d pt);
  void SetDefaultNote();
  void BeginPoint(EoGePoint3d pt) { m_ln.begin = pt; }
  void EndPoint(EoGePoint3d pt) { m_ln.end = pt; }
  void SetText(const CString& str) { m_strText = str; }
  void SetTextHorAlign(EoUInt16 w) { m_fd.HorizontalAlignment(w); }
  void SetTextPenColor(EoInt16 nPenColor) { m_nTextPenColor = nPenColor; }
  void SetTextVerAlign(EoUInt16 w) { m_fd.VerticalAlignment(w); }
  const CString& Text() { return m_strText; }
  const EoInt16& TextPenColor() { return m_nTextPenColor; }

 private:
  static EoUInt16 sm_wFlags;  // bit 1	clear if dimension selected at note
                              //			set if dimension selected at line
};
