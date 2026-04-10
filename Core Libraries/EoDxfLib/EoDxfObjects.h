#pragma once

#include <map>
#include <string>
#include <vector>

#include "EoDxfEntity.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfReader.h"

class EoDxfWriter;

/** @brief Base class for DXF OBJECTS section entries.
 *
 *  Derives from EoDxfEntity to share the common database-identity fields (handle, owner handle,
 *  reactor handles, extension dictionary handle, extended data, and application-defined groups)
 *  with graphical entity types. This unifies all handle-bearing DXF objects under a single
 *  hierarchy, following the same pattern as ezdxf's DXFEntity base.
 *
 *  Common group codes inherited from EoDxfEntity:
 *  - 5: Handle
 *  - 102: Application-defined groups (ACAD_REACTORS, ACAD_XDICTIONARY, and others)
 *  - 330: Soft-pointer ID/handle to owner dictionary (or reactor handle within ACAD_REACTORS)
 *  - 360: Hard-owner ID/handle (extension dictionary handle within ACAD_XDICTIONARY)
 *  - 1000-1071: Extended data
 */
class EoDxfObjectEntry : public EoDxfEntity {
 public:
  EoDxfObjectEntry() = default;
  ~EoDxfObjectEntry() override = default;

  EoDxfObjectEntry(const EoDxfObjectEntry&) = default;
  EoDxfObjectEntry& operator=(const EoDxfObjectEntry&) = default;
  EoDxfObjectEntry(EoDxfObjectEntry&&) noexcept = default;
  EoDxfObjectEntry& operator=(EoDxfObjectEntry&&) noexcept = default;

 protected:
  /// Delegates to EoDxfEntity::ParseCode for all common group codes.
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();
};

// ACDBPLACEHOLDER, DICTIONARYVAR, FIELD, GROUP, IDBUFFER, IMAGEDEF_REACTOR, LAYER_FILTER, LAYER_INDEX, LAYOUT,
// MATERIAL, MLINESTYLE, PLOTSETTINGS, RASTERVARIABLES, SORTENTSTABLE, SPATIAL_FILTER, SPATIAL_INDEX,
// TABLESTYLE, VBA_PROJECT, VISUALSTYLE, XRECORD

/** @brief Class to handle image definition entries
 *
 *  An image definition object entry represents an image definition in a drawing, which defines the properties of an
 *  image that can be inserted into the drawing. It is defined by its name (code 1) and various properties such as the
 *  image size in pixels (code 10 and 20), default size of one pixel (code 11 and 12), loaded flag (code 280), and
 *  resolution units (code 281). The image definition can also include properties such as the group class version
 *  (code 90) and a map of reactors, which are objects that react to changes in the image definition.
 */
class EoDxfImageDefinition : public EoDxfObjectEntry {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfImageDefinition() { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

  void Reset();

 public:
  std::wstring m_fileNameOfImage;  // Group code 1
  std::int32_t m_classVersion;  // Group code 90, 0=R14 version
  double m_uImageSizeInPixels{};  // Group code 10
  double m_vImageSizeInPixels{};  // Group code 20
  double m_uSizeOfOnePixel{};  // Group code 11
  double m_vSizeOfOnePixel{};  // Group code 21
  std::int16_t m_imageIsLoadedFlag{};  // Group code 280, 0=unloaded, 1=loaded
  std::int16_t m_resolutionUnits{};  // Group code 281, 0=no, 2=centimeters, 5=inch

  std::map<std::wstring, std::wstring> reactors;
};

class EoDxfUnsupportedObject {
 public:
  void Write(EoDxfWriter* writer) const;

  std::wstring m_objectType;
  std::vector<EoDxfGroupCodeValuesVariant> m_values;
};

/** @brief Structured representation of a DXF LAYOUT object (OBJECTS section).
 *
 *  A LAYOUT object carries two subclass sections:
 *  - **AcDbPlotSettings**: paper size, margins, plotter configuration, scale, plot area type, etc.
 *  - **AcDbLayout**: layout name, tab order, paper-space limits/extents, UCS definition, and
 *    the handle link to the associated BLOCK_RECORD.
 *
 *  Group codes 1, 70, 76, and 330 appear in both subclasses with different meanings.
 *  `m_currentSubclass` tracks the active subclass marker (code 100) to disambiguate.
 *
 *  The Model layout (tab order 0) links to *Model_Space block record (typically handle 0x1F).
 *  Paper-space layouts link to *Paper_Space or named block records.
 *
 *  ## Handle Identity
 *  Inherits `m_handle` and `m_ownerHandle` from `EoDxfObjectEntry` → `EoDxfEntity`.
 *  The owner is typically the ACAD_LAYOUT dictionary.
 */
class EoDxfLayout : public EoDxfObjectEntry {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfLayout() { Reset(); }

  /// @brief Writes the LAYOUT object to a DXF writer in the standard group-code order.
  void Write(EoDxfWriter* writer) const;

  /// @brief Returns true if this layout represents the Model tab (tab order 0, name "Model").
  [[nodiscard]] bool IsModelLayout() const noexcept { return m_tabOrder == 0; }

 protected:
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();

 private:
  /// Tracks which AcDb subclass section the parser is currently in.
  enum class Subclass { None, PlotSettings, Layout };
  Subclass m_currentSubclass{Subclass::None};

 public:
  // ── AcDbPlotSettings ─────────────────────────────────────────────────────────
  std::wstring m_pageSetupName;          ///< Code 1: Page/plot setup name
  std::wstring m_plotConfigName;         ///< Code 2: Plotter configuration name (e.g. "none_device")
  std::wstring m_paperSizeName;          ///< Code 4: Paper size name (e.g. "ISO_A4_(210.00_x_297.00_MM)")
  std::wstring m_plotViewName;           ///< Code 6: Plot view name (empty = no named view)
  double m_leftMargin{};                 ///< Code 40: Left unprintable margin (mm)
  double m_bottomMargin{};               ///< Code 41: Bottom unprintable margin (mm)
  double m_rightMargin{};                ///< Code 42: Right unprintable margin (mm)
  double m_topMargin{};                  ///< Code 43: Top unprintable margin (mm)
  double m_paperWidth{};                 ///< Code 44: Paper width (mm)
  double m_paperHeight{};                ///< Code 45: Paper height (mm)
  double m_plotOriginX{};                ///< Code 46: Plot origin X offset (mm)
  double m_plotOriginY{};                ///< Code 47: Plot origin Y offset (mm)
  double m_plotWindowLowerLeftX{};       ///< Code 48: Plot window area lower-left X
  double m_plotWindowLowerLeftY{};       ///< Code 49: Plot window area lower-left Y
  double m_plotWindowUpperRightX{};      ///< Code 140: Plot window area upper-right X
  double m_plotWindowUpperRightY{};      ///< Code 141: Plot window area upper-right Y
  double m_customScaleNumerator{1.0};    ///< Code 142: Numerator of custom print scale
  double m_customScaleDenominator{1.0};  ///< Code 143: Denominator of custom print scale
  std::int16_t m_plotLayoutFlags{};      ///< Code 70: Plot layout flags (bitfield)
  std::int16_t m_plotPaperUnits{};       ///< Code 72: 0=inches, 1=mm, 2=pixels
  std::int16_t m_plotRotation{};         ///< Code 73: 0=none, 1=90°CCW, 2=180°, 3=90°CW
  std::int16_t m_plotType{};             ///< Code 74: 0=last, 1=extents, 2=limits, 3=view, 4=window, 5=layout
  std::wstring m_currentStyleSheet;      ///< Code 7: Current plot style sheet name
  std::int16_t m_standardScaleType{};    ///< Code 75: Standard scale type (0=scaled to fit, 16=1:1, etc.)
  std::int16_t m_shadePlotMode{};        ///< Code 76 (PlotSettings): Shade plot mode
  std::int16_t m_shadePlotResLevel{};    ///< Code 77: Shade plot resolution level
  std::int16_t m_shadePlotCustomDpi{};   ///< Code 78: Shade plot custom DPI
  double m_scaleFactor{1.0};             ///< Code 147: Scale factor (paper units / drawing units)
  double m_paperImageOriginX{};          ///< Code 148: Paper image origin X
  double m_paperImageOriginY{};          ///< Code 149: Paper image origin Y

  // ── AcDbLayout ───────────────────────────────────────────────────────────────
  std::wstring m_layoutName;             ///< Code 1: Layout name (e.g. "Model", "Layout1")
  std::int16_t m_layoutFlags{};          ///< Code 70: Layout flags (bit 1 = PSLTSCALE)
  std::int16_t m_tabOrder{};             ///< Code 71: Tab order (0 = Model)
  double m_limminX{};                    ///< Code 10: Minimum limits X
  double m_limminY{};                    ///< Code 20: Minimum limits Y
  double m_limmaxX{};                    ///< Code 11: Maximum limits X
  double m_limmaxY{};                    ///< Code 21: Maximum limits Y
  double m_insertBaseX{};                ///< Code 12: Insert base point X
  double m_insertBaseY{};                ///< Code 22: Insert base point Y
  double m_insertBaseZ{};                ///< Code 32: Insert base point Z
  double m_extminX{};                    ///< Code 14: Minimum extents X
  double m_extminY{};                    ///< Code 24: Minimum extents Y
  double m_extminZ{};                    ///< Code 34: Minimum extents Z
  double m_extmaxX{};                    ///< Code 15: Maximum extents X
  double m_extmaxY{};                    ///< Code 25: Maximum extents Y
  double m_extmaxZ{};                    ///< Code 35: Maximum extents Z
  double m_elevation{};                  ///< Code 146: Elevation
  double m_ucsOriginX{};                 ///< Code 13: UCS origin X
  double m_ucsOriginY{};                 ///< Code 23: UCS origin Y
  double m_ucsOriginZ{};                 ///< Code 33: UCS origin Z
  double m_ucsXAxisX{1.0};              ///< Code 16: UCS X-axis direction X
  double m_ucsXAxisY{};                  ///< Code 26: UCS X-axis direction Y
  double m_ucsXAxisZ{};                  ///< Code 36: UCS X-axis direction Z
  double m_ucsYAxisX{};                  ///< Code 17: UCS Y-axis direction X
  double m_ucsYAxisY{1.0};              ///< Code 27: UCS Y-axis direction Y
  double m_ucsYAxisZ{};                  ///< Code 37: UCS Y-axis direction Z
  std::int16_t m_ucsOrthoType{};         ///< Code 76 (Layout): UCS orthographic type
  std::uint64_t m_blockRecordHandle{EoDxf::NoHandle};  ///< Code 330 (Layout): Associated block record handle
  std::uint64_t m_lastActiveViewportHandle{EoDxf::NoHandle};  ///< Code 331: Last active viewport handle
};
