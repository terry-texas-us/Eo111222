There are two versions of this file type. The origins, and number of these files produced has forced the maintenance of the vax float format. The oldest version did not have a header component, so the first group begins at offset 0. 
This version was probably the version used when there was linkage to a separate "sheet file" and all names were 4 letters which defined the directory tree.
The second version (3) had a header which contained a copy of the information displayed in the sheet file, but none of its information is valid now. 
Some version 3 files will have a version stamp in bytes [4-5] ([4] is the general tag for type; 'T' for tracing and [5] is the version tag; 'c' for 3. etc.)
Group (version 1)
Note: [0-3] provide group information only and are used only once per group. This means on each primitive after the first [0-3] are unused.

Offset to next group (not valid now)			WORD	[0-1]
Number of primitives							WORD	[2-3]

Note: After looking at incomplete source from original Fortran code I now believe the arc primitive was replaced by the conic primitive in version 1. 
This was as early as summer 89. I do not have enough information to determine if 1 or 2 32-byte chunks were used. 
If the z-coordinate for the center point and begin point was used then it would be 2. 
The early decision based on how much could be placed in a 32-byte chunk would make me think the z-coordinate of one of the points was not used. 
This would allow the primitive to fit in one chunk. The other choice would have required 33 bytes which would have wasted 31 bytes. 
Back then this was major. Another oddity is that byte [7] is not used. I think this is because of the structure alignment requirements. 
I am just guessing that the sweep angle used the bytes of the z-coordinate of the center point instead of the begin point.

Segment (version 1)
		Offset to next group (not valid now)    WORD			[0-1]
		Number of primitives					WORD			[2-3]

		Arc primitive (version 1)
			Pen color							char			[4](0:4)
			Pen style							char			[4](5:8)							
			type code (61)						char			[5]
			Number of 32 byte chunks (1)		char			[6]
			Begin point							vaxfloat[2]		[8-11][12-15]
			??													[16-19]
			Center point						vaxfloat[2]		[20-23][24-27]
			Sweep angle							vaxfloat		[28-31]
		BSpline primitive (version 1)
			Pen color							char			[4](0:4)
			Pen style							char			[4](5:8)							
			type code (24)						char			[5]
			Number of 32 byte chunks (1)		char			[6]
			Number of control points			vaxfloat		[8-11]
			{0 or more control points}			vaxfloat[3]     [12-15][16-19][20-23]
		Line primitive (version 1)
			Pen color							char			[4](0:4)
			Pen style							char			[4](5:8)							
			type code (67)						char			[5]
			Number of 32 byte chunks (1)		char			[6]
			Point 1								vaxfloat[3]		[8-11][12-15][16-19]
			Point 2								vaxfloat[3]		[20-23][24-27][28-31]
		Mark primitive (version 1)
			Pen color							char			[4](0:4)
			Pen style							char			[4](5:8)							
			type code (70)						char			[5]
			Number of 32 byte chunks (1)		char			[6]
			Point								vaxfloat[3]		[8-11][12-15][16-19]
			{3 data values}						vaxfloat		[20-23][24-27][28-31]
		Polygon primitive (version 1)
			Pen color							char			[4](0:4)
			type code (100)						char			[5]
			Number of 32 byte chunks			char			[6]
			Number of verticies					vaxfloat		[8-11]
			style & index ??					vaxfloat		[12-15]
			Hatch
				x scale factor					vaxfloat		[16-19]
				y scale factor					vaxfloat		[20-23]
				rotation angle					vaxfloat		[24-27]
			??													[28-35]
			{0 or more points}					vaxfloat[3]		[36-39][40-43][44-47]
		Text primitive (version 1)
			Pen color							char			[4](0:4)
			type code (17)						char			[5]
			Number of 32 byte chunks			char			[6]
			Insertion point						vaxfloat[3]		[8-11][12-15][16-19]
			Character height					vaxfloat		[20-23]
			Character expansion factor			vaxfloat		[24-27]
			Character rotations angle			vaxfloat		[28-31]
			??													[32-35]
			Character spacing					vaxfloat		[36-39]
			path,halign,valign					vaxfloat		[40-43]
			Text ('\' terminated)				char[]			[44-]


 Segment (version 3)
		??										char			[0] 
		Number of primitives					WORD			[1-2]

		Arc primitive (version 3)
			Number of 32 byte chunks (2)		char			[3]
			CPrim::Type::Arc type code			WORD 0x0100     [4-5]
			Pen color							char			[6]
			Pen style							char			[7]							
			Center point						vaxfloat[3]		[8-11][12-15][16-19]
			Major axis							vaxfloat[3]		[20-23][24-27][28-31]
			Minor axis							vaxfloat[3]		[32-35][36-39][40-43]
			Sweep angle							vaxfloat		[44-47]
		BSpline primitive (version 3)
			Number of 32 byte chunks			char			[3]
				(2 + nPts * 3) / 8 + 1
			CPrim::Type::BSpline type code		WORD 0x2000		[4-5]
			Pen color							char			[6]
			Pen style							char			[7]
			Number of control points			WORD			[8-9]
			{0 or more control points}			vaxfloat[3]     [10-13][14-17][18-21]
		CSpline primitive (version 3 only)
			Number of 32 byte chunks			char			[3]
				(69 + nPts * 12) / 32
			CPrim::Type::CSpline type code		WORD 0x2001		[4-5]
			Pen color							char			[6]
			Pen style							char			[7]
			m_wPtsS								WORD			[8-9]
			Number of control points			WORD			[10-11]
			End condition Type					WORD			[12-13]
			Begin point tangent vector			vaxfloat[3]		[14-17][18-21][22-25]
			End point tangent vector			vaxfloat[3]		[26-29][30-33][34-37]
			{0 or more control points}			vaxfloat[3]     [38-41][42-45][46-49]
		Dim primitive (version 3 only)
			Number of 32 byte chunks			char			[3]								
				(118 + nLen) / 32
			CPrim::Type::Dim type code			WORD 0x4200		[4-5]
			Line pen color						char			[6]
			Line pen style						char			[7]
			Point 1								vaxfloat[3]		[8-11][12-15][16-19]
			Point 2								vaxfloat[3]		[20-23][24-27][28-31]
			Text pen color						char			[32]
			Text precision						char			[33]
			Text font	(0 for simplex)			WORD			[34-35]
			Character spacing					vaxfloat		[36-39]
			Text path							char			[40]
			Horizontal alignment				char			[41]
			Vertical alignment					char			[42]
			Insertion point						vaxfloat[3]		[43-46][47-50][51-54]
			Local reference x-axis				vaxfloat[3]		[55-58][59-62][63-66]
			Local reference y-axis				vaxfloat[3]		[67-70][71-74][75-78]
			Number of characters				WORD			[79-80]
			Text								char[]			[81..]
		Line primitive (version 3)
			Number of 32 byte chunks (1)		char			[3]
			CPrim::Type::Line type code			WORD 0x0200		[4-5]
			Pen color							char			[6]
			Pen style							char			[7]
			Point 1								vaxfloat[3]		[8-11][12-15][16-19]
			Point 2								vaxfloat[3]		[20-23][24-27][28-31]
		Mark primitive (version 3)
			Number of 32 byte chunks (1)		char			[3]
			CPrim::Type::Mark type code			WORD 0x0100		[4-5]
			Pen color							char			[6]
			Mark style							char			[7]
			Point								vaxfloat[3]		[8-11][12-15][16-19]
			{3 data values}						vaxfloat		[20-23][24-27][28-31]
		Polygon primitive (version 3)
			Number of 32 byte chunks			char			[3]					
				(79 + nPts * 12) / 32
			CPrim::Type::Polygon type code		WORD 0x0400		[4-5]
			Pen color							char			[6]
			Polygon style						char			[7]
			Polygon Style Index					WORD			[8-9]
			Number of verticies					WORD			[10-11]
			Hatch origin						vaxfloat[3]		[12-15][16-19][20-23]				
			Hatch/pattern reference x-axis		vaxfloat[3]		[24-27][28-31][32-35]
			Hatch/pattern reference y-axis		vaxfloat[3]		[36-39][40-43][44-47]
			{0 or more points}					vaxfloat[3]		[48-51][52-55][56-59]
		Tag primitive
			Number of 32 byte chunks (1)		char			[3]
			CPrim::Type::Tag type code			WORD 0x4100		[4-5]
			Pen color							char			[6]
			Pen style							char			[7]
			Point								Pnt				[8-11][12-15][16-19]
												unused			[20-31]
		Text primitive (version 3)
			Number of 32 byte chunks			char			[3]								
				(86 + nLen) / 32
			CPrim::Type::Text type code			WORD 0x4000		[4-5]
			Pen color							char			[6]
			Text precision						char			[7]
			Text font	(0 for simplex)			WORD			[8-9]
			Character spacing					vaxfloat		[10-13]
			Text path							char			[14]
			Horizontal alignment				char			[15]
			Vertical alignment					char			[16]
			Insertion point						vaxfloat[3]		[17-20][21-24][25-28]
			Local reference x-axis				vaxfloat[3]		[29-32][33-36][37-40]
			Local reference y-axis				vaxfloat[3]		[41-44][45-48][49-52]
			Number of characters				WORD			[53-54]
			Text								char[]			[55-]