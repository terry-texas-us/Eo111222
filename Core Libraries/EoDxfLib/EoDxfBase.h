#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

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

}  // namespace EoDxf

/** @brief Class representing a generalized 3D coordinate (using for point and vector) with x, y, and z components.
 *  Provides constructors for initialization and a method to unitize the vector.
 */
class EoDxfGeometryBase3d {
 public:
  double x{};
  double y{};
  double z{};

  EoDxfGeometryBase3d() noexcept = default;

  EoDxfGeometryBase3d(double x, double y, double z) noexcept : x{x}, y{y}, z{z} {}

  EoDxfGeometryBase3d(const EoDxfGeometryBase3d&) noexcept = default;
  EoDxfGeometryBase3d& operator=(const EoDxfGeometryBase3d&) noexcept = default;

  EoDxfGeometryBase3d(EoDxfGeometryBase3d&&) noexcept = default;
  EoDxfGeometryBase3d& operator=(EoDxfGeometryBase3d&&) noexcept = default;

  /** @brief Checks if the coordinate is effectively the default normal vector (0.0, 0.0, 1.0) within a specified
   * tolerance.
   *
   * This method determines if the x and y components of the coordinate are within a certain distance (tolerance) from
   * zero, and if the z component is within a certain distance from 1.0. This is useful for geometric calculations
   * where the default normal vector may not be exactly (0.0, 0.0, 1.0) due to floating-point precision limitations. The
   * default tolerance is defined by EoDxf::geometricTolerance, but it can be overridden by providing a different value
   * when calling the method.
   *
   * @param tolerance The distance from zero for x and y, and from one for z, within which the coordinate is considered
   * effectively the default normal vector.
   * @return true if x and y are within the specified tolerance of zero and z is within the specified tolerance of one;
   * otherwise, false.
   */
  [[nodiscard]] bool IsDefaultNormal(double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x) < tolerance && std::abs(y) < tolerance && std::abs(z - 1.0) < tolerance;
  }

  /** @brief Checks if this coordinate is effectively equal to another coordinate within a specified tolerance.
   *
   * This method compares the x, y, and z components of this coordinate with those of another EoDxfGeometryBase3d
   * instance. It determines if the absolute difference between each corresponding component is less than a specified
   * tolerance value. This is useful for geometric calculations where exact equality may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can be
   * overridden by providing a different value when calling the method.
   *
   * @param other The other EoDxfGeometryBase3d instance to compare against.
   * @param tolerance The distance within which the components of the two coordinates are considered equal.
   * @return true if the absolute difference between each corresponding component (x, y, z) of the two coordinates is
   * less than the specified tolerance; otherwise, false.
   */
  [[nodiscard]] bool IsEqualTo(
      const EoDxfGeometryBase3d& other, double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x - other.x) < tolerance && std::abs(y - other.y) < tolerance && std::abs(z - other.z) < tolerance;
  }

  /** @brief Checks if the coordinate is effectively zero within a specified tolerance.
   *
   * This method determines if the x, y, and z components of the coordinate are all within a certain distance
   * (tolerance) from zero. This is useful for geometric calculations where exact zero may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can be
   * overridden by providing a different value when calling the method.
   *
   * @param tolerance The distance from zero within which the coordinate components are considered effectively zero.
   * @return true if all components (x, y, z) are within the specified tolerance of zero; otherwise, false.
   */
  [[nodiscard]] bool IsZero(double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x) < tolerance && std::abs(y) < tolerance && std::abs(z) < tolerance;
  }

  /** @brief Convert `this` coordinate to a unit-length vector (in-place).
   */
  void Unitize() noexcept {
    const double dist = sqrt(x * x + y * y + z * z);

    if (dist > EoDxf::geometricTolerance) {
      x = x / dist;
      y = y / dist;
      z = z / dist;
    }
  }

  /** @brief Return a unitized copy of `this` coordinate.
   *  This method creates a copy of the current EoDxfGeometryBase3d instance, applies the Unitize() method to the copy,
   *  and returns the unitized copy. The original coordinate remains unchanged.
   *  @return A new EoDxfGeometryBase3d instance that is a unitized version of the original coordinate.
   */
  [[nodiscard]] EoDxfGeometryBase3d Unitized() const noexcept {
    EoDxfGeometryBase3d result{*this};
    result.Unitize();
    return result;
  }
};

/** @brief Class representing a 2D coordinate with x and y components.
 *  Used for DXF group code pairs that are inherently 2D (e.g., snap base, grid spacing, view center in DCS).
 */
class EoDxfGeometryBase2d {
 public:
  double x{};
  double y{};

  EoDxfGeometryBase2d() noexcept = default;

  constexpr EoDxfGeometryBase2d(double x, double y) noexcept : x{x}, y{y} {}

  EoDxfGeometryBase2d(const EoDxfGeometryBase2d&) noexcept = default;
  EoDxfGeometryBase2d& operator=(const EoDxfGeometryBase2d&) noexcept = default;

  EoDxfGeometryBase2d(EoDxfGeometryBase2d&&) noexcept = default;
  EoDxfGeometryBase2d& operator=(EoDxfGeometryBase2d&&) noexcept = default;

  /** @brief Checks if this coordinate is effectively equal to another coordinate within a specified tolerance.
   *
   * This method compares the x and y components of this coordinate with those of another EoDxfGeometryBase2d
   * instance. It determines if the absolute difference between each corresponding component is less than a specified
   * tolerance value. This is useful for geometric calculations where exact equality may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can be
   * overridden by providing a different value when calling the method.
   *
   * @param other The other EoDxfGeometryBase2d instance to compare against.
   * @param tolerance The distance within which the components of the two coordinates are considered equal.
   * @return true if the absolute difference between each corresponding component (x, y) of the two coordinates is
   * less than the specified tolerance; otherwise, false.
   */
  [[nodiscard]] bool IsEqualTo(
      const EoDxfGeometryBase2d& other, double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x - other.x) < tolerance && std::abs(y - other.y) < tolerance;
  }

  /** @brief Checks if the coordinate is effectively zero within a specified tolerance.
   *
   * This method determines if the x and y components of the coordinate are all within a certain distance
   * (tolerance) from zero. This is useful for geometric calculations where exact zero may not be achievable due to
   * floating-point precision limitations. The default tolerance is defined by EoDxf::geometricTolerance, but it can
   * be overridden by providing a different value when calling the method.
   *
   * @param tolerance The distance from zero within which the coordinate components are considered effectively zero.
   * @return true if all components (x, y) are within the specified tolerance of zero; otherwise, false.
   */
  [[nodiscard]] bool IsZero(double tolerance = EoDxf::geometricTolerance) const noexcept {
    return std::abs(x) < tolerance && std::abs(y) < tolerance;
  }

  /** @brief Promote to 3D with z = 0.
   *  This method creates a new EoDxfGeometryBase3d instance using the x and y components of the current 2D coordinate
   *  and the specified z value (default is 0.0). The original 2D coordinate remains unchanged.
   *  @param z The z-coordinate for the new 3D coordinate. Default is 0.0.
   *  @return A new EoDxfGeometryBase3d instance with the specified z value.
   */
  [[nodiscard]] EoDxfGeometryBase3d To3d(double z = 0.0) const noexcept { return {x, y, z}; }
};

/** @brief Class representing a vertex of a lightweight polyline, with 2D coordinates and optional width and bulge
 * properties. The vertex is defined by its x and y coordinates (group codes 10 and 20), optional start and end widths
 * (group codes 40 and 41), and an optional bulge value (group code 42) that defines the curvature between this vertex
 * and the next one in the polyline.
 */
struct EoDxfPolylineVertex2d {
  double x{};  // x coordinate, code 10
  double y{};  // y coordinate, code 20
  double stawidth{};  // Start width, code 40
  double endwidth{};  // End width, code 41
  double bulge{};  // bulge, code 42

  /** @brief Default-constructed vertex (all zero). */
  EoDxfPolylineVertex2d() noexcept = default;

  /** @brief Position + bulge constructor (widths stay zero).
   * Matches the most common usage in DXF parsers.
   */
  constexpr EoDxfPolylineVertex2d(double x, double y, double bulge = {}) noexcept
      : x{x}, y{y}, stawidth{}, endwidth{}, bulge{bulge} {}
};

/** @brief Class representing a variant type for storing different types of values associated with DXF group codes.
 *  The class can hold a string, integer, double, or coordinate value, and it tracks the type of value currently stored.
 *  It provides constructors for each type, as well as copy and move semantics. The Add* methods allow updating the
 *  stored value and type after construction.
 */
class EoDxfGroupCodeValuesVariant {
 public:
  enum Type {
    String,
    Int16,  ///< 16-bit signed integer (DXF group codes 60-79, 170-179, 270-289, 370-389, 400-409, 1060-1070)
    Integer,  ///< 32-bit signed integer (DXF group codes 90-99, 420-429, 440-449, 450-459, 1071)
    Double,
    GeometryBase,
    Handle,  ///< 64-bit unsigned handle (DXF group codes 5, 105, 310-369 handle references)
    Invalid
  };

  EoDxfGroupCodeValuesVariant() = default;

  EoDxfGroupCodeValuesVariant(int code, std::int16_t i16) noexcept
      : m_content{i16}, m_variantType{Type::Int16}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::int32_t i) noexcept
      : m_content{i}, m_variantType{Type::Integer}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::uint32_t i) noexcept
      : m_content{static_cast<std::int32_t>(i)}, m_variantType{Type::Integer}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::uint64_t h) noexcept
      : m_content{h}, m_variantType{Type::Handle}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, double d) noexcept : m_content{d}, m_variantType{Type::Double}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::string s) noexcept
      : m_stringValue{std::move(s)}, m_content{&m_stringValue}, m_variantType{Type::String}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, EoDxfGeometryBase3d gb) noexcept
      : m_geometryBaseValue{gb}, m_content{&m_geometryBaseValue}, m_variantType{Type::GeometryBase}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(const EoDxfGroupCodeValuesVariant& other)
      : m_stringValue{other.m_stringValue},
        m_geometryBaseValue{other.m_geometryBaseValue},
        m_content{other.m_content},
        m_variantType{other.m_variantType},
        m_code{other.m_code} {
    if (other.m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
    if (other.m_variantType == Type::String) { m_content.s = &m_stringValue; }
  }

  EoDxfGroupCodeValuesVariant& operator=(const EoDxfGroupCodeValuesVariant& other) {
    if (this != &other) {
      m_stringValue = other.m_stringValue;
      m_geometryBaseValue = other.m_geometryBaseValue;
      m_content = other.m_content;
      m_variantType = other.m_variantType;
      m_code = other.m_code;
      if (other.m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
      if (other.m_variantType == Type::String) { m_content.s = &m_stringValue; }
    }
    return *this;
  }

  EoDxfGroupCodeValuesVariant(EoDxfGroupCodeValuesVariant&& other) noexcept
      : m_stringValue{std::move(other.m_stringValue)},
        m_geometryBaseValue{other.m_geometryBaseValue},
        m_content{other.m_content},
        m_variantType{other.m_variantType},
        m_code{other.m_code} {
    if (m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
    if (m_variantType == Type::String) { m_content.s = &m_stringValue; }
  }

  EoDxfGroupCodeValuesVariant& operator=(EoDxfGroupCodeValuesVariant&& other) noexcept {
    if (this != &other) {
      m_stringValue = std::move(other.m_stringValue);
      m_geometryBaseValue = other.m_geometryBaseValue;
      m_content = other.m_content;
      m_variantType = other.m_variantType;
      m_code = other.m_code;
      if (m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
      if (m_variantType == Type::String) { m_content.s = &m_stringValue; }
    }
    return *this;
  }

  ~EoDxfGroupCodeValuesVariant() = default;

  void AddString(int code, std::string s) {
    m_variantType = Type::String;
    m_stringValue = std::move(s);
    m_content.s = &m_stringValue;
    m_code = code;
  }
  void AddInt16(int code, std::int16_t i16) {
    m_variantType = Type::Int16;
    m_content.i16 = i16;
    m_code = code;
  }
  void AddInteger(int code, int i) {
    m_variantType = Type::Integer;
    m_content.i = i;
    m_code = code;
  }
  void AddDouble(int code, double d) {
    m_variantType = Type::Double;
    m_content.d = d;
    m_code = code;
  }
  void AddGeometryBase(int code, EoDxfGeometryBase3d v) {
    m_variantType = Type::GeometryBase;
    m_geometryBaseValue = v;
    m_content.v = &m_geometryBaseValue;
    m_code = code;
  }
  void AddHandle(int code, std::uint64_t h) {
    m_variantType = Type::Handle;
    m_content.h = h;
    m_code = code;
  }

  void SetGeometryBaseX(double d) {
    assert(m_variantType == Type::GeometryBase);
    m_geometryBaseValue.x = d;
  }
  void SetGeometryBaseY(double d) {
    assert(m_variantType == Type::GeometryBase);
    m_geometryBaseValue.y = d;
  }
  void SetGeometryBaseZ(double d) {
    assert(m_variantType == Type::GeometryBase);
    m_geometryBaseValue.z = d;
  }

  [[nodiscard]] const std::string& GetString() const {
    assert(m_variantType == Type::String);
    return m_stringValue;
  }
  [[nodiscard]] std::int16_t GetInt16() const {
    assert(m_variantType == Type::Int16);
    return m_content.i16;
  }
  [[nodiscard]] std::int32_t GetInteger() const {
    assert(m_variantType == Type::Integer);
    return m_content.i;
  }
  [[nodiscard]] double GetDouble() const {
    assert(m_variantType == Type::Double);
    return m_content.d;
  }
  [[nodiscard]] const EoDxfGeometryBase3d& GetGeometryBase() const {
    assert(m_variantType == Type::GeometryBase);
    return m_geometryBaseValue;
  }
  [[nodiscard]] std::uint64_t GetHandle() const {
    assert(m_variantType == Type::Handle);
    return m_content.h;
  }

  [[nodiscard]] enum Type GetType() const noexcept { return m_variantType; }
  [[nodiscard]] int Code() const noexcept { return m_code; }

 private:
  std::string m_stringValue;
  EoDxfGeometryBase3d m_geometryBaseValue;

  union EoDxfVariantContent {
    std::string* s;
    std::int16_t i16;  ///< 16-bit signed integer (group codes 60-79, 170-179, 270-289, etc.)
    std::int32_t i;
    std::uint64_t h;  ///< handle stored as 64-bit unsigned (parsed from hex string)
    double d;
    EoDxfGeometryBase3d* v;

    EoDxfVariantContent() noexcept : i{} {}
    EoDxfVariantContent(std::string* sd) noexcept : s{sd} {}
    EoDxfVariantContent(std::int16_t i16d) noexcept : i16{i16d} {}
    EoDxfVariantContent(std::int32_t id) noexcept : i{id} {}
    EoDxfVariantContent(std::uint64_t hd) noexcept : h{hd} {}
    EoDxfVariantContent(double dd) noexcept : d{dd} {}
    EoDxfVariantContent(EoDxfGeometryBase3d* gb) noexcept : v{gb} {}
  };

 public:
  EoDxfVariantContent m_content;

 private:
  enum EoDxfGroupCodeValuesVariant::Type m_variantType{Type::Invalid};
  int m_code{};
};

/** @brief Class to handle conversion between DXF line width codes and internal line width enumeration.
 *
 *  The EoDxfLineWidths class provides an enumeration of line widths corresponding to standard DXF line width codes, as
 * well as static methods to convert between the internal line width enumeration and the integer codes used in DXF
 * files. This allows for consistent handling of line widths when reading from or writing to DXF files, ensuring that
 * the correct line widths are applied based on the specified codes.
 */
class EoDxfLineWidths {
 public:
  enum lineWidth {
    width00 = 0,  // 0.00mm (dxf 0)
    width01 = 1,  // 0.05mm (dxf 5)
    width02 = 2,  // 0.09mm (dxf 9)
    width03 = 3,  // 0.13mm (dxf 13)
    width04 = 4,  // 0.15mm (dxf 15)
    width05 = 5,  // 0.18mm (dxf 18)
    width06 = 6,  // 0.20mm (dxf 20)
    width07 = 7,  // 0.25mm (dxf 25)
    width08 = 8,  // 0.30mm (dxf 30)
    width09 = 9,  // 0.35mm (dxf 35)
    width10 = 10,  // 0.40mm (dxf 40)
    width11 = 11,  // 0.50mm (dxf 50)
    width12 = 12,  // 0.53mm (dxf 53)
    width13 = 13,  // 0.60mm (dxf 60)
    width14 = 14,  // 0.70mm (dxf 70)
    width15 = 15,  // 0.80mm (dxf 80)
    width16 = 16,  // 0.90mm (dxf 90)
    width17 = 17,  // 1.00mm (dxf 100)
    width18 = 18,  // 1.06mm (dxf 106)
    width19 = 19,  // 1.20mm (dxf 120)
    width20 = 20,  // 1.40mm (dxf 140)
    width21 = 21,  // 1.58mm (dxf 158)
    width22 = 22,  // 2.00mm (dxf 200)
    width23 = 23,  // 2.11mm (dxf 211)
    widthByLayer = 29,  // by layer (dxf -1)
    widthByBlock = 30,  // by block (dxf -2)
    widthDefault = 31  // by default (dxf -3)
  };

  static int lineWidth2dxfInt(enum lineWidth lw) {
    switch (lw) {
      case widthByLayer:
        return -1;
      case widthByBlock:
        return -2;
      case widthDefault:
        return -3;
      case width00:
        return 0;
      case width01:
        return 5;
      case width02:
        return 9;
      case width03:
        return 13;
      case width04:
        return 15;
      case width05:
        return 18;
      case width06:
        return 20;
      case width07:
        return 25;
      case width08:
        return 30;
      case width09:
        return 35;
      case width10:
        return 40;
      case width11:
        return 50;
      case width12:
        return 53;
      case width13:
        return 60;
      case width14:
        return 70;
      case width15:
        return 80;
      case width16:
        return 90;
      case width17:
        return 100;
      case width18:
        return 106;
      case width19:
        return 120;
      case width20:
        return 140;
      case width21:
        return 158;
      case width22:
        return 200;
      case width23:
        return 211;
      default:
        break;
    }
    return -3;
  }

  static int lineWidth2dwgInt(enum lineWidth lw) { return static_cast<int>(lw); }

  static enum lineWidth dxfInt2lineWidth(int i) {
    if (i < 0) {
      if (i == -1) {
        return widthByLayer;
      } else if (i == -2) {
        return widthByBlock;
      } else if (i == -3) {
        return widthDefault;
      }
    } else if (i < 3) {
      return width00;
    } else if (i < 7) {
      return width01;
    } else if (i < 11) {
      return width02;
    } else if (i < 14) {
      return width03;
    } else if (i < 16) {
      return width04;
    } else if (i < 19) {
      return width05;
    } else if (i < 22) {
      return width06;
    } else if (i < 27) {
      return width07;
    } else if (i < 32) {
      return width08;
    } else if (i < 37) {
      return width09;
    } else if (i < 45) {
      return width10;
    } else if (i < 52) {
      return width11;
    } else if (i < 57) {
      return width12;
    } else if (i < 65) {
      return width13;
    } else if (i < 75) {
      return width14;
    } else if (i < 85) {
      return width15;
    } else if (i < 95) {
      return width16;
    } else if (i < 103) {
      return width17;
    } else if (i < 112) {
      return width18;
    } else if (i < 130) {
      return width19;
    } else if (i < 149) {
      return width20;
    } else if (i < 180) {
      return width21;
    } else if (i < 205) {
      return width22;
    } else {
      return width23;
    }
    // default by default
    return widthDefault;
  }
};