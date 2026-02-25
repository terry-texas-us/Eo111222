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
  writer->writeString(0, "CLASS");
  writer->writeString(1, recName);
  writer->writeString(2, className);
  writer->writeString(3, appName);
  writer->writeInt32(90, proxyCapabilities);
  if (version >= DRW::Version::AC1018) { writer->writeInt32(91, instanceCount); }
  writer->writeInt16(280, wasAProxyFlag);
  writer->writeInt16(281, isAnEntityFlag);
}
