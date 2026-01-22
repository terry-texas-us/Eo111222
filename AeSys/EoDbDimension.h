#pragma once

#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbDimension : public EoDbPrimitive {
  EoGeLine m_ln;

  EoInt16 m_nTextPenColor{1};
  EoDbFontDefinition m_fontDefinition;
  EoGeReferenceSystem m_ReferenceSystem;
  CString m_strText;

 public:  // Constructors and destructor
  EoDbDimension() : EoDbPrimitive(), m_nTextPenColor(COLOR_BYLAYER), m_fontDefinition(), m_ReferenceSystem(), m_strText() {}

  EoDbDimension(EoInt16 color, EoInt16 lineType, EoGeLine line);
  EoDbDimension(EoInt16 color, EoInt16 lineType, EoGeLine line, EoInt16 textPenColor,
                const EoDbFontDefinition& fontDefinition, const EoGeReferenceSystem& referenceSystem,
                const CString& text);
  EoDbDimension(EoUInt8* buffer);

  EoDbDimension(const EoDbDimension& src);

  ~EoDbDimension() override {}

 public:  // Operators
  const EoDbDimension& operator=(const EoDbDimension& src);

 public:  // Methods - absolute virtuals
  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbDimension*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override { return m_ln.Midpoint(); }
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 type) override { return type == EoDb::kDimensionPrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptInt) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoUInt8* buffer) override;

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
  const EoDbFontDefinition& FontDef() { return m_fontDefinition; }
  const EoGeLine& Line() { return m_ln; }
  void GetPts(EoGePoint3d& begin, EoGePoint3d& end) const {
    begin = m_ln.begin;
    end = m_ln.end;
  }
  void GetRefSys(EoGeReferenceSystem& referenceSystem) const { referenceSystem = m_ReferenceSystem; }
  double Length() { return m_ln.Length(); }
  double RelOfPt(EoGePoint3d pt);
  void SetDefaultNote();
  void BeginPoint(EoGePoint3d pt) { m_ln.begin = pt; }
  void EndPoint(EoGePoint3d pt) { m_ln.end = pt; }
  void SetText(const CString& str) { m_strText = str; }
  void SetTextHorAlign(EoUInt16 w) { m_fontDefinition.HorizontalAlignment(w); }
  void SetTextPenColor(EoInt16 color) { m_nTextPenColor = color; }
  void SetTextVerAlign(EoUInt16 w) { m_fontDefinition.VerticalAlignment(w); }
  const CString& Text() { return m_strText; }
  const EoInt16& TextPenColor() { return m_nTextPenColor; }

 private:
  static EoUInt16 sm_wFlags;  // bit 1	clear if dimension selected at note
                              //			set if dimension selected at line
};
