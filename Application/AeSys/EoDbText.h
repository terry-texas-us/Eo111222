#pragma once

#include <cstdint>
#include <optional>
#include <string>

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

/// @brief MTEXT-specific DXF properties for round-trip fidelity.
/// When present on an EoDbText, indicates the text originated from a DXF MTEXT entity
/// and should be exported as MTEXT rather than TEXT.
struct EoDbMTextProperties {
  double referenceRectangleWidth{};  ///< DXF group 41 (wrapping box width)
  double lineSpacingFactor{1.0};  ///< DXF group 44 (clamped to [0.25, 4.0])
  std::int16_t lineSpacingStyle{1};  ///< DXF group 73 (1 = AtLeast, 2 = Exact)
  std::int16_t drawingDirection{1};  ///< DXF group 72 (1 = LeftToRight, 3 = TopToBottom, 5 = ByStyle)
  std::int16_t attachmentPoint{1};  ///< DXF group 71 (1–9, maps to 3×3 alignment grid)
};

class EoDbText : public EoDbPrimitive {
  EoDbFontDefinition m_fontDefinition;
  EoGeReferenceSystem m_ReferenceSystem;
  CString m_strText;
  std::int16_t m_textGenerationFlags{};  ///< DXF group code 71 (2 = backward, 4 = upside-down)
  EoGeVector3d m_extrusion{EoGeVector3d::positiveUnitZ};  ///< DXF extrusion direction (group codes 210/220/230)

  /// When set, the text originated from a DXF MTEXT entity and ExportToDxf writes MTEXT instead of TEXT.
  std::optional<EoDbMTextProperties> m_mtextProperties{};

 public:  // Constructors and destructor
  EoDbText() noexcept {}
  EoDbText(std::uint8_t* buffer, int version);
  EoDbText(const EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const CString& text);
  EoDbText(const EoDbFontDefinition& fd, EoGeReferenceSystem& referenceSystem, const std::wstring& text);
  EoDbText(const EoDbText&);

  ~EoDbText() override = default;

  const EoDbText& operator=(const EoDbText&);

  void AddReportToMessageList(const EoGePoint3d&) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbText*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice) override;
  void ExportToDxf(EoDxfInterface* writer) const override;
  void FormatExtra(CString& str) override;
  void FormatGeometry(CString& str) override;
  void GetAllPoints(EoGePoint3dArray& points) override;
  EoGePoint3d GetControlPoint() override;
  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) override;
  EoGePoint3d GoToNextControlPoint() noexcept override { return (m_ReferenceSystem.Origin()); }
  bool Identical(EoDbPrimitive*) noexcept override { return false; }
  bool IsInView(AeSysView* view) override;
  bool Is(std::uint16_t type) noexcept override { return type == EoDb::kTextPrimitive; }
  bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  void ModifyState() override;
  void ModifyNotes(const EoDbFontDefinition& fontDefinition,
      const EoDbCharacterCellDefinition& characterCellDefinition,
      int attributes);
  EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) override;
  bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray&) override;
  bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) override;
  bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) override;
  void Translate(const EoGeVector3d& v) noexcept override {
    m_ReferenceSystem.SetOrigin(m_ReferenceSystem.Origin() + v);
  }
  void TranslateUsingMask(EoGeVector3d, const DWORD) override;
  void Transform(const EoGeTransformMatrix&) override;
  bool Write(CFile& file) override;
  void Write(CFile& file, std::uint8_t* buffer) override;

  /// @brief Reads a text primitive from a PEG file stream (type code kTextPrimitive).
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbText.
  static EoDbText* ReadFromPeg(CFile& file);

  void ConvertFormattingCharacters();
  void GetBoundingBox(EoGePoint3dArray&, double);
  void GetFontDef(EoDbFontDefinition& fd) const { fd = m_fontDefinition; }
  void GetRefSys(EoGeReferenceSystem& referenceSystem) const noexcept { referenceSystem = m_ReferenceSystem; }
  const CString& Text() const noexcept { return m_strText; }

  EoGeVector3d RefNorm() { return m_ReferenceSystem.UnitNormal(); }

  EoGePoint3d ReferenceOrigin() noexcept { return m_ReferenceSystem.Origin(); }

  void SetFontDefinition(const EoDbFontDefinition& fontDefinition) noexcept { m_fontDefinition = fontDefinition; }
  void SetReferenceOrigin(const EoGePoint3d& origin) noexcept { m_ReferenceSystem.SetOrigin(origin); }
  void SetReferenceSystem(const EoGeReferenceSystem& referenceSystem) noexcept { m_ReferenceSystem = referenceSystem; }
  void SetText(const CString& text) { m_strText = text; }
  void SetText(const std::wstring& text) { m_strText = text.c_str(); }
  void SetTextGenerationFlags(std::int16_t flags) noexcept { m_textGenerationFlags = flags; }
  [[nodiscard]] std::int16_t TextGenerationFlags() const noexcept { return m_textGenerationFlags; }
  void SetExtrusion(const EoGeVector3d& extrusion) noexcept { m_extrusion = extrusion; }
  [[nodiscard]] const EoGeVector3d& Extrusion() const noexcept { return m_extrusion; }

  void SetMTextProperties(const EoDbMTextProperties& properties) noexcept { m_mtextProperties = properties; }
  [[nodiscard]] bool IsFromMText() const noexcept { return m_mtextProperties.has_value(); }
  [[nodiscard]] const std::optional<EoDbMTextProperties>& MTextProperties() const noexcept { return m_mtextProperties; }

 private:
  /// @brief Exports this text primitive as a DXF MTEXT entity using stored MTEXT properties.
  void ExportAsMText(EoDxfInterface* writer) const;
};

void DisplayText(AeSysView* view,
    EoGsRenderDevice* renderDevice,
    EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem,
    const CString& text);
void DisplayTextSegment(AeSysView* view,
    EoGsRenderDevice* renderDevice,
    EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem,
    int startPosition,
    int numberOfCharacters,
    const CString& text);
/// @brief Displays a text string using a stroke font.
void DisplayTextSegmentUsingStrokeFont(AeSysView* view,
    EoGsRenderDevice* renderDevice,
    const EoDbFontDefinition& fd,
    const EoGeReferenceSystem& referenceSystem,
    int startPosition,
    int numberOfCharacters,
    const CString& text);
/// @brief Attempts to display text is using true type font.
bool DisplayTextSegmentUsingTrueTypeFont(AeSysView* view,
    EoGsRenderDevice* renderDevice,
    const EoDbFontDefinition& fd,
    const EoGeReferenceSystem& referenceSystem,
    int startPosition,
    int numberOfCharacters,
    const CString& text);

/**
 * Displays text with embedded formatting characters for line breaks, alignment changes, and stacking.
 * Supported formatting characters are:
 *   \P: Hard line break
 *   \A: Alignment change with parameter (0=left, 1=center, 2=right)
 *   \S: Stacked text or fractions with format \S<top text>/<bottom text>;
 *
 * The formatting characters are parsed and applied during display. For example, the text "Line1\PLine2" will be
 * displayed as: Line1
 *               Line2
 *
 * The function processes the input text sequentially, displaying segments of regular text and applying formatting as it
 * encounters the special characters.
 */
void DisplayTextWithFormattingCharacters(AeSysView* view,
    EoGsRenderDevice* renderDevice,
    EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem,
    const CString& text);

/// @brief Renders MTEXT text with automatic word-wrapping based on the reference rectangle width.
/// Lines are broken at word boundaries (spaces) when the accumulated line width exceeds the
/// wrap threshold derived from EoDbMTextProperties::referenceRectangleWidth. Hard paragraph
/// breaks (\P) are honored. Formatting codes (\A, \S) are skipped during width measurement
/// and passed through to DisplayTextWithFormattingCharacters for each wrapped line segment.
void DisplayMTextWithWordWrap(AeSysView* view,
    EoGsRenderDevice* renderDevice,
    EoDbFontDefinition& fd,
    EoGeReferenceSystem& referenceSystem,
    const CString& text,
    const EoDbMTextProperties& mtextProperties);

/// @brief Determines the count of characters in string excluding formatting characters.
[[nodiscard]] int LengthSansFormattingCharacters(const CString& text);

/// @brief Determines the offset to the bottom left alignment position of a text string
/// using per-character advance widths (v2) or fixed cell width (v1) in the z=0 plane.
void GetBottomLeftCorner(const EoDbFontDefinition& fd, const CString& text, EoGePoint3d& pt);

/// @brief Returns the region boundaries of a text string applying an optional inflation factor.
void text_GetBoundingBox(const EoDbFontDefinition& fd,
    const EoGeReferenceSystem& referenceSystem,
    const CString& text,
    double spaceFactor,
    EoGePoint3dArray& ptsBox);
EoGePoint3d text_GetNewLinePos(const EoDbFontDefinition& fontdefinition,
    const EoGeReferenceSystem& referenceSystem,
    double dLineSpaceFac,
    double dChrSpaceFac);
