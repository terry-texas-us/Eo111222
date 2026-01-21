#pragma once
#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>
#include <utility>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "drw_base.h"

// Represents circle primitives using DXF style center, radius and extrusion (OCS)
class EoDbCircle : public EoDbPrimitive {
 protected:
  EoGePoint3d m_center{};
  double m_radius{0.0};
  EoGeVector3d m_extrusion{EoGeVector3d::kZAxis};

 public:
  EoDbCircle() = default;

  // Construct from DRW parameters
  EoDbCircle(const EoGePoint3d& center, double radius, const EoGeVector3d& extrusion = EoGeVector3d::kZAxis);

  EoDbCircle(EoInt16 color, EoInt16 lineType, const EoGePoint3d& center, double radius,
             const EoGeVector3d& extrusion = EoGeVector3d::kZAxis);

  EoDbCircle(const EoDbCircle& other);
  ~EoDbCircle() override = default;

  const EoDbCircle& operator=(const EoDbCircle& other);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbCircle*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*& primitive) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPts(EoGePoint3dArray& pts) override;
  EoGePoint3d GetCtrlPt() override;
  void GetExtents(AeSysView* view, EoGePoint3d& minPt, EoGePoint3d& maxPt, EoGeTransformMatrix& tm) override;
  EoGePoint3d GoToNxtCtrlPt() override;
  bool Identical(EoDbPrimitive*) override;
  bool Is(EoUInt16) override;
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& ptsInt) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& outPt) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d a, EoGePoint3d b) override;
  void Transform(EoGeTransformMatrix& tm) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d translate, const DWORD mask) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoUInt8* buffer) override;

 public:  // Optional overrides
  void CutAtPt(EoGePoint3d& pt, EoDbGroup* group) override;
  void CutAt2Pts(EoGePoint3d* pt, EoDbGroupList* groups, EoDbGroupList* newGroups) override;
  int IsWithinArea(EoGePoint3d a, EoGePoint3d b, EoGePoint3d* arr) override;

 public:  // Accessors
  const EoGePoint3d& CenterPoint() const noexcept { return m_center; }
  double Radius() const noexcept { return m_radius; }
  const EoGeVector3d& Extrusion() const noexcept { return m_extrusion; }
  void SetCenter(const EoGePoint3d& p) { m_center = p; }
  void SetRadius(double r) { m_radius = r; }
  void SetExtrusion(const EoGeVector3d& v) { m_extrusion = v; }
};
