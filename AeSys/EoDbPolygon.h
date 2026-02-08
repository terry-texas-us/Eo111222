#pragma once

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbPolygon : public EoDbPrimitive {
  static EoUInt16 sm_EdgeToEvaluate;
  static EoUInt16 sm_Edge;
  static EoUInt16 sm_PivotVertex;
  static EoDb::PolygonStyle sm_SpecialPolygonStyle;

  EoGePoint3d m_hatchOrigin;
  EoGeVector3d m_positiveX;
  EoGeVector3d m_positiveY;
  EoGePoint3d* m_vertices{};
  EoDb::PolygonStyle m_polygonStyle;
  EoInt16 m_fillStyleIndex;
  EoUInt16 m_numberOfVertices;

 public:  // Constructors and destructor
  EoDbPolygon();
  EoDbPolygon(EoUInt8* buffer, int version);

  /** @brief Constructs an EoDbPolygon object from an array of 3D points.
   *  @param points An array of 3D points that define the vertices of the polygon. Must contain at least 3 points for proper initialization of the plane vectors.
   */
  EoDbPolygon(EoGePoint3dArray& points);

  EoDbPolygon(EoUInt16, EoGePoint3d*);
  EoDbPolygon(const EoGePoint3d& origin, const EoGeVector3d& xAxis, const EoGeVector3d& yAxis, EoGePoint3dArray& pts);
  EoDbPolygon(EoInt16 color, EoDb::PolygonStyle style, EoInt16 styleIndex, const EoGePoint3d& origin,
              const EoGeVector3d& xAxis, const EoGeVector3d& yAxis, EoGePoint3dArray& points);
  EoDbPolygon(EoUInt16, EoGePoint3d, EoGeVector3d, EoGeVector3d, const EoGePoint3d*);

  EoDbPolygon(const EoDbPolygon& other);

 public:
  const EoDbPolygon& operator=(const EoDbPolygon& other);

  [[nodiscard]] EoGePoint3d& operator[](int i) { return m_vertices[i]; }
  [[nodiscard]] const EoGePoint3d& operator[](int i) const { return m_vertices[i]; }

  ~EoDbPolygon() override;

 public:  // Methods - absolute virtuals
  void AddReportToMessageList(EoGePoint3d) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbPolygon*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override;
  bool Identical(EoDbPrimitive*) override { return false; }
  bool Is(EoUInt16 wType) override { return wType == EoDb::kPolygonPrimitive; }
  bool IsInView(AeSysView* view) override;
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Transform(EoGeTransformMatrix&) override;
  void Translate(EoGeVector3d translate) override;
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoUInt8* buffer) override;

  CString FormatIntStyle();
  [[nodiscard]] const EoDb::PolygonStyle& PolygonStyle() { return m_polygonStyle; }
  [[nodiscard]] const EoInt16& FillStyleIndex() { return m_fillStyleIndex; }
  [[nodiscard]] EoGePoint3d Vertex(int i) { return m_vertices[i]; }
  [[nodiscard]] int NumberOfVertices() const { return m_numberOfVertices; }
  void ModifyState() override;
  bool PivotOnControlPoint(AeSysView* view, const EoGePoint4d&) override;
  void SetPolygonStyle(const EoDb::PolygonStyle n) { m_polygonStyle = n; }
  void SetFillStyleIndex(const EoInt16 fillStyleIndex) { m_fillStyleIndex = fillStyleIndex; }
  void SetHatRefVecs(double, double, double);

 private:
  EoUInt16 SwingVertex() const;

 public:
  static void SetSpecialPolygonStyle(EoDb::PolygonStyle polygonStyle) { sm_SpecialPolygonStyle = polygonStyle; }
  static EoUInt16& EdgeToEvaluate() { return sm_EdgeToEvaluate; }
  static EoUInt16& Edge() { return sm_Edge; }
};
/// <summary>A fill area set primative with interior style hatch is generated using ines.</summary>
// Parameters:	deviceContext
//				iSets		number of point lists
//				iPtLstsId	starting indicies for point lists
void DisplayFilAreaHatch(AeSysView* view, CDC* deviceContext, EoGeTransformMatrix& tm, const int iSets,
                         const int* iPtLstsId, EoGePoint3d*);
/// <summary>Generates polygon.</summary>
// The polygon is closed automatically by drawing a line from the last vertex to the first.
// Arrays of vertices are previously modelview transformed and clipped to view volume.
void Polygon_Display(AeSysView* view, CDC* deviceContext, EoGePoint4dArray& pointsArray);
