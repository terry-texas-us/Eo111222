#include "stdafx.h"

#include "EoDbLineType.h"

EoDbLineType::EoDbLineType() {
  m_NumberOfDashElements = 0;
  m_DashElements = nullptr;
}
EoDbLineType::EoDbLineType(EoUInt16 index, const CString& name, const CString& description,
                           EoUInt16 numberOfDashElements, double* dashLengths) {
  m_Index = index;
  m_Name = name;
  m_Description = description;
  m_NumberOfDashElements = numberOfDashElements;
  if (m_NumberOfDashElements == 0) {
    m_DashElements = 0;
  } else {
    m_DashElements = new double[m_NumberOfDashElements];
    for (int i = 0; i < m_NumberOfDashElements; i++) { m_DashElements[i] = EoMax(-99., EoMin(dashLengths[i], 99.)); }
  }
}
EoDbLineType::EoDbLineType(const EoDbLineType& lineType) {
  m_Index = lineType.m_Index;
  m_Name = lineType.m_Name;
  m_Description = lineType.m_Description;
  m_NumberOfDashElements = lineType.m_NumberOfDashElements;
  m_DashElements = new double[m_NumberOfDashElements];
  for (int i = 0; i < m_NumberOfDashElements; i++) { m_DashElements[i] = lineType.m_DashElements[i]; }
}
EoDbLineType::~EoDbLineType() { delete[] m_DashElements; }
EoDbLineType& EoDbLineType::operator=(const EoDbLineType& lineType) {
  m_Index = lineType.m_Index;
  m_Name = lineType.m_Name;
  m_Description = lineType.m_Description;
  m_NumberOfDashElements = lineType.m_NumberOfDashElements;
  m_DashElements = new double[m_NumberOfDashElements];
  for (int i = 0; i < m_NumberOfDashElements; i++) { m_DashElements[i] = lineType.m_DashElements[i]; }
  return *this;
}
double EoDbLineType::GetPatternLen() {
  double dLen = 0.;

  for (int i = 0; i < m_NumberOfDashElements; i++) dLen += fabs(m_DashElements[i]);

  return (dLen);
}
