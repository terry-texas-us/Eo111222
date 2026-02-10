#pragma once

#include <cstdint>

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

class EoDbLine : public EoDbPrimitive {
  EoGeLine m_line;

 public:
  EoDbLine() = default;

  EoDbLine(const EoGeLine& line);

  EoDbLine(std::int16_t color, std::int16_t lineType, EoGeLine line);

  EoDbLine(const EoGePoint3d& begin, const EoGePoint3d& end);

  EoDbLine(std::int16_t color, std::int16_t lineType, const EoGePoint3d& begin, const EoGePoint3d& end);

  EoDbLine(const EoDbLine& other);

  ~EoDbLine() override = default;

  const EoDbLine& operator=(const EoDbLine& other);

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbLine*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override { return m_line.Midpoint(); }
  /** @brief Gets the extents of the line in the current view.
   * @param view The view for which to get the extents.
   * @param[in/out] minPoint Receives the minimum point of the extents. 
   * @param[in/out] maxPoint Receives the maximum point of the extents.
   * @param transformMatrix The transformation matrix to apply to the points before calculating the extents.
   */
  void GetExtents(AeSysView* view, EoGePoint3d& minPoint, EoGePoint3d& maxPoint, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive* primitive) override;
  bool Is(std::uint16_t wType) override { return wType == EoDb::kLinePrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(const EoGeTransformMatrix& transformMatrix) override;
  void Translate(const EoGeVector3d& v) override { m_line += v; }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t*) override;

  void CutAt2Points(
      const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*, EoDbGroupList*) override;

  /** @brief Cuts a line at a point.
   * @param point Point for the line cut.
   * @param group Group to receive the new line segment if the cut is successful.
   * @note Line segment from the point to the end of the line goes in group.
   */
  void CutAtPoint(const EoGePoint3d& point, EoDbGroup* group) override;

  int IsWithinArea(const EoGePoint3d& lowerLeft, const EoGePoint3d& upperRight, EoGePoint3d* clippedPoints) override;

  [[nodiscard]] const EoGePoint3d& Begin() const noexcept { return m_line.begin; }
  [[nodiscard]] const EoGePoint3d& End() const noexcept { return m_line.end; }
  [[nodiscard]] const EoGeLine& Line() const noexcept { return m_line; }
  [[nodiscard]] double Length() const noexcept { return (m_line.Length()); }

  /** @brief Projects a point onto the line.
   * @param point The point to project onto the line.
   * @return The projected point on the line.
   */
  [[nodiscard]] EoGePoint3d ProjectPointToLine(const EoGePoint3d& point) { return (m_line.ProjectPointToLine(point)); }
  double RelOfPt(const EoGePoint3d& point);

  void SetLine(const EoGeLine& line) { m_line = line; }
  void SetBeginPoint(const EoGePoint3d& point) { m_line.begin = point; }
  void SetEndPoint(const EoGePoint3d& point) { m_line.end = point; }

  void Square(AeSysView* view);
};
