#pragma once

#include <cstdint>
#include <utility>

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
  CString m_blockName;
  EoGePoint3d m_insertionPoint;
  EoGeVector3d m_normal;
  EoGeVector3d m_scaleFactors;
  double m_rotation;

  std::uint16_t m_columnCount;
  std::uint16_t m_rowCount;
  double m_columnSpacing;
  double m_rowSpacing;

 public:
  EoDbBlockReference();
  EoDbBlockReference(const CString& strName, const EoGePoint3d& pt);
  EoDbBlockReference(const EoDbBlockReference&);
  EoDbBlockReference(std::uint16_t color, std::uint16_t lineType, const CString& name, const EoGePoint3d& point,
      const EoGeVector3d& normal, const EoGeVector3d scaleFactors, double rotation);
  ~EoDbBlockReference() override = default;

  const EoDbBlockReference& operator=(const EoDbBlockReference&);

 public:
  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbBlockReference*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override { return m_insertionPoint; }
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(std::uint16_t type) override { return type == EoDb::kGroupReferencePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(const EoGeTransformMatrix&) override;
  void Translate(const EoGeVector3d& v) override { m_insertionPoint += v; }
  void TranslateUsingMask(EoGeVector3d v, const DWORD mask) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override {
    (void)file;
    (void)buffer;
  };

 public:
  EoGeTransformMatrix BuildTransformMatrix(const EoGePoint3d& insertionPoint) const;

  std::uint16_t ColumnCount() const noexcept { return m_columnCount; }
  double ColumnSpacing() const noexcept { return m_columnSpacing; }
  const CString& BlockName() const noexcept { return m_blockName; }
  double Rotation() const noexcept { return m_rotation; }
  const EoGeVector3d& ScaleFactors() const noexcept { return m_scaleFactors; }
  const EoGePoint3d& InsertionPoint() const noexcept { return m_insertionPoint; }
  const EoGeVector3d& Normal() const noexcept { return m_normal; }
  std::uint16_t RowCount() const noexcept { return m_rowCount; }
  double RowSpacing() const noexcept { return m_rowSpacing; }

  void SetColumns(std::uint16_t columns) { m_columnCount = columns; }
  void SetColumnSpacing(double columnSpacing) { m_columnSpacing = columnSpacing; }
  void SetName(CString name) { m_blockName = std::move(name); }
  void SetNormal(EoGeVector3d normal) { m_normal = std::move(normal); }
  void SetInsertionPoint(const DRW_Coord& point);
  void SetInsertionPoint(const EoGePoint3d& point) { m_insertionPoint = point; }
  void SetRotation(double rotation) { m_rotation = rotation; }
  void SetRows(std::uint16_t rows) { m_rowCount = rows; }
  void SetRowSpacing(double rowSpacing) { m_rowSpacing = rowSpacing; }
  void SetScaleFactors(const EoGeVector3d& scaleFactors) { m_scaleFactors = scaleFactors; }
};
