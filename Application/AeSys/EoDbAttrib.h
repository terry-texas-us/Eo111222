#pragma once

#include <cstdint>
#include <string>

#include "EoDb.h"
#include "EoDbText.h"

/** @brief Attribute instance primitive — carries attribute identity (tag, flags, parent INSERT handle)
 *         alongside the rendered text inherited from EoDbText.
 *
 *  ## AE2011 (V1) Backward Compatibility
 *  - `Write(CFile&)` delegates to `EoDbText::Write()`, which writes `kTextPrimitive`. The attribute
 *    identity (tag, flags, insert handle) is **silently lost** — a V1 reader sees plain text.
 *  - `Is(kTextPrimitive)` returns true so existing trap filters, selection, and rendering code
 *    that queries by text type continues to work unchanged.
 *
 *  ## AE2026 (V2) Serialization
 *  - `Write(CFile&)` still writes `kTextPrimitive` V1 data (so the base V1 reader can still parse
 *    the stream). The V2 extension block (handle, ownerHandle, lineWeight, lineTypeScale) written
 *    by `EoDbGroup::Write(file, AE2026)` preserves handle identity. Additionally, `ReadFromPeg()`
 *    reads the V2 attribute extension (tag, flags, insertHandle) appended after the V1 text data
 *    when the type code is `kAttribPrimitive`.
 *
 *  ## DXF Round-Trip
 *  - `ExportToDxf()` writes a DXF ATTRIB entity with full alignment, style, and attribute fields.
 *    The parent INSERT entity's `ExportToDxf()` is responsible for writing SEQEND after all ATTRIBs
 *    (deferred to Phase 4).
 */
class EoDbAttrib : public EoDbText {
  std::wstring m_tagString;               ///< Attribute tag name (DXF group code 2)
  std::int16_t m_attributeFlags{};        ///< Attribute flags (DXF group code 70: 1=Invisible, 2=Constant, 4=Verify, 8=Preset)
  std::uint64_t m_insertHandle{};         ///< Handle of the parent INSERT (EoDbBlockReference) — session-only link

 public:  // Constructors and destructor
  EoDbAttrib() = default;

  /// @brief Constructs an EoDbAttrib from text properties and attribute identity.
  EoDbAttrib(const EoDbFontDefinition& fontDefinition, EoGeReferenceSystem& referenceSystem,
      const std::wstring& text, std::wstring tagString, std::int16_t attributeFlags);

  EoDbAttrib(const EoDbAttrib&);

  ~EoDbAttrib() override = default;

  const EoDbAttrib& operator=(const EoDbAttrib&);

  // --- EoDbPrimitive overrides ---
  void AddReportToMessageList(const EoGePoint3d& point) override;
  void AddToTreeViewControl(HWND hTree, HTREEITEM hParent) override;
  void Assign(EoDbPrimitive* primitive) override { *this = *static_cast<EoDbAttrib*>(primitive); }
  EoDbPrimitive*& Copy(EoDbPrimitive*&) override;
  void ExportToDxf(EoDxfInterface* writer) const override;
  void FormatExtra(CString& extra) override;
  void FormatGeometry(CString& str) override;

  /// @brief Returns true for both kAttribPrimitive and kTextPrimitive.
  /// This dual-type identity allows existing text-based operations (trap filters, selection,
  /// rendering queries) to work on attributes without modification.
  bool Is(std::uint16_t type) noexcept override {
    return type == EoDb::kAttribPrimitive || type == EoDb::kTextPrimitive;
  }

  /// @brief V1 write: delegates to EoDbText::Write() — attribute identity is lost (kTextPrimitive).
  /// V2 extension (handle, owner, lineWeight, lineTypeScale) is written by EoDbGroup::Write().
  bool Write(CFile& file) override;

  /// @brief Legacy buffer-based write: delegates to EoDbText (kTextPrimitive).
  void Write(CFile& file, std::uint8_t* buffer) override;

  /// @brief Reads an attrib primitive from a PEG V2 file stream (type code kAttribPrimitive).
  /// The V1 text data (color, lineType, fontDef, refSys, text) is read first, followed by
  /// the V2 attribute extension (tag, flags, insertHandle).
  /// @param file The CFile object representing the PEG file to read from.
  /// @return A pointer to the constructed EoDbAttrib.
  static EoDbAttrib* ReadFromPeg(CFile& file);

  // --- Attribute-specific accessors ---
  [[nodiscard]] const std::wstring& TagString() const noexcept { return m_tagString; }
  void SetTagString(std::wstring tag) noexcept { m_tagString = std::move(tag); }

  [[nodiscard]] std::int16_t AttributeFlags() const noexcept { return m_attributeFlags; }
  void SetAttributeFlags(std::int16_t flags) noexcept { m_attributeFlags = flags; }

  [[nodiscard]] std::uint64_t InsertHandle() const noexcept { return m_insertHandle; }
  void SetInsertHandle(std::uint64_t handle) noexcept { m_insertHandle = handle; }
};
