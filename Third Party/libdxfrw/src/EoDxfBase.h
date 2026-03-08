#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

namespace EoDxf {
constexpr double geometricTolerance{1e-9};  // TAS: Added for geometric calculations, point coincidence etc.

constexpr auto Pi{std::numbers::pi};
constexpr auto TwoPi{2.0 * std::numbers::pi};
constexpr auto HalfPi{std::numbers::pi / 2.0};
constexpr auto RadiansToDegrees{180.0 / std::numbers::pi};
constexpr auto DegreesToRadians{std::numbers::pi / 180.0};

// Version numbers for the DXF Format.
enum Version {
  UNKNOWNV,  // UNKNOWN VERSION (default / unreadable header)
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

// Special codes for colors
enum ColorCodes { ColorByLayer = 256, ColorByBlock = 0 };

// Spaces
enum Space { ModelSpace = 0, PaperSpace = 1 };

// Special kinds of handles
enum HandleCodes { NoHandle = 0 };

// Shadow mode
enum ShadowMode { CastAndReceiveShadows = 0, CastShadows = 1, ReceiveShadows = 2, IgnoreShadows = 3 };

// Special kinds of materials
enum MaterialCodes { MaterialByLayer = 0 };

// Special kinds of plot styles
enum PlotStyleCodes { DefaultPlotStyle = 0 };

// Special kinds of transparencies
enum TransparencyCodes { Opaque = 0, Transparent = -1 };

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

  EoDxfGeometryBase3d(const EoDxfGeometryBase3d& other) noexcept = default;
  EoDxfGeometryBase3d& operator=(const EoDxfGeometryBase3d& data) noexcept = default;

  EoDxfGeometryBase3d(EoDxfGeometryBase3d&& other) noexcept = default;
  EoDxfGeometryBase3d& operator=(EoDxfGeometryBase3d&& other) noexcept = default;

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

  /// @todo Non-modifying version of unitize() that returns a new EoDxfGeometryBase3d instead of modifying `this`
  /// in-place.
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
    Int16,       ///< 16-bit signed integer (DXF group codes 60-79, 170-179, 270-289, 370-389, 400-409, 1060-1070)
    Integer,     ///< 32-bit signed integer (DXF group codes 90-99, 420-429, 440-449, 450-459, 1071)
    Double,
    GeometryBase,
    Handle,      ///< 64-bit unsigned handle (DXF group codes 5, 105, 310-369 handle references)
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
    std::int16_t i16;     ///< 16-bit signed integer (group codes 60-79, 170-179, 270-289, etc.)
    std::int32_t i;
    std::uint64_t h;      ///< handle stored as 64-bit unsigned (parsed from hex string)
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
 *  The DRW_LW_Conv class provides an enumeration of line widths corresponding to standard DXF line width codes, as well
 * as static methods to convert between the internal line width enumeration and the integer codes used in DXF files.
 * This allows for consistent handling of line widths when reading from or writing to DXF files, ensuring that the
 * correct line widths are applied based on the specified codes.
 */
class DRW_LW_Conv {
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