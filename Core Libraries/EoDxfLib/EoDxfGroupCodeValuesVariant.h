#pragma once

#include <cassert>
#include <cstdint>
#include <string>
#include <variant>

#include "EoDxfGeometry.h"

/** @brief Class representing a variant type for storing different types of values associated with DXF group codes.
 *  The class can hold all DXF Group Coded value types. The active type is tracked automatically by
 *  the underlying std::variant. It provides constructors for each type, as well as compiler-generated copy and move
 *  semantics (Rule of Zero). The Add* methods allow updating the stored value and type after construction.
 *
 * @note To add a new stored type in future:
 *  1. Add the new type to `m_content` in the correct semantic position.
 *  2. Add a constructor overload taking `(int code, NewType value)`.
 *  3. Add an `Add...()` mutator for assigning the new type after construction.
 *  4. Add a `Get...()` accessor and rely on `GetIf<NewType>()` for non-asserting access.
 *  5. Update all DXF read paths that should parse the new group-code range into that type.
 *  6. Update all DXF write paths that should emit that type back out.
 *  7. Update any widening or compatibility helpers such as header retrieval methods.
 *  8. Add round-trip coverage for ASCII DXF and binary DXF persistence.
 *
 *  The preferred extension model is to let `std::variant` remain the single source of truth.
 *  New types should therefore be integrated through direct `GetIf<T>()`, `std::holds_alternative<T>()`,
 *  or `std::visit` usage rather than by introducing parallel type enums.
 */
class EoDxfGroupCodeValuesVariant {
 public:
  EoDxfGroupCodeValuesVariant() = default;

  /// Constructor overloads taking `(int code, NewType value)`
  
  EoDxfGroupCodeValuesVariant(int code, std::int16_t i16) noexcept : m_content{i16}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::int32_t i) noexcept : m_content{i}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::int64_t i64) noexcept : m_content{i64}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::uint64_t h) noexcept : m_content{h}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, bool b) noexcept : m_content{b}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, double d) noexcept : m_content{d}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::wstring s) noexcept : m_content{std::move(s)}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, EoDxfGeometryBase3d gb) noexcept : m_content{gb}, m_code{code} {}

  /// `Add...()` mutator for assigning the new type after construction.

  void AddWideString(int code, std::wstring s) noexcept {
    m_content = std::move(s);
    m_code = code;
  }
  void AddInt16(int code, std::int16_t i16) noexcept {
    m_content = i16;
    m_code = code;
  }
  void AddInt32(int code, std::int32_t i32) noexcept {
    m_content = i32;
    m_code = code;
  }
  void AddInt64(int code, std::int64_t i64) noexcept {
    m_content = i64;
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
  void AddBoolean(int code, bool b) noexcept {
    m_content = b;
    m_code = code;
  }

  template <typename T>
  [[nodiscard]] const T* GetIf() const noexcept {
    return std::get_if<T>(&m_content);
  }

  template <typename T>
  [[nodiscard]] T* GetIf() noexcept {
    return std::get_if<T>(&m_content);
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

  /// `Get...()` accessor and rely on `GetIf<NewType>()` for non-asserting access.

  [[nodiscard]] const std::wstring& GetWideString() const {
    assert(std::holds_alternative<std::wstring>(m_content));
    return std::get<std::wstring>(m_content);
  }
  [[nodiscard]] std::int16_t GetInt16() const {
    assert(std::holds_alternative<std::int16_t>(m_content));
    return std::get<std::int16_t>(m_content);
  }
  [[nodiscard]] std::int32_t GetInt32() const {
    assert(std::holds_alternative<std::int32_t>(m_content));
    return std::get<std::int32_t>(m_content);
  }
  [[nodiscard]] std::int64_t GetInt64() const {
    assert(std::holds_alternative<std::int64_t>(m_content));
    return std::get<std::int64_t>(m_content);
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
  [[nodiscard]] bool GetBoolean() const {
    assert(std::holds_alternative<bool>(m_content));
    return std::get<bool>(m_content);
  }

  [[nodiscard]] int Code() const noexcept { return m_code; }

 private:
  std::variant<std::monostate, std::wstring, std::int16_t, std::int32_t, std::int64_t, std::uint64_t, bool, double,
      EoDxfGeometryBase3d>
      m_content;

  int m_code{};
};
