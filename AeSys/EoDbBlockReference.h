#pragma once

#include "EoDbPrimitive.h"
#include "EoGeLine.h"

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

 public:  // Constructors and destructor
  EoDbBlockReference();
  EoDbBlockReference(const CString& strName, const EoGePoint3d& pt);
  EoDbBlockReference(const EoDbBlockReference&);
  EoDbBlockReference(EoUInt16 penColor, EoUInt16 lineType, const CString& name, const EoGePoint3d& point,
                     const EoGeVector3d& normal, const EoGeVector3d scaleFactors, double rotation);
  ~EoDbBlockReference() override {};

 public:  // Operators
  const EoDbBlockReference& operator=(const EoDbBlockReference&);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbBlockReference*>(primitive); }
  EoGeTransformMatrix BuildTransformMatrix(const EoGePoint3d& ptBase);
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void GetAllPts(EoGePoint3dArray& pts) override {
    pts.SetSize(0);
    pts.Add(m_pt);
  }
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetCtrlPt() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override { return m_pt; }
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 type) override { return type == EoDb::kGroupReferencePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) override { return false; }
  void Read(CFile&);
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) override { return false; }
  /// <summary>Evaluates whether a point lies within tolerance specified of block.</summary>
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d v) override { m_pt += v; }
  void TranslateUsingMask(EoGeVector3d v, const DWORD mask) override;
  bool Write(CFile& file) override;
  void Write(CFile& /* file */, EoByte* /* buffer */) override {};

 public:  // Methods
  EoUInt16& ColCnt() { return m_wColCnt; }
  double& ColSpacing() { return m_dColSpac; }
  CString GetName() { return m_strName; }
  double GetRotation() { return m_dRotation; }
  EoGeVector3d GetScaleFactors() { return m_vScaleFactors; }
  EoGePoint3d& InsPt() { return m_pt; }
  EoGeVector3d Normal() { return m_vNormal; }
  EoUInt16& RowCnt() { return m_wRowCnt; }
  double& RowSpacing() { return m_dRowSpac; }

  void SetNormal(const EoGeVector3d& normal) { m_vNormal = normal; }
  void SetPosition(const EoGePoint3d& position) { m_pt = position; }
  void SetScaleFactors(const EoGeVector3d& scaleFactors) { m_vScaleFactors = scaleFactors; }
  void SetRotation(double rotation) { m_dRotation = rotation; }
  void SetRows(EoUInt16 rows) { m_wRowCnt = rows; }
  void SetRowSpacing(double rowSpacing) { m_dRowSpac = rowSpacing; }
  void SetColumns(EoUInt16 columns) { m_wColCnt = columns; }
  void SetColumnSpacing(double columnSpacing) { m_dColSpac = columnSpacing; }
};
