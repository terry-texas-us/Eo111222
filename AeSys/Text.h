#pragma once

void DisplayText(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text);
void DisplayTextSegment(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text);
/// <summary> Displays a text string using a stroke font.</summary>
void DisplayTextSegmentUsingStrokeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text);
/// <summary> Attempts to display text is using true type font.</summary>
bool DisplayTextSegmentUsingTrueTypeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters, const CString& text);
void DisplayTextWithFormattingCharacters(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text);
/// <summary> Determines the count of characters in string excluding formatting characters.</summary>
int LengthSansFormattingCharacters(const CString& text);
/// <summary> Determines the offset to the bottom left alignment position of a string of the specified number of characters and text attributes in the z=0 plane.</summary>
void GetBottomLeftCorner(EoDbFontDefinition& fd, int iChrs, EoGePoint3d& pt);
/// <summary>Returns the region boundaries of a text string applying and optional inflation factor.</summary>
void text_GetBoundingBox(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int nLen, double dSpacFac,  CPnts& ptsBox);
EoGePoint3d text_GetNewLinePos(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, double dLineSpaceFac, double dChrSpaceFac);
