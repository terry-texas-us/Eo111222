#include "stdafx.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoGeReferenceSystem.h"

EoGeReferenceSystem::EoGeReferenceSystem()
    : m_Origin(EoGePoint3d::kOrigin), m_XDirection(EoGeVector3d::kXAxis), m_YDirection(EoGeVector3d::kYAxis) {}
EoGeReferenceSystem::EoGeReferenceSystem(const EoGePoint3d& origin, EoDbCharacterCellDefinition& ccd) {
  auto* activeView = AeSysView::GetActiveView();

  m_Origin = origin;

  EoGeVector3d vNorm = activeView->CameraDirection();

  m_YDirection = activeView->ViewUp();
  m_YDirection.RotAboutArbAx(vNorm, ccd.TextRotAngGet());

  m_XDirection = m_YDirection;
  m_XDirection.RotAboutArbAx(vNorm, -Eo::HalfPi);
  m_YDirection.RotAboutArbAx(vNorm, ccd.ChrSlantAngGet());
  m_XDirection *= 0.6 * ccd.ChrHgtGet() * ccd.ChrExpFacGet();
  m_YDirection *= ccd.ChrHgtGet();
}
EoGeReferenceSystem::EoGeReferenceSystem(const EoGePoint3d& origin, EoGeVector3d xDirection, EoGeVector3d yDirection)
    : m_Origin(origin), m_XDirection(xDirection), m_YDirection(yDirection) {}
EoGeReferenceSystem::EoGeReferenceSystem(const EoGeReferenceSystem& src) {
  m_Origin = src.m_Origin;
  m_XDirection = src.m_XDirection;
  m_YDirection = src.m_YDirection;
}
EoGeReferenceSystem& EoGeReferenceSystem::operator=(const EoGeReferenceSystem& src) {
  m_Origin = src.m_Origin;
  m_XDirection = src.m_XDirection;
  m_YDirection = src.m_YDirection;

  return (*this);
}
void EoGeReferenceSystem::GetUnitNormal(EoGeVector3d& normal) {
  normal = EoGeCrossProduct(m_XDirection, m_YDirection);
  normal.Normalize();
}
EoGePoint3d EoGeReferenceSystem::Origin() const { return m_Origin; }
EoGeTransformMatrix EoGeReferenceSystem::TransformMatrix() const {
  return EoGeTransformMatrix(m_Origin, m_XDirection, m_YDirection);
}

void EoGeReferenceSystem::Read(CFile& file) {
  m_Origin.Read(file);
  m_XDirection.Read(file);
  m_YDirection.Read(file);
}
void EoGeReferenceSystem::Rescale(EoDbCharacterCellDefinition& ccd) {
  EoGeVector3d vNorm;
  GetUnitNormal(vNorm);
  m_XDirection.Normalize();
  m_YDirection = m_XDirection;
  m_YDirection.RotAboutArbAx(vNorm, Eo::HalfPi + ccd.ChrSlantAngGet());
  m_XDirection *= 0.6 * ccd.ChrHgtGet() * ccd.ChrExpFacGet();
  m_YDirection *= ccd.ChrHgtGet();
}
void EoGeReferenceSystem::Set(const EoGePoint3d& origin, const EoGeVector3d xDirection, const EoGeVector3d yDirection) {
  m_Origin = origin;
  m_XDirection = xDirection;
  m_YDirection = yDirection;
}
void EoGeReferenceSystem::SetOrigin(const EoGePoint3d& origin) { m_Origin = origin; }
void EoGeReferenceSystem::SetXDirection(const EoGeVector3d& xDirection) { m_XDirection = xDirection; }
void EoGeReferenceSystem::SetYDirection(const EoGeVector3d& yDirection) { m_YDirection = yDirection; }
void EoGeReferenceSystem::Transform(EoGeTransformMatrix& tm) {
  m_Origin = tm * m_Origin;
  m_XDirection = tm * m_XDirection;
  m_YDirection = tm * m_YDirection;
}
void EoGeReferenceSystem::Write(CFile& file) {
  m_Origin.Write(file);
  m_XDirection.Write(file);
  m_YDirection.Write(file);
}
EoGeVector3d EoGeReferenceSystem::XDirection() const { return m_XDirection; }
EoGeVector3d EoGeReferenceSystem::YDirection() const { return m_YDirection; }
