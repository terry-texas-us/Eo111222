#include "stdafx.h"
#include "AeSys.h"
#include "AeSysDoc.h"
#include "FileJob.h"
#include "Vax.h"

void CFileJob::ReadMemFile(CFile& file, EoGeVector3d translateVector)
{
	AeSysDoc* Document = AeSysDoc::GetDoc();

	Document->RemoveAllTrappedGroups();

	EoDbGroup* Group;
	while (GetNextVisibleGroup(file, Group))
	{
		Document->AddWorkLayerGroup(Group); 
		Document->AddGroupToTrap(Group);
	}
	Document->TranslateTrappedGroups(translateVector);
}	 
void CFileJob::ReadHeader(CFile& file)
{
	if (file.Read(m_PrimBuf, 32) == 32)
	{
		m_Version = Version();

		if (m_Version == 1)
		{
			file.SeekToBegin();
		}
		else
		{
			if (file.GetLength() >= 96)
			{
				file.Seek(96, CFile::begin);
			}
		}
	}
}
void CFileJob::ReadLayer(CFile& file, EoDbLayer* layer)
{	
	EoDbGroup* Group;

	while (GetNextVisibleGroup(file, Group))
	{
		if (Group != 0) 
		{
			layer->AddTail(Group);
		}
	}
}	
bool CFileJob::GetNextVisibleGroup(CFile& file, EoDbGroup*& group)
{
	ULONGLONG Position = file.GetPosition();

	group = 0;
	try
	{
		EoDbPrimitive*	Primitive;
		if (!GetNextPrimitive(file, Primitive)) 
		{
			return false;
		}
		group = new EoDbGroup(Primitive); 
		EoUInt16 wPrims = *((EoUInt16*) ((m_Version == 1) ? &m_PrimBuf[2] : &m_PrimBuf[1]));
		for (EoUInt16 w = 1; w < wPrims; w++)
		{
			try
			{
				Position = file.GetPosition();
				if (!GetNextPrimitive(file, Primitive))
					throw L"Exception.FileJob: Unexpected end of file.";
				group->AddTail(Primitive); 
			}
			catch (LPWSTR szMessage)
			{
				app.AddStringToMessageList(szMessage);
				file.Seek(Position + 32, CFile::begin);
			}
		}
	}
	catch (LPWSTR szMessage)
	{
		if (Position >= 96)
		{
			if (::MessageBoxW(0, szMessage, 0, MB_ICONERROR | MB_RETRYCANCEL) == IDCANCEL)
				return false;
		}
		file.Seek(Position + 32, CFile::begin);
	}
	return true;
}
bool CFileJob::GetNextPrimitive(CFile& file, EoDbPrimitive*& primitive)
{
	EoInt16 PrimitiveType = 0;
	do
	{
		if (!ReadNextPrimitive(file, m_PrimBuf, PrimitiveType))
		{
			return false;
		}
	}	 
	while (PrimitiveType <= 0);
	ConstructPrimitive(primitive, PrimitiveType);
	return true;
}
bool CFileJob::ReadNextPrimitive(CFile &file, EoByte *buffer, EoInt16& primitiveType)
{
	if (file.Read(buffer, 32) < 32)
	{
		return false;
	}
	primitiveType = *((EoInt16*) &buffer[4]);

	if (!IsValidPrimitive(primitiveType))
	{
		throw L"Exception.FileJob: Invalid primitive type.";
	}
	int LengthInChunks = (m_Version == 1) ? buffer[6] : buffer[3];
	if (LengthInChunks > 1)
	{
		UINT BytesRemaining = (LengthInChunks - 1) * 32;

		if (BytesRemaining >= EoDbPrimitive::BUFFER_SIZE - 32)
		{
			throw L"Exception.FileJob: Primitive buffer overflow.";
		}
		if (file.Read(&buffer[32], BytesRemaining) < BytesRemaining)
		{
			throw L"Exception.FileJob: Unexpected end of file.";
		}
	}
	return true;
}
int CFileJob::Version()
{
	switch (m_PrimBuf[5])
	{
	case 17: // 0x11 text
	case 24: // 0x18 bspline
	case 33: // 0x21 conic
	case 61: // 0x3D arc
	case 67: // 0x43 line
	case 70: // 0x46 point
	case 100:// 0x64 polygon
		m_Version = 1;
		break;

	default:
		m_Version = 3;
	}
	return (m_Version);
}
bool CFileJob::IsValidPrimitive(EoInt16 primitiveType)
{
	switch (primitiveType)
	{
	case EoDbPrimitive::kPointPrimitive:			// 0x0100
	case EoDbPrimitive::kLinePrimitive:				// 0x0200
	case EoDbPrimitive::kPolygonPrimitive:			// 0x0400
	case EoDbPrimitive::kEllipsePrimitive:			// 0x1003
	case EoDbPrimitive::kSplinePrimitive:			// 0x2000
	case EoDbPrimitive::kCSplinePrimitive:			// 0x2001
	case EoDbPrimitive::kTextPrimitive:				// 0x4000
	case EoDbPrimitive::kTagPrimitive:				// 0x4100
	case EoDbPrimitive::kDimensionPrimitive:		// 0x4200
		return true;

	default:
		return IsValidVersion1Primitive(primitiveType);
	}
}
bool CFileJob::IsValidVersion1Primitive(EoInt16 primitiveType)
{
	EoByte* PrimitiveType = (EoByte*) &primitiveType;
	switch (PrimitiveType[1])
	{
	case 17: // 0x11 text
	case 24: // 0x18 bspline
	case 33: // 0x21 conic
	case 61: // 0x3d arc
	case 67: // 0x43 line
	case 70: // 0x46 point
	case 100:// 0x64 polygon
		return true;

	default:
		return false;
	}
}
void CFileJob::WriteHeader(CFile& file)
{
	::ZeroMemory(m_PrimBuf, 96);
	m_PrimBuf[4] = 'T';
	m_PrimBuf[5] = 'c';
	file.Write(m_PrimBuf, 96);
}
void CFileJob::WriteLayer(CFile& file, EoDbLayer* layer)
{
	layer->BreakSegRefs();
	layer->BreakPolylines();

	POSITION pos = layer->GetHeadPosition();
	while (pos != 0)
	{	
		EoDbGroup* Group = layer->GetNext(pos);
		WriteGroup(file, Group);
	}
}
void CFileJob::WriteGroup(CFile& file, EoDbGroup* group)
{
	m_PrimBuf[0] = 0; 
	*((EoUInt16*) &m_PrimBuf[1]) = EoUInt16(group->GetCount());

	POSITION pos = group->GetHeadPosition();
	while (pos != 0)
	{
		EoDbPrimitive* Primitive = group->GetNext(pos);
		Primitive->Write(file, m_PrimBuf);
	}
}
void CFileJob::ConstructPrimitive(EoDbPrimitive *&primitive, EoInt16 PrimitiveType)
{
	switch(PrimitiveType)
	{
	case EoDbPrimitive::kTagPrimitive:
		ConvertTagToPoint();
		// fall through and construct a EoDbPoint instead
	case EoDbPrimitive::kPointPrimitive:
		primitive = ConvertPointPrimitive();
		break;
	case EoDbPrimitive::kLinePrimitive:
		primitive = ConvertLinePrimitive();
		break;
	case EoDbPrimitive::kPolygonPrimitive:
		primitive = new EoDbPolygon(m_PrimBuf, 3);
		break;
	case EoDbPrimitive::kEllipsePrimitive:
		primitive = ConvertEllipsePrimitive();
		break;
	case EoDbPrimitive::kCSplinePrimitive:
		ConvertCSplineToBSpline();
		// fall through and construct a EoDbSpline instead
	case EoDbPrimitive::kSplinePrimitive:
		primitive = new EoDbSpline(m_PrimBuf, 3);
		break;
	case EoDbPrimitive::kTextPrimitive:
		primitive = new EoDbText(m_PrimBuf, 3);
		static_cast<EoDbText*>(primitive)->ConvertFormattingCharacters();
		break;
	case EoDbPrimitive::kDimensionPrimitive:
		primitive = new EoDbDimension(m_PrimBuf);
		break;

	default:
		ConstructPrimitiveFromVersion1(primitive);
	}
}
void CFileJob::ConstructPrimitiveFromVersion1(EoDbPrimitive *&primitive)
{
	switch (m_PrimBuf[5])
	{
	case 17:
		primitive = new EoDbText(m_PrimBuf, 1);
		static_cast<EoDbText*>(primitive)->ConvertFormattingCharacters();
		break;
	case 24:
		primitive = new EoDbSpline(m_PrimBuf, 1);
		break;
	case 33:
		break;
	case 61:
		primitive = ConvertVersion1EllipsePrimitive();
		break;
	case 67:
		primitive = ConvertVersion1LinePrimitive();
		break;
	case 70:
		primitive = ConvertVersion1PointPrimitive();
		break;
	case 100:
		primitive = new EoDbPolygon(m_PrimBuf, 1);
		break;

	default:
		throw L"Exception.FileJob: Invalid primitive type.";
	}
}
EoDbPrimitive* CFileJob::ConvertEllipsePrimitive()
{
	EoInt16 PenColor = EoInt16(m_PrimBuf[6]);
	EoInt16 LineType = EoInt16(m_PrimBuf[7]);

	EoGePoint3d CenterPoint = ((CVaxPnt*) &m_PrimBuf[8])->Convert();
	EoGeVector3d MajorAxis = ((CVaxVec*) &m_PrimBuf[20])->Convert();
	EoGeVector3d MinorAxis = ((CVaxVec*) &m_PrimBuf[32])->Convert();

	double SweepAngle = ((CVaxFloat*) &m_PrimBuf[44])->Convert();

	if (SweepAngle > TWOPI || SweepAngle < - TWOPI)
		SweepAngle = TWOPI;

	return new EoDbEllipse(PenColor, LineType, CenterPoint, MajorAxis, MinorAxis, SweepAngle);
}
EoDbPrimitive* CFileJob::ConvertLinePrimitive()
{
	EoInt16 PenColor = EoInt16(m_PrimBuf[6]);
	EoInt16 LineType = EoInt16(m_PrimBuf[7]);

	CLine Line;
	Line.begin = ((CVaxPnt*) &m_PrimBuf[8])->Convert();
	Line.end = ((CVaxPnt*) &m_PrimBuf[20])->Convert();

	return new EoDbLine(PenColor, LineType, Line);
}
EoDbPrimitive* CFileJob::ConvertPointPrimitive()
{
	EoInt16 PenColor = EoInt16(m_PrimBuf[6]);
	EoInt16 PointStyle = EoInt16(m_PrimBuf[7]);

	EoGePoint3d Point = ((CVaxPnt*) &m_PrimBuf[8])->Convert();

	double Data[3];

	Data[0] = ((CVaxFloat*) &m_PrimBuf[20])->Convert(); 
	Data[1] = ((CVaxFloat*) &m_PrimBuf[24])->Convert(); 
	Data[2] = ((CVaxFloat*) &m_PrimBuf[28])->Convert(); 

	return new EoDbPoint(PenColor, PointStyle, Point, 3, Data);
}
EoDbPrimitive* CFileJob::ConvertVersion1EllipsePrimitive()
{
	EoInt16 PenColor = EoInt16(m_PrimBuf[4] & 0x000f);
	EoInt16 LineType = EoInt16((m_PrimBuf[4] & 0x00ff) >> 4);

	EoGePoint3d BeginPoint(((CVaxFloat*) &m_PrimBuf[8])->Convert(), ((CVaxFloat*) &m_PrimBuf[12])->Convert(), 0.);
	BeginPoint *= 1.e-3;		

	EoGePoint3d CenterPoint(((CVaxFloat*) &m_PrimBuf[20])->Convert(), ((CVaxFloat*) &m_PrimBuf[24])->Convert(), 0.);
	CenterPoint *= 1.e-3;

	double SweepAngle = ((CVaxFloat*) &m_PrimBuf[28])->Convert();;

	EoGeVector3d MajorAxis;
	if (SweepAngle < 0.)
	{
		EoGePoint3d pt;
		pt.x = (CenterPoint.x + ((BeginPoint.x - CenterPoint.x) * cos(SweepAngle) - (BeginPoint.y - CenterPoint.y) * sin(SweepAngle)));
		pt.y = (CenterPoint.y + ((BeginPoint.x - CenterPoint.x) * sin(SweepAngle) + (BeginPoint.y - CenterPoint.y) * cos(SweepAngle)));
		MajorAxis = CenterPoint - pt;
	}
	else
	{
		MajorAxis = CenterPoint - BeginPoint;
	}
	EoGeVector3d MinorAxis = EoGeVector3d::kZAxis.CrossProduct(MajorAxis);
	SweepAngle = fabs(SweepAngle);

	return new EoDbEllipse(PenColor, LineType, CenterPoint, MajorAxis, MinorAxis, SweepAngle);
}
EoDbPrimitive* CFileJob::ConvertVersion1LinePrimitive()
{
	EoInt16 PenColor = EoInt16(m_PrimBuf[4] & 0x000f);
	EoInt16 LineType = EoInt16((m_PrimBuf[4] & 0x00ff) >> 4);

	CLine Line;
	Line.begin = ((CVaxPnt*) &m_PrimBuf[8])->Convert() * 1.e-3;
	Line.end = ((CVaxPnt*) &m_PrimBuf[20])->Convert() * 1.e-3;

	return new EoDbLine(PenColor, LineType, Line);
}
EoDbPrimitive* CFileJob::ConvertVersion1PointPrimitive()
{
	EoInt16 PenColor = EoInt16(m_PrimBuf[4] & 0x000f);
	EoInt16 PointStyle = EoInt16((m_PrimBuf[4] & 0x00ff) >> 4);

	EoGePoint3d Point = ((CVaxPnt*) &m_PrimBuf[8])->Convert();
	Point *= 1.e-3;

	double Data[3];

	Data[0] = ((CVaxFloat*) &m_PrimBuf[20])->Convert(); 
	Data[1] = ((CVaxFloat*) &m_PrimBuf[24])->Convert(); 
	Data[2] = ((CVaxFloat*) &m_PrimBuf[28])->Convert(); 

	return new EoDbPoint(PenColor, PointStyle, Point, 3, Data);
}
void CFileJob::ConvertCSplineToBSpline()
{
	EoUInt16 NumberOfControlPoints = *((EoUInt16*) &m_PrimBuf[10]);

	m_PrimBuf[3] = EoSbyte((2 + NumberOfControlPoints * 3) / 8 + 1);
	*((EoUInt16*) &m_PrimBuf[4]) = EoUInt16(EoDbPrimitive::kSplinePrimitive);
	m_PrimBuf[8] = m_PrimBuf[10];
	m_PrimBuf[9] = m_PrimBuf[11];
	::MoveMemory(&m_PrimBuf[10], &m_PrimBuf[38], NumberOfControlPoints * 3 * sizeof(CVaxFloat));
}
void CFileJob::ConvertTagToPoint()
{
	*((EoUInt16*) &m_PrimBuf[4]) = EoDbPrimitive::kPointPrimitive;
	::ZeroMemory(&m_PrimBuf[20], 12);
}
EoDbDimension::EoDbDimension(EoByte* buffer)
{
	m_PenColor = EoInt16(buffer[6]);
	m_LineType = EoInt16(buffer[7]);

	m_ln.begin = ((CVaxPnt*) &buffer[8])->Convert();
	m_ln.end = ((CVaxPnt*) &buffer[20])->Convert();

	m_PenColor = EoInt16(buffer[32]);

	m_fd.FontName(L"Simplex.psf");
	m_fd.Precision(EoDb::kStrokeType);
	m_fd.CharacterSpacing(((CVaxFloat*) &buffer[36])->Convert());
	m_fd.Path(EoByte(buffer[40]));
	m_fd.HorizontalAlignment(EoByte(buffer[41]));
	m_fd.VerticalAlignment(EoByte(buffer[42]));

	m_rs.SetOrigin(((CVaxPnt*) &buffer[43])->Convert());
	m_rs.SetXDirection(((CVaxVec*) &buffer[55])->Convert());
	m_rs.SetYDirection(((CVaxVec*) &buffer[67])->Convert());

	EoInt16 TextLength = *((EoInt16*) &buffer[79]);

	buffer[81 + TextLength] = '\0';
	m_strText = CString((LPCSTR) &buffer[81]);
}
EoDbPolygon::EoDbPolygon(EoByte* buffer, int version)
{
	if (version == 1)
	{
		m_PenColor = EoInt16(buffer[4] & 0x000f);
		m_InteriorStyleIndex = 0;

		double d = ((CVaxFloat*) &buffer[12])->Convert();
		m_InteriorStyle = EoInt16(int(d) % 16);

		switch (m_InteriorStyle)
		{
		case EoDb::Hatch:
			{
				double dXScal = ((CVaxFloat*) &buffer[16])->Convert();
				double dYScal = ((CVaxFloat*) &buffer[20])->Convert();
				double dAng = ((CVaxFloat*) &buffer[24])->Convert();

				m_vPosXAx.z = 0.;
				m_vPosYAx.z = 0.;

				if (fabs(dXScal) > FLT_EPSILON && fabs(dYScal) > FLT_EPSILON)
				{ // Have 2 hatch lines
					m_InteriorStyleIndex = 2;
					m_vPosXAx.x = cos(dAng);
					m_vPosXAx.y = sin(dAng);
					m_vPosYAx.x = - m_vPosXAx.y;
					m_vPosYAx.y = m_vPosXAx.x;
					m_vPosXAx *= dXScal * 1.e-3;
					m_vPosYAx *= dYScal * 1.e-3;
				}
				else if (fabs(dXScal) > FLT_EPSILON)
				{ // Vertical hatch lines
					m_InteriorStyleIndex = 1;
					m_vPosXAx.x = cos(dAng + HALF_PI);
					m_vPosXAx.y = sin(dAng + HALF_PI);
					m_vPosYAx.x = - m_vPosXAx.y;
					m_vPosYAx.y = m_vPosXAx.x;
					m_vPosYAx *= dXScal * 1.e-3;
				}
				else
				{ // Horizontal hatch lines
					m_InteriorStyleIndex = 1;
					m_vPosXAx.x = cos(dAng);
					m_vPosXAx.y = sin(dAng);
					m_vPosYAx.x = - m_vPosXAx.y;
					m_vPosYAx.y = m_vPosXAx.x;
					m_vPosYAx *= dYScal * 1.e-3;
				}
				break;
			}
		case EoDb::Hollow:
		case EoDb::Solid:
		case EoDb::Pattern:
			m_vPosXAx(1., 0., 0.);
			m_vPosYAx(0., 1., 0.);
			break;

		default:
			m_NumberOfPoints = 3;
			m_Pt = new EoGePoint3d[m_NumberOfPoints];
			m_Pt[0] = EoGePoint3d::kOrigin;
			m_Pt[1] = EoGePoint3d::kOrigin + EoGeVector3d::kXAxis;
			m_Pt[2] = EoGePoint3d::kOrigin + EoGeVector3d::kYAxis;
			m_HatchOrigin = m_Pt[0];
			return;
		}
		m_NumberOfPoints = EoUInt16(((CVaxFloat*) &buffer[8])->Convert());

		m_Pt = new EoGePoint3d[m_NumberOfPoints];

		int i = 36;

		for (EoUInt16 w = 0; w < m_NumberOfPoints; w++)
		{
			m_Pt[w] = ((CVaxPnt*) &buffer[i])->Convert() * 1.e-3;
			i += sizeof(CVaxPnt);
		}
		m_HatchOrigin = m_Pt[0];
	}
	else
	{
		m_PenColor = EoInt16(buffer[6]);
		m_InteriorStyle = EoSbyte(buffer[7]);
		m_InteriorStyleIndex = *((EoInt16*) &buffer[8]);
		m_NumberOfPoints = *((EoInt16*) &buffer[10]);
		m_HatchOrigin = ((CVaxPnt*) &buffer[12])->Convert();
		m_vPosXAx = ((CVaxVec*) &buffer[24])->Convert();
		m_vPosYAx = ((CVaxVec*) &buffer[36])->Convert();
		m_Pt = new EoGePoint3d[m_NumberOfPoints];

		int i = 48;

		for (EoUInt16 w = 0; w < m_NumberOfPoints; w++)
		{
			m_Pt[w] = ((CVaxPnt*) &buffer[i])->Convert();
			i += sizeof(CVaxPnt);
		}
	}
}
EoDbSpline::EoDbSpline(EoByte* buffer, int version)
{
	if (version == 1)
	{
		m_PenColor = EoInt16(buffer[4] & 0x000f);
		m_LineType = EoInt16((buffer[4] & 0x00ff) >> 4);

		EoUInt16 wPts = EoUInt16(((CVaxFloat*) &buffer[8])->Convert());

		int i = 12;

		for (EoUInt16 w = 0; w < wPts; w++)
		{
			EoGePoint3d pt = ((CVaxPnt*) &buffer[i])->Convert() * 1.e-3;
			m_pts.Add(pt);
			i += sizeof(CVaxPnt);
		}
	}
	else
	{
		m_PenColor = EoInt16(buffer[6]);
		m_LineType = EoInt16(buffer[7]);

		EoUInt16 wPts = *((EoInt16*) &buffer[8]);

		int i = 10;

		for (EoUInt16 w = 0; w < wPts; w++)
		{
			EoGePoint3d pt = ((CVaxPnt*) &buffer[i])->Convert();
			m_pts.Add(pt);
			i += sizeof(CVaxPnt);
		}
	}
}
EoDbText::EoDbText(EoByte* buffer, int version)
{
	m_fd.Precision(EoDb::kStrokeType);
	m_fd.FontName(L"Simplex.psf");

	if (version == 1)
	{
		m_PenColor = EoInt16(buffer[4] & 0x000f);
		m_fd.CharacterSpacing(((CVaxFloat*) &buffer[36])->Convert());
		m_fd.CharacterSpacing(min(max(m_fd.CharacterSpacing(), 0.), 4.));

		double d = ((CVaxFloat*) &buffer[40])->Convert();

		m_fd.Path(EoUInt16(fmod(d, 10.)));
		if (m_fd.Path() < 0 || m_fd.Path() > 4) 
			m_fd.Path(EoDb::kPathRight);
		m_fd.HorizontalAlignment(EoUInt16(fmod(d / 10., 10.)));
		if (m_fd.HorizontalAlignment() < 1 || m_fd.HorizontalAlignment() > 3) 
			m_fd.HorizontalAlignment(EoDb::kAlignCenter);
		m_fd.VerticalAlignment(EoUInt16((d / 100.)));
		if (m_fd.VerticalAlignment() < 2 || m_fd.VerticalAlignment() > 4) 
			m_fd.VerticalAlignment(EoDb::kAlignMiddle);

		m_rs.SetOrigin(((CVaxPnt*) &buffer[8])->Convert() * 1.e-3);

		double dChrHgt = ((CVaxFloat*) &buffer[20])->Convert();
		dChrHgt = min(max(dChrHgt, .01e3), 100.e3);

		double dChrExpFac = ((CVaxFloat*) &buffer[24])->Convert();
		dChrExpFac = min(max(dChrExpFac, 0.), 10.);

		double dAng = ((CVaxFloat*) &buffer[28])->Convert();
		dAng = min(max(dAng, - TWOPI), TWOPI);

		m_rs.SetXDirection(EoGeVector3d(.6 * dChrHgt * dChrExpFac, 0., 0.) * 1.e-3);
		m_rs.SetYDirection(EoGeVector3d(0., dChrHgt, 0.) * 1.e-3);

		if (fabs(dAng) > FLT_EPSILON)
		{
			EoGeVector3d vDirX(m_rs.XDirection());
			vDirX.RotateAboutZAxis(dAng);
			m_rs.SetXDirection(vDirX);
			EoGeVector3d vDirY(m_rs.YDirection());
			vDirY.RotateAboutZAxis(dAng);
			m_rs.SetYDirection(vDirY);
		}
		char* NextToken = NULL;
		char* pChr = strtok_s((char*) &buffer[44], "\\", &NextToken);

		if (pChr == 0)
			m_strText = L"CFileJob.PrimText error: Missing string terminator.";
		else if (strlen(pChr) > 132)
			m_strText = L"CFileJob.PrimText error: Text too long.";
		else
		{
			while (*pChr != 0)
			{
				if (!isprint(*pChr)) *pChr = '.';
				pChr++;
			}
			m_strText = &buffer[44];
		}
	}
	else
	{
		m_PenColor = EoInt16(buffer[6]);
		m_fd.CharacterSpacing(((CVaxFloat*) &buffer[10])->Convert());
		m_fd.Path(EoByte(buffer[14]));
		m_fd.HorizontalAlignment(EoByte(buffer[15]));
		m_fd.VerticalAlignment(EoByte(buffer[16]));
		m_rs.SetOrigin(((CVaxPnt*) &buffer[17])->Convert());
		m_rs.SetXDirection(((CVaxVec*) &buffer[29])->Convert());
		m_rs.SetYDirection(((CVaxVec*) &buffer[41])->Convert());

		EoInt16 TextLength = *((EoInt16*) &buffer[53]);
		buffer[55 + TextLength] = '\0';
		m_strText = CString((LPCSTR) &buffer[55]);
	}
}
void EoDbEllipse::Write(CFile& file, EoByte* buffer)
{
	buffer[3] = 2;
	*((EoUInt16*) &buffer[4]) = EoUInt16(EoDbPrimitive::kEllipsePrimitive);
	buffer[6] = EoSbyte(m_PenColor == PENCOLOR_BYLAYER ? mS_nLayerPenColor : m_PenColor);
	buffer[7] = EoSbyte(m_LineType == LINETYPE_BYLAYER ? mS_LayerLineType : m_LineType);
	if (buffer[7] >= 16) buffer[7] = 2;

	((CVaxPnt*) &buffer[8])->Convert(m_ptCenter);
	((CVaxVec*) &buffer[20])->Convert(m_vMajAx);
	((CVaxVec*) &buffer[32])->Convert(m_vMinAx);
	((CVaxFloat*) &buffer[44])->Convert(m_dSwpAng);

	file.Write(buffer, 64);
}
void EoDbDimension::Write(CFile& file, EoByte* buffer)
{
	EoUInt16 TextLength = EoUInt16(m_strText.GetLength());

	buffer[3] = EoByte((118 + TextLength) / 32);
	*((EoUInt16*) &buffer[4]) = EoUInt16(EoDbPrimitive::kDimensionPrimitive);
	buffer[6] = EoSbyte(m_PenColor == PENCOLOR_BYLAYER ? mS_nLayerPenColor : m_PenColor);
	buffer[7] = EoSbyte(m_LineType == LINETYPE_BYLAYER ? mS_LayerLineType : m_LineType);
	if (buffer[7] >= 16) buffer[7] = 2;

	((CVaxPnt*) &buffer[8])->Convert(m_ln.begin);
	((CVaxPnt*) &buffer[20])->Convert(m_ln.end);

	buffer[32] = EoSbyte(m_PenColor);
	buffer[33] = EoSbyte(EoDb::kStrokeType);
	*((EoInt16*) &buffer[34]) = 0;
	((CVaxFloat*) &buffer[36])->Convert(m_fd.CharacterSpacing());
	buffer[40] = EoSbyte(m_fd.Path());
	buffer[41] = EoSbyte(m_fd.HorizontalAlignment());
	buffer[42] = EoSbyte(m_fd.VerticalAlignment());

	CRefSys rs = m_rs;

	((CVaxPnt*) &buffer[43])->Convert(rs.Origin());
	((CVaxVec*) &buffer[55])->Convert(rs.XDirection());
	((CVaxVec*) &buffer[67])->Convert(rs.YDirection());

	*((EoInt16*) &buffer[79]) = TextLength;
	memcpy(&buffer[81], (LPCWSTR) m_strText, TextLength);

	file.Write(buffer, buffer[3] * 32);
}
void EoDbLine::Write(CFile& file, EoByte* buffer)
{
	buffer[3] = 1;	
	*((EoUInt16*) &buffer[4]) = EoUInt16(EoDbPrimitive::kLinePrimitive);
	buffer[6] = EoSbyte(m_PenColor == PENCOLOR_BYLAYER ? mS_nLayerPenColor : m_PenColor);
	buffer[7] = EoSbyte(m_LineType == LINETYPE_BYLAYER ? mS_LayerLineType : m_LineType);
	if (buffer[7] >= 16) buffer[7] = 2;

	((CVaxPnt*) &buffer[8])->Convert(m_ln.begin);
	((CVaxPnt*) &buffer[20])->Convert(m_ln.end);

	file.Write(buffer, 32);
}
void EoDbPoint::Write(CFile& file, EoByte* buffer)
{
	buffer[3] = 1;
	*((EoUInt16*) &buffer[4]) = EoUInt16(EoDbPrimitive::kPointPrimitive);
	buffer[6] = EoSbyte(m_PenColor == PENCOLOR_BYLAYER ? mS_nLayerPenColor : m_PenColor);
	buffer[7] = EoSbyte(m_PointStyle);

	((CVaxPnt*) &buffer[8])->Convert(m_Point);

	::ZeroMemory(&buffer[20], 12);

	int i = 20;

	for (EoUInt16 w = 0; w < m_NumberOfDatums; w++)
	{
		((CVaxFloat*) &buffer[i])->Convert(m_Data[w]);
		i += sizeof(CVaxFloat);
	}

	file.Write(buffer, 32);
}
void EoDbPolygon::Write(CFile& file, EoByte* buffer)
{
	buffer[3] = EoSbyte((79 + m_NumberOfPoints * 12) / 32);
	*((EoUInt16*) &buffer[4]) = EoUInt16(EoDbPrimitive::kPolygonPrimitive);
	buffer[6] = EoSbyte(m_PenColor == PENCOLOR_BYLAYER ? mS_nLayerPenColor : m_PenColor);
	buffer[7] = EoSbyte(m_InteriorStyle);
	*((EoInt16*) &buffer[8]) = EoInt16(m_InteriorStyleIndex);
	*((EoInt16*) &buffer[10]) = m_NumberOfPoints;

	((CVaxPnt*) &buffer[12])->Convert(m_HatchOrigin);
	((CVaxVec*) &buffer[24])->Convert(m_vPosXAx);
	((CVaxVec*) &buffer[36])->Convert(m_vPosYAx);

	int i = 48;

	for (EoUInt16 w = 0; w < m_NumberOfPoints; w++)
	{
		((CVaxPnt*) &buffer[i])->Convert(m_Pt[w]);
		i += sizeof(CVaxPnt);
	}
	file.Write(buffer, buffer[3] * 32);
}
void EoDbSpline::Write(CFile& file, EoByte* buffer)
{
	buffer[3] = EoSbyte((2 + m_pts.GetSize() * 3) / 8 + 1);
	*((EoUInt16*) &buffer[4]) = EoUInt16(EoDbPrimitive::kSplinePrimitive);
	buffer[6] = EoSbyte(m_PenColor == PENCOLOR_BYLAYER ? mS_nLayerPenColor : m_PenColor);
	buffer[7] = EoSbyte(m_LineType == LINETYPE_BYLAYER ? mS_LayerLineType : m_LineType);

	*((EoInt16*) &buffer[8]) = (EoInt16) m_pts.GetSize();

	int i = 10;

	for (EoUInt16 w = 0; w < m_pts.GetSize(); w++)
	{
		((CVaxPnt*) &buffer[i])->Convert(m_pts[w]);
		i += sizeof(CVaxPnt);
	}
	file.Write(buffer, buffer[3] * 32);
}
void EoDbText::Write(CFile& file, EoByte* buffer)
{
	EoUInt16 TextLength = EoUInt16(m_strText.GetLength());

	buffer[3] = EoSbyte((86 + TextLength) / 32);
	*((EoUInt16*) &buffer[4]) = EoUInt16(EoDbPrimitive::kTextPrimitive);
	buffer[6] = EoSbyte(m_PenColor == PENCOLOR_BYLAYER ? mS_nLayerPenColor : m_PenColor);
	buffer[7] = EoSbyte(m_fd.Precision());
	*((EoInt16*) &buffer[8]) = 0;
	((CVaxFloat*) &buffer[10])->Convert(m_fd.CharacterSpacing());
	buffer[14] = EoSbyte(m_fd.Path());
	buffer[15] = EoSbyte(m_fd.HorizontalAlignment());
	buffer[16] = EoSbyte(m_fd.VerticalAlignment());

	CRefSys rs = m_rs;
	((CVaxPnt*) &buffer[17])->Convert(rs.Origin());
	((CVaxVec*) &buffer[29])->Convert(rs.XDirection());
	((CVaxVec*) &buffer[41])->Convert(rs.YDirection());


	*((EoUInt16*) &buffer[53]) = TextLength;
	memcpy(&buffer[55], (LPCWSTR) m_strText, TextLength);

	file.Write(buffer, buffer[3] * 32);
}