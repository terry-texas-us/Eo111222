#include "Stdafx.h"

#include <algorithm>
#include <cstdlib>

#include "EoDbLineType.h"

EoDbLineType::EoDbLineType() : m_Index(0), m_Name(L""), m_Description(L""), m_DashElements() {}

EoDbLineType::EoDbLineType(std::int16_t index, const CString& name, const CString& description, std::uint16_t numberOfDashElements,
                           double* dashLengths)
    : m_Index(index), m_Name(name), m_Description(description) {
  m_DashElements.reserve(numberOfDashElements);
  for (auto i = 0; i < numberOfDashElements; i++) { m_DashElements.push_back(std::clamp(dashLengths[i], -99.0, 99.0)); }
}

EoDbLineType::EoDbLineType(const EoDbLineType& other)
    : m_Index(other.m_Index), m_Name(other.m_Name), m_Description(other.m_Description), m_DashElements(other.m_DashElements) {}

EoDbLineType& EoDbLineType::operator=(const EoDbLineType& other) {
  if (this != &other) {
    m_Index = other.m_Index;
    m_Name = other.m_Name;
    m_Description = other.m_Description;
    m_DashElements = other.m_DashElements;
  }
  return (*this);
}

double EoDbLineType::GetPatternLength() const {
  double length{};

  for (double dash : m_DashElements) { length += std::abs(dash); }
  return length;
}
