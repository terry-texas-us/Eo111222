#include "Stdafx.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbConic.h"
#include "EoDbDxfInterface.h"
#include "EoDbFace.h"
#include "EoDbLine.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

void EoDbDxfInterface::Convert3dFaceEntity(const EoDxf3dFace& _3dFace, AeSysDoc* document) const {
  ATLTRACE2(traceGeneral, 2, L"3DFACE entity conversion\n");

  const auto& firstCorner = _3dFace.m_firstCorner;
  const auto& secondCorner = _3dFace.m_secondCorner;
  const auto& thirdCorner = _3dFace.m_thirdCorner;
  const auto& fourthCorner = _3dFace.m_fourthCorner;

  // Detect degenerate faces: all corners coincident
  if (firstCorner.IsEqualTo(secondCorner) && secondCorner.IsEqualTo(thirdCorner)) {
    ATLTRACE2(traceGeneral, 1, L"3DFACE entity skipped: degenerate (all corners coincident)\n");
    return;
  }

  // Build WCS vertex points
  const EoGePoint3d v0{firstCorner.x, firstCorner.y, firstCorner.z};
  const EoGePoint3d v1{secondCorner.x, secondCorner.y, secondCorner.z};
  const EoGePoint3d v2{thirdCorner.x, thirdCorner.y, thirdCorner.z};

  const auto edgeFlags = static_cast<std::uint8_t>(_3dFace.m_invisibleFlag);

  // Determine triangle vs quadrilateral: 4th corner == 3rd corner means triangle
  const bool isTriangle = thirdCorner.IsEqualTo(fourthCorner);

  EoDbFace* facePrimitive{};
  if (isTriangle) {
    facePrimitive = EoDbFace::CreateTriangleFrom3dFace(v0, v1, v2, edgeFlags);
  } else {
    const EoGePoint3d v3{fourthCorner.x, fourthCorner.y, fourthCorner.z};
    facePrimitive = EoDbFace::CreateFrom3dFace(v0, v1, v2, v3, edgeFlags);
  }
  facePrimitive->SetBaseProperties(&_3dFace, document);

  AddToDocument(facePrimitive, document, _3dFace.m_space, _3dFace.m_ownerHandle);

  ATLTRACE2(
      traceGeneral, 3, L"  3DFACE \u2192 EoDbFace (%d vertices, edgeFlags=0x%02X)\n", isTriangle ? 3 : 4, edgeFlags);
}

void EoDbDxfInterface::ConvertSolidEntity(const EoDxfSolid& solid, AeSysDoc* document) const {
  ATLTRACE2(traceGeneral, 3, L"SOLID entity conversion\n");

  // DXF bowtie order: codes 10,11,12,13 → corners 0,1,3,2
  // Reorder to sequential: 10→v0, 11→v1, 13→v2, 12→v3
  const EoGePoint3d v0{solid.m_firstCorner.x, solid.m_firstCorner.y, solid.m_firstCorner.z};
  const EoGePoint3d v1{solid.m_secondCorner.x, solid.m_secondCorner.y, solid.m_secondCorner.z};
  const EoGePoint3d v2{solid.m_fourthCorner.x, solid.m_fourthCorner.y, solid.m_fourthCorner.z};
  const EoGePoint3d v3{solid.m_thirdCorner.x, solid.m_thirdCorner.y, solid.m_thirdCorner.z};

  const EoGeVector3d extrusion{
      solid.m_extrusionDirection.x, solid.m_extrusionDirection.y, solid.m_extrusionDirection.z};

  const bool isTriangle = solid.m_thirdCorner.IsEqualTo(solid.m_fourthCorner);

  EoDbFace* facePrimitive{};
  if (isTriangle) {
    facePrimitive = EoDbFace::CreateTriangleFromSolid(v0, v1, v2, extrusion);
  } else {
    facePrimitive = EoDbFace::CreateFromSolid(v0, v1, v2, v3, extrusion);
  }
  facePrimitive->SetBaseProperties(&solid, document);

  AddToDocument(facePrimitive, document, solid.m_space, solid.m_ownerHandle);

  ATLTRACE2(traceGeneral, 3, L"  SOLID \u2192 EoDbFace (%s)\n", isTriangle ? L"triangle" : L"quad");
}

/** @brief Converts a DXF TRACE entity to an AeSys EoDbFace primitive.
 *
 *  A TRACE is identical in structure to SOLID (same AcDbTrace subclass, same bowtie vertex
 *  order, same OCS coordinate system). The only difference is the DXF entity name. TRACE is
 *  always a quadrilateral (no triangle variant in practice, but the degenerate case is handled).
 *
 *  @param trace The parsed DXF TRACE entity (coordinates already in WCS after ApplyExtrusion).
 *  @param document The AeSys document receiving the created primitive.
 */
void EoDbDxfInterface::ConvertTraceEntity(const EoDxfTrace& trace, AeSysDoc* document) const {
  ATLTRACE2(traceGeneral, 3, L"TRACE entity conversion\n");

  // DXF bowtie order: codes 10,11,12,13 → corners 0,1,3,2
  // Reorder to sequential: 10→v0, 11→v1, 13→v2, 12→v3
  const EoGePoint3d v0{trace.m_firstCorner.x, trace.m_firstCorner.y, trace.m_firstCorner.z};
  const EoGePoint3d v1{trace.m_secondCorner.x, trace.m_secondCorner.y, trace.m_secondCorner.z};
  const EoGePoint3d v2{trace.m_fourthCorner.x, trace.m_fourthCorner.y, trace.m_fourthCorner.z};
  const EoGePoint3d v3{trace.m_thirdCorner.x, trace.m_thirdCorner.y, trace.m_thirdCorner.z};

  const EoGeVector3d extrusion{
      trace.m_extrusionDirection.x, trace.m_extrusionDirection.y, trace.m_extrusionDirection.z};

  auto* facePrimitive = EoDbFace::CreateFromTrace(v0, v1, v2, v3, extrusion);
  facePrimitive->SetBaseProperties(&trace, document);

  AddToDocument(facePrimitive, document, trace.m_space, trace.m_ownerHandle);

  ATLTRACE2(traceGeneral, 3, L"  TRACE \u2192 EoDbFace\n");
}

void EoDbDxfInterface::ConvertAcadProxyEntity(const EoDxfAcadProxyEntity& proxyEntity, AeSysDoc* document) const {
  ATLTRACE2(traceGeneral, 2, L"ACAD_PROXY_ENTITY conversion (classId=%d, appClassId=%d)\n",
      proxyEntity.m_proxyEntityClassId, proxyEntity.m_applicationEntityClassId);

  if (!proxyEntity.HasGraphicsData()) {
    ATLTRACE2(traceGeneral, 2, L"  No graphics data — proxy entity skipped\n");
    return;
  }

  const auto declaredSize = proxyEntity.m_graphicsDataSizeInBytes;
  const auto computedSize = proxyEntity.ComputedGraphicsDataSizeInBytes();

  ATLTRACE2(traceGeneral, 2, L"  Graphics data: declared=%d bytes, computed=%d bytes, chunks=%zu\n", declaredSize,
      computedSize, proxyEntity.m_graphicsDataChunks.size());
  ATLTRACE2(traceGeneral, 2, L"  Entity data: %d bits, chunks=%zu\n", proxyEntity.m_entityDataSizeInBits,
      proxyEntity.m_entityDataChunks.size());
  ATLTRACE2(traceGeneral, 2, L"  Handle refs: soft=%zu, hard=%zu, softOwner=%zu, hardOwner=%zu\n",
      proxyEntity.m_softPointerHandles.size(), proxyEntity.m_hardPointerHandles.size(),
      proxyEntity.m_softOwnerHandles.size(), proxyEntity.m_hardOwnerHandles.size());

  if (declaredSize != computedSize) {
    ATLTRACE2(traceGeneral, 1,
        L"  WARNING: Graphics data size mismatch (declared=%d, computed=%d) — data may be truncated\n", declaredSize,
        computedSize);
  }

  // Decode the graphics hex chunks into raw binary for AcGi stream parsing.
  const auto hexData = proxyEntity.ConcatenateGraphicsHexChunks();
  const auto binaryData = EoDxfAcadProxyEntity::DecodeHexToBytes(hexData);
  const auto dataSize = binaryData.size();

  ATLTRACE2(traceGeneral, 2, L"  Decoded %zu bytes of proxy graphics data\n", dataSize);

  if (dataSize == 0) { return; }

  const auto* data = binaryData.data();

  // Helper lambdas for reading little-endian values from the binary stream.
  // Using memcpy ensures correct behavior regardless of alignment.
  auto readInt32 = [data, dataSize](std::size_t offset, std::int32_t& value) -> bool {
    if (offset + sizeof(std::int32_t) > dataSize) { return false; }
    std::memcpy(&value, data + offset, sizeof(std::int32_t));
    return true;
  };

  auto readDouble = [data, dataSize](std::size_t offset, double& value) -> bool {
    if (offset + sizeof(double) > dataSize) { return false; }
    std::memcpy(&value, data + offset, sizeof(double));
    return true;
  };

  auto readPoint3d = [&readDouble](std::size_t offset, EoGePoint3d& point) -> bool {
    return readDouble(offset, point.x) && readDouble(offset + 8, point.y) && readDouble(offset + 16, point.z);
  };

  auto readVector3d = [&readDouble](std::size_t offset, EoGeVector3d& vector) -> bool {
    return readDouble(offset, vector.x) && readDouble(offset + 8, vector.y) && readDouble(offset + 16, vector.z);
  };

  int primitiveCount = 0;
  int skippedGeometryCount = 0;
  std::size_t offset = 0;

  while (offset < dataSize) {
    // ODA format: type code is int32 (RL), 4 bytes little-endian
    std::int32_t typeCode = 0;
    if (!readInt32(offset, typeCode)) {
      ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated type code at offset %zu\n", offset);
      break;
    }
    offset += 4;

    switch (typeCode) {
      case 1: {  // Extents: 6 × double (48 bytes) — bounding box, skip for now
        constexpr std::size_t extentsSize{48};
        if (offset + extentsSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated extents data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 1 → Extents (skipped, 48 bytes)\n");
        offset += extentsSize;
        break;
      }

      case 2: {  // Circle: Point3d center, double radius, Vector3d normal (56 bytes)
        constexpr std::size_t circleSize{24 + 8 + 24};
        if (offset + circleSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated circle data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }

        EoGePoint3d center;
        double radius = 0.0;
        EoGeVector3d normal;
        readPoint3d(offset, center);
        readDouble(offset + 24, radius);
        readVector3d(offset + 32, normal);

        if (radius > Eo::geometricTolerance) {
          if (normal.IsNearNull()) { normal = EoGeVector3d::positiveUnitZ; }
          auto* conicPrimitive = EoDbConic::CreateCircle(center, normal, radius);
          conicPrimitive->SetBaseProperties(&proxyEntity, document);
          AddToDocument(conicPrimitive, document, proxyEntity.m_space, proxyEntity.m_ownerHandle);
          ++primitiveCount;

          ATLTRACE2(traceGeneral, 3, L"  Proxy type 2 → Circle center(%.2f,%.2f,%.2f) r=%.2f\n", center.x, center.y,
              center.z, radius);
        }

        offset += circleSize;
        break;
      }

      case 4: {  // CircularArc: Point3d center, double radius, Vector3d normal, Vector3d startVector,
                 //              double sweepAngle, int32 arcType (92 bytes)
        constexpr std::size_t circularArcSize{24 + 8 + 24 + 24 + 8 + 4};
        if (offset + circularArcSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated circular arc data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }

        EoGePoint3d center;
        double radius = 0.0;
        EoGeVector3d normal;
        EoGeVector3d startVector;
        double sweepAngle = 0.0;
        // int32 arcType at offset+88 — not used for radial arc creation (0=open, 1=sector, 2=chord)

        readPoint3d(offset, center);
        readDouble(offset + 24, radius);
        readVector3d(offset + 32, normal);
        readVector3d(offset + 56, startVector);
        readDouble(offset + 80, sweepAngle);

        if (radius > Eo::geometricTolerance && Eo::IsGeometricallyNonZero(sweepAngle)) {
          if (normal.IsNearNull()) { normal = EoGeVector3d::positiveUnitZ; }
          normal.Unitize();

          // Convert WCS startVector to OCS angle via arbitrary axis decomposition.
          // The arbitrary axis algorithm produces the OCS X-axis from the normal vector,
          // then OCS Y = normal × OCS X. Projecting the startVector onto these axes gives
          // the start angle in OCS coordinates.
          auto arbitraryX = ComputeArbitraryAxis(normal);
          arbitraryX.Unitize();
          auto arbitraryY = CrossProduct(normal, arbitraryX);
          arbitraryY.Unitize();

          double startAngle = std::atan2(DotProduct(startVector, arbitraryY), DotProduct(startVector, arbitraryX));
          double endAngle = startAngle + sweepAngle;

          // For negative Z extrusion, mirror angles to match AutoCAD behavior (same as ConvertArcEntity)
          const bool isNegativeExtrusion = normal.z < -Eo::geometricTolerance;
          if (isNegativeExtrusion) {
            startAngle = Eo::TwoPi - startAngle;
            endAngle = Eo::TwoPi - endAngle;
            std::swap(startAngle, endAngle);
          }

          startAngle = EoDbConic::NormalizeTo2Pi(startAngle);
          endAngle = EoDbConic::NormalizeTo2Pi(endAngle);

          auto* conicPrimitive = EoDbConic::CreateRadialArc(center, normal, radius, startAngle, endAngle);
          if (conicPrimitive != nullptr) {
            conicPrimitive->SetBaseProperties(&proxyEntity, document);
            AddToDocument(conicPrimitive, document, proxyEntity.m_space, proxyEntity.m_ownerHandle);
            ++primitiveCount;

            ATLTRACE2(traceGeneral, 3, L"  Proxy type 4 → Arc center(%.2f,%.2f,%.2f) r=%.2f start=%.4f end=%.4f\n",
                center.x, center.y, center.z, radius, startAngle, endAngle);
          }
        }

        offset += circularArcSize;
        break;
      }

      case 6:  // Polyline: int32 numVertices, Point3d[numVertices]
      case 7: {  // Polygon: same layout as polyline
        std::int32_t numVertices = 0;
        if (!readInt32(offset, numVertices)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated vertex count at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;

        if (numVertices <= 0 || numVertices > 10000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable vertex count %d at offset %zu\n", numVertices,
              offset - 4);
          offset = dataSize;
          break;
        }

        const auto pointsSize = static_cast<std::size_t>(numVertices) * 24;
        if (offset + pointsSize > dataSize) {
          ATLTRACE2(traceGeneral, 1,
              L"  Proxy graphics: insufficient data for %d vertices (need %zu bytes, have %zu)\n", numVertices,
              pointsSize, dataSize - offset);
          offset = dataSize;
          break;
        }

        if (numVertices == 2) {
          // Two-point polyline → create EoDbLine
          EoGePoint3d startPoint;
          EoGePoint3d endPoint;
          readPoint3d(offset, startPoint);
          readPoint3d(offset + 24, endPoint);

          auto* linePrimitive = new EoDbLine();
          linePrimitive->SetBaseProperties(&proxyEntity, document);
          linePrimitive->SetLine(EoGeLine(startPoint, endPoint));
          AddToDocument(linePrimitive, document, proxyEntity.m_space, proxyEntity.m_ownerHandle);
          ++primitiveCount;

          ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → Line (%.2f,%.2f,%.2f)→(%.2f,%.2f,%.2f)\n", typeCode,
              startPoint.x, startPoint.y, startPoint.z, endPoint.x, endPoint.y, endPoint.z);
        } else {
          // Multi-point polyline or polygon → create EoDbPolyline
          EoGePoint3dArray points;
          points.SetSize(numVertices);
          for (std::int32_t vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex) {
            EoGePoint3d point;
            readPoint3d(offset + static_cast<std::size_t>(vertexIndex) * 24, point);
            points[vertexIndex] = point;
          }

          auto* polylinePrimitive = new EoDbPolyline(points);
          polylinePrimitive->SetBaseProperties(&proxyEntity, document);
          if (typeCode == 7) { polylinePrimitive->SetFlag(EoDbPolyline::sm_Closed); }
          AddToDocument(polylinePrimitive, document, proxyEntity.m_space, proxyEntity.m_ownerHandle);
          ++primitiveCount;

          ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → Polyline with %d vertices\n", typeCode, numVertices);
        }

        offset += pointsSize;
        break;
      }

      case 3: {  // Circle (3pt): Point3d pt1, Point3d pt2, Point3d pt3 (72 bytes)
        constexpr std::size_t circle3ptSize{72};
        if (offset + circle3ptSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated 3pt circle data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 3 → Circle3pt (not rendered, 72 bytes)\n");
        ++skippedGeometryCount;
        offset += circle3ptSize;
        break;
      }

      case 5: {  // CircularArc (3pt): Point3d start, Point3d point, Point3d end (72 bytes)
        constexpr std::size_t arc3ptSize{72};
        if (offset + arc3ptSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated 3pt arc data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 5 → CircularArc3pt (not rendered, 72 bytes)\n");
        ++skippedGeometryCount;
        offset += arc3ptSize;
        break;
      }

      case 8: {  // Mesh: int32 rows, int32 cols, Point3d[rows×cols]
        std::int32_t rows = 0;
        std::int32_t cols = 0;
        if (!readInt32(offset, rows) || !readInt32(offset + 4, cols)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated mesh header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        const auto meshPayloadSize = 8 + static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols) * 24;
        if (rows <= 0 || cols <= 0 || offset + meshPayloadSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: invalid mesh %dx%d at offset %zu\n", rows, cols, offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(
            traceGeneral, 2, L"  Proxy type 8 → Mesh %dx%d (not rendered, %zu bytes)\n", rows, cols, meshPayloadSize);
        ++skippedGeometryCount;
        offset += meshPayloadSize;
        break;
      }

      case 9: {  // Shell: int32 numVerts, Point3d[numVerts], int32 numFaceEntries, int32[numFaceEntries]
        std::int32_t numVerts = 0;
        if (!readInt32(offset, numVerts)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;
        if (numVerts <= 0 || numVerts > 100000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable shell vertex count %d\n", numVerts);
          offset = dataSize;
          break;
        }
        const auto verticesSize = static_cast<std::size_t>(numVerts) * 24;
        if (offset + verticesSize + 4 > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell vertices at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += verticesSize;
        std::int32_t numFaceEntries = 0;
        if (!readInt32(offset, numFaceEntries)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell face count at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;
        if (numFaceEntries < 0 || numFaceEntries > 1000000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable shell face entries %d\n", numFaceEntries);
          offset = dataSize;
          break;
        }
        const auto facesSize = static_cast<std::size_t>(numFaceEntries) * 4;
        if (offset + facesSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated shell faces at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 9 → Shell %d verts, %d face entries (not rendered)\n", numVerts,
            numFaceEntries);
        ++skippedGeometryCount;
        offset += facesSize;
        break;
      }

      case 10:
      case 11: {  // Text: Point3d position, Vector3d normal, Vector3d direction, string
        constexpr std::size_t textHeaderSize{24 + 24 + 24};  // position + normal + direction = 72 bytes
        if (offset + textHeaderSize + 4 > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated text header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += textHeaderSize;
        // Text string: int32 charCount followed by charCount × int16 (UTF-16 characters)
        std::int32_t charCount = 0;
        if (!readInt32(offset, charCount)) {
          offset = dataSize;
          break;
        }
        offset += 4;
        if (charCount < 0 || charCount > 100000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable text char count %d\n", charCount);
          offset = dataSize;
          break;
        }
        const auto stringSize = static_cast<std::size_t>(charCount) * 2;
        if (offset + stringSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated text string at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type %d → Text (%d chars, not rendered)\n", typeCode, charCount);
        ++skippedGeometryCount;
        offset += stringSize;
        break;
      }

      case 12:  // Xline: Point3d basePoint, Vector3d direction (48 bytes)
      case 13: {  // Ray: same layout as xline
        constexpr std::size_t xlineSize{48};
        if (offset + xlineSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated xline/ray data at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type %d → Xline/Ray (not rendered, 48 bytes)\n", typeCode);
        ++skippedGeometryCount;
        offset += xlineSize;
        break;
      }

      case 14: {  // SUBENT: Color (int16 colorIndex = 2 bytes)
        constexpr std::size_t payloadSize{2};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 14 → SUBENT Color (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 15: {  // SUBENT: Layer name (int16 len, char[len])
        if (offset + 2 > dataSize) {
          offset = dataSize;
          break;
        }
        std::int16_t nameLength = 0;
        std::memcpy(&nameLength, data + offset, sizeof(std::int16_t));
        offset += 2;
        if (nameLength < 0 || offset + static_cast<std::size_t>(nameLength) > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 15 → SUBENT Layer (skipped, %d chars)\n", nameLength);
        offset += static_cast<std::size_t>(nameLength);
        break;
      }

      case 16: {  // SUBENT: Linetype name (int16 len, char[len])
        if (offset + 2 > dataSize) {
          offset = dataSize;
          break;
        }
        std::int16_t nameLength = 0;
        std::memcpy(&nameLength, data + offset, sizeof(std::int16_t));
        offset += 2;
        if (nameLength < 0 || offset + static_cast<std::size_t>(nameLength) > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 16 → SUBENT Linetype (skipped, %d chars)\n", nameLength);
        offset += static_cast<std::size_t>(nameLength);
        break;
      }

      case 17: {  // SUBENT: Marker (int32 = 4 bytes)
        constexpr std::size_t payloadSize{4};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 17 → SUBENT Marker (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 18: {  // SUBENT: Fill (int16 = 2 bytes)
        constexpr std::size_t payloadSize{2};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 18 → SUBENT Fill (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 20: {  // SUBENT: True Color (int32 = 4 bytes)
        constexpr std::size_t payloadSize{4};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 20 → SUBENT True Color (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 21: {  // SUBENT: Lineweight (int16 = 2 bytes)
        constexpr std::size_t payloadSize{2};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 21 → SUBENT Lineweight (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 22: {  // SUBENT: Linetype Scale (double = 8 bytes)
        constexpr std::size_t payloadSize{8};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 22 → SUBENT Linetype Scale (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 23: {  // SUBENT: Thickness (double = 8 bytes)
        constexpr std::size_t payloadSize{8};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 23 → SUBENT Thickness (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 24: {  // SUBENT: Plot Style Name (int32 = 4 bytes)
        constexpr std::size_t payloadSize{4};
        if (offset + payloadSize > dataSize) {
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 24 → SUBENT Plot Style (skipped)\n");
        offset += payloadSize;
        break;
      }

      case 19:  // SUBENT: no payload
      case 25:  // SUBENT: no payload
      case 26: {  // SUBENT: no payload
        ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → SUBENT (no payload, skipped)\n", typeCode);
        break;
      }

      case 27:  // Push Clip Boundary (no payload)
      case 28: {  // Pop Clip Boundary (no payload)
        ATLTRACE2(traceGeneral, 3, L"  Proxy type %d → Clip Boundary (skipped)\n", typeCode);
        break;
      }

      case 29: {  // Push Model Transform: 4×3 matrix (12 × double = 96 bytes)
        constexpr std::size_t xformSize{96};
        if (offset + xformSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated model transform at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 3, L"  Proxy type 29 → Push Model Transform (skipped, 96 bytes)\n");
        offset += xformSize;
        break;
      }

      case 33: {  // LwPolyline: int32 numPoints, int32 flags, Point3d[numPoints]
        std::int32_t numPoints = 0;
        if (!readInt32(offset, numPoints)) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated lwpolyline header at offset %zu\n", offset);
          offset = dataSize;
          break;
        }
        offset += 4;
        if (numPoints <= 0 || numPoints > 100000) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: unreasonable lwpolyline point count %d\n", numPoints);
          offset = dataSize;
          break;
        }
        // Skip flags (int32) + point data
        const auto lwPolyPayloadSize = 4 + static_cast<std::size_t>(numPoints) * 24;
        if (offset + lwPolyPayloadSize > dataSize) {
          ATLTRACE2(traceGeneral, 1, L"  Proxy graphics: truncated lwpolyline data for %d points at offset %zu\n",
              numPoints, offset);
          offset = dataSize;
          break;
        }
        ATLTRACE2(traceGeneral, 2, L"  Proxy type 33 → LwPolyline %d points (not rendered)\n", numPoints);
        ++skippedGeometryCount;
        offset += lwPolyPayloadSize;
        break;
      }

      default:
        // Unknown type code — cannot determine record length, must abort parsing.
        ATLTRACE2(traceGeneral, 1,
            L"  Proxy graphics: unknown type code %d at offset %zu — aborting parse (%d primitives created)\n",
            typeCode, offset - 4, primitiveCount);
        offset = dataSize;
        break;
    }
  }

  ATLTRACE2(traceGeneral, 2, L"  Proxy entity produced %d primitives", primitiveCount);
  if (skippedGeometryCount > 0) {
    ATLTRACE2(traceGeneral, 2, L", %d geometry records not rendered", skippedGeometryCount);
  }
  ATLTRACE2(traceGeneral, 2, L"\n");
}
