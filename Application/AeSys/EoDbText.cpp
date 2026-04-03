#include "Stdafx.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "AeSys.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"
#include "EoDbLineTypeTable.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDlgTrapModify.h"
#include "EoDxfInterface.h"
#include "EoDxfText.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGePolyline.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDevice.h"
#include "EoGsRenderState.h"

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
        case 'P':  //                Hard line break
                   // case '~':	//    Nonbreaking space
                   // case '/':	//    Single backslash; otherwise used as an escape character
                   // case '{':	//    Single opening curly bracket; otherwise used as block begin
                   // case '}':	//    Single closing curly bracket; otherwise used as block end
        case 'A':  // 0, 1, or 2     Change alignment to bottom, center, or top
                   // case 'C':	//    ACI color number  Change character color
                   // case 'F':	//    Font information  Change to a different font
                   //                  acad:	\FArial.shx
                   //                  windows	\FArial|b1|i0|c0|p34
                   // case 'H':	//    New height or relative  Change text height - height followed by an x
                   // case 'L':	//    Start underlining
                   // case 'l':	//    End underlining
                   // case 'O':	//    Start overlining
                   // case 'o':	//    End overlining
                   // case 'T':	//    Change kerning, i.e. character spacing
                   // case 'W':	//    Change character width, i.e X scaling
        case 'S':  //                Stacked text or fractions
          //                  the S is follwed by two text segments separated by a / (fraction bar) or ^ (no fraction
          //                  bar)
          return true;
      }
    }
  }
  return false;
}

}  // namespace

EoDbText::EoDbText(const EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text)
    : EoDbPrimitive(), m_fontDefinition{fd}, m_ReferenceSystem{referenceSystem}, m_strText{text} {
  m_color = Gs::renderState.Color();
}

EoDbText::EoDbText(const EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const std::wstring& text)
    : EoDbPrimitive(), m_fontDefinition{fd}, m_ReferenceSystem{referenceSystem}, m_strText{text.c_str()} {
  m_color = Gs::renderState.Color();
}

EoDbText::EoDbText(const EoDbText& other) {
  m_color = other.m_color;
  m_fontDefinition = other.m_fontDefinition;
  m_ReferenceSystem = other.m_ReferenceSystem;
  m_strText = other.m_strText;
  m_textGenerationFlags = other.m_textGenerationFlags;
  m_extrusion = other.m_extrusion;
  m_mtextProperties = other.m_mtextProperties;
}

const EoDbText& EoDbText::operator=(const EoDbText& other) {
  m_color = other.m_color;
  m_fontDefinition = other.m_fontDefinition;
  m_ReferenceSystem = other.m_ReferenceSystem;
  m_strText = other.m_strText;
  m_textGenerationFlags = other.m_textGenerationFlags;
  m_extrusion = other.m_extrusion;
  m_mtextProperties = other.m_mtextProperties;

  return (*this);
}

void EoDbText::AddToTreeViewControl(HWND tree, HTREEITEM parent) { tvAddItem(tree, parent, L"<Text>", this); }

EoDbPrimitive*& EoDbText::Copy(EoDbPrimitive*& primitive) {
  primitive = new EoDbText(*this);
  return primitive;
}

void EoDbText::ConvertFormattingCharacters() {
  for (int i = 0; i < m_strText.GetLength() - 1; i++) {
    if (m_strText[i] != '^') { continue; }
    if (m_strText[i + 1] != '/') { continue; }

    int endCaret = m_strText.Find('^', i + 1);
    if (endCaret == -1) { continue; }

    int fractionBar = m_strText.Find('/', i + 2);
    if (fractionBar == -1 || fractionBar >= endCaret) { continue; }

    m_strText.SetAt(i++, '\\');
    m_strText.SetAt(i, 'S');
    m_strText.SetAt(endCaret, ';');
    i = endCaret;
  }
}

void EoDbText::Display(AeSysView* view, EoGsRenderDevice* renderDevice) {
  std::int16_t color = LogicalColor();
  Gs::renderState.SetColor(renderDevice, color);

  std::int16_t lineTypeIndex = Gs::renderState.LineTypeIndex();
  Gs::renderState.SetLineType(renderDevice, 1);

  DisplayText(view, renderDevice, m_fontDefinition, m_ReferenceSystem, m_strText);
  Gs::renderState.SetLineType(renderDevice, lineTypeIndex);
}

void EoDbText::ExportToDxf(EoDxfInterface* writer) const {
  if (m_mtextProperties.has_value()) {
    ExportAsMText(writer);
    return;
  }

  EoDxfText text;
  PopulateDxfBaseProperties(&text);

  text.m_string = std::wstring(m_strText.GetString());
  text.m_textStyleName = std::wstring(m_fontDefinition.FontName().GetString());

  // Recover DXF properties from the reference system
  const double textHeight = m_ReferenceSystem.YDirection().Length();
  text.m_textHeight = textHeight;

  const double xDirLength = m_ReferenceSystem.XDirection().Length();
  if (textHeight > Eo::geometricTolerance) {
    text.m_scaleFactorWidth = xDirLength / (textHeight * Eo::defaultCharacterCellAspectRatio);
  }

  // Rotation angle from xDirection — DXF TEXT group 50 is in degrees
  const auto& xDir = m_ReferenceSystem.XDirection();
  text.m_textRotation = Eo::RadianToDegree(std::atan2(xDir.y, xDir.x));

  // Recover oblique angle from Y-direction shear — DXF TEXT group 51 is in degrees
  // Import bakes oblique angle into the reference system by rotating Y toward X by -obliqueAngle.
  // The dot product of unitized X and Y directions equals sin(obliqueAngle) since the angle between
  // them is (90° - obliqueAngle) instead of the perpendicular 90°.
  if (textHeight > Eo::geometricTolerance && xDirLength > Eo::geometricTolerance) {
    const auto xUnit = xDir * (1.0 / xDirLength);
    const auto yUnit = m_ReferenceSystem.YDirection() * (1.0 / textHeight);
    const double sinOblique = DotProduct(xUnit, yUnit);
    text.m_obliqueAngle = Eo::RadianToDegree(std::asin(std::clamp(sinOblique, -1.0, 1.0)));
  }

  // Map AeSys alignment to DXF alignment
  switch (m_fontDefinition.HorizontalAlignment()) {
    case EoDb::HorizontalAlignment::Center:
      text.m_horizontalAlignment = EoDxfText::HorizontalAlignment::Center;
      break;
    case EoDb::HorizontalAlignment::Right:
      text.m_horizontalAlignment = EoDxfText::HorizontalAlignment::Right;
      break;
    case EoDb::HorizontalAlignment::Left:
      [[fallthrough]];
    default:
      text.m_horizontalAlignment = EoDxfText::HorizontalAlignment::Left;
      break;
  }

  switch (m_fontDefinition.VerticalAlignment()) {
    case EoDb::VerticalAlignment::Top:
      text.m_verticalAlignment = EoDxfText::VerticalAlignment::Top;
      break;
    case EoDb::VerticalAlignment::Middle:
      text.m_verticalAlignment = EoDxfText::VerticalAlignment::Middle;
      break;
    case EoDb::VerticalAlignment::Bottom:
      [[fallthrough]];
    default:
      text.m_verticalAlignment = EoDxfText::VerticalAlignment::Bottom;
      break;
  }

  const auto origin = m_ReferenceSystem.Origin();

  // DXF TEXT: Left+Baseline uses first alignment point; all other alignments use second alignment point
  const bool isDefaultAlignment = (text.m_horizontalAlignment == EoDxfText::HorizontalAlignment::Left &&
      text.m_verticalAlignment == EoDxfText::VerticalAlignment::BaseLine);
  if (isDefaultAlignment) {
    text.m_firstAlignmentPoint = {origin.x, origin.y, origin.z};
  } else {
    text.m_firstAlignmentPoint = {origin.x, origin.y, origin.z};
    text.m_secondAlignmentPoint = {origin.x, origin.y, origin.z};
  }

  text.m_textGenerationFlags = m_textGenerationFlags;
  text.m_extrusionDirection = {m_extrusion.x, m_extrusion.y, m_extrusion.z};

  writer->AddText(text);
}

void EoDbText::ExportAsMText(EoDxfInterface* writer) const {
  EoDxfMText mtext;
  PopulateDxfBaseProperties(&mtext);

  mtext.m_textString = std::wstring(m_strText.GetString());
  mtext.m_textStyleName = std::wstring(m_fontDefinition.FontName().GetString());

  // Recover text height from the reference system
  mtext.m_nominalTextHeight = m_ReferenceSystem.YDirection().Length();

  // Rotation angle from xDirection — MTEXT group 50 is in radians
  const auto& xDir = m_ReferenceSystem.XDirection();
  mtext.m_rotationAngle = std::atan2(xDir.y, xDir.x);

  // Insertion point from origin
  const auto origin = m_ReferenceSystem.Origin();
  mtext.m_insertionPoint = {origin.x, origin.y, origin.z};

  mtext.m_extrusionDirection = {m_extrusion.x, m_extrusion.y, m_extrusion.z};

  // Restore preserved MTEXT-specific properties
  const auto& properties = *m_mtextProperties;
  mtext.m_attachmentPoint = static_cast<EoDxfMText::AttachmentPoint>(properties.attachmentPoint);
  mtext.m_drawingDirection = static_cast<EoDxfMText::DrawingDirection>(properties.drawingDirection);
  mtext.m_lineSpacingStyle = static_cast<EoDxfMText::LineSpacingStyle>(properties.lineSpacingStyle);
  mtext.m_lineSpacingFactor = properties.lineSpacingFactor;
  mtext.m_referenceRectangleWidth = properties.referenceRectangleWidth;

  writer->AddMText(mtext);
}

void EoDbText::AddReportToMessageList(const EoGePoint3d& point) {
  app.AddStringToMessageList(m_mtextProperties.has_value() ? CString(L"<Text (MTEXT)>") : CString(L"<Text>"));
  EoDbPrimitive::AddReportToMessageList(point);

  CString message;
  message = L"  Font: " + m_fontDefinition.FontName() + L" Precision: " + m_fontDefinition.FormatPrecision() +
      L" Path: " + m_fontDefinition.FormatPath() + L" Alignment: (" + m_fontDefinition.FormatHorizontalAlignment() +
      L"," + m_fontDefinition.FormatVerticalAlignment() + L")";
  app.AddStringToMessageList(message);

  if (m_mtextProperties.has_value()) {
    const auto& properties = *m_mtextProperties;
    CString mtextMessage;
    mtextMessage.Format(
        L"  MTEXT: AttachmentPoint=%d  DrawingDirection=%d  LineSpacing=%.3f (style %d)  RectWidth=%.4f",
        properties.attachmentPoint, properties.drawingDirection, properties.lineSpacingFactor,
        properties.lineSpacingStyle, properties.referenceRectangleWidth);
    app.AddStringToMessageList(mtextMessage);
  }
}

void EoDbText::FormatExtra(CString& str) {
  EoDbPrimitive::FormatExtra(str);
  str.AppendFormat(L"\tFont;%s\tPrecision;%s\tPath;%s\tAlignment;(%s,%s)\tSpacing;%f\tLength;%d\tSource;%s\tText;%s",
      m_fontDefinition.FontName().GetString(), m_fontDefinition.FormatPrecision().GetString(),
      m_fontDefinition.FormatPath().GetString(), m_fontDefinition.FormatHorizontalAlignment().GetString(),
      m_fontDefinition.FormatVerticalAlignment().GetString(), m_fontDefinition.CharacterSpacing(),
      m_strText.GetLength(), m_mtextProperties.has_value() ? L"MTEXT" : L"TEXT", m_strText.GetString());
  str += L'\t';
}
void EoDbText::FormatGeometry(CString& str) {
  EoGeReferenceSystem ReferenceSystem = m_ReferenceSystem;
  EoGePoint3d Origin = ReferenceSystem.Origin();

  str += L"Origin;" + Origin.ToString();
  str += L"X Axis;" + ReferenceSystem.XDirection().ToString();
  str += L"Y Axis;" + ReferenceSystem.YDirection().ToString();
}
void EoDbText::GetBoundingBox(EoGePoint3dArray& ptsBox, double spaceFactor) {
  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText, spaceFactor, ptsBox);
}

void EoDbText::GetAllPoints(EoGePoint3dArray& points) {
  points.SetSize(0);
  auto origin = m_ReferenceSystem.Origin();
  points.Add(origin);
}

EoGePoint3d EoDbText::GetControlPoint() { return m_ReferenceSystem.Origin(); }
void EoDbText::GetExtents(
    AeSysView* view, EoGePoint3d& ptMin, EoGePoint3d& ptMax, const EoGeTransformMatrix& transformMatrix) {
  EoGePoint3dArray pts;

  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText, 0.0, pts);

  for (auto i = 0; i < pts.GetSize(); i++) {
    view->ModelTransformPoint(pts[i]);
    pts[i] = transformMatrix * pts[i];
    ptMin = EoGePoint3d::Min(ptMin, pts[i]);
    ptMax = EoGePoint3d::Max(ptMax, pts[i]);
  }
}

bool EoDbText::IsInView(AeSysView* view) {
  EoGePoint4d pt[2]{};

  EoGePoint3dArray pts;

  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText, 0.0, pts);

  for (INT_PTR n = 0; n <= 2;) {
    pt[0] = EoGePoint4d{pts[n++]};
    pt[1] = EoGePoint4d{pts[n++]};

    view->ModelViewTransformPoints(2, pt);

    if (EoGePoint4d::ClipLine(pt[0], pt[1])) { return true; }
  }
  return false;
}
bool EoDbText::SelectUsingRectangle(AeSysView* view, EoGePoint3d pt1, EoGePoint3d pt2) {
  EoGePoint3dArray pts;
  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText, 0.0, pts);
  return polyline::SelectUsingRectangle(view, pt1, pt2, pts);
}
bool EoDbText::IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) {
  EoGePoint4d pt(m_ReferenceSystem.Origin());
  view->ModelViewTransformPoint(pt);

  return ((point.DistanceToPointXY(pt) < sm_SelectApertureSize) ? true : false);
}

void EoDbText::ModifyState() {
  EoDbPrimitive::ModifyState();

  m_fontDefinition = Gs::renderState.FontDefinition();
  auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();
  m_ReferenceSystem.Rescale(characterCellDefinition);
}

void EoDbText::ModifyNotes(const EoDbFontDefinition& fontDefinition,
    const EoDbCharacterCellDefinition& characterCellDefinition, int attributes) {
  if (attributes == TM_TEXT_ALL) {
    m_color = Gs::renderState.Color();
    m_fontDefinition = fontDefinition;
    m_ReferenceSystem.Rescale(characterCellDefinition);
  } else if (attributes == TM_TEXT_FONT) {
    m_fontDefinition.SetFontName(fontDefinition.FontName());
    m_fontDefinition.SetPrecision(fontDefinition.Precision());
  } else if (attributes == TM_TEXT_HEIGHT) {
    m_fontDefinition.SetCharacterSpacing(fontDefinition.CharacterSpacing());
    m_fontDefinition.SetPath(fontDefinition.Path());

    m_ReferenceSystem.Rescale(characterCellDefinition);
  }
}
EoGePoint3d EoDbText::SelectAtControlPoint(AeSysView*, const EoGePoint4d& point) {
  sm_controlPointIndex = SHRT_MAX;
  return EoGePoint3d{point};
}

bool EoDbText::SelectUsingLine(
    [[maybe_unused]] AeSysView* view, [[maybe_unused]] EoGeLine line, [[maybe_unused]] EoGePoint3dArray&) {
  return false;
}

bool EoDbText::SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d& ptProj) {
  if (m_strText.GetLength() == 0) { return false; }

  EoGePoint3dArray pts;

  text_GetBoundingBox(m_fontDefinition, m_ReferenceSystem, m_strText, 0.0, pts);

  EoGePoint4d pt0[] = {EoGePoint4d(pts[0]), EoGePoint4d(pts[1]), EoGePoint4d(pts[2]), EoGePoint4d(pts[3])};

  view->ModelViewTransformPoints(4, pt0);

  for (size_t n = 0; n < 4; n++) {
    if (EoGeLine(EoGePoint3d{pt0[n]}, EoGePoint3d{pt0[(n + 1) % 4]}).DirRelOfPt(EoGePoint3d{point}) < 0) {
      return false;
    }
  }
  ptProj = EoGePoint3d{point};

  return true;
}
void EoDbText::Transform(const EoGeTransformMatrix& transformMatrix) { m_ReferenceSystem.Transform(transformMatrix); }

void EoDbText::TranslateUsingMask(EoGeVector3d v, const DWORD mask) {
  if (mask != 0) { m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v); }
}
EoDbText* EoDbText::ReadFromPeg(CFile& file) {
  auto color = EoDb::ReadInt16(file);
  (void)color;  // currently unused, but may be used in the future to indicate the text color
  auto lineType = EoDb::ReadInt16(file);
  (void)lineType;  // currently unused, but may be used in the future to indicate the text line type
  EoDbFontDefinition fontDefinition;
  fontDefinition.Read(file);
  EoGeReferenceSystem referenceSystem;
  referenceSystem.Read(file);
  CString text;
  EoDb::Read(file, text);

  auto* textPrimitive = new EoDbText(fontDefinition, referenceSystem, text);
  textPrimitive->ConvertFormattingCharacters();
  return textPrimitive;
}

bool EoDbText::Write(CFile& file) {
  EoDb::WriteUInt16(file, std::uint16_t(EoDb::kTextPrimitive));
  EoDb::WriteInt16(file, m_color);
  EoDb::WriteInt16(file, EoDbLineTypeTable::LegacyLineTypeIndex(m_lineType));
  m_fontDefinition.Write(file);
  m_ReferenceSystem.Write(file);
  EoDb::Write(file, m_strText);

  return true;
}

void DisplayText(AeSysView* view, EoGsRenderDevice* renderDevice, EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem, const CString& text) {
  if (text.IsEmpty()) { return; }

  if (HasFormattingCharacters(text)) {
    DisplayTextWithFormattingCharacters(view, renderDevice, fd, referenceSystem, text);
    return;
  }
  EoGeReferenceSystem ReferenceSystem = referenceSystem;
  EoGeTransformMatrix transformMatrix(ReferenceSystem.TransformMatrix());
  transformMatrix.Inverse();

  EoGePoint3d BottomLeftCorner;
  GetBottomLeftCorner(fd, text, BottomLeftCorner);
  BottomLeftCorner = transformMatrix * BottomLeftCorner;
  ReferenceSystem.SetOrigin(BottomLeftCorner);

  int NumberOfCharactersToDisplay = 0;
  int StartPosition = 0;
  int CurrentPosition = StartPosition;
  while (CurrentPosition < text.GetLength()) {
    wchar_t c = text[CurrentPosition++];

    if (c == '\r' && text[CurrentPosition] == '\n') {
      DisplayTextSegment(view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

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
  DisplayTextSegment(view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
}
void DisplayTextSegment(AeSysView* view, EoGsRenderDevice* renderDevice, EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text) {
  if (renderDevice != nullptr && fd.Precision() == EoDb::Precision::TrueType && view->ViewTrueTypeFonts()) {
    EoGeVector3d XDirection(referenceSystem.XDirection());
    EoGeVector3d YDirection(referenceSystem.YDirection());

    view->ModelViewTransformVector(XDirection);
    view->ModelViewTransformVector(YDirection);

    auto normal = CrossProduct(XDirection, YDirection);
    normal.Unitize();

    if (normal == EoGeVector3d::positiveUnitZ) {
      if (DisplayTextSegmentUsingTrueTypeFont(
              view, renderDevice, fd, referenceSystem, startPosition, numberOfCharacters, text)) {
        return;
      }
    }
  }
  DisplayTextSegmentUsingStrokeFont(view, renderDevice, fd, referenceSystem, startPosition, numberOfCharacters, text);
}
void DisplayTextSegmentUsingStrokeFont(AeSysView* view, EoGsRenderDevice* renderDevice,
    EoDbFontDefinition& fontDefinition, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters,
    const CString& text) {
  if (numberOfCharacters == 0) { return; }

  auto* fontData = reinterpret_cast<long*>(app.SimplexStrokeFont());
  if (fontData == nullptr) { return; }

  EoGeTransformMatrix transformMatrix(referenceSystem.TransformMatrix());
  transformMatrix.Inverse();

  // Resolve font layout based on version (v1 = legacy 96-entry header, v2 = extended with advance widths and left
  // bearings)
  long* offsetTable;
  long* advanceWidthTable;
  long* leftBearingTable;
  long* strokeData;
  int maxCharacterCode;

  if (app.StrokeFontVersion() == 2) {
    offsetTable = fontData + Eo::strokeFontV2OffsetTableStart;
    advanceWidthTable = fontData + Eo::strokeFontV2AdvanceWidthTableStart;
    leftBearingTable = fontData + Eo::strokeFontV2LeftBearingTableStart;
    strokeData = fontData + Eo::strokeFontV2StrokeDataStart;
    maxCharacterCode = Eo::strokeFontV2MaxCharacterCode;
  } else {
    offsetTable = fontData;
    advanceWidthTable = nullptr;
    leftBearingTable = nullptr;
    strokeData = fontData + Eo::strokeFontV1OffsetTableSize;
    maxCharacterCode = Eo::strokeFontV1MaxCharacterCode;
  }

  double interCharacterGap = (0.32 + fontDefinition.CharacterSpacing()) / Eo::defaultCharacterCellAspectRatio;
  double fixedCharacterAdvance = 1.0 + interCharacterGap;

  EoGePoint3d ptStroke = EoGePoint3d::kOrigin;
  EoGePoint3d ptChrPos = ptStroke;
  EoGePoint3d ptLinePos = ptChrPos;

  int n = startPosition;

  while (n < startPosition + numberOfCharacters) {
    polyline::BeginLineStrip();

    int character = text.GetAt(n);
    if (character < 32 || character > maxCharacterCode) { character = '.'; }

    // Apply left bearing offset so character strokes are left-aligned within their proportional cell
    if (leftBearingTable != nullptr) {
      int rawLeftBearing = leftBearingTable[character - 32];
      ptStroke += EoGeVector3d(-rawLeftBearing * 0.01 / Eo::defaultCharacterCellAspectRatio, 0.0, 0.0);
    }

    for (int i = offsetTable[character - 32]; i <= offsetTable[character - 32 + 1] - 1; i++) {
      int iY = static_cast<int>(strokeData[i - 1] % 4096L);
      if ((iY & 2048) != 0) { iY = -(iY - 2048); }
      int iX = static_cast<int>((strokeData[i - 1] / 4096L) % 4096L);
      if ((iX & 2048) != 0) { iX = -(iX - 2048); }

      ptStroke += EoGeVector3d(0.01 / Eo::defaultCharacterCellAspectRatio * iX, 0.01 * iY, 0.0);

      if (strokeData[i - 1] / 16777216 == 5) {
        polyline::End(view, renderDevice, 1);
        polyline::BeginLineStrip();
      }
      polyline::SetVertex(transformMatrix * ptStroke);
    }
    polyline::End(view, renderDevice, 1);

    // Per-character proportional advance when v2 advance widths are available
    double characterAdvance = fixedCharacterAdvance;
    if (advanceWidthTable != nullptr) {
      int rawAdvanceWidth = advanceWidthTable[character - 32];
      if (rawAdvanceWidth > 0) {
        double characterCellWidth = rawAdvanceWidth * 0.01 / Eo::defaultCharacterCellAspectRatio;
        characterAdvance = characterCellWidth + interCharacterGap;
      }
    }

    switch (fontDefinition.Path()) {
      case EoDb::Path::Left:
        ptChrPos.x -= characterAdvance;
        break;
      case EoDb::Path::Up:
        ptChrPos.y += characterAdvance;
        break;
      case EoDb::Path::Down:
        ptChrPos.y -= characterAdvance;
        break;
      case EoDb::Path::Right:
        [[fallthrough]];  // default is left to right
      default:
        ptChrPos.x += characterAdvance;
    }
    ptStroke = ptChrPos;
    n++;
  }
}

bool DisplayTextSegmentUsingTrueTypeFont(AeSysView* view, EoGsRenderDevice* renderDevice,
    EoDbFontDefinition& fontDefinition, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters,
    const CString& text) {
  if (numberOfCharacters <= 0) { return true; }

  EoGeTransformMatrix transformMatrix(referenceSystem.TransformMatrix());
  transformMatrix.Inverse();

  EoGePoint4d ndcPoint = EoGePoint4d{transformMatrix * EoGePoint3d::kOrigin};
  view->ModelViewTransformPoint(ndcPoint);
  CPoint clientPoint = view->ProjectToClient(ndcPoint);

  EoGePoint4d ndcPoints[3]{};

  ndcPoints[1] = EoGePoint4d{transformMatrix * EoGePoint3d(0.0, 1.0, 0.0)};
  ndcPoints[2] = EoGePoint4d{transformMatrix * EoGePoint3d(1.0, 0.0, 0.0)};

  view->ModelViewTransformPoint(ndcPoints[1]);
  view->ModelViewTransformPoint(ndcPoints[2]);

  CPoint clientPoints[4]{};

  clientPoints[1] = view->ProjectToClient(ndcPoints[1]);
  clientPoints[2] = view->ProjectToClient(ndcPoints[2]);

  EoGeVector3d vX(double(clientPoints[2].x - clientPoint.x), double(clientPoints[2].y - clientPoint.y), 0.0);
  EoGeVector3d vY(double(clientPoints[1].x - clientPoint.x), double(clientPoints[1].y - clientPoint.y), 0.0);

  double dHeight = vY.Length();
  if (dHeight == 0.0) { return true; }
  LOGFONT logfont{};
  logfont.lfHeight = -Eo::Round(1.33 * dHeight);
  logfont.lfEscapement = -FontEscapementAngle(vX);
  logfont.lfOrientation = logfont.lfEscapement;
  logfont.lfWeight = FW_THIN;
  logfont.lfCharSet = ANSI_CHARSET;
  logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
  logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  logfont.lfQuality = DEFAULT_QUALITY;
  logfont.lfPitchAndFamily = DEFAULT_PITCH;
  wcscpy_s(logfont.lfFaceName, LF_FACESIZE, fontDefinition.FontName());

  renderDevice->SelectFont(&logfont);
  UINT uTextAlign = renderDevice->SetTextAlign(TA_LEFT | TA_BASELINE);
  int iBkMode = renderDevice->SetBkMode(TRANSPARENT);

  const wchar_t* textData = static_cast<const wchar_t*>(text) + startPosition;
  renderDevice->TextOut(clientPoint.x, clientPoint.y, textData, numberOfCharacters);
  renderDevice->SetBkMode(iBkMode);
  renderDevice->SetTextAlign(uTextAlign);
  renderDevice->RestoreFont();

  return true;
}

void DisplayTextWithFormattingCharacters(AeSysView* view, EoGsRenderDevice* renderDevice, EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem, const CString& text) {
  EoGeReferenceSystem ReferenceSystem = referenceSystem;

  EoGeTransformMatrix transformMatrix(ReferenceSystem.TransformMatrix());
  transformMatrix.Inverse();

  EoGePoint3d BottomLeftCorner;
  GetBottomLeftCorner(fd, text, BottomLeftCorner);
  BottomLeftCorner = transformMatrix * BottomLeftCorner;
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
          DisplayTextSegment(view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

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
                DisplayTextSegment(
                    view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);

                // Offset the line position left of current position
                ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, referenceSystem, 0.0,
                    NumberOfCharactersToDisplay * (1 + 0.32 / Eo::defaultCharacterCellAspectRatio)));
                BottomLeftCorner = ReferenceSystem.Origin();
              }
              StartPosition = EndSemicolon + 1;
              CurrentPosition = StartPosition;
              NumberOfCharactersToDisplay = 0;

              if (Parameter == '1') {
                ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 0.5, 0.0));
              } else if (Parameter == '2') {
                ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, -0.5, 0.0));
              }
              BottomLeftCorner = ReferenceSystem.Origin();
            }
          }
        }
      } else if (c == 'S') {  // Stacked text or fractions
        int EndSemicolon = text.Find(';', CurrentPosition);
        if (EndSemicolon != -1) {
          int TextSegmentDelimiter = text.Find('/', CurrentPosition);
          if (TextSegmentDelimiter == -1) { TextSegmentDelimiter = text.Find('^', CurrentPosition); }

          if (TextSegmentDelimiter != -1 && TextSegmentDelimiter < EndSemicolon) {
            if (NumberOfCharactersToDisplay > 0) {  // display text segment preceding the formatting
              DisplayTextSegment(
                  view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
              StartPosition += NumberOfCharactersToDisplay;
            }
            // Offset the line position up and conditionally left of current position
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, -0.35,
                NumberOfCharactersToDisplay * (1 + 0.32 / Eo::defaultCharacterCellAspectRatio)));
            BottomLeftCorner = ReferenceSystem.Origin();
            StartPosition += 2;  // skip the formatting characters
            NumberOfCharactersToDisplay = TextSegmentDelimiter - StartPosition;
            if (NumberOfCharactersToDisplay > 0) {  // Display superscripted text segment
              DisplayTextSegment(
                  view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
              StartPosition += NumberOfCharactersToDisplay;
            }
            // Offset the line position back down left
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 0.35,
                NumberOfCharactersToDisplay * (1 + 0.32 / Eo::defaultCharacterCellAspectRatio) - 0.72));
            BottomLeftCorner = ReferenceSystem.Origin();

            if (text[TextSegmentDelimiter] == '/') {  // display the text segment delimitier
              DisplayTextSegment(view, renderDevice, fd, ReferenceSystem, TextSegmentDelimiter, 1, text);
            }
            StartPosition = TextSegmentDelimiter + 1;
            NumberOfCharactersToDisplay = EndSemicolon - StartPosition;
            // Offset the line position down
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, 0.35, 0.72));
            BottomLeftCorner = ReferenceSystem.Origin();

            if (NumberOfCharactersToDisplay > 0) {  // Display subscripted text segment
              DisplayTextSegment(
                  view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
              StartPosition += NumberOfCharactersToDisplay;
            }
            ReferenceSystem.SetOrigin(text_GetNewLinePos(fd, ReferenceSystem, -0.35,
                NumberOfCharactersToDisplay * (1 + 0.32 / Eo::defaultCharacterCellAspectRatio)));
            BottomLeftCorner = ReferenceSystem.Origin();

            NumberOfCharactersToDisplay = 0;
            StartPosition = EndSemicolon + 1;
            CurrentPosition = StartPosition;
          }
        }
      }
    }
  }
  DisplayTextSegment(view, renderDevice, fd, ReferenceSystem, StartPosition, NumberOfCharactersToDisplay, text);
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
          if (TextSegmentDelimiter == -1) { TextSegmentDelimiter = text.Find('^', CurrentPosition); }

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
/// Returns the normalized cell width for a single character code using v2 advance widths when
/// available, or 1.0 (fixed cell width) for v1.
static double CharacterCellWidth(int characterCode, const long* advanceWidthTable, int maxCharacterCode) {
  if (characterCode < 32 || characterCode > maxCharacterCode) { characterCode = '.'; }
  if (advanceWidthTable != nullptr) {
    int rawAdvanceWidth = advanceWidthTable[characterCode - 32];
    if (rawAdvanceWidth > 0) { return rawAdvanceWidth * 0.01 / Eo::defaultCharacterCellAspectRatio; }
  }
  return 1.0;
}
/// Computes the total text extent in normalized stroke-font coordinates along the text path,
/// accounting for per-character v2 advance widths when available. Formatting characters
/// (\P, \A, \S sequences) are excluded from the extent calculation.
static double ComputeStrokeFontTextExtent(const EoDbFontDefinition& fontDefinition, const CString& text) {
  auto* fontData = reinterpret_cast<long*>(app.SimplexStrokeFont());

  long* advanceWidthTable = nullptr;
  int maxCharacterCode = Eo::strokeFontV1MaxCharacterCode;

  if (fontData != nullptr && app.StrokeFontVersion() == 2) {
    advanceWidthTable = fontData + Eo::strokeFontV2AdvanceWidthTableStart;
    maxCharacterCode = Eo::strokeFontV2MaxCharacterCode;
  }

  double interCharacterGap = (0.32 + fontDefinition.CharacterSpacing()) / Eo::defaultCharacterCellAspectRatio;
  int displayableCount = 0;
  double totalCellWidth = 0.0;

  int currentPosition = 0;
  while (currentPosition < text.GetLength()) {
    wchar_t c = text[currentPosition++];

    // Skip formatting sequences (mirrors LengthSansFormattingCharacters logic)
    if (c == '\\' && currentPosition < text.GetLength()) {
      wchar_t next = text[currentPosition];
      if (next == 'A') {
        int endSemicolon = text.Find(';', currentPosition);
        if (endSemicolon != -1 && endSemicolon == currentPosition + 2) {
          currentPosition = endSemicolon + 1;
          continue;
        }
      } else if (next == 'P') {
        currentPosition++;
        continue;
      } else if (next == 'S') {
        int endSemicolon = text.Find(';', currentPosition);
        if (endSemicolon != -1) {
          int delimiter = text.Find('/', currentPosition);
          if (delimiter == -1) { delimiter = text.Find('^', currentPosition); }
          if (delimiter != -1 && delimiter < endSemicolon) {
            for (int i = currentPosition + 1; i < endSemicolon; i++) {
              if (i != delimiter) {
                totalCellWidth += CharacterCellWidth(text[i], advanceWidthTable, maxCharacterCode);
                displayableCount++;
              }
            }
            currentPosition = endSemicolon + 1;
            continue;
          }
        }
      }
    }

    totalCellWidth += CharacterCellWidth(c, advanceWidthTable, maxCharacterCode);
    displayableCount++;
  }

  if (displayableCount <= 0) { return 0.0; }
  return totalCellWidth + (displayableCount - 1) * interCharacterGap;
}
void GetBottomLeftCorner(EoDbFontDefinition& fd, const CString& text, EoGePoint3d& pt) {
  double dTxtExt = ComputeStrokeFontTextExtent(fd, text);
  if (dTxtExt > 0.0) {
    if (fd.Path() == EoDb::Path::Right || fd.Path() == EoDb::Path::Left) {
      if (fd.Path() == EoDb::Path::Right) {
        if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Left) {
          pt.x = 0.;
        } else if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Center) {
          pt.x = -dTxtExt * 0.5;
        } else if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Right) {
          pt.x = -dTxtExt;
        }
      } else {
        if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Left) {
          pt.x = dTxtExt;
        } else if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Center) {
          pt.x = dTxtExt * 0.5;
        } else if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Right) {
          pt.x = 0.;
        }
        pt.x = pt.x - 1.;
      }
      if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Bottom) {
        pt.y = 0.;
      } else if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Middle) {
        pt.y = -0.5;
      } else if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Top) {
        pt.y = -1.;
      }
    } else if (fd.Path() == EoDb::Path::Down || fd.Path() == EoDb::Path::Up) {
      if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Left) {
        pt.x = 0.;
      } else if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Center) {
        pt.x = -0.5;
      } else if (fd.HorizontalAlignment() == EoDb::HorizontalAlignment::Right) {
        pt.x = -1.;
      }
      if (fd.Path() == EoDb::Path::Up) {
        if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Bottom) {
          pt.y = 0.;
        } else if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Middle) {
          pt.y = -dTxtExt * 0.5;
        } else if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Top) {
          pt.y = -dTxtExt;
        }
      } else {
        if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Bottom) {
          pt.y = dTxtExt;
        } else if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Middle) {
          pt.y = dTxtExt * 0.5;
        } else if (fd.VerticalAlignment() == EoDb::VerticalAlignment::Top) {
          pt.y = 0.;
        }
        pt.y = pt.y - 1.;
      }
    }
  } else {
    pt.x = 0.;
    pt.y = 0.;
  }
  pt.z = 0.;
}

void text_GetBoundingBox(EoDbFontDefinition& fontDefinition, EoGeReferenceSystem& referenceSystem, const CString& text,
    double spaceFactor, EoGePoint3dArray& ptsBox) {
  ptsBox.SetSize(4);

  double textExtent = ComputeStrokeFontTextExtent(fontDefinition, text);
  if (textExtent > 0.0) {
    EoGeTransformMatrix transformMatrix(referenceSystem.TransformMatrix());
    transformMatrix.Inverse();

    double TextHeight = 1.0;
    double TextWidth = 1.0;

    if (fontDefinition.Path() == EoDb::Path::Right || fontDefinition.Path() == EoDb::Path::Left) {
      TextWidth = textExtent;
    } else {
      TextHeight = textExtent;
    }
    ptsBox[0] = EoGePoint3d::kOrigin;
    ptsBox[1] = ptsBox[0];
    ptsBox[2] = ptsBox[0];
    ptsBox[3] = ptsBox[0];

    if (fontDefinition.HorizontalAlignment() == EoDb::HorizontalAlignment::Left) {
      ptsBox[2].x = TextWidth;
    } else if (fontDefinition.HorizontalAlignment() == EoDb::HorizontalAlignment::Center) {
      ptsBox[0].x = -TextWidth * 0.5;
      ptsBox[2].x = ptsBox[0].x + TextWidth;
    } else {
      ptsBox[0].x = -TextWidth;
    }
    if (fontDefinition.VerticalAlignment() == EoDb::VerticalAlignment::Top) {
      ptsBox[0].y = -TextHeight;
    } else if (fontDefinition.VerticalAlignment() == EoDb::VerticalAlignment::Middle) {
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

    for (int n = 0; n < 4; n++) { ptsBox[n] = transformMatrix * ptsBox[n]; }
  } else {
    for (int n = 0; n < 4; n++) { ptsBox[n] = referenceSystem.Origin(); }
  }
}

EoGePoint3d text_GetNewLinePos(const EoDbFontDefinition& fontDefinition, EoGeReferenceSystem& referenceSystem,
    double lineSpaceFactor, double characterSpaceFactor) {
  auto position = referenceSystem.Origin();
  auto path = referenceSystem.XDirection();
  auto yDirection = referenceSystem.YDirection();

  if (fontDefinition.Path() == EoDb::Path::Right || fontDefinition.Path() == EoDb::Path::Left) {
    position += path * characterSpaceFactor;
    EoGeVector3d unitNormal = referenceSystem.UnitNormal();

    path.Unitize();
    path *= -(yDirection.Length() * lineSpaceFactor);
    path.RotateAboutArbitraryAxis(unitNormal, Eo::HalfPi);
  }
  return (position + (path * 1.5));
}
