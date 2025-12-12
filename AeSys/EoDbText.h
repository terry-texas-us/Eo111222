#pragma once

class EoDbText : public EoDbPrimitive {
  EoDbFontDefinition m_fd;
  EoGeReferenceSystem m_ReferenceSystem;
  CString m_strText;

 public:  // Constructors and destructor
  EoDbText() {}
  EoDbText(EoByte* buffer, int version);
  EoDbText(const EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text);
  EoDbText(const EoDbText&);

  ~EoDbText() override {};

 public:  // Operators
  const EoDbText& operator=(const EoDbText&);

 public:  // Methods - absolute virtuals
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbText*>(primitive); }
#if defined(USING_ODA)
  OdDbEntity* Convert(const OdDbObjectId& blockTableRecord);
#endif  // USING_ODA
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, CDC* deviceContext) override;
  void AddReportToMessageList(EoGePoint3d) override;
  void GetAllPts(EoGePoint3dArray& pts) override {
    pts.SetSize(0);
    EoGePoint3d pt = m_ReferenceSystem.Origin();
    pts.Add(pt);
  }
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  EoGePoint3d GetCtrlPt() override;
  /// <summary>Determines the extent.</summary>
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&) override;
  EoGePoint3d GoToNxtCtrlPt() override { return (m_ReferenceSystem.Origin()); }
  bool Identical(EoDbPrimitive*) override { return false; }
  bool IsInView(AeSysView* view) override;
  bool Is(EoUInt16 type) override { return type == EoDb::kTextPrimitive; }
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  void ModifyState() override;
  void ModifyNotes(EoDbFontDefinition& fd, EoDbCharacterCellDefinition& ccd, int iAtt);
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* /* view */, EoGeLine /* line */, EoGePoint3dArray&) override { return false; }
  /// <summary>Evaluates whether a point lies within the bounding region of text.</summary>
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Translate(EoGeVector3d v) override { m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v); }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  void Transform(EoGeTransformMatrix&) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, EoByte* buffer) override;

 public:  // Methods
  void ConvertFormattingCharacters();
  /// <summary>Get the bounding box of text.</summary>
  void GetBoundingBox(EoGePoint3dArray&, double);
  void GetFontDef(EoDbFontDefinition& fd) { fd = m_fd; }
  void GetRefSys(EoGeReferenceSystem& referenceSystem) { referenceSystem = m_ReferenceSystem; }
  const CString& Text() { return m_strText; }
  EoGeVector3d RefNorm() {
    EoGeVector3d vNorm;
    m_ReferenceSystem.GetUnitNormal(vNorm);
    return vNorm;
  }
  EoGePoint3d ReferenceOrigin() { return m_ReferenceSystem.Origin(); }
  void SetText(const CString& text) { m_strText = text; }
};

void DisplayText(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem,
                 const CString& text);
void DisplayTextSegment(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
                        EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters,
                        const CString& text);
/// <summary> Displays a text string using a stroke font.</summary>
void DisplayTextSegmentUsingStrokeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
                                       EoGeReferenceSystem& referenceSystem, int startPosition, int numberOfCharacters,
                                       const CString& text);
/// <summary> Attempts to display text is using true type font.</summary>
bool DisplayTextSegmentUsingTrueTypeFont(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
                                         EoGeReferenceSystem& referenceSystem, int startPosition,
                                         int numberOfCharacters, const CString& text);
void DisplayTextWithFormattingCharacters(AeSysView* view, CDC* deviceContext, EoDbFontDefinition& fd,
                                         EoGeReferenceSystem& referenceSystem, const CString& text);
/// <summary> Determines the count of characters in string excluding formatting characters.</summary>
int LengthSansFormattingCharacters(const CString& text);
/// <summary> Determines the offset to the bottom left alignment position of a string of the specified number of characters and text attributes in the z=0 plane.</summary>
void GetBottomLeftCorner(EoDbFontDefinition& fd, int iChrs, EoGePoint3d& pt);
/// <summary>Returns the region boundaries of a text string applying and optional inflation factor.</summary>
void text_GetBoundingBox(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, int nLen, double dSpacFac,
                         EoGePoint3dArray& ptsBox);
EoGePoint3d text_GetNewLinePos(EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, double dLineSpaceFac,
                               double dChrSpaceFac);
