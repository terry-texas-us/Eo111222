#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

/// @brief Single command entry in the command-line registry.
///
/// Three dispatch modes — only one should be populated per entry:
///   1. modeId + opId  — sends two sequential WM_COMMAND messages through CMainFrame.
///   2. modeId only    — sends a single WM_COMMAND (mode switch without a sub-op).
///   3. functor        — invokes a callable directly; modeId/opId are 0 and unused.
///
/// Aliases are stored separately in m_lookup; the canonical entry owns helpText.
struct EoCommandEntry {
  std::wstring canonicalName;     ///< Canonical upper-case name, e.g. L"LINE".
  unsigned int modeId{0};         ///< Top-level mode id (e.g. ID_MODE_DRAW). 0 for functor cmds.
  unsigned int opId{0};           ///< Optional op id (e.g. ID_DRAW_MODE_LINE). 0 = none.
  std::wstring helpText;          ///< Short one-line help for the HELP listing.
  std::function<void()> functor;  ///< Direct invocation; takes priority when non-null.
};

/// @brief Singleton registry mapping command names and aliases to mode/op ids or functors.
///
/// All lookup keys are stored in ASCII upper case. The registry is populated
/// once on first access via RegisterDefaults() and is intentionally read-only
/// at runtime — user scripts may call Register() to add project-specific commands.
///
/// ### Alias table (built into RegisterDefaults)
/// Aliases are registered as additional lookup keys pointing at the same entry.
/// They are not listed by HELP unless an entry with the alias as canonicalName
/// also exists. Use the aliases parameter of Register() to add short forms.
class EoCommandRegistry {
 public:
  EoCommandRegistry(const EoCommandRegistry&) = delete;
  EoCommandRegistry& operator=(const EoCommandRegistry&) = delete;

  /// @brief Returns the process-wide registry instance.
  static EoCommandRegistry& Instance();

  /// @brief Registers @p entry under @p name and every alias in @p aliases.
  void Register(const EoCommandEntry& entry, const std::vector<std::wstring>& aliases = {});

  /// @brief Looks up a command by upper-case @p name. Returns nullptr if unknown.
  [[nodiscard]] const EoCommandEntry* Find(const std::wstring& name) const noexcept;

  /// @brief Returns the canonical entries (one per registration, no alias dupes).
  [[nodiscard]] const std::vector<EoCommandEntry>& Entries() const noexcept { return m_entries; }

  /// @brief Returns all registered lookup keys (canonical names + aliases) in sorted order.
  ///        Used by the Tab-completion engine to enumerate candidates.
  [[nodiscard]] std::vector<std::wstring> AllKeys() const;

 private:
  EoCommandRegistry();

  void RegisterDefaults();

  std::vector<EoCommandEntry> m_entries;
  std::unordered_map<std::wstring, std::size_t> m_lookup;
};
