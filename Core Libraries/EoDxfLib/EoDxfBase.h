#pragma once

#include <cstdint>
#include <limits>
#include <numbers>

namespace EoDxf {
// For geometric calculations, point coincidence etc.
inline constexpr double geometricTolerance{1e-9};
// Machine epsilon for dimensionless comparisons
inline constexpr double numericEpsilon = std::numeric_limits<double>::epsilon();

inline constexpr auto Pi{std::numbers::pi};
inline constexpr auto TwoPi{2.0 * std::numbers::pi};
inline constexpr auto HalfPi{std::numbers::pi / 2.0};
inline constexpr auto RadiansToDegrees{180.0 / std::numbers::pi};
inline constexpr auto DegreesToRadians{std::numbers::pi / 180.0};

inline constexpr std::size_t StringGroupCodeMaxChunk{250};
inline constexpr std::uint64_t NoHandle{};
inline constexpr std::int16_t colorByBlock{0};  /// Color inherited from block insert context (code 62 = 0)
inline constexpr std::int16_t colorByLayer{256};  /// Color inherited from layer definition  (code 62 = 256)

/// @brief Fixed AutoCAD infrastructure handle allocations present in every valid AC1015+ DXF file.
///
/// These are the canonical, non-negotiable handle values that form the non-graphical skeleton:
/// symbol tables, their mandatory entries, the two built-in block definitions, and the root
/// OBJECTS-section dictionaries.
///
/// All user entity and table-entry handles must be assigned at or above @c FirstUserHandle (0x30),
/// which is enforced by the writer's initial entity-counter seed.
///
/// Layout (hex):
/// @code
///   0x00  (database root — owner of all top-level tables and root dictionary)
///   ├─ 0x01  BLOCK_RECORD table
///   │   ├─ 0x1E  *Paper_Space block record
///   │   └─ 0x1F  *Model_Space block record
///   ├─ 0x02  LAYER table  → 0x10 layer "0"
///   ├─ 0x03  STYLE table
///   ├─ 0x05  LTYPE table  → 0x14 ByBlock, 0x15 ByLayer, 0x16 Continuous
///   ├─ 0x06  VIEW table
///   ├─ 0x07  UCS table
///   ├─ 0x08  VPORT table
///   ├─ 0x09  APPID table  → 0x12 ACAD
///   ├─ 0x0A  DIMSTYLE table
///   └─ 0x0C  root DICTIONARY → 0x0D ACAD_GROUP
///   BLOCKS section:
///   *Paper_Space: BLOCK=0x1C  ENDBLK=0x1D
///   *Model_Space: BLOCK=0x20  ENDBLK=0x21
/// @endcode
namespace Handles {
// ── Symbol tables ─────────────────────────────────────────────────────────────
inline constexpr std::uint64_t BlockRecordTable{0x01};
inline constexpr std::uint64_t LayerTable{0x02};
inline constexpr std::uint64_t StyleTable{0x03};
inline constexpr std::uint64_t LTypeTable{0x05};
inline constexpr std::uint64_t ViewTable{0x06};
inline constexpr std::uint64_t UcsTable{0x07};
inline constexpr std::uint64_t VPortTable{0x08};
inline constexpr std::uint64_t AppIdTable{0x09};
inline constexpr std::uint64_t DimStyleTable{0x0A};

// ── OBJECTS section root ──────────────────────────────────────────────────────
inline constexpr std::uint64_t RootDictionary{0x0C};
inline constexpr std::uint64_t AcadGroupDictionary{0x0D};

// ── Mandatory table entries ───────────────────────────────────────────────────
inline constexpr std::uint64_t DefaultLayer{0x10};        ///< layer "0"
inline constexpr std::uint64_t AcadAppId{0x12};           ///< ACAD APPID entry
inline constexpr std::uint64_t ByBlockLinetype{0x14};     ///< ByBlock linetype entry
inline constexpr std::uint64_t ByLayerLinetype{0x15};     ///< ByLayer linetype entry
inline constexpr std::uint64_t ContinuousLinetype{0x16};  ///< Continuous linetype entry

// ── *Paper_Space built-in block ───────────────────────────────────────────────
inline constexpr std::uint64_t PaperSpaceBlock{0x1C};        ///< *Paper_Space BLOCK entity
inline constexpr std::uint64_t PaperSpaceEndBlk{0x1D};       ///< *Paper_Space ENDBLK entity
inline constexpr std::uint64_t PaperSpaceBlockRecord{0x1E};  ///< *Paper_Space BLOCK_RECORD entry

// ── *Model_Space built-in block ───────────────────────────────────────────────
inline constexpr std::uint64_t ModelSpaceBlockRecord{0x1F};  ///< *Model_Space BLOCK_RECORD entry
inline constexpr std::uint64_t ModelSpaceBlock{0x20};        ///< *Model_Space BLOCK entity
inline constexpr std::uint64_t ModelSpaceEndBlk{0x21};       ///< *Model_Space ENDBLK entity

// ── First handle safe for user allocation ─────────────────────────────────────
/// Exceeds all infrastructure handles above, with reserved headroom for future additions.
inline constexpr std::uint64_t FirstUserHandle{0x30};  ///< = 48 decimal
}  // namespace Handles

/** @brief Canonical AutoCAD Color Index (ACI) RGB lookup table — 256 entries, DXF spec-faithful.
 *
 *  Maps each ACI value (1–255) to its standard {R, G, B} triple as defined by the AutoCAD DXF/DWG specification.
 *  Index [0] is an unused placeholder; direct indexing starts at [1]. Use @c AciToRgb() for bounds-safe access.
 *
 *  Two entries carry context-dependent meaning in AutoCAD:
 *  - **Index 0** (ByBlock): color is inherited from the owning block insert context.
 *  - **Index 7** (black/white): renders as white on dark backgrounds, black on light.
 *    This table stores {0,0,0} per the spec default; callers must adjust for display context when needed.
 *
 *  Indices 250–255 are the neutral gray ramp: approximately 20 %, 36 %, 52 %, 68 %, 84 %, and 100 % white.
 *
 *  @par Relationship to Eo::ColorPalette
 *  @c Eo::ColorPalette (in AeSys) maps the same 256 ACI indices to @c COLORREF values but uses a different
 *  brightness quantization and an extra dark level per hue, optimised for the AeSys dark-background display
 *  pipeline. The two tables share the same hue structure but are **not interchangeable**:
 *  - Use @c dxfColors for spec-faithful DXF I/O (e.g., deriving group code 420
 *    from an ACI index, or validating ACI round-trips during import/export).
 *  - Use @c Eo::ColorPalette for on-screen rendering within AeSys.
 *
 *  When the EoDxfLib and AeSys colour systems are more tightly integrated, this table remains the authoritative
 *  DXF reference; @c Eo::ColorPalette remains the display reference. A conversion layer at the interface
 *  boundary is the intended integration point.
 *
 *  @see AciToRgb()
 */
inline constexpr unsigned char aciCanonicalRgbTable[][3] = {{0, 0, 0},
    // Official named ACI colors, indices 1–7 (Red, Yellow, Green, Cyan, Blue, Magenta, Black/White)
    {255, 0, 0}, {255, 255, 0}, {0, 255, 0}, {0, 255, 255}, {0, 0, 255}, {255, 0, 255}, {0, 0, 0},
    // Official unnamed ACI colors, indices 8–249
    {65, 65, 65}, {128, 128, 128}, {255, 0, 0}, {255, 127, 127}, {204, 0, 0}, {204, 102, 102}, {153, 0, 0},
    {153, 76, 76}, {127, 0, 0}, {127, 63, 63}, {76, 0, 0}, {76, 38, 38}, {255, 63, 0}, {255, 159, 127}, {204, 51, 0},
    {204, 127, 102}, {153, 38, 0}, {153, 95, 76}, {127, 31, 0}, {127, 79, 63}, {76, 19, 0}, {76, 47, 38}, {255, 127, 0},
    {255, 191, 127}, {204, 102, 0}, {204, 153, 102}, {153, 76, 0}, {153, 114, 76}, {127, 63, 0}, {127, 95, 63},
    {76, 38, 0}, {76, 57, 38}, {255, 191, 0}, {255, 223, 127}, {204, 153, 0}, {204, 178, 102}, {153, 114, 0},
    {153, 133, 76}, {127, 95, 0}, {127, 111, 63}, {76, 57, 0}, {76, 66, 38}, {255, 255, 0}, {255, 255, 127},
    {204, 204, 0}, {204, 204, 102}, {153, 153, 0}, {153, 153, 76}, {127, 127, 0}, {127, 127, 63}, {76, 76, 0},
    {76, 76, 38}, {191, 255, 0}, {223, 255, 127}, {153, 204, 0}, {178, 204, 102}, {114, 153, 0}, {133, 153, 76},
    {95, 127, 0}, {111, 127, 63}, {57, 76, 0}, {66, 76, 38}, {127, 255, 0}, {191, 255, 127}, {102, 204, 0},
    {153, 204, 102}, {76, 153, 0}, {114, 153, 76}, {63, 127, 0}, {95, 127, 63}, {38, 76, 0}, {57, 76, 38}, {63, 255, 0},
    {159, 255, 127}, {51, 204, 0}, {127, 204, 102}, {38, 153, 0}, {95, 153, 76}, {31, 127, 0}, {79, 127, 63},
    {19, 76, 0}, {47, 76, 38}, {0, 255, 0}, {127, 255, 127}, {0, 204, 0}, {102, 204, 102}, {0, 153, 0}, {76, 153, 76},
    {0, 127, 0}, {63, 127, 63}, {0, 76, 0}, {38, 76, 38}, {0, 255, 63}, {127, 255, 159}, {0, 204, 51}, {102, 204, 127},
    {0, 153, 38}, {76, 153, 95}, {0, 127, 31}, {63, 127, 79}, {0, 76, 19}, {38, 76, 47}, {0, 255, 127}, {127, 255, 191},
    {0, 204, 102}, {102, 204, 153}, {0, 153, 76}, {76, 153, 114}, {0, 127, 63}, {63, 127, 95}, {0, 76, 38},
    {38, 76, 57}, {0, 255, 191}, {127, 255, 223}, {0, 204, 153}, {102, 204, 178}, {0, 153, 114}, {76, 153, 133},
    {0, 127, 95}, {63, 127, 111}, {0, 76, 57}, {38, 76, 66}, {0, 255, 255}, {127, 255, 255}, {0, 204, 204},
    {102, 204, 204}, {0, 153, 153}, {76, 153, 153}, {0, 127, 127}, {63, 127, 127}, {0, 76, 76}, {38, 76, 76},
    {0, 191, 255}, {127, 223, 255}, {0, 153, 204}, {102, 178, 204}, {0, 114, 153}, {76, 133, 153}, {0, 95, 127},
    {63, 111, 127}, {0, 57, 76}, {38, 66, 76}, {0, 127, 255}, {127, 191, 255}, {0, 102, 204}, {102, 153, 204},
    {0, 76, 153}, {76, 114, 153}, {0, 63, 127}, {63, 95, 127}, {0, 38, 76}, {38, 57, 76}, {0, 66, 255}, {127, 159, 255},
    {0, 51, 204}, {102, 127, 204}, {0, 38, 153}, {76, 95, 153}, {0, 31, 127}, {63, 79, 127}, {0, 19, 76}, {38, 47, 76},
    {0, 0, 255}, {127, 127, 255}, {0, 0, 204}, {102, 102, 204}, {0, 0, 153}, {76, 76, 153}, {0, 0, 127}, {63, 63, 127},
    {0, 0, 76}, {38, 38, 76}, {63, 0, 255}, {159, 127, 255}, {50, 0, 204}, {127, 102, 204}, {38, 0, 153}, {95, 76, 153},
    {31, 0, 127}, {79, 63, 127}, {19, 0, 76}, {47, 38, 76}, {127, 0, 255}, {191, 127, 255}, {102, 0, 204},
    {153, 102, 204}, {76, 0, 153}, {114, 76, 153}, {63, 0, 127}, {95, 63, 127}, {38, 0, 76}, {57, 38, 76},
    {191, 0, 255}, {223, 127, 255}, {153, 0, 204}, {178, 102, 204}, {114, 0, 153}, {133, 76, 153}, {95, 0, 127},
    {111, 63, 127}, {57, 0, 76}, {66, 38, 76}, {255, 0, 255}, {255, 127, 255}, {204, 0, 204}, {204, 102, 204},
    {153, 0, 153}, {153, 76, 153}, {127, 0, 127}, {127, 63, 127}, {76, 0, 76}, {76, 38, 76}, {255, 0, 191},
    {255, 127, 223}, {204, 0, 153}, {204, 102, 178}, {153, 0, 114}, {153, 76, 133}, {127, 0, 95}, {127, 63, 11},
    {76, 0, 57}, {76, 38, 66}, {255, 0, 127}, {255, 127, 191}, {204, 0, 102}, {204, 102, 153}, {153, 0, 76},
    {153, 76, 114}, {127, 0, 63}, {127, 63, 95}, {76, 0, 38}, {76, 38, 57}, {255, 0, 63}, {255, 127, 159}, {204, 0, 51},
    {204, 102, 127}, {153, 0, 38}, {153, 76, 95}, {127, 0, 31}, {127, 63, 79}, {76, 0, 19}, {76, 38, 47},
    // Neutral grays, indices 250–255 (approximately 20 %, 36 %, 52 %, 68 %, 84 %, and 100 % white.)
    {51, 51, 51}, {91, 91, 91}, {132, 132, 132}, {173, 173, 173}, {214, 214, 214}, {255, 255, 255}};

// Version numbers for the DXF Format.
enum class Version : std::uint8_t {
  UNKNOWN,  // UNKNOWN VERSION (default / unreadable header)
  AC1006,  // R10
  AC1009,  // R11 & R12
  AC1012,  // R13
  AC1014,  // R14
  AC1015,  // AutoCAD 2000 / 2000i / 2002
  AC1018,  // AutoCAD 2004 / 2005 / 2006
  AC1021,  // AutoCAD 2007 / 2008 / 2009
  AC1024,  // AutoCAD 2010 / 2011 / 2012
  AC1027,  // AutoCAD 2013 / 2014 / 2015 / 2016 / 2017
  AC1032  // AutoCAD 2018 / 2019 / 2020 / 2021 / 2022 / 2023 / 2024 / 2025 / 2026 (no new code since 2018)
};

enum class Space : std::uint8_t { ModelSpace, PaperSpace };
enum class ShadowMode : std::uint8_t { CastAndReceiveShadows, CastShadows, ReceiveShadows, IgnoreShadows };
enum class TransparencyCodes : int8_t { Opaque, Transparent = -1 };

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

//! Entity's type.
enum ETYPE {
  _3DFACE,
  ACAD_PROXY_ENTITY,
  ARC,
  ATTDEF,
  ATTRIB,
  BLOCK,  // and ENDBLK
  CIRCLE,
  DIMENSION,
  DIMALIGNED,
  DIMLINEAR,
  DIMRADIAL,
  DIMDIAMETRIC,
  DIMANGULAR,
  DIMANGULAR3P,
  DIMORDINATE,
  ELLIPSE,
  HATCH,
  IMAGE,
  INSERT,
  LEADER,
  LINE,
  LWPOLYLINE,
  MLEADER,
  MTEXT,
  POINT,
  POLYLINE,
  RAY,
  SEQEND,
  SOLID,
  SPLINE,
  TEXT,
  TRACE,
  UNDERLAY,
  VERTEX,
  VIEWPORT,
  XLINE,
  UNKNOWN
};

namespace Detail {

/** @brief Trims leading and trailing whitespace exactly as DXF ASCII parsers (atoi/istringstream) have done
 * historically. Uses std::isspace so it accepts space, tab, \r, \n, etc. This is the canonical whitespace rule for all
 * EoDxf ASCII parsing.
 */
inline std::string_view Trim(std::string_view s) noexcept {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) { s.remove_prefix(1); }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) { s.remove_suffix(1); }
  return s;
}

}  // namespace Detail
}  // namespace EoDxf
