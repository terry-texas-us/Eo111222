#include "stdafx.h"

EoGsViewport::EoGsViewport(const EoGsViewport& viewport) {
  m_DeviceHeightInPixels = viewport.m_DeviceHeightInPixels;
  m_DeviceWidthInPixels = viewport.m_DeviceWidthInPixels;
  m_DeviceHeightInInches = viewport.m_DeviceHeightInInches;
  m_DeviceWidthInInches = viewport.m_DeviceWidthInInches;
  m_Height = viewport.m_Height;
  m_Width = viewport.m_Width;
}
EoGsViewport& EoGsViewport::operator=(const EoGsViewport& viewport) {
  m_DeviceHeightInPixels = viewport.m_DeviceHeightInPixels;
  m_DeviceWidthInPixels = viewport.m_DeviceWidthInPixels;
  m_Width = viewport.m_Width;
  m_Height = viewport.m_Height;
  m_DeviceHeightInInches = viewport.m_DeviceHeightInInches;
  m_DeviceWidthInInches = viewport.m_DeviceWidthInInches;

  return *this;
}
CPoint EoGsViewport::DoProjection(const EoGePoint4d& pt) {
  CPoint pnt;

  pnt.x = EoRound((pt.x / pt.w + 1.) * ((m_Width - 1.0) / 2.0));
  pnt.y = EoRound((-pt.y / pt.w + 1.) * ((m_Height - 1.0) / 2.0));

  return pnt;
}
void EoGsViewport::DoProjection(CPoint* pnt, int iPts, EoGePoint4d* pt) {
  for (int i = 0; i < iPts; i++) {
    pt[i] /= pt[i].w;

    pnt[i].x = EoRound((pt[i].x + 1.) * ((m_Width - 1.0) / 2.0));
    pnt[i].y = EoRound((-pt[i].y + 1.) * ((m_Height - 1.0) / 2.0));
  }
}
void EoGsViewport::DoProjection(CPoint* pnt, EoGePoint4dArray& pointsArray) {
  int iPts = (int)pointsArray.GetSize();

  for (int i = 0; i < iPts; i++) {
    pointsArray[i] /= pointsArray[i].w;

    pnt[i].x = EoRound((pointsArray[i].x + 1.) * ((m_Width - 1.0) / 2.0));
    pnt[i].y = EoRound((-pointsArray[i].y + 1.) * ((m_Height - 1.0) / 2.0));
  }
}
void EoGsViewport::DoProjectionInverse(EoGePoint3d& pt) {
  pt.x = (pt.x * 2.) / (m_Width - 1.) - 1.;
  pt.y = -((pt.y * 2.) / (m_Height - 1.) - 1.);
}
double EoGsViewport::Height() const { return m_Height; }
double EoGsViewport::HeightInInches() const { 
  return m_Height / (m_DeviceHeightInPixels / m_DeviceHeightInInches); }
double EoGsViewport::Width() const { return m_Width; }
double EoGsViewport::WidthInInches() const {
  return (m_Width / (m_DeviceWidthInPixels / m_DeviceWidthInInches));
}
void EoGsViewport::SetDeviceHeightInInches(const double height) { m_DeviceHeightInInches = height; }
void EoGsViewport::SetDeviceWidthInInches(const double width) { m_DeviceWidthInInches = width; }
void EoGsViewport::SetSize(int width, int height) {
  m_Width = static_cast<double>(width);
  m_Height = static_cast<double>(height);
}
void EoGsViewport::SetDeviceHeightInPixels(const double height) { m_DeviceHeightInPixels = height; }
void EoGsViewport::SetDeviceWidthInPixels(const double width) { m_DeviceWidthInPixels = width; }
