#pragma once

/// <remarks>
/// There a two versions of this file type. The origins, and number of these files produced has forced the 
/// maintenance of the vax float format. The oldest version did not have a header component, so the first group 
/// begins at offset 0. This version was probably the version used when there was linkage to a separate 
/// "sheet file" and all names were 4 letters which defined the directory tree. The second version (3) had
/// a header which contained a copy of the information displayed in the sheet file, but none of its information 
/// is valid now. Some version 3 files will have a version stamp in bytes [4-5] ([4] is the general tag for type;
/// 'T' for tracing and [5] is the version tag; 'c' for 3. etc.)
///
/// Group (version 1)
///		Offset to next group (not valid now)	EoUInt16	[0-1]   Note: [0-3] provide group information only and are
///																	used only once per group. This means on each 
///																	primitive after the first [0-3] are unused.
///		Number of primitives					EoUInt16	[2-3]			
///
/// Note: After looking at incomplete source from original fortran code I now believe the arc primitive was replaced by
/// the conic primitive in version 1. This was as early as summer 89. I do not have enough information to determine if
/// 1 or 2 32 byte chunks were used. If the z-coordinate for the center point and begin point was used then it would be
/// 2. The early decision based on how much could be placed in a 32 byte chunk would make me think the z-coorditate of one
/// of the points was not used. This would allow the primitive to fit in one chunk. The other choice would have
/// required 33 bytes which would have wasted 31 bytes. Back then this was major. Another oddity is byte [7] is not
/// used. I think this is because of the structure alignment requirements. I am just guessing that the sweep angle used
/// the bytes of the z-coordinate of the center point instead of the begin point.
///	Previous Thinking
///	{
///		Arc primitive (version 1)
///			Pen color							EoByte		[4](0:4)
///			Line type							EoByte		[4](5:8)							
///			Type code {0x3d} 0011 1101			EoByte		[5]
///			Number of 32 byte chunks {1}		EoByte		[6]
///			Begin point							vaxfloat[2]	[8-11][12-15]
///			??												[16-19]
///			Center point						vaxfloat[2]	[20-23][24-27]
///			Sweep angle							vaxfloat	[28-31]
///	}
/// Current Thinking
///	{
///		Arc primitive (version 1 only)		
///			Pen color							EoByte		[4](0:4)
///			Line type							EoByte		[4](5:8)
///			Type code {0x3d} 0011 1101			EoByte		[5]
///			Number of 32 byte chunks {2?}		EoByte		[6]
///			Center point						vaxfloat[3]	[8-11][12-15]
///			Sweep angle							vaxfloat	[16-19]
///			Begin point							vaxfloat[3]	[20-23][24-27][28-31]
///
///		Conic primitive (version 1 - almost exactly the same as version 3)
///			Pen color							EoByte		[4](0:4)
///			Line type							EoByte		[4](5:8)
///			Type code {0x21} 0010 0001			EoByte		[5]
///			Number of 32 byte chunks {2}		EoByte		[6]
///			Center point						vaxfloat[3]	[8-11][12-15][16-19]
///			Major axis							vaxfloat[3]	[20-23][24-27][28-31]
///			Minor axis							vaxfloat[3]	[32-35][36-39][40-43]
///			Sweep angle							vaxfloat	[44-47]
///	}	
///		BSpline primitive (version 1)
///			Pen color							EoByte		[4](0:4)
///			Line type							EoByte		[4](5:8)							
///			Type code {0x18} 0001 1000			EoByte		[5]
///			Number of 32 byte chunks			EoByte		[6]
///			Number of control points			vaxfloat	[8-11]
///			{0 or more control points}			vaxfloat[3] [12-15][16-19][20-23]...
///		Line primitive (version 1)
///			Pen color							EoByte		[4](0:4)
///			Line type							EoByte		[4](5:8)							
///			Type code {0x43} 0100 0011			EoByte		[5]
///			Number of 32 byte chunks {1}		EoByte		[6]
///			Begin point							vaxfloat[3]	[8-11][12-15][16-19]
///			End point							vaxfloat[3]	[20-23][24-27][28-31]
///		Point primitive (version 1)
///			Pen color							EoByte		[4](0:4)
///			Line type							EoByte		[4](5:8)							
///			Type code {0x46} 0100 0110			EoByte		[5]
///			Number of 32 byte chunks {1}		EoByte		[6]
///			Point								vaxfloat[3]	[8-11][12-15][16-19]
///			{3 data values}						vaxfloat[3]	[20-23][24-27][28-31]
///		Polygon primitive (version 1)
///			Pen color							EoByte		[4](0:4)
///			Type code {0x64} 0110 0100			EoByte		[5]
///			Number of 32 byte chunks			EoByte		[6]
///			Number of verticies					vaxfloat	[8-11]
///			style & index ??					vaxfloat	[12-15]
///			Hatch
///				x scale factor					vaxfloat	[16-19]
///				y scale factor					vaxfloat	[20-23]
///				rotation angle					vaxfloat	[24-27]
///			??												[28-35]
///			{0 or more points}					vaxfloat[3]	[36-39][40-43][44-47]...
///		Text primitive (version 1)
///			Pen color							EoByte		[4](0:4)
///			Type code {0x11} 0001 0001			EoByte		[5]
///			Number of 32 byte chunks			EoByte		[6]
///			Insertion point						vaxfloat[3]	[8-11][12-15][16-19]
///			Character height					vaxfloat	[20-23]
///			Character expansion factor			vaxfloat	[24-27]
///			Character rotations angle			vaxfloat	[28-31]
///			??												[32-35]
///			Character spacing					vaxfloat	[36-39]
///			path,halign,valign					vaxfloat	[40-43]
///			Text ('\' terminated)				EoByte[]	[44]...
/// Group (version 3)
///		Offset to next group (not valid now)	EoByte		[0]		Note: [0-2] provide group information only and are used only once per group.
///		Number of primitives					EoUInt16	[1-2]	This means on each primitive after the first [0-2] are unused.
///		Ellipse primitive (version 3)
///			Number of 32 byte chunks {2}		EoByte		[3]
///			Type code {0x1003}					EoUInt16    [4-5]
///			Pen color							EoByte		[6]
///			Line type							EoByte		[7]							
///			Center point						vaxfloat[3]	[8-11][12-15][16-19]
///			Major axis							vaxfloat[3]	[20-23][24-27][28-31]
///			Minor axis							vaxfloat[3]	[32-35][36-39][40-43]
///			Sweep angle							vaxfloat	[44-47]
///		BSpline primitive (version 3)
///			Number of 32 byte chunks			EoByte		[3]
///				{(2 + nPts * 3) / 8 + 1}
///			Type code {0x2000}					EoUInt16	[4-5]
///			Pen color							EoByte		[6]
///			Line type							EoByte		[7]
///			Number of control points			EoUInt16	[8-9]
///			{0 or more control points}			vaxfloat[3] [10-13][14-17][18-21]
///		CSpline primitive (version 3 only)							Note: This primitive may still exist in some files and is readable, but is
///			Number of 32 byte chunks			EoByte		[3]		converted on the read to a BSpline primitive and is never written to file.
///				{(69 + nPts * 12) / 32}
///			Type code {0x2001}					EoUInt16	[4-5]
///			Pen color							EoByte		[6]
///			Line type							EoByte		[7]
///			??									EoUInt16	[8-9]
///			Number of control points			EoUInt16	[10-11]
///			End condition Type					EoUInt16	[12-13]
///			Begin point tangent vector			vaxfloat[3]	[14-17][18-21][22-25]
///			End point tangent vector			vaxfloat[3]	[26-29][30-33][34-37]
///			{0 or more control points}			vaxfloat[3] [38-41][42-45][46-49]
///		Dim primitive (version 3 only)
///			Number of 32 byte chunks			EoByte		[3]								
///				{(118 + nLen) / 32}
///			Type code {0x4200}					EoUInt16 	[4-5]
///			Line pen color						EoByte		[6]
///			Line line type						EoByte		[7]
///			Begin point							vaxfloat[3]	[8-11][12-15][16-19]
///			End point							vaxfloat[3]	[20-23][24-27][28-31]
///			Text pen color						EoByte		[32]
///			Text precision						EoByte		[33]
///			Text font	{always 0 for simplex}	EoUInt16	[34-35]
///			Character spacing					vaxfloat	[36-39]
///			Text path							EoByte		[40]
///			Horizontal alignment				EoByte		[41]
///			Vertical alignment					EoByte		[42]
///			Insertion point						vaxfloat[3]	[43-46][47-50][51-54]
///			Local reference x-axis				vaxfloat[3]	[55-58][59-62][63-66]
///			Local reference y-axis				vaxfloat[3]	[67-70][71-74][75-78]
///			Number of characters				EoUInt16	[79-80]
///			Text								EoByte[]	[81..]
///		Line primitive (version 3)
///			Number of 32 byte chunks {1}		EoByte		[3]
///			Type code {0x0200}					EoUInt16	[4-5]
///			Pen color							EoByte		[6]
///			Line type							EoByte		[7]
///			Begin point							vaxfloat[3]	[8-11][12-15][16-19]
///			End point							vaxfloat[3]	[20-23][24-27][28-31]
///		Point primitive (version 3)
///			Number of 32 byte chunks {1}		EoByte		[3]
///			Type code {0x0100}					EoUInt16	[4-5]
///			Pen color							EoByte		[6]
///			Point style							EoByte		[7]
///			Point								vaxfloat[3]	[8-11][12-15][16-19]
///			{3 data values}						vaxfloat[3]	[20-23][24-27][28-31]
///		Polygon primitive (version 3)
///			Number of 32 byte chunks			EoByte		[3]					
///				{(79 + nPts * 12) / 32}
///			Type code {0x0400}					EoUInt16	[4-5]
///			Pen color							EoByte		[6]
///			Polygon style						EoByte		[7]
///			Polygon Style Index					EoUInt16	[8-9]
///			Number of verticies					EoUInt16	[10-11]
///			Hatch origin						vaxfloat[3]	[12-15][16-19][20-23]				
///			Hatch/pattern reference x-axis		vaxfloat[3]	[24-27][28-31][32-35]
///			Hatch/pattern reference y-axis		vaxfloat[3]	[36-39][40-43][44-47]
///			{0 or more points}					vaxfloat[3]	[48-51][52-55][56-59]
///		Tag primitive (version 3 only)								Note: This primitive may still exist in some files and is readable, but is
///			Number of 32 byte chunks {1}		EoByte		[3]		converted on the read to a Point primitive and is never written to file.
///			Type code {0x4100}					EoUInt16	[4-5]
///			Pen color							EoByte		[6]
///			Line type							EoByte		[7]
///			Point								Pnt			[8-11][12-15][16-19]
///			Unused 								??			[20-31]
///		Text primitive (version 3)
///			Number of 32 byte chunks			EoByte		[3]								
///				{(86 + nLen) / 32}
///			Type code {0x4000}					EoUInt16	[4-5]
///			Pen color							EoByte		[6]
///			Text precision						EoByte		[7]
///			Text font	{always 0 for simplex}	EoUInt16	[8-9]
///			Character spacing					vaxfloat	[10-13]
///			Text path							EoByte		[14]
///			Horizontal alignment				EoByte		[15]
///			Vertical alignment					EoByte		[16]
///			Insertion point						vaxfloat[3]	[17-20][21-24][25-28]
///			Local reference x-axis				vaxfloat[3]	[29-32][33-36][37-40]
///			Local reference y-axis				vaxfloat[3]	[41-44][45-48][49-52]
///			Number of characters				EoUInt16	[53-54]
///			Text								EoByte[]	[55]...
/// </remarks>

class CFileJob
{
private:
	int m_Version;
	EoByte*	m_PrimBuf;
	
public:
	CFileJob()
	{
		m_Version = 3;
		m_PrimBuf = new EoByte[EoDbPrimitive::BUFFER_SIZE];
	}
	virtual ~CFileJob()
	{
		delete [] m_PrimBuf;
	}
	/// <summary>Reads document data from a memory file and adds all groups to the trap with a translation. This is a data stream retrieved from the clipboard.</summary>
	void ReadMemFile(CFile& file, EoGeVector3d translateVector);
	void ReadHeader(CFile& file);
	void ReadLayer(CFile& file, EoDbLayer* layer);

	bool GetNextVisibleGroup(CFile& file, EoDbGroup*& group);
	bool GetNextPrimitive(CFile& file, EoDbPrimitive*& primitve);
	bool ReadNextPrimitive(CFile &file, EoByte *buffer, EoInt16& primitiveType);

	int Version();
	static bool IsValidPrimitive(EoInt16 primitiveType);
	static bool IsValidVersion1Primitive(EoInt16 primitiveType);
	
	void WriteHeader(CFile& file);
	void WriteLayer(CFile& file, EoDbLayer* layer);
	void WriteGroup(CFile& file, EoDbGroup* group);
	void ConstructPrimitive(EoDbPrimitive *&primitive, EoInt16 PrimitiveType);
	void ConstructPrimitiveFromVersion1(EoDbPrimitive *&primitive);
	
	EoDbPrimitive* ConvertEllipsePrimitive();
	EoDbPrimitive* ConvertLinePrimitive();
	EoDbPrimitive* ConvertPointPrimitive();
	
	EoDbPrimitive* ConvertVersion1EllipsePrimitive();
	EoDbPrimitive* ConvertVersion1LinePrimitive();
	EoDbPrimitive* ConvertVersion1PointPrimitive();

	/// <summary> Converts a deprecated version 1 CSpline to a BSpline</summary>
	void ConvertCSplineToBSpline();
	void ConvertTagToPoint();
};
