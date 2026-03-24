#pragma once

#include <string>
#include <type_traits>
#include <variant>

#include "AeSysDoc.h"
#include "Eo.h"
#include "EoDbBlock.h"
#include "EoDbHeaderSection.h"
#include "EoDbPrimitive.h"
#include "EoDxfAttributes.h"
#include "EoDxfClasses.h"
#include "EoDxfEntities.h"
#include "EoDxfHeader.h"
#include "EoDxfInterface.h"
#include "EoDxfMLeader.h"
#include "EoDxfObjects.h"
#include "EoDxfSpline.h"

#include "EoDxfWrite.h"

class EoDbBlockReference;
class EoDbText;

// Minimal implementation of EoDxfInterface
// In a real scenario, implement these methods to handle the parsed entities
class EoDbDxfInterface : public EoDxfInterface {
 public:
  EoDbDxfInterface(AeSysDoc* document) : m_document(document) {}

  /// @brief Sets the DXF writer back-pointer for write-mode operation.
  /// When set, Add* methods forward to the writer instead of importing.
  void SetDxfWriter(EoDxfWrite* dxfWriter) noexcept { m_dxfWriter = dxfWriter; }

  void AddHeader(const EoDxfHeader* header) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddHeader called\n");
    ConvertHeaderSection(header, m_document);
  }

  void AddClass(const EoDxfClass& class_) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddClass called\n");
    ConvertClassesSection(class_, m_document);
  }

  // Table objects

  void AddAppId(const EoDxfAppId& appId) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAppId called\n");
    ConvertAppIdTable(appId, m_document);
  }
  void AddDimStyle(const EoDxfDimensionStyle& dimensionStyle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddDimStyle called\n");
    ConvertDimStyle(dimensionStyle, m_document);
  }

  void AddLayer(const EoDxfLayer& layer) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLayer called\n");
    ConvertLayerTable(layer, m_document);
  }
  void AddLinetype(const EoDxfLinetype& lType) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLinetype called\n");
    ConvertLinetypesTable(lType, m_document);
  }
  void AddTextStyle(const EoDxfTextStyle& textStyle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddTextStyle called\n");
    ConvertTextStyleTable(textStyle, m_document);
  }

  void AddVport(const EoDxfVPort& viewport) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddVport called\n");
    ConvertVPortTable(viewport, m_document);
  }

  // Blocks
  void AddBlock(const EoDxfBlock& block) override {
    m_inBlockDefinition = true;
    m_blockName = block.m_blockName;
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddBlock <%s>\n", m_blockName.c_str());
    m_currentOpenBlockDefinition = ConvertBlock(block, m_document);
  }
  void SetBlock(const int handle) override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::SetBlock\n");
    ConvertBlockSet(handle, m_document);
  }
  void EndBlock() override {
    ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::EndBlock\n");
    m_inBlockDefinition = false;
    m_currentOpenBlockDefinition = nullptr;
    m_blockName.clear();
    ConvertBlockEnd(m_document);
  }

  // AutoDesk DXF Reference
  // https://help.autodesk.com/view/OARX/2026/ENU/?guid=GUID-235B22E0-A567-4CF6-92D3-38A2306D73F3

  void Add3dFace(const EoDxf3dFace& _3dFace) override {
    countOf3dFace++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::Add3dFace - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::Add3dFace - entities section\n");
    }
    Convert3dFaceEntity(_3dFace, m_document);
  }
  // 3DSOLID not implemented

  void AddAcadProxyEntity(const EoDxfAcadProxyEntity& proxyEntity) override {
    countOfAcadProxyEntity++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddAcadProxyEntity - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAcadProxyEntity - entities section\n");
    }
    ConvertAcadProxyEntity(proxyEntity, m_document);
  }

  void AddArc(const EoDxfArc& arc) override {
    if (m_dxfWriter) {
      auto mutableArc = arc;
      mutableArc.m_space = m_currentExportSpace;
      m_dxfWriter->WriteArc(&mutableArc);
      return;
    }
    countOfArc++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddArc - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddArc - entities section\n");
    }
    ConvertArcEntity(arc, m_document);
  }

  void AddAttDef(const EoDxfAttDef& attdef) override {
    countOfAttDef++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddAttDef - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddAttDef - entities section\n");
    }
    ConvertAttDefEntity(attdef, m_document);
  }

  void AddAttrib(const EoDxfAttrib& attrib) override;

  void AddCircle(const EoDxfCircle& circle) override {
    if (m_dxfWriter) {
      auto mutableCircle = circle;
      mutableCircle.m_space = m_currentExportSpace;
      m_dxfWriter->WriteCircle(&mutableCircle);
      return;
    }
    countOfCircle++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddCircle - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddCircle - entities section\n");
    }
    ConvertCircleEntity(circle, m_document);
  }

  // Dimensions

  void AddDimAlign([[maybe_unused]] const EoDxfAlignedDimension* dimAlign) override { countOfDimAlign--; }

  void AddDimAngular([[maybe_unused]] const EoDxf2LineAngularDimension* dimAngular) override { countOfDimAngular--; }
  void AddDimAngular3P([[maybe_unused]] const EoDxf3PointAngularDimension* dimAngular3P) override {
    countOfDimAngular3P--;
  }

  void AddDimLinear([[maybe_unused]] const EoDxfDimLinear* dimLinear) override { countOfDimLinear--; }
  void AddDimOrdinate([[maybe_unused]] const EoDxfOrdinateDimension* dimOrdinate) override { countOfDimOrdinate--; }

  void AddDimRadial([[maybe_unused]] const EoDxfRadialDimension* dimRadial) override { countOfDimRadial--; }

  void AddDimDiametric([[maybe_unused]] const EoDxfDiametricDimension* dimDiametric) override { countOfDimDiametric--; }

  void AddEllipse(const EoDxfEllipse& ellipse) override {
    if (m_dxfWriter) {
      auto mutableEllipse = ellipse;
      mutableEllipse.m_space = m_currentExportSpace;
      m_dxfWriter->WriteEllipse(&mutableEllipse);
      return;
    }
    countOfEllipse++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddEllipse - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddEllipse - entities section\n");
    }
    ConvertEllipseEntity(ellipse, m_document);
  }

  void AddHatch(const EoDxfHatch& hatch) override {
    if (m_dxfWriter) {
      const_cast<EoDxfHatch&>(hatch).m_space = m_currentExportSpace;
      m_dxfWriter->WriteHatch(const_cast<EoDxfHatch*>(&hatch));
      return;
    }
    countOfHatch++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddHatch - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddHatch - entities section\n");
    }
    ConvertHatchEntity(hatch, m_document);
  }

  void AddImage([[maybe_unused]] const EoDxfImage* image) override { countOfImage--; }

  void AddInsert(const EoDxfInsert& blockReference) override {
    if (m_dxfWriter) {
      auto mutableInsert = blockReference;
      mutableInsert.m_space = m_currentExportSpace;
      m_dxfWriter->WriteInsert(&mutableInsert);
      return;
    }
    countOfInsert++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddInsert - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddInsert - entities section\n");
    }
    m_currentInsertPrimitive = ConvertInsertEntity(blockReference, m_document);
  }

  void AddLeader([[maybe_unused]] const EoDxfLeader* leader) override {countOfLeader--;}

  void AddLine(const EoDxfLine& line) override {
    if (m_dxfWriter) {
      auto mutableLine = line;
      mutableLine.m_space = m_currentExportSpace;
      m_dxfWriter->WriteLine(&mutableLine);
      return;
    }
    countOfLine++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddLine - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLine - entities section\n");
    }
    ConvertLineEntity(line, m_document);
  }

  void AddLWPolyline(const EoDxfLwPolyline& polyline) override {
    if (m_dxfWriter) {
      auto mutablePolyline = polyline;
      mutablePolyline.m_space = m_currentExportSpace;
      m_dxfWriter->WriteLWPolyline(&mutablePolyline);
      return;
    }
    countOfLWPolyline++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddLWPolyline - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddLWPolyline - entities section\n");
    }
    ConvertLWPolylineEntity(polyline, m_document);
  }

  void AddMLeader([[maybe_unused]] const EoDxfMLeader* mLeader) override { countOfMLeader--; }

  void AddMText(const EoDxfMText& mText) override {
    if (m_dxfWriter) {
      auto mutableMText = mText;
      mutableMText.m_space = m_currentExportSpace;
      m_dxfWriter->WriteMText(&mutableMText);
      return;
    }
    countOfMText++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addMText - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::addMText - entities section\n");
    }
    ConvertMTextEntity(mText, m_document);
  }

  void AddPoint(const EoDxfPoint& point) override {
    if (m_dxfWriter) {
      auto mutablePoint = point;
      mutablePoint.m_space = m_currentExportSpace;
      m_dxfWriter->WritePoint(&mutablePoint);
      return;
    }
    countOfPoint++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddPoint - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddPoint - entities section\n");
    }
    ConvertPointEntity(point, m_document);
  }

  void AddPolyline(const EoDxfPolyline& polyline) override {
    if (m_dxfWriter) {
      // EoDxfPolyline has deleted copy ctor (owns raw vertex pointers) — const_cast is safe here
      // because ExportToDxf constructs the object and m_space is a plain enum member.
      auto& mutablePolyline = const_cast<EoDxfPolyline&>(polyline);
      mutablePolyline.m_space = m_currentExportSpace;
      m_dxfWriter->WritePolyline(&mutablePolyline);
      return;
    }
    const auto flag = polyline.m_polylineFlag;
    if (flag & 0x40) {
      // Polyface mesh — decompose face records into individual polygons (deferred to PEG V2)
      ATLTRACE2(traceGeneral, 1, L"EoDxfInterface::AddPolyline - polyface mesh skipped (flag 0x%04X)\n", flag);
      countOfPolyline--;
    } else if (flag & 0x10) {
      // Polygon mesh — not mappable to current primitives
      ATLTRACE2(traceGeneral, 1, L"EoDxfInterface::AddPolyline - polygon mesh skipped (flag 0x%04X)\n", flag);
      countOfPolyline--;
    } else if (flag & 0x08) {
      // 3D polyline — straightforward vertex chain
      countOfPolyline++;
      if (m_inBlockDefinition) {
        ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddPolyline3D - block <%s>\n", m_blockName.c_str());
      } else {
        ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddPolyline3D - entities section\n");
      }
      ConvertPolyline3DEntity(polyline, m_document);
    } else {
      // 2D polyline — elevation + optional bulge/width
      countOfPolyline++;
      if (m_inBlockDefinition) {
        ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddPolyline2D - block <%s>\n", m_blockName.c_str());
      } else {
        ATLTRACE2(traceGeneral, 3, L"EoDxfInterface::AddPolyline2D - entities section\n");
      }
      ConvertPolyline2DEntity(polyline, m_document);
    }
  }
  void AddRay([[maybe_unused]] const EoDxfRay& ray) override { countOfRay--; }

  void AddSolid([[maybe_unused]] const EoDxfSolid& solid) override { countOfSolid--; }

  void AddSpline(const EoDxfSpline& spline) override {
    if (m_dxfWriter) {
      const_cast<EoDxfSpline&>(spline).m_space = m_currentExportSpace;
      m_dxfWriter->WriteSpline(const_cast<EoDxfSpline*>(&spline));
      return;
    }
    countOfSpline++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddSpline - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddSpline - entities section\n");
    }
    ConvertSplineEntity(spline, m_document);
  }

  void AddText(const EoDxfText& text) override {
    if (m_dxfWriter) {
      auto mutableText = text;
      mutableText.m_space = m_currentExportSpace;
      m_dxfWriter->WriteText(&mutableText);
      return;
    }
    countOfText++;
    if (m_inBlockDefinition) {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddText - block <%s>\n", m_blockName.c_str());
    } else {
      ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddText - entities section\n");
    }
    ConvertTextEntity(text, m_document);
  }

  void AddTrace([[maybe_unused]] const EoDxfTrace& trace) override { countOfTrace--; }

  void AddViewport(const EoDxfViewport& viewport) override {
    if (m_dxfWriter) {
      auto mutableViewport = viewport;
      mutableViewport.m_space = m_currentExportSpace;
      m_dxfWriter->WriteViewport(&mutableViewport);
      return;
    }
    countOfViewport++;
    ConvertViewportEntity(viewport, m_document);
  }

  void AddXline([[maybe_unused]] const EoDxfXline& Xline) override { countOfXline--; }

  void AddUnsupportedObject(const EoDxfUnsupportedObject& objectData) override {
    if (m_dxfWriter) {
      m_dxfWriter->WriteUnsupportedObject(objectData);
      return;
    }
    m_document->AddUnsupportedObject(objectData);
  }

  // Others
  void AddComment(std::wstring_view comment) override {
    ATLTRACE2(traceGeneral, 2, L"EoDxfInterface::AddComment(%.*s)\n", static_cast<int>(comment.size()), comment.data());
  }
  void LinkImage([[maybe_unused]] const EoDxfImageDefinition* imageDefinition) override {}
  void AddKnot([[maybe_unused]] const EoDxfGraphic& knot) override { countOfKnot--; }

  // Writing methods
  void WriteAppId() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }
    for (const auto& entry : m_document->AppIdTable()) {
      EoDxfAppId dxfAppId;
      dxfAppId.m_tableName = entry.m_name;
      dxfAppId.m_flagValues = entry.m_flagValues;
      m_dxfWriter->WriteAppId(&dxfAppId);
    }
  };
  void WriteBlockRecords() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }
    auto position = m_document->GetFirstBlockPosition();
    while (position != nullptr) {
      CString name;
      EoDbBlock* block{};
      m_document->GetNextBlock(position, name, block);
      if (block == nullptr) { continue; }
      m_dxfWriter->WriteBlockRecord(std::wstring(name), block->OwnerHandle());
    }
  };
  void WriteBlocks() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }
    auto position = m_document->GetFirstBlockPosition();
    while (position != nullptr) {
      CString name;
      EoDbBlock* block{};
      m_document->GetNextBlock(position, name, block);
      if (block == nullptr) { continue; }

      EoDxfBlock dxfBlock;
      dxfBlock.m_blockName = std::wstring(name);
      dxfBlock.m_blockTypeFlags = static_cast<std::int16_t>(block->BlockTypeFlags());
      dxfBlock.m_handle = block->Handle();
      auto basePoint = block->BasePoint();
      dxfBlock.m_basePoint = {basePoint.x, basePoint.y, basePoint.z};
      m_dxfWriter->WriteBlock(&dxfBlock);

      // Write block content entities (primitives stored directly in EoDbBlock)
      auto primitivePosition = block->GetHeadPosition();
      while (primitivePosition != nullptr) {
        auto* primitive = block->GetNext(primitivePosition);
        if (primitive != nullptr) { primitive->ExportToDxf(this); }
      }
    }
  };
  void WriteClasses() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }

    for (const auto& entry : m_document->ClassTable()) {
      EoDxfClass dxfClass;
      dxfClass.m_classDxfRecordName = entry.m_classDxfRecordName;
      dxfClass.m_cppClassName = entry.m_cppClassName;
      dxfClass.m_applicationName = entry.m_applicationName;
      dxfClass.m_proxyCapabilitiesFlag = entry.m_proxyCapabilitiesFlag;
      dxfClass.m_instanceCount = entry.m_instanceCount;
      dxfClass.m_wasAProxyFlag = entry.m_wasAProxyFlag;
      dxfClass.m_isAnEntityFlag = entry.m_isAnEntityFlag;
      m_dxfWriter->WriteClass(&dxfClass);
    }
  };
  void WriteDimstyles() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }

    for (const auto& entry : m_document->DimStyleTable()) {
      EoDxfDimensionStyle dxfDimStyle;
      dxfDimStyle.m_tableName = entry.m_name;
      dxfDimStyle.m_handle = entry.m_handle;
      dxfDimStyle.m_flagValues = entry.m_flagValues;

      dxfDimStyle.dimpost = entry.dimpost;
      dxfDimStyle.dimapost = entry.dimapost;
      dxfDimStyle.dimblk = entry.dimblk;
      dxfDimStyle.dimblk1 = entry.dimblk1;
      dxfDimStyle.dimblk2 = entry.dimblk2;

      dxfDimStyle.dimscale = entry.dimscale;
      dxfDimStyle.dimasz = entry.dimasz;
      dxfDimStyle.dimexo = entry.dimexo;
      dxfDimStyle.dimdli = entry.dimdli;
      dxfDimStyle.dimexe = entry.dimexe;
      dxfDimStyle.dimrnd = entry.dimrnd;
      dxfDimStyle.dimdle = entry.dimdle;
      dxfDimStyle.dimtp = entry.dimtp;
      dxfDimStyle.dimtm = entry.dimtm;
      dxfDimStyle.dimfxl = entry.dimfxl;
      dxfDimStyle.dimtxt = entry.dimtxt;
      dxfDimStyle.dimcen = entry.dimcen;
      dxfDimStyle.dimtsz = entry.dimtsz;
      dxfDimStyle.dimaltf = entry.dimaltf;
      dxfDimStyle.dimlfac = entry.dimlfac;
      dxfDimStyle.dimtvp = entry.dimtvp;
      dxfDimStyle.dimtfac = entry.dimtfac;
      dxfDimStyle.dimgap = entry.dimgap;
      dxfDimStyle.dimaltrnd = entry.dimaltrnd;

      dxfDimStyle.dimtol = entry.dimtol;
      dxfDimStyle.dimlim = entry.dimlim;
      dxfDimStyle.dimtih = entry.dimtih;
      dxfDimStyle.dimtoh = entry.dimtoh;
      dxfDimStyle.dimse1 = entry.dimse1;
      dxfDimStyle.dimse2 = entry.dimse2;
      dxfDimStyle.dimtad = entry.dimtad;
      dxfDimStyle.dimzin = entry.dimzin;
      dxfDimStyle.dimazin = entry.dimazin;
      dxfDimStyle.dimalt = entry.dimalt;
      dxfDimStyle.dimaltd = entry.dimaltd;
      dxfDimStyle.dimtofl = entry.dimtofl;
      dxfDimStyle.dimsah = entry.dimsah;
      dxfDimStyle.dimtix = entry.dimtix;
      dxfDimStyle.dimsoxd = entry.dimsoxd;
      dxfDimStyle.dimclrd = entry.dimclrd;
      dxfDimStyle.dimclre = entry.dimclre;
      dxfDimStyle.dimclrt = entry.dimclrt;
      dxfDimStyle.dimadec = entry.dimadec;
      dxfDimStyle.dimunit = entry.dimunit;
      dxfDimStyle.dimdec = entry.dimdec;
      dxfDimStyle.dimtdec = entry.dimtdec;
      dxfDimStyle.dimaltu = entry.dimaltu;
      dxfDimStyle.dimalttd = entry.dimalttd;
      dxfDimStyle.dimaunit = entry.dimaunit;
      dxfDimStyle.dimfrac = entry.dimfrac;
      dxfDimStyle.dimlunit = entry.dimlunit;
      dxfDimStyle.dimdsep = entry.dimdsep;
      dxfDimStyle.dimtmove = entry.dimtmove;
      dxfDimStyle.dimjust = entry.dimjust;
      dxfDimStyle.dimsd1 = entry.dimsd1;
      dxfDimStyle.dimsd2 = entry.dimsd2;
      dxfDimStyle.dimtolj = entry.dimtolj;
      dxfDimStyle.dimtzin = entry.dimtzin;
      dxfDimStyle.dimaltz = entry.dimaltz;
      dxfDimStyle.dimaltttz = entry.dimaltttz;
      dxfDimStyle.dimfit = entry.dimfit;
      dxfDimStyle.dimupt = entry.dimupt;
      dxfDimStyle.dimatfit = entry.dimatfit;

      dxfDimStyle.dimfxlon = entry.dimfxlon;

      dxfDimStyle.dimtxsty = entry.dimtxsty;
      dxfDimStyle.dimldrblk = entry.dimldrblk;

      dxfDimStyle.dimlwd = entry.dimlwd;
      dxfDimStyle.dimlwe = entry.dimlwe;

      m_dxfWriter->WriteDimStyle(&dxfDimStyle);
    }
  };
  void WriteEntities() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }

    // Export model-space entities
    m_currentExportSpace = EoDxf::Space::ModelSpace;
    auto& modelLayers = m_document->SpaceLayers(EoDxf::Space::ModelSpace);
    for (INT_PTR i = 0; i < modelLayers.GetSize(); i++) {
      auto* layer = modelLayers.GetAt(i);
      if (layer == nullptr) { continue; }
      auto position = layer->GetHeadPosition();
      while (position != nullptr) {
        auto* group = layer->GetNext(position);
        if (group == nullptr) { continue; }
        auto primitivePosition = group->GetHeadPosition();
        while (primitivePosition != nullptr) {
          auto* primitive = group->GetNext(primitivePosition);
          if (primitive != nullptr) {
            if (primitive->LayerName().empty()) { primitive->SetLayerName(std::wstring(layer->Name())); }
            primitive->ExportToDxf(this);
          }
        }
      }
    }

    // Export paper-space entities
    m_currentExportSpace = EoDxf::Space::PaperSpace;
    auto& paperLayers = m_document->SpaceLayers(EoDxf::Space::PaperSpace);
    for (INT_PTR i = 0; i < paperLayers.GetSize(); i++) {
      auto* layer = paperLayers.GetAt(i);
      if (layer == nullptr) { continue; }
      auto position = layer->GetHeadPosition();
      while (position != nullptr) {
        auto* group = layer->GetNext(position);
        if (group == nullptr) { continue; }
        auto primitivePosition = group->GetHeadPosition();
        while (primitivePosition != nullptr) {
          auto* primitive = group->GetNext(primitivePosition);
          if (primitive != nullptr) {
            if (primitive->LayerName().empty()) { primitive->SetLayerName(std::wstring(layer->Name())); }
            primitive->ExportToDxf(this);
          }
        }
      }
    }

    m_currentExportSpace = EoDxf::Space::ModelSpace;
  };
  [[nodiscard]] std::uint64_t GetHandleSeed() const override {
    return m_document != nullptr ? m_document->HandleManager().NextHandleValue() : 0;
  }

  void WriteHeader(EoDxfHeader& header) override {
    if (m_document == nullptr) { return; }
    const auto& headerSection = m_document->HeaderSection();
    for (const auto& [name, value] : headerSection.GetVariables()) {
      const int groupCode = headerSection.GetGroupCode(name);
      std::visit(
          [&header, &name, groupCode](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, double>) {
              header.AddDouble(name, val, groupCode != 0 ? groupCode : 40);
            } else if constexpr (std::is_same_v<T, int>) {
              header.AddInt16(name, static_cast<std::int16_t>(val), groupCode != 0 ? groupCode : 70);
            } else if constexpr (std::is_same_v<T, std::wstring>) {
              header.AddWideString(name, val, groupCode != 0 ? groupCode : 1);
            } else if constexpr (std::is_same_v<T, EoGePoint3d>) {
              header.AddGeometryBase(name, {val.x, val.y, val.z}, groupCode != 0 ? groupCode : 10);
            } else if constexpr (std::is_same_v<T, EoGeVector3d>) {
              header.AddGeometryBase(name, {val.x, val.y, val.z}, groupCode != 0 ? groupCode : 10);
            } else if constexpr (std::is_same_v<T, std::uint64_t>) {
              header.AddHandle(name, val, groupCode != 0 ? groupCode : 5);
            } else if constexpr (std::is_same_v<T, bool>) {
              header.AddInt16(name, val ? 1 : 0, groupCode != 0 ? groupCode : 290);
            }
          },
                   value);
            }

            // Update $HANDSEED to reflect the document's current handle state.
            // This ensures the exported DXF header advertises a seed above all entity handles.
            header.AddHandle(L"$HANDSEED", m_document->HandleManager().NextHandleValue(), 5);
          };
  void WriteObjects() override {};
  [[nodiscard]] bool HasUnsupportedObjects() const override {
    return m_document != nullptr && !m_document->UnsupportedObjects().empty();
  }
  void WriteUnsupportedObjects() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }
    for (const auto& object : m_document->UnsupportedObjects()) {
      m_dxfWriter->WriteUnsupportedObject(object);
    }
  };
  void WriteLayers() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }

    auto writeLayers = [this](CLayers& layers) {
      for (INT_PTR i = 0; i < layers.GetSize(); i++) {
        auto* layer = layers.GetAt(i);
        if (layer == nullptr) { continue; }

        EoDxfLayer dxfLayer;
        dxfLayer.m_tableName = std::wstring(layer->Name());
        dxfLayer.m_handle = layer->Handle();
        dxfLayer.m_colorNumber = layer->IsOff() ? static_cast<std::int16_t>(-std::abs(layer->ColorIndex()))
                                                : static_cast<std::int16_t>(std::abs(layer->ColorIndex()));
        dxfLayer.m_linetypeName = layer->LineType() != nullptr ? std::wstring(layer->LineTypeName()) : L"Continuous";
        dxfLayer.m_plottingFlag = true;
        m_dxfWriter->WriteLayer(&dxfLayer);
      }
    };

    writeLayers(m_document->SpaceLayers(EoDxf::Space::ModelSpace));
    writeLayers(m_document->SpaceLayers(EoDxf::Space::PaperSpace));
  };
  void WriteLTypes() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }

    auto* lineTypeTable = m_document->LineTypeTable();
    auto position = lineTypeTable->GetStartPosition();
    while (position != nullptr) {
      CString name;
      EoDbLineType* lineType{};
      lineTypeTable->GetNextAssoc(position, name, lineType);
      if (lineType == nullptr) { continue; }

      EoDxfLinetype dxfLinetype;
      dxfLinetype.m_tableName = std::wstring(lineType->Name());
      dxfLinetype.m_handle = lineType->Handle();
      dxfLinetype.desc = std::wstring(lineType->Description());
      dxfLinetype.m_numberOfLinetypeElements = static_cast<std::int16_t>(lineType->GetNumberOfDashes());
      dxfLinetype.length = lineType->GetPatternLength();

      const auto& dashElements = lineType->DashElements();
      dxfLinetype.path.assign(dashElements.begin(), dashElements.end());

      m_dxfWriter->WriteLinetype(&dxfLinetype);
    }
  };
  void WriteTextstyles() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }

    for (const auto& entry : m_document->TextStyleTable()) {
      EoDxfTextStyle dxfTextStyle;
      dxfTextStyle.m_tableName = entry.m_name;
      dxfTextStyle.m_handle = entry.m_handle;
      dxfTextStyle.height = entry.m_height;
      dxfTextStyle.width = entry.m_widthFactor;
      dxfTextStyle.oblique = Eo::RadianToDegree(entry.m_obliqueAngle);
      dxfTextStyle.m_textGenerationFlag = entry.m_textGenerationFlags;
      dxfTextStyle.lastHeight = entry.m_lastHeight;
      dxfTextStyle.font = entry.m_font;
      dxfTextStyle.bigFont = entry.m_bigFont;
      dxfTextStyle.fontFamily = entry.m_fontFamily;
      dxfTextStyle.m_flagValues = entry.m_flagValues;
      m_dxfWriter->WriteTextstyle(&dxfTextStyle);
    }
  };
  void WriteVports() override {
    if (m_dxfWriter == nullptr || m_document == nullptr) { return; }

    for (const auto& entry : m_document->VPortTable()) {
      EoDxfVPort dxfVPort;
      dxfVPort.m_tableName = entry.m_name;
      dxfVPort.m_lowerLeftCorner = {entry.m_lowerLeftCorner.x, entry.m_lowerLeftCorner.y};
      dxfVPort.m_upperRightCorner = {entry.m_upperRightCorner.x, entry.m_upperRightCorner.y};
      dxfVPort.m_viewCenter = {entry.m_viewCenter.x, entry.m_viewCenter.y};
      dxfVPort.m_snapBasePoint = {entry.m_snapBasePoint.x, entry.m_snapBasePoint.y};
      dxfVPort.m_snapSpacing = {entry.m_snapSpacing.x, entry.m_snapSpacing.y};
      dxfVPort.m_gridSpacing = {entry.m_gridSpacing.x, entry.m_gridSpacing.y};
      dxfVPort.m_viewDirection = {entry.m_viewDirection.x, entry.m_viewDirection.y, entry.m_viewDirection.z};
      dxfVPort.m_viewTargetPoint = {entry.m_viewTargetPoint.x, entry.m_viewTargetPoint.y, entry.m_viewTargetPoint.z};
      dxfVPort.m_viewHeight = entry.m_viewHeight;
      dxfVPort.m_viewAspectRatio = entry.m_viewAspectRatio;
      dxfVPort.m_lensLength = entry.m_lensLength;
      dxfVPort.m_frontClipPlane = entry.m_frontClipPlane;
      dxfVPort.m_backClipPlane = entry.m_backClipPlane;
      dxfVPort.m_snapRotationAngle = entry.m_snapRotationAngle;
      dxfVPort.m_viewTwistAngle = entry.m_viewTwistAngle;
      dxfVPort.m_viewMode = entry.m_viewMode;
      dxfVPort.m_circleZoomPercent = entry.m_circleZoomPercent;
      dxfVPort.m_fastZoom = entry.m_fastZoom;
      dxfVPort.m_ucsIcon = entry.m_ucsIcon;
      dxfVPort.m_snapOn = entry.m_snapOn;
      dxfVPort.m_gridOn = entry.m_gridOn;
      dxfVPort.m_snapStyle = entry.m_snapStyle;
      dxfVPort.m_snapIsopair = entry.m_snapIsopair;
      dxfVPort.m_gridBehavior = entry.m_gridBehavior;
      m_dxfWriter->WriteVport(&dxfVPort);
    }
  };

  void SetHeaderSectionVariable(
      const EoDxfHeader* header, std::wstring_view keyToFind, EoDbHeaderSection& headerSection);

  void ConvertHeaderSection(const EoDxfHeader* header, AeSysDoc* document);
  void ConvertClassesSection(const EoDxfClass& class_, AeSysDoc* document);

  void ConvertAppIdTable(const EoDxfAppId& appId, AeSysDoc* document);
  void ConvertDimStyle(const EoDxfDimensionStyle& dimensionStyle, AeSysDoc* document);

  /** @brief Converts a EoDxfLayer object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfLayer object, which represents layer information from a DXF/DWG file, and converts it into
   * the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param layer The EoDxfLayer object containing layer data to be converted.
   * @param document A pointer to the AeSysDoc where the converted layer will be stored.
   */
  void ConvertLayerTable(const EoDxfLayer& layer, AeSysDoc* document);

  /** @brief Converts a EoDxfLinetype object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfLinetype object, which represents line type information from a DXF/DWG file, and converts
   * it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param lineType The EoDxfLinetype object containing line type data to be converted.
   * @param document A pointer to the AeSysDoc where the converted line type will be stored.
   * @note unimplemented complex linetype elements (group code 74)
   *  Complex linetype element type (one per element). Default is 0 (no embedded shape/text). The following codes are
   * bit values: 0x01 = If set, code 50 specifies an absolute rotation; else code 50 specifies a relative rotation 0x02
   * = Embedded element is a text string 0x04 = Embedded element is a shape
   *
   *   group code 75 - Shape number (one per element) if code 74 specifies an embedded shape. If code 74 specifies an
   * embedded text string, this value is set to 0 else if code 74 is set to 0, code 75 is omitted. group code 340 -
   * Pointer to STYLE object (one per element if code 74 > 0) group code 46 - S = Scale value (optional); multiple
   * entries can exist group code 50 - R = (relative) or A = (absolute) rotation value in radians of embedded shape or
   * text; one per element if code 74 specifies an embedded shape or text string group code 44 - X = X offset value
   * (optional); multiple entries can exist group code 45 - Y = Y offset value (optional); multiple entries can exist
   *   group code 9 - Text string (one per element if code 74 = 2)
   */
  void ConvertLinetypesTable(const EoDxfLinetype& lineType, AeSysDoc* document);

  /** @brief Converts a EoDxfTextstyle object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfTextstyle object, which represents text style information from a DXF/DWG file, and
   * converts it into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param textStyle The EoDxfTextStyle object containing text style data to be converted.
   * @param document A pointer to the AeSysDoc where the converted text style will be stored.
   * @note A style table item is also used to record shape file LOAD command requests. In this case the first bit (0x01)
   * is set in the 70 group flags and only the 3 group (shape file name) is meaningful (all the other groups are output,
   * however).
   */
  void ConvertTextStyleTable(const EoDxfTextStyle& textStyle, AeSysDoc* document);

  /** @brief Converts a EoDxfVPort object to the corresponding AeSys document representation.
   *
   * This method takes a EoDxfVPort object, which represents viewport information from a DXF/DWG file, and converts it
   * into the appropriate format for storage in the provided AeSysDoc document.
   *
   * @param viewport The EoDxfVPort object containing viewport data to be converted.
   * @param document A pointer to the AeSysDoc where the converted viewport will be stored.
   */
  void ConvertVPortTable(const EoDxfVPort& viewport, AeSysDoc* document);

  EoDbBlock* ConvertBlock(const EoDxfBlock& block, AeSysDoc* document);
  void ConvertBlockSet(const int handle, AeSysDoc* document);
  void ConvertBlockEnd(AeSysDoc* document);

  void AddToDocument(EoDbPrimitive* primitive, AeSysDoc* document, EoDxf::Space space);

  void Convert3dFaceEntity(const EoDxf3dFace& _3dFace, AeSysDoc* document);
  void ConvertAcadProxyEntity(const EoDxfAcadProxyEntity& proxyEntity, AeSysDoc* document);
  void ConvertArcEntity(const EoDxfArc& arc, AeSysDoc* document);
  void ConvertAttDefEntity(const EoDxfAttDef& attdef, [[maybe_unused]] AeSysDoc* document);
  EoDbText* ConvertAttribEntity(const EoDxfAttrib& attrib, AeSysDoc* document);
  void ConvertCircleEntity(const EoDxfCircle& circle, AeSysDoc* document);
  void ConvertEllipseEntity(const EoDxfEllipse& ellipse, AeSysDoc* document);
  void ConvertHatchEntity(const EoDxfHatch& hatch, [[maybe_unused]] AeSysDoc* document);
  EoDbBlockReference* ConvertInsertEntity(const EoDxfInsert& insert, AeSysDoc* document);
  void ConvertLineEntity(const EoDxfLine& line, AeSysDoc* document);
  void ConvertLWPolylineEntity(const EoDxfLwPolyline& lwPolyline, AeSysDoc* document);
  void ConvertMTextEntity(const EoDxfMText& mText, [[maybe_unused]] AeSysDoc* document);
  void ConvertPointEntity(const EoDxfPoint& point, AeSysDoc* document);
  void ConvertPolyline2DEntity(const EoDxfPolyline& polyline, AeSysDoc* document);
  void ConvertPolyline3DEntity(const EoDxfPolyline& polyline, AeSysDoc* document);
  void ConvertSplineEntity(const EoDxfSpline& spline, AeSysDoc* document);
  void ConvertTextEntity(const EoDxfText& text, [[maybe_unused]] AeSysDoc* document);
  void ConvertViewportEntity(const EoDxfViewport& viewport, AeSysDoc* document);

 private:
  AeSysDoc* m_document{};
  EoDxfWrite* m_dxfWriter{};
  std::wstring m_blockName{};
  bool m_inBlockDefinition{};
  EoDbBlock* m_currentOpenBlockDefinition{};
  EoDxf::Space m_currentExportSpace{EoDxf::Space::ModelSpace};

  /// Non-owning pointer to the most recently created EoDbBlockReference from AddInsert.
  /// Set during DXF import so that subsequent AddAttrib calls can link ATTRIB handles
  /// to their parent INSERT. Cleared when the next non-ATTRIB entity is processed.
  EoDbBlockReference* m_currentInsertPrimitive{};

 public:
  std::int16_t countOf3dFace{};
  std::int16_t countOfAcadProxyEntity{};
  std::int16_t countOfArc{};
  std::int16_t countOfAttDef{};
  std::int16_t countOfAttrib{};
  std::int16_t countOfCircle{};
  std::int16_t countOfDimAlign{};
  std::int16_t countOfDimAngular{};
  std::int16_t countOfDimAngular3P{};
  std::int16_t countOfDimDiametric{};
  std::int16_t countOfDimLinear{};
  std::int16_t countOfDimOrdinate{};
  std::int16_t countOfDimRadial{};
  std::int16_t countOfEllipse{};
  std::int16_t countOfHatch{};
  std::int16_t countOfImage{};
  std::int16_t countOfInsert{};
  std::int16_t countOfKnot{};
  std::int16_t countOfLeader{};
  std::int16_t countOfLine{};
  std::int16_t countOfLWPolyline{};
  std::int16_t countOfMLeader{};
  std::int16_t countOfMText{};
  std::int16_t countOfPoint{};
  std::int16_t countOfPolyline{};
  std::int16_t countOfRay{};
  std::int16_t countOfSolid{};
  std::int16_t countOfSpline{};
  std::int16_t countOfText{};
  std::int16_t countOfTrace{};
  std::int16_t countOfViewport{};
  std::int16_t countOfXline{};
};