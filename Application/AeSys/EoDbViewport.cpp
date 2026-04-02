#include "Stdafx.h"

#include <cmath>
#include <format>
#include <utility>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbLineTypeTable.h"
#include "EoDbViewport.h"
#include "EoDxfInterface.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"

EoDbViewport::EoDbViewport(const EoDbViewport& other)
    : EoDbPrimitive(other),
      m_centerPoint(other.m_centerPoint),
      m_width(other.m_width),
      m_height(other.m_height),
      m_viewportStatus(other.m_viewportStatus),
      m_viewportId(other.m_viewportId),
      m_viewCenter(other.m_viewCenter),
      m_snapBasePoint(other.m_snapBasePoint),
      m_snapSpacing(other.m_snapSpacing),
      m_gridSpacing(other.m_gridSpacing),
      m_viewDirection(other.m_viewDirection),
      m_viewTargetPoint(other.m_viewTargetPoint),
      m_lensLength(other.m_lensLength),
      m_frontClipPlane(other.m_frontClipPlane),
      m_backClipPlane(other.m_backClipPlane),
      m_viewHeight(other.m_viewHeight),
      m_snapAngle(other.m_snapAngle),
      m_twistAngle(other.m_twistAngle) {}

EoDbViewport& EoDbViewport::operator=(EoDbViewport other) noexcept {
  swap(other);
  return *this;
}

void EoDbViewport::swap(EoDbViewport& other) noexcept {
  using std::swap;
  // Base class protected members (m_handle intentionally excluded — entity identity is preserved)
  swap(m_color, other.m_color);
  swap(m_lineType, other.m_lineType);
  swap(m_layerName, other.m_layerName);
  swap(m_ownerHandle, other.m_ownerHandle);
  // Paper-space geometry
  swap(m_centerPoint, other.m_centerPoint);
  swap(m_width, other.m_width);
  swap(m_height, other.m_height);
  // Viewport identity
  swap(m_viewportStatus, other.m_viewportStatus);
  swap(m_viewportId, other.m_viewportId);
  // Model-space view parameters
  swap(m_viewCenter, other.m_viewCenter);
  swap(m_snapBasePoint, other.m_snapBasePoint);
  swap(m_snapSpacing, other.m_snapSpacing);
  swap(m_gridSpacing, other.m_gridSpacing);
  swap(m_viewDirection, other.m_viewDirection);
  swap(m_viewTargetPoint, other.m_viewTargetPoint);
  swap(m_lensLength, other.m_lensLength);
  swap(m_frontClipPlane, other.m_frontClipPlane);
  swap(m_backClipPlane, other.m_backClipPlane);
  swap(m_viewHeight, other.m_viewHeight);
  swap(m_snapAngle, other.m_snapAngle);
  swap(m_twistAngle, other.m_twistAngle);
}

void EoDbViewport::AddReportToMessageList(const EoGePoint3d& point) {
  app.AddStringToMessageList(L"<Viewport>");
  EoDbPrimitive::AddReportToMessageList(point);
  CString message;
  message.Format(L"  Id: %d  Center: (%.4f, %.4f, %.4f)  Size: %.4f x %.4f", m_viewportId, m_centerPoint.x,
      m_centerPoint.y, m_centerPoint.z, m_width, m_height);
  app.AddStringToMessageList(message);
}

void EoDbViewport::AddToTreeViewControl(HWND tree, HTREEITEM parent) { tvAddItem(tree, parent, L"<Viewport>", this); }

EoDbPrimitive*& EoDbViewport::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbViewport(*this);
  return primitive;
}

void EoDbViewport::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  // Minimal display: draw the viewport boundary rectangle in paper space.
  // Full paper-space clipping pipeline is deferred.
  const double halfWidth = m_width / 2.0;
  const double halfHeight = m_height / 2.0;

  EoGePoint3d corners[4] = {
      {m_centerPoint.x - halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
      {m_centerPoint.x - halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
  };

  const std::int16_t color = LogicalColor();
  const COLORREF hotPenColor = app.PenColorsGetHot(color);

  renderDevice->SelectPen(PS_DOT, 1, hotPenColor);

  EoGePoint4d ndcCorners[4];
  CPoint clientCorners[4];
  bool anyInView = false;

  for (int i = 0; i < 4; ++i) {
    ndcCorners[i] = EoGePoint4d(corners[i]);
    view->ModelViewTransformPoint(ndcCorners[i]);
    clientCorners[i] = view->ProjectToClient(ndcCorners[i]);
    if (ndcCorners[i].IsInView()) { anyInView = true; }
  }

  if (anyInView) {
    renderDevice->MoveTo(clientCorners[0].x, clientCorners[0].y);
    for (int i = 1; i < 4; ++i) { renderDevice->LineTo(clientCorners[i].x, clientCorners[i].y); }
    renderDevice->LineTo(clientCorners[0].x, clientCorners[0].y);
  }

  renderDevice->RestorePen();
}

void EoDbViewport::FormatExtra(CString& extra) {
  EoDbPrimitive::FormatExtra(extra);
  extra.AppendFormat(L"\tViewportId;%d\tStatus;%d", m_viewportId, m_viewportStatus);
  extra += L'\t';
}

void EoDbViewport::FormatGeometry(CString& str) {
  str += L"Center;" + m_centerPoint.ToString();
  CString sizeStr;
  sizeStr.Format(L"\tWidth;%.4f\tHeight;%.4f", m_width, m_height);
  str += sizeStr;
}

void EoDbViewport::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);

  const double halfWidth = m_width / 2.0;
  const double halfHeight = m_height / 2.0;

  points.Add({m_centerPoint.x - halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z});
  points.Add({m_centerPoint.x + halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z});
  points.Add({m_centerPoint.x + halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z});
  points.Add({m_centerPoint.x - halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z});
}

void EoDbViewport::GetExtents(
    AeSysView* view, EoGePoint3d& minPoint, EoGePoint3d& maxPoint, const EoGeTransformMatrix& transformMatrix) {
  const double halfWidth = m_width / 2.0;
  const double halfHeight = m_height / 2.0;

  EoGePoint3d corners[4] = {
      {m_centerPoint.x - halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
      {m_centerPoint.x - halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
  };

  for (auto& corner : corners) {
    view->ModelTransformPoint(corner);
    corner = transformMatrix * corner;
    minPoint = EoGePoint3d::Min(minPoint, corner);
    maxPoint = EoGePoint3d::Max(maxPoint, corner);
  }
}

bool EoDbViewport::Identical(EoDbPrimitive* primitive) {
  auto* other = static_cast<EoDbViewport*>(primitive);
  return m_centerPoint == other->m_centerPoint && m_width == other->m_width && m_height == other->m_height &&
      m_viewportStatus == other->m_viewportStatus && m_viewportId == other->m_viewportId &&
      m_viewCenter == other->m_viewCenter && m_snapBasePoint == other->m_snapBasePoint &&
      m_snapSpacing == other->m_snapSpacing && m_gridSpacing == other->m_gridSpacing &&
      m_viewDirection == other->m_viewDirection && m_viewTargetPoint == other->m_viewTargetPoint &&
      m_lensLength == other->m_lensLength && m_frontClipPlane == other->m_frontClipPlane &&
      m_backClipPlane == other->m_backClipPlane && m_viewHeight == other->m_viewHeight &&
      m_snapAngle == other->m_snapAngle && m_twistAngle == other->m_twistAngle;
}

bool EoDbViewport::IsInView(AeSysView* view) {
  const double halfWidth = m_width / 2.0;
  const double halfHeight = m_height / 2.0;

  EoGePoint3d corners[4] = {
      {m_centerPoint.x - halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
      {m_centerPoint.x - halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
  };

  for (const auto& corner : corners) {
    EoGePoint4d ndcPoint(corner);
    view->ModelViewTransformPoint(ndcPoint);
    if (ndcPoint.IsInView()) { return true; }
  }
  return false;
}

bool EoDbViewport::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d ndcCenter(m_centerPoint);
  view->ModelViewTransformPoint(ndcCenter);
  return point.DistanceToPointXY(ndcCenter) < sm_SelectApertureSize;
}

EoGePoint3d EoDbViewport::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d ndcCenter(m_centerPoint);
  view->ModelViewTransformPoint(ndcCenter);

  sm_controlPointIndex = (point.DistanceToPointXY(ndcCenter) < sm_SelectApertureSize) ? 0 : SHRT_MAX;
  return (sm_controlPointIndex == 0) ? m_centerPoint : EoGePoint3d::kOrigin;
}

bool EoDbViewport::SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) {
  const double halfWidth = m_width / 2.0;
  const double halfHeight = m_height / 2.0;

  // 4 corners forming a closed rectangle
  EoGePoint3d corners[4] = {
      {m_centerPoint.x - halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
      {m_centerPoint.x - halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
  };

  // Transform corners to NDC and test 4 edges (closed loop)
  EoGePoint4d ndcCorners[4];
  for (int i = 0; i < 4; ++i) {
    ndcCorners[i] = EoGePoint4d(corners[i]);
    view->ModelViewTransformPoint(ndcCorners[i]);
  }

  for (int i = 0; i < 4; ++i) {
    const auto& ptBeg = ndcCorners[i];
    const auto& ptEnd = ndcCorners[(i + 1) % 4];

    EoGePoint3d intersection;
    if (EoGeLine::Intersection_xy(line, EoGeLine(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd}), intersection)) {
      double relation{};

      if (line.ComputeParametricRelation(intersection, relation) && relation >= -Eo::geometricTolerance &&
          relation <= 1.0 + Eo::geometricTolerance) {
        if (EoGeLine(EoGePoint3d{ptBeg}, EoGePoint3d{ptEnd}).ComputeParametricRelation(intersection, relation) &&
            relation >= -Eo::geometricTolerance && relation <= 1.0 + Eo::geometricTolerance) {
          intersection.z = ptBeg.z + relation * (ptEnd.z - ptBeg.z);
          intersections.Add(intersection);
        }
      }
    }
  }
  return !intersections.IsEmpty();
}

bool EoDbViewport::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& intersectionPoint) {
  const double halfWidth = m_width / 2.0;
  const double halfHeight = m_height / 2.0;

  // 4 corners forming a closed rectangle
  EoGePoint3d corners[4] = {
      {m_centerPoint.x - halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y - halfHeight, m_centerPoint.z},
      {m_centerPoint.x + halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
      {m_centerPoint.x - halfWidth, m_centerPoint.y + halfHeight, m_centerPoint.z},
  };

  EoGePoint4d ndcBeg(corners[0]);
  view->ModelViewTransformPoint(ndcBeg);

  for (int i = 1; i <= 4; ++i) {
    EoGePoint4d ndcEnd(corners[i % 4]);
    view->ModelViewTransformPoint(ndcEnd);

    EoGeLine edge(EoGePoint3d{ndcBeg}, EoGePoint3d{ndcEnd});
    if (edge.IsSelectedByPointXY(
            EoGePoint3d{point}, view->SelectApertureSize(), intersectionPoint, &sm_RelationshipOfPoint)) {
      intersectionPoint.z = ndcBeg.z + sm_RelationshipOfPoint * (ndcEnd.z - ndcBeg.z);
      return true;
    }
    ndcBeg = ndcEnd;
  }
  return false;
}

bool EoDbViewport::SelectUsingRectangle(AeSysView* view, EoGePoint3d lowerLeft, EoGePoint3d upperRight) {
  EoGePoint4d ndcCenter(m_centerPoint);
  view->ModelViewTransformPoint(ndcCenter);

  return ndcCenter.x >= lowerLeft.x && ndcCenter.x <= upperRight.x && ndcCenter.y >= lowerLeft.y &&
      ndcCenter.y <= upperRight.y;
}

void EoDbViewport::Transform(const EoGeTransformMatrix& transformMatrix) {
  m_centerPoint = transformMatrix * m_centerPoint;
}

void EoDbViewport::TranslateUsingMask(EoGeVector3d translateVector, const DWORD mask) {
  if (mask != 0) { m_centerPoint += translateVector; }
}

// --- PEG Serialization ---

EoDbViewport* EoDbViewport::ReadFromPeg(CFile& file) {
  auto* viewport = new EoDbViewport();
  viewport->SetColor(EoDb::ReadInt16(file));
  auto lineTypeIndex = EoDb::ReadInt16(file);
  viewport->SetLineTypeName(EoDbLineTypeTable::LegacyLineTypeName(lineTypeIndex));
  viewport->m_centerPoint = EoDb::ReadPoint3d(file);
  EoDb::Read(file, viewport->m_width);
  EoDb::Read(file, viewport->m_height);
  EoDb::Read(file, viewport->m_viewportStatus);
  EoDb::Read(file, viewport->m_viewportId);
  viewport->m_viewCenter = EoDb::ReadPoint3d(file);
  viewport->m_snapBasePoint = EoDb::ReadPoint3d(file);
  viewport->m_snapSpacing = EoDb::ReadPoint3d(file);
  viewport->m_gridSpacing = EoDb::ReadPoint3d(file);
  viewport->m_viewDirection = EoDb::ReadPoint3d(file);
  viewport->m_viewTargetPoint = EoDb::ReadPoint3d(file);
  EoDb::Read(file, viewport->m_lensLength);
  EoDb::Read(file, viewport->m_frontClipPlane);
  EoDb::Read(file, viewport->m_backClipPlane);
  EoDb::Read(file, viewport->m_viewHeight);
  EoDb::Read(file, viewport->m_snapAngle);
  EoDb::Read(file, viewport->m_twistAngle);
  return viewport;
}

bool EoDbViewport::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kViewportPrimitive));
  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, EoDbLineTypeTable::LegacyLineTypeIndex(m_lineType));
  m_centerPoint.Write(file);
  EoDb::WriteDouble(file, m_width);
  EoDb::WriteDouble(file, m_height);
  EoDb::WriteInt16(file, m_viewportStatus);
  EoDb::WriteInt16(file, m_viewportId);
  m_viewCenter.Write(file);
  m_snapBasePoint.Write(file);
  m_snapSpacing.Write(file);
  m_gridSpacing.Write(file);
  m_viewDirection.Write(file);
  m_viewTargetPoint.Write(file);
  EoDb::WriteDouble(file, m_lensLength);
  EoDb::WriteDouble(file, m_frontClipPlane);
  EoDb::WriteDouble(file, m_backClipPlane);
  EoDb::WriteDouble(file, m_viewHeight);
  EoDb::WriteDouble(file, m_snapAngle);
  EoDb::WriteDouble(file, m_twistAngle);
  return true;
}

void EoDbViewport::Write(CFile& file, [[maybe_unused]] std::uint8_t* buffer) {
  // Job file format not supported for viewport — delegate to PEG write
  Write(file);
}

// --- DXF Export ---

void EoDbViewport::ExportToDxf(EoDxfInterface* writer) const {
  EoDxfViewport viewport;
  PopulateDxfBaseProperties(&viewport);
  ExportToDxf(viewport);
  writer->AddViewport(viewport);
}

void EoDbViewport::ExportToDxf(EoDxfViewport& viewport) const {
  viewport.m_centerPoint = {m_centerPoint.x, m_centerPoint.y, m_centerPoint.z};
  viewport.m_width = m_width;
  viewport.m_height = m_height;
  viewport.m_viewportStatus = m_viewportStatus;
  viewport.m_viewportId = m_viewportId;
  viewport.m_viewCenter = {m_viewCenter.x, m_viewCenter.y};
  viewport.m_snapBasePoint = {m_snapBasePoint.x, m_snapBasePoint.y};
  viewport.m_snapSpacing = {m_snapSpacing.x, m_snapSpacing.y};
  viewport.m_gridSpacing = {m_gridSpacing.x, m_gridSpacing.y};
  viewport.m_viewDirection = {m_viewDirection.x, m_viewDirection.y, m_viewDirection.z};
  viewport.m_viewTargetPoint = {m_viewTargetPoint.x, m_viewTargetPoint.y, m_viewTargetPoint.z};
  viewport.m_lensLength = m_lensLength;
  viewport.m_frontClipPlane = m_frontClipPlane;
  viewport.m_backClipPlane = m_backClipPlane;
  viewport.m_viewHeight = m_viewHeight;
  viewport.m_snapAngle = m_snapAngle;
  viewport.m_twistAngle = m_twistAngle;
  viewport.m_layer = m_layerName.empty() ? L"0" : m_layerName;
  viewport.m_space = EoDxf::Space::PaperSpace;
}
