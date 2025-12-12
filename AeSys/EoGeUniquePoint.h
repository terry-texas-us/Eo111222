#pragma once

#define btest(m, p) ((bool)(((((DWORD)m) >> ((int)p)) & 1UL) == 1 ? true : false))

class EoGeUniquePoint : public CObject {
 public:
  EoGePoint3d m_Point;
  int m_References;

 public:
  EoGeUniquePoint() : m_References(0), m_Point(EoGePoint3d::kOrigin) {}

  EoGeUniquePoint(const EoGeUniquePoint& other) {
    m_Point = other.m_Point;
    m_References = other.m_References;
  }

  EoGeUniquePoint& operator=(const EoGeUniquePoint& other) {
    if (this != &other) {
      m_Point = other.m_Point;
      m_References = other.m_References;
    }
    return *this;
  }
  EoGeUniquePoint(int references, const EoGePoint3d& point) {
    m_References = references;
    m_Point = point;
  }
};
