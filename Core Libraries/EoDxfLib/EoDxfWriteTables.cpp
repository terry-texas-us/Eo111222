#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfWriter.h"

bool EoDxfWrite::WriteTables() {
  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "VPORT");

  WriteCodeString(5, "8");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  /*** VPORT ***/
  m_standardDimensionStyle = false;
  m_interface->writeVports();
  if (!m_standardDimensionStyle) {
    EoDxfVPort activeViewport;
    activeViewport.m_tableName = "*ACTIVE";
    WriteVport(&activeViewport);
  }
  WriteCodeString(0, "ENDTAB");
  /*** LTYPE ***/
  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "LTYPE");

  WriteCodeString(5, "5");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 4);  // end table def
  // Mandatory linetypes
  WriteCodeString(0, "LTYPE");

  WriteCodeString(5, "14");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "5"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbLinetypeTableRecord");
  WriteCodeString(2, "ByBlock");

  WriteCodeInt16(70, 0);
  WriteCodeString(3, "");
  WriteCodeInt16(72, 65);
  WriteCodeInt16(73, 0);
  WriteCodeDouble(40, 0.0);

  WriteCodeString(0, "LTYPE");

  WriteCodeString(5, "15");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "5"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbLinetypeTableRecord");
  WriteCodeString(2, "ByLayer");

  WriteCodeInt16(70, 0);
  WriteCodeString(3, "");
  WriteCodeInt16(72, 65);
  WriteCodeInt16(73, 0);
  WriteCodeDouble(40, 0.0);

  WriteCodeString(0, "LTYPE");

  WriteCodeString(5, "16");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "5"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbLinetypeTableRecord");
  WriteCodeString(2, "Continuous");

  WriteCodeInt16(70, 0);
  WriteCodeString(3, "Solid line");
  WriteCodeInt16(72, 65);
  WriteCodeInt16(73, 0);
  WriteCodeDouble(40, 0.0);
  // Application linetypes
  m_interface->writeLTypes();
  WriteCodeString(0, "ENDTAB");
  /*** LAYER ***/
  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "LAYER");

  WriteCodeString(5, "2");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  wlayer0 = false;
  m_interface->writeLayers();
  if (!wlayer0) {
    EoDxfLayer defaultLayer;
    defaultLayer.m_tableName = "0";
    WriteLayer(&defaultLayer);
  }
  WriteCodeString(0, "ENDTAB");
  /*** STYLE ***/
  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "STYLE");

  WriteCodeString(5, "3");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 3);  // end table def
  m_standardDimensionStyle = false;
  m_interface->writeTextstyles();
  if (!m_standardDimensionStyle) {
    EoDxfTextStyle textStyle;
    textStyle.m_tableName = "Standard";
    WriteTextstyle(&textStyle);
  }
  WriteCodeString(0, "ENDTAB");

  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "VIEW");

  WriteCodeString(5, "6");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 0);  // end table def
  WriteCodeString(0, "ENDTAB");

  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "UCS");

  WriteCodeString(5, "7");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 0);  // end table def
  WriteCodeString(0, "ENDTAB");

  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "APPID");

  WriteCodeString(5, "9");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  WriteCodeString(0, "APPID");

  WriteCodeString(5, "12");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "9"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbRegAppTableRecord");

  WriteCodeString(2, "ACAD");
  WriteCodeInt16(70, 0);
  m_interface->writeAppId();
  WriteCodeString(0, "ENDTAB");

  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "DIMSTYLE");

  WriteCodeString(5, "A");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");

  WriteCodeInt16(70, 1);  // end table def
  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeString(100, "AcDbDimStyleTable");
    WriteCodeInt16(71, 1);  // end table def
  }
  m_standardDimensionStyle = false;
  m_interface->writeDimstyles();
  if (!m_standardDimensionStyle) {
    EoDxfDimensionStyle dimensionStyle;
    dimensionStyle.m_tableName = "Standard";
    WriteDimStyle(&dimensionStyle);
  }
  WriteCodeString(0, "ENDTAB");

  WriteCodeString(0, "TABLE");
  WriteCodeString(2, "BLOCK_RECORD");
  WriteCodeString(5, "1");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "0"); }
  WriteCodeString(100, "AcDbSymbolTable");
  WriteCodeInt16(70, 2);  // end table def
  WriteCodeString(0, "BLOCK_RECORD");
  WriteCodeString(5, "1F");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "1"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbBlockTableRecord");
  WriteCodeString(2, "*Model_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    WriteCodeInt16(70, 0);
    WriteCodeInt16(280, 1);
    WriteCodeInt16(281, 0);
  }
  WriteCodeString(0, "BLOCK_RECORD");
  WriteCodeString(5, "1E");
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "1"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbBlockTableRecord");
  WriteCodeString(2, "*Paper_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    WriteCodeInt16(70, 0);
    WriteCodeInt16(280, 1);
    WriteCodeInt16(281, 0);
  }

  /* always call writeBlockRecords to interface for prepare unnamed blocks */
  m_interface->writeBlockRecords();
  WriteCodeString(0, "ENDTAB");
  return m_writeOk;
}

bool EoDxfWrite::WriteAppId(EoDxfAppId* appId) {
  std::string strname = appId->m_tableName;
  std::transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write mandatory ACAD appId, handled by library
  if (strname == "ACAD") { return true; }
  WriteCodeString(0, "APPID");

  WriteCodeString(5, ToHexString(++m_entityCount));
  if (m_version > EoDxf::Version::AC1014) { WriteCodeString(330, "9"); }
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbRegAppTableRecord");
  WriteCodeUtf8String(2, appId->m_tableName);

  WriteCodeInt16(70, appId->m_flagValues);
  return m_writeOk;
}

bool EoDxfWrite::WriteDimStyle(EoDxfDimensionStyle* dimensionStyle) {
  WriteCodeString(0, "DIMSTYLE");
  if (!m_standardDimensionStyle) {
    std::string name = dimensionStyle->m_tableName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if (name == "STANDARD") { m_standardDimensionStyle = true; }
  }
  WriteCodeString(105, ToHexString(++m_entityCount));

  WriteCodeString(330, "A");

  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbDimStyleTableRecord");
  WriteCodeUtf8String(2, dimensionStyle->m_tableName);

  WriteCodeInt16(70, dimensionStyle->m_flagValues);
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimpost.empty())) {
    WriteCodeUtf8String(3, dimensionStyle->dimpost);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimapost.empty())) {
    WriteCodeUtf8String(4, dimensionStyle->dimapost);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk.empty())) {
    WriteCodeUtf8String(5, dimensionStyle->dimblk);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk1.empty())) {
    WriteCodeUtf8String(6, dimensionStyle->dimblk1);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk2.empty())) {
    WriteCodeUtf8String(7, dimensionStyle->dimblk2);
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
  WriteCodeUtf8String(340, dimensionStyle->dimtxsty);
  if (m_version > EoDxf::Version::AC1014) {
    WriteCodeUtf8String(341, dimensionStyle->dimldrblk);
    WriteCodeInt16(371, dimensionStyle->dimlwd);
    WriteCodeInt16(372, dimensionStyle->dimlwe);
  }
  return m_writeOk;
}

bool EoDxfWrite::WriteLayer(EoDxfLayer* layer) {
  WriteCodeString(0, "LAYER");
  if (!wlayer0 && layer->m_tableName == "0") {
    wlayer0 = true;
    WriteCodeString(5, "10");
  } else {
    WriteCodeString(5, ToHexString(++m_entityCount));
  }
  WriteCodeString(330, "2");

  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbLayerTableRecord");
  WriteCodeUtf8String(2, layer->m_tableName);

  WriteCodeInt16(70, layer->m_flagValues);
  WriteCodeInt16(62, layer->m_colorNumber);
  if (m_version > EoDxf::Version::AC1015 && layer->color24 >= 0) { WriteCodeInt32(420, layer->color24); }
  WriteCodeUtf8String(6, layer->m_linetypeName);
  if (!layer->m_plottingFlag) { WriteCodeBool(290, layer->m_plottingFlag); }
  WriteCodeInt16(370, EoDxfLineWidths::lineWidth2dxfInt(layer->m_lineweightEnumValue));
  WriteCodeString(390, "F");

  if (!layer->m_extensionData.empty()) { WriteExtData(layer->m_extensionData); }
  //    m_writer->WriteString(347, "10012");
  return m_writeOk;
}

bool EoDxfWrite::WriteLinetype(EoDxfLinetype* lineType) {
  std::string strname = lineType->m_tableName;

  std::transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write linetypes handled by library
  if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") { return true; }
  WriteCodeString(0, "LTYPE");

  WriteCodeString(5, ToHexString(++m_entityCount));
  WriteCodeString(330, "5");
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbLinetypeTableRecord");
  WriteCodeUtf8String(2, lineType->m_tableName);

  WriteCodeInt16(70, lineType->m_flagValues);
  WriteCodeUtf8String(3, lineType->desc);
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
  WriteCodeString(0, "STYLE");
  if (!m_standardDimensionStyle) {
    std::string name = textStyle->m_tableName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if (name == "STANDARD") { m_standardDimensionStyle = true; }
  }
  WriteCodeString(5, ToHexString(++m_entityCount));

  WriteCodeString(330, "2");

  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbTextStyleTableRecord");
  WriteCodeUtf8String(2, textStyle->m_tableName);

  WriteCodeInt16(70, textStyle->m_flagValues);
  WriteCodeDouble(40, textStyle->height);
  WriteCodeDouble(41, textStyle->width);
  WriteCodeDouble(50, textStyle->oblique);
  WriteCodeInt16(71, textStyle->m_textGenerationFlag);
  WriteCodeDouble(42, textStyle->lastHeight);

  WriteCodeUtf8String(3, textStyle->font);
  WriteCodeUtf8String(4, textStyle->bigFont);
  if (textStyle->fontFamily != 0) { WriteCodeInt32(1071, textStyle->fontFamily); }
  return m_writeOk;
}

bool EoDxfWrite::WriteVport(EoDxfVPort* viewport) {
  if (!m_standardDimensionStyle) {
    viewport->m_tableName = "*ACTIVE";
    m_standardDimensionStyle = true;
  }
  WriteCodeString(0, "VPORT");

  WriteCodeString(5, ToHexString(++m_entityCount));
  WriteCodeString(330, "2");
  WriteCodeString(100, "AcDbSymbolTableRecord");
  WriteCodeString(100, "AcDbViewportTableRecord");
  WriteCodeUtf8String(2, viewport->m_tableName);

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
      WriteCodeString(348, "10020");
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
