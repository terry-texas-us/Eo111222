#include "drw_classes.h"

#include "drw_base.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

void DRW_Class::parseCode(int code, dxfReader* reader) noexcept {
  switch (code) {
    case 1:
      recName = reader->GetUtf8String();
      break;
    case 2:
      className = reader->GetUtf8String();
      break;
    case 3:
      appName = reader->GetUtf8String();
      break;
    case 90:
      proxyCapabilities = reader->GetInt32();
      break;
    case 91:
      instanceCount = reader->GetInt32();
      break;
    case 280:
      wasAProxyFlag = reader->GetInt32();
      break;
    case 281:
      isAnEntityFlag = reader->GetInt32();
      break;
    default:
      break;
  }
}

void DRW_Class::clear() noexcept {
  recName.clear();
  className.clear();
  appName.clear();
  proxyCapabilities = 0;
  instanceCount = 0;
  wasAProxyFlag = 0;
  isAnEntityFlag = 0;
}

void DRW_Class::write(dxfWriter* writer, DRW::Version version) const noexcept {
  if (version < DRW::Version::AC1012) { return; }
  writer->WriteString(0, "CLASS");
  writer->WriteString(1, recName);
  writer->WriteString(2, className);
  writer->WriteString(3, appName);
  writer->WriteInt32(90, proxyCapabilities);
  if (version >= DRW::Version::AC1018) { writer->WriteInt32(91, instanceCount); }
  writer->WriteInt16(280, wasAProxyFlag);
  writer->WriteInt16(281, isAnEntityFlag);
}
