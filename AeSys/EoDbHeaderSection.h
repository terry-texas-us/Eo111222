#pragma once
#include <string>
#include <unordered_map>
#include <variant>

#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

using HeaderVariable = std::variant<double, int, std::wstring, EoGePoint3d, EoGeVector3d>;

class EoDbHeaderSection {
 public:
  EoDbHeaderSection() = default;
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
  const std::unordered_map<std::wstring, HeaderVariable>& GetVariables() const { return m_variables; }

  /** @brief Sets a specific header variable.
   *
   * @param name The name of the header variable to set.
   * @param value The value of the header variable to set.
   */
  void SetVariable(const std::wstring& name, const HeaderVariable& value) { m_variables[name] = value; }

  /** @brief Retrieves a specific header variable.
   *
   * @param name The name of the header variable to retrieve.
   * @return A pointer to the HeaderVariable if found, nullptr otherwise.
   */
  const HeaderVariable* SetVariable(const std::wstring& name) const {
    auto iterator = m_variables.find(name);
    return iterator != m_variables.end() ? &iterator->second : nullptr;
  }

  /** @brief Provides iterators to traverse the header variables.
   *
   * @return Iterators for the beginning and end of the header variables map.
   */
  auto Begin() { return m_variables.begin(); }
  auto End() { return m_variables.end(); }
  
  /** @brief Provides constant iterators to traverse the header variables.
   *
   * @return Constant iterators for the beginning and end of the header variables map.
   */
  auto Begin() const { return m_variables.begin(); }
  auto End() const { return m_variables.end(); }
  
  //void Read(AeSysDoc* document, EoDbPegFile& pegFile);
  //void Write(AeSysDoc* document, EoDbPegFile& pegFile);

 private:
  std::unordered_map<std::wstring, HeaderVariable> m_variables;
};