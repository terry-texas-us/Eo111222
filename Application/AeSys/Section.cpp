#include "StdAfx.h"

#include "Section.h"

Section::Section() : m_width(0.0), m_depth(0.0), m_properties(0) {}
Section::Section(double width, double depth, long properties) : m_width(width), m_depth(depth), m_properties(properties) {}

bool Section::operator==(const Section& section) { return Identical(section); }
bool Section::operator!=(const Section& section) { return !Identical(section); }
void Section::operator()(double width, double depth, long properties) {
  m_width = width;
  m_depth = depth;
  m_properties = properties;
}
void Section::SetWidth(double width) { m_width = width; }
void Section::SetDepth(double depth) { m_depth = depth; }

double Section::Width() const { return m_width; }
double Section::Depth() const { return m_depth; }
bool Section::Identical(const Section& section) const {
  return (m_width == section.m_width && m_depth == section.m_depth && m_properties == section.m_properties);
}
bool Section::IsRectangular() const { return (m_properties & Rectangular) == Rectangular; }
bool Section::IsRound() const { return (m_properties & Round) == Round; }
