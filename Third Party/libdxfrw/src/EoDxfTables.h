#pragma once

#include <string>
#include <vector>

#include "EoDxfBase.h"
#include "intern/EoDxfReader.h"

namespace EoDxf {

enum class SymbolTable : std::uint8_t {
  Unknown,
  Block,
  DimStyle,
  Layer,
  Linetype,
  RegApp,
  TextStyle,
  UCS,
  View,
  Viewport
};

}  // namespace EoDxf

class EoDxfTableEntry {
 public:
  EoDxfTableEntry() : tType{EoDxf::SymbolTable::Unknown}, m_flagValues{}, m_ownerHandle{}, m_currentVariant{} {}

 protected:
  explicit EoDxfTableEntry(EoDxf::SymbolTable tableType) noexcept
      : tType{tableType}, m_flagValues{}, m_ownerHandle{}, m_currentVariant{} {}
 public:
  virtual ~EoDxfTableEntry() {
    for (std::vector<EoDxfGroupCodeValuesVariant*>::iterator it = m_extensionData.begin(); it != m_extensionData.end(); ++it) { delete *it; }
    m_extensionData.clear();
  }

  EoDxfTableEntry(const EoDxfTableEntry& e) {
    tType = e.tType;
    m_handle = e.m_handle;
    m_ownerHandle = e.m_ownerHandle;
    m_tableName = e.m_tableName;
    m_flagValues = e.m_flagValues;
    m_currentVariant = e.m_currentVariant;
    for (std::vector<EoDxfGroupCodeValuesVariant*>::const_iterator it = e.m_extensionData.begin(); it != e.m_extensionData.end(); ++it) {
      m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(*(*it)));
    }
  }

 protected:
  void ParseCode(int code, EoDxfReader* reader);
  void Reset();

 public:
  EoDxf::SymbolTable tType{EoDxf::SymbolTable::Unknown};  // Group code 0
  std::uint32_t m_handle{};  // Group code 5
  std::uint32_t m_ownerHandle{};  // Group code 330
  std::string m_tableName;  // Group code 2
  int m_flagValues{};  // Group code 70
  std::vector<EoDxfGroupCodeValuesVariant*> m_extensionData;  // Group codes 1000 to 1071

 private:
  EoDxfGroupCodeValuesVariant* m_currentVariant{};
};

/**@brief Class to handle dimension style table entry
 *
 *  A dimension style table entry represents a set of properties that control the appearance of dimensions in a drawing.
 *  It is defined by its name (code 2) and various properties such as text height (code 140), arrow size (code 41),
 *  extension line offset (code 42), dimension line gap (code 147), and many others. The dimension style can also
 * include properties such as the dimension scale factor (code 40), dimension line extension (code 44), and dimension
 * text placement (code 71), which can affect how dimensions are displayed in the drawing.
 */
class EoDxfDimensionStyle : public EoDxfTableEntry {
  friend class dxfRW;

 public:
  EoDxfDimensionStyle() : EoDxfTableEntry(EoDxf::SymbolTable::DimStyle) { Reset(); }
 protected:
  void ParseCode(int code, EoDxfReader* reader);
  void Reset();

 public:
  std::string dimpost;  // Group code 3
  std::string dimapost;  // Group code 4

  std::string dimblk;  // Group code 5, code 342 V2000+
  std::string dimblk1;  // Group code 6, code 343 V2000+
  std::string dimblk2;  // Group code 7, code 344 V2000+
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
  std::string dimtxsty;  // Group code 340 R13+
  std::string dimldrblk;  // Group code 341 V2000+
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
class EoDxfLinetype : public EoDxfTableEntry {
  friend class dxfRW;

 public:
  EoDxfLinetype() : EoDxfTableEntry(EoDxf::SymbolTable::Linetype) { Reset(); }
 protected:
  void ParseCode(int code, EoDxfReader* reader);
  void Reset();
  void Update();

 public:
  std::string desc;  // Group code 3
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
class EoDxfLayer : public EoDxfTableEntry {
  friend class dxfRW;

 public:
  EoDxfLayer() : EoDxfTableEntry(EoDxf::SymbolTable::Layer) { Reset(); }
 protected:
  void ParseCode(int code, EoDxfReader* reader);
  void Reset();

 public:
  std::string m_linetypeName;  // Group code 6
  int m_colorNumber;  // Group code 62 (if negative, then layer is turned off)
  int color24;  // Group code 420
  bool m_plottingFlag;  // Group code 290 (if set to zero, then layer is not plotted)
  enum DRW_LW_Conv::lineWidth m_lineweightEnumValue;  // Group code 370
  std::string m_handleOfPlotStyleName;  // Group code 390
  std::string m_handleOfMaterialStyleName;  // Group code 347
};

/** Class to handle block record entries
 *
 *  A block record table entry represents a block definition in a drawing, which is a collection of entities that can be
 * inserted into the drawing as a single object. It is defined by its name (code 2) and various properties such as the
 * block insertion units (code 70) and the block insertion base point (code 10, 20, and 30). The block record can also
 * include properties such as flags (code 70), which can indicate whether the block is anonymous, external, or has
 * attributes.
 */
class EoDxfBlockRecord : public EoDxfTableEntry {
  friend class dxfRW;

 public:
  EoDxfBlockRecord() : EoDxfTableEntry(EoDxf::SymbolTable::Block) { Reset(); }
 protected:
  void ParseCode(int code, EoDxfReader* reader);
  void Reset();

 public:
  int m_blockInsertionUnits;  // Group code 70
  EoDxfGeometryBase3d m_firstPoint;
};

/** Class to handle textstyle entries
 *
 *  A text style table entry represents a text style in a drawing, which defines the appearance of text entities such as
 * single-line text and multi-line text. It is defined by its name (code 2) and various properties such as the fixed
 * text height (code 40), width factor (code 41), oblique angle (code 50), and font file name (code 3). The text style
 * can also include properties such as the last height used (code 42) and the big font file name (code 4), which can
 * affect how text entities are rendered in the drawing.
 */
class EoDxfTextStyle : public EoDxfTableEntry {
  friend class dxfRW;

 public:
  EoDxfTextStyle() : EoDxfTableEntry(EoDxf::SymbolTable::TextStyle) { Reset(); }
 protected:
  void ParseCode(int code, EoDxfReader* reader);

  void Reset() {
    height = oblique = 0.0;
    width = lastHeight = 1.0;
    font = "txt";
    genFlag = 0;  // 2= X mirror, 4= Y mirror
    fontFamily = 0;
    EoDxfTableEntry::Reset();
  }

 public:
  double height;  // Group code 40
  double width;  // Group code 41
  double oblique;  // Group code 50
  int genFlag;  // Group code 71
  double lastHeight;  // Group code 42
  std::string font;  // Group code 3
  std::string bigFont;  // Group code 4
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
class EoDxfViewport : public EoDxfTableEntry {
  friend class dxfRW;

 public:
  EoDxfViewport() : EoDxfTableEntry(EoDxf::SymbolTable::Viewport) { Reset(); }
  void Reset();

 protected:
  void ParseCode(int code, EoDxfReader* reader);

 public:
  EoDxfGeometryBase3d lowerLeft;  // Group codes 10 & 20
  EoDxfGeometryBase3d upperRight;  // Group codes 11 & 21
  EoDxfGeometryBase3d center;  // Group codes 12 & 22
  EoDxfGeometryBase3d snapBase;  // Group codes 13 & 23
  EoDxfGeometryBase3d snapSpacing;  // Group codes 14 & 24
  EoDxfGeometryBase3d gridSpacing;  // Group codes 15 & 25
  EoDxfGeometryBase3d viewDir;  // Group codes 16, 26 & 36
  EoDxfGeometryBase3d viewTarget;  // Group codes 17, 27 & 37
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

/** Class to handle application ID entries
 *
 *  An application ID table entry represents an application-defined identifier in a drawing, which can be used to
 * associate custom data with entities in the drawing. It is defined by its name (code 2) and various properties such as
 * flags (code 70), which can indicate whether the application ID is for internal use only or is externally referenced.
 */
class EoDxfAppId : public EoDxfTableEntry {
  friend class dxfRW;

 public:
  EoDxfAppId() : EoDxfTableEntry(EoDxf::SymbolTable::RegApp) { Reset(); }
 protected:
  void ParseCode(int code, EoDxfReader* reader) { EoDxfTableEntry::ParseCode(code, reader); }

  void Reset() {
    m_tableName = "";
    EoDxfTableEntry::Reset();
  }
};

namespace EoDxf {

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

}  // namespace EoDxf