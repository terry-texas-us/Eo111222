#pragma once

#include "drw_base.h"
#include <map>
#include <string>
#include <vector>

class dxfReader;
class dxfWriter;

namespace DRW {

enum TTYPE { UNKNOWNT, LTYPE, LAYER, STYLE, DIMSTYLE, VPORT, BLOCK_RECORD, APPID, IMAGEDEF };

// pending VIEW, UCS, VP_ENT_HDR, GROUP, MLINESTYLE, LONG_TRANSACTION, XRECORD,
// ACDBPLACEHOLDER, VBA_PROJECT, ACAD_TABLE, CELLSTYLEMAP, DBCOLOR, DICTIONARYVAR,
// DICTIONARYWDFLT, FIELD, IDBUFFER, IMAGEDEFREACTOR, LAYER_INDEX, LAYOUT
// MATERIAL, PLACEHOLDER, PLOTSETTINGS, RASTERVARIABLES, SCALE, SORTENTSTABLE,
// SPATIAL_INDEX, SPATIAL_FILTER, TABLEGEOMETRY, TABLESTYLES, VISUALSTYLE

}  // namespace DRW

class DRW_TableEntry {
 public:
  DRW_TableEntry() : tType{DRW::UNKNOWNT}, flags{}, parentHandle{}, m_currentVariant{} {}

 protected:
  explicit DRW_TableEntry(DRW::TTYPE tableType) noexcept
      : tType{tableType}, flags{}, parentHandle{}, m_currentVariant{} {}

 public:
  virtual ~DRW_TableEntry() {
    for (std::vector<DRW_Variant*>::iterator it = extData.begin(); it != extData.end(); ++it) { delete *it; }
    extData.clear();
  }

  DRW_TableEntry(const DRW_TableEntry& e) {
    tType = e.tType;
    handle = e.handle;
    parentHandle = e.parentHandle;
    name = e.name;
    flags = e.flags;
    m_currentVariant = e.m_currentVariant;
    for (std::vector<DRW_Variant*>::const_iterator it = e.extData.begin(); it != e.extData.end(); ++it) {
      extData.push_back(new DRW_Variant(*(*it)));
    }
  }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void Reset() {
    flags = 0;
    for (auto* variant : extData) { delete variant; }
    extData.clear();
    m_currentVariant = nullptr;
  }

 public:
  enum DRW::TTYPE tType{DRW::UNKNOWNT};  // Group code 0
  std::uint32_t handle{};  // Group code 5
  int parentHandle{};  // Group code 330
  UTF8STRING name;  // Group code 2
  int flags{};  // Group code 70
  std::vector<DRW_Variant*> extData;  // Group codes 1000 to 1071

 private:
  DRW_Variant* m_currentVariant{};
};

/**@brief Class to handle dimension style table entry
 *
 *  A dimension style table entry represents a set of properties that control the appearance of dimensions in a drawing.
 *  It is defined by its name (code 2) and various properties such as text height (code 140), arrow size (code 41),
 *  extension line offset (code 42), dimension line gap (code 147), and many others. The dimension style can also
 * include properties such as the dimension scale factor (code 40), dimension line extension (code 44), and dimension
 * text placement (code 71), which can affect how dimensions are displayed in the drawing.
 */
class DRW_Dimstyle : public DRW_TableEntry {
  friend class dxfRW;

 public:
  DRW_Dimstyle() : DRW_TableEntry(DRW::DIMSTYLE) { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void Reset() {
    dimasz = dimtxt = dimexe = 0.18;
    dimexo = 0.0625;
    dimgap = dimcen = 0.09;
    dimtxsty = "Standard";
    dimscale = dimlfac = dimtfac = dimfxl = 1.0;
    dimdli = 0.38;
    dimrnd = dimdle = dimtp = dimtm = dimtsz = dimtvp = 0.0;
    dimaltf = 25.4;
    dimtol = dimlim = dimse1 = dimse2 = dimtad = dimzin = 0;
    dimtoh = dimtolj = 1;
    dimalt = dimtofl = dimsah = dimtix = dimsoxd = dimfxlon = 0;
    dimaltd = dimunit = dimaltu = dimalttd = dimlunit = 2;
    dimclrd = dimclre = dimclrt = dimjust = dimupt = 0;
    dimazin = dimaltz = dimaltttz = dimtzin = dimfrac = 0;
    dimtih = dimadec = dimaunit = dimsd1 = dimsd2 = dimtmove = 0;
    dimaltrnd = 0.0;
    dimdec = dimtdec = 4;
    dimfit = dimatfit = 3;
    dimdsep = '.';
    dimlwd = dimlwe = -2;
    DRW_TableEntry::Reset();
  }

 public:
  UTF8STRING dimpost;  // Group code 3
  UTF8STRING dimapost;  // Group code 4

  UTF8STRING dimblk;  // Group code 5, code 342 V2000+
  UTF8STRING dimblk1;  // Group code 6, code 343 V2000+
  UTF8STRING dimblk2;  // Group code 7, code 344 V2000+
  double dimscale;  // Group code 40
  double dimasz;  // Group code 41
  double dimexo;  // Group code 42
  double dimdli;  // Group code 43
  double dimexe;  // Group code 44
  double dimrnd;  // Group code 45
  double dimdle;  // Group code 46
  double dimtp;  // Group code 47
  double dimtm;  // Group code 48
  double dimfxl;  // Group code 49 V2007+
  double dimtxt;  // Group code 140
  double dimcen;  // Group code 141
  double dimtsz;  // Group code 142
  double dimaltf;  // Group code 143
  double dimlfac;  // Group code 144
  double dimtvp;  // Group code 145
  double dimtfac;  // Group code 146
  double dimgap;  // Group code 147
  double dimaltrnd;  // Group code 148 V2000+
  int dimtol;  // Group code 71
  int dimlim;  // Group code 72
  int dimtih;  // Group code 73
  int dimtoh;  // Group code 74
  int dimse1;  // Group code 75
  int dimse2;  // Group code 76
  int dimtad;  // Group code 77
  int dimzin;  // Group code 78
  int dimazin;  // Group code 79 V2000+
  int dimalt;  // Group code 170
  int dimaltd;  // Group code 171
  int dimtofl;  // Group code 172
  int dimsah;  // Group code 173
  int dimtix;  // Group code 174
  int dimsoxd;  // Group code 175
  int dimclrd;  // Group code 176
  int dimclre;  // Group code 177
  int dimclrt;  // Group code 178
  int dimadec;  // Group code 179 V2000+
  int dimunit;  // Group code 270 R13+ (obsolete 2000+, use dimlunit & dimfrac)
  int dimdec;  // Group code 271 R13+
  int dimtdec;  // Group code 272 R13+
  int dimaltu;  // Group code 273 R13+
  int dimalttd;  // Group code 274 R13+
  int dimaunit;  // Group code 275 R13+
  int dimfrac;  // Group code 276 V2000+
  int dimlunit;  // Group code 277 V2000+
  int dimdsep;  // Group code 278 V2000+
  int dimtmove;  // Group code 279 V2000+
  int dimjust;  // Group code 280 R13+
  int dimsd1;  // Group code 281 R13+
  int dimsd2;  // Group code 282 R13+
  int dimtolj;  // Group code 283 R13+
  int dimtzin;  // Group code 284 R13+
  int dimaltz;  // Group code 285 R13+
  int dimaltttz;  // Group code 286 R13+
  int dimfit;  // Group code 287 R13+  (obsolete 2000+, use dimatfit & dimtmove)
  int dimupt;  // Group code 288 R13+
  int dimatfit;  // Group code 289 V2000+
  int dimfxlon;  // Group code 290 V2007+
  UTF8STRING dimtxsty;  // Group code 340 R13+
  UTF8STRING dimldrblk;  // Group code 341 V2000+
  int dimlwd;  // Group code 371 V2000+
  int dimlwe;  // Group code 372 V2000+
};

/** Class to handle linetype entries
 *
 *  A linetype table entry represents a pattern of dashes and gaps that can be applied to lines in a drawing.
 *  It is defined by its name (code 2) and various properties such as the description (code 3), number of elements
 * (code 73), total length of the pattern (code 40), and the sequence of dash and gap lengths (code 49). The linetype
 * can also include properties such as the alignment code (code 72) and the complex linetype type flag (code 74),
 * which can affect how the linetype is rendered in the drawing.
 */
class DRW_LType : public DRW_TableEntry {
  friend class dxfRW;

 public:
  DRW_LType() : DRW_TableEntry(DRW::LTYPE) { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void Reset() {
    desc = "";
    size = 0;
    length = 0.0;
    pathIdx = 0;
    DRW_TableEntry::Reset();
  }

  void update();

 public:
  UTF8STRING desc;  // Group code 3
  int size;  // Group code 73
  double length;  // Group code 40
  std::vector<double> path;  // Group code 49
 private:
  int pathIdx;
};

/** Class to handle layer entries
 *
 *  A layer table entry represents a layer in a drawing, which is a logical grouping of entities that can be turned on
 * or off for display and plotting purposes. It is defined by its name (code 2) and various properties such as the line
 * type (code 6), color (code 62), plot flag (code 290), line weight (code 370), and 24-bit color (code 420). The layer
 * can also include properties such as the hard-pointer ID/handle of the plot style (code 390) and material style (code
 * 347), which can affect how entities on the layer are rendered in the drawing.
 */
class DRW_Layer : public DRW_TableEntry {
  friend class dxfRW;

 public:
  DRW_Layer() : DRW_TableEntry(DRW::LAYER) { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void Reset() {
    lineType = "CONTINUOUS";
    color = 7;  // default BYLAYER (256)
    plotF = true;  // default TRUE (plot yes)
    lWeight = DRW_LW_Conv::widthDefault;  // default BYDEFAULT (dxf -3)
    color24 = -1;  // default -1 not set
    DRW_TableEntry::Reset();
  }

 public:
  UTF8STRING lineType;  // Group code 6
  int color;  // Group code 62
  int color24;  // Group code 420
  bool plotF;  // Group code 290
  enum DRW_LW_Conv::lineWidth lWeight;  // Group code 370
  std::string handlePlotS;  // Group code 390
  std::string handleMaterialS;  // Group code 347
};

/** Class to handle block record entries
 *
 *  A block record table entry represents a block definition in a drawing, which is a collection of entities that can be
 * inserted into the drawing as a single object. It is defined by its name (code 2) and various properties such as the
 * block insertion units (code 70) and the block insertion base point (code 10, 20, and 30). The block record can also
 * include properties such as flags (code 70), which can indicate whether the block is anonymous, external, or has
 * attributes.
 */
class DRW_Block_Record : public DRW_TableEntry {
  friend class dxfRW;

 public:
  DRW_Block_Record() : DRW_TableEntry(DRW::BLOCK_RECORD) { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void Reset() {
    m_blockInsertionUnits = 0;
    flags = 0;
    DRW_TableEntry::Reset();
  }

 public:
  int m_blockInsertionUnits;  // Group code 70
  DRW_Coord m_firstPoint;
};

/** Class to handle textstyle entries
 *
 *  A text style table entry represents a text style in a drawing, which defines the appearance of text entities such as
 * single-line text and multi-line text. It is defined by its name (code 2) and various properties such as the fixed
 * text height (code 40), width factor (code 41), oblique angle (code 50), and font file name (code 3). The text style
 * can also include properties such as the last height used (code 42) and the big font file name (code 4), which can
 * affect how text entities are rendered in the drawing.
 */
class DRW_Textstyle : public DRW_TableEntry {
  friend class dxfRW;

 public:
  DRW_Textstyle() : DRW_TableEntry(DRW::STYLE) { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void reset() {
    height = oblique = 0.0;
    width = lastHeight = 1.0;
    font = "txt";
    genFlag = 0;  // 2= X mirror, 4= Y mirror
    fontFamily = 0;
    DRW_TableEntry::Reset();
  }

 public:
  double height;  // Group code 40
  double width;  // Group code 41
  double oblique;  // Group code 50
  int genFlag;  // Group code 71
  double lastHeight;  // Group code 42
  UTF8STRING font;  // Group code 3
  UTF8STRING bigFont;  // Group code 4
  int fontFamily;  // Group code 1071
};

/** Class to handle viewport entries
 *
 *  A viewport table entry represents a viewport in a drawing, which is a window through which the model space is viewed
 * in paper space. It is defined by its name (code 2) and various properties such as the lower left corner (code 10,
 * 20), upper right corner (code 11, 21), center point (code 12, 22), snap base point (code 13, 23), snap spacing (code
 * 14, 24), grid spacing (code 15, 25), view direction (code 16, 26, 36), and view target point (code 17, 27, 37). The
 * viewport can also include properties such as the view height (code 40), aspect ratio (code 41), lens height (code
 * 42), front clipping plane (code 43), back clipping plane (code 44), snap rotation angle (code 50), view twist angle
 * (code 51), and various flags for view mode, snap, grid, and UCS icon display.
 */
class DRW_Vport : public DRW_TableEntry {
  friend class dxfRW;

 public:
  DRW_Vport() : DRW_TableEntry(DRW::VPORT) { Reset(); }

  void Reset() {
    upperRight.x = upperRight.y = 1.0;
    snapSpacing.x = snapSpacing.y = 10.0;
    gridSpacing = snapSpacing;
    center.x = 0.651828;
    center.y = -0.16;
    viewDir.z = 1;
    height = 5.13732;
    ratio = 2.4426877;
    lensHeight = 50;
    frontClip = backClip = snapAngle = twistAngle = 0.0;
    viewMode = snap = grid = snapStyle = snapIsopair = 0;
    fastZoom = 1;
    circleZoom = 100;
    ucsIcon = 3;
    gridBehavior = 7;
    DRW_TableEntry::Reset();
  }

 protected:
  void ParseCode(int code, dxfReader* reader);

 public:
  DRW_Coord lowerLeft;  // Group codes 10 & 20
  DRW_Coord upperRight;  // Group codes 11 & 21
  DRW_Coord center;  // Group codes 12 & 22
  DRW_Coord snapBase;  // Group codes 13 & 23
  DRW_Coord snapSpacing;  // Group codes 14 & 24
  DRW_Coord gridSpacing;  // Group codes 15 & 25
  DRW_Coord viewDir;  // Group codes 16, 26 & 36
  DRW_Coord viewTarget;  // Group codes 17, 27 & 37
  double height;  // Group code 40
  double ratio;  // Group code 41
  double lensHeight;  // Group code 42
  double frontClip;  // Group code 43
  double backClip;  // Group code 44
  double snapAngle;  // Group code 50
  double twistAngle;  // Group code 51
  int viewMode;  // Group code 71
  int circleZoom;  // Group code 72
  int fastZoom;  // Group code 73
  int ucsIcon;  // Group code 74
  int snap;  // Group code 75
  int grid;  // Group code 76
  int snapStyle;  // Group code 77
  int snapIsopair;  // Group code 78
  int gridBehavior;  // Group code 60, undocumented
  /** code 60, bit coded possible value are
   * bit 1 (1) show out of limits
   * bit 2 (2) adaptive grid
   * bit 3 (4) allow subdivision
   * bit 4 (8) follow dinamic SCP
   **/
};

/** Class to handle image definition entries
 *
 *  A image definition table entry represents an image definition in a drawing, which defines the properties of an image
 * that can be inserted into the drawing. It is defined by its name (code 1) and various properties such as the image
 * size in pixels (code 10 and 20), default size of one pixel (code 11 and 12), loaded flag (code 280), and resolution
 * units (code 281). The image definition can also include properties such as the group class version (code 90) and a
 * map of reactors, which are objects that react to changes in the image definition.
 */
class DRW_ImageDef : public DRW_TableEntry {  //
  friend class dxfRW;

 public:
  DRW_ImageDef() : DRW_TableEntry(DRW::IMAGEDEF) { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void Reset();

 public:
  //    std::string handle;       // Group code 5
  UTF8STRING name;  // Group code 1
  int imgVersion;  // Group code 90, 0=R14 version
  double u{};  // Group code 10
  double v{};  // Group code 20
  double up{};  // Group code 11
  double vp{};  // Group code 12 really is 21
  int loaded{};  // Group code 280, 0=unloaded, 1=loaded
  int resolution{};  // Group code 281, 0=no, 2=centimeters, 5=inch

  std::map<std::string, std::string> reactors;
};

/** Class to handle application ID entries
 *
 *  An application ID table entry represents an application-defined identifier in a drawing, which can be used to
 * associate custom data with entities in the drawing. It is defined by its name (code 2) and various properties such as
 * flags (code 70), which can indicate whether the application ID is for internal use only or is externally referenced.
 */
class DRW_AppId : public DRW_TableEntry {
  friend class dxfRW;

 public:
  DRW_AppId() : DRW_TableEntry(DRW::APPID) { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader) { DRW_TableEntry::ParseCode(code, reader); }

  void Reset() {
    flags = 0;
    name = "";
    DRW_TableEntry::Reset();
  }
};

namespace DRW {

// Extended color palette: The first entry is only for direct indexing starting with [1]
inline constexpr unsigned char dxfColors[][3] = {
    {0, 0, 0},  // unused
    {255, 0, 0},  // 1 red
    {255, 255, 0},  // 2 yellow
    {0, 255, 0},  // 3 green
    {0, 255, 255},  // 4 cyan
    {0, 0, 255},  // 5 blue
    {255, 0, 255},  // 6 magenta
    {0, 0, 0},  // 7 black or white
    {128, 128, 128},  // 8 50% gray
    {192, 192, 192},  // 9 75% gray
    {255, 0, 0},  // 10
    {255, 127, 127}, {204, 0, 0}, {204, 102, 102}, {153, 0, 0}, {153, 76, 76},  // 15
    {127, 0, 0}, {127, 63, 63}, {76, 0, 0}, {76, 38, 38}, {255, 63, 0},  // 20
    {255, 159, 127}, {204, 51, 0}, {204, 127, 102}, {153, 38, 0}, {153, 95, 76},  // 25
    {127, 31, 0}, {127, 79, 63}, {76, 19, 0}, {76, 47, 38}, {255, 127, 0},  // 30
    {255, 191, 127}, {204, 102, 0}, {204, 153, 102}, {153, 76, 0}, {153, 114, 76},  // 35
    {127, 63, 0}, {127, 95, 63}, {76, 38, 0}, {76, 57, 38}, {255, 191, 0},  // 40
    {255, 223, 127}, {204, 153, 0}, {204, 178, 102}, {153, 114, 0}, {153, 133, 76},  // 45
    {127, 95, 0}, {127, 111, 63}, {76, 57, 0}, {76, 66, 38}, {255, 255, 0},  // 50
    {255, 255, 127}, {204, 204, 0}, {204, 204, 102}, {153, 153, 0}, {153, 153, 76},  // 55
    {127, 127, 0}, {127, 127, 63}, {76, 76, 0}, {76, 76, 38}, {191, 255, 0},  // 60
    {223, 255, 127}, {153, 204, 0}, {178, 204, 102}, {114, 153, 0}, {133, 153, 76},  // 65
    {95, 127, 0}, {111, 127, 63}, {57, 76, 0}, {66, 76, 38}, {127, 255, 0},  // 70
    {191, 255, 127}, {102, 204, 0}, {153, 204, 102}, {76, 153, 0}, {114, 153, 76},  // 75
    {63, 127, 0}, {95, 127, 63}, {38, 76, 0}, {57, 76, 38}, {63, 255, 0},  // 80
    {159, 255, 127}, {51, 204, 0}, {127, 204, 102}, {38, 153, 0}, {95, 153, 76},  // 85
    {31, 127, 0}, {79, 127, 63}, {19, 76, 0}, {47, 76, 38}, {0, 255, 0},  // 90
    {127, 255, 127}, {0, 204, 0}, {102, 204, 102}, {0, 153, 0}, {76, 153, 76},  // 95
    {0, 127, 0}, {63, 127, 63}, {0, 76, 0}, {38, 76, 38}, {0, 255, 63},  // 100
    {127, 255, 159}, {0, 204, 51}, {102, 204, 127}, {0, 153, 38}, {76, 153, 95},  // 105
    {0, 127, 31}, {63, 127, 79}, {0, 76, 19}, {38, 76, 47}, {0, 255, 127},  // 110
    {127, 255, 191}, {0, 204, 102}, {102, 204, 153}, {0, 153, 76}, {76, 153, 114},  // 115
    {0, 127, 63}, {63, 127, 95}, {0, 76, 38}, {38, 76, 57}, {0, 255, 191},  // 120
    {127, 255, 223}, {0, 204, 153}, {102, 204, 178}, {0, 153, 114}, {76, 153, 133},  // 125
    {0, 127, 95}, {63, 127, 111}, {0, 76, 57}, {38, 76, 66}, {0, 255, 255},  // 130
    {127, 255, 255}, {0, 204, 204}, {102, 204, 204}, {0, 153, 153}, {76, 153, 153},  // 135
    {0, 127, 127}, {63, 127, 127}, {0, 76, 76}, {38, 76, 76}, {0, 191, 255},  // 140
    {127, 223, 255}, {0, 153, 204}, {102, 178, 204}, {0, 114, 153}, {76, 133, 153},  // 145
    {0, 95, 127}, {63, 111, 127}, {0, 57, 76}, {38, 66, 76}, {0, 127, 255},  // 150
    {127, 191, 255}, {0, 102, 204}, {102, 153, 204}, {0, 76, 153}, {76, 114, 153},  // 155
    {0, 63, 127}, {63, 95, 127}, {0, 38, 76}, {38, 57, 76}, {0, 66, 255},  // 160
    {127, 159, 255}, {0, 51, 204}, {102, 127, 204}, {0, 38, 153}, {76, 95, 153},  // 165
    {0, 31, 127}, {63, 79, 127}, {0, 19, 76}, {38, 47, 76}, {0, 0, 255},  // 170
    {127, 127, 255}, {0, 0, 204}, {102, 102, 204}, {0, 0, 153}, {76, 76, 153},  // 175
    {0, 0, 127}, {63, 63, 127}, {0, 0, 76}, {38, 38, 76}, {63, 0, 255},  // 180
    {159, 127, 255}, {50, 0, 204}, {127, 102, 204}, {38, 0, 153}, {95, 76, 153},  // 185
    {31, 0, 127}, {79, 63, 127}, {19, 0, 76}, {47, 38, 76}, {127, 0, 255},  // 190
    {191, 127, 255}, {102, 0, 204}, {153, 102, 204}, {76, 0, 153}, {114, 76, 153},  // 195
    {63, 0, 127}, {95, 63, 127}, {38, 0, 76}, {57, 38, 76}, {191, 0, 255},  // 200
    {223, 127, 255}, {153, 0, 204}, {178, 102, 204}, {114, 0, 153}, {133, 76, 153},  // 205
    {95, 0, 127}, {111, 63, 127}, {57, 0, 76}, {66, 38, 76}, {255, 0, 255},  // 210
    {255, 127, 255}, {204, 0, 204}, {204, 102, 204}, {153, 0, 153}, {153, 76, 153},  // 215
    {127, 0, 127}, {127, 63, 127}, {76, 0, 76}, {76, 38, 76}, {255, 0, 191},  // 220
    {255, 127, 223}, {204, 0, 153}, {204, 102, 178}, {153, 0, 114}, {153, 76, 133},  // 225
    {127, 0, 95}, {127, 63, 11}, {76, 0, 57}, {76, 38, 66}, {255, 0, 127},  // 230
    {255, 127, 191}, {204, 0, 102}, {204, 102, 153}, {153, 0, 76}, {153, 76, 114},  // 235
    {127, 0, 63}, {127, 63, 95}, {76, 0, 38}, {76, 38, 57}, {255, 0, 63},  // 240
    {255, 127, 159}, {204, 0, 51}, {204, 102, 127}, {153, 0, 38}, {153, 76, 95},  // 245
    {127, 0, 31}, {127, 63, 79}, {76, 0, 19}, {76, 38, 47}, {51, 51, 51},  // 250
    {91, 91, 91}, {132, 132, 132}, {173, 173, 173}, {214, 214, 214}, {255, 255, 255}  // 255
};

}  // namespace DRW