#pragma once

#include <cstdint>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoDxfGeometry.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

/** @brief Primitive representing a DXF VIEWPORT entity in paper space.
 *
 *  Stores the viewport's paper-space center, width/height, and model-space view
 *  parameters needed for DXF round-trip fidelity.  Display is deliberately
 *  minimal (boundary rectangle only) — the full paper-space clipping pipeline
 *  is deferred to a later phase.
 */
class EoDbViewport : public EoDbPrimitive {
 public:
  EoDbViewport() = default;
  EoDbViewport(const EoDbViewport& other);
  ~EoDbViewport() override = default;

  EoDbViewport& operator=(EoDbViewport other) noexcept;

  /// @brief Swaps all members (base and derived) with @p other.
  void swap(EoDbViewport& other) noexcept;
  friend void swap(EoDbViewport& lhs, EoDbViewport& rhs) noexcept { lhs.swap(rhs); }

  // --- EoDbPrimitive virtual overrides ---
  void AddReportToMessageList(const EoGePoint3d& point) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbViewport*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*& primitive) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& extra) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() noexcept override { return m_centerPoint; }
  void GetExtents(AeSysView* view, EoGePoint3d& minPoint, EoGePoint3d& maxPoint,
      const EoGeTransformMatrix& transformMatrix) override;
  EoGePoint3d GoToNextControlPoint() noexcept override { return m_centerPoint; }
  bool Identical(EoDbPrimitive* primitive) override;
  bool Is(std::uint16_t type) noexcept override { return type == EoDb::kViewportPrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& intersectionPoint) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeft, EoGePoint3d upperRight) override;
  void Transform(const EoGeTransformMatrix& transformMatrix) override;
  void Translate(const EoGeVector3d& translateVector) noexcept override { m_centerPoint += translateVector; }
  void TranslateUsingMask(EoGeVector3d translateVector, const DWORD mask) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  /// @brief Reads a viewport primitive from a PEG file stream (type code kViewportPrimitive).
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbViewport.
  static EoDbViewport* ReadFromPeg(CFile& file);

  /// @brief Populates an EoDxfViewport from this primitive's stored properties for DXF export.
  /// @param viewport The DXF viewport entity to populate.
  void ExportToDxf(EoDxfViewport& viewport) const;

  // --- Accessors ---
  [[nodiscard]] const EoGePoint3d& CenterPoint() const noexcept { return m_centerPoint; }
  [[nodiscard]] double Width() const noexcept { return m_width; }
  [[nodiscard]] double Height() const noexcept { return m_height; }
  [[nodiscard]] std::int16_t ViewportStatus() const noexcept { return m_viewportStatus; }
  [[nodiscard]] std::int16_t ViewportId() const noexcept { return m_viewportId; }

  // --- Mutators ---
  void SetCenterPoint(const EoGePoint3d& centerPoint) noexcept { m_centerPoint = centerPoint; }
  void SetWidth(double width) noexcept { m_width = width; }
  void SetHeight(double height) noexcept { m_height = height; }
  void SetViewportStatus(std::int16_t viewportStatus) noexcept { m_viewportStatus = viewportStatus; }
  void SetViewportId(std::int16_t viewportId) noexcept { m_viewportId = viewportId; }
  void SetViewCenter(const EoGePoint3d& viewCenter) noexcept { m_viewCenter = viewCenter; }
  void SetSnapBasePoint(const EoGePoint3d& snapBasePoint) noexcept { m_snapBasePoint = snapBasePoint; }
  void SetSnapSpacing(const EoGePoint3d& snapSpacing) noexcept { m_snapSpacing = snapSpacing; }
  void SetGridSpacing(const EoGePoint3d& gridSpacing) noexcept { m_gridSpacing = gridSpacing; }
  void SetViewDirection(const EoGePoint3d& viewDirection) noexcept { m_viewDirection = viewDirection; }
  void SetViewTargetPoint(const EoGePoint3d& viewTargetPoint) noexcept { m_viewTargetPoint = viewTargetPoint; }
  void SetLensLength(double lensLength) noexcept { m_lensLength = lensLength; }
  void SetFrontClipPlane(double frontClipPlane) noexcept { m_frontClipPlane = frontClipPlane; }
  void SetBackClipPlane(double backClipPlane) noexcept { m_backClipPlane = backClipPlane; }
  void SetViewHeight(double viewHeight) noexcept { m_viewHeight = viewHeight; }
  void SetSnapAngle(double snapAngle) noexcept { m_snapAngle = snapAngle; }
  void SetTwistAngle(double twistAngle) noexcept { m_twistAngle = twistAngle; }

 private:
  // Paper-space geometry (DCS)
  EoGePoint3d m_centerPoint{};  // DXF group codes 10, 20, 30
  double m_width{};  // DXF group code 40
  double m_height{};  // DXF group code 41

  // Viewport identity
  std::int16_t m_viewportStatus{};  // DXF group code 68
  std::int16_t m_viewportId{};  // DXF group code 69

  // Model-space view parameters (preserved for round-trip)
  EoGePoint3d m_viewCenter{};  // DXF group codes 12, 22 (2D in DCS)
  EoGePoint3d m_snapBasePoint{};  // DXF group codes 13, 23
  EoGePoint3d m_snapSpacing{};  // DXF group codes 14, 24
  EoGePoint3d m_gridSpacing{};  // DXF group codes 15, 25
  EoGePoint3d m_viewDirection{0.0, 0.0, 1.0};  // DXF group codes 16, 26, 36
  EoGePoint3d m_viewTargetPoint{};  // DXF group codes 17, 27, 37
  double m_lensLength{};  // DXF group code 42
  double m_frontClipPlane{};  // DXF group code 43
  double m_backClipPlane{};  // DXF group code 44
  double m_viewHeight{};  // DXF group code 45
  double m_snapAngle{};  // DXF group code 50
  double m_twistAngle{};  // DXF group code 51
};
