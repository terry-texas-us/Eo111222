#pragma once

#include <cstdint>

#include "EoDbGroupList.h"
#include "EoDbLineType.h"

/** @brief Represents a layer in the drawing, which can contain groups of entities and has properties such as color and line type.
 * Layer states:
 *   b0 set - resident - layer is displayed using bitmap when possible
 *   b1 set - internal - layer is externally referenced. This means it is not necessary to write disk.
 *   b2 set - work
 *   b3 set - active - may have groups modified (0 or more), displayed using warm color set
 *   b4 set - static - tracing which is viewed or layer which is static (no additions or modifications), displayed using warm color set
 *   b5 set - off - layer is not displayed
 * Tracing states:
 *   b2 set - tracing is opened
     b3 set - tracing is mapped
     b4 set - tracing is viewed
     b5 set - tracing is cloaked
*/

class EoDbLayer : public EoDbGroupList {
  CString m_name;  // layer name. If layer is externally referenced this is the full specification of the file name.
  std::uint16_t m_state;         // layer state flag values
  std::uint16_t m_tracingState;  // Tracing state flags
  std::int16_t m_color;          // color index, negative if layer is off)
  EoDbLineType* m_lineType;

 public:
  enum State {
    isResident = 0x0001,
    isInternal = 0x0002,
    isWork = 0x0004,
    isActive = 0x0008,
    isStatic = 0x0010,
    isOff = 0x0020
  };
  enum TracingState { isOpened = 0x0004, isMapped = 0x0008, isViewed = 0x0010, isCloaked = 0x0020 };
  
  EoDbLayer(const CString& name, std::uint16_t flags);
  EoDbLayer(const CString& name, std::uint16_t flags, EoDbLineType* lineType);
  EoDbLayer(const EoDbLayer& layer) = delete;
  EoDbLayer& operator=(const EoDbLayer&) = delete;

  ~EoDbLayer() override {}
  void Display(AeSysView* view, CDC* deviceContext);
  void Display(AeSysView* view, CDC* deviceContext, bool identifyTrap);

  COLORREF ColorValue() const;
  [[nodiscard]] std::int16_t ColorIndex() const { return m_color; }
  void SetColorIndex(std::int16_t color) { m_color = color; }

  EoDbLineType* LineType() const;
  std::int16_t LineTypeIndex();
  
  [[nodiscard]] CString LineTypeName() { return m_lineType->Name(); }
  
  void SetLineType(EoDbLineType* lineType);
  void PenTranslation(std::uint16_t, std::int16_t*, std::int16_t*);
  
  [[nodiscard]] CString Name() const { return m_name; }
  void SetName(const CString& name) { m_name = name; }

    void ClearTracingStateBit(std::uint16_t w = 0xffff) { m_tracingState &= ~w; }
  [[nodiscard]] bool IsOpened() const {
    return ((m_tracingState & EoDbLayer::TracingState::isOpened) == EoDbLayer::TracingState::isOpened);
  }
  [[nodiscard]] bool IsMapped() const {
    return ((m_tracingState & EoDbLayer::TracingState::isMapped) == EoDbLayer::TracingState::isMapped);
  }
  [[nodiscard]] bool IsViewed() const {
    return ((m_tracingState & EoDbLayer::TracingState::isViewed) == EoDbLayer::TracingState::isViewed);
  }
  [[nodiscard]] std::uint16_t TracingState() const { return m_tracingState; }
  
  void SetTracingState(std::uint16_t state) { m_tracingState = state; }

  void ClearStateFlag(std::uint16_t w = 0xffff) { m_state &= ~w; }
  
  [[nodiscard]] std::uint16_t State() const { return m_state; }
  [[nodiscard]] bool IsActive() const { return ((m_state & State::isActive) == State::isActive); }
  [[nodiscard]] bool IsInternal() const { return ((m_state & State::isInternal) == State::isInternal); }
  [[nodiscard]] bool IsOff() const { return ((m_state & State::isOff) == State::isOff); }
  [[nodiscard]] bool IsResident() const { return ((m_state & State::isResident) == State::isResident); }
  [[nodiscard]] bool IsStatic() const { return ((m_state & State::isStatic) == State::isStatic); }
  [[nodiscard]] bool IsWork() const { return ((m_state & State::isWork) == State::isWork); }

  void MakeInternal() { m_state |= State::isInternal; }
  void MakeResident() { m_state |= State::isResident; }
  
  void SetStateActive() {
    m_state &= ~(State::isWork | State::isStatic | State::isOff);
    m_state |= State::isActive;
  }

  void SetStateOff() {
    m_state &= ~(State::isWork | State::isActive | State::isStatic);
    m_state |= State::isOff;
  }

  void SetStateStatic() {
    m_state &= ~(State::isWork | State::isActive | State::isOff);
    m_state |= State::isStatic;
  }

  void SetStateWork() {
    m_state &= ~(State::isActive | State::isStatic | State::isOff);
    m_state |= State::isWork;
  }
};

typedef CTypedPtrArray<CObArray, EoDbLayer*> CLayers;
