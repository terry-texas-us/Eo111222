#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <variant>

#include "EoDxfGeometry.h"

/** @brief Class representing a variant type for storing different types of values associated with DXF group codes.
 *  The class can hold a string, integer, double, or coordinate value. The active type is tracked automatically by
 *  the underlying std::variant. It provides constructors for each type, as well as compiler-generated copy and move
 *  semantics (Rule of Zero). The Add* methods allow updating the stored value and type after construction.
 *
 *  Variant index mapping (used internally by GetType()):
 *    0 = std::monostate (Invalid), 1 = std::string (String), 2 = std::int16_t (Int16),
 *    3 = std::int32_t (Integer), 4 = std::uint64_t (Handle), 5 = double (Double),
 *    6 = EoDxfGeometryBase3d (GeometryBase)
 */
class EoDxfGroupCodeValuesVariant {
 public:
  EoDxfGroupCodeValuesVariant() = default;

  EoDxfGroupCodeValuesVariant(int code, std::int16_t i16) noexcept : m_content{i16}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::int32_t i) noexcept : m_content{i}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::uint32_t i) noexcept
      : m_content{static_cast<std::int32_t>(i)}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::uint64_t h) noexcept : m_content{h}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, double d) noexcept : m_content{d}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::string s) noexcept : m_content{std::move(s)}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, EoDxfGeometryBase3d gb) noexcept : m_content{gb}, m_code{code} {}

  // Rule of Zero: compiler-generated copy/move/destructor handle std::variant correctly.

  void AddString(int code, std::string s) {
    m_content = std::move(s);
    m_code = code;
  }
  void AddInt16(int code, std::int16_t i16) noexcept {
    m_content = i16;
    m_code = code;
  }
  void AddInteger(int code, std::int32_t i) noexcept {
    m_content = i;
    m_code = code;
  }
  void AddDouble(int code, double d) noexcept {
    m_content = d;
    m_code = code;
  }
  void AddGeometryBase(int code, EoDxfGeometryBase3d v) noexcept {
    m_content = v;
    m_code = code;
  }
  void AddHandle(int code, std::uint64_t h) noexcept {
    m_content = h;
    m_code = code;
  }

  void SetGeometryBaseX(double d) {
    assert(std::holds_alternative<EoDxfGeometryBase3d>(m_content));
    std::get<EoDxfGeometryBase3d>(m_content).x = d;
  }
  void SetGeometryBaseY(double d) {
    assert(std::holds_alternative<EoDxfGeometryBase3d>(m_content));
    std::get<EoDxfGeometryBase3d>(m_content).y = d;
  }
  void SetGeometryBaseZ(double d) {
    assert(std::holds_alternative<EoDxfGeometryBase3d>(m_content));
    std::get<EoDxfGeometryBase3d>(m_content).z = d;
  }

  [[nodiscard]] const std::string& GetString() const {
    assert(std::holds_alternative<std::string>(m_content));
    return std::get<std::string>(m_content);
  }
  [[nodiscard]] std::int16_t GetInt16() const {
    assert(std::holds_alternative<std::int16_t>(m_content));
    return std::get<std::int16_t>(m_content);
  }
  [[nodiscard]] std::int32_t GetInteger() const {
    assert(std::holds_alternative<std::int32_t>(m_content));
    return std::get<std::int32_t>(m_content);
  }
  [[nodiscard]] double GetDouble() const {
    assert(std::holds_alternative<double>(m_content));
    return std::get<double>(m_content);
  }
  [[nodiscard]] const EoDxfGeometryBase3d& GetGeometryBase() const {
    assert(std::holds_alternative<EoDxfGeometryBase3d>(m_content));
    return std::get<EoDxfGeometryBase3d>(m_content);
  }
  [[nodiscard]] std::uint64_t GetHandle() const {
    assert(std::holds_alternative<std::uint64_t>(m_content));
    return std::get<std::uint64_t>(m_content);
  }

  enum VariantType {
    String,
    Int16,  ///< 16-bit signed integer (DXF group codes 60-79, 170-179, 270-289, 370-389, 400-409, 1060-1070)
    Integer,  ///< 32-bit signed integer (DXF group codes 90-99, 420-429, 440-449, 450-459, 1071)
    Double,
    GeometryBase,
    Handle,  ///< 64-bit unsigned handle (DXF group codes 5, 105, 310-369 handle references)
    Invalid
  };

  [[nodiscard]] enum VariantType GetType() const noexcept {
    if (m_content.valueless_by_exception()) { return Invalid; }
    // Maps std::variant index to the public VariantType enum.
    // Order must match the variant alternative order declared below.
    static constexpr VariantType indexToType[] = {Invalid, String, Int16, Integer, Handle, Double, GeometryBase};
    return indexToType[m_content.index()];
  }

  [[nodiscard]] int Code() const noexcept { return m_code; }

 private:
  /** Discriminated storage — Alternative order: monostate(Invalid), 
   * string(String), int16_t(Int16), int32_t(Integer), uint64_t(Handle), double(Double), EoDxfGeometryBase3d(GeometryBase).*/
  std::variant<std::monostate, std::string, std::int16_t, std::int32_t, std::uint64_t, double, EoDxfGeometryBase3d>
      m_content;

  int m_code{};
};
