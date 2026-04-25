#include "Stdafx.h"

#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockReference.h"
#include "EoDbGroup.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoDxfEntities.h"
#include "EoDxfInterface.h"
#include "EoGeLine.h"
#include "EoGeMatrix.h"
#include "EoGeOcsTransform.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

EoDbBlockReference::EoDbBlockReference() {
  m_insertionPoint = EoGePoint3d::kOrigin;
  m_normal = EoGeVector3d::positiveUnitZ;
  m_scaleFactors = EoGeVector3d(1.0, 1.0, 1.0);
  m_rotation = 0.0;
  m_columnCount = 1;
  m_rowCount = 1;
  m_columnSpacing = 0.0;
  m_rowSpacing = 0.0;
}

EoDbBlockReference::EoDbBlockReference(const CString& name, const EoGePoint3d& insertionPoint)
    : m_blockName(name), m_insertionPoint(insertionPoint) {
  m_normal = EoGeVector3d::positiveUnitZ;
  m_scaleFactors = EoGeVector3d(1.0, 1.0, 1.0);
  m_rotation = 0.0;
  m_columnCount = 1;
  m_rowCount = 1;
  m_columnSpacing = 0.0;
  m_rowSpacing = 0.0;
}

EoDbBlockReference::EoDbBlockReference(const EoDbBlockReference& other) : EoDbPrimitive(other) {
  m_blockName = other.m_blockName;
  m_insertionPoint = other.m_insertionPoint;
  m_normal = other.m_normal;
  m_scaleFactors = other.m_scaleFactors;
  m_rotation = other.m_rotation;
  m_columnCount = other.m_columnCount;
  m_rowCount = other.m_rowCount;
  m_columnSpacing = other.m_columnSpacing;
  m_rowSpacing = other.m_rowSpacing;
  m_attributeHandles = other.m_attributeHandles;
  m_extensionDictionaryHandle = other.m_extensionDictionaryHandle;
}
EoDbBlockReference::EoDbBlockReference(std::uint16_t color, std::uint16_t lineType, const CString& name,
    const EoGePoint3d& point, const EoGeVector3d& normal, const EoGeVector3d scaleFactors, double rotation)
    : m_blockName(name), m_insertionPoint(point), m_normal(normal), m_scaleFactors(scaleFactors) {
  m_color = static_cast<std::int16_t>(color);
  SetLineTypeName(EoDbLineTypeTable::LegacyLineTypeName(static_cast<std::int16_t>(lineType)));

  m_rotation = rotation;
  m_columnCount = 1;
  m_rowCount = 1;
  m_columnSpacing = 0.0;
  m_rowSpacing = 0.0;
}
const EoDbBlockReference& EoDbBlockReference::operator=(const EoDbBlockReference& other) {
  if (this != &other) {
    EoDbPrimitive::operator=(other);
    m_blockName = other.m_blockName;
    m_insertionPoint = other.m_insertionPoint;
    m_normal = other.m_normal;
    m_scaleFactors = other.m_scaleFactors;
    m_rotation = other.m_rotation;
    m_columnCount = other.m_columnCount;
    m_rowCount = other.m_rowCount;
    m_columnSpacing = other.m_columnSpacing;
    m_rowSpacing = other.m_rowSpacing;
    m_attributeHandles = other.m_attributeHandles;
    m_extensionDictionaryHandle = other.m_extensionDictionaryHandle;
  }
  return *this;
}
void EoDbBlockReference::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  EoDbBlock* block{};
  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return; }

  const auto hti = tvAddItem(tree, parent, L"<BlockReference>", this);

  ((EoDbGroup*)block)->AddPrimsToTreeViewControl(tree, hti);
}

EoGeTransformMatrix EoDbBlockReference::BuildTransformMatrix(const EoGePoint3d& basePoint) const {
  return BuildTransformMatrix(basePoint, m_insertionPoint);
}

/** @brief Builds the composite transform for a specific OCS insertion point.
 *
 *  The transform chain applies in order (left-to-right in the operator* chain):
 *  1. Translate by -basePoint (center block geometry at origin)
 *  2. Scale by INSERT scale factors
 *  3. Rotate around OCS Z-axis by INSERT rotation angle
 *  4. Translate by ocsInsertionPoint (still in OCS space)
 *  5. Transform OCS → WCS using the DXF arbitrary axis algorithm
 *
 *  The two-parameter overload enables array INSERT rendering, where each grid
 *  instance has an adjusted insertion point offset by (col × colSpacing, row × rowSpacing)
 *  in OCS coordinates.
 *
 *  @param basePoint The block definition's base point (subtracted first to center geometry).
 *  @param ocsInsertionPoint The insertion point in Object Coordinate System (group 10/20/30).
 */
EoGeTransformMatrix EoDbBlockReference::BuildTransformMatrix(
    const EoGePoint3d& basePoint, const EoGePoint3d& ocsInsertionPoint) const {
  // Step 1: Translate block geometry so base point is at origin
  EoGeTransformMatrix tmNegBase;
  tmNegBase.Translate(EoGeVector3d(basePoint, EoGePoint3d::kOrigin));

  // Step 2: Scale by INSERT scale factors
  EoGeTransformMatrix tmScale;
  tmScale.Scale(m_scaleFactors);

  // Step 3: Rotate around OCS Z-axis by INSERT rotation angle
  const auto tmZRot = EoGeTransformMatrix::ZAxisRotation(std::sin(m_rotation), std::cos(m_rotation));

  // Step 4: Translate by insertion point in OCS (before OCS→WCS transform)
  EoGeTransformMatrix tmInsertOcs;
  tmInsertOcs.Translate(EoGeVector3d(EoGePoint3d::kOrigin, ocsInsertionPoint));

  // Step 5: OCS → WCS using the DXF arbitrary axis algorithm
  const EoGeOcsTransform tmOcsToWcs(m_normal);

  return ((EoGeMatrix)tmNegBase * (EoGeMatrix)tmScale * (EoGeMatrix)tmZRot * (EoGeMatrix)tmInsertOcs *
      (EoGeMatrix)tmOcsToWcs);
}

EoDbPrimitive*& EoDbBlockReference::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbBlockReference(*this);
  return primitive;
}
void EoDbBlockReference::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  EoDbBlock* block{};
  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return; }

  const auto basePoint = block->BasePoint();

  for (std::int16_t col = 0; col < m_columnCount; ++col) {
    for (std::int16_t row = 0; row < m_rowCount; ++row) {
      // Compute per-instance insertion point in OCS (grid offset along OCS X/Y axes)
      auto ocsInsertionPoint = m_insertionPoint;
      ocsInsertionPoint.x += col * m_columnSpacing;
      ocsInsertionPoint.y += row * m_rowSpacing;

      auto transformMatrix = BuildTransformMatrix(basePoint, ocsInsertionPoint);

      view->PushModelTransform();
      view->SetLocalModelTransform(transformMatrix);

      block->Display(view, renderDevice);

      view->PopModelTransform();
    }
  }
}
void EoDbBlockReference::ExportToDxf(EoDxfInterface* writer) const {
  EoDxfInsert insert;
  PopulateDxfBaseProperties(&insert);

  insert.m_blockName = std::wstring(m_blockName.GetString());
  insert.m_insertionPoint = {m_insertionPoint.x, m_insertionPoint.y, m_insertionPoint.z};
  insert.m_xScaleFactor = m_scaleFactors.x;
  insert.m_yScaleFactor = m_scaleFactors.y;
  insert.m_zScaleFactor = m_scaleFactors.z;
  insert.m_rotationAngle = m_rotation;
  insert.m_extrusionDirection = {m_normal.x, m_normal.y, m_normal.z};
  insert.m_columnCount = m_columnCount;
  insert.m_rowCount = m_rowCount;
  insert.m_columnSpacing = m_columnSpacing;
  insert.m_rowSpacing = m_rowSpacing;

  if (!m_attributeHandles.empty()) { insert.SetAttributesFollow(true); }
  writer->AddInsert(insert);

  // Emit ATTRIB entities owned by this INSERT, then SEQEND
  if (!m_attributeHandles.empty()) {
    auto* document = AeSysDoc::GetDoc();
    for (const auto attribHandle : m_attributeHandles) {
      auto* primitive = document->FindPrimitiveByHandle(attribHandle);
      if (primitive != nullptr) { primitive->ExportToDxf(writer); }
    }
    // SEQEND terminates the ATTRIB sequence; its owner handle points to the INSERT
    EoDxfSeqend seqend;
    seqend.m_layer = insert.m_layer;
    seqend.m_ownerHandle = insert.m_handle;
    writer->AddSeqend(seqend);
  }
}
void EoDbBlockReference::AddReportToMessageList(const EoGePoint3d& point) {
  app.AddStringToMessageList(L"<BlockReference>");
  EoDbPrimitive::AddReportToMessageList(point);
  CString message;
  message.Format(L"  BlockName: %s", m_blockName.GetString());
  app.AddStringToMessageList(message);
  message.Format(L"  InsertionPoint: %s", m_insertionPoint.ToString().GetString());
  app.AddStringToMessageList(message);
  message.Format(L"  Scale Factors: %s", m_scaleFactors.ToString().GetString());
  app.AddStringToMessageList(message);
  message.Format(L"  Normal: %s", m_normal.ToString().GetString());
  app.AddStringToMessageList(message);
  message.Format(L"  Rotation Angle: %f", m_rotation);
  app.AddStringToMessageList(message);
  if (!m_attributeHandles.empty()) {
    message.Format(L"  Attributes: %zu", m_attributeHandles.size());
    app.AddStringToMessageList(message);
    for (const auto attributeHandle : m_attributeHandles) {
      message.Format(L"    ATTRIB Handle: %I64X", attributeHandle);
      app.AddStringToMessageList(message);
    }
  }
}

void EoDbBlockReference::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_insertionPoint);
}

void EoDbBlockReference::FormatExtra(CString& extra) {
  EoDbPrimitive::FormatExtra(extra);
  extra.AppendFormat(L"\tSegment Name;%s\tRotation Angle;%f\tAttributes;%zu", m_blockName.GetString(), m_rotation,
      m_attributeHandles.size());
  extra += L'\t';
}
void EoDbBlockReference::FormatGeometry(CString& geometry) {
  geometry += L"Insertion Point;" + m_insertionPoint.ToString();
  geometry += L"Normal;" + m_normal.ToString();
  geometry += L"Scale;" + m_scaleFactors.ToString();
}
EoGePoint3d EoDbBlockReference::GetControlPoint() {
  EoGePoint3d point;
  point = m_insertionPoint;
  return point;
}
void EoDbBlockReference::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoDbBlock* block{};

  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return; }

  auto tmIns = BuildTransformMatrix(block->BasePoint());

  view->PushModelTransform();
  view->SetLocalModelTransform(tmIns);

  block->GetExtents(view, ptMin, ptMax, transformMatrix);

  view->PopModelTransform();
}

bool EoDbBlockReference::IsInView(AeSysView* view) {
  // Test whether an instance of a block is wholly or partially within the current view volume.
  EoDbBlock* block{};

  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return false; }

  const auto basePoint = block->BasePoint();
  EoGeTransformMatrix transformMatrix = BuildTransformMatrix(basePoint);

  view->PushModelTransform();
  view->SetLocalModelTransform(transformMatrix);

  const bool bInView = block->IsInView(view);

  view->PopModelTransform();
  return bInView;
}

bool EoDbBlockReference::IsPointOnControlPoint(
    [[maybe_unused]] AeSysView* view, [[maybe_unused]] const EoGePoint4d& point) {
  return false;
}

EoGePoint3d EoDbBlockReference::SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;
  EoGePoint3d ptCtrl;

  EoDbBlock* block{};

  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return ptCtrl; }

  const auto basePoint = block->BasePoint();

  EoGeTransformMatrix transformMatrix = BuildTransformMatrix(basePoint);

  view->PushModelTransform();
  view->SetLocalModelTransform(transformMatrix);

  auto position = block->GetHeadPosition();
  while (position != nullptr) {
    auto* primitive = block->GetNext(position);
    ptCtrl = primitive->SelectAtControlPoint(view, point);
    if (sm_controlPointIndex != SHRT_MAX) {
      view->ModelTransformPoint(ptCtrl);
      break;
    }
  }
  view->PopModelTransform();
  return ptCtrl;
}

bool EoDbBlockReference::SelectUsingLine(
    [[maybe_unused]] AeSysView* view, [[maybe_unused]] EoGeLine line, [[maybe_unused]] EoGePoint3dArray&) {
  return false;
}

bool EoDbBlockReference::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoDbBlock* block{};

  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return false; }

  const auto basePoint = block->BasePoint();

  EoGeTransformMatrix transformMatrix = BuildTransformMatrix(basePoint);

  view->PushModelTransform();
  view->SetLocalModelTransform(transformMatrix);

  const bool bResult = block->SelectUsingRectangle(view, pt1, pt2);

  view->PopModelTransform();
  return bResult;
}
bool EoDbBlockReference::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  bool bResult{};

  EoDbBlock* block{};

  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return bResult; }
  const auto basePoint = block->BasePoint();

  EoGeTransformMatrix transformMatrix = BuildTransformMatrix(basePoint);

  view->PushModelTransform();
  view->SetLocalModelTransform(transformMatrix);

  auto position = block->GetHeadPosition();
  while (position != nullptr) {
    if ((block->GetNext(position))->SelectUsingPoint(view, point, ptProj)) {
      bResult = true;
      break;
    }
  }
  view->PopModelTransform();
  return bResult;
}

void EoDbBlockReference::SetInsertionPoint(const EoDxfGeometryBase3d& point) {
  m_insertionPoint.x = point.x;
  m_insertionPoint.y = point.y;
  m_insertionPoint.z = point.z;
}

void EoDbBlockReference::Transform(const EoGeTransformMatrix& transformMatrix) {
  m_insertionPoint = transformMatrix * m_insertionPoint;
  m_normal = transformMatrix * m_normal;

  if (Eo::IsGeometricallyZero(m_normal.x) && Eo::IsGeometricallyZero(m_normal.y)) {
    m_scaleFactors = transformMatrix * m_scaleFactors;
  }
}
void EoDbBlockReference::TranslateUsingMask(EoGeVector3d v, DWORD mask) {
  if (mask != 0) { m_insertionPoint += v; }
}

EoDbBlockReference* EoDbBlockReference::ReadLegacyInsertPeg(CFile& file) {
  const auto color = EoDb::ReadInt16(file);
  const auto lineType = EoDb::ReadInt16(file);
  CString name;
  EoDb::Read(file, name);
  const auto insertionPoint(EoDb::ReadPoint3d(file));
  const auto xAxis(EoDb::ReadVector3d(file));
  const auto yAxis(EoDb::ReadVector3d(file));
  const auto zAxis(EoDb::ReadVector3d(file));
  const auto numberOfColumns = EoDb::ReadInt16(file);
  const auto numberOfRows = EoDb::ReadInt16(file);
  const double columnSpacing = EoDb::ReadDouble(file);
  const double rowSpacing = EoDb::ReadDouble(file);
  // Convert legacy axis vectors to modern normal/scaleFactors/rotation representation.
  // Legacy format stores a full 3-axis local reference system; modern format stores
  // normal direction, per-axis scale factors, and a rotation angle relative to OCS X.
  const auto scaleX = xAxis.Length();
  const auto scaleY = yAxis.Length();
  const auto scaleZ = zAxis.Length();

  EoGeVector3d normal;
  if (!zAxis.IsNearNull()) {
    normal = zAxis.Unitized();
  } else {
    const auto cross = CrossProduct(xAxis, yAxis);
    normal = cross.IsNearNull() ? EoGeVector3d::positiveUnitZ : cross.Unitized();
  }

  double rotation = 0.0;
  if (scaleX > Eo::geometricTolerance) {
    const auto unitX = xAxis * (1.0 / scaleX);
    auto ocsXAxis = ComputeArbitraryAxis(normal);
    ocsXAxis.Unitize();
    auto ocsYAxis = CrossProduct(normal, ocsXAxis);
    rotation = atan2(DotProduct(unitX, ocsYAxis), DotProduct(unitX, ocsXAxis));
  }

  const EoGeVector3d scaleFactors(scaleX, scaleY, scaleZ);

  auto* blockReference = new EoDbBlockReference(static_cast<std::uint16_t>(color), static_cast<std::uint16_t>(lineType),
      name, insertionPoint, normal, scaleFactors, rotation);
  blockReference->m_columnCount = numberOfColumns;
  blockReference->m_rowCount = numberOfRows;
  blockReference->m_columnSpacing = columnSpacing;
  blockReference->m_rowSpacing = rowSpacing;

  return blockReference;
}

EoDbBlockReference* EoDbBlockReference::ReadFromPeg(CFile& file) {
  const auto color = EoDb::ReadInt16(file);
  const auto lineType = EoDb::ReadInt16(file);
  CString name;
  EoDb::Read(file, name);
  const auto insertionPoint(EoDb::ReadPoint3d(file));
  const auto normal(EoDb::ReadVector3d(file));
  const auto scaleFactors(EoDb::ReadVector3d(file));
  const double rotation = EoDb::ReadDouble(file);
  const auto numberOfColumns = EoDb::ReadInt16(file);
  const auto numberOfRows = EoDb::ReadInt16(file);
  const double columnSpacing = EoDb::ReadDouble(file);
  const double rowSpacing = EoDb::ReadDouble(file);

  auto* blockReference = new EoDbBlockReference(static_cast<std::uint16_t>(color), static_cast<std::uint16_t>(lineType),
      name, insertionPoint, normal, scaleFactors, rotation);
  blockReference->m_columnCount = numberOfColumns;
  blockReference->m_rowCount = numberOfRows;
  blockReference->m_columnSpacing = columnSpacing;
  blockReference->m_rowSpacing = rowSpacing;

  return blockReference;
}

bool EoDbBlockReference::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kGroupReferencePrimitive));
  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, EoDbLineTypeTable::LegacyLineTypeIndex(m_lineType));
  EoDb::Write(file, m_blockName);
  m_insertionPoint.Write(file);
  m_normal.Write(file);
  m_scaleFactors.Write(file);
  EoDb::WriteDouble(file, m_rotation);
  EoDb::WriteInt16(file, m_columnCount);
  EoDb::WriteInt16(file, m_rowCount);
  EoDb::WriteDouble(file, m_columnSpacing);
  EoDb::WriteDouble(file, m_rowSpacing);
  return true;
}

void EoDbBlockReference::WriteV2Extension(CFile& file) const {
  EoDb::WriteUInt16(file, static_cast<std::uint16_t>(m_attributeHandles.size()));
  for (const auto handle : m_attributeHandles) { EoDb::WriteUInt64(file, handle); }
}

void EoDbBlockReference::ReadV2Extension(CFile& file) {
  const auto count = EoDb::ReadUInt16(file);
  m_attributeHandles.clear();
  m_attributeHandles.reserve(count);
  for (std::uint16_t i = 0; i < count; ++i) { m_attributeHandles.push_back(EoDb::ReadUInt64(file)); }
}
