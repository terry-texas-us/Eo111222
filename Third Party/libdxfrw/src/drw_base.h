#pragma once

#pragma warning(disable : 4996)  // Ignore C4996, unsafe strncpy. TODO use safe alternative

constexpr auto DRW_VERSION = "0.6.3";

#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

#ifdef DRW_ASSERTS
#define drw_assert(a) assert(a)
#else
#define drw_assert(a)
#endif

#define UTF8STRING std::string
#define DRW_UNUSED(x) (void)x

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#define DRW_WIN
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define DRW_WIN
#elif defined(__MWERKS__) && defined(__INTEL__)
#define DRW_WIN
#else
#define DRW_POSIX
#endif

namespace DRW {
constexpr double geometricTolerance{1e-9};  // TAS: Added for geometric calculations, point coincidence etc.

constexpr auto Pi{std::numbers::pi};
constexpr auto TwoPi{2.0 * std::numbers::pi};
constexpr auto HalfPi{std::numbers::pi / 2.0};
constexpr auto ARAD{180.0 / std::numbers::pi};

/// Version numbers for the DXF Format.
enum Version {
  UNKNOWNV,  // UNKNOWN VERSION (default / unreadable header)
  AC1006,    // R10
  AC1009,    // R11 & R12
  AC1012,    // R13
  AC1014,    // R14
  AC1015,    // AutoCAD 2000 / 2000i / 2002
  AC1018,    // AutoCAD 2004 / 2005 / 2006
  AC1021,    // AutoCAD 2007 / 2008 / 2009
  AC1024,    // AutoCAD 2010 / 2011 / 2012
  AC1027,    // AutoCAD 2013 / 2014 / 2015 / 2016 / 2017
  AC1032  // AutoCAD 2018 / 2019 / 2020 / 2021 / 2022 / 2023 / 2024 / 2025 / 2026 (current format – no new code since
          // 2018)
};

enum DebugTraceLevel { None, Debug };

//! Special codes for colors
enum ColorCodes { ColorByLayer = 256, ColorByBlock = 0 };

//! Spaces
enum Space { ModelSpace = 0, PaperSpace = 1 };

//! Special kinds of handles
enum HandleCodes { NoHandle = 0 };

//! Shadow mode
enum ShadowMode { CastAndReceiveShadows = 0, CastShadows = 1, ReceiveShadows = 2, IgnoreShadows = 3 };

//! Special kinds of materials
enum MaterialCodes { MaterialByLayer = 0 };

//! Special kinds of plot styles
enum PlotStyleCodes { DefaultPlotStyle = 0 };

//! Special kinds of transparencies
enum TransparencyCodes { Opaque = 0, Transparent = -1 };

}  // namespace DRW

/** @brief Class representing a generalized 3D coordinate (using for point and vector) with x, y, and z components.
 *  Provides constructors for initialization and a method to unitize the vector.
 */
class DRW_Coord {
 public:
  double x{};
  double y{};
  double z{};

  DRW_Coord() noexcept = default;

  DRW_Coord(double x, double y, double z) : x{x}, y{y}, z{z} {}

  DRW_Coord(const DRW_Coord& other) noexcept = default;
  DRW_Coord& operator=(const DRW_Coord& data) noexcept = default;

  DRW_Coord(DRW_Coord&& other) noexcept = default;
  DRW_Coord& operator=(DRW_Coord&& other) noexcept = default;

  /** @brief Convert `this` coordinate to a unit-length vector (in-place).
   */
  void unitize() noexcept {
    const double dist = sqrt(x * x + y * y + z * z);
    if (dist > DRW::geometricTolerance) {
      x = x / dist;
      y = y / dist;
      z = z / dist;
    }
  }

  /// @todo Non-modifying version of unitize() that returns a new DRW_Coord instead of modifying `this` in-place.
};

/**
 * @brief 2D vertex for DXF LWPOLYLINE/POLYLINE entities (group codes 10/20/40/41/42).
 *
 * All values are in the entity's OCS.
 */
struct DRW_Vertex2D {
  double x{};         // x coordinate, code 10
  double y{};         // y coordinate, code 20
  double stawidth{};  // Start width, code 40
  double endwidth{};  // End width, code 41
  double bulge{};     // bulge, code 42

  /** @brief Default-constructed vertex (all zero). */
  DRW_Vertex2D() noexcept = default;

  /** @brief Position + bulge constructor (widths stay zero).
   * Matches the most common usage in DXF parsers.
   */
  constexpr DRW_Vertex2D(double x, double y, double bulge = {}) noexcept
      : x{x}, y{y}, stawidth{}, endwidth{}, bulge{bulge} {}
};

//! Class to handle header vars
/*!
 *  Class to handle header vars
 */
class DRW_Variant {
 public:
  enum Type { String, Integer, Double, Coord, Invalid };
  // TODO: add INT64 support
  DRW_Variant() : sdata(std::string()), vdata(), content(0), vType(Type::Invalid), vCode(0) {}

  DRW_Variant(int c, std::int32_t i) : sdata(std::string()), vdata(), content(i), vType(Type::Integer), vCode(c) {}
  DRW_Variant(int c, std::uint32_t i)
      : sdata(std::string()), vdata(), content(static_cast<std::int32_t>(i)), vType(Type::Integer), vCode(c) {}

  DRW_Variant(int c, double d) : sdata(std::string()), vdata(), content(d), vType(Type::Double), vCode(c) {}
  DRW_Variant(int c, UTF8STRING s) : sdata(s), vdata(), content(&sdata), vType(Type::String), vCode(c) {}

  DRW_Variant(int c, DRW_Coord crd) : sdata(std::string()), vdata(crd), content(&vdata), vType(Type::Coord), vCode(c) {}

  DRW_Variant(const DRW_Variant& d)
      : sdata(d.sdata), vdata(d.vdata), content(d.content), vType(d.vType), vCode(d.vCode) {
    if (d.vType == Type::Coord) { content.v = &vdata; }
    if (d.vType == Type::String) { content.s = &sdata; }
  }

  ~DRW_Variant() {}

  void addString(int c, UTF8STRING s) {
    vType = Type::String;
    sdata = s;
    content.s = &sdata;
    vCode = c;
  }
  void addInt(int c, int i) {
    vType = Type::Integer;
    content.i = i;
    vCode = c;
  }
  void addDouble(int c, double d) {
    vType = Type::Double;
    content.d = d;
    vCode = c;
  }
  void addCoord(int c, DRW_Coord v) {
    vType = Type::Coord;
    vdata = v;
    content.v = &vdata;
    vCode = c;
  }
  void setCoordX(double d) {
    if (vType == DRW_Variant::Type::Coord) { vdata.x = d; }
  }
  void setCoordY(double d) {
    if (vType == DRW_Variant::Type::Coord) { vdata.y = d; }
  }
  void setCoordZ(double d) {
    if (vType == DRW_Variant::Type::Coord) { vdata.z = d; }
  }
  enum DRW_Variant::Type type() const { return vType; }
  int code() const { return vCode; }

 private:
  std::string sdata;
  DRW_Coord vdata;

 private:
  union DRW_VarContent {
    UTF8STRING* s;
    std::int32_t i;
    double d;
    DRW_Coord* v;

    DRW_VarContent(UTF8STRING* sd) : s(sd) {}
    DRW_VarContent(std::int32_t id) : i(id) {}
    DRW_VarContent(double dd) : d(dd) {}
    DRW_VarContent(DRW_Coord* vd) : v(vd) {}
  };

 public:
  DRW_VarContent content;

 private:
  enum DRW_Variant::Type vType;
  int vCode;
};

//! Class to convert between line width and integer
/*!
 *  Class to convert between line width and integer
 *  verifing valid values, if value is not valid
 *  returns widthDefault.
 */
class DRW_LW_Conv {
 public:
  enum lineWidth {
    width00 = 0,       /*!< 0.00mm (dxf 0)*/
    width01 = 1,       /*!< 0.05mm (dxf 5)*/
    width02 = 2,       /*!< 0.09mm (dxf 9)*/
    width03 = 3,       /*!< 0.13mm (dxf 13)*/
    width04 = 4,       /*!< 0.15mm (dxf 15)*/
    width05 = 5,       /*!< 0.18mm (dxf 18)*/
    width06 = 6,       /*!< 0.20mm (dxf 20)*/
    width07 = 7,       /*!< 0.25mm (dxf 25)*/
    width08 = 8,       /*!< 0.30mm (dxf 30)*/
    width09 = 9,       /*!< 0.35mm (dxf 35)*/
    width10 = 10,      /*!< 0.40mm (dxf 40)*/
    width11 = 11,      /*!< 0.50mm (dxf 50)*/
    width12 = 12,      /*!< 0.53mm (dxf 53)*/
    width13 = 13,      /*!< 0.60mm (dxf 60)*/
    width14 = 14,      /*!< 0.70mm (dxf 70)*/
    width15 = 15,      /*!< 0.80mm (dxf 80)*/
    width16 = 16,      /*!< 0.90mm (dxf 90)*/
    width17 = 17,      /*!< 1.00mm (dxf 100)*/
    width18 = 18,      /*!< 1.06mm (dxf 106)*/
    width19 = 19,      /*!< 1.20mm (dxf 120)*/
    width20 = 20,      /*!< 1.40mm (dxf 140)*/
    width21 = 21,      /*!< 1.58mm (dxf 158)*/
    width22 = 22,      /*!< 2.00mm (dxf 200)*/
    width23 = 23,      /*!< 2.11mm (dxf 211)*/
    widthByLayer = 29, /*!< by layer (dxf -1) */
    widthByBlock = 30, /*!< by block (dxf -2) */
    widthDefault = 31  /*!< by default (dxf -3) */
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
      if (i == -1)
        return widthByLayer;
      else if (i == -2)
        return widthByBlock;
      else if (i == -3)
        return widthDefault;
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

  static enum lineWidth dwgInt2lineWidth(int i) {
    if ((i > -1 && i < 24) || (i > 28 && i < 32)) { return static_cast<lineWidth>(i); }
    // default by default
    return widthDefault;
  }
};