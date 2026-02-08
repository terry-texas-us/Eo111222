#pragma once

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
  EoDbFontDefinition m_fontDefinition;
  EoGeReferenceSystem m_ReferenceSystem;
  CString m_text;
  EoInt16 m_textColor{5};

 public:
  EoDbDimension() : EoDbPrimitive(), m_fontDefinition{}, m_ReferenceSystem{}, m_text{}, m_textColor{5} {}

  EoDbDimension(EoInt16 color, EoInt16 lineType, EoGeLine line);

  EoDbDimension(EoInt16 color, EoInt16 lineType, EoGeLine line, EoInt16 textPenColor,
                const EoDbFontDefinition& fontDefinition, const EoGeReferenceSystem& referenceSystem,
                const CString& text);

  EoDbDimension(EoUInt8* buffer);

  EoDbDimension(const EoDbDimension& other);

  ~EoDbDimension() override {}

  const EoDbDimension& operator=(const EoDbDimension& other);

  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbDimension*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  [[nodiscard]] EoGePoint3d GetControlPoint() override { return m_ln.Midpoint(); }
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  [[nodiscard]] EoGePoint3d GoToNextControlPoint() override;
  [[nodiscard]] bool Identical(EoDbPrimitive*) override { return false; }
  [[nodiscard]] bool Is(EoUInt16 type) override { return type == EoDb::kDimensionPrimitive; }
  [[nodiscard]] bool IsInView(AeSysView* view) override;
  [[nodiscard]] bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  [[nodiscard]] EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptInt) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoUInt8* buffer) override;

  void CutAt2Points(const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*,
                    EoDbGroupList*) override;
  void CutAtPoint(EoGePoint3d& point, EoDbGroup*) override;
  void ModifyState() override;

  void GetBoundingBox(EoGePoint3dArray& ptsBox, double dSpacFac);
  [[nodiscard]] const EoDbFontDefinition& FontDefinition() const { return m_fontDefinition; }
  [[nodiscard]] const EoGeLine& Line() const noexcept { return m_ln; }
  
  void GetRefSys(EoGeReferenceSystem& referenceSystem) const { referenceSystem = m_ReferenceSystem; }
  [[nodiscard]] double Length() const { return m_ln.Length(); }
  [[nodiscard]] double RelOfPt(EoGePoint3d pt);
  void SetDefaultNote();
  void BeginPoint(EoGePoint3d pt) { m_ln.begin = pt; }
  void EndPoint(EoGePoint3d pt) { m_ln.end = pt; }

  void SetText(const CString& text) { m_text = text; }

  void SetAlignment(EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment) {
    m_fontDefinition.SetHorizontalAlignment(horizontalAlignment);
    m_fontDefinition.SetVerticalAlignment(verticalAlignment);
  }

  void SetHorizontalAlignment(EoUInt16 horizontalAlignment) {
    m_fontDefinition.SetHorizontalAlignment(horizontalAlignment);
  }
  void SetTextColor(EoInt16 color) { m_textColor = color; }

  void SetVerticalAlignment(EoUInt16 verticalAlignment) { m_fontDefinition.SetVerticalAlignment(verticalAlignment); }

  [[nodiscard]] const CString& Text() const noexcept { return m_text; }
  [[nodiscard]] EoInt16 TextColor() const noexcept { return m_textColor; }

 private:
  static EoUInt16 sm_flags;  // bit 1	clear if dimension selected at note
                             //			  set if dimension selected at line
};
