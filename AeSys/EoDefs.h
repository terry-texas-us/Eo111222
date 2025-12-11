#pragma once

class EoDbPrimitive;

namespace EoDb
{
	enum FileTypes
	{
		kDwg = 0x00,
		kDxf = 0x01,
		kDxb = 0x02,
		kPeg = 0x20,
		kJob = 0x21,
		kTracing = 0x22,
		kUnknown = - 1
	};
	enum PolygonStyle 
	{
		kHollow, 
		kSolid, 
		kPattern, 
		kHatch
	};
	enum PrimitiveTypes
	{
		kPointPrimitive = 0x0100,
		kInsertPrimitive = 0x0101,
		kGroupReferencePrimitive = 0x0102,
		kLinePrimitive = 0x0200,
		kPolygonPrimitive = 0x0400,
		kEllipsePrimitive = 0x1003,
		kSplinePrimitive = 0x2000,
		kCSplinePrimitive = 0x2001,
		kPolylinePrimitive = 0x2002,
		kTextPrimitive = 0x4000,
		kTagPrimitive = 0x4100,
		kDimensionPrimitive = 0x4200
	};
	enum UpdateViewHints
	{
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
	enum Sentinals
	{
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
	enum Path
	{
		kPathRight,
		kPathLeft,
		kPathUp,
		kPathDown
	};
	enum HorizontalAlignment
	{
		kAlignLeft = 1,
		kAlignCenter,
		kAlignRight
	};
	enum VerticalAlignment
	{
		kAlignTop = 2,
		kAlignMiddle,
		kAlignBottom
	};
	enum Precision
	{
		kEoTrueType = 1,
		kStrokeType
	};

	void ConstructBlockReferencePrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructBlockReferencePrimitiveFromInsertPrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructDimensionPrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructEllipsePrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructLinePrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructPointPrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructPointPrimitiveFromTagPrimitive(CFile &file, EoDbPrimitive *&primitive);
	void ConstructPolygonPrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructPolylinePrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructPolylinePrimitiveFromCSplinePrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructSplinePrimitive(CFile& file, EoDbPrimitive*& primitive);
	void ConstructTextPrimitive(CFile& file, EoDbPrimitive*& primitive);

	void Read(CFile& file, CString& string);
	void Read(CFile& file, double& number);
	bool Read(CFile& file, EoDbPrimitive*& primitive);
	void Read(CFile& file, EoInt16& number);
	void Read(CFile& file, EoUInt16& number);
	double ReadDouble(CFile& file);
	EoInt16 ReadInt16(CFile& file);
	EoGePoint3d ReadPoint3d(CFile& file);
	EoGeVector3d ReadVector3d(CFile& file);
	EoUInt16 ReadUInt16(CFile& file);
	void Write(CFile& file, const CString& string);
	void Write(CFile& file, double number);
	void Write(CFile& file, EoInt16 number);
	void Write(CFile& file, EoUInt16 number);
}
