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
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGeMatrix.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "drw_base.h"

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

EoDbBlockReference::EoDbBlockReference(const EoDbBlockReference& other) {
  m_blockName = other.m_blockName;
  m_insertionPoint = other.m_insertionPoint;
  m_normal = other.m_normal;
  m_scaleFactors = other.m_scaleFactors;
  m_rotation = other.m_rotation;
  m_columnCount = other.m_columnCount;
  m_rowCount = other.m_rowCount;
  m_columnSpacing = other.m_columnSpacing;
  m_rowSpacing = other.m_rowSpacing;
}
EoDbBlockReference::EoDbBlockReference(std::uint16_t color, std::uint16_t lineType, const CString& name,
    const EoGePoint3d& point, const EoGeVector3d& normal, const EoGeVector3d scaleFactors, double rotation)
    : m_blockName(name), m_insertionPoint(point), m_normal(normal), m_scaleFactors(scaleFactors) {
  m_color = static_cast<std::int16_t>(color);
  m_lineTypeIndex = static_cast<std::int16_t>(lineType);

  m_rotation = rotation;
  m_columnCount = 1;
  m_rowCount = 1;
  m_columnSpacing = 0.0;
  m_rowSpacing = 0.0;
}
const EoDbBlockReference& EoDbBlockReference::operator=(const EoDbBlockReference& other) {
  m_blockName = other.m_blockName;
  m_insertionPoint = other.m_insertionPoint;
  m_normal = other.m_normal;
  m_scaleFactors = other.m_scaleFactors;
  m_rotation = other.m_rotation;
  m_columnCount = other.m_columnCount;
  m_rowCount = other.m_rowCount;
  m_columnSpacing = other.m_columnSpacing;
  m_rowSpacing = other.m_rowSpacing;

  return (*this);
}
void EoDbBlockReference::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  EoDbBlock* Block{};
  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, Block) == 0) { return; }

  CString label{L"<BlockReference>"};
  auto hti = tvAddItem(tree, parent, label.GetBuffer(), this);

  ((EoDbGroup*)Block)->AddPrimsToTreeViewControl(tree, hti);
}

EoGeTransformMatrix EoDbBlockReference::BuildTransformMatrix(const EoGePoint3d& insertionPoint) const {
  // TODO: Validate normal vector

  EoGeTransformMatrix tm1;
  tm1.Translate(EoGeVector3d(insertionPoint, EoGePoint3d::kOrigin));
  EoGeTransformMatrix tm2;
  tm2.Scale(m_scaleFactors);
  auto zAxisRotation = EoGeTransformMatrix::ZAxisRotation(std::sin(m_rotation), std::cos(m_rotation));
  EoGeTransformMatrix tm4(EoGePoint3d::kOrigin, m_normal);
  EoGeTransformMatrix tm5;
  tm5.Translate(EoGeVector3d(EoGePoint3d::kOrigin, m_insertionPoint));

  return ((EoGeMatrix)tm1 * (EoGeMatrix)tm2 * (EoGeMatrix)zAxisRotation * (EoGeMatrix)tm4 * (EoGeMatrix)tm5);
}

EoDbPrimitive*& EoDbBlockReference::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbBlockReference(*this);
  return primitive;
}
void EoDbBlockReference::Display(AeSysView* view, CDC* deviceContext) {
  EoDbBlock* block{};
  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return; }

  auto transformMatrix = BuildTransformMatrix(block->BasePoint());

  view->PushModelTransform();
  view->SetLocalModelTransform(transformMatrix);

  block->Display(view, deviceContext);

  view->PopModelTransform();
}
void EoDbBlockReference::AddReportToMessageList(const EoGePoint3d&) {
  CString message;
  message.Format(L"<BlockReference> Color: %s Line Type: %s BlockName %s Layer: %s", FormatPenColor().GetString(),
      FormatLineType().GetString(), m_blockName.GetString(), m_layerName.c_str());
  app.AddStringToMessageList(message);
  message.Format(L"  InsertionPoint: %s", m_insertionPoint.ToString().GetString());
  app.AddStringToMessageList(message);
  message.Format(L"  Scale Factors: %s", m_scaleFactors.ToString().GetString());
  app.AddStringToMessageList(message);
  message.Format(L"  Normal: %s", m_normal.ToString().GetString());
  app.AddStringToMessageList(message);
  message.Format(L"  Rotation Angle: %f", m_rotation);
  app.AddStringToMessageList(message);
}

void EoDbBlockReference::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  points.Add(m_insertionPoint);
}

void EoDbBlockReference::FormatExtra(CString& extra) {
  extra.Format(L"Color;%s\tStyle;%s\tSegment Name;%s\tRotation Angle;%f", FormatPenColor().GetString(),
      FormatLineType().GetString(), m_blockName.GetString(), m_rotation);
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
  EoDbBlock* Block{};

  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, Block) == 0) { return false; }

  EoGePoint3d basePoint = Block->BasePoint();

  EoGeTransformMatrix transformMatrix = BuildTransformMatrix(basePoint);

  view->PushModelTransform();
  view->SetLocalModelTransform(transformMatrix);

  bool bInView = Block->IsInView(view);

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

  EoGePoint3d basePoint = block->BasePoint();

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

  EoGePoint3d basePoint = block->BasePoint();

  EoGeTransformMatrix transformMatrix = BuildTransformMatrix(basePoint);

  view->PushModelTransform();
  view->SetLocalModelTransform(transformMatrix);

  bool bResult = block->SelectUsingRectangle(view, pt1, pt2);

  view->PopModelTransform();
  return bResult;
}
bool EoDbBlockReference::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  bool bResult{};

  EoDbBlock* block{};

  if (AeSysDoc::GetDoc()->LookupBlock(m_blockName, block) == 0) { return bResult; }
  EoGePoint3d basePoint = block->BasePoint();

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

void EoDbBlockReference::SetInsertionPoint(const DRW_Coord& point) {
  m_insertionPoint.x = point.x;
  m_insertionPoint.y = point.y;
  m_insertionPoint.z = point.z;
}

void EoDbBlockReference::Transform(const EoGeTransformMatrix& transformMatrix) {
  m_insertionPoint = transformMatrix * m_insertionPoint;
  m_normal = transformMatrix * m_normal;

  if (std::abs(m_normal.x) < Eo::geometricTolerance && std::abs(m_normal.y) <= Eo::geometricTolerance) {
    m_scaleFactors = transformMatrix * m_scaleFactors;
  }
}
void EoDbBlockReference::TranslateUsingMask(EoGeVector3d v, DWORD mask) {
  if (mask != 0) { m_insertionPoint += v; }
}
bool EoDbBlockReference::Write(CFile& file) {
  EoDb::Write(file, std::uint16_t(EoDb::kGroupReferencePrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  EoDb::Write(file, m_blockName);
  m_insertionPoint.Write(file);
  m_normal.Write(file);
  m_scaleFactors.Write(file);
  EoDb::Write(file, m_rotation);
  EoDb::Write(file, m_columnCount);
  EoDb::Write(file, m_rowCount);
  EoDb::Write(file, m_columnSpacing);
  EoDb::Write(file, m_rowSpacing);

  return true;
}
