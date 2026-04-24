#pragma once

#include <cstdint>

#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

class EoDbPrimitive;

/** * @file EoDb.h
 * @brief Defines the EoDb namespace, which contains enumerations and functions for reading and writing PEG files.
 *
 * The EoDb namespace includes:
 * - Enumerations for PEG file versions, file types, primitive types, update view hints, sentinels, polygon styles,
 *   paths, horizontal and vertical alignments, and precision.
 * - Functions for reading various data types from a CFile object, including strings, numbers, primitives, and custom
 *   enumerations.
 * - Functions for writing various data types to a CFile object, including strings, numbers, and custom enumerations.
 *
 * This header is intended to be included in other parts of the application that need to read from or write to PEG
 * files, as well as those that need to work with the defined enumerations.
 */
namespace EoDb {

enum class PegFileVersion : std::uint16_t { AE2011 = 0, AE2026 = 1 };
enum class FileTypes {
  Dwg = 0x00,
  Dxf = 0x01,
  Dxb = 0x02,
  Peg11 = 0x11,
  Peg = 0x20,
  Job = 0x21,
  Tracing = 0x22,
  Unknown = -1
};

enum PrimitiveTypes {
  kPointPrimitive = 0x0100,
  kInsertPrimitive = 0x0101,
  kGroupReferencePrimitive = 0x0102,
  kLinePrimitive = 0x0200,
  kPolygonPrimitive = 0x0400,
  kFacePrimitive = 0x0500,
  kEllipsePrimitive = 0x1003,  // @deprecated Use kConicPrimitive
  kConicPrimitive = 0x1004,
  kSplinePrimitive = 0x2000,
  kCSplinePrimitive = 0x2001,
  kPolylinePrimitive = 0x2002,
  kTextPrimitive = 0x4000,
  kAttribPrimitive = 0x4001,
  kTagPrimitive = 0x4100,
  kDimensionPrimitive = 0x4200,
  kViewportPrimitive = 0x8000
};
enum UpdateViewHints {
  kPrimitive = 0x0001,
  kGroup = 0x0002,
  kGroups = 0x0004,
  kLayer = 0x0008,
  kSafe = 0x0100,
  kPrimitiveSafe = kPrimitive | kSafe,
  kGroupSafe = kGroup | kSafe,
  kGroupsSafe = kGroups | kSafe,
  kLayerSafe = kLayer | kSafe,
  kErase = 0x0200,
  kLayerErase = kLayer | kErase,
  kPrimitiveEraseSafe = kPrimitive | kErase | kSafe,
  kGroupEraseSafe = kGroup | kErase | kSafe,
  kTrap = 0x0400,
  kGroupsTrap = kGroups | kTrap,
  kGroupSafeTrap = kGroup | kSafe | kTrap,
  kGroupsSafeTrap = kGroups | kSafe | kTrap,
  kGroupEraseSafeTrap = kGroup | kErase | kSafe | kTrap,
  kGroupsEraseSafeTrap = kGroups | kErase | kSafe | kTrap
};
enum Sentinels {
  kHeaderSection = 0x0101,
  kTablesSection = 0x0102,
  kBlocksSection = 0x0103,
  kGroupsSection = 0x0104,
  kPaperSpaceSection = 0x0105,
  kMultiLayoutPaperSpaceSection = 0x0106,
  kEndOfSection = 0x01ff,

  kViewPortTable = 0x0201,
  kLinetypeTable = 0x0202,
  kLayerTable = 0x0203,
  kTextStyleTable = 0x0204,
  kLayoutTable = 0x0205,
  kEndOfTable = 0x02ff
};
enum class PolygonStyle { Special = -1, Hollow, Solid, Pattern, Hatch };
enum class Path : std::uint16_t { Right, Left, Up, Down };
enum class HorizontalAlignment : std::uint16_t { Left = 1, Center, Right };
enum class VerticalAlignment : std::uint16_t { Top = 2, Middle, Bottom };
enum class Precision : std::uint16_t { TrueType = 1, StrokeType };

void Read(CFile& file, CString& string);
void Read(CFile& file, double& number);
bool Read(CFile& file, EoDbPrimitive*& primitive, PegFileVersion fileVersion = PegFileVersion::AE2011);
void Read(CFile& file, std::int16_t& number);
void Read(CFile& file, std::uint16_t& number);

inline void Read(CFile& file, EoDb::Path& path) {
  std::uint16_t number;
  Read(file, number);
  path = static_cast<EoDb::Path>(number);
}

inline void Read(CFile& file, EoDb::Precision& precision) {
  std::uint16_t number;
  Read(file, number);
  precision = static_cast<EoDb::Precision>(number);
}

inline void Read(CFile& file, EoDb::HorizontalAlignment& horizontalAlignment) {
  std::uint16_t number;
  Read(file, number);
  horizontalAlignment = static_cast<EoDb::HorizontalAlignment>(number);
}

inline void Read(CFile& file, EoDb::VerticalAlignment& verticalAlignment) {
  std::uint16_t number;
  Read(file, number);
  verticalAlignment = static_cast<EoDb::VerticalAlignment>(number);
}

[[nodiscard]] double ReadDouble(CFile& file);
[[nodiscard]] std::int8_t ReadInt8(CFile& file);
[[nodiscard]] std::int16_t ReadInt16(CFile& file);
[[nodiscard]] std::int32_t ReadInt32(CFile& file);
[[nodiscard]] EoGePoint3d ReadPoint3d(CFile& file);
[[nodiscard]] EoGeVector3d ReadVector3d(CFile& file);
[[nodiscard]] std::uint16_t ReadUInt16(CFile& file);
[[nodiscard]] std::uint64_t ReadUInt64(CFile& file);

void Write(CFile& file, const CString& string, UINT codePage = CP_ACP);
void WriteDouble(CFile& file, double number);
void WriteInt8(CFile& file, std::int8_t number);
void WriteInt16(CFile& file, std::int16_t number);
void WriteInt32(CFile& file, std::int32_t number);
void WriteUInt16(CFile& file, std::uint16_t number);
void WriteUInt64(CFile& file, std::uint64_t number);

inline void Write(CFile& file, EoDb::Path path) { WriteUInt16(file, static_cast<std::uint16_t>(path)); }

inline void Write(CFile& file, EoDb::Precision precision) { WriteUInt16(file, static_cast<std::uint16_t>(precision)); }

inline void Write(CFile& file, EoDb::HorizontalAlignment horizontalAlignment) {
  WriteUInt16(file, static_cast<std::uint16_t>(horizontalAlignment));
}

inline void Write(CFile& file, EoDb::VerticalAlignment verticalAlignment) {
  WriteUInt16(file, static_cast<std::uint16_t>(verticalAlignment));
}

}  // namespace EoDb
