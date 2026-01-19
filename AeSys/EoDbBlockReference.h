#pragma once

#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "drw_base.h"

class EoDbBlockReference : public EoDbPrimitive {
 private:
  CString m_blockName;
  EoGePoint3d m_insertionPoint;
  EoGeVector3d m_normal;
  EoGeVector3d m_scaleFactors;
  double m_rotation;

  EoUInt16 m_columnCount;
  EoUInt16 m_rowCount;
  double m_columnSpacing;
  double m_rowSpacing;

 public:
  EoDbBlockReference();
  EoDbBlockReference(const CString& strName, const EoGePoint3d& pt);
  EoDbBlockReference(const EoDbBlockReference&);
  EoDbBlockReference(EoUInt16 penColor, EoUInt16 lineType, const CString& name, const EoGePoint3d& point, const EoGeVector3d& normal,
                     const EoGeVector3d scaleFactors, double rotation);
  ~EoDbBlockReference() override {};

  const EoDbBlockReference& operator=(const EoDbBlockReference&);

 public:
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbBlockReference*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void GetAllPts(EoGePoint3dArray& pts) override {
    pts.SetSize(0);
    pts.Add(m_insertionPoint);
  }
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetCtrlPt() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override { return m_insertionPoint; }
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 type) override { return type == EoDb::kGroupReferencePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) override { return false; }
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) override { return false; }
  /// <summary>Evaluates whether a point lies within tolerance specified of block.</summary>
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d v) override { m_insertionPoint += v; }
  void TranslateUsingMask(EoGeVector3d v, const DWORD mask) override;
  bool Write(CFile& file) override;
  void Write(CFile& /* file */, EoByte* /* buffer */) override {};

 public:  // Methods
  EoGeTransformMatrix BuildTransformMatrix(const EoGePoint3d& ptBase);

  EoUInt16& ColumnCount() { return m_columnCount; }
  double& ColumnSpacing() { return m_columnSpacing; }
  CString GetName() { return m_blockName; }
  double GetRotation() const { return m_rotation; }
  EoGeVector3d GetScaleFactors() const { return m_scaleFactors; }
  EoGePoint3d& InsertionPoint() { return m_insertionPoint; }
  EoGeVector3d Normal() const { return m_normal; }
  void Read(CFile&);
  EoUInt16& RowCount() { return m_rowCount; }
  double& RowSpacing() { return m_rowSpacing; }

  void SetColumns(EoUInt16 columns) { m_columnCount = columns; }
  void SetColumnSpacing(double columnSpacing) { m_columnSpacing = columnSpacing; }
  void SetName(const CString& name) { m_blockName = name; }
  void SetNormal(const EoGeVector3d& normal) { m_normal = normal; }

  void SetInsertionPoint(const DRW_Coord& point) {
    m_insertionPoint.x = point.x;
    m_insertionPoint.y = point.y;
    m_insertionPoint.z = point.z;
  }
  void SetInsertionPoint(const EoGePoint3d& point) { m_insertionPoint = point; }
  void SetRotation(double rotation) { m_rotation = rotation; }
  void SetRows(EoUInt16 rows) { m_rowCount = rows; }
  void SetRowSpacing(double rowSpacing) { m_rowSpacing = rowSpacing; }
  void SetScaleFactors(const EoGeVector3d& scaleFactors) { m_scaleFactors = scaleFactors; }
};
