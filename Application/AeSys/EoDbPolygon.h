#pragma once

#include <cstdint>
#include <vector>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoDxfHatch.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbPolygon : public EoDbPrimitive {
  static std::uint16_t sm_EdgeToEvaluate;
  static std::uint16_t sm_Edge;
  static int sm_pivotVertex;
  static EoDb::PolygonStyle sm_SpecialPolygonStyle;

  EoGePoint3d m_hatchOrigin;
  EoGeVector3d m_positiveX;
  EoGeVector3d m_positiveY;
  EoGePoint3d* m_vertices{};
  EoDb::PolygonStyle m_polygonStyle{EoDb::PolygonStyle::Hollow};
  std::int16_t m_fillStyleIndex{};
  std::uint16_t m_numberOfVertices{};
  std::int16_t m_hatchPatternDoubleFlag{};  ///< DXF group code 77 passthrough
  std::vector<EoDxfHatchPatternDefinitionLine> m_patternDefinitionLines;  ///< DXF pattern lines passthrough

 public:
  EoDbPolygon();
  EoDbPolygon(std::uint8_t* buffer, int version);

  /** * @brief Constructs an EoDbPolygon object from an array of 3D points.
   *
   * The first three points are used to define the plane of the polygon and the
   * orientation of the hatch pattern. If fewer than three points are provided,
   * the polygon defaults to the XY plane with standard orientation.
   *
   * @param points An array of 3D points that define the vertices of the polygon.
   */
  EoDbPolygon(EoGePoint3dArray& points);

  EoDbPolygon(std::uint16_t, EoGePoint3d*);
  EoDbPolygon(const EoGePoint3d& origin, const EoGeVector3d& xAxis, const EoGeVector3d& yAxis, EoGePoint3dArray& pts);
  EoDbPolygon(std::int16_t color,
      EoDb::PolygonStyle style,
      std::int16_t styleIndex,
      const EoGePoint3d& origin,
      const EoGeVector3d& xAxis,
      const EoGeVector3d& yAxis,
      EoGePoint3dArray& points);
  EoDbPolygon(std::uint16_t, EoGePoint3d, EoGeVector3d, EoGeVector3d, const EoGePoint3d*);

  EoDbPolygon(const EoDbPolygon& other);

  const EoDbPolygon& operator=(const EoDbPolygon& other);

  [[nodiscard]] EoGePoint3d& operator[](int i) noexcept { return m_vertices[i]; }
  [[nodiscard]] const EoGePoint3d& operator[](int i) const noexcept { return m_vertices[i]; }

  ~EoDbPolygon() override;

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbPolygon*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice) override;
  void ExportToDxf(EoDxfInterface* writer) const override;
  [[nodiscard]] CString TypeLabel() const override { return L"Polygon"; }
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(std::uint16_t type) noexcept override { return type == EoDb::kPolygonPrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  bool IsWhollyContainedByRectangle(AeSysView* view, EoGePoint3d lowerLeft, EoGePoint3d upperRight) override;
  void Transform(const EoGeTransformMatrix&) override;
  void Translate(const EoGeVector3d& v) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  /// @brief Reads a polygon primitive from a PEG file stream (type code kPolygonPrimitive).
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbPolygon.
  static EoDbPolygon* ReadFromPeg(CFile& file);

  CString FormatIntStyle();
  [[nodiscard]] const EoDb::PolygonStyle& PolygonStyle() const noexcept { return m_polygonStyle; }
  [[nodiscard]] std::int16_t FillStyleIndex() const noexcept { return m_fillStyleIndex; }
  [[nodiscard]] EoGePoint3d Vertex(int i) const noexcept { return m_vertices[i]; }
  [[nodiscard]] int NumberOfVertices() const noexcept { return m_numberOfVertices; }
  void ModifyState() override;
  bool PivotOnControlPoint(AeSysView* view, const EoGePoint4d&) override;
  void SetPolygonStyle(const EoDb::PolygonStyle n) noexcept { m_polygonStyle = n; }
  void SetFillStyleIndex(const std::int16_t fillStyleIndex) noexcept { m_fillStyleIndex = fillStyleIndex; }
  void SetHatchPatternDoubleFlag(std::int16_t flag) noexcept { m_hatchPatternDoubleFlag = flag; }
  void SetPatternDefinitionLines(const std::vector<EoDxfHatchPatternDefinitionLine>& lines) {
    m_patternDefinitionLines = lines;
  }
  void SetHatRefVecs(double, double, double);

 private:
  std::uint16_t SwingVertex() const;

 public:
  static void SetSpecialPolygonStyle(EoDb::PolygonStyle polygonStyle) noexcept { sm_SpecialPolygonStyle = polygonStyle; }
  static std::uint16_t& EdgeToEvaluate() noexcept { return sm_EdgeToEvaluate; }
  static std::uint16_t& Edge() noexcept { return sm_Edge; }
};

/// @brief Renders a clipped polygon using the current polygon interior style.
/// @param view The active view for client-space projection.
/// @param renderDevice The GDI render device.
/// @param ndcPoints NDC-space polygon vertices (already model-view transformed and clipped).
void Polygon_Display(AeSysView* view, EoGsRenderDevice* renderDevice, EoGePoint4dArray& ndcPoints);
