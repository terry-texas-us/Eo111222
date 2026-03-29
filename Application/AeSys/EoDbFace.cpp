#include "Stdafx.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbFace.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoDxfInterface.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderState.h"

// ── Constructors / Assignment ────────────────────────────────────────────────

EoDbFace::EoDbFace(const EoDbFace& other)
    : EoDbPrimitive(other),
      m_extrusion(other.m_extrusion),
      m_sourceType(other.m_sourceType),
      m_edgeFlags(other.m_edgeFlags),
      m_vertexCount(other.m_vertexCount) {
  for (int i = 0; i < m_vertexCount; ++i) { m_vertices[i] = other.m_vertices[i]; }
}

const EoDbFace& EoDbFace::operator=(const EoDbFace& other) {
  if (this != &other) {
    EoDbPrimitive::operator=(other);
    for (int i = 0; i < 4; ++i) { m_vertices[i] = other.m_vertices[i]; }
    m_extrusion = other.m_extrusion;
    m_sourceType = other.m_sourceType;
    m_edgeFlags = other.m_edgeFlags;
    m_vertexCount = other.m_vertexCount;
  }
  return *this;
}

// ── Factory Methods ──────────────────────────────────────────────────────────

EoDbFace* EoDbFace::CreateFrom3dFace(const EoGePoint3d& v0, const EoGePoint3d& v1, const EoGePoint3d& v2,
    const EoGePoint3d& v3, std::uint8_t edgeFlags) {
  auto* face = new EoDbFace();
  face->m_vertices[0] = v0;
  face->m_vertices[1] = v1;
  face->m_vertices[2] = v2;
  face->m_vertices[3] = v3;
  face->m_sourceType = SourceType::Face3d;
  face->m_edgeFlags = edgeFlags;
  face->m_vertexCount = 4;
  return face;
}

EoDbFace* EoDbFace::CreateTriangleFrom3dFace(
    const EoGePoint3d& v0, const EoGePoint3d& v1, const EoGePoint3d& v2, std::uint8_t edgeFlags) {
  auto* face = new EoDbFace();
  face->m_vertices[0] = v0;
  face->m_vertices[1] = v1;
  face->m_vertices[2] = v2;
  face->m_sourceType = SourceType::Face3d;
  face->m_edgeFlags = edgeFlags;
  face->m_vertexCount = 3;
  return face;
}

EoDbFace* EoDbFace::CreateFromSolid(const EoGePoint3d& v0, const EoGePoint3d& v1, const EoGePoint3d& v2,
    const EoGePoint3d& v3, const EoGeVector3d& extrusion) {
  auto* face = new EoDbFace();
  face->m_vertices[0] = v0;
  face->m_vertices[1] = v1;
  face->m_vertices[2] = v2;
  face->m_vertices[3] = v3;
  face->m_sourceType = SourceType::Solid;
  face->m_edgeFlags = AllVisible;
  face->m_vertexCount = 4;
  face->m_extrusion = extrusion;
  return face;
}

EoDbFace* EoDbFace::CreateTriangleFromSolid(
    const EoGePoint3d& v0, const EoGePoint3d& v1, const EoGePoint3d& v2, const EoGeVector3d& extrusion) {
  auto* face = new EoDbFace();
  face->m_vertices[0] = v0;
  face->m_vertices[1] = v1;
  face->m_vertices[2] = v2;
  face->m_sourceType = SourceType::Solid;
  face->m_edgeFlags = AllVisible;
  face->m_vertexCount = 3;
  face->m_extrusion = extrusion;
  return face;
}

EoDbFace* EoDbFace::CreateFromTrace(const EoGePoint3d& v0, const EoGePoint3d& v1, const EoGePoint3d& v2,
    const EoGePoint3d& v3, const EoGeVector3d& extrusion) {
  auto* face = new EoDbFace();
  face->m_vertices[0] = v0;
  face->m_vertices[1] = v1;
  face->m_vertices[2] = v2;
  face->m_vertices[3] = v3;
  face->m_sourceType = SourceType::Trace;
  face->m_edgeFlags = AllVisible;
  face->m_vertexCount = 4;
  face->m_extrusion = extrusion;
  return face;
}

// ── EoDbPrimitive Virtual Contract ───────────────────────────────────────────

void EoDbFace::AddReportToMessageList(const EoGePoint3d& point) {
  EoDbPrimitive::AddReportToMessageList(point);

  const wchar_t* sourceLabel = L"<Face>";
  switch (m_sourceType) {
    case SourceType::Face3d:
      sourceLabel = L"<3DFACE>";
      break;
    case SourceType::Solid:
      sourceLabel = L"<SOLID>";
      break;
    case SourceType::Trace:
      sourceLabel = L"<TRACE>";
      break;
  }
  app.AddStringToMessageList(sourceLabel);

  CString message;
  for (int i = 0; i < m_vertexCount; ++i) {
    message.Format(L"  V%d: (%.4f, %.4f, %.4f)", i, m_vertices[i].x, m_vertices[i].y, m_vertices[i].z);
    app.AddStringToMessageList(message);
  }
  if (m_sourceType == SourceType::Face3d && m_edgeFlags != AllVisible) {
    message.Format(L"  Edge Flags: 0x%02X", m_edgeFlags);
    app.AddStringToMessageList(message);
  }
}

void EoDbFace::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  const wchar_t* label = L"<Face>";
  switch (m_sourceType) {
    case SourceType::Face3d:
      label = L"<3DFACE>";
      break;
    case SourceType::Solid:
      label = L"<SOLID>";
      break;
    case SourceType::Trace:
      label = L"<TRACE>";
      break;
  }
  tvAddItem(tree, parent, label, this);
}

EoDbPrimitive*& EoDbFace::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbFace(*this);
  return primitive;
}

void EoDbFace::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  const std::int16_t color = LogicalColor();
  const std::int16_t lineType = LogicalLineType();
  const auto& lineTypeName = LogicalLineTypeName();

  if (IsFilled()) {
    // SOLID/TRACE: render as filled polygon
    renderState.SetColor(renderDevice, color);
    renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Solid);

    EoGePoint4dArray pointsArray;
    pointsArray.SetSize(m_vertexCount);
    for (int i = 0; i < m_vertexCount; ++i) { pointsArray[i] = EoGePoint4d(m_vertices[i]); }
    view->ModelViewTransformPoints(pointsArray);
    EoGePoint4d::ClipPolygon(pointsArray);
    Polygon_Display(view, renderDevice, pointsArray);
  } else {
    // 3DFACE: render visible edges as wireframe
    renderState.SetPen(view, renderDevice, color, lineType, lineTypeName, m_lineWeight, m_lineTypeScale);

    for (int i = 0; i < m_vertexCount; ++i) {
      if (!IsEdgeVisible(i)) { continue; }

      const auto& edgeStart = m_vertices[i];
      const auto& edgeEnd = m_vertices[(i + 1) % m_vertexCount];

      polyline::BeginLineStrip();
      polyline::SetVertex(edgeStart);
      polyline::SetVertex(edgeEnd);
      polyline::End(view, renderDevice, lineType, lineTypeName);
    }
  }
}

void EoDbFace::ExportToDxf(EoDxfInterface* writer) const {
  switch (m_sourceType) {
    case SourceType::Face3d: {
      EoDxf3dFace face;
      PopulateDxfBaseProperties(&face);
      face.m_firstCorner = {m_vertices[0].x, m_vertices[0].y, m_vertices[0].z};
      face.m_secondCorner = {m_vertices[1].x, m_vertices[1].y, m_vertices[1].z};
      face.m_thirdCorner = {m_vertices[2].x, m_vertices[2].y, m_vertices[2].z};
      if (m_vertexCount == 4) {
        face.m_fourthCorner = {m_vertices[3].x, m_vertices[3].y, m_vertices[3].z};
      } else {
        // Triangle: fourth corner == third corner
        face.m_fourthCorner = face.m_thirdCorner;
      }
      face.m_invisibleFlag = static_cast<std::int16_t>(m_edgeFlags);
      writer->Add3dFace(face);
      break;
    }

    case SourceType::Solid: {
      EoDxfSolid solid;
      PopulateDxfBaseProperties(&solid);
      // Sequential → bowtie reorder: internal [0,1,2,3] → DXF codes 10,11,13,12
      solid.m_firstCorner = {m_vertices[0].x, m_vertices[0].y, m_vertices[0].z};
      solid.m_secondCorner = {m_vertices[1].x, m_vertices[1].y, m_vertices[1].z};
      if (m_vertexCount == 4) {
        solid.m_thirdCorner = {m_vertices[3].x, m_vertices[3].y, m_vertices[3].z};
        solid.m_fourthCorner = {m_vertices[2].x, m_vertices[2].y, m_vertices[2].z};
      } else {
        solid.m_thirdCorner = {m_vertices[2].x, m_vertices[2].y, m_vertices[2].z};
        solid.m_fourthCorner = solid.m_thirdCorner;
      }
      solid.m_extrusionDirection = {m_extrusion.x, m_extrusion.y, m_extrusion.z};
      solid.m_haveExtrusion = !(m_extrusion == EoGeVector3d::positiveUnitZ);
      writer->AddSolid(solid);
      break;
    }

    case SourceType::Trace: {
      EoDxfTrace trace;
      PopulateDxfBaseProperties(&trace);
      // Sequential → bowtie reorder: internal [0,1,2,3] → DXF codes 10,11,13,12
      trace.m_firstCorner = {m_vertices[0].x, m_vertices[0].y, m_vertices[0].z};
      trace.m_secondCorner = {m_vertices[1].x, m_vertices[1].y, m_vertices[1].z};
      if (m_vertexCount == 4) {
        trace.m_thirdCorner = {m_vertices[3].x, m_vertices[3].y, m_vertices[3].z};
        trace.m_fourthCorner = {m_vertices[2].x, m_vertices[2].y, m_vertices[2].z};
      } else {
        trace.m_thirdCorner = {m_vertices[2].x, m_vertices[2].y, m_vertices[2].z};
        trace.m_fourthCorner = trace.m_thirdCorner;
      }
      trace.m_extrusionDirection = {m_extrusion.x, m_extrusion.y, m_extrusion.z};
      trace.m_haveExtrusion = !(m_extrusion == EoGeVector3d::positiveUnitZ);
      writer->AddTrace(trace);
      break;
    }
  }
}

void EoDbFace::FormatExtra(CString& extra) {
  EoDbPrimitive::FormatExtra(extra);
  const wchar_t* sourceLabel = L"Face3d";
  switch (m_sourceType) {
    case SourceType::Face3d:
      sourceLabel = L"Face3d";
      break;
    case SourceType::Solid:
      sourceLabel = L"Solid";
      break;
    case SourceType::Trace:
      sourceLabel = L"Trace";
      break;
  }
  extra.AppendFormat(L"\tSource;%s\tVertices;%d", sourceLabel, static_cast<int>(m_vertexCount));
  if (m_sourceType == SourceType::Face3d && m_edgeFlags != AllVisible) {
    extra.AppendFormat(L"\tEdgeFlags;0x%02X", m_edgeFlags);
  }
  extra += L'\t';
}

void EoDbFace::FormatGeometry(CString& str) {
  for (int i = 0; i < m_vertexCount; ++i) {
    CString label;
    label.Format(L"V%d;", i);
    str += label + m_vertices[i].ToString();
  }
}

void EoDbFace::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  for (int i = 0; i < m_vertexCount; ++i) { points.Add(m_vertices[i]); }
}

EoGePoint3d EoDbFace::GetControlPoint() {
  if (sm_controlPointIndex >= 0 && sm_controlPointIndex < m_vertexCount) {
    return m_vertices[sm_controlPointIndex];
  }
  return m_vertices[0];
}

void EoDbFace::GetExtents(
    AeSysView* view, EoGePoint3d& minPoint, EoGePoint3d& maxPoint, const EoGeTransformMatrix& transformMatrix) {
  for (int i = 0; i < m_vertexCount; ++i) {
    EoGePoint3d point = m_vertices[i];
    view->ModelTransformPoint(point);
    point = transformMatrix * point;
    minPoint = EoGePoint3d::Min(minPoint, point);
    maxPoint = EoGePoint3d::Max(maxPoint, point);
  }
}

EoGePoint3d EoDbFace::GoToNextControlPoint() {
  if (sm_controlPointIndex >= 0 && sm_controlPointIndex < m_vertexCount) {
    sm_controlPointIndex = (sm_controlPointIndex + 1) % m_vertexCount;
  } else {
    // Initial engagement: pick the vertex at lower-left
    sm_controlPointIndex = 0;
    for (int i = 1; i < m_vertexCount; ++i) {
      if (m_vertices[i].x < m_vertices[sm_controlPointIndex].x ||
          (m_vertices[i].x == m_vertices[sm_controlPointIndex].x &&
              m_vertices[i].y < m_vertices[sm_controlPointIndex].y)) {
        sm_controlPointIndex = i;
      }
    }
  }
  return m_vertices[sm_controlPointIndex];
}

bool EoDbFace::Identical(EoDbPrimitive* primitive) {
  auto* other = static_cast<EoDbFace*>(primitive);
  if (m_vertexCount != other->m_vertexCount || m_sourceType != other->m_sourceType) { return false; }
  if (m_edgeFlags != other->m_edgeFlags) { return false; }
  if (!(m_extrusion == other->m_extrusion)) { return false; }
  for (int i = 0; i < m_vertexCount; ++i) {
    if (m_vertices[i] != other->m_vertices[i]) { return false; }
  }
  return true;
}

bool EoDbFace::IsInView(AeSysView* view) {
  // Test each edge — if any edge is visible after clipping, the face is in view
  for (int i = 0; i < m_vertexCount; ++i) {
    EoGePoint4d edgePoints[2] = {EoGePoint4d(m_vertices[i]), EoGePoint4d(m_vertices[(i + 1) % m_vertexCount])};
    view->ModelViewTransformPoints(2, edgePoints);
    if (EoGePoint4d::ClipLine(edgePoints[0], edgePoints[1])) { return true; }
  }
  return false;
}

bool EoDbFace::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  for (int i = 0; i < m_vertexCount; ++i) {
    EoGePoint4d ndcPoint(m_vertices[i]);
    view->ModelViewTransformPoint(ndcPoint);
    if (point.DistanceToPointXY(ndcPoint) < sm_SelectApertureSize) { return true; }
  }
  return false;
}

EoGePoint3d EoDbFace::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;
  double apertureSize = sm_SelectApertureSize;

  for (int i = 0; i < m_vertexCount; ++i) {
    EoGePoint4d ndcPoint(m_vertices[i]);
    view->ModelViewTransformPoint(ndcPoint);
    double distance = point.DistanceToPointXY(ndcPoint);
    if (distance < apertureSize) {
      sm_controlPointIndex = i;
      apertureSize = distance;
    }
  }
  return (sm_controlPointIndex == SHRT_MAX) ? EoGePoint3d::kOrigin : m_vertices[sm_controlPointIndex];
}

bool EoDbFace::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  polyline::BeginLineLoop();
  for (int i = 0; i < m_vertexCount; ++i) { polyline::SetVertex(m_vertices[i]); }
  return polyline::SelectUsingLine(view, line, intersections);
}

bool EoDbFace::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& foundPoint) {
  polyline::BeginLineLoop();
  for (int i = 0; i < m_vertexCount; ++i) { polyline::SetVertex(m_vertices[i]); }
  return polyline::SelectUsingPoint(view, point, sm_RelationshipOfPoint, foundPoint);
}

bool EoDbFace::SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeft, EoGePoint3d upperRight) {
  polyline::BeginLineLoop();
  for (int i = 0; i < m_vertexCount; ++i) { polyline::SetVertex(m_vertices[i]); }
  return polyline::SelectUsingRectangle(view, lowerLeft, upperRight);
}

void EoDbFace::Transform(const EoGeTransformMatrix& transformMatrix) {
  for (int i = 0; i < m_vertexCount; ++i) { m_vertices[i] = transformMatrix * m_vertices[i]; }
}

void EoDbFace::Translate(const EoGeVector3d& v) {
  for (int i = 0; i < m_vertexCount; ++i) { m_vertices[i] += v; }
}

void EoDbFace::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  for (int i = 0; i < m_vertexCount; ++i) {
    if ((mask & (1U << i)) != 0) { m_vertices[i] += v; }
  }
}

// ── PEG Serialization ────────────────────────────────────────────────────────

EoDbFace* EoDbFace::ReadFromPeg(CFile& file) {
  auto color = EoDb::ReadInt16(file);
  auto lineTypeIndex = EoDb::ReadInt16(file);
  auto sourceType = static_cast<SourceType>(EoDb::ReadInt8(file));
  auto edgeFlags = static_cast<std::uint8_t>(EoDb::ReadInt8(file));
  auto vertexCount = static_cast<std::uint8_t>(EoDb::ReadInt8(file));
  (void)EoDb::ReadInt8(file);  // padding

  auto extrusion = EoDb::ReadVector3d(file);

  auto* face = new EoDbFace();
  face->SetColor(color);
  face->SetLineTypeIndex(lineTypeIndex);
  face->m_sourceType = sourceType;
  face->m_edgeFlags = edgeFlags;
  face->m_vertexCount = std::min(vertexCount, static_cast<std::uint8_t>(4));
  face->m_extrusion = extrusion;

  for (int i = 0; i < face->m_vertexCount; ++i) { face->m_vertices[i] = EoDb::ReadPoint3d(file); }

  return face;
}

bool EoDbFace::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kFacePrimitive));
  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, m_lineTypeIndex);
  EoDb::WriteInt8(file, static_cast<std::int8_t>(m_sourceType));
  EoDb::WriteInt8(file, static_cast<std::int8_t>(m_edgeFlags));
  EoDb::WriteInt8(file, static_cast<std::int8_t>(m_vertexCount));
  EoDb::WriteInt8(file, std::int8_t(0));  // padding

  m_extrusion.Write(file);

  for (int i = 0; i < m_vertexCount; ++i) { m_vertices[i].Write(file); }

  return true;
}

void EoDbFace::Write([[maybe_unused]] CFile& file, [[maybe_unused]] std::uint8_t* buffer) {
  // V1 buffer-based write is not supported for EoDbFace — face entities are V2-only.
  // If called, silently skip (no V1 format existed for this primitive type).
  ATLTRACE2(traceGeneral, 1, L"EoDbFace::Write(buffer) called — V1 format not supported for face entities\n");
}

// ── Face-Specific Methods ────────────────────────────────────────────────────

EoGeVector3d EoDbFace::ComputeNormal() const {
  auto edge1 = EoGeVector3d(m_vertices[0], m_vertices[1]);
  auto edge2 = EoGeVector3d(m_vertices[0], m_vertices[2]);
  auto normal = CrossProduct(edge1, edge2);
  if (normal.IsNearNull()) { return EoGeVector3d(0.0, 0.0, 0.0); }
  normal.Unitize();
  return normal;
}

bool EoDbFace::IsPlanar() const {
  if (m_vertexCount <= 3) { return true; }

  auto normal = ComputeNormal();
  if (normal.IsNearNull()) { return true; }  // Degenerate — consider planar

  auto edge3 = EoGeVector3d(m_vertices[0], m_vertices[3]);
  double deviation = std::abs(DotProduct(normal, edge3));
  return deviation < Eo::geometricTolerance;
}

EoGePoint3d EoDbFace::Centroid() const noexcept {
  double sumX = 0.0;
  double sumY = 0.0;
  double sumZ = 0.0;
  for (int i = 0; i < m_vertexCount; ++i) {
    sumX += m_vertices[i].x;
    sumY += m_vertices[i].y;
    sumZ += m_vertices[i].z;
  }
  double invCount = 1.0 / m_vertexCount;
  return EoGePoint3d(sumX * invCount, sumY * invCount, sumZ * invCount);
}
