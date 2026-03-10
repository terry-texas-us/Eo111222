#pragma once

#include <cassert>
#include <cstdint>
#include <string>

#include "EoDxfGeometry.h"

/** @brief Class representing a variant type for storing different types of values associated with DXF group codes.
 *  The class can hold a string, integer, double, or coordinate value, and it tracks the type of value currently stored.
 *  It provides constructors for each type, as well as copy and move semantics. The Add* methods allow updating the
 *  stored value and type after construction.
 */
class EoDxfGroupCodeValuesVariant {
 public:
  enum Type {
    String,
    Int16,  ///< 16-bit signed integer (DXF group codes 60-79, 170-179, 270-289, 370-389, 400-409, 1060-1070)
    Integer,  ///< 32-bit signed integer (DXF group codes 90-99, 420-429, 440-449, 450-459, 1071)
    Double,
    GeometryBase,
    Handle,  ///< 64-bit unsigned handle (DXF group codes 5, 105, 310-369 handle references)
    Invalid
  };

  EoDxfGroupCodeValuesVariant() = default;

  EoDxfGroupCodeValuesVariant(int code, std::int16_t i16) noexcept
      : m_content{i16}, m_variantType{Type::Int16}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::int32_t i) noexcept
      : m_content{i}, m_variantType{Type::Integer}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::uint32_t i) noexcept
      : m_content{static_cast<std::int32_t>(i)}, m_variantType{Type::Integer}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::uint64_t h) noexcept
      : m_content{h}, m_variantType{Type::Handle}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, double d) noexcept : m_content{d}, m_variantType{Type::Double}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, std::string s) noexcept
      : m_stringValue{std::move(s)}, m_content{&m_stringValue}, m_variantType{Type::String}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(int code, EoDxfGeometryBase3d gb) noexcept
      : m_geometryBaseValue{gb}, m_content{&m_geometryBaseValue}, m_variantType{Type::GeometryBase}, m_code{code} {}

  EoDxfGroupCodeValuesVariant(const EoDxfGroupCodeValuesVariant& other)
      : m_stringValue{other.m_stringValue},
        m_geometryBaseValue{other.m_geometryBaseValue},
        m_content{other.m_content},
        m_variantType{other.m_variantType},
        m_code{other.m_code} {
    if (other.m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
    if (other.m_variantType == Type::String) { m_content.s = &m_stringValue; }
  }

  EoDxfGroupCodeValuesVariant& operator=(const EoDxfGroupCodeValuesVariant& other) {
    if (this != &other) {
      m_stringValue = other.m_stringValue;
      m_geometryBaseValue = other.m_geometryBaseValue;
      m_content = other.m_content;
      m_variantType = other.m_variantType;
      m_code = other.m_code;
      if (other.m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
      if (other.m_variantType == Type::String) { m_content.s = &m_stringValue; }
    }
    return *this;
  }

  EoDxfGroupCodeValuesVariant(EoDxfGroupCodeValuesVariant&& other) noexcept
      : m_stringValue{std::move(other.m_stringValue)},
        m_geometryBaseValue{other.m_geometryBaseValue},
        m_content{other.m_content},
        m_variantType{other.m_variantType},
        m_code{other.m_code} {
    if (m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
    if (m_variantType == Type::String) { m_content.s = &m_stringValue; }
  }

  EoDxfGroupCodeValuesVariant& operator=(EoDxfGroupCodeValuesVariant&& other) noexcept {
    if (this != &other) {
      m_stringValue = std::move(other.m_stringValue);
      m_geometryBaseValue = other.m_geometryBaseValue;
      m_content = other.m_content;
      m_variantType = other.m_variantType;
      m_code = other.m_code;
      if (m_variantType == Type::GeometryBase) { m_content.v = &m_geometryBaseValue; }
      if (m_variantType == Type::String) { m_content.s = &m_stringValue; }
    }
    return *this;
  }

  ~EoDxfGroupCodeValuesVariant() = default;

  void AddString(int code, std::string s) {
    m_variantType = Type::String;
    m_stringValue = std::move(s);
    m_content.s = &m_stringValue;
    m_code = code;
  }
  void AddInt16(int code, std::int16_t i16) {
    m_variantType = Type::Int16;
    m_content.i16 = i16;
    m_code = code;
  }
  void AddInteger(int code, int i) {
    m_variantType = Type::Integer;
    m_content.i = i;
    m_code = code;
  }
  void AddDouble(int code, double d) {
    m_variantType = Type::Double;
    m_content.d = d;
    m_code = code;
  }
  void AddGeometryBase(int code, EoDxfGeometryBase3d v) {
    m_variantType = Type::GeometryBase;
    m_geometryBaseValue = v;
    m_content.v = &m_geometryBaseValue;
    m_code = code;
  }
  void AddHandle(int code, std::uint64_t h) {
    m_variantType = Type::Handle;
    m_content.h = h;
    m_code = code;
  }

  void SetGeometryBaseX(double d) {
    assert(m_variantType == Type::GeometryBase);
    m_geometryBaseValue.x = d;
  }
  void SetGeometryBaseY(double d) {
    assert(m_variantType == Type::GeometryBase);
    m_geometryBaseValue.y = d;
  }
  void SetGeometryBaseZ(double d) {
    assert(m_variantType == Type::GeometryBase);
    m_geometryBaseValue.z = d;
  }

  [[nodiscard]] const std::string& GetString() const {
    assert(m_variantType == Type::String);
    return m_stringValue;
  }
  [[nodiscard]] std::int16_t GetInt16() const {
    assert(m_variantType == Type::Int16);
    return m_content.i16;
  }
  [[nodiscard]] std::int32_t GetInteger() const {
    assert(m_variantType == Type::Integer);
    return m_content.i;
  }
  [[nodiscard]] double GetDouble() const {
    assert(m_variantType == Type::Double);
    return m_content.d;
  }
  [[nodiscard]] const EoDxfGeometryBase3d& GetGeometryBase() const {
    assert(m_variantType == Type::GeometryBase);
    return m_geometryBaseValue;
  }
  [[nodiscard]] std::uint64_t GetHandle() const {
    assert(m_variantType == Type::Handle);
    return m_content.h;
  }

  [[nodiscard]] enum Type GetType() const noexcept { return m_variantType; }
  [[nodiscard]] int Code() const noexcept { return m_code; }

 private:
  std::string m_stringValue;
  EoDxfGeometryBase3d m_geometryBaseValue;

  union EoDxfVariantContent {
    std::string* s;
    std::int16_t i16;  ///< 16-bit signed integer (group codes 60-79, 170-179, 270-289, etc.)
    std::int32_t i;
    std::uint64_t h;  ///< handle stored as 64-bit unsigned (parsed from hex string)
    double d;
    EoDxfGeometryBase3d* v;

    EoDxfVariantContent() noexcept : i{} {}
    EoDxfVariantContent(std::string* sd) noexcept : s{sd} {}
    EoDxfVariantContent(std::int16_t i16d) noexcept : i16{i16d} {}
    EoDxfVariantContent(std::int32_t id) noexcept : i{id} {}
    EoDxfVariantContent(std::uint64_t hd) noexcept : h{hd} {}
    EoDxfVariantContent(double dd) noexcept : d{dd} {}
    EoDxfVariantContent(EoDxfGeometryBase3d* gb) noexcept : v{gb} {}
  };

 public:
  EoDxfVariantContent m_content;

 private:
  enum EoDxfGroupCodeValuesVariant::Type m_variantType{Type::Invalid};
  int m_code{};
};
