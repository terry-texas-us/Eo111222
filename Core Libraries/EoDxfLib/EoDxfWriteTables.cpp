#include <algorithm>
#include <cctype>
#include <cwctype>
#include <string>
#include <vector>

#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfWriter.h"

namespace {

[[nodiscard]] std::wstring UppercaseTableToken(std::wstring value) {
  std::transform(value.begin(), value.end(), value.begin(), [](const wchar_t character) {
    return static_cast<wchar_t>(std::towupper(character));
  });
  return value;
}

}  // namespace

bool EoDxfWrite::WriteTables() {
  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"VPORT");

  WriteCodeString(5, L"8");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  /*** VPORT ***/
  m_standardDimensionStyle = false;
  m_interface->WriteVports();
  if (!m_standardDimensionStyle) {
    EoDxfVPort activeViewport;
    activeViewport.m_tableName = L"*ACTIVE";
    WriteVport(&activeViewport);
  }
  WriteCodeString(0, L"ENDTAB");
  /*** LTYPE ***/
  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"LTYPE");

  WriteCodeString(5, L"5");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 4);  // end table def
  // Mandatory linetypes
  WriteCodeString(0, L"LTYPE");

  WriteCodeString(5, L"14");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"5"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbLinetypeTableRecord");
  WriteCodeString(2, L"ByBlock");

  WriteCodeInt16(70, 0);
  WriteCodeString(3, L"");
  WriteCodeInt16(72, 65);
  WriteCodeInt16(73, 0);
  WriteCodeDouble(40, 0.0);

  WriteCodeString(0, L"LTYPE");

  WriteCodeString(5, L"15");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"5"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbLinetypeTableRecord");
  WriteCodeString(2, L"ByLayer");

  WriteCodeInt16(70, 0);
  WriteCodeString(3, L"");
  WriteCodeInt16(72, 65);
  WriteCodeInt16(73, 0);
  WriteCodeDouble(40, 0.0);

  WriteCodeString(0, L"LTYPE");

  WriteCodeString(5, L"16");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"5"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbLinetypeTableRecord");
  WriteCodeString(2, L"CONTINUOUS");

  WriteCodeInt16(70, 0);
  WriteCodeString(3, L"Solid line");
  WriteCodeInt16(72, 65);
  WriteCodeInt16(73, 0);
  WriteCodeDouble(40, 0.0);
  // Application linetypes
  m_interface->WriteLTypes();
  WriteCodeString(0, L"ENDTAB");
  /*** LAYER ***/
  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"LAYER");

  WriteCodeString(5, L"2");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  wlayer0 = false;
  m_interface->WriteLayers();
  if (!wlayer0) {
    EoDxfLayer defaultLayer;
    defaultLayer.m_tableName = L"0";
    WriteLayer(&defaultLayer);
  }
  WriteCodeString(0, L"ENDTAB");
  /*** STYLE ***/
  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"STYLE");

  WriteCodeString(5, L"3");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 3);  // end table def
  m_standardDimensionStyle = false;
  m_interface->WriteTextstyles();
  if (!m_standardDimensionStyle) {
    EoDxfTextStyle textStyle;
    textStyle.m_tableName = L"Standard";
    WriteTextstyle(&textStyle);
  }
  WriteCodeString(0, L"ENDTAB");

  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"VIEW");

  WriteCodeString(5, L"6");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 0);  // end table def
  WriteCodeString(0, L"ENDTAB");

  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"UCS");

  WriteCodeString(5, L"7");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 0);  // end table def
  WriteCodeString(0, L"ENDTAB");

  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"APPID");

  WriteCodeString(5, L"9");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  WriteCodeString(0, L"APPID");

  WriteCodeString(5, L"12");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"9"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbRegAppTableRecord");

  WriteCodeString(2, L"ACAD");
  WriteCodeInt16(70, 0);
  m_interface->WriteAppId();
  WriteCodeString(0, L"ENDTAB");

  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"DIMSTYLE");

  WriteCodeString(5, L"A");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeString(100, L"AcDbDimStyleTable");
    WriteCodeInt16(71, 1);  // end table def
  }
  m_standardDimensionStyle = false;
  m_interface->WriteDimstyles();
  if (!m_standardDimensionStyle) {
    EoDxfDimensionStyle dimensionStyle;
    dimensionStyle.m_tableName = L"Standard";
    WriteDimStyle(&dimensionStyle);
  }
  WriteCodeString(0, L"ENDTAB");

  WriteCodeString(0, L"TABLE");
  WriteCodeString(2, L"BLOCK_RECORD");
  WriteCodeString(5, L"1");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"0"); }
  WriteCodeString(100, L"AcDbSymbolTable");
  WriteCodeInt16(70, 2);  // end table def
  WriteCodeString(0, L"BLOCK_RECORD");
  WriteCodeString(5, L"1F");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"1"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbBlockTableRecord");
  WriteCodeString(2, L"*Model_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    WriteCodeInt16(70, 0);
    WriteCodeInt16(280, 1);
    WriteCodeInt16(281, 0);
  }
  WriteCodeString(0, L"BLOCK_RECORD");
  WriteCodeString(5, L"1E");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"1"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbBlockTableRecord");
  WriteCodeString(2, L"*Paper_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    WriteCodeInt16(70, 0);
    WriteCodeInt16(280, 1);
    WriteCodeInt16(281, 0);
  }

  /* always call WriteBlockRecords to interface for prepare unnamed blocks */
  m_interface->WriteBlockRecords();
  WriteCodeString(0, L"ENDTAB");
  return m_writeOk;
}

bool EoDxfWrite::WriteAppId(EoDxfAppId* appId) {
  const auto upperName = UppercaseTableToken(appId->m_tableName);
  // do not write mandatory ACAD appId, handled by library
  if (upperName == L"ACAD") { return true; }
  WriteCodeString(0, L"APPID");

  if (appId->m_handle != EoDxf::NoHandle) {
    if (appId->m_handle >= m_entityCount) { m_entityCount = appId->m_handle; }
    WriteCodeString(5, ToHexString(appId->m_handle));
  } else {
    WriteCodeString(5, ToHexString(++m_entityCount));
  }
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, L"9"); }
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbRegAppTableRecord");
  WriteCodeWideString(2, appId->m_tableName);

  WriteCodeInt16(70, appId->m_flagValues);
  return m_writeOk;
}

bool EoDxfWrite::WriteDimStyle(EoDxfDimensionStyle* dimensionStyle) {
  WriteCodeString(0, L"DIMSTYLE");
  if (!m_standardDimensionStyle) {
    if (UppercaseTableToken(dimensionStyle->m_tableName) == L"STANDARD") { m_standardDimensionStyle = true; }
  }
  if (dimensionStyle->m_handle != EoDxf::NoHandle) {
    if (dimensionStyle->m_handle >= m_entityCount) { m_entityCount = dimensionStyle->m_handle; }
    WriteCodeString(105, ToHexString(dimensionStyle->m_handle));
  } else {
    WriteCodeString(105, ToHexString(++m_entityCount));
  }

  WriteCodeString(330, L"A");

  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbDimStyleTableRecord");
  WriteCodeWideString(2, dimensionStyle->m_tableName);

  WriteCodeInt16(70, dimensionStyle->m_flagValues);
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimpost.empty())) {
    WriteCodeWideString(3, dimensionStyle->dimpost);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimapost.empty())) {
    WriteCodeWideString(4, dimensionStyle->dimapost);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk.empty())) {
    WriteCodeWideString(5, dimensionStyle->dimblk);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk1.empty())) {
    WriteCodeWideString(6, dimensionStyle->dimblk1);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk2.empty())) {
    WriteCodeWideString(7, dimensionStyle->dimblk2);
  }
  WriteCodeDouble(40, dimensionStyle->dimscale);
  WriteCodeDouble(41, dimensionStyle->dimasz);
  WriteCodeDouble(42, dimensionStyle->dimexo);
  WriteCodeDouble(43, dimensionStyle->dimdli);
  WriteCodeDouble(44, dimensionStyle->dimexe);
  WriteCodeDouble(45, dimensionStyle->dimrnd);
  WriteCodeDouble(46, dimensionStyle->dimdle);
  WriteCodeDouble(47, dimensionStyle->dimtp);
  WriteCodeDouble(48, dimensionStyle->dimtm);
  if (m_version > EoDxf::Version::AC1018 || dimensionStyle->dimfxl != 0) {
    WriteCodeDouble(49, dimensionStyle->dimfxl);
  }
  WriteCodeDouble(140, dimensionStyle->dimtxt);
  WriteCodeDouble(141, dimensionStyle->dimcen);
  WriteCodeDouble(142, dimensionStyle->dimtsz);
  WriteCodeDouble(143, dimensionStyle->dimaltf);
  WriteCodeDouble(144, dimensionStyle->dimlfac);
  WriteCodeDouble(145, dimensionStyle->dimtvp);
  WriteCodeDouble(146, dimensionStyle->dimtfac);
  WriteCodeDouble(147, dimensionStyle->dimgap);
  if (m_version > EoDxf::Version::AC1014) { WriteCodeDouble(148, dimensionStyle->dimaltrnd); }
  WriteCodeInt16(71, dimensionStyle->dimtol);
  WriteCodeInt16(72, dimensionStyle->dimlim);
  WriteCodeInt16(73, dimensionStyle->dimtih);
  WriteCodeInt16(74, dimensionStyle->dimtoh);
  WriteCodeInt16(75, dimensionStyle->dimse1);
  WriteCodeInt16(76, dimensionStyle->dimse2);
  WriteCodeInt16(77, dimensionStyle->dimtad);
  WriteCodeInt16(78, dimensionStyle->dimzin);
  if (m_version > EoDxf::Version::AC1014) { WriteCodeInt16(79, dimensionStyle->dimazin); }
  WriteCodeInt16(170, dimensionStyle->dimalt);
  WriteCodeInt16(171, dimensionStyle->dimaltd);
  WriteCodeInt16(172, dimensionStyle->dimtofl);
  WriteCodeInt16(173, dimensionStyle->dimsah);
  WriteCodeInt16(174, dimensionStyle->dimtix);
  WriteCodeInt16(175, dimensionStyle->dimsoxd);
  WriteCodeInt16(176, dimensionStyle->dimclrd);
  WriteCodeInt16(177, dimensionStyle->dimclre);
  WriteCodeInt16(178, dimensionStyle->dimclrt);
  if (m_version > EoDxf::Version::AC1014) { WriteCodeInt16(179, dimensionStyle->dimadec); }

  if (m_version < EoDxf::Version::AC1015) { WriteCodeInt16(270, dimensionStyle->dimunit); }
  WriteCodeInt16(271, dimensionStyle->dimdec);
  WriteCodeInt16(272, dimensionStyle->dimtdec);
  WriteCodeInt16(273, dimensionStyle->dimaltu);
  WriteCodeInt16(274, dimensionStyle->dimalttd);
  WriteCodeInt16(275, dimensionStyle->dimaunit);

  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeInt16(276, dimensionStyle->dimfrac);
    WriteCodeInt16(277, dimensionStyle->dimlunit);
    WriteCodeInt16(278, dimensionStyle->dimdsep);
    WriteCodeInt16(279, dimensionStyle->dimtmove);
  }

  WriteCodeInt16(280, dimensionStyle->dimjust);
  WriteCodeInt16(281, dimensionStyle->dimsd1);
  WriteCodeInt16(282, dimensionStyle->dimsd2);
  WriteCodeInt16(283, dimensionStyle->dimtolj);
  WriteCodeInt16(284, dimensionStyle->dimtzin);
  WriteCodeInt16(285, dimensionStyle->dimaltz);
  WriteCodeInt16(286, dimensionStyle->dimaltttz);
  if (m_version < EoDxf::Version::AC1015) { WriteCodeInt16(287, dimensionStyle->dimfit); }
  WriteCodeInt16(288, dimensionStyle->dimupt);

  if (m_version > EoDxf::Version::AC1014) { WriteCodeInt16(289, dimensionStyle->dimatfit); }
  if (m_version > EoDxf::Version::AC1018 && dimensionStyle->dimfxlon) {
    WriteCodeBool(290, dimensionStyle->dimfxlon);
  }
  WriteCodeWideString(340, dimensionStyle->dimtxsty);
  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeWideString(341, dimensionStyle->dimldrblk);
    WriteCodeInt16(371, dimensionStyle->dimlwd);
    WriteCodeInt16(372, dimensionStyle->dimlwe);
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteLayer(EoDxfLayer* layer) {
  WriteCodeString(0, L"LAYER");
  if (!wlayer0 && layer->m_tableName == L"0") {
    wlayer0 = true;
    WriteCodeString(5, L"10");
  } else if (layer->m_handle != EoDxf::NoHandle) {
    if (layer->m_handle >= m_entityCount) { m_entityCount = layer->m_handle; }
    WriteCodeString(5, ToHexString(layer->m_handle));
  } else {
    WriteCodeString(5, ToHexString(++m_entityCount));
  }
  WriteCodeString(330, L"2");

  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbLayerTableRecord");
  WriteCodeWideString(2, layer->m_tableName);

  WriteCodeInt16(70, layer->m_flagValues);
  WriteCodeInt16(62, layer->m_colorNumber);
  if (m_version > EoDxf::Version::AC1015 && layer->color24 >= 0) { WriteCodeInt32(420, layer->color24); }
  WriteCodeWideString(6, layer->m_linetypeName);
  if (!layer->m_plottingFlag) { WriteCodeBool(290, layer->m_plottingFlag); }
  WriteCodeInt16(370, EoDxfLineWeights::LineWeightToDxfIndex(layer->m_lineweightEnumValue));
  WriteCodeString(390, L"F");

  if (!layer->m_extensionData.empty()) { WriteExtData(layer->m_extensionData); }
  //    m_writer->WriteString(347, "10012");
  return m_writeOk;
}

bool EoDxfWrite::WriteLinetype(EoDxfLinetype* lineType) {
  const auto upperName = UppercaseTableToken(lineType->m_tableName);
  // do not write linetypes handled by library
  if (upperName == L"BYLAYER" || upperName == L"BYBLOCK" || upperName == L"CONTINUOUS") { return true; }
  WriteCodeString(0, L"LTYPE");

  if (lineType->m_handle != EoDxf::NoHandle) {
    if (lineType->m_handle >= m_entityCount) { m_entityCount = lineType->m_handle; }
    WriteCodeString(5, ToHexString(lineType->m_handle));
  } else {
    WriteCodeString(5, ToHexString(++m_entityCount));
  }
  WriteCodeString(330, L"5");
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbLinetypeTableRecord");
  WriteCodeWideString(2, lineType->m_tableName);

  WriteCodeInt16(70, lineType->m_flagValues);
  WriteCodeWideString(3, lineType->desc);
  lineType->Update();
  WriteCodeInt16(72, 65);
  WriteCodeInt16(73, lineType->m_numberOfLinetypeElements);
  WriteCodeDouble(40, lineType->length);

  for (unsigned int i = 0; i < lineType->path.size(); i++) {
    WriteCodeDouble(49, lineType->path.at(i));
    WriteCodeInt16(74, 0);
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteTextstyle(EoDxfTextStyle* textStyle) {
  WriteCodeString(0, L"STYLE");
  if (!m_standardDimensionStyle) {
    if (UppercaseTableToken(textStyle->m_tableName) == L"STANDARD") { m_standardDimensionStyle = true; }
  }
  if (textStyle->m_handle != EoDxf::NoHandle) {
    if (textStyle->m_handle >= m_entityCount) { m_entityCount = textStyle->m_handle; }
    WriteCodeString(5, ToHexString(textStyle->m_handle));
  } else {
    WriteCodeString(5, ToHexString(++m_entityCount));
  }

  WriteCodeString(330, L"3");

  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbTextStyleTableRecord");
  WriteCodeWideString(2, textStyle->m_tableName);

  WriteCodeInt16(70, textStyle->m_flagValues);
  WriteCodeDouble(40, textStyle->height);
  WriteCodeDouble(41, textStyle->width);
  WriteCodeDouble(50, textStyle->oblique);
  WriteCodeInt16(71, textStyle->m_textGenerationFlag);
  WriteCodeDouble(42, textStyle->lastHeight);

  WriteCodeWideString(3, textStyle->font);
  WriteCodeWideString(4, textStyle->bigFont);
  if (textStyle->fontFamily != 0) { WriteCodeInt32(1071, textStyle->fontFamily); }
  return m_writeOk;
}

bool EoDxfWrite::WriteVport(EoDxfVPort* viewport) {
  if (!m_standardDimensionStyle) {
    viewport->m_tableName = L"*ACTIVE";
    m_standardDimensionStyle = true;
  }
  WriteCodeString(0, L"VPORT");

  if (viewport->m_handle != EoDxf::NoHandle) {
    if (viewport->m_handle >= m_entityCount) { m_entityCount = viewport->m_handle; }
    WriteCodeString(5, ToHexString(viewport->m_handle));
  } else {
    WriteCodeString(5, ToHexString(++m_entityCount));
  }
  WriteCodeString(330, L"8");
  WriteCodeString(100, L"AcDbSymbolTableRecord");
  WriteCodeString(100, L"AcDbViewportTableRecord");
  WriteCodeWideString(2, viewport->m_tableName);

  if (viewport->m_flagValues != 0) { WriteCodeInt16(70, viewport->m_flagValues); }
  if (!viewport->m_lowerLeftCorner.IsZero()) {
    WriteCodeDouble(10, viewport->m_lowerLeftCorner.x);
    WriteCodeDouble(20, viewport->m_lowerLeftCorner.y);
  }
  WriteCodeDouble(11, viewport->m_upperRightCorner.x);
  WriteCodeDouble(21, viewport->m_upperRightCorner.y);
  WriteCodeDouble(12, viewport->m_viewCenter.x);
  WriteCodeDouble(22, viewport->m_viewCenter.y);
  if (!viewport->m_snapBasePoint.IsZero()) {
    WriteCodeDouble(13, viewport->m_snapBasePoint.x);
    WriteCodeDouble(23, viewport->m_snapBasePoint.y);
  }
  WriteCodeDouble(14, viewport->m_snapSpacing.x);
  WriteCodeDouble(24, viewport->m_snapSpacing.y);
  WriteCodeDouble(15, viewport->m_gridSpacing.x);
  WriteCodeDouble(25, viewport->m_gridSpacing.y);
  WriteCodeDouble(16, viewport->m_viewDirection.x);
  WriteCodeDouble(26, viewport->m_viewDirection.y);
  WriteCodeDouble(36, viewport->m_viewDirection.z);
  if (!viewport->m_viewTargetPoint.IsZero()) {
    WriteCodeDouble(17, viewport->m_viewTargetPoint.x);
    WriteCodeDouble(27, viewport->m_viewTargetPoint.y);
    WriteCodeDouble(37, viewport->m_viewTargetPoint.z);
  }
  WriteCodeDouble(40, viewport->m_viewHeight);
  WriteCodeDouble(41, viewport->m_viewAspectRatio);
  WriteCodeDouble(42, viewport->m_lensLength);
  if (viewport->m_frontClipPlane != 0.0) { WriteCodeDouble(43, viewport->m_frontClipPlane); }
  if (viewport->m_backClipPlane != 0.0) { WriteCodeDouble(44, viewport->m_backClipPlane); }
  if (viewport->m_snapRotationAngle != 0.0) { WriteCodeDouble(50, viewport->m_snapRotationAngle); }
  if (viewport->m_viewTwistAngle != 0.0) { WriteCodeDouble(51, viewport->m_viewTwistAngle); }
  if (viewport->m_viewMode != 0) { WriteCodeInt16(71, viewport->m_viewMode); }
  WriteCodeInt16(72, viewport->m_circleZoomPercent);
  WriteCodeInt16(73, viewport->m_fastZoom);
  WriteCodeInt16(74, viewport->m_ucsIcon);
  if (viewport->m_snapOn != 0) { WriteCodeInt16(75, viewport->m_snapOn); }
  if (viewport->m_gridOn != 0) { WriteCodeInt16(76, viewport->m_gridOn); }
  if (viewport->m_snapStyle != 0) { WriteCodeInt16(77, viewport->m_snapStyle); }
  if (viewport->m_snapIsopair != 0) { WriteCodeInt16(78, viewport->m_snapIsopair); }
  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeInt16(281, 0);
    WriteCodeInt16(65, 1);
    WriteCodeDouble(110, 0.0);
    WriteCodeDouble(120, 0.0);
    WriteCodeDouble(130, 0.0);
    WriteCodeDouble(111, 1.0);
    WriteCodeDouble(121, 0.0);
    WriteCodeDouble(131, 0.0);
    WriteCodeDouble(112, 0.0);
    WriteCodeDouble(122, 1.0);
    WriteCodeDouble(132, 0.0);
    WriteCodeInt16(79, 0);
    WriteCodeDouble(146, 0.0);
    if (m_version > EoDxf::Version::AC1018) {
       WriteCodeString(348, L"10020");
      WriteCodeInt16(60, viewport->m_gridBehavior);
      WriteCodeInt16(61, 5);
      WriteCodeBool(292, true);
      WriteCodeInt16(282, 1);
      WriteCodeDouble(141, 0.0);
      WriteCodeDouble(142, 0.0);
      WriteCodeInt16(63, 250);
      WriteCodeInt32(421, 3358443);
    }
  }
  return m_writeOk;
}
