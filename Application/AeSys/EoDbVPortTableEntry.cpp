#include "Stdafx.h"

#include "EoDb.h"
#include "EoDbVPortTableEntry.h"

void EoDbVPortTableEntry::Read(CFile& file) {
  CString name;
  EoDb::Read(file, name);
  m_name = std::wstring(name.GetString());

  m_lowerLeftCorner.Read(file);
  m_upperRightCorner.Read(file);
  m_viewCenter.Read(file);
  m_snapBasePoint.Read(file);
  m_snapSpacing.Read(file);
  m_gridSpacing.Read(file);
  m_viewDirection.Read(file);
  m_viewTargetPoint.Read(file);

  m_viewHeight = EoDb::ReadDouble(file);
  m_viewAspectRatio = EoDb::ReadDouble(file);
  m_lensLength = EoDb::ReadDouble(file);
  m_frontClipPlane = EoDb::ReadDouble(file);
  m_backClipPlane = EoDb::ReadDouble(file);
  m_snapRotationAngle = EoDb::ReadDouble(file);
  m_viewTwistAngle = EoDb::ReadDouble(file);

  m_viewMode = EoDb::ReadInt16(file);
  m_circleZoomPercent = EoDb::ReadInt16(file);
  m_fastZoom = EoDb::ReadInt16(file);
  m_ucsIcon = EoDb::ReadInt16(file);
  m_snapOn = EoDb::ReadInt16(file);
  m_gridOn = EoDb::ReadInt16(file);
  m_snapStyle = EoDb::ReadInt16(file);
  m_snapIsopair = EoDb::ReadInt16(file);
  m_gridBehavior = EoDb::ReadInt16(file);
}

void EoDbVPortTableEntry::Write(CFile& file) const {
  EoDb::Write(file, CString(m_name.c_str()));

  m_lowerLeftCorner.Write(file);
  m_upperRightCorner.Write(file);
  m_viewCenter.Write(file);
  m_snapBasePoint.Write(file);
  m_snapSpacing.Write(file);
  m_gridSpacing.Write(file);
  m_viewDirection.Write(file);
  m_viewTargetPoint.Write(file);

  EoDb::WriteDouble(file, m_viewHeight);
  EoDb::WriteDouble(file, m_viewAspectRatio);
  EoDb::WriteDouble(file, m_lensLength);
  EoDb::WriteDouble(file, m_frontClipPlane);
  EoDb::WriteDouble(file, m_backClipPlane);
  EoDb::WriteDouble(file, m_snapRotationAngle);
  EoDb::WriteDouble(file, m_viewTwistAngle);

  EoDb::WriteInt16(file, m_viewMode);
  EoDb::WriteInt16(file, m_circleZoomPercent);
  EoDb::WriteInt16(file, m_fastZoom);
  EoDb::WriteInt16(file, m_ucsIcon);
  EoDb::WriteInt16(file, m_snapOn);
  EoDb::WriteInt16(file, m_gridOn);
  EoDb::WriteInt16(file, m_snapStyle);
  EoDb::WriteInt16(file, m_snapIsopair);
  EoDb::WriteInt16(file, m_gridBehavior);
}
