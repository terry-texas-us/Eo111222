#include <string>

#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfObjects.h"
#include "EoDxfReader.h"
#include "EoDxfWriter.h"

void EoDxfObjectEntry::ParseCode(int code, EoDxfReader& reader) {
  EoDxfEntity::ParseCode(code, reader);
}

void EoDxfObjectEntry::Reset() {
  m_handle = EoDxf::NoHandle;
  m_ownerHandle = EoDxf::NoHandle;
  m_extensionDictionaryHandle = EoDxf::NoHandle;
  m_reactorHandles.clear();
  m_extendedData.clear();
  m_currentVariant = nullptr;
  m_appData.clear();
}

void EoDxfImageDefinition::ParseCode(int code, EoDxfReader& reader) {
  switch (code) {
    case 1:
      m_fileNameOfImage = reader.GetWideString();
      break;
    case 90:
      m_classVersion = reader.GetInt32();
      break;
    case 10:
      m_uImageSizeInPixels = reader.GetDouble();
      break;
    case 20:
      m_vImageSizeInPixels = reader.GetDouble();
      break;
    case 11:
      m_uSizeOfOnePixel = reader.GetDouble();
      break;
    case 12:
      [[fallthrough]];  // Group code 12 is used for the v size of one pixel in the DXF documentation.
    case 21:  // However, we will always write group code 21 for the v size of one pixel.
      m_vSizeOfOnePixel = reader.GetDouble();
      break;
    case 280:
      m_imageIsLoadedFlag = reader.GetInt16();
      break;
    case 281:
      m_resolutionUnits = reader.GetInt16();
      break;
    default:
      EoDxfObjectEntry::ParseCode(code, reader);
      break;
  }
}

void EoDxfImageDefinition::Reset() noexcept {
  m_classVersion = 0;
  m_uImageSizeInPixels = 0.0;
  m_vImageSizeInPixels = 0.0;
  m_uSizeOfOnePixel = 0.0;
  m_vSizeOfOnePixel = 0.0;
  m_imageIsLoadedFlag = 0;
  m_resolutionUnits = 0;
  EoDxfObjectEntry::Reset();
}

void EoDxfUnsupportedObject::Write(EoDxfWriter* writer) const {
  if (writer == nullptr || m_objectType.empty()) { return; }

  writer->WriteWideString(0, m_objectType);
  for (const auto& value : m_values) {
    const auto code = value.Code();
    if (const auto* wideStringValue = value.GetIf<std::wstring>()) {
      writer->WriteWideString(code, *wideStringValue);
    } else if (const auto* int16Value = value.GetIf<std::int16_t>()) {
      writer->WriteInt16(code, *int16Value);
    } else if (const auto* int32Value = value.GetIf<std::int32_t>()) {
      writer->WriteInt32(code, *int32Value);
    } else if (const auto* int64Value = value.GetIf<std::int64_t>()) {
      writer->WriteInt64(code, *int64Value);
    } else if (const auto* boolValue = value.GetIf<bool>()) {
      writer->WriteBool(code, *boolValue);
    } else if (const auto* doubleValue = value.GetIf<double>()) {
      writer->WriteDouble(code, *doubleValue);
    }
  }
}

// ── EoDxfLayout ──────────────────────────────────────────────────────────────

void EoDxfLayout::Reset() noexcept {
  m_currentSubclass = Subclass::None;

  // AcDbPlotSettings
  m_pageSetupName.clear();
  m_plotConfigName.clear();
  m_paperSizeName.clear();
  m_plotViewName.clear();
  m_leftMargin = 0.0;
  m_bottomMargin = 0.0;
  m_rightMargin = 0.0;
  m_topMargin = 0.0;
  m_paperWidth = 0.0;
  m_paperHeight = 0.0;
  m_plotOriginX = 0.0;
  m_plotOriginY = 0.0;
  m_plotWindowLowerLeftX = 0.0;
  m_plotWindowLowerLeftY = 0.0;
  m_plotWindowUpperRightX = 0.0;
  m_plotWindowUpperRightY = 0.0;
  m_customScaleNumerator = 1.0;
  m_customScaleDenominator = 1.0;
  m_plotLayoutFlags = 0;
  m_plotPaperUnits = 0;
  m_plotRotation = 0;
  m_plotType = 0;
  m_currentStyleSheet.clear();
  m_standardScaleType = 0;
  m_shadePlotMode = 0;
  m_shadePlotResLevel = 0;
  m_shadePlotCustomDpi = 0;
  m_scaleFactor = 1.0;
  m_paperImageOriginX = 0.0;
  m_paperImageOriginY = 0.0;

  // AcDbLayout
  m_layoutName.clear();
  m_layoutFlags = 0;
  m_tabOrder = 0;
  m_limminX = 0.0;
  m_limminY = 0.0;
  m_limmaxX = 0.0;
  m_limmaxY = 0.0;
  m_insertBaseX = 0.0;
  m_insertBaseY = 0.0;
  m_insertBaseZ = 0.0;
  m_extminX = 0.0;
  m_extminY = 0.0;
  m_extminZ = 0.0;
  m_extmaxX = 0.0;
  m_extmaxY = 0.0;
  m_extmaxZ = 0.0;
  m_elevation = 0.0;
  m_ucsOriginX = 0.0;
  m_ucsOriginY = 0.0;
  m_ucsOriginZ = 0.0;
  m_ucsXAxisX = 1.0;
  m_ucsXAxisY = 0.0;
  m_ucsXAxisZ = 0.0;
  m_ucsYAxisX = 0.0;
  m_ucsYAxisY = 1.0;
  m_ucsYAxisZ = 0.0;
  m_ucsOrthoType = 0;
  m_blockRecordHandle = EoDxf::NoHandle;
  m_lastActiveViewportHandle = EoDxf::NoHandle;

  EoDxfObjectEntry::Reset();
}

void EoDxfLayout::ParseCode(int code, EoDxfReader& reader) {
  // Track subclass transitions — codes 1, 70, 76, 330 have different meanings in each section.
  if (code == 100) {
    const auto subclassName = reader.GetWideString();
    if (subclassName == L"AcDbPlotSettings") {
      m_currentSubclass = Subclass::PlotSettings;
    } else if (subclassName == L"AcDbLayout") {
      m_currentSubclass = Subclass::Layout;
    }
    return;
  }

  if (m_currentSubclass == Subclass::PlotSettings) {
    switch (code) {
      case 1: m_pageSetupName = reader.GetWideString(); return;
      case 2: m_plotConfigName = reader.GetWideString(); return;
      case 4: m_paperSizeName = reader.GetWideString(); return;
      case 6: m_plotViewName = reader.GetWideString(); return;
      case 7: m_currentStyleSheet = reader.GetWideString(); return;
      case 40: m_leftMargin = reader.GetDouble(); return;
      case 41: m_bottomMargin = reader.GetDouble(); return;
      case 42: m_rightMargin = reader.GetDouble(); return;
      case 43: m_topMargin = reader.GetDouble(); return;
      case 44: m_paperWidth = reader.GetDouble(); return;
      case 45: m_paperHeight = reader.GetDouble(); return;
      case 46: m_plotOriginX = reader.GetDouble(); return;
      case 47: m_plotOriginY = reader.GetDouble(); return;
      case 48: m_plotWindowLowerLeftX = reader.GetDouble(); return;
      case 49: m_plotWindowLowerLeftY = reader.GetDouble(); return;
      case 70: m_plotLayoutFlags = reader.GetInt16(); return;
      case 72: m_plotPaperUnits = reader.GetInt16(); return;
      case 73: m_plotRotation = reader.GetInt16(); return;
      case 74: m_plotType = reader.GetInt16(); return;
      case 75: m_standardScaleType = reader.GetInt16(); return;
      case 76: m_shadePlotMode = reader.GetInt16(); return;
      case 77: m_shadePlotResLevel = reader.GetInt16(); return;
      case 78: m_shadePlotCustomDpi = reader.GetInt16(); return;
      case 140: m_plotWindowUpperRightX = reader.GetDouble(); return;
      case 141: m_plotWindowUpperRightY = reader.GetDouble(); return;
      case 142: m_customScaleNumerator = reader.GetDouble(); return;
      case 143: m_customScaleDenominator = reader.GetDouble(); return;
      case 147: m_scaleFactor = reader.GetDouble(); return;
      case 148: m_paperImageOriginX = reader.GetDouble(); return;
      case 149: m_paperImageOriginY = reader.GetDouble(); return;
      default: break;
    }
  } else if (m_currentSubclass == Subclass::Layout) {
    switch (code) {
      case 1: m_layoutName = reader.GetWideString(); return;
      case 70: m_layoutFlags = reader.GetInt16(); return;
      case 71: m_tabOrder = reader.GetInt16(); return;
      case 10: m_limminX = reader.GetDouble(); return;
      case 20: m_limminY = reader.GetDouble(); return;
      case 11: m_limmaxX = reader.GetDouble(); return;
      case 21: m_limmaxY = reader.GetDouble(); return;
      case 12: m_insertBaseX = reader.GetDouble(); return;
      case 22: m_insertBaseY = reader.GetDouble(); return;
      case 32: m_insertBaseZ = reader.GetDouble(); return;
      case 13: m_ucsOriginX = reader.GetDouble(); return;
      case 23: m_ucsOriginY = reader.GetDouble(); return;
      case 33: m_ucsOriginZ = reader.GetDouble(); return;
      case 14: m_extminX = reader.GetDouble(); return;
      case 24: m_extminY = reader.GetDouble(); return;
      case 34: m_extminZ = reader.GetDouble(); return;
      case 15: m_extmaxX = reader.GetDouble(); return;
      case 25: m_extmaxY = reader.GetDouble(); return;
      case 35: m_extmaxZ = reader.GetDouble(); return;
      case 16: m_ucsXAxisX = reader.GetDouble(); return;
      case 26: m_ucsXAxisY = reader.GetDouble(); return;
      case 36: m_ucsXAxisZ = reader.GetDouble(); return;
      case 17: m_ucsYAxisX = reader.GetDouble(); return;
      case 27: m_ucsYAxisY = reader.GetDouble(); return;
      case 37: m_ucsYAxisZ = reader.GetDouble(); return;
      case 76: m_ucsOrthoType = reader.GetInt16(); return;
      case 146: m_elevation = reader.GetDouble(); return;
      case 330: m_blockRecordHandle = reader.GetHandleString(); return;
      case 331: m_lastActiveViewportHandle = reader.GetHandleString(); return;
      default: break;
    }
  }

  // Entity header codes (handle, owner, reactors, xdictionary, extended data)
  // are handled by the base class.
  EoDxfObjectEntry::ParseCode(code, reader);
}

void EoDxfLayout::Write(EoDxfWriter* writer) const {
  if (writer == nullptr) { return; }

  writer->WriteWideString(0, L"LAYOUT");

  // Entity header: handle
  if (m_handle != EoDxf::NoHandle) {
    // Format handle as hex string
    wchar_t handleBuffer[20]{};
    swprintf_s(handleBuffer, L"%I64X", m_handle);
    writer->WriteWideString(5, handleBuffer);
  }

  // Reactor handles (ACAD_REACTORS group)
  if (!m_reactorHandles.empty()) {
    writer->WriteWideString(102, L"{ACAD_REACTORS");
    for (const auto reactorHandle : m_reactorHandles) {
      wchar_t reactorBuffer[20]{};
      swprintf_s(reactorBuffer, L"%I64X", reactorHandle);
      writer->WriteWideString(330, reactorBuffer);
    }
    writer->WriteWideString(102, L"}");
  }

  // Extension dictionary (ACAD_XDICTIONARY group)
  if (m_extensionDictionaryHandle != EoDxf::NoHandle) {
    writer->WriteWideString(102, L"{ACAD_XDICTIONARY");
    wchar_t xdictBuffer[20]{};
    swprintf_s(xdictBuffer, L"%I64X", m_extensionDictionaryHandle);
    writer->WriteWideString(360, xdictBuffer);
    writer->WriteWideString(102, L"}");
  }

  // Owner handle
  if (m_ownerHandle != EoDxf::NoHandle) {
    wchar_t ownerBuffer[20]{};
    swprintf_s(ownerBuffer, L"%I64X", m_ownerHandle);
    writer->WriteWideString(330, ownerBuffer);
  }

  // ── AcDbPlotSettings ─────────────────────────────────────────────────────────
  writer->WriteWideString(100, L"AcDbPlotSettings");
  writer->WriteWideString(1, m_pageSetupName);
  writer->WriteWideString(2, m_plotConfigName);
  writer->WriteWideString(4, m_paperSizeName);
  writer->WriteWideString(6, m_plotViewName);
  writer->WriteDouble(40, m_leftMargin);
  writer->WriteDouble(41, m_bottomMargin);
  writer->WriteDouble(42, m_rightMargin);
  writer->WriteDouble(43, m_topMargin);
  writer->WriteDouble(44, m_paperWidth);
  writer->WriteDouble(45, m_paperHeight);
  writer->WriteDouble(46, m_plotOriginX);
  writer->WriteDouble(47, m_plotOriginY);
  writer->WriteDouble(48, m_plotWindowLowerLeftX);
  writer->WriteDouble(49, m_plotWindowLowerLeftY);
  writer->WriteDouble(140, m_plotWindowUpperRightX);
  writer->WriteDouble(141, m_plotWindowUpperRightY);
  writer->WriteDouble(142, m_customScaleNumerator);
  writer->WriteDouble(143, m_customScaleDenominator);
  writer->WriteInt16(70, m_plotLayoutFlags);
  writer->WriteInt16(72, m_plotPaperUnits);
  writer->WriteInt16(73, m_plotRotation);
  writer->WriteInt16(74, m_plotType);
  writer->WriteWideString(7, m_currentStyleSheet);
  writer->WriteInt16(75, m_standardScaleType);
  writer->WriteInt16(76, m_shadePlotMode);
  writer->WriteInt16(77, m_shadePlotResLevel);
  writer->WriteInt16(78, m_shadePlotCustomDpi);
  writer->WriteDouble(147, m_scaleFactor);
  writer->WriteDouble(148, m_paperImageOriginX);
  writer->WriteDouble(149, m_paperImageOriginY);

  // ── AcDbLayout ───────────────────────────────────────────────────────────────
  writer->WriteWideString(100, L"AcDbLayout");
  writer->WriteWideString(1, m_layoutName);
  writer->WriteInt16(70, m_layoutFlags);
  writer->WriteInt16(71, m_tabOrder);
  writer->WriteDouble(10, m_limminX);
  writer->WriteDouble(20, m_limminY);
  writer->WriteDouble(11, m_limmaxX);
  writer->WriteDouble(21, m_limmaxY);
  writer->WriteDouble(12, m_insertBaseX);
  writer->WriteDouble(22, m_insertBaseY);
  writer->WriteDouble(32, m_insertBaseZ);
  writer->WriteDouble(14, m_extminX);
  writer->WriteDouble(24, m_extminY);
  writer->WriteDouble(34, m_extminZ);
  writer->WriteDouble(15, m_extmaxX);
  writer->WriteDouble(25, m_extmaxY);
  writer->WriteDouble(35, m_extmaxZ);
  writer->WriteDouble(146, m_elevation);
  writer->WriteDouble(13, m_ucsOriginX);
  writer->WriteDouble(23, m_ucsOriginY);
  writer->WriteDouble(33, m_ucsOriginZ);
  writer->WriteDouble(16, m_ucsXAxisX);
  writer->WriteDouble(26, m_ucsXAxisY);
  writer->WriteDouble(36, m_ucsXAxisZ);
  writer->WriteDouble(17, m_ucsYAxisX);
  writer->WriteDouble(27, m_ucsYAxisY);
  writer->WriteDouble(37, m_ucsYAxisZ);
  writer->WriteInt16(76, m_ucsOrthoType);

  if (m_blockRecordHandle != EoDxf::NoHandle) {
    wchar_t blockRecBuffer[20]{};
    swprintf_s(blockRecBuffer, L"%I64X", m_blockRecordHandle);
    writer->WriteWideString(330, blockRecBuffer);
  }

  if (m_lastActiveViewportHandle != EoDxf::NoHandle) {
    wchar_t viewportBuffer[20]{};
    swprintf_s(viewportBuffer, L"%I64X", m_lastActiveViewportHandle);
    writer->WriteWideString(331, viewportBuffer);
  }

  // Write extended data (group codes 1000+)
  for (const auto& variant : m_extendedData) {
    if (variant == nullptr) { continue; }
    const auto extCode = variant->Code();
    if (const auto* wideStringValue = variant->GetIf<std::wstring>()) {
      writer->WriteWideString(extCode, *wideStringValue);
    } else if (const auto* int16Value = variant->GetIf<std::int16_t>()) {
      writer->WriteInt16(extCode, *int16Value);
    } else if (const auto* int32Value = variant->GetIf<std::int32_t>()) {
      writer->WriteInt32(extCode, *int32Value);
    } else if (const auto* doubleValue = variant->GetIf<double>()) {
      writer->WriteDouble(extCode, *doubleValue);
    }
  }
}
