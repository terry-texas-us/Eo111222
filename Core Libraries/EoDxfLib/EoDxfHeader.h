#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "EoDxfBase.h"
#include "EoDxfGroupCodeValuesVariant.h"

class EoDxfReader;
class EoDxfWriter;

/** Class to handle header vars, to read iterate over "std::map vars".
 *  To write, use the add* helper functions.
 */
class EoDxfHeader {
 public:
  EoDxfHeader() { m_version = EoDxf::Version::AC1021; }

  EoDxfHeader(const EoDxfHeader& other);

  EoDxfHeader& operator=(const EoDxfHeader& other);

  ~EoDxfHeader() = default;

  void AddDouble(std::wstring_view key, double value, int code);
  void AddInt16(std::wstring_view key, std::int16_t value, int code);
  void AddInt32(std::wstring_view key, std::int32_t value, int code);
  void AddWideString(std::wstring_view key, std::wstring value, int code);
  void AddGeometryBase(std::wstring_view key, EoDxfGeometryBase3d value, int code);
  void AddHandle(std::wstring_view key, std::uint64_t value, int code);

  [[nodiscard]] std::wstring GetComments() const { return m_comments; }
  [[nodiscard]] const std::wstring& GetCodePageToken() const noexcept { return m_originalCodePageToken; }

  /** @brief Writes the header variables stored in this EoDxfHeader instance to the provided dxfWriter object in the
   * format specified by the given DXF version. This method iterates over the header variables stored in the m_variants
   * map and writes them to the dxfWriter using the appropriate group codes and value types based on the DXF version. It
   * handles writing of standard header variables such as $ACADVER, $DWGCODEPAGE, and others, ensuring that the output
   * conforms to the specifications of the target DXF version.
   *
   * @param writer Pointer to a dxfWriter object used to write the header variables to the output DXF file.
   * @param version The version of the DXF format to use for writing (e.g., AC1009, AC1015, etc.).
   */
  void Write(EoDxfWriter* writer, EoDxf::Version version);
  void AddComment(std::wstring_view comment);

 protected:
  /** @brief Parses a header variable from the given EoDxfReader object based on the provided group code.
   *  This method reads the value corresponding to the code from the reader and stores it in the m_variants map
   *  using the current variable name (m_name) as the key. The type of value read (string, double, int, or geometryBase)
   *  is determined by the code and stored in a EoDxfGroupCodeValuesVariant object.
   *
   *  @param code The group code indicating the type of header variable being parsed.
   *  @param reader Pointer to EoDxfReader object to read values from.
   */
  void ParseCode(int code, EoDxfReader* reader);

 private:
  void WriteBase(EoDxfWriter* writer);
  void WriteStoredWideString(EoDxfWriter* writer, std::wstring_view key, int code, std::wstring_view defaultValue);

  /** @brief Writes header variables that were added in AC1009 (R11/R12) but not present in AC1006 (R10).
   * Maximum legacy compatibility (still the safest target for 30-year-old viewers.
   * @param writer The EoDxfWriter to write the header variables to.
   */ 
  void WriteAC1009Additions(EoDxfWriter* writer);

  /** @brief Writes header variables that were added in AC1012 (R13)
   * Writes AC1012 additions to the header section.
   * @param writer The EoDxfWriter to write the header variables to.
   */
  void WriteAC1012Additions(EoDxfWriter* writer);

  /** @brief Writes header variables that were added in AC1014 (R14)
   * Big internal redesign after R13; the first truly stable “modern” DWG
   * @param writer The EoDxfWriter to write the header variables to.
   */
  void WriteAC1014Additions(EoDxfWriter* writer);

  /** @brief Writes header variables that were added in AC1015 (2000)
   * The gold-standard safe default — almost every CAD program on the planet opens it perfectly
   * @param writer The EoDxfWriter to write the header variables to.
   */
  void WriteAC1015Additions(EoDxfWriter* writer);

  /** @brief Writes header variables that were added in AC1018 (2004)
   * Handle/security improvements; still the most common corporate baseline
   * @param writer The EoDxfWriter to write the header variables to.
   * @param version The DXF version being written, used to determine how to write string variables.
   */
  void WriteAC1018Additions(EoDxfWriter* writer, EoDxf::Version version);
  void WriteAC1021Additions(EoDxfWriter* writer);
  
  /** @brief Writes header variables that were added in AC1024 (2010)
   * The latest version with any new header variables; the current “modern” standard
   * @param writer The EoDxfWriter to write the header variables to.
   */
  void WriteAC1024Additions(EoDxfWriter* writer);

  [[nodiscard]] bool GetBool(std::wstring_view key, bool* variantBool);
  [[nodiscard]] bool GetDouble(std::wstring_view key, double* varDouble);
  [[nodiscard]] bool GetInt16(std::wstring_view key, std::int16_t* varInt);
  [[nodiscard]] bool GetInt32(std::wstring_view key, std::int32_t* varInt);

  /** @brief Retrieves a String variant from the map and removes it if found. If the variant's string pointer is null,
   * the output string is set to empty.
   * @param key The key to look up in the map.
   * @param variantWideString Pointer to the output wide string.
   * @return True if the variant was found and removed, false otherwise.
   */
  [[nodiscard]] bool GetWideString(std::wstring_view key, std::wstring* variantWideString);
  [[nodiscard]] bool GetGeometryBase(std::wstring_view key, EoDxfGeometryBase3d* variantGeometryBase);
  [[nodiscard]] bool GetHandle(std::wstring_view key, std::uint64_t* varHandle);

  /** @brief Clears all header variables stored in the m_variants map and resets the state of the
   * EoDxfHeader instance. unique_ptr elements are automatically destroyed when erased.
   */
  void ClearVariants();

 public:
  std::map<std::wstring, std::unique_ptr<EoDxfGroupCodeValuesVariant>> m_variants;

 private:
  std::wstring m_comments{};
  std::wstring m_originalCodePageToken{};
  std::wstring m_name{};
  EoDxfGroupCodeValuesVariant* m_currentVariant{};
  EoDxf::Version m_version{};

  friend class EoDxfRead;
  friend class EoDxfWrite;
};
