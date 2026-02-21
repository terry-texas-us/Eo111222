#include "Stdafx.h"

#include "Eo.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGsViewport.h"

EoGsViewport::EoGsViewport(const EoGsViewport& other) {
  m_DeviceHeightInPixels = other.m_DeviceHeightInPixels;
  m_DeviceWidthInPixels = other.m_DeviceWidthInPixels;
  m_DeviceHeightInInches = other.m_DeviceHeightInInches;
  m_DeviceWidthInInches = other.m_DeviceWidthInInches;
  m_height = other.m_height;
  m_width = other.m_width;
}

EoGsViewport& EoGsViewport::operator=(const EoGsViewport& other) {
  m_DeviceHeightInPixels = other.m_DeviceHeightInPixels;
  m_DeviceWidthInPixels = other.m_DeviceWidthInPixels;
  m_width = other.m_width;
  m_height = other.m_height;
  m_DeviceHeightInInches = other.m_DeviceHeightInInches;
  m_DeviceWidthInInches = other.m_DeviceWidthInInches;

  return *this;
}

CPoint EoGsViewport::ProjectToClient(double x, double y, double w) const noexcept {
  assert(std::abs(w) > Eo::geometricTolerance && "w must not be zero");
  return {Eo::Round((x / w + 1.0) * ((m_width - 1.0) / 2.0)), Eo::Round((-y / w + 1.0) * ((m_height - 1.0) / 2.0))};
}

CPoint EoGsViewport::ProjectToClient(const EoGePoint4d& ndcPoint) const {
  return ProjectToClient(ndcPoint.x, ndcPoint.y, ndcPoint.w);
}

void EoGsViewport::ProjectToClient(CPoint* clientPoints, int numberOfPoints, const EoGePoint4d* ndcPoints) const {
  for (int i = 0; i < numberOfPoints; ++i) {
    clientPoints[i] = ProjectToClient(ndcPoints[i].x, ndcPoints[i].y, ndcPoints[i].w);
  }
}

void EoGsViewport::ProjectToClient(CPoint* clientPoints, EoGePoint4dArray& ndcPoints) const {
  int numberOfPoints = static_cast<int>(ndcPoints.GetSize());
  for (int i = 0; i < numberOfPoints; ++i) {
    clientPoints[i] = ProjectToClient(ndcPoints[i].x, ndcPoints[i].y, ndcPoints[i].w);
  }
}

void EoGsViewport::DoProjectionInverse(EoGePoint3d& pt) {
  pt.x = (pt.x * 2.0) / (m_width - 1.0) - 1.;
  pt.y = -((pt.y * 2.0) / (m_height - 1.0) - 1.0);
}

double EoGsViewport::HeightInInches() const { return m_height / (m_DeviceHeightInPixels / m_DeviceHeightInInches); }
double EoGsViewport::WidthInInches() const { return (m_width / (m_DeviceWidthInPixels / m_DeviceWidthInInches)); }
void EoGsViewport::SetDeviceHeightInInches(double height) { m_DeviceHeightInInches = height; }
void EoGsViewport::SetDeviceWidthInInches(double width) { m_DeviceWidthInInches = width; }
void EoGsViewport::SetSize(int width, int height) {
  m_width = static_cast<double>(width);
  m_height = static_cast<double>(height);
}
void EoGsViewport::SetDeviceHeightInPixels(double height) { m_DeviceHeightInPixels = height; }
void EoGsViewport::SetDeviceWidthInPixels(double width) { m_DeviceWidthInPixels = width; }
