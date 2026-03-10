#include <string>
#include <vector>

#include "EoDxfEntities.h"
#include "EoDxfMLeader.h"
#include "EoDxfReader.h"

void EoDxfMLeader::ParseCode(int code, EoDxfReader* reader) {
  // Handle state transitions driven by string-valued bracket markers.
  if (code >= 300 && code <= 305) {
    const std::string marker = reader->GetString();

    if (code == 300 && marker == "CONTEXT_DATA{") {
      m_parseState = ParseState::ContextData;
      return;
    }
    if (code == 301 && marker == "}") {
      // Closing bracket — could end MTextContent or CONTEXT_DATA.
      if (m_parseState == ParseState::MTextContent) {
        m_parseState = ParseState::ContextData;
      } else {
        m_parseState = ParseState::TopLevel;
      }
      return;
    }
    if (code == 302 && marker == "LEADER{") {
      m_contextData.m_leaders.emplace_back();
      m_parseState = ParseState::Leader;
      return;
    }
    if (code == 303 && marker == "}") {
      m_parseState = ParseState::ContextData;
      return;
    }
    if (code == 304 && marker == "LEADER_LINE{") {
      if (!m_contextData.m_leaders.empty()) { m_contextData.m_leaders.back().m_leaderLines.emplace_back(); }
      m_parseState = ParseState::LeaderLine;
      return;
    }
    if (code == 304 && marker == "{") {
      // Opens the MText content sub-block inside CONTEXT_DATA.
      m_parseState = ParseState::MTextContent;
      return;
    }
    if (code == 305 && marker == "}") {
      m_parseState = ParseState::Leader;
      return;
    }
    // Unrecognized bracket marker — fall through.
    return;
  }

  switch (m_parseState) {
    // ---------------------------------------------------------------
    case ParseState::LeaderLine: {
      if (m_contextData.m_leaders.empty() || m_contextData.m_leaders.back().m_leaderLines.empty()) { break; }
      auto& line = m_contextData.m_leaders.back().m_leaderLines.back();
      switch (code) {
        case 10:
          line.m_vertices.emplace_back();
          line.m_vertices.back().x = reader->GetDouble();
          break;
        case 20:
          if (!line.m_vertices.empty()) { line.m_vertices.back().y = reader->GetDouble(); }
          break;
        case 30:
          if (!line.m_vertices.empty()) { line.m_vertices.back().z = reader->GetDouble(); }
          break;
        case 91:
          line.m_leaderLineIndex = reader->GetInt32();
          break;
        case 92:
          line.m_leaderLineColorOverride = reader->GetInt32();
          break;
        case 171:
          line.m_leaderLineWeightOverride = reader->GetInt16();
          break;
        case 40:
          line.m_arrowheadSize = reader->GetDouble();
          break;
        case 340:
          line.m_leaderLineTypeHandle = reader->GetHandleString();
          break;
        case 341:
          line.m_arrowheadHandle = reader->GetHandleString();
          break;
        default:
          EoDxfEntity::ParseCode(code, reader);
          break;
      }
      break;
    }

    // ---------------------------------------------------------------
    case ParseState::Leader: {
      if (m_contextData.m_leaders.empty()) { break; }
      auto& branch = m_contextData.m_leaders.back();
      switch (code) {
        case 290:
          branch.m_hasSetLastLeaderLinePoint = reader->GetBool();
          break;
        case 291:
          branch.m_hasSetDoglegVector = reader->GetBool();
          break;
        case 10:
          branch.m_lastLeaderLinePoint.x = reader->GetDouble();
          break;
        case 20:
          branch.m_lastLeaderLinePoint.y = reader->GetDouble();
          break;
        case 30:
          branch.m_lastLeaderLinePoint.z = reader->GetDouble();
          break;
        case 11:
          branch.m_doglegVector.x = reader->GetDouble();
          break;
        case 21:
          branch.m_doglegVector.y = reader->GetDouble();
          break;
        case 31:
          branch.m_doglegVector.z = reader->GetDouble();
          break;
        case 90:
          branch.m_leaderBranchIndex = reader->GetInt32();
          break;
        case 40:
          branch.m_doglegLength = reader->GetDouble();
          break;
        default:
          EoDxfEntity::ParseCode(code, reader);
          break;
      }
      break;
    }

    // ---------------------------------------------------------------
    case ParseState::MTextContent: {
      auto& ctx = m_contextData;
      switch (code) {
        case 12:
          ctx.m_textLocation.x = reader->GetDouble();
          break;
        case 22:
          ctx.m_textLocation.y = reader->GetDouble();
          break;
        case 32:
          ctx.m_textLocation.z = reader->GetDouble();
          break;
        case 13:
          ctx.m_textDirection.x = reader->GetDouble();
          break;
        case 23:
          ctx.m_textDirection.y = reader->GetDouble();
          break;
        case 33:
          ctx.m_textDirection.z = reader->GetDouble();
          break;
        case 42:
          ctx.m_textRotation = reader->GetDouble();
          break;
        case 43:
          ctx.m_textWidth = reader->GetDouble();
          break;
        case 44:
          ctx.m_textDefinedWidth = reader->GetDouble();
          break;
        case 45:
          ctx.m_textDefinedHeight = reader->GetDouble();
          break;
        case 170:
          ctx.m_textAttachment = reader->GetInt16();
          break;
        case 90:
          ctx.m_textFlowDirection = reader->GetInt32();
          break;
        case 91:
          ctx.m_textColor = reader->GetInt32();
          break;
        case 141:
          ctx.m_textLineSpacingFactor = reader->GetDouble();
          break;
        case 171:
          ctx.m_textLineSpacingStyle = reader->GetInt16();
          break;
        case 172:
          ctx.m_textBackgroundFill = reader->GetInt16();
          break;
        case 340:
          ctx.m_textStyleHandle = reader->GetHandleString();
          break;
        case 1:
          ctx.m_textString = reader->GetUtf8String();
          break;
        case 3:
          ctx.m_textString += reader->GetUtf8String();
          break;
        default:
          EoDxfEntity::ParseCode(code, reader);
          break;
      }
      break;
    }

    // ---------------------------------------------------------------
    case ParseState::ContextData: {
      auto& ctx = m_contextData;
      switch (code) {
        case 40:
          ctx.m_contentScale = reader->GetDouble();
          break;
        case 10:
          ctx.m_contentBasePoint.x = reader->GetDouble();
          break;
        case 20:
          ctx.m_contentBasePoint.y = reader->GetDouble();
          break;
        case 30:
          ctx.m_contentBasePoint.z = reader->GetDouble();
          break;
        case 41:
          ctx.m_textHeight = reader->GetDouble();
          break;
        case 140:
          ctx.m_arrowheadSize = reader->GetDouble();
          break;
        case 145:
          ctx.m_landingGap = reader->GetDouble();
          break;
        case 174:
          ctx.m_textLeftAttachment = reader->GetInt16();
          break;
        case 175:
          ctx.m_textRightAttachment = reader->GetInt16();
          break;
        case 176:
          ctx.m_textAlignmentType = reader->GetInt16();
          break;
        case 177:
          ctx.m_blockContentConnectionType = reader->GetInt16();
          break;
        case 290:
          ctx.m_hasMText = reader->GetBool();
          break;
        case 296:
          ctx.m_hasContent = reader->GetBool();
          break;
        case 341:
          ctx.m_blockContentHandle = reader->GetHandleString();
          break;
        case 14:
          ctx.m_blockContentNormalDirection.x = reader->GetDouble();
          break;
        case 24:
          ctx.m_blockContentNormalDirection.y = reader->GetDouble();
          break;
        case 34:
          ctx.m_blockContentNormalDirection.z = reader->GetDouble();
          break;
        case 15:
          ctx.m_blockContentScale.x = reader->GetDouble();
          break;
        case 25:
          ctx.m_blockContentScale.y = reader->GetDouble();
          break;
        case 35:
          ctx.m_blockContentScale.z = reader->GetDouble();
          break;
        case 46:
          ctx.m_blockContentRotation = reader->GetDouble();
          break;
        case 93:
          ctx.m_blockContentColor = reader->GetInt32();
          break;
        default:
          EoDxfEntity::ParseCode(code, reader);
          break;
      }
      break;
    }

    // ---------------------------------------------------------------
    case ParseState::TopLevel:
    default: {
      switch (code) {
        case 170:
          m_leaderType = reader->GetInt16();
          break;
        case 91:
          m_leaderLineColor = reader->GetInt32();
          break;
        case 341:
          m_leaderLineTypeHandle = reader->GetHandleString();
          break;
        case 171:
          m_leaderLineWeight = reader->GetInt16();
          break;
        case 290:
          m_enableLanding = reader->GetBool();
          break;
        case 291:
          m_enableDogleg = reader->GetBool();
          break;
        case 41:
          m_doglegLength = reader->GetDouble();
          break;
        case 42:
          m_arrowheadSize = reader->GetDouble();
          break;
        case 172:
          m_contentType = reader->GetInt16();
          break;
        case 343:
          m_textStyleHandle = reader->GetHandleString();
          break;
        case 173:
          m_textLeftAttachmentType = reader->GetInt16();
          break;
        case 95:
          m_textRightAttachmentType = reader->GetInt32();
          break;
        case 174:
          m_textAngleType = reader->GetInt16();
          break;
        case 175:
          m_textAlignmentType = reader->GetInt16();
          break;
        case 92:
          m_textColor = reader->GetInt32();
          break;
        case 292:
          m_enableFrameText = reader->GetBool();
          break;
        case 344:
          m_blockContentHandle = reader->GetHandleString();
          break;
        case 93:
          m_blockContentColor = reader->GetInt32();
          break;
        case 10:
          m_blockContentScale.x = reader->GetDouble();
          break;
        case 20:
          m_blockContentScale.y = reader->GetDouble();
          break;
        case 30:
          m_blockContentScale.z = reader->GetDouble();
          break;
        case 43:
          m_blockContentRotation = reader->GetDouble();
          break;
        case 176:
          m_blockContentConnectionType = reader->GetInt16();
          break;
        case 293:
          m_enableAnnotationScale = reader->GetBool();
          break;
        case 340:
          m_leaderStyleHandle = reader->GetHandleString();
          break;
        case 90:
          m_propertyOverrideFlag = reader->GetInt32();
          break;
        case 45:
          m_overallScale = reader->GetDouble();
          break;
        case 294:
          m_textDirectionNegative = reader->GetInt32();
          break;
        case 271:
          m_textTopAttachmentType = reader->GetInt16();
          break;
        case 272:
          m_textBottomAttachmentType = reader->GetInt16();
          break;
        default:
          EoDxfEntity::ParseCode(code, reader);
          break;
      }
      break;
    }
  }
}
