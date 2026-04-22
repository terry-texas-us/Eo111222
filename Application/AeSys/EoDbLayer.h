#pragma once

#include <cstdint>
#include <utility>

#include "EoDbGroupList.h"
#include "EoDbLineType.h"
#include "EoDxfLineWeights.h"

/** @brief Represents a layer in the drawing, which can contain groups of entities and has properties such as color and
 line type.
 * Layer states:
 *   b0 set - resident - layer is displayed using bitmap when possible
 *   b1 set - internal - layer is externally referenced. This means it is not necessary to write disk.
 *   b2 set - work
 *   b3 set - active - may have groups modified (0 or more), displayed using warm color set
 *   b4 set - static - tracing which is viewed or layer which is static (no additions or modifications), displayed using
 warm color set
 *   b5 set - off - layer is not displayed
 * Tracing states:
 *   b2 set - tracing is opened
     b3 set - tracing is mapped
     b4 set - tracing is viewed
     b5 set - tracing is cloaked
*/

class EoDbLayer : public EoDbGroupList {
  CString m_name;  // layer name. If layer is externally referenced this is the full specification of the file name.
  std::uint16_t m_state;  // layer state flag values
  std::uint16_t m_tracingState;  // Tracing state flags
  std::int16_t m_color;  // color index, negative if layer is off)
  EoDbLineType* m_lineType;
  EoDxfLineWeights::LineWeight m_lineWeight{EoDxfLineWeights::LineWeight::kLnWtByLwDefault};
  double m_lineTypeScale{1.0};
  bool m_isFrozen{};
  bool m_isLocked{};
  bool m_plottingFlag{true};
  std::int32_t m_color24{-1};  ///< DXF group code 420; -1 means not set (use ACI color)
  std::uint64_t m_handle{};
  std::uint64_t m_ownerHandle{};
  std::wstring m_tracingFilePath;  ///< Absolute path to .tra file; non-empty only for |name tracing layers

 public:
  enum class State : std::uint16_t {
    isResident = 0x0001,
    isInternal = 0x0002,
    isWork = 0x0004,
    isActive = 0x0008,
    isStatic = 0x0010,
    isOff = 0x0020
  };
  enum class TracingState : std::uint16_t {
    isOpened = 0x0004,
    isMapped = 0x0008,
    isViewed = 0x0010,
    isCloaked = 0x0020
  };

  static constexpr std::uint16_t AllStateBits = 0xFFFF;

  EoDbLayer(const CString& name, State state);
  EoDbLayer(const CString& name, std::uint16_t state);

  EoDbLayer(const EoDbLayer& layer) = delete;
  EoDbLayer& operator=(const EoDbLayer&) = delete;

  ~EoDbLayer() override {}

  void Display(AeSysView* view, EoGsRenderDevice* renderDevice);
  void Display(AeSysView* view, EoGsRenderDevice* renderDevice, bool identifyTrap);

  [[nodiscard]] COLORREF ColorValue() const { return Eo::ColorPalette[m_color]; }

  [[nodiscard]] std::int16_t ColorIndex() const noexcept { return m_color; }
  void SetColorIndex(std::int16_t color) noexcept { m_color = color; }

  [[nodiscard]] std::uint64_t Handle() const noexcept { return m_handle; }
  [[nodiscard]] std::uint64_t OwnerHandle() const noexcept { return m_ownerHandle; }
  void SetHandle(std::uint64_t handle) noexcept { m_handle = handle; }
  void SetOwnerHandle(std::uint64_t ownerHandle) noexcept { m_ownerHandle = ownerHandle; }

  [[nodiscard]] EoDbLineType* LineType() const noexcept { return m_lineType; }

  [[nodiscard]] std::int16_t LineTypeIndex() const;

  [[nodiscard]] CString LineTypeName() const { return (m_lineType != nullptr) ? m_lineType->Name() : CString{}; }

  void SetLineType(EoDbLineType* lineType) noexcept { m_lineType = lineType; }

  [[nodiscard]] EoDxfLineWeights::LineWeight LineWeight() const noexcept { return m_lineWeight; }
  void SetLineWeight(EoDxfLineWeights::LineWeight lineWeight) noexcept { m_lineWeight = lineWeight; }

  [[nodiscard]] double LineTypeScale() const noexcept { return m_lineTypeScale; }
  void SetLineTypeScale(double lineTypeScale) noexcept { m_lineTypeScale = lineTypeScale; }

  [[nodiscard]] bool IsFrozen() const noexcept { return m_isFrozen; }
  void SetFrozen(bool frozen) noexcept { m_isFrozen = frozen; }

  [[nodiscard]] bool IsLocked() const noexcept { return m_isLocked; }
  void SetLocked(bool locked) noexcept { m_isLocked = locked; }

  [[nodiscard]] bool PlottingFlag() const noexcept { return m_plottingFlag; }
  void SetPlottingFlag(bool plottingFlag) noexcept { m_plottingFlag = plottingFlag; }

  [[nodiscard]] std::int32_t Color24() const noexcept { return m_color24; }
  void SetColor24(std::int32_t color24) noexcept { m_color24 = color24; }
  void PenTranslation(std::uint16_t, std::int16_t*, std::int16_t*);

  [[nodiscard]] CString Name() const noexcept { return m_name; }
  void SetName(const CString& name) noexcept { m_name = name; }

  /// Returns true when this layer is an embedded tracing reference (name starts with '|').
  [[nodiscard]] bool IsTracingLayer() const noexcept { return !m_name.IsEmpty() && m_name[0] == L'|'; }

  [[nodiscard]] const std::wstring& TracingFilePath() const noexcept { return m_tracingFilePath; }
  void SetTracingFilePath(const std::wstring& path) { m_tracingFilePath = path; }

  void ClearTracingStateBit(std::uint16_t w = AllStateBits) noexcept { m_tracingState &= ~w; }

  [[nodiscard]] bool IsOpened() const noexcept {
    return (
        (m_tracingState & std::to_underlying(TracingState::isOpened)) == std::to_underlying(TracingState::isOpened));
  }
  [[nodiscard]] bool IsMapped() const noexcept {
    return (
        (m_tracingState & std::to_underlying(TracingState::isMapped)) == std::to_underlying(TracingState::isMapped));
  }
  [[nodiscard]] bool IsViewed() const noexcept {
    return (
        (m_tracingState & std::to_underlying(TracingState::isViewed)) == std::to_underlying(TracingState::isViewed));
  }
  [[nodiscard]] std::uint16_t GetTracingState() const noexcept { return m_tracingState; }

  void SetTracingState(std::uint16_t state) noexcept { m_tracingState = state; }

  void ClearStateFlag(std::uint16_t w = AllStateBits) noexcept { m_state &= ~w; }

  [[nodiscard]] std::uint16_t GetState() const noexcept { return m_state; }
  [[nodiscard]] bool IsActive() const noexcept {
    return ((m_state & std::to_underlying(State::isActive)) == std::to_underlying(State::isActive));
  }
  [[nodiscard]] bool IsInternal() const noexcept {
    return ((m_state & std::to_underlying(State::isInternal)) == std::to_underlying(State::isInternal));
  }
  [[nodiscard]] bool IsOff() const noexcept {
    return ((m_state & std::to_underlying(State::isOff)) == std::to_underlying(State::isOff));
  }
  [[nodiscard]] bool IsResident() const noexcept {
    return ((m_state & std::to_underlying(State::isResident)) == std::to_underlying(State::isResident));
  }
  [[nodiscard]] bool IsStatic() const noexcept {
    return ((m_state & std::to_underlying(State::isStatic)) == std::to_underlying(State::isStatic));
  }
  [[nodiscard]] bool IsWork() const noexcept {
    return ((m_state & std::to_underlying(State::isWork)) == std::to_underlying(State::isWork));
  }

  void MakeInternal() noexcept { m_state |= std::to_underlying(State::isInternal); }
  void MakeResident() noexcept { m_state |= std::to_underlying(State::isResident); }

  void SetStateActive() noexcept {
    constexpr auto toClear =
        std::to_underlying(State::isWork) | std::to_underlying(State::isStatic) | std::to_underlying(State::isOff);
    m_state &= ~toClear;
    m_state |= std::to_underlying(State::isActive);
  }

  void SetStateOff() noexcept {
    constexpr auto toClear =
        std::to_underlying(State::isWork) | std::to_underlying(State::isActive) | std::to_underlying(State::isStatic);
    m_state &= ~toClear;
    m_state |= std::to_underlying(State::isOff);
  }

  void SetStateStatic() noexcept {
    constexpr auto toClear =
        std::to_underlying(State::isWork) | std::to_underlying(State::isActive) | std::to_underlying(State::isOff);
    m_state &= ~toClear;
    m_state |= std::to_underlying(State::isStatic);
  }

  void SetStateWork() noexcept {
    constexpr auto toClear =
        std::to_underlying(State::isActive) | std::to_underlying(State::isStatic) | std::to_underlying(State::isOff);
    m_state &= ~toClear;
    m_state |= std::to_underlying(State::isWork);
  }
};

inline constexpr EoDbLayer::State operator|(EoDbLayer::State lhs, EoDbLayer::State rhs) noexcept {
  return static_cast<EoDbLayer::State>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

typedef CTypedPtrArray<CObArray, EoDbLayer*> CLayers;
