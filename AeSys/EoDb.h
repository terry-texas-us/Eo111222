#pragma once

#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

class EoDbPrimitive;

namespace EoDb {

enum class FileTypes { Dwg = 0x00, Dxf = 0x01, Dxb = 0x02, Peg = 0x20, Job = 0x21, Tracing = 0x22, Unknown = -1 };
enum PrimitiveTypes {
  kPointPrimitive = 0x0100,
  kInsertPrimitive = 0x0101,
  kGroupReferencePrimitive = 0x0102,
  kLinePrimitive = 0x0200,
  kPolygonPrimitive = 0x0400,
  kEllipsePrimitive = 0x1003,  // @deprecated Use kConicPrimitive
  kConicPrimitive = 0x1004,
  kSplinePrimitive = 0x2000,
  kCSplinePrimitive = 0x2001,
  kPolylinePrimitive = 0x2002,
  kTextPrimitive = 0x4000,
  kTagPrimitive = 0x4100,
  kDimensionPrimitive = 0x4200
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
  kEndOfSection = 0x01ff,

  kViewPortTable = 0x0201,
  kLinetypeTable = 0x0202,
  kLayerTable = 0x0203,
  kEndOfTable = 0x02ff
};
enum class PolygonStyle { Hollow, Solid, Pattern, Hatch, Special = -1 };
enum class Path : EoUInt16 { Right, Left, Up, Down };
enum class HorizontalAlignment : EoUInt16 { Left = 1, Center, Right };
enum class VerticalAlignment : EoUInt16 { Top = 2, Middle, Bottom };
enum class Precision : EoUInt16 { TrueType = 1, StrokeType };

void ConstructBlockReferencePrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructBlockReferencePrimitiveFromInsertPrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructConicPrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructDimensionPrimitive(CFile& file, EoDbPrimitive*& primitive);

void ConstructEllipsePrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructLinePrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructPointPrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructPointPrimitiveFromTagPrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructPolygonPrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructPolylinePrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructPolylinePrimitiveFromCSplinePrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructSplinePrimitive(CFile& file, EoDbPrimitive*& primitive);
void ConstructTextPrimitive(CFile& file, EoDbPrimitive*& primitive);

void Read(CFile& file, CString& string);
void Read(CFile& file, double& number);
bool Read(CFile& file, EoDbPrimitive*& primitive);
void Read(CFile& file, std::int16_t& number);
void Read(CFile& file, EoUInt16& number);

inline void Read(CFile& file, EoDb::Path& path) {
  EoUInt16 number;
  Read(file, number);
  path = static_cast<EoDb::Path>(number);
}

inline void Read(CFile& file, EoDb::Precision& precision) {
  EoUInt16 number;
  Read(file, number);
  precision = static_cast<EoDb::Precision>(number);
}

inline void Read(CFile& file, EoDb::HorizontalAlignment& horizontalAlignment) {
  EoUInt16 number;
  Read(file, number);
  horizontalAlignment = static_cast<EoDb::HorizontalAlignment>(number);
}

inline void Read(CFile& file, EoDb::VerticalAlignment& verticalAlignment) {
  EoUInt16 number;
  Read(file, number);
  verticalAlignment = static_cast<EoDb::VerticalAlignment>(number);
}

[[nodiscard]] double ReadDouble(CFile& file);
[[nodiscard]] std::int16_t ReadInt16(CFile& file);
[[nodiscard]] EoGePoint3d ReadPoint3d(CFile& file);
[[nodiscard]] EoGeVector3d ReadVector3d(CFile& file);
[[nodiscard]] EoUInt16 ReadUInt16(CFile& file);

void Write(CFile& file, const CString& string, UINT codePage = CP_ACP);
void Write(CFile& file, double number);
void Write(CFile& file, std::int16_t number);
void Write(CFile& file, EoUInt16 number);

inline void Write(CFile& file, EoDb::Path path) { Write(file, static_cast<EoUInt16>(path)); }

inline void Write(CFile& file, EoDb::Precision precision) { Write(file, static_cast<EoUInt16>(precision)); }

inline void Write(CFile& file, EoDb::HorizontalAlignment horizontalAlignment) {
  Write(file, static_cast<EoUInt16>(horizontalAlignment));
}

inline void Write(CFile& file, EoDb::VerticalAlignment verticalAlignment) {
  Write(file, static_cast<EoUInt16>(verticalAlignment));
}

}  // namespace EoDb
