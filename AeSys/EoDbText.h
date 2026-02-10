#pragma once

#include <cstdint>

#include "AeSysView.h"
#include "EoDb.h"
#include "EoDbFontDefinition.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"

class EoDbCharacterCellDefinition;

class EoDbText : public EoDbPrimitive {
  EoDbFontDefinition m_fontDefinition;
  EoGeReferenceSystem m_ReferenceSystem;
  CString m_strText;

 public:  // Constructors and destructor
  EoDbText() {}
  EoDbText(std::uint8_t* buffer, int version);
  EoDbText(const EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text);
  EoDbText(const EoDbText&);

  ~EoDbText() override = default;

  const EoDbText& operator=(const EoDbText&);

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbText*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() override { return (m_ReferenceSystem.Origin()); }
  bool Identical(EoDbPrimitive*) override { return false; }
  bool IsInView(AeSysView* view) override;
  bool Is(std::uint16_t type) override { return type == EoDb::kTextPrimitive; }
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  void ModifyState() override;
  void ModifyNotes(const EoDbFontDefinition& fontDefinition, const EoDbCharacterCellDefinition& characterCellDefinition,
      int attributes);
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Translate(const EoGeVector3d& v) override { m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v); }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  void Transform(const EoGeTransformMatrix&) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  void ConvertFormattingCharacters();
  void GetBoundingBox(EoGePoint3dArray&, double);
  void GetFontDef(EoDbFontDefinition& fd) const { fd = m_fontDefinition; }
  void GetRefSys(EoGeReferenceSystem& referenceSystem) const { referenceSystem = m_ReferenceSystem; }
  const CString& Text() { return m_strText; }

  EoGeVector3d RefNorm() { return m_ReferenceSystem.UnitNormal(); }

  EoGePoint3d ReferenceOrigin() { return m_ReferenceSystem.Origin(); }

  void SetText(const CString& text) { m_strText = text; }
};

void DisplayText(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem,
    const CString& text);
void DisplayTextSegment(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text);
/// <summary> Displays a text string using a stroke font.</summary>
void DisplayTextSegmentUsingStrokeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text);
/// <summary> Attempts to display text is using true type font.</summary>
bool DisplayTextSegmentUsingTrueTypeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text);
void DisplayTextWithFormattingCharacters(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem, const CString& text);
/// <summary> Determines the count of characters in string excluding formatting characters.</summary>
int LengthSansFormattingCharacters(const CString& text);
/// <summary> Determines the offset to the bottom left alignment position of a string of the specified number of characters and text attributes in the z=0 plane.</summary>
void GetBottomLeftCorner(EoDbFontDefinition& fd, int iChrs, EoGePoint3d& pt);
/// <summary>Returns the region boundaries of a text string applying and optional inflation factor.</summary>
void text_GetBoundingBox(
    EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int nLen, double dSpacFac, EoGePoint3dArray& ptsBox);
EoGePoint3d text_GetNewLinePos(const EoDbFontDefinition& fontdefinition, EoGeReferenceSystem& referenceSystem,
    double dLineSpaceFac, double dChrSpaceFac);
