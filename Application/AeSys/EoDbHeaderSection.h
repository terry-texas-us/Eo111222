#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

/// Header variable value — extended to support DXF handle (uint64_t) and boolean round-trip.
using HeaderVariable = std::variant<double, int, std::wstring, EoGePoint3d, EoGeVector3d, std::uint64_t, bool>;

class EoDbHeaderSection {
 public:
  EoDbHeaderSection() noexcept = default;
  EoDbHeaderSection(const EoDbHeaderSection&) = delete;
  EoDbHeaderSection& operator=(const EoDbHeaderSection&) = delete;
  virtual ~EoDbHeaderSection() {}

  /** @brief Sets the header variables.
   *
   * @param variables An unordered map containing the header variables to set.
   */
  void SetVariables(const std::unordered_map<std::wstring, HeaderVariable>& variables) { m_variables = variables; }

  /** @brief Retrieves the header variables.
   *
   * @return A constant reference to an unordered map containing the header variables.
   */
  const std::unordered_map<std::wstring, HeaderVariable>& GetVariables() const noexcept { return m_variables; }

  /** @brief Sets a specific header variable (no group code — uses default 0).
   *
   * @param name The name of the header variable to set.
   * @param value The value of the header variable to set.
   */
  void SetVariable(const std::wstring& name, const HeaderVariable& value) { m_variables[name] = value; }

  /** @brief Sets a specific header variable together with its DXF group code.
   *
   * The group code is preserved so that the variable can be written back to DXF
   * with the correct code (e.g. 40 for doubles, 70 for int16, 5 for handles).
   *
   * @param name The name of the header variable to set.
   * @param value The value of the header variable to set.
   * @param groupCode The DXF group code associated with this variable.
   */
  void SetVariable(const std::wstring& name, const HeaderVariable& value, int groupCode) {
    m_variables[name] = value;
    m_groupCodes[name] = groupCode;
  }

  /** @brief Retrieves a specific header variable.
   *
   * @param name The name of the header variable to retrieve.
   * @return A pointer to the HeaderVariable if found, nullptr otherwise.
   */
  const HeaderVariable* SetVariable(const std::wstring& name) const {
    const auto iterator = m_variables.find(name);
    return iterator != m_variables.end() ? &iterator->second : nullptr;
  }

  /** @brief Retrieves the DXF group code for a header variable.
   *
   * @param name The name of the header variable.
   * @return The group code if stored, or 0 if no group code was recorded.
   */
  [[nodiscard]] int GetGroupCode(const std::wstring& name) const {
    const auto iterator = m_groupCodes.find(name);
    return iterator != m_groupCodes.end() ? iterator->second : 0;
  }

  /** @brief Returns the group code map (read-only).
   *
   * @return A constant reference to the group code map.
   */
  [[nodiscard]] const std::unordered_map<std::wstring, int>& GetGroupCodes() const noexcept { return m_groupCodes; }

  /** @brief Provides iterators to traverse the header variables.
   *
   * @return Iterators for the beginning and end of the header variables map.
   */
  auto Begin() noexcept { return m_variables.begin(); }
  auto End() noexcept { return m_variables.end(); }

  /** @brief Provides constant iterators to traverse the header variables.
   *
   * @return Constant iterators for the beginning and end of the header variables map.
   */
  auto Begin() const noexcept { return m_variables.begin(); }
  auto End() const noexcept { return m_variables.end(); }

  // void Read(AeSysDoc* document, EoDbPegFile& pegFile);
  // void Write(AeSysDoc* document, EoDbPegFile& pegFile);

 private:
  std::unordered_map<std::wstring, HeaderVariable> m_variables;

  /// DXF group code per variable name — preserved for correct round-trip export.
  std::unordered_map<std::wstring, int> m_groupCodes;
};