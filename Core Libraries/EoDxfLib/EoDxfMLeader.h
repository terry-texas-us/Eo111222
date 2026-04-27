#pragma once

#include <cstdint>

#include "EoDxfBase.h"
#include "EoDxfEntities.h"
#include "EoDxfReader.h"

/** @brief Class to handle MULTILEADER (MLEADER) entity.
 *
 *  A multileader entity is an annotation object that associates leader lines with
 *  either MText or a block reference.  Introduced in AutoCAD 2008 (DXF AC1021).
 *  The entity has three nested sub-elements:
 *    1. **CONTEXT_DATA** — overall layout parameters, text/block content
 *    2. **LEADER** — a leader branch with a dogleg vector and dogleg length
 *    3. **LEADER_LINE** — vertex list for one leader line segment
 *
 *  Nesting is delimited in DXF by string-valued group codes:
 *    - 300 "CONTEXT_DATA{"  /  301 "}"
 *    - 302 "LEADER{"        /  303 "}"
 *    - 304 "LEADER_LINE{"   /  305 "}"
 */
class EoDxfMLeader : public EoDxfGraphic {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfMLeader() noexcept : EoDxfGraphic{EoDxf::MLEADER} {}

  EoDxfMLeader(const EoDxfMLeader&) = delete;
  EoDxfMLeader& operator=(const EoDxfMLeader&) = delete;

  EoDxfMLeader(EoDxfMLeader&&) noexcept = default;
  EoDxfMLeader& operator=(EoDxfMLeader&&) noexcept = default;

  ~EoDxfMLeader() = default;

  void ApplyExtrusion() noexcept override {}

 protected:
  void ParseCode(int code, EoDxfReader& reader);

 public:
  // --- Top-level AcDbMLeader properties ---
  std::int16_t m_leaderType{1};  // code 170 (0=invisible, 1=straight, 2=spline)
  std::int32_t m_leaderLineColor{EoDxf::colorByLayer};  // code 91
  std::uint64_t m_leaderLineTypeHandle{EoDxf::NoHandle};  // code 341
  std::int16_t m_leaderLineWeight{-1};  // code 171
  bool m_enableLanding{true};  // code 290
  bool m_enableDogleg{true};  // code 291
  double m_doglegLength{};  // code 41
  double m_arrowheadSize{};  // code 42
  std::int16_t m_contentType{2};  // code 172 (0=none, 1=block, 2=mtext)
  std::uint64_t m_textStyleHandle{EoDxf::NoHandle};  // code 343
  std::int16_t m_textLeftAttachmentType{1};  // code 173
  std::int32_t m_textRightAttachmentType{1};  // code 95
  std::int16_t m_textAngleType{};  // code 174
  std::int16_t m_textAlignmentType{};  // code 175
  std::int32_t m_textColor{EoDxf::colorByLayer};  // code 92
  bool m_enableFrameText{};  // code 292
  std::uint64_t m_blockContentHandle{EoDxf::NoHandle};  // code 344
  std::int32_t m_blockContentColor{EoDxf::colorByLayer};  // code 93
  EoDxfGeometryBase3d m_blockContentScale{1.0, 1.0, 1.0};  // code 10, 20, 30 (top-level)
  double m_blockContentRotation{};  // code 43
  std::int16_t m_blockContentConnectionType{};  // code 176
  bool m_enableAnnotationScale{true};  // code 293
  std::uint64_t m_leaderStyleHandle{EoDxf::NoHandle};  // code 340
  std::int32_t m_propertyOverrideFlag{};  // code 90
  double m_overallScale{1.0};  // code 45
  bool m_textDirectionNegative{};  // code 294 (0=left-to-right, 1=used by BiDi)
  std::int16_t m_textTopAttachmentType{9};  // code 271
  std::int16_t m_textBottomAttachmentType{9};  // code 272

  // Context data (only one per MLEADER)
  EoDxfMLeaderContextData m_contextData;

 private:
  /** @brief Parsing state for the nested MLEADER sub-element structure. */
  enum class ParseState { TopLevel, ContextData, Leader, LeaderLine, MTextContent };
  ParseState m_parseState{ParseState::TopLevel};
};
