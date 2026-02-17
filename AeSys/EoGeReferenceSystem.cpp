#include "Stdafx.h"

#include "AeSysView.h"
#include "Eo.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoGePoint3d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

EoGeReferenceSystem::EoGeReferenceSystem(
    const EoGePoint3d& origin, const EoDbCharacterCellDefinition& characterCellDefinition)
    : m_origin{origin} {
  auto* activeView = AeSysView::GetActiveView();
  auto cameraDirection = activeView->CameraDirection();

  m_yDirection = activeView->ViewUp();
  m_yDirection.RotAboutArbAx(cameraDirection, characterCellDefinition.RotationAngle());

  m_xDirection = m_yDirection;
  m_xDirection.RotAboutArbAx(cameraDirection, -Eo::HalfPi);
  m_yDirection.RotAboutArbAx(cameraDirection, characterCellDefinition.SlantAngle());
  m_xDirection *= Eo::defaultCharacterCellAspectRatio * characterCellDefinition.Height() *
                  characterCellDefinition.ExpansionFactor();
  m_yDirection *= characterCellDefinition.Height();
}

EoGeReferenceSystem::EoGeReferenceSystem(const EoGePoint3d& origin, EoGeVector3d xDirection, EoGeVector3d yDirection)
    : m_origin{origin}, m_xDirection{xDirection}, m_yDirection{yDirection} {}

EoGeVector3d EoGeReferenceSystem::UnitNormal() const {
  EoGeVector3d normal = CrossProduct(m_xDirection, m_yDirection);
  normal.Normalize();
  return normal;
}

EoGeTransformMatrix EoGeReferenceSystem::TransformMatrix() const {
  return EoGeTransformMatrix(m_origin, m_xDirection, m_yDirection);
}

void EoGeReferenceSystem::Rescale(const EoDbCharacterCellDefinition& characterCellDefinition) {
  EoGeVector3d normal = UnitNormal();
  m_xDirection.Normalize();
  m_yDirection = m_xDirection;
  m_yDirection.RotAboutArbAx(normal, Eo::HalfPi + characterCellDefinition.SlantAngle());
  m_xDirection *= Eo::defaultCharacterCellAspectRatio * characterCellDefinition.Height() *
                  characterCellDefinition.ExpansionFactor();
  m_yDirection *= characterCellDefinition.Height();
}

void EoGeReferenceSystem::Transform(const EoGeTransformMatrix& transformMatrix) {
  m_origin = transformMatrix * m_origin;
  m_xDirection = transformMatrix * m_xDirection;
  m_yDirection = transformMatrix * m_yDirection;
}

void EoGeReferenceSystem::Read(CFile& file) {
  m_origin.Read(file);
  m_xDirection.Read(file);
  m_yDirection.Read(file);
}

void EoGeReferenceSystem::Write(CFile& file) {
  m_origin.Write(file);
  m_xDirection.Write(file);
  m_yDirection.Write(file);
}
