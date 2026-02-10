#pragma once

#include <cstdint>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbSpline : public EoDbPrimitive {
  EoGePoint3dArray m_pts;

 public:  // Constructors and destructor
  EoDbSpline() {}
  EoDbSpline(std::uint8_t* buffer, int version);
  EoDbSpline(std::uint16_t, EoGePoint3d*);
  EoDbSpline(EoGePoint3dArray& points);
  EoDbSpline(std::int16_t penColor, std::int16_t lineType, EoGePoint3dArray& points);
  EoDbSpline(const EoDbSpline&);

  ~EoDbSpline() override = default;

  const EoDbSpline& operator=(const EoDbSpline&);

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbSpline*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void GetAllPoints(EoGePoint3dArray& pts) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(std::uint16_t type) override { return type == EoDb::kSplinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(const EoGeTransformMatrix&) override;
  void Translate(const EoGeVector3d& v) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  int GenPts(const int iOrder, EoGePoint3dArray& pts);
  void SetPt(std::uint16_t w, EoGePoint3d pt) { m_pts[w] = pt; }
};
