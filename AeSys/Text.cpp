#include "Stdafx.h"
#include "AeSys.h"
#include "AeSysView.h"

#include "Text.h"

bool HasFormattingCharacters(const CString& text)
{
	for (int i = 0; i < text.GetLength() - 1; i++)
	{
		if (text[i] == '\\')
		{
			switch (text[i + 1])
			{			//	Parameter					Meaning
			case 'P':	//								Hard line break
				//case '~':	//								Nonbreaking space
				//case '/':	//								Single backslash; otherwise used as an escape character
				//case '{':	//								Single opening curly bracket; otherwise used as block begin
				//case '}':	//								Single closing curly bracket; otherwise used as block end
			case 'A':	// 0, 1, or 2					Change alignment to bottom, center, or top
				//case 'C':	// ACI color number				Change character color
				//case 'F':	// Font information				Change to a different font
				//									acad:   \FArial.shx
				//									windows \FArial|b1|i0|c0|p34
				//case 'H':	// New height or relative		Change text height
				// height followed by an x
				//case 'L':	//								Start underlining
				//case 'l':	//								End underlining
				//case 'O':	//								Start overlining
				//case 'o':	//								End overlining
				//case 'T':	//								Change kerning, i.e. character spacing
				//case 'W':	//								Change character width, i.e X scaling
			case 'S':	//								Stacked text or fractions
				//									the S is follwed by two text segments separated by a 
				//									/ (fraction bar) or ^ (no fraction bar)
				return true;
			}
		}
	}
	return false;
}
void DisplayText(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text)
{
	if (text.IsEmpty())
		return;

	if (HasFormattingCharacters(text))
	{
		DisplayTextWithFormattingCharacters(view, deviceContext, fd, referenceSystem, text);
		return;
	}
	EoGeReferenceSystem ReferenceSystem = referenceSystem;
	EoGeTransformMatrix tm(ReferenceSystem.TransformMatrix()); 
	tm.Inverse();

	EoGePoint3d BottomLeftCorner;
	GetBottomLeftCorner(fd, text.GetLength(), BottomLeftCorner);
	BottomLeftCorner = tm * BottomLeftCorner;
	ReferenceSystem.SetOrigin(BottomLeftCorner);

	int NumberOfCharactersToDisplay = 0;
	int StartPosition = 0;
	int CurrentPosition = StartPosition;
	while (CurrentPosition < text.GetLength())
	{
		WCHAR c = text[CurrentPosition++];

		if (c == '\r' && text[CurrentPosition] == '\n')
		{	
			DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

			ReferenceSystem.SetOrigin(BottomLeftCorner);
			ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 1., 0));
			BottomLeftCorner = ReferenceSystem.Origin();

			StartPosition += 2 + NumberOfCharactersToDisplay;
			CurrentPosition = StartPosition;
			NumberOfCharactersToDisplay = 0;
		}
		else
		{
			NumberOfCharactersToDisplay++;
		}
	}
	DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
}
void DisplayTextSegment(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text)
{
	if (deviceContext != 0 && fd.Precision() == EoDb::kEoTrueType && view->ViewTrueTypeFonts())	
	{
		EoGeVector3d XDirection(referenceSystem.XDirection());
		EoGeVector3d YDirection(referenceSystem.YDirection());

		view->ModelViewTransformVector(XDirection);
		view->ModelViewTransformVector(YDirection);

		EoGeVector3d PlaneNormal = EoGeCrossProduct(XDirection, YDirection);
		PlaneNormal.Normalize();

		if (PlaneNormal == EoGeVector3d::kZAxis)
		{
			if (DisplayTextSegmentUsingTrueTypeFont(view, deviceContext, fd, referenceSystem, startPosition, numberOfCharacters, text))
				return;	
		}
	}
	DisplayTextSegmentUsingStrokeFont(view, deviceContext, fd, referenceSystem, startPosition, numberOfCharacters, text);
}
void DisplayTextSegmentUsingStrokeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text)
{
	if (numberOfCharacters == 0) return;

	long* plStrokeFontDef = (long*) app.SimplexStrokeFont();
	if (plStrokeFontDef == 0) return;

	EoGeTransformMatrix tm(referenceSystem.TransformMatrix()); 
	tm.Inverse();

	long* plStrokeChrDef = plStrokeFontDef + 96;
	double dChrSpac = 1. + (.32 + fd.CharacterSpacing()) / .6;

	EoGePoint3d ptStroke = EoGePoint3d::kOrigin;
	EoGePoint3d ptChrPos = ptStroke;
	EoGePoint3d ptLinePos = ptChrPos;

	int n = startPosition;

	while (n < startPosition + numberOfCharacters)
	{
		polyline::BeginLineStrip();

		int Character = text.GetAt(n);
		if (Character < 32 || Character > 126) Character = '.';

		for (int i = (int) plStrokeFontDef[Character - 32]; i <= plStrokeFontDef[Character - 32 + 1] - 1; i++) 
		{
			int iY = (int) (plStrokeChrDef[i - 1] % 4096L);
			if ((iY & 2048) != 0) 
				iY = - (iY - 2048);
			int iX = (int) ((plStrokeChrDef[i - 1] / 4096L) % 4096L);
			if ((iX & 2048) != 0) 
				iX = - (iX - 2048);

			ptStroke += EoGeVector3d(.01 / .6 * iX, .01 * iY, 0.0);

			if (plStrokeChrDef[i - 1] / 16777216 == 5)
			{
				polyline::__End(view, deviceContext, 1);
				polyline::BeginLineStrip();
			}
			polyline::SetVertex(tm * ptStroke);
		}
		polyline::__End(view, deviceContext, 1);

		switch (fd.Path())
		{
		case EoDb::kPathLeft:
			ptChrPos.x -= dChrSpac;
			break;
		case EoDb::kPathUp:
			ptChrPos.y += dChrSpac;
			break;
		case EoDb::kPathDown:
			ptChrPos.y -= dChrSpac;
			break;
		default:
			ptChrPos.x += dChrSpac;
		}
		ptStroke = ptChrPos;
		n++;
	}
}
int FontEscapementAngle(const EoGeVector3d& xAxis)
{
	double Angle = 0.0;

	Angle = atan2(xAxis.y, xAxis.x); // -pi to pi radians
	if (Angle < 0.0)
	{
		Angle  += TWOPI;
	}
	return EoRound(EoToDegree(Angle) * 10.);
}
bool DisplayTextSegmentUsingTrueTypeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text)
{
	if (numberOfCharacters <= 0)
		return true;

	EoGeTransformMatrix tm(referenceSystem.TransformMatrix()); 
	tm.Inverse();

	EoGePoint4d StartPoint = tm * EoGePoint3d::kOrigin;
	view->ModelViewTransformPoint(StartPoint);
	CPoint ProjectedStartPoint = view->DoProjection(StartPoint);

	EoGePoint4d ptsBox[3];

	ptsBox[1] = tm * EoGePoint3d(0.0, 1.0, 0.0);
	ptsBox[2] = tm * EoGePoint3d(1.0, 0.0, 0.0);

	view->ModelViewTransformPoint(ptsBox[1]);
	view->ModelViewTransformPoint(ptsBox[2]);

	CPoint pnt[4];

	pnt[1] = view->DoProjection(ptsBox[1]);
	pnt[2] = view->DoProjection(ptsBox[2]);

	EoGeVector3d vX(double(pnt[2].x - ProjectedStartPoint.x), double(pnt[2].y - ProjectedStartPoint.y), 0.0); 
	EoGeVector3d vY(double(pnt[1].x - ProjectedStartPoint.x), double(pnt[1].y - ProjectedStartPoint.y), 0.0); 

	double dHeight = vY.Length();
	if (dHeight == 0.0)
	{
		return true;
	}
	LOGFONT logfont;
	logfont.lfHeight = - EoRound(1.33 * dHeight);
	logfont.lfWidth = 0;
	logfont.lfEscapement = - FontEscapementAngle(vX);
	logfont.lfOrientation = logfont.lfEscapement;
	logfont.lfWeight = FW_THIN;
	logfont.lfItalic = FALSE;
	logfont.lfUnderline = FALSE;
	logfont.lfStrikeOut = FALSE;
	logfont.lfCharSet = ANSI_CHARSET;
	logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	logfont.lfQuality = DEFAULT_QUALITY;
	logfont.lfPitchAndFamily = DEFAULT_PITCH;
	wcscpy_s(logfont.lfFaceName, LF_FACESIZE, fd.FontName());

	CFont font;
	font.CreateFontIndirect(&logfont);
	CFont* pfntold = (CFont*) deviceContext->SelectObject(&font);
	UINT uTextAlign = deviceContext->SetTextAlign(TA_LEFT | TA_BASELINE);
	int iBkMode = deviceContext->SetBkMode(TRANSPARENT);

	deviceContext->TextOut(ProjectedStartPoint.x, ProjectedStartPoint.y, (LPCWSTR) text.Mid(startPosition), numberOfCharacters);
	deviceContext->SetBkMode(iBkMode);
	deviceContext->SetTextAlign(uTextAlign);
	deviceContext->SelectObject(pfntold);

	return true;
}
void DisplayTextWithFormattingCharacters(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text)
{
	EoGeReferenceSystem ReferenceSystem = referenceSystem;

	EoGeTransformMatrix tm(ReferenceSystem.TransformMatrix()); 
	tm.Inverse();

	int Length = LengthSansFormattingCharacters(text);

	EoGePoint3d BottomLeftCorner;
	GetBottomLeftCorner(fd, Length, BottomLeftCorner);
	BottomLeftCorner = tm * BottomLeftCorner;
	ReferenceSystem.SetOrigin(BottomLeftCorner);

	int NumberOfCharactersToDisplay = 0;
	int StartPosition = 0;
	int CurrentPosition = StartPosition;

	while (CurrentPosition < text.GetLength())
	{
		WCHAR c = text[CurrentPosition++];
		if (c != '\\')
		{
			NumberOfCharactersToDisplay++;
		}
		else
		{
			c = text[CurrentPosition];
			if (c == 'P') // Hard line bresk
			{
				if (CurrentPosition < text.GetLength())
				{
					DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

					ReferenceSystem.SetOrigin(BottomLeftCorner);
					ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 1., 0));
					BottomLeftCorner = ReferenceSystem.Origin();
					StartPosition += 2 + NumberOfCharactersToDisplay;
					CurrentPosition = StartPosition;
					NumberOfCharactersToDisplay = 0;
				}
			}
			else if (c == 'A') // Change alignment to bottom, center middle
			{
				int EndSemicolon = text.Find(';', CurrentPosition);
				if (EndSemicolon != - 1)
				{
					if (CurrentPosition + 1 < EndSemicolon) 
					{
						WCHAR Parameter = text[CurrentPosition + 1];
						if (Parameter >= '0' && Parameter <= '2')
						{
							if (NumberOfCharactersToDisplay > 0)
							{ // display text segment preceding the formatting
								DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

								// Offset the line position left of current position
								ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, referenceSystem, 0.0, NumberOfCharactersToDisplay * (1 + .32 / .6)));
								BottomLeftCorner = ReferenceSystem.Origin();
							}
							StartPosition = EndSemicolon + 1;	
							CurrentPosition = StartPosition;
							NumberOfCharactersToDisplay = text.GetLength() - StartPosition;

							if (Parameter == '1')
							{
								ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, .5, 0.0));
							}
							else if (Parameter == '2')
							{
								ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, - .5, 0.0));
							}
							DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
							return;
						}
					}
				}
			}
			else if (c == 'S')	// Stacked text or fractions
			{
				int EndSemicolon = text.Find(';', CurrentPosition);
				if (EndSemicolon != - 1)
				{
					int TextSegmentDelimiter = text.Find('/', CurrentPosition);
					if (TextSegmentDelimiter == - 1)
						TextSegmentDelimiter = text.Find('^', CurrentPosition);

					if (TextSegmentDelimiter != - 1 && TextSegmentDelimiter < EndSemicolon)
					{ 
						if (NumberOfCharactersToDisplay > 0)
						{ // display text segment preceding the formatting
							DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
							StartPosition += NumberOfCharactersToDisplay;	
						}
						// Offset the line position up and conditionally left of current position
						ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, - .35, NumberOfCharactersToDisplay * (1 + .32 / .6)));
						BottomLeftCorner = ReferenceSystem.Origin();
						StartPosition += 2; // skip the formatting characters
						NumberOfCharactersToDisplay = TextSegmentDelimiter - StartPosition;
						if (NumberOfCharactersToDisplay > 0)
						{ // Display superscripted text segment
							DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
							StartPosition += NumberOfCharactersToDisplay;
						}
						// Offset the line position back down left
						ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, .35, NumberOfCharactersToDisplay * (1 + .32 / .6) - .72));
						BottomLeftCorner = ReferenceSystem.Origin();

						if (text[TextSegmentDelimiter] == '/')
						{ // display the text segment delimitier
							DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, TextSegmentDelimiter, 1, text);
						}
						StartPosition = TextSegmentDelimiter + 1;
						NumberOfCharactersToDisplay = EndSemicolon - StartPosition;
						//Offset the line position down
						ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, .35, .72));
						BottomLeftCorner = ReferenceSystem.Origin();

						if (NumberOfCharactersToDisplay > 0)
						{ // Display subscripted text segment
							DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
							StartPosition += NumberOfCharactersToDisplay;
						}
						ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, - .35, NumberOfCharactersToDisplay * (1 + .32 / .6)));
						BottomLeftCorner = ReferenceSystem.Origin();

						NumberOfCharactersToDisplay = 0;
						StartPosition = EndSemicolon + 1;
						CurrentPosition = StartPosition;
					}
				}
			}
		}
	}
	DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
}
int LengthSansFormattingCharacters(const CString& text)
{
	int Length = text.GetLength();
	int CurrentPosition = 0;

	while (CurrentPosition < text.GetLength()) 
	{
		WCHAR c = text[CurrentPosition++];
		if (c == '\\')
		{
			c = text[CurrentPosition];
			if (c == 'A')
			{
				int EndSemicolon = text.Find(';', CurrentPosition);
				if (EndSemicolon != - 1 && EndSemicolon == CurrentPosition + 2)
				{
					Length -= 4;
					CurrentPosition = EndSemicolon + 1;
				}
			}
			else if (c == 'P')
			{
				Length -= 2;
				CurrentPosition++;
			}
			else if (c == 'S') 
			{
				int EndSemicolon = text.Find(';', CurrentPosition);
				if (EndSemicolon != - 1)
				{
					int TextSegmentDelimiter = text.Find('/', CurrentPosition);
					if (TextSegmentDelimiter == - 1)
						TextSegmentDelimiter = text.Find('^', CurrentPosition);

					if (TextSegmentDelimiter != - 1 && TextSegmentDelimiter < EndSemicolon)
					{
						Length -= 4;
						CurrentPosition = EndSemicolon + 1;
					}
				}
			}
		}
	}
	return Length;
}
void GetBottomLeftCorner(EoDbFontDefinition& fd, int iChrs, EoGePoint3d& pt)
{
	if (iChrs > 0) 
	{
		double dTxtExt = iChrs + (iChrs - 1) * (.32 + fd.CharacterSpacing()) / .6;

		if (fd.Path() == EoDb::kPathRight || fd.Path() == EoDb::kPathLeft)
		{	
			if (fd.Path() == EoDb::kPathRight)
			{	
				if (fd.HorizontalAlignment() == EoDb::kAlignLeft)
					pt.x = 0.;
				else if (fd.HorizontalAlignment() == EoDb::kAlignCenter)
					pt.x = - dTxtExt * .5;
				else if (fd.HorizontalAlignment() == EoDb::kAlignRight)
					pt.x = - dTxtExt;
			}
			else
			{	
				if (fd.HorizontalAlignment() == EoDb::kAlignLeft)
					pt.x = dTxtExt;
				else if (fd.HorizontalAlignment() == EoDb::kAlignCenter)
					pt.x = dTxtExt * .5;
				else if (fd.HorizontalAlignment() == EoDb::kAlignRight)
					pt.x = 0.;
				pt.x = pt.x - 1.;
			}
			if (fd.VerticalAlignment() == EoDb::kAlignBottom)
				pt.y = 0.;
			else if (fd.VerticalAlignment() == EoDb::kAlignMiddle)
				pt.y = - .5;
			else if (fd.VerticalAlignment() == EoDb::kAlignTop)
				pt.y = - 1.;
		}
		else if (fd.Path() == EoDb::kPathDown || fd.Path() == EoDb::kPathUp)
		{	
			if (fd.HorizontalAlignment() == EoDb::kAlignLeft)
				pt.x = 0.;
			else if (fd.HorizontalAlignment() == EoDb::kAlignCenter)
				pt.x = - .5;
			else if (fd.HorizontalAlignment() == EoDb::kAlignRight)
				pt.x = - 1.;
			if (fd.Path() == EoDb::kPathUp)
			{	
				if (fd.VerticalAlignment() == EoDb::kAlignBottom)
					pt.y = 0.;
				else if (fd.VerticalAlignment() == EoDb::kAlignMiddle)
					pt.y = - dTxtExt * .5;
				else if (fd.VerticalAlignment() == EoDb::kAlignTop)
					pt.y = - dTxtExt;
			}
			else
			{	
				if (fd.VerticalAlignment() == EoDb::kAlignBottom)
					pt.y = dTxtExt;
				else if (fd.VerticalAlignment() == EoDb::kAlignMiddle)
					pt.y = dTxtExt * .5;
				else if (fd.VerticalAlignment() == EoDb::kAlignTop)
					pt.y = 0.;
				pt.y = pt.y - 1.;
			}
		}
	}
	else 
	{
		pt.x = 0.;
		pt.y = 0.;
	}
	pt.z = 0.;
}
void text_GetBoundingBox(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int numberOfCharacters, double spaceFactor, CPnts& ptsBox)
{
	ptsBox.SetSize(4);

	if (numberOfCharacters > 0) 
	{
		EoGeTransformMatrix tm(referenceSystem.TransformMatrix());
		tm.Inverse();

		double TextHeight = 1.;
		double TextWidth = 1.;

		double CharacterSpacing = (.32 + fd.CharacterSpacing()) / .6;
		double d = (double) numberOfCharacters + CharacterSpacing * (double) (numberOfCharacters - 1);

		if (fd.Path() == EoDb::kPathRight || fd.Path() == EoDb::kPathLeft)
		{
			TextWidth = d;
		}
		else
		{
			TextHeight = d;
		}
		ptsBox[0] = EoGePoint3d::kOrigin; 
		ptsBox[1] = ptsBox[0]; 
		ptsBox[2] = ptsBox[0]; 
		ptsBox[3] = ptsBox[0];

		if (fd.HorizontalAlignment() == EoDb::kAlignLeft)
		{
			ptsBox[2].x = TextWidth;
		}
		else if (fd.HorizontalAlignment() == EoDb::kAlignCenter)
		{
			ptsBox[0].x = - TextWidth * .5; 
			ptsBox[2].x = ptsBox[0].x + TextWidth;
		}
		else
		{
			ptsBox[0].x = - TextWidth;
		}
		if (fd.VerticalAlignment() == EoDb::kAlignTop)
		{
			ptsBox[0].y = - TextHeight;
		}
		else if (fd.VerticalAlignment() == EoDb::kAlignMiddle)
		{
			ptsBox[0].y = - TextHeight * .5; 
			ptsBox[2].y = ptsBox[0].y + TextHeight;
		}
		else
		{
			ptsBox[2].y = TextHeight;
		}
		if (spaceFactor > DBL_EPSILON) 
		{
			ptsBox[0].x -= spaceFactor / .6; 
			ptsBox[0].y -= spaceFactor;
			ptsBox[2].x += spaceFactor / .6; 
			ptsBox[2].y += spaceFactor;
		}
		ptsBox[1].x = ptsBox[2].x; 
		ptsBox[1].y = ptsBox[0].y;
		ptsBox[3].x = ptsBox[0].x; 
		ptsBox[3].y = ptsBox[2].y;

		for (int n = 0; n < 4; n++)
		{
			ptsBox[n] = tm * ptsBox[n];
		}
	}
	else
	{
		for (int n = 0; n < 4; n++)
		{
			ptsBox[n] = referenceSystem.Origin();
		}
	}
}
EoGePoint3d text_GetNewLinePos(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, double dLineSpaceFac, double dChrSpaceFac)
{

	EoGePoint3d pt = referenceSystem.Origin();
	EoGeVector3d vPath = referenceSystem.XDirection();
	EoGeVector3d YDirection = referenceSystem.YDirection();

	if (fd.Path() == EoDb::kPathRight || fd.Path() == EoDb::kPathLeft)
	{
		pt += vPath * dChrSpaceFac;
		EoGeVector3d vRefNorm;
		referenceSystem.GetUnitNormal(vRefNorm);

		vPath.Normalize();
		vPath *= - (YDirection.Length() * dLineSpaceFac);
		vPath.RotAboutArbAx(vRefNorm, HALF_PI);
	}
	return (pt + (vPath * 1.5));
}