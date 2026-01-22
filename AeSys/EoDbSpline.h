#pragma once

#include "EoDbPrimitive.h"

class EoDbSpline : public EoDbPrimitive {
 private:
  EoGePoint3dArray m_pts;

 public:  // Constructors and destructor
  EoDbSpline() {}
  EoDbSpline(EoUInt8* buffer, int version);
  EoDbSpline(EoUInt16, EoGePoint3d*);
  EoDbSpline(EoGePoint3dArray& points);
  EoDbSpline(EoInt16 penColor, EoInt16 lineType, EoGePoint3dArray& points);
  EoDbSpline(const EoDbSpline&);

  ~EoDbSpline() override {}

 public:  // Operators
  const EoDbSpline& operator=(const EoDbSpline&);

 public:  // Methods - absolute virtuals
  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbSpline*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void GetAllPoints(EoGePoint3dArray& pts) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 type) override { return type == EoDb::kSplinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* /* view */, const EoGePoint4d& /* point */) override { return false; }
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) override { return false; }
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoUInt8* buffer) override;

  int GenPts(const int iOrder, EoGePoint3dArray& pts);
  void SetPt(EoUInt16 w, EoGePoint3d pt) { m_pts[w] = pt; }
};
