#include "drw_base.h"
#include "drw_classes.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

void DRW_Class::parseCode(int code, dxfReader* reader) {
  switch (code) {
    case 1:
      recName = reader->getUtf8String();
      break;
    case 2:
      className = reader->getUtf8String();
      break;
    case 3:
      appName = reader->getUtf8String();
      break;
    case 90:
      proxyFlag = reader->getInt32();
      break;
    case 91:
      instanceCount = reader->getInt32();
      break;
    case 280:
      wasaProxyFlag = reader->getInt32();
      break;
    case 281:
      entityFlag = reader->getInt32();
      break;
    default:
      break;
  }
}

void DRW_Class::write(dxfWriter* writer, DRW::Version ver) const {
  if (ver > DRW::Version::AC1009) {
    writer->writeString(0, "CLASS");
    writer->writeString(1, recName);
    writer->writeString(2, className);
    writer->writeString(3, appName);
    writer->writeInt32(90, proxyFlag);
    if (ver > DRW::Version::AC1015) {
      writer->writeInt32(91, instanceCount);
    }
    writer->writeInt16(280, wasaProxyFlag);
    writer->writeInt16(281, entityFlag);
  }
}
