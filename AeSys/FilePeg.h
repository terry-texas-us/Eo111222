#pragma once

class AeSysDoc;

class CFilePeg : public CFile
{
public:
	CFilePeg() 
	{
	};
	virtual ~CFilePeg() 
	{
	}
	void Load(AeSysDoc* document);
	void ReadBlocksSection(AeSysDoc* document);
	void ReadEntitiesSection(AeSysDoc* document);
	void ReadHeaderSection(AeSysDoc* document);
	void ReadLayerTable(AeSysDoc* document);
	void ReadLinetypesTable(AeSysDoc* document);
	void ReadLinetypeDefinition(double* dashLength, CString& name, CString& description, EoUInt16& definitionLength);
	void ReadTablesSection(AeSysDoc* document);
	void ReadViewportTable(AeSysDoc* document);
	void Unload(AeSysDoc* document);
	void WriteBlocksSection(AeSysDoc* document);
	void WriteEntitiesSection(AeSysDoc* document);
	void WriteHeaderSection(AeSysDoc* document);
	void WriteLayerTable(AeSysDoc* document);
	void WriteLinetypeTable(AeSysDoc* document);
	void WriteTablesSection(AeSysDoc* document);
	void WriteVPortTable(AeSysDoc* document);
};

// EoDb::kHeaderSection sentinel				EoUInt16 0x0101
//	{0 or more key-value pairs}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// EoDb::kTablesSection sentinel				EoUInt16 0x0102
//	EoDb::kViewPortTable sentinel				EoUInt16 0x0201
//	 {0 or more viewport definitions}			EoUInt16
//	EoDb::kEndOfTable sentinel					EoUInt16 0x02ff
//	EoDb::kLinetypeTable sentinel				EoUInt16 0x0202
//	 Number of linetypes						EoUInt16
//	  {0 or more linetype definitions}
//	EoDb::kEndOfTable sentinel					EoUInt16 0x02ff
//	EoDb::kLayerTable sentinel					EoUInt16 0x0203
//	 Number of layers (resident only)			EoUInt16
//	  {0 or more layer definitions}
//	EoDb::kEndOfTable sentinel					EoUInt16 0x02ff
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// EoDb::kBlocksSection sentinel				EoUInt16 0x0103
//	Number of block definitions					EoUInt16
//	 {0 or more block definitions}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// EoDb::kGroupsSection sentinel				EoUInt16 0x0104
//	 Number of groups definitions				EoUInt16
//	  {0 or more groups definitions}
// EoDb::kEndOfSection sentinel					EoUInt16 0x01ff
// Layer definition
//		Layers
//			Name								string
//			Tracing flags						EoUInt16
//			State								EoUInt16
//			Layer pen color						EoUInt16
//			Layer line type						string
//		Tracings
//			?? nothing
// Line style definition
//		Name									string
//		Flags (always 0)						EoUInt16
//		Description								string
//		Definition length						EoUInt16		
//		Pattern length							double
//		if (definition length > 0)
//			1 or more dash length				double...
// Block definition
//		Number of primitives					EoUInt16
//		Name									string
//		Flags									EoUInt16
//		Base point								EoGePoint3d
//		{0 or more primitive definitions}
// Groups definition
//		Number of Group definition				EoUInt16
//		{0 or more group definitions}
// Group definition
//		Number of primitive definitions			EoUInt16
//		{0 or more primitive definitions}
// Primitive definition
//		Point primitive
//			Type code <0x0100>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Point style							EoUInt16	[4-5]
//			Point								EoGePoint3d	[6-9][10-13][14-17]
//			Number of data values				EoUInt16	[18-19]
//			{0 or more data values}				double		[20-]
//		Insert primitive
//			Type code <0x0101>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Block name							string
//			Insertion point						EoGePoint3d	
//			Local reference x-axis				EoGeVector3d
//			Local reference y-axis				EoGeVector3d
//			Local reference z-axis				EoGeVector3d
//			Number of columns					EoUInt16
//			Number of rows						EoUInt16
//			Column spacing						double
//			Row spacing							double
//		SegRef primitive
//			Type code <0x0102>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Group name							string
//			Insertion point						EoGePoint3d
//			Local normal vector					EoGeVector3d
//			Scale factors(x, y, z)				EoGeVector3d
//			Rotatation							double
//			Number of columns					EoUInt16
//			Number of rows						EoUInt16
//			Column spacing						double
//			Row spacing							double
//		Line primitive
//			Type code <0x0200>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Line.begin							EoGePoint3d	[6-9][10-13][14-17]
//			Line.end							EoGePoint3d	[18-21][22-25][26-29]
//		Polygon primitive
//			Type code <0x0400>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Polygon style						EoUInt16	[4-5]
//			Polygon Style Index					EoUInt16	[6-7]
//			Number of verticies					EoUInt16	[8-9]
//			Hatch origin						EoGePoint3d	[10-13][14-17][18-21]
//			Hatch/pattern reference x-axis		EoGeVector3d[22-25][26-29][30-33]
//			Hatch/pattern reference y-axis		EoGeVector3d[34-37][38-41][42-45]
//			{0 or more points}					EoGePoint3d	[46-]
//		Ellipse primitive
//			Type code <0x1003>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Center point						EoGePoint3d	[6-9][10-13][14-17]
//			Major axis							EoGeVector3d[18-21][22-25][26-29]
//			Minor axis							EoGeVector3d[30-33][34-37][38-41]
//			Sweep angle							double		[42-45]
//		Spline primitive
//			Type code <0x2000>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Number of control points			EoUInt16	[6-7]
//			{0 or more control points}			EoGePoint3d	[8-]
//		CSpline primitive											Note: This primitive may still exist in some files
//																	and is readable, but is converted on the read to a 
//																	Polyline primitive and is never written to file.
//			Type code <0x2001>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			m_wPtsS								EoUInt16	[6-7]
//			Number of control points			EoUInt16	[8-9]
//			End condition Type					EoUInt16	[10-11]
//			Begin point tangent vector			EoGeVector3d[12-15][16-19][20-23]
//			End point tangent vector			EoGeVector3d[24-27][28-31][32-35]
//			{0 or more control points}			EoGePoint3d	[36-]	
//	!!!	Polyline primitive (not ready for io: if already written flags not stremed)						
//			Type code <0x2002>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Flags								EoUInt16	[6-7]
//			Number of points					EoUInt16	[8-9]
//			{0 or more points}					EoGePoint3d	[10-]
//		Text primitive
//			Type code <0x4000>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Font definition
//			 Text precision						EoUInt16	[6-7]
//			 Text font name						string		
//			 Text path							EoUInt16
//			 Horizontal alignment				EoUInt16
//			 Vertical alignment					EoUInt16
//			 Character spacing					double
//			Insertion point						EoGePoint3d
//			Local reference x-axis				EoGeVector3d
//			Local reference y-axis				EoGeVector3d
//			Text ('\t' terminated)				string
//		Tag primitive												Note: This primitive may still exist in some files
//																	and is readable, but is converted on the read to a 
//																	Point primitive and is never written to file.
//			Type code <0x4100>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Point style							EoUInt16	[4-5]
//			Point								EoGePoint3d	[6-9][10-13][14-17]
//		Dim primitive 
//			Type code <0x4200>					EoUInt16	[0-1]
//			Pen color							EoUInt16	[2-3]
//			Line type							EoUInt16	[4-5]
//			Line.begin							EoGePoint3d	[6-9][10-13][14-17]
//			Line.end							EoGePoint3d	[18-21][22-25][26-29]

//			Text pen color						EoUInt16	[30-31]
//			Font definition
//			 Text precision						EoUInt16	[32-33]
//			 Text font name						string		[34
//			 Text path							EoUInt16
//			 Horizontal alignment				EoUInt16
//			 Vertical alignment					EoUInt16
//			 Character spacing					double
//			Insertion point						EoGePoint3d
//			Local reference x-axis				EoGeVector3d
//			Local reference y-axis				EoGeVector3d
//			Text ('\t' terminated)				string