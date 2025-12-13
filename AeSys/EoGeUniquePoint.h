#pragma once

#define btest(m, p) ((bool)(((((DWORD)m) >> ((int)p)) & 1UL) == 1 ? true : false))

class EoGeUniquePoint : public CObject {
 public:
  EoGePoint3d m_Point = EoGePoint3d::kOrigin;
  int m_References = 0;

 public:
  EoGeUniquePoint() = default;

  EoGeUniquePoint(const EoGePoint3d& point, int references) : m_Point(point), m_References(references) {}

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
};
