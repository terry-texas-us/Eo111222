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

class EoDbPoint : public EoDbPrimitive {
  std::int16_t m_pointStyle;
  EoGePoint3d m_Point;
  std::uint16_t m_NumberOfDatums;
  double* m_Data;

 public:  // Constructors and destructor
  EoDbPoint();
  EoDbPoint(const EoGePoint3d& point);
  EoDbPoint(std::int16_t penColor, std::int16_t pointStyle, const EoGePoint3d& point);
  EoDbPoint(std::int16_t penColor, std::int16_t pointStyle, const EoGePoint3d& point, std::uint16_t numberOfDatums, double* data);

  EoDbPoint(const EoDbPoint& src);

  ~EoDbPoint() override;

  const EoDbPoint& operator=(const EoDbPoint& src);

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbPoint*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* context) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override { return m_Point; }
  bool Identical(EoDbPrimitive* primitive) override { return m_Point == static_cast<EoDbPoint*>(primitive)->m_Point; }
  bool Is(std::uint16_t wType) override { return wType == EoDb::kPointPrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(const EoGeTransformMatrix&) override;
  void Translate(const EoGeVector3d& v) override { m_Point += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  double GetDat(std::uint16_t wDat) { return (m_Data[wDat]); }
  EoGePoint3d GetPt() const { return m_Point; }
  std::int16_t& PointStyle() { return m_pointStyle; }
  void ModifyState() override;
  void SetDat(std::uint16_t, double*);
  void SetPt(EoGePoint3d pt) { m_Point = pt; }
  void SetPoint(double x, double y, double z);
};
