#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "EoDxfBase.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfReader.h"

class EoDxfTableEntry {
 public:
  EoDxfTableEntry() = default;

 protected:
  explicit EoDxfTableEntry(EoDxf::SymbolTable tableType) noexcept
      : tType{tableType} {}

 public:
  virtual ~EoDxfTableEntry() = default;

  EoDxfTableEntry(const EoDxfTableEntry& other)
      : tType{other.tType},
        m_handle{other.m_handle},
        m_ownerHandle{other.m_ownerHandle},
        m_tableName{other.m_tableName},
        m_flagValues{other.m_flagValues} {
    m_extensionData.reserve(other.m_extensionData.size());
    for (const auto& variant : other.m_extensionData) {
      m_extensionData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(*variant));
    }
  }

  EoDxfTableEntry& operator=(const EoDxfTableEntry& other) {
    if (this != &other) {
      std::vector<std::unique_ptr<EoDxfGroupCodeValuesVariant>> extensionData;
      extensionData.reserve(other.m_extensionData.size());
      for (const auto& variant : other.m_extensionData) {
        extensionData.push_back(std::make_unique<EoDxfGroupCodeValuesVariant>(*variant));
      }

      tType = other.tType;
      m_handle = other.m_handle;
      m_ownerHandle = other.m_ownerHandle;
      m_tableName = other.m_tableName;
      m_flagValues = other.m_flagValues;
      m_extensionData = std::move(extensionData);
      m_currentVariant = nullptr;
    }
    return *this;
  }

  EoDxfTableEntry(EoDxfTableEntry&& other) noexcept
      : tType{other.tType},
        m_handle{other.m_handle},
        m_ownerHandle{other.m_ownerHandle},
        m_tableName{std::move(other.m_tableName)},
        m_flagValues{other.m_flagValues},
        m_extensionData{std::move(other.m_extensionData)},
        m_currentVariant{std::exchange(other.m_currentVariant, nullptr)} {}

  EoDxfTableEntry& operator=(EoDxfTableEntry&& other) noexcept {
    if (this != &other) {
      tType = other.tType;
      m_handle = other.m_handle;
      m_ownerHandle = other.m_ownerHandle;
      m_tableName = std::move(other.m_tableName);
      m_flagValues = other.m_flagValues;
      m_extensionData = std::move(other.m_extensionData);
      m_currentVariant = std::exchange(other.m_currentVariant, nullptr);
    }
    return *this;
  }

 protected:
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();

 public:
  EoDxf::SymbolTable tType{EoDxf::SymbolTable::Unknown};  // Group code 0
  std::uint64_t m_handle{};  // Group code 5
  std::uint64_t m_ownerHandle{};  // Group code 330
  std::wstring m_tableName;  // Group code 2
  std::int16_t m_flagValues{};  // Group code 70
  std::vector<std::unique_ptr<EoDxfGroupCodeValuesVariant>> m_extensionData;  // Group codes 1000 to 1071

 private:
  /// Non-owning observer for the in-progress geometry variant during extended-data 1010/1020/1030 triplet parsing.
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
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfDimensionStyle() : EoDxfTableEntry(EoDxf::SymbolTable::DimStyle) { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();

 public:
  std::wstring dimpost;  // Group code 3
  std::wstring dimapost;  // Group code 4

  std::wstring dimblk;  // Group code 5, code 342 (AC1015+)
  std::wstring dimblk1;  // Group code 6, code 343 (AC1015+)
  std::wstring dimblk2;  // Group code 7, code 344 (AC1015+)
  double dimscale;  // Group code 40
  double dimasz;  // Group code 41
  double dimexo;  // Group code 42
  double dimdli;  // Group code 43
  double dimexe;  // Group code 44
  double dimrnd;  // Group code 45
  double dimdle;  // Group code 46
  double dimtp;  // Group code 47
  double dimtm;  // Group code 48
  double dimfxl;  // Group code 49 (AC1021+)
  double dimtxt;  // Group code 140
  double dimcen;  // Group code 141
  double dimtsz;  // Group code 142
  double dimaltf;  // Group code 143
  double dimlfac;  // Group code 144
  double dimtvp;  // Group code 145
  double dimtfac;  // Group code 146
  double dimgap;  // Group code 147
  double dimaltrnd;  // Group code 148 (AC1015+)
  std::int16_t dimtol;  // Group code 71
  std::int16_t dimlim;  // Group code 72
  std::int16_t dimtih;  // Group code 73
  std::int16_t dimtoh;  // Group code 74
  std::int16_t dimse1;  // Group code 75
  std::int16_t dimse2;  // Group code 76
  std::int16_t dimtad;  // Group code 77
  std::int16_t dimzin;  // Group code 78
  std::int16_t dimazin;  // Group code 79 (AC1015+)
  std::int16_t dimalt;  // Group code 170
  std::int16_t dimaltd;  // Group code 171
  std::int16_t dimtofl;  // Group code 172
  std::int16_t dimsah;  // Group code 173
  std::int16_t dimtix;  // Group code 174
  std::int16_t dimsoxd;  // Group code 175
  std::int16_t dimclrd;  // Group code 176
  std::int16_t dimclre;  // Group code 177
  std::int16_t dimclrt;  // Group code 178
  std::int16_t dimadec;  // Group code 179 (AC1015+)
  std::int16_t dimunit;  // Group code 270 (AC1012+) (obsolete 2000+, use dimlunit & dimfrac)
  std::int16_t dimdec;  // Group code 271 (AC1012+)
  std::int16_t dimtdec;  // Group code 272 (AC1012+)
  std::int16_t dimaltu;  // Group code 273 (AC1012+)
  std::int16_t dimalttd;  // Group code 274 (AC1012+)
  std::int16_t dimaunit;  // Group code 275 (AC1012+)
  std::int16_t dimfrac;  // Group code 276 (AC1015+)
  std::int16_t dimlunit;  // Group code 277 (AC1015+)
  std::int16_t dimdsep;  // Group code 278 (AC1015+)
  std::int16_t dimtmove;  // Group code 279 (AC1015+)
  std::int16_t dimjust;  // Group code 280 (AC1012+)
  std::int16_t dimsd1;  // Group code 281 (AC1012+)
  std::int16_t dimsd2;  // Group code 282 (AC1012+)
  std::int16_t dimtolj;  // Group code 283 (AC1012+)
  std::int16_t dimtzin;  // Group code 284 (AC1012+)
  std::int16_t dimaltz;  // Group code 285 (AC1012+)
  std::int16_t dimaltttz;  // Group code 286 (AC1012+)
  std::int16_t dimfit;  // Group code 287 (AC1012+)  (obsolete 2000+, use dimatfit & dimtmove)
  std::int16_t dimupt;  // Group code 288 (AC1012+)
  std::int16_t dimatfit;  // Group code 289 (AC1015+)
  bool dimfxlon;  // Group code 290 (AC1021+)
  std::wstring dimtxsty;  // Group code 340 (AC1012+)
  std::wstring dimldrblk;  // Group code 341 (AC1015+)
  std::int16_t dimlwd;  // Group code 371 (AC1015+)
  std::int16_t dimlwe;  // Group code 372 (AC1015+)
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
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfLinetype() : EoDxfTableEntry(EoDxf::SymbolTable::Linetype) { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();
  void Update();

 public:
  std::wstring desc;  // Group code 3
  std::int16_t m_numberOfLinetypeElements;  // Group code 73
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
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfLayer() : EoDxfTableEntry(EoDxf::SymbolTable::Layer) { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();

 public:
  std::wstring m_linetypeName;  // Group code 6
  std::int16_t m_colorNumber;  // Group code 62 (if negative, then layer is turned off)
  int color24;  // Group code 420
  bool m_plottingFlag;  // Group code 290 (if set to zero, then layer is not plotted)
  enum EoDxfLineWeights::LineWeight m_lineweightEnumValue;  // Group code 370
  std::wstring m_handleOfPlotStyleName;  // Group code 390
  std::wstring m_handleOfMaterialStyleName;  // Group code 347
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
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfBlockRecord() : EoDxfTableEntry(EoDxf::SymbolTable::Block) { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();

 public:
  std::int16_t m_blockInsertionUnits;  // Group code 70
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
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfTextStyle() : EoDxfTableEntry(EoDxf::SymbolTable::TextStyle) { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

  void Reset() {
    height = 0.0;
    oblique = 0.0;
    width = lastHeight = 1.0;
    font = L"txt";
    m_textGenerationFlag = 0;  // 2= X mirror, 4= Y mirror
    fontFamily = 0;
    EoDxfTableEntry::Reset();
  }

 public:
  double height;  // Group code 40
  double width;  // Group code 41
  double oblique;  // Group code 50
  std::int16_t m_textGenerationFlag;  // Group code 71
  double lastHeight;  // Group code 42
  std::wstring font;  // Group code 3
  std::wstring bigFont;  // Group code 4
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
class EoDxfVPort : public EoDxfTableEntry {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfVPort() : EoDxfTableEntry(EoDxf::SymbolTable::Viewport) { Reset(); }
  void Reset();

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  EoDxfGeometryBase2d m_lowerLeftCorner;  // Lower-left corner of viewport, code 10 & 20
  EoDxfGeometryBase2d m_upperRightCorner{1.0, 1.0};  // Upper-right corner of viewport, code 11 & 21
  EoDxfGeometryBase2d m_viewCenter{0.651828, -0.16};  // View center point (in DCS), code 12 & 22
  EoDxfGeometryBase2d m_snapBasePoint;  // Snap base point (in DCS), code 13 & 23
  EoDxfGeometryBase2d m_snapSpacing{10.0, 10.0};  // Snap spacing X and Y, code 14 & 24
  EoDxfGeometryBase2d m_gridSpacing{10.0, 10.0};  // Grid spacing X and Y, code 15 & 25
  EoDxfGeometryBase3d m_viewDirection{0.0, 0.0, 1.0};  // View direction from target point (WCS), code 16, 26 & 36
  EoDxfGeometryBase3d m_viewTargetPoint;  // View target point (WCS), code 17, 27 & 37
  double m_viewHeight{5.13732};  // View height, code 40
  double m_viewAspectRatio{2.4426877};  // Viewport aspect ratio, code 41
  double m_lensLength{50.0};  // Lens length, code 42
  double m_frontClipPlane{};  // Front clipping plane offset, code 43
  double m_backClipPlane{};  // Back clipping plane offset, code 44
  double m_snapRotationAngle{};  // Snap rotation angle, code 50
  double m_viewTwistAngle{};  // View twist angle, code 51
  std::int16_t m_viewMode{};  // Group code 71 (VIEWMODE system variable)
  std::int16_t m_circleZoomPercent{100};  // Circle zoom percent, code 72
  std::int16_t m_fastZoom{1};  // Fast zoom setting, code 73
  std::int16_t m_ucsIcon{3};  // UCSICON setting, code 74
  std::int16_t m_snapOn{};  // Snap on/off, code 75
  std::int16_t m_gridOn{};  // Grid on/off, code 76
  std::int16_t m_snapStyle{};  // Snap style, code 77
  std::int16_t m_snapIsopair{};  // Snap isopair, code 78
  std::int16_t m_gridBehavior{7}; /** Group code 60, possible values are
                                   * bit 1 (1) show out of limits
                                   * bit 2 (2) adaptive grid
                                   * bit 3 (4) allow subdivision
                                   * bit 4 (8) follow dynamic SCP **/
};

/** Class to handle application ID entries
 *
 *  An application ID table entry represents an application-defined identifier in a drawing, which can be used to
 * associate custom data with entities in the drawing. It is defined by its name (code 2) and various properties such as
 * flags (code 70), which can indicate whether the application ID is for internal use only or is externally referenced.
 */
class EoDxfAppId : public EoDxfTableEntry {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfAppId() : EoDxfTableEntry(EoDxf::SymbolTable::RegApp) { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader) { EoDxfTableEntry::ParseCode(code, reader); }

  void Reset() {
    m_tableName = L"";
    EoDxfTableEntry::Reset();
  }
};
