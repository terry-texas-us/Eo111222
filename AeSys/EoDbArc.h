#pragma once

#include <Windows.h>
#include <afx.h>
#include <afxstr.h>
#include <afxwin.h>

#include "AeSysView.h"
#include "EoDbCircle.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

/** * @class EoDbArc
 * @brief Represents a circular arc primitive in a CAD drawing.
 *
 * The EoDbArc class inherits from EoDbCircle and encapsulates the properties and behaviors of a circular arc,
 * defined by its center point, radius, start angle, end angle, and extrusion vector. It provides methods for
 * displaying the arc, transforming it, selecting it using various methods, and writing it to a file.
 */
class EoDbArc : public EoDbCircle {
 private:
  double m_startAngle{0.0};
  double m_endAngle{0.0};

 public:
  EoDbArc() = default;

  EoDbArc(const EoGePoint3d& center, double radius, double startAngle, double endAngle,
          const EoGeVector3d& extrusion = EoGeVector3d::kZAxis);

  // Construct with explicit pen/linetype
  EoDbArc(EoInt16 color, EoInt16 lineType, const EoGePoint3d& center, double radius, double startAngle, double endAngle,
          const EoGeVector3d& extrusion = EoGeVector3d::kZAxis);

  EoDbArc(const EoDbArc& other);
  ~EoDbArc() override = default;

  const EoDbArc& operator=(const EoDbArc& other);

 public:  // Methods - absolute virtuals
  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbArc*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*& primitive) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d& minPt, EoGePoint3d& maxPt, EoGeTransformMatrix& tm) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override;
  bool Is(EoUInt16) override;
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) override;
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
  double StartAngle() const noexcept { return m_startAngle; }
  double EndAngle() const noexcept { return m_endAngle; }
  void SetAngles(double startAngle, double endAngle) {
    m_startAngle = startAngle;
    m_endAngle = endAngle;
  }
};
