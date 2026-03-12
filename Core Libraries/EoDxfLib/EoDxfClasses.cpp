#include "EoDxfClasses.h"

#include "EoDxfBase.h"
#include "EoDxfReader.h"
#include "EoDxfWriter.h"

void EoDxfClass::ParseCode(int code, EoDxfReader* reader) noexcept {
  switch (code) {
    case 1:
      m_classDxfRecordName = reader->GetWideString();
      break;
    case 2:
      m_cppClassName = reader->GetWideString();
      break;
    case 3:
      m_applicationName = reader->GetWideString();
      break;
    case 90:
      m_proxyCapabilitiesFlag = reader->GetInt32();
      break;
    case 91:
      m_instanceCount = reader->GetInt32();
      break;
    case 280:
      m_wasAProxyFlag = reader->GetInt16();
      break;
    case 281:
      m_isAnEntityFlag = reader->GetInt16();
      break;
    default:
      break;
  }
}

void EoDxfClass::clear() noexcept {
  m_classDxfRecordName.clear();
  m_cppClassName.clear();
  m_applicationName.clear();
  m_proxyCapabilitiesFlag = 0;
  m_instanceCount = 0;
  m_wasAProxyFlag = 0;
  m_isAnEntityFlag = 0;
}

void EoDxfClass::write(EoDxfWriter* writer, EoDxf::Version version) const noexcept {
  if (version < EoDxf::Version::AC1012) { return; }
  writer->WriteWideString(0, L"CLASS");
  writer->WriteWideString(1, m_classDxfRecordName);
  writer->WriteWideString(2, m_cppClassName);
  writer->WriteWideString(3, m_applicationName);
  writer->WriteInt32(90, m_proxyCapabilitiesFlag);
  if (version >= EoDxf::Version::AC1018) { writer->WriteInt32(91, m_instanceCount); }
  writer->WriteInt16(280, m_wasAProxyFlag);
  writer->WriteInt16(281, m_isAnEntityFlag);
}
