#include <cassert>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>

#include "EoDxfRead.h"

#include "EoDxfClasses.h"
#include "EoDxfEntities.h"
#include "EoDxfHeader.h"
#include "EoDxfInterface.h"
#include "EoDxfMLeader.h"
#include "EoDxfObjects.h"
#include "EoDxfReader.h"

namespace {

[[nodiscard]] EoDxfGroupCodeValuesVariant CreateRawRecordValue(int code, EoDxfReader* reader) {
  if (code <= 9 || code == 100 || code == 102 || code == 105 || (code >= 300 && code < 370) || (code >= 390 && code < 400) ||
      (code >= 410 && code < 420) || (code >= 430 && code < 440) || (code >= 470 && code < 481) ||
      (code >= 999 && code <= 1009)) {
    return EoDxfGroupCodeValuesVariant(code, reader->GetWideString());
  }
  if ((code >= 10 && code < 60) || (code >= 110 && code < 150) || (code >= 210 && code < 240) || (code >= 460 && code < 470) ||
      (code >= 1010 && code <= 1059)) {
    return EoDxfGroupCodeValuesVariant(code, reader->GetDouble());
  }
  if ((code >= 60 && code < 80) || (code >= 170 && code < 180) || (code >= 270 && code < 290) ||
      (code >= 370 && code < 390) || (code >= 400 && code < 410) || (code >= 1060 && code <= 1070)) {
    return EoDxfGroupCodeValuesVariant(code, reader->GetInt16());
  }
  if ((code >= 90 && code < 100) || (code >= 420 && code < 460) || code == 1071) {
    return EoDxfGroupCodeValuesVariant(code, reader->GetInt32());
  }
  if (code >= 160 && code < 170) { return EoDxfGroupCodeValuesVariant(code, reader->GetInt64()); }
  if (code >= 290 && code < 300) { return EoDxfGroupCodeValuesVariant(code, reader->GetBool()); }
  return EoDxfGroupCodeValuesVariant(code, reader->GetWideString());
}

}  // namespace

EoDxfRead::EoDxfRead(const std::filesystem::path& fileName) {
  m_fileName = fileName.wstring();
  m_reader = nullptr;
  m_applyExtrusion = false;
  m_ellipseParts = 128;  // parts number when convert ellipse to polyline
  m_headerParsed = false;
}

EoDxfRead::~EoDxfRead() {
  if (m_reader != nullptr) { delete m_reader; }
}

bool EoDxfRead::Read(EoDxfInterface* interface_, bool ext) {
  assert(!m_fileName.empty());
  bool isOk = false;
  m_applyExtrusion = ext;
  m_headerParsed = false;
  m_pendingComments.clear();
  std::ifstream filestr;
  if (interface_ == nullptr) { return isOk; }
  filestr.open(std::filesystem::path{m_fileName}, std::ios_base::in | std::ios::binary);
  if (!filestr.is_open()) { return isOk; }
  if (!filestr.good()) { return isOk; }

  char line[22]{};
  filestr.read(line, 22);
  filestr.close();

  // Declare sentinel indicating a binary DXF(DXB) file in AutoCAD, regardless
  // of the file extension, is a 22-byte sequence at the beginning of the file :
  // the ASCII string "AutoCAD Binary DXF" (18 bytes) followed by
  // a carriage return (CR, 0x0D), line feed(LF, 0x0A), substitute
  // character(SUB, 0x1A), and null byte(NUL, 0x00).
  char line2[22] = "AutoCAD Binary DXF\r\n";
  line2[20] = static_cast<char>(26);
  line2[21] = '\0';

  m_interface = interface_;
  if (strcmp(line, line2) == 0) {
    filestr.open(std::filesystem::path{m_fileName}, std::ios_base::in | std::ios::binary);
    m_binaryFile = true;
    // skip sentinel
    filestr.seekg(22, std::ios::beg);
    m_reader = new EoDxfReaderBinary(&filestr);
  } else {
    m_binaryFile = false;
    filestr.open(std::filesystem::path{m_fileName}, std::ios_base::in);
    m_reader = new EoDxfReaderAscii(&filestr);
  }
  isOk = ProcessDxf();
  filestr.close();
  delete m_reader;
  m_reader = nullptr;
  return isOk;
}

/********* Reader Process *********/

bool EoDxfRead::ProcessDxf() {
  int code;
  bool more = true;
  std::wstring sectionstr;
  //    section = secUnknown;
  while (m_reader->ReadRec(&code)) {
    if (code == 999) {
      if (m_headerParsed) {
        m_header.AddComment(m_reader->GetWideString());
      } else {
        m_pendingComments.emplace_back(m_reader->GetEncodedText());
      }
    } else if (code == 0) {
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"EOF") {
        return true;  // found EOF terminate
      }
      if (sectionstr == L"SECTION") {
        more = m_reader->ReadRec(&code);
        // check for early return if malformed dxf file
        if (!more) { return false; }
        if (code == 2) {
          sectionstr = m_reader->GetWideString();
          // found section, process it
          if (sectionstr == L"HEADER") {
            if (!ProcessHeader()) { return false; }
          } else if (sectionstr == L"CLASSES") {
            ProcessClasses();
          } else if (sectionstr == L"TABLES") {
            if (!ProcessTables()) { return false; }
          } else if (sectionstr == L"BLOCKS") {
            if (!ProcessBlocks()) { return false; }
          } else if (sectionstr == L"ENTITIES") {
            if (!ProcessEntities(false)) { return false; }
          } else if (sectionstr == L"OBJECTS") {
            if (!ProcessObjects()) { return false; }
          }
        }
      }
    }
    /*    if (!more)
            return true;*/
  }
  return true;
}

/********* Header Section *********/

bool EoDxfRead::ProcessHeader() {
  std::wstring zeroGroupTag;
  int code{};
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      zeroGroupTag = m_reader->GetWideString();
      if (zeroGroupTag == L"ENDSEC") {
        EoTcTextCodec commentDecoder;
        commentDecoder.SetCodePage(m_reader->GetCodePage());
        for (const auto& pendingComment : m_pendingComments) {
          m_header.AddComment(commentDecoder.DecodeText(pendingComment));
        }
        m_pendingComments.clear();
        m_headerParsed = true;
        m_interface->addHeader(&m_header);
        return true;
      }
    } else {
      m_header.ParseCode(code, m_reader);
    }
  }
  return true;
}

/********* Classes Section *********/

bool EoDxfRead::ProcessClasses() {
  std::wstring zeroGroupTag;
  int code{};
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      zeroGroupTag = m_reader->GetWideString();
      if (zeroGroupTag == L"CLASS") {
        EoDxfClass class_;
        while (m_reader->ReadRec(&code)) {
          if (code == 0) {
            zeroGroupTag = m_reader->GetWideString();
            if (zeroGroupTag == L"CLASS") {
              m_interface->addClass(class_);
              class_.clear();
            } else if (zeroGroupTag == L"ENDSEC") {
              m_interface->addClass(class_);
              return true;
            }
          } else {
            class_.ParseCode(code, m_reader);
          }
        }
      } else if (zeroGroupTag == L"ENDSEC") {
        return true;
      }
    }
  }
  return false;
}

/********* Tables Section *********/

bool EoDxfRead::ProcessTables() {
  int code;
  std::wstring sectionstr;
  bool more = true;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"TABLE") {
        more = m_reader->ReadRec(&code);
        if (!more) {
          return false;  // wrong dxf file
        }
        if (code == 2) {
          sectionstr = m_reader->GetWideString();

          if (sectionstr == L"LTYPE") {
            ProcessLType();
          } else if (sectionstr == L"LAYER") {
            ProcessLayer();
          } else if (sectionstr == L"STYLE") {
            ProcessTextStyle();
          } else if (sectionstr == L"VPORT") {
            ProcessVports();
          } else if (sectionstr == L"VIEW") {
            // processView();
          } else if (sectionstr == L"UCS") {
            // processUCS();
          } else if (sectionstr == L"APPID") {
            ProcessAppId();
          } else if (sectionstr == L"DIMSTYLE") {
            ProcessDimStyle();
          } else if (sectionstr == L"BLOCK_RECORD") {
            // processBlockRecord();
          }
        }
      } else if (sectionstr == L"ENDSEC") {
        return true;
      }
    }
  }
  return true;
}

bool EoDxfRead::ProcessLType() {
  int code;
  std::wstring sectionstr;
  bool reading = false;
  EoDxfLinetype linetype;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) {
        linetype.Update();
        m_interface->addLinetype(linetype);
      }
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"LTYPE") {
        reading = true;
        linetype.Reset();
      } else if (sectionstr == L"ENDTAB") {
        return true;
      }
    } else if (reading) {
      linetype.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool EoDxfRead::ProcessLayer() {
  int code;
  std::wstring sectionstr;
  bool reading = false;
  EoDxfLayer layer;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addLayer(layer); }
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"LAYER") {
        reading = true;
        layer.Reset();
      } else if (sectionstr == L"ENDTAB") {
        return true;
      }
    } else if (reading) {
      layer.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool EoDxfRead::ProcessDimStyle() {
  int code;
  std::wstring sectionstr;
  bool reading{};
  EoDxfDimensionStyle dimensionStyle;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addDimStyle(dimensionStyle); }
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"DIMSTYLE") {
        reading = true;
        dimensionStyle.Reset();
      } else if (sectionstr == L"ENDTAB") {
        return true;
      }
    } else if (reading) {
      dimensionStyle.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool EoDxfRead::ProcessTextStyle() {
  int code;
  std::wstring sectionstr;
  bool reading{};
  EoDxfTextStyle textStyle;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addTextStyle(textStyle); }
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"STYLE") {
        reading = true;
        textStyle.Reset();
      } else if (sectionstr == L"ENDTAB") {
        return true;
      }
    } else if (reading) {
      textStyle.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool EoDxfRead::ProcessVports() {
  int code;
  std::wstring sectionstr;
  bool reading{};
  EoDxfVPort viewport;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addVport(viewport); }
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"VPORT") {
        reading = true;
        viewport.Reset();
      } else if (sectionstr == L"ENDTAB") {
        return true;
      }
    } else if (reading) {
      viewport.ParseCode(code, m_reader);
    }
  }
  return true;
}

bool EoDxfRead::ProcessAppId() {
  int code;
  std::wstring sectionstr;
  bool reading = false;
  EoDxfAppId appId;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      if (reading) { m_interface->addAppId(appId); }
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"APPID") {
        reading = true;
        appId.Reset();
      } else if (sectionstr == L"ENDTAB") {
        return true;
      }
    } else if (reading) {
      appId.ParseCode(code, m_reader);
    }
  }
  return true;
}

/********* Block Section *********/

bool EoDxfRead::ProcessBlocks() {
  int code;
  std::wstring sectionstr;
  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      sectionstr = m_reader->GetWideString();
      if (sectionstr == L"BLOCK") {
        ProcessBlock();
      } else if (sectionstr == L"ENDSEC") {
        return true;
      }
    }
  }
  return true;
}

bool EoDxfRead::ProcessBlock() {
  int code;
  EoDxfBlock block;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addBlock(block);
        if (m_nextEntity == L"ENDBLK") {
          m_interface->endBlock();
          return true;  // found ENDBLK, terminate
        } else {
          ProcessEntities(true);
          m_interface->endBlock();
          return true;  // found ENDBLK, terminate
        }
      }
      default:
        block.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

/********* Entities Section *********/

bool EoDxfRead::ProcessEntities(bool isblock) {
  int code;
  if (!m_reader->ReadRec(&code)) { return false; }
  bool next = true;
  if (code == 0) {
    m_nextEntity = m_reader->GetWideString();
  } else if (!isblock) {
    return false;  // first record in entities is 0
  }
  do {
    if (m_nextEntity == L"ENDSEC" || m_nextEntity == L"ENDBLK") {
      return true;
    } else if (m_nextEntity == L"POINT") {
      ProcessPoint();
    } else if (m_nextEntity == L"LINE") {
      ProcessLine();
    } else if (m_nextEntity == L"CIRCLE") {
      ProcessCircle();
    } else if (m_nextEntity == L"ARC") {
      ProcessArc();
    } else if (m_nextEntity == L"ELLIPSE") {
      ProcessEllipse();
    } else if (m_nextEntity == L"TRACE") {
      ProcessTrace();
    } else if (m_nextEntity == L"SOLID") {
      ProcessSolid();
    } else if (m_nextEntity == L"INSERT") {
      ProcessInsert();
    } else if (m_nextEntity == L"LWPOLYLINE") {
      ProcessLWPolyline();
    } else if (m_nextEntity == L"POLYLINE") {
      ProcessPolyline();
    } else if (m_nextEntity == L"TEXT") {
      ProcessText();
    } else if (m_nextEntity == L"MTEXT") {
      ProcessMText();
    } else if (m_nextEntity == L"HATCH") {
      ProcessHatch();
    } else if (m_nextEntity == L"SPLINE") {
      ProcessSpline();
    } else if (m_nextEntity == L"3DFACE") {
      Process3dFace();
    } else if (m_nextEntity == L"VIEWPORT") {
      ProcessViewport();
    } else if (m_nextEntity == L"IMAGE") {
      ProcessImage();
    } else if (m_nextEntity == L"DIMENSION") {
      ProcessDimension();
    } else if (m_nextEntity == L"LEADER") {
      ProcessLeader();
    } else if (m_nextEntity == L"MULTILEADER" || m_nextEntity == L"MLEADER") {
      ProcessMLeader();
    } else if (m_nextEntity == L"RAY") {
      ProcessRay();
    } else if (m_nextEntity == L"XLINE") {
      ProcessXline();
    } else {
      if (m_reader->ReadRec(&code)) {
        if (code == 0) { m_nextEntity = m_reader->GetWideString(); }
      } else {
        return false;  // end of file without ENDSEC
      }
    }

  } while (next);
  return true;
}

bool EoDxfRead::ProcessEllipse() {
  int code;
  EoDxfEllipse ellipse;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        if (m_applyExtrusion) { ellipse.ApplyExtrusion(); }
        m_interface->addEllipse(ellipse);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        ellipse.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessTrace() {
  int code;
  EoDxfTrace trace;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        if (m_applyExtrusion) { trace.ApplyExtrusion(); }
        m_interface->addTrace(trace);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        trace.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessSolid() {
  int code;
  EoDxfSolid solid;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        if (m_applyExtrusion) { solid.ApplyExtrusion(); }
        m_interface->addSolid(solid);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        solid.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::Process3dFace() {
  int code;
  EoDxf3dFace face;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->add3dFace(face);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        face.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessViewport() {
  int code;
  EoDxfViewport viewport;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addViewport(viewport);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        viewport.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessPoint() {
  int code;
  EoDxfPoint point;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addPoint(point);
        return true;
      }
      default:
        point.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessLine() {
  int code;
  EoDxfLine line;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addLine(line);
        return true;
      }
      default:
        line.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessRay() {
  int code;
  EoDxfRay ray;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addRay(ray);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        ray.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessXline() {
  int code;
  EoDxfXline line;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addXline(line);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        line.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessCircle() {
  int code;
  EoDxfCircle circle;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        if (m_applyExtrusion) { circle.ApplyExtrusion(); }
        m_interface->addCircle(circle);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        circle.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessArc() {
  int code;
  EoDxfArc arc;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        if (m_applyExtrusion) { arc.ApplyExtrusion(); }
        m_interface->addArc(arc);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        arc.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessInsert() {
  int code;
  EoDxfInsert insert;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addInsert(insert);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        insert.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessLWPolyline() {
  int code;
  EoDxfLwPolyline polyline;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        if (m_applyExtrusion) { polyline.ApplyExtrusion(); }
        m_interface->addLWPolyline(polyline);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        polyline.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessPolyline() {
  int code;
  EoDxfPolyline polyline;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        if (m_nextEntity == L"VERTEX") { ProcessVertex(&polyline); }
        m_interface->addPolyline(polyline);
        return true;
      }
      default:
        polyline.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessVertex(EoDxfPolyline* polyline) {
  int code;
  auto* vertex = new EoDxfVertex();
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        polyline->appendVertex(vertex);
        m_nextEntity = m_reader->GetWideString();
        if (m_nextEntity == L"SEQEND") { return true; }
        if (m_nextEntity == L"VERTEX") {
          vertex = new EoDxfVertex();
          break;
        }
        // Unexpected entity, stop processing vertices
        return true;
      }
      default:
        vertex->ParseCode(code, m_reader);
        break;
    }
  }
  // Unexpected end of file while processing vertices - avoid leaking in-process vertex
  delete vertex;
  return true;
}

bool EoDxfRead::ProcessText() {
  int code;
  EoDxfText text;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addText(text);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        text.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessMText() {
  int code;
  EoDxfMText txt;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        txt.UpdateAngle();
        m_interface->addMText(txt);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        txt.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessHatch() {
  int code;
  EoDxfHatch hatch;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addHatch(&hatch);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        hatch.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessSpline() {
  int code;
  EoDxfSpline sp;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addSpline(&sp);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        sp.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessImage() {
  int code;
  EoDxfImage image;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addImage(&image);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        image.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessDimension() {
  int code;
  EoDxfDimension dimension;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        auto type = dimension.m_dimensionType & 0x0F;
        switch (type) {
          case 0: {
            EoDxfDimLinear d(dimension);
            m_interface->addDimLinear(&d);
            break;
          }
          case 1: {
            EoDxfAlignedDimension d(dimension);
            m_interface->addDimAlign(&d);
            break;
          }
          case 2: {
            EoDxf2LineAngularDimension d(dimension);
            m_interface->addDimAngular(&d);
            break;
          }
          case 3: {
            EoDxfDiametricDimension d(dimension);
            m_interface->addDimDiametric(&d);
            break;
          }
          case 4: {
            EoDxfRadialDimension d(dimension);
            m_interface->addDimRadial(&d);
            break;
          }
          case 5: {
            EoDxf3PointAngularDimension d(dimension);
            m_interface->addDimAngular3P(&d);
            break;
          }
          case 6: {
            EoDxfOrdinateDimension d(dimension);
            m_interface->addDimOrdinate(&d);
            break;
          }
        }
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        dimension.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessLeader() {
  int code;
  EoDxfLeader leader;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addLeader(&leader);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        leader.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessMLeader() {
  int code;
  EoDxfMLeader mLeader;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->addMLeader(&mLeader);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        mLeader.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

/********* Objects Section *********/

bool EoDxfRead::ProcessObjects() {
  int code;
  if (!m_reader->ReadRec(&code)) { return false; }
  if (code == 0) {
    m_nextEntity = m_reader->GetWideString();
  } else {
    return false;  // first record in objects is 0
  }
  for (;;) {
    if (m_nextEntity == L"ENDSEC") {
      return true;  // found ENDSEC terminate
    } else if (m_nextEntity == L"IMAGEDEF") {
      ProcessImageDef();
    } else {
      ProcessUnsupportedObject();
    }
  }
}

bool EoDxfRead::ProcessImageDef() {
  int code;
  EoDxfImageDefinition imageDefinition;
  while (m_reader->ReadRec(&code)) {
    switch (code) {
      case 0: {
        m_nextEntity = m_reader->GetWideString();
        m_interface->linkImage(&imageDefinition);
        return true;  // found new entity or ENDSEC, terminate
      }
      default:
        imageDefinition.ParseCode(code, m_reader);
        break;
    }
  }
  return true;
}

bool EoDxfRead::ProcessUnsupportedObject() {
  int code;
  EoDxfUnsupportedObject unsupportedObject;
  unsupportedObject.m_objectType = m_nextEntity;

  while (m_reader->ReadRec(&code)) {
    if (code == 0) {
      m_nextEntity = m_reader->GetWideString();
      m_interface->addUnsupportedObject(unsupportedObject);
      return true;
    }
    unsupportedObject.m_values.push_back(CreateRawRecordValue(code, m_reader));
  }

  if (!unsupportedObject.m_objectType.empty()) {
    m_interface->addUnsupportedObject(unsupportedObject);
    return true;
  }
  return false;
}
