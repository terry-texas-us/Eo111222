#pragma once

#include <cstdint>

class EoDbHandleManager {
  /// @brief First handle safe for user-created entities.  Matches EoDxf::Handles::FirstUserHandle
  /// (0x30 = 48 decimal).  Handles 0x01–0x21 are reserved for DXF infrastructure objects
  /// (symbol tables, block records, built-in blocks, linetypes, etc.).
  static constexpr std::uint64_t kFirstUserHandle{0x30};

  std::uint64_t m_nextHandle{kFirstUserHandle};

 public:
  [[nodiscard]] std::uint64_t AssignHandle() noexcept { return m_nextHandle++; }
  [[nodiscard]] std::uint64_t NextHandleValue() const noexcept { return m_nextHandle; }
  void SetNextHandle(std::uint64_t seed) noexcept { m_nextHandle = seed; }

  /// @brief Resets the handle counter to kFirstUserHandle.  Called from AeSysDoc::DeleteContents()
  /// so that a fresh document starts past the infrastructure handle range rather than continuing
  /// from the previous document's counter.
  void Reset() noexcept { m_nextHandle = kFirstUserHandle; }

  /** @brief Accommodates an existing handle by ensuring the next handle value is greater than it.
   *
   * This function should be called after importing objects with pre-assigned handles to ensure that
   * the next handle assigned by this manager does not conflict with any existing handles.
   *
   * @param existingHandle The handle to accommodate. If this handle is greater than or equal to the current next
   * handle, the next handle will be updated to be one greater than this existing handle.
   */
  void AccommodateHandle(std::uint64_t existingHandle) noexcept {
    if (existingHandle >= m_nextHandle) { m_nextHandle = existingHandle + 1; }
  }
};