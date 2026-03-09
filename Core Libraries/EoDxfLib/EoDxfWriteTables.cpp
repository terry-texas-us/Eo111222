#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "EoDxfWrite.h"

#include "EoDxfBase.h"
#include "EoDxfWriter.h"

bool EoDxfWrite::WriteTables() {
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "VPORT");

  m_writer->WriteString(5, "8");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  /*** VPORT ***/
  m_standardDimensionStyle = false;
  m_interface->writeVports();
  if (!m_standardDimensionStyle) {
    EoDxfVPort activeViewport;
    activeViewport.m_tableName = "*ACTIVE";
    WriteVport(&activeViewport);
  }
  m_writer->WriteString(0, "ENDTAB");
  /*** LTYPE ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "LTYPE");

  m_writer->WriteString(5, "5");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 4);  // end table def
  // Mandatory linetypes
  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, "14");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "5"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteString(2, "ByBlock");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);

  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, "15");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "5"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteString(2, "ByLayer");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);

  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, "16");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "5"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteString(2, "Continuous");

  m_writer->WriteInt16(70, 0);
  m_writer->WriteString(3, "Solid line");
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, 0);
  m_writer->WriteDouble(40, 0.0);
  // Application linetypes
  m_interface->writeLTypes();
  m_writer->WriteString(0, "ENDTAB");
  /*** LAYER ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "LAYER");

  m_writer->WriteString(5, "2");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  wlayer0 = false;
  m_interface->writeLayers();
  if (!wlayer0) {
    EoDxfLayer defaultLayer;
    defaultLayer.m_tableName = "0";
    WriteLayer(&defaultLayer);
  }
  m_writer->WriteString(0, "ENDTAB");
  /*** STYLE ***/
  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "STYLE");

  m_writer->WriteString(5, "3");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 3);  // end table def
  m_standardDimensionStyle = false;
  m_interface->writeTextstyles();
  if (!m_standardDimensionStyle) {
    EoDxfTextStyle textStyle;
    textStyle.m_tableName = "Standard";
    WriteTextstyle(&textStyle);
  }
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "VIEW");

  m_writer->WriteString(5, "6");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 0);  // end table def
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "UCS");

  m_writer->WriteString(5, "7");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 0);  // end table def
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "APPID");

  m_writer->WriteString(5, "9");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  m_writer->WriteString(0, "APPID");

  m_writer->WriteString(5, "12");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "9"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbRegAppTableRecord");

  m_writer->WriteString(2, "ACAD");
  m_writer->WriteInt16(70, 0);
  m_interface->writeAppId();
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "DIMSTYLE");

  m_writer->WriteString(5, "A");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");

  m_writer->WriteInt16(70, 1);  // end table def
  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteString(100, "AcDbDimStyleTable");
    m_writer->WriteInt16(71, 1);  // end table def
  }
  m_standardDimensionStyle = false;
  m_interface->writeDimstyles();
  if (!m_standardDimensionStyle) {
    EoDxfDimensionStyle dimensionStyle;
    dimensionStyle.m_tableName = "Standard";
    WriteDimStyle(&dimensionStyle);
  }
  m_writer->WriteString(0, "ENDTAB");

  m_writer->WriteString(0, "TABLE");
  m_writer->WriteString(2, "BLOCK_RECORD");
  m_writer->WriteString(5, "1");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "0"); }
  m_writer->WriteString(100, "AcDbSymbolTable");
  m_writer->WriteInt16(70, 2);  // end table def
  m_writer->WriteString(0, "BLOCK_RECORD");
  m_writer->WriteString(5, "1F");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbBlockTableRecord");
  m_writer->WriteString(2, "*Model_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    m_writer->WriteInt16(70, 0);
    m_writer->WriteInt16(280, 1);
    m_writer->WriteInt16(281, 0);
  }
  m_writer->WriteString(0, "BLOCK_RECORD");
  m_writer->WriteString(5, "1E");
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "1"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbBlockTableRecord");
  m_writer->WriteString(2, "*Paper_Space");
  if (m_version > EoDxf::Version::AC1018) {
    //    m_writer->WriteInt16(340, 22);
    m_writer->WriteInt16(70, 0);
    m_writer->WriteInt16(280, 1);
    m_writer->WriteInt16(281, 0);
  }

  /* always call writeBlockRecords to interface for prepare unnamed blocks */
  m_interface->writeBlockRecords();
  m_writer->WriteString(0, "ENDTAB");
  return true;
}

bool EoDxfWrite::WriteAppId(EoDxfAppId* appId) {
  std::string strname = appId->m_tableName;
  std::transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write mandatory ACAD appId, handled by library
  if (strname == "ACAD") { return true; }
  m_writer->WriteString(0, "APPID");

  m_writer->WriteString(5, ToHexString(++m_entityCount));
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteString(330, "9"); }
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbRegAppTableRecord");
  m_writer->WriteUtf8String(2, appId->m_tableName);

  m_writer->WriteInt16(70, appId->m_flagValues);
  return true;
}

bool EoDxfWrite::WriteDimStyle(EoDxfDimensionStyle* dimensionStyle) {
  m_writer->WriteString(0, "DIMSTYLE");
  if (!m_standardDimensionStyle) {
    std::string name = dimensionStyle->m_tableName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if (name == "STANDARD") { m_standardDimensionStyle = true; }
  }
  m_writer->WriteString(105, ToHexString(++m_entityCount));

  m_writer->WriteString(330, "A");

  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbDimStyleTableRecord");
  m_writer->WriteUtf8String(2, dimensionStyle->m_tableName);

  m_writer->WriteInt16(70, dimensionStyle->m_flagValues);
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimpost.empty())) {
    m_writer->WriteUtf8String(3, dimensionStyle->dimpost);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimapost.empty())) {
    m_writer->WriteUtf8String(4, dimensionStyle->dimapost);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk.empty())) {
    m_writer->WriteUtf8String(5, dimensionStyle->dimblk);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk1.empty())) {
    m_writer->WriteUtf8String(6, dimensionStyle->dimblk1);
  }
  if (m_version == EoDxf::Version::AC1009 || !(dimensionStyle->dimblk2.empty())) {
    m_writer->WriteUtf8String(7, dimensionStyle->dimblk2);
  }
  m_writer->WriteDouble(40, dimensionStyle->dimscale);
  m_writer->WriteDouble(41, dimensionStyle->dimasz);
  m_writer->WriteDouble(42, dimensionStyle->dimexo);
  m_writer->WriteDouble(43, dimensionStyle->dimdli);
  m_writer->WriteDouble(44, dimensionStyle->dimexe);
  m_writer->WriteDouble(45, dimensionStyle->dimrnd);
  m_writer->WriteDouble(46, dimensionStyle->dimdle);
  m_writer->WriteDouble(47, dimensionStyle->dimtp);
  m_writer->WriteDouble(48, dimensionStyle->dimtm);
  if (m_version > EoDxf::Version::AC1018 || dimensionStyle->dimfxl != 0) {
    m_writer->WriteDouble(49, dimensionStyle->dimfxl);
  }
  m_writer->WriteDouble(140, dimensionStyle->dimtxt);
  m_writer->WriteDouble(141, dimensionStyle->dimcen);
  m_writer->WriteDouble(142, dimensionStyle->dimtsz);
  m_writer->WriteDouble(143, dimensionStyle->dimaltf);
  m_writer->WriteDouble(144, dimensionStyle->dimlfac);
  m_writer->WriteDouble(145, dimensionStyle->dimtvp);
  m_writer->WriteDouble(146, dimensionStyle->dimtfac);
  m_writer->WriteDouble(147, dimensionStyle->dimgap);
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteDouble(148, dimensionStyle->dimaltrnd); }
  m_writer->WriteInt16(71, dimensionStyle->dimtol);
  m_writer->WriteInt16(72, dimensionStyle->dimlim);
  m_writer->WriteInt16(73, dimensionStyle->dimtih);
  m_writer->WriteInt16(74, dimensionStyle->dimtoh);
  m_writer->WriteInt16(75, dimensionStyle->dimse1);
  m_writer->WriteInt16(76, dimensionStyle->dimse2);
  m_writer->WriteInt16(77, dimensionStyle->dimtad);
  m_writer->WriteInt16(78, dimensionStyle->dimzin);
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteInt16(79, dimensionStyle->dimazin); }
  m_writer->WriteInt16(170, dimensionStyle->dimalt);
  m_writer->WriteInt16(171, dimensionStyle->dimaltd);
  m_writer->WriteInt16(172, dimensionStyle->dimtofl);
  m_writer->WriteInt16(173, dimensionStyle->dimsah);
  m_writer->WriteInt16(174, dimensionStyle->dimtix);
  m_writer->WriteInt16(175, dimensionStyle->dimsoxd);
  m_writer->WriteInt16(176, dimensionStyle->dimclrd);
  m_writer->WriteInt16(177, dimensionStyle->dimclre);
  m_writer->WriteInt16(178, dimensionStyle->dimclrt);
  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteInt16(179, dimensionStyle->dimadec); }

  if (m_version < EoDxf::Version::AC1015) { m_writer->WriteInt16(270, dimensionStyle->dimunit); }
  m_writer->WriteInt16(271, dimensionStyle->dimdec);
  m_writer->WriteInt16(272, dimensionStyle->dimtdec);
  m_writer->WriteInt16(273, dimensionStyle->dimaltu);
  m_writer->WriteInt16(274, dimensionStyle->dimalttd);
  m_writer->WriteInt16(275, dimensionStyle->dimaunit);

  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteInt16(276, dimensionStyle->dimfrac);
    m_writer->WriteInt16(277, dimensionStyle->dimlunit);
    m_writer->WriteInt16(278, dimensionStyle->dimdsep);
    m_writer->WriteInt16(279, dimensionStyle->dimtmove);
  }

  m_writer->WriteInt16(280, dimensionStyle->dimjust);
  m_writer->WriteInt16(281, dimensionStyle->dimsd1);
  m_writer->WriteInt16(282, dimensionStyle->dimsd2);
  m_writer->WriteInt16(283, dimensionStyle->dimtolj);
  m_writer->WriteInt16(284, dimensionStyle->dimtzin);
  m_writer->WriteInt16(285, dimensionStyle->dimaltz);
  m_writer->WriteInt16(286, dimensionStyle->dimaltttz);
  if (m_version < EoDxf::Version::AC1015) { m_writer->WriteInt16(287, dimensionStyle->dimfit); }
  m_writer->WriteInt16(288, dimensionStyle->dimupt);

  if (m_version > EoDxf::Version::AC1014) { m_writer->WriteInt16(289, dimensionStyle->dimatfit); }
  if (m_version > EoDxf::Version::AC1018 && dimensionStyle->dimfxlon != 0) {
    m_writer->WriteInt16(290, dimensionStyle->dimfxlon);
  }
  m_writer->WriteUtf8String(340, dimensionStyle->dimtxsty);
  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteUtf8String(341, dimensionStyle->dimldrblk);
    m_writer->WriteInt16(371, dimensionStyle->dimlwd);
    m_writer->WriteInt16(372, dimensionStyle->dimlwe);
  }
  return true;
}

bool EoDxfWrite::WriteLayer(EoDxfLayer* layer) {
  m_writer->WriteString(0, "LAYER");
  if (!wlayer0 && layer->m_tableName == "0") {
    wlayer0 = true;
    m_writer->WriteString(5, "10");
  } else {
    m_writer->WriteString(5, ToHexString(++m_entityCount));
  }
  m_writer->WriteString(330, "2");

  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLayerTableRecord");
  m_writer->WriteUtf8String(2, layer->m_tableName);

  m_writer->WriteInt16(70, layer->m_flagValues);
  m_writer->WriteInt16(62, layer->m_colorNumber);
  if (m_version > EoDxf::Version::AC1015 && layer->color24 >= 0) { m_writer->WriteInt32(420, layer->color24); }
  m_writer->WriteUtf8String(6, layer->m_linetypeName);
  if (!layer->m_plottingFlag) { m_writer->WriteBool(290, layer->m_plottingFlag); }
  m_writer->WriteInt16(370, EoDxfLineWidths::lineWidth2dxfInt(layer->m_lineweightEnumValue));
  m_writer->WriteString(390, "F");

  if (!layer->m_extensionData.empty()) { WriteExtData(layer->m_extensionData); }
  //    m_writer->WriteString(347, "10012");
  return true;
}

bool EoDxfWrite::WriteLinetype(EoDxfLinetype* lineType) {
  std::string strname = lineType->m_tableName;

  std::transform(strname.begin(), strname.end(), strname.begin(), ::toupper);
  // do not write linetypes handled by library
  if (strname == "BYLAYER" || strname == "BYBLOCK" || strname == "CONTINUOUS") { return true; }
  m_writer->WriteString(0, "LTYPE");

  m_writer->WriteString(5, ToHexString(++m_entityCount));
  m_writer->WriteString(330, "5");
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbLinetypeTableRecord");
  m_writer->WriteUtf8String(2, lineType->m_tableName);

  m_writer->WriteInt16(70, lineType->m_flagValues);
  m_writer->WriteUtf8String(3, lineType->desc);
  lineType->Update();
  m_writer->WriteInt16(72, 65);
  m_writer->WriteInt16(73, lineType->size);
  m_writer->WriteDouble(40, lineType->length);

  for (unsigned int i = 0; i < lineType->path.size(); i++) {
    m_writer->WriteDouble(49, lineType->path.at(i));
    m_writer->WriteInt16(74, 0);
  }
  return true;
}

bool EoDxfWrite::WriteTextstyle(EoDxfTextStyle* textStyle) {
  m_writer->WriteString(0, "STYLE");
  if (!m_standardDimensionStyle) {
    std::string name = textStyle->m_tableName;
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if (name == "STANDARD") { m_standardDimensionStyle = true; }
  }
  m_writer->WriteString(5, ToHexString(++m_entityCount));

  m_writer->WriteString(330, "2");

  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbTextStyleTableRecord");
  m_writer->WriteUtf8String(2, textStyle->m_tableName);

  m_writer->WriteInt16(70, textStyle->m_flagValues);
  m_writer->WriteDouble(40, textStyle->height);
  m_writer->WriteDouble(41, textStyle->width);
  m_writer->WriteDouble(50, textStyle->oblique);
  m_writer->WriteInt16(71, textStyle->genFlag);
  m_writer->WriteDouble(42, textStyle->lastHeight);

  m_writer->WriteUtf8String(3, textStyle->font);
  m_writer->WriteUtf8String(4, textStyle->bigFont);
  if (textStyle->fontFamily != 0) { m_writer->WriteInt32(1071, textStyle->fontFamily); }
  return true;
}

bool EoDxfWrite::WriteVport(EoDxfVPort* viewport) {
  if (!m_standardDimensionStyle) {
    viewport->m_tableName = "*ACTIVE";
    m_standardDimensionStyle = true;
  }
  m_writer->WriteString(0, "VPORT");

  m_writer->WriteString(5, ToHexString(++m_entityCount));
  m_writer->WriteString(330, "2");
  m_writer->WriteString(100, "AcDbSymbolTableRecord");
  m_writer->WriteString(100, "AcDbViewportTableRecord");
  m_writer->WriteUtf8String(2, viewport->m_tableName);

  m_writer->WriteInt16(70, viewport->m_flagValues);
  m_writer->WriteDouble(10, viewport->lowerLeft.x);
  m_writer->WriteDouble(20, viewport->lowerLeft.y);
  m_writer->WriteDouble(11, viewport->upperRight.x);
  m_writer->WriteDouble(21, viewport->upperRight.y);
  m_writer->WriteDouble(12, viewport->center.x);
  m_writer->WriteDouble(22, viewport->center.y);
  m_writer->WriteDouble(13, viewport->snapBase.x);
  m_writer->WriteDouble(23, viewport->snapBase.y);
  m_writer->WriteDouble(14, viewport->snapSpacing.x);
  m_writer->WriteDouble(24, viewport->snapSpacing.y);
  m_writer->WriteDouble(15, viewport->gridSpacing.x);
  m_writer->WriteDouble(25, viewport->gridSpacing.y);
  m_writer->WriteDouble(16, viewport->viewDir.x);
  m_writer->WriteDouble(26, viewport->viewDir.y);
  m_writer->WriteDouble(36, viewport->viewDir.z);
  m_writer->WriteDouble(17, viewport->viewTarget.x);
  m_writer->WriteDouble(27, viewport->viewTarget.y);
  m_writer->WriteDouble(37, viewport->viewTarget.z);
  m_writer->WriteDouble(40, viewport->height);
  m_writer->WriteDouble(41, viewport->ratio);
  m_writer->WriteDouble(42, viewport->lensHeight);
  m_writer->WriteDouble(43, viewport->frontClip);
  m_writer->WriteDouble(44, viewport->backClip);
  m_writer->WriteDouble(50, viewport->snapAngle);
  m_writer->WriteDouble(51, viewport->twistAngle);
  m_writer->WriteInt16(71, viewport->viewMode);
  m_writer->WriteInt16(72, viewport->circleZoom);
  m_writer->WriteInt16(73, viewport->fastZoom);
  m_writer->WriteInt16(74, viewport->ucsIcon);
  m_writer->WriteInt16(75, viewport->snap);
  m_writer->WriteInt16(76, viewport->grid);
  m_writer->WriteInt16(77, viewport->snapStyle);
  m_writer->WriteInt16(78, viewport->snapIsopair);
  if (m_version > EoDxf::Version::AC1014) {
    m_writer->WriteInt16(281, 0);
    m_writer->WriteInt16(65, 1);
    m_writer->WriteDouble(110, 0.0);
    m_writer->WriteDouble(120, 0.0);
    m_writer->WriteDouble(130, 0.0);
    m_writer->WriteDouble(111, 1.0);
    m_writer->WriteDouble(121, 0.0);
    m_writer->WriteDouble(131, 0.0);
    m_writer->WriteDouble(112, 0.0);
    m_writer->WriteDouble(122, 1.0);
    m_writer->WriteDouble(132, 0.0);
    m_writer->WriteInt16(79, 0);
    m_writer->WriteDouble(146, 0.0);
    if (m_version > EoDxf::Version::AC1018) {
      m_writer->WriteString(348, "10020");
      m_writer->WriteInt16(60, viewport->gridBehavior);
      m_writer->WriteInt16(61, 5);
      m_writer->WriteBool(292, 1);
      m_writer->WriteInt16(282, 1);
      m_writer->WriteDouble(141, 0.0);
      m_writer->WriteDouble(142, 0.0);
      m_writer->WriteInt16(63, 250);
      m_writer->WriteInt32(421, 3358443);
    }
  }
  return true;
}
