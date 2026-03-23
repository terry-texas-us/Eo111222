#pragma once

#include <cstdint>
#include <string>

/// @brief Represents a single DIMSTYLE table entry for DXF dimension style round-trip.
///
/// Stores the DXF DIMSTYLE table fields needed to preserve dimension style definitions
/// across DXF import/export. The field names and semantics match the DXF DIMSTYLE
/// table entry (AcDbDimStyleTableRecord) group codes.
///
/// AeSys does not interpret dimension styles — this is a passthrough container that
/// ensures dimension style tables survive a DXF read/write cycle intact.
class EoDbDimStyle {
 public:
  EoDbDimStyle() = default;

  /// @brief Style name (DXF group 2). Typically "Standard" for the default style.
  std::wstring m_name{L"Standard"};

  /// @brief Standard DXF flag values (DXF group 70).
  std::int16_t m_flagValues{};

  /// @brief Entity handle from the DXF DIMSTYLE table (DXF group 105).
  std::uint64_t m_handle{};

  /// @brief Owner handle — typically the DIMSTYLE table handle (DXF group 330).
  std::uint64_t m_ownerHandle{};

  // String properties
  std::wstring dimpost;   ///< Group code 3 — General dimensioning suffix
  std::wstring dimapost;  ///< Group code 4 — Alternate dimensioning suffix
  std::wstring dimblk;    ///< Group code 5 / 342 — Arrow block name
  std::wstring dimblk1;   ///< Group code 6 / 343 — First arrow block name
  std::wstring dimblk2;   ///< Group code 7 / 344 — Second arrow block name

  // Double properties
  double dimscale{};  ///< Group code 40 — Overall dimensioning scale factor
  double dimasz{};    ///< Group code 41 — Dimensioning arrow size
  double dimexo{};    ///< Group code 42 — Extension line offset
  double dimdli{};    ///< Group code 43 — Dimension line increment
  double dimexe{};    ///< Group code 44 — Extension line extension
  double dimrnd{};    ///< Group code 45 — Rounding value for dimension distances
  double dimdle{};    ///< Group code 46 — Dimension line extension
  double dimtp{};     ///< Group code 47 — Plus tolerance
  double dimtm{};     ///< Group code 48 — Minus tolerance
  double dimfxl{};    ///< Group code 49 — Fixed extension line length (AC1021+)
  double dimtxt{};    ///< Group code 140 — Dimensioning text height
  double dimcen{};    ///< Group code 141 — Size of center mark/lines
  double dimtsz{};    ///< Group code 142 — Dimensioning tick size
  double dimaltf{};   ///< Group code 143 — Alternate unit scale factor
  double dimlfac{};   ///< Group code 144 — Linear measurements scale factor
  double dimtvp{};    ///< Group code 145 — Text vertical position
  double dimtfac{};   ///< Group code 146 — Dimension tolerance display scale factor
  double dimgap{};    ///< Group code 147 — Dimension line gap
  double dimaltrnd{}; ///< Group code 148 — Alternate unit rounding (AC1015+)

  // Int16 properties
  std::int16_t dimtol{};    ///< Group code 71
  std::int16_t dimlim{};    ///< Group code 72
  std::int16_t dimtih{};    ///< Group code 73
  std::int16_t dimtoh{};    ///< Group code 74
  std::int16_t dimse1{};    ///< Group code 75
  std::int16_t dimse2{};    ///< Group code 76
  std::int16_t dimtad{};    ///< Group code 77
  std::int16_t dimzin{};    ///< Group code 78
  std::int16_t dimazin{};   ///< Group code 79 (AC1015+)
  std::int16_t dimalt{};    ///< Group code 170
  std::int16_t dimaltd{};   ///< Group code 171
  std::int16_t dimtofl{};   ///< Group code 172
  std::int16_t dimsah{};    ///< Group code 173
  std::int16_t dimtix{};    ///< Group code 174
  std::int16_t dimsoxd{};   ///< Group code 175
  std::int16_t dimclrd{};   ///< Group code 176
  std::int16_t dimclre{};   ///< Group code 177
  std::int16_t dimclrt{};   ///< Group code 178
  std::int16_t dimadec{};   ///< Group code 179 (AC1015+)
  std::int16_t dimunit{};   ///< Group code 270 (AC1012+, obsolete 2000+)
  std::int16_t dimdec{};    ///< Group code 271 (AC1012+)
  std::int16_t dimtdec{};   ///< Group code 272 (AC1012+)
  std::int16_t dimaltu{};   ///< Group code 273 (AC1012+)
  std::int16_t dimalttd{};  ///< Group code 274 (AC1012+)
  std::int16_t dimaunit{};  ///< Group code 275 (AC1012+)
  std::int16_t dimfrac{};   ///< Group code 276 (AC1015+)
  std::int16_t dimlunit{};  ///< Group code 277 (AC1015+)
  std::int16_t dimdsep{};   ///< Group code 278 (AC1015+)
  std::int16_t dimtmove{};  ///< Group code 279 (AC1015+)
  std::int16_t dimjust{};   ///< Group code 280 (AC1012+)
  std::int16_t dimsd1{};    ///< Group code 281 (AC1012+)
  std::int16_t dimsd2{};    ///< Group code 282 (AC1012+)
  std::int16_t dimtolj{};   ///< Group code 283 (AC1012+)
  std::int16_t dimtzin{};   ///< Group code 284 (AC1012+)
  std::int16_t dimaltz{};   ///< Group code 285 (AC1012+)
  std::int16_t dimaltttz{}; ///< Group code 286 (AC1012+)
  std::int16_t dimfit{};    ///< Group code 287 (AC1012+, obsolete 2000+)
  std::int16_t dimupt{};    ///< Group code 288 (AC1012+)
  std::int16_t dimatfit{};  ///< Group code 289 (AC1015+)

  // Bool properties
  bool dimfxlon{};  ///< Group code 290 — Fixed extension line length on (AC1021+)

  // Handle references stored as strings for passthrough
  std::wstring dimtxsty;   ///< Group code 340 — Dimension text style handle (AC1012+)
  std::wstring dimldrblk;  ///< Group code 341 — Leader block handle (AC1015+)

  // Lineweight properties
  std::int16_t dimlwd{};  ///< Group code 371 — Dimension line lineweight (AC1015+)
  std::int16_t dimlwe{};  ///< Group code 372 — Extension line lineweight (AC1015+)
};
