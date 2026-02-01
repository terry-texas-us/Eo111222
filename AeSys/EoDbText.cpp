#include "Stdafx.h"

#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDlgTrapModify.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "PrimState.h"

namespace {
/** @brief Calculate the font escapement angle in tenths of degrees from the X axis vector.
 *  @param xAxis The X axis vector of the text reference system.
 */
int FontEscapementAngle(const EoGeVector3d& xAxis) {
  double angle = atan2(xAxis.y, xAxis.x);  // -π to π radians
  if (angle < 0.0) { angle += Eo::TwoPi; }
  return Eo::Round(Eo::RadianToDegree(angle) * 10.0);
}

/** @brief Determines if the text contains formatting characters.
 *  @param text The text to check for formatting characters.
 *  @return true if the text contains formatting characters; otherwise false.
 */
bool HasFormattingCharacters(const CString& text) {
  for (int i = 0; i < text.GetLength() - 1; i++) {
    if (text[i] == '\\') {
      switch (text[i + 1]) {  // Parameter Meaning
        case 'P':             //                Hard line break
                              //case '~':	//    Nonbreaking space
                              //case '/':	//    Single backslash; otherwise used as an escape character
                              //case '{':	//    Single opening curly bracket; otherwise used as block begin
                              //case '}':	//    Single closing curly bracket; otherwise used as block end
        case 'A':             // 0, 1, or 2     Change alignment to bottom, center, or top
                              //case 'C':	//    ACI color number  Change character color
                              //case 'F':	//    Font information  Change to a different font
                              //                  acad:	\FArial.shx
                              //                  windows	\FArial|b1|i0|c0|p34
                              //case 'H':	//    New height or relative  Change text height - height followed by an x
                              //case 'L':	//    Start underlining
                              //case 'l':	//    End underlining
                              //case 'O':	//    Start overlining
                              //case 'o':	//    End overlining
                              //case 'T':	//    Change kerning, i.e. character spacing
                              //case 'W':	//    Change character width, i.e X scaling
        case 'S':             //                Stacked text or fractions
                              //                  the S is follwed by two text segments separated by a / (fraction bar) or ^ (no fraction bar)
          return true;
      }
    }
  }
  return false;
}

}  // namespace
EoDbText::EoDbText(const EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text) {
  m_color = pstate.PenColor();
  m_fontDefinition = fd;
  m_ReferenceSystem = referenceSystem;
  m_strText = text;
}
EoDbText::EoDbText(const EoDbText& src) {
  m_color = src.m_color;
  m_fontDefinition = src.m_fontDefinition;
  m_ReferenceSystem = src.m_ReferenceSystem;
  m_strText = src.m_strText;
}
const EoDbText& EoDbText::operator=(const EoDbText& src) {
  m_color = src.m_color;
  m_fontDefinition = src.m_fontDefinition;
  m_ReferenceSystem = src.m_ReferenceSystem;
  m_strText = src.m_strText;

  return (*this);
}
void EoDbText::AddToTreeViewControl(HWND tree, HTREEITEM parent) {
  CString label{L"<Text>"};
  tvAddItem(tree, parent, label.GetBuffer(), this);
}

EoDbPrimitive*& EoDbText::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbText(*this);
  return (primitive);
}
void EoDbText::ConvertFormattingCharacters() {
  for (int i = 0; i < m_strText.GetLength() - 1; i++) {
    if (m_strText[i] == '^') {
      if (m_strText[i + 1] == '/') {  // Fractions
        int EndCaret = m_strText.Find('^', i + 1);

        if (EndCaret != -1) {
          int FractionBar = m_strText.Find('/', i + 2);
          if (FractionBar != -1 && FractionBar < EndCaret) {
            m_strText.SetAt(i++, '\\');
            m_strText.SetAt(i, 'S');
            m_strText.SetAt(EndCaret, ';');
            i = EndCaret;
          }
        }
      }
    }
  }
}
void EoDbText::Display(AeSysView* view, CDC* deviceContext) {
  EoInt16 color = LogicalColor();
  pstate.SetColor(deviceContext, color);

  EoInt16 LineType = pstate.LineType();
  pstate.SetLineType(deviceContext, 1);

  DisplayText(view, deviceContext, m_fontDefinition, m_ReferenceSystem, m_strText);
  pstate.SetLineType(deviceContext, LineType);
}
void EoDbText::AddReportToMessageList(EoGePoint3d) {
  CString str;
  str = L"Color: " + FormatPenColor() + L" Font: " + m_fontDefinition.FontName() + L" Precision: " + m_fontDefinition.FormatPrecision() + L" Path: " + m_fontDefinition.FormatPath() +
        L" Alignment: (" + m_fontDefinition.FormatHorizonatlAlignment() + L"," + m_fontDefinition.FormatVerticalAlignment() + L")";

  app.AddStringToMessageList(str);
}
void EoDbText::FormatExtra(CString& str) {
  str.Format(L"Color;%s\tFont;%s\tPrecision;%s\tPath;%s\tAlignment;(%s,%s)\tSpacing;%f\tLength;%d\tText;%s", FormatPenColor().GetString(),
             m_fontDefinition.FontName().GetString(), m_fontDefinition.FormatPrecision().GetString(), m_fontDefinition.FormatPath().GetString(), m_fontDefinition.FormatHorizonatlAlignment().GetString(),
             m_fontDefinition.FormatVerticalAlignment().GetString(), m_fontDefinition.CharacterSpacing(), m_strText.GetLength(), m_strText.GetString());
}
void EoDbText::FormatGeometry(CString& str) {
  EoGeReferenceSystem ReferenceSystem = m_ReferenceSystem;
  EoGePoint3d Origin = ReferenceSystem.Origin();

  str += L"Origin;" + Origin.ToString();
  str += L"X Axis;" + ReferenceSystem.XDirection().ToString();
  str += L"Y Axis;" + ReferenceSystem.YDirection().ToString();
}
void EoDbText::GetBoundingBox(EoGePoint3dArray& ptsBox, double spaceFactor) {
  int Length = LengthSansFormattingCharacters(m_strText);
  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, Length, spaceFactor, ptsBox);
}

void EoDbText::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  auto origin = m_ReferenceSystem.Origin();
  points.Add(origin);
}

EoGePoint3d EoDbText::GetControlPoint() { return m_ReferenceSystem.Origin(); }
void EoDbText::GetExtents(AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, EoGeTransformMatrix& tm) {
  EoGePoint3dArray pts;

  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText.GetLength(), 0.0, pts);

  for (EoUInt16 w = 0; w < pts.GetSize(); w++) {
    view->ModelTransformPoint(pts[w]);
    pts[w] = tm * pts[w];
    ptMin = EoGePoint3d::Min(ptMin, pts[w]);
    ptMax = EoGePoint3d::Max(ptMax, pts[w]);
  }
}
bool EoDbText::IsInView(AeSysView* view) {
  EoGePoint4d pt[2]{};

  EoGePoint3dArray pts;

  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText.GetLength(), 0.0, pts);

  for (INT_PTR n = 0; n <= 2;) {
    pt[0] = pts[n++];
    pt[1] = pts[n++];

    view->ModelViewTransformPoints(2, pt);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) return true;
  }
  return false;
}
bool EoDbText::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint3dArray pts;
  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText.GetLength(), 0.0, pts);
  return polyline::SelectUsingRectangle(view, pt1, pt2, pts);
}
bool EoDbText::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt(m_ReferenceSystem.Origin());
  view->ModelViewTransformPoint(pt);

  return ((point.DistanceToPointXY(pt) < sm_SelectApertureSize) ? true : false);
}
void EoDbText::ModifyState() {
  EoDbPrimitive::ModifyState();

  pstate.GetFontDef(m_fontDefinition);

  EoDbCharacterCellDefinition ccd;
  pstate.GetCharCellDef(ccd);

  m_ReferenceSystem.Rescale(ccd);
}
void EoDbText::ModifyNotes(EoDbFontDefinition& fd, EoDbCharacterCellDefinition& ccd, int iAtt) {
  if (iAtt == TM_TEXT_ALL) {
    m_color = pstate.PenColor();
    m_fontDefinition = fd;
    m_ReferenceSystem.Rescale(ccd);
  } else if (iAtt == TM_TEXT_FONT) {
    m_fontDefinition.FontName(fd.FontName());
    m_fontDefinition.Precision(fd.Precision());
  } else if (iAtt == TM_TEXT_HEIGHT) {
    m_fontDefinition.CharacterSpacing(fd.CharacterSpacing());
    m_fontDefinition.Path(fd.Path());

    m_ReferenceSystem.Rescale(ccd);
  }
}
EoGePoint3d EoDbText::SelectAtControlPoint(AeSysView*, const EoGePoint4d& point) {
  sm_ControlPointIndex = USHRT_MAX;
  return (point);
}
bool EoDbText::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  if (m_strText.GetLength() == 0) return false;

  EoGePoint3dArray pts;

  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText.GetLength(), 0.0, pts);

  EoGePoint4d pt0[] = {EoGePoint4d(pts[0]), EoGePoint4d(pts[1]), EoGePoint4d(pts[2]), EoGePoint4d(pts[3])};

  view->ModelViewTransformPoints(4, pt0);

  for (size_t n = 0; n < 4; n++) {
    if (EoGeLine(pt0[n], pt0[(n + 1) % 4]).DirRelOfPt(point) < 0) return false;
  }
  ptProj = point;

  return true;
}
void EoDbText::Transform(EoGeTransformMatrix& tm) { m_ReferenceSystem.Transform(tm); }
void EoDbText::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v);
}
bool EoDbText::Write(CFile& file) {
  EoDb::Write(file, EoUInt16(EoDb::kTextPrimitive));
  EoDb::Write(file, m_color);
  EoDb::Write(file, m_lineTypeIndex);
  m_fontDefinition.Write(file);
  m_ReferenceSystem.Write(file);
  EoDb::Write(file, m_strText);

  return true;
}

void DisplayText(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text) {
  if (text.IsEmpty()) return;

  if (HasFormattingCharacters(text)) {
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
  while (CurrentPosition < text.GetLength()) {
    wchar_t c = text[CurrentPosition++];

    if (c == '\r' && text[CurrentPosition] == '\n') {
      DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

      ReferenceSystem.SetOrigin(BottomLeftCorner);
      ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 1.0, 0));
      BottomLeftCorner = ReferenceSystem.Origin();

      StartPosition += 2 + NumberOfCharactersToDisplay;
      CurrentPosition = StartPosition;
      NumberOfCharactersToDisplay = 0;
    } else {
      NumberOfCharactersToDisplay++;
    }
  }
  DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
}
void DisplayTextSegment(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition,
                        int numberOfCharacters, const CString& text) {
  if (deviceContext != 0 && fd.Precision() == EoDb::kEoTrueType && view->ViewTrueTypeFonts()) {
    EoGeVector3d XDirection(referenceSystem.XDirection());
    EoGeVector3d YDirection(referenceSystem.YDirection());

    view->ModelViewTransformVector(XDirection);
    view->ModelViewTransformVector(YDirection);

    auto normal = CrossProduct(XDirection, YDirection);
    normal.Normalize();

    if (normal == EoGeVector3d::positiveUnitZ) {
      if (DisplayTextSegmentUsingTrueTypeFont(view, deviceContext, fd, referenceSystem, startPosition, numberOfCharacters, text)) return;
    }
  }
  DisplayTextSegmentUsingStrokeFont(view, deviceContext, fd, referenceSystem, startPosition, numberOfCharacters, text);
}
void DisplayTextSegmentUsingStrokeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition,
                                       int numberOfCharacters, const CString& text) {
  if (numberOfCharacters == 0) return;

  long* plStrokeFontDef = (long*)app.SimplexStrokeFont();
  if (plStrokeFontDef == 0) return;

  EoGeTransformMatrix tm(referenceSystem.TransformMatrix());
  tm.Inverse();

  long* plStrokeChrDef = plStrokeFontDef + 96;
  double dChrSpac = 1.0 + (0.32 + fd.CharacterSpacing()) / 0.6;

  EoGePoint3d ptStroke = EoGePoint3d::kOrigin;
  EoGePoint3d ptChrPos = ptStroke;
  EoGePoint3d ptLinePos = ptChrPos;

  int n = startPosition;

  while (n < startPosition + numberOfCharacters) {
    polyline::BeginLineStrip();

    int Character = text.GetAt(n);
    if (Character < 32 || Character > 126) Character = '.';

    for (int i = (int)plStrokeFontDef[Character - 32]; i <= plStrokeFontDef[Character - 32 + 1] - 1; i++) {
      int iY = (int)(plStrokeChrDef[i - 1] % 4096L);
      if ((iY & 2048) != 0) iY = -(iY - 2048);
      int iX = (int)((plStrokeChrDef[i - 1] / 4096L) % 4096L);
      if ((iX & 2048) != 0) iX = -(iX - 2048);

      ptStroke += EoGeVector3d(0.01 / 0.6 * iX, 0.01 * iY, 0.0);

      if (plStrokeChrDef[i - 1] / 16777216 == 5) {
        polyline::__End(view, deviceContext, 1);
        polyline::BeginLineStrip();
      }
      polyline::SetVertex(tm * ptStroke);
    }
    polyline::__End(view, deviceContext, 1);

    switch (fd.Path()) {
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

bool DisplayTextSegmentUsingTrueTypeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition,
                                         int numberOfCharacters, const CString& text) {
  if (numberOfCharacters <= 0) return true;

  EoGeTransformMatrix tm(referenceSystem.TransformMatrix());
  tm.Inverse();

  EoGePoint4d StartPoint = tm * EoGePoint3d::kOrigin;
  view->ModelViewTransformPoint(StartPoint);
  CPoint ProjectedStartPoint = view->DoProjection(StartPoint);

  EoGePoint4d ptsBox[3]{};

  ptsBox[1] = tm * EoGePoint3d(0.0, 1.0, 0.0);
  ptsBox[2] = tm * EoGePoint3d(1.0, 0.0, 0.0);

  view->ModelViewTransformPoint(ptsBox[1]);
  view->ModelViewTransformPoint(ptsBox[2]);

  CPoint pnt[4]{};

  pnt[1] = view->DoProjection(ptsBox[1]);
  pnt[2] = view->DoProjection(ptsBox[2]);

  EoGeVector3d vX(double(pnt[2].x - ProjectedStartPoint.x), double(pnt[2].y - ProjectedStartPoint.y), 0.0);
  EoGeVector3d vY(double(pnt[1].x - ProjectedStartPoint.x), double(pnt[1].y - ProjectedStartPoint.y), 0.0);

  double dHeight = vY.Length();
  if (dHeight == 0.0) { return true; }
  LOGFONT logfont{};
  logfont.lfHeight = -Eo::Round(1.33 * dHeight);
  logfont.lfEscapement = -FontEscapementAngle(vX);
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
  CFont* pfntold = (CFont*)deviceContext->SelectObject(&font);
  UINT uTextAlign = deviceContext->SetTextAlign(TA_LEFT | TA_BASELINE);
  int iBkMode = deviceContext->SetBkMode(TRANSPARENT);

  deviceContext->TextOutW(ProjectedStartPoint.x, ProjectedStartPoint.y, (LPCWSTR)text.Mid(startPosition), numberOfCharacters);
  deviceContext->SetBkMode(iBkMode);
  deviceContext->SetTextAlign(uTextAlign);
  deviceContext->SelectObject(pfntold);

  return true;
}
void DisplayTextWithFormattingCharacters(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem,
                                         const CString& text) {
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

  while (CurrentPosition < text.GetLength()) {
    wchar_t c = text[CurrentPosition++];
    if (c != '\\') {
      NumberOfCharactersToDisplay++;
    } else {
      c = text[CurrentPosition];
      if (c == 'P') {  // Hard line bresk
        if (CurrentPosition < text.GetLength()) {
          DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

          ReferenceSystem.SetOrigin(BottomLeftCorner);
          ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 1.0, 0));
          BottomLeftCorner = ReferenceSystem.Origin();
          StartPosition += 2 + NumberOfCharactersToDisplay;
          CurrentPosition = StartPosition;
          NumberOfCharactersToDisplay = 0;
        }
      } else if (c == 'A') {  // Change alignment to bottom, center middle
        int EndSemicolon = text.Find(';', CurrentPosition);
        if (EndSemicolon != -1) {
          if (CurrentPosition + 1 < EndSemicolon) {
            wchar_t Parameter = text[CurrentPosition + 1];
            if (Parameter >= '0' && Parameter <= '2') {
              if (NumberOfCharactersToDisplay > 0) {  // display text segment preceding the formatting
                DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

                // Offset the line position left of current position
                ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, referenceSystem, 0.0, NumberOfCharactersToDisplay * (1 + 0.32 / 0.6)));
                BottomLeftCorner = ReferenceSystem.Origin();
              }
              StartPosition = EndSemicolon + 1;
              CurrentPosition = StartPosition;
              NumberOfCharactersToDisplay = text.GetLength() - StartPosition;

              if (Parameter == '1') {
                ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 0.5, 0.0));
              } else if (Parameter == '2') {
                ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, -0.5, 0.0));
              }
              DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
              return;
            }
          }
        }
      } else if (c == 'S') {  // Stacked text or fractions
        int EndSemicolon = text.Find(';', CurrentPosition);
        if (EndSemicolon != -1) {
          int TextSegmentDelimiter = text.Find('/', CurrentPosition);
          if (TextSegmentDelimiter == -1) TextSegmentDelimiter = text.Find('^', CurrentPosition);

          if (TextSegmentDelimiter != -1 && TextSegmentDelimiter < EndSemicolon) {
            if (NumberOfCharactersToDisplay > 0) {  // display text segment preceding the formatting
              DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
              StartPosition += NumberOfCharactersToDisplay;
            }
            // Offset the line position up and conditionally left of current position
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, -0.35, NumberOfCharactersToDisplay * (1 + 0.32 / 0.6)));
            BottomLeftCorner = ReferenceSystem.Origin();
            StartPosition += 2;  // skip the formatting characters
            NumberOfCharactersToDisplay = TextSegmentDelimiter - StartPosition;
            if (NumberOfCharactersToDisplay > 0) {  // Display superscripted text segment
              DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
              StartPosition += NumberOfCharactersToDisplay;
            }
            // Offset the line position back down left
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 0.35, NumberOfCharactersToDisplay * (1 + 0.32 / 0.6) - 0.72));
            BottomLeftCorner = ReferenceSystem.Origin();

            if (text[TextSegmentDelimiter] == '/') {  // display the text segment delimitier
              DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, TextSegmentDelimiter, 1, text);
            }
            StartPosition = TextSegmentDelimiter + 1;
            NumberOfCharactersToDisplay = EndSemicolon - StartPosition;
            //Offset the line position down
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 0.35, 0.72));
            BottomLeftCorner = ReferenceSystem.Origin();

            if (NumberOfCharactersToDisplay > 0) {  // Display subscripted text segment
              DisplayTextSegment(view, deviceContext, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
              StartPosition += NumberOfCharactersToDisplay;
            }
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, -0.35, NumberOfCharactersToDisplay * (1 + 0.32 / 0.6)));
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
int LengthSansFormattingCharacters(const CString& text) {
  int Length = text.GetLength();
  int CurrentPosition = 0;

  while (CurrentPosition < text.GetLength()) {
    wchar_t c = text[CurrentPosition++];
    if (c == '\\') {
      c = text[CurrentPosition];
      if (c == 'A') {
        int EndSemicolon = text.Find(';', CurrentPosition);
        if (EndSemicolon != -1 && EndSemicolon == CurrentPosition + 2) {
          Length -= 4;
          CurrentPosition = EndSemicolon + 1;
        }
      } else if (c == 'P') {
        Length -= 2;
        CurrentPosition++;
      } else if (c == 'S') {
        int EndSemicolon = text.Find(';', CurrentPosition);
        if (EndSemicolon != -1) {
          int TextSegmentDelimiter = text.Find('/', CurrentPosition);
          if (TextSegmentDelimiter == -1) TextSegmentDelimiter = text.Find('^', CurrentPosition);

          if (TextSegmentDelimiter != -1 && TextSegmentDelimiter < EndSemicolon) {
            Length -= 4;
            CurrentPosition = EndSemicolon + 1;
          }
        }
      }
    }
  }
  return Length;
}
void GetBottomLeftCorner(EoDbFontDefinition& fd, int iChrs, EoGePoint3d& pt) {
  if (iChrs > 0) {
    double dTxtExt = iChrs + (iChrs - 1) * (0.32 + fd.CharacterSpacing()) / 0.6;

    if (fd.Path() == EoDb::kPathRight || fd.Path() == EoDb::kPathLeft) {
      if (fd.Path() == EoDb::kPathRight) {
        if (fd.HorizontalAlignment() == EoDb::kAlignLeft)
          pt.x = 0.;
        else if (fd.HorizontalAlignment() == EoDb::kAlignCenter)
          pt.x = -dTxtExt * 0.5;
        else if (fd.HorizontalAlignment() == EoDb::kAlignRight)
          pt.x = -dTxtExt;
      } else {
        if (fd.HorizontalAlignment() == EoDb::kAlignLeft)
          pt.x = dTxtExt;
        else if (fd.HorizontalAlignment() == EoDb::kAlignCenter)
          pt.x = dTxtExt * 0.5;
        else if (fd.HorizontalAlignment() == EoDb::kAlignRight)
          pt.x = 0.;
        pt.x = pt.x - 1.;
      }
      if (fd.VerticalAlignment() == EoDb::kAlignBottom)
        pt.y = 0.;
      else if (fd.VerticalAlignment() == EoDb::kAlignMiddle)
        pt.y = -0.5;
      else if (fd.VerticalAlignment() == EoDb::kAlignTop)
        pt.y = -1.;
    } else if (fd.Path() == EoDb::kPathDown || fd.Path() == EoDb::kPathUp) {
      if (fd.HorizontalAlignment() == EoDb::kAlignLeft)
        pt.x = 0.;
      else if (fd.HorizontalAlignment() == EoDb::kAlignCenter)
        pt.x = -0.5;
      else if (fd.HorizontalAlignment() == EoDb::kAlignRight)
        pt.x = -1.;
      if (fd.Path() == EoDb::kPathUp) {
        if (fd.VerticalAlignment() == EoDb::kAlignBottom)
          pt.y = 0.;
        else if (fd.VerticalAlignment() == EoDb::kAlignMiddle)
          pt.y = -dTxtExt * 0.5;
        else if (fd.VerticalAlignment() == EoDb::kAlignTop)
          pt.y = -dTxtExt;
      } else {
        if (fd.VerticalAlignment() == EoDb::kAlignBottom)
          pt.y = dTxtExt;
        else if (fd.VerticalAlignment() == EoDb::kAlignMiddle)
          pt.y = dTxtExt * 0.5;
        else if (fd.VerticalAlignment() == EoDb::kAlignTop)
          pt.y = 0.;
        pt.y = pt.y - 1.;
      }
    }
  } else {
    pt.x = 0.;
    pt.y = 0.;
  }
  pt.z = 0.;
}
void text_GetBoundingBox(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int numberOfCharacters, double spaceFactor, EoGePoint3dArray& ptsBox) {
  ptsBox.SetSize(4);

  if (numberOfCharacters > 0) {
    EoGeTransformMatrix tm(referenceSystem.TransformMatrix());
    tm.Inverse();

    double TextHeight = 1.;
    double TextWidth = 1.;

    double CharacterSpacing = (0.32 + fd.CharacterSpacing()) / 0.6;
    double d = (double)numberOfCharacters + CharacterSpacing * (double)(numberOfCharacters - 1);

    if (fd.Path() == EoDb::kPathRight || fd.Path() == EoDb::kPathLeft) {
      TextWidth = d;
    } else {
      TextHeight = d;
    }
    ptsBox[0] = EoGePoint3d::kOrigin;
    ptsBox[1] = ptsBox[0];
    ptsBox[2] = ptsBox[0];
    ptsBox[3] = ptsBox[0];

    if (fd.HorizontalAlignment() == EoDb::kAlignLeft) {
      ptsBox[2].x = TextWidth;
    } else if (fd.HorizontalAlignment() == EoDb::kAlignCenter) {
      ptsBox[0].x = -TextWidth * 0.5;
      ptsBox[2].x = ptsBox[0].x + TextWidth;
    } else {
      ptsBox[0].x = -TextWidth;
    }
    if (fd.VerticalAlignment() == EoDb::kAlignTop) {
      ptsBox[0].y = -TextHeight;
    } else if (fd.VerticalAlignment() == EoDb::kAlignMiddle) {
      ptsBox[0].y = -TextHeight * 0.5;
      ptsBox[2].y = ptsBox[0].y + TextHeight;
    } else {
      ptsBox[2].y = TextHeight;
    }
    if (spaceFactor > Eo::geometricTolerance) {
      ptsBox[0].x -= spaceFactor / 0.6;
      ptsBox[0].y -= spaceFactor;
      ptsBox[2].x += spaceFactor / 0.6;
      ptsBox[2].y += spaceFactor;
    }
    ptsBox[1].x = ptsBox[2].x;
    ptsBox[1].y = ptsBox[0].y;
    ptsBox[3].x = ptsBox[0].x;
    ptsBox[3].y = ptsBox[2].y;

    for (int n = 0; n < 4; n++) { ptsBox[n] = tm * ptsBox[n]; }
  } else {
    for (int n = 0; n < 4; n++) { ptsBox[n] = referenceSystem.Origin(); }
  }
}
EoGePoint3d text_GetNewLinePos(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, double dLineSpaceFac, double dChrSpaceFac) {
  EoGePoint3d pt = referenceSystem.Origin();
  EoGeVector3d vPath = referenceSystem.XDirection();
  EoGeVector3d YDirection = referenceSystem.YDirection();

  if (fd.Path() == EoDb::kPathRight || fd.Path() == EoDb::kPathLeft) {
    pt += vPath * dChrSpaceFac;
    EoGeVector3d vRefNorm;
    referenceSystem.GetUnitNormal(vRefNorm);

    vPath.Normalize();
    vPath *= -(YDirection.Length() * dLineSpaceFac);
    vPath.RotAboutArbAx(vRefNorm, Eo::HalfPi);
  }
  return (pt + (vPath * 1.5));
}
