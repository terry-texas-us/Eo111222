#include "Stdafx.h"

#include "EoDbCharacterCellDefinition.h"
#include "EoGeVector3d.h"

EoDbCharacterCellDefinition::EoDbCharacterCellDefinition() {
  m_dChrHgt = 0.1;
  m_dChrExpFac = 1.;
  m_dTextRotAng = 0.;
  m_dChrSlantAng = 0.;
}
EoDbCharacterCellDefinition::EoDbCharacterCellDefinition(double dTxtOffAng, double dChrSlantAng, double dChrExpFac, double dChrHgt) {
  m_dChrHgt = dChrHgt;
  m_dChrExpFac = dChrExpFac;
  m_dTextRotAng = dTxtOffAng;
  m_dChrSlantAng = dChrSlantAng;
}
EoDbCharacterCellDefinition::EoDbCharacterCellDefinition(const EoDbCharacterCellDefinition& fd) {
  m_dChrHgt = fd.m_dChrHgt;
  m_dChrExpFac = fd.m_dChrExpFac;
  m_dTextRotAng = fd.m_dTextRotAng;
  m_dChrSlantAng = fd.m_dChrSlantAng;
}
EoDbCharacterCellDefinition& EoDbCharacterCellDefinition::operator=(const EoDbCharacterCellDefinition& fd) {
  m_dChrHgt = fd.m_dChrHgt;
  m_dChrExpFac = fd.m_dChrExpFac;
  m_dTextRotAng = fd.m_dTextRotAng;
  m_dChrSlantAng = fd.m_dChrSlantAng;

  return (*this);
}
void CharCellDef_EncdRefSys(const EoGeVector3d& normal, EoDbCharacterCellDefinition& ccd, EoGeVector3d& xAxis, EoGeVector3d& yAxis) {
  xAxis = ComputeArbitraryAxis(normal);
  xAxis.RotAboutArbAx(normal, ccd.TextRotAngGet());

  yAxis = EoGeCrossProduct(normal, xAxis);

  xAxis *= 0.6 * ccd.ChrHgtGet() * ccd.ChrExpFacGet();

  yAxis.RotAboutArbAx(normal, ccd.ChrSlantAngGet());
  yAxis *= ccd.ChrHgtGet();
}
