#pragma once

#include <map>
#include <string>

#include "drw_base.h"

class dxfReader;
class dxfWriter;

/** Class to handle header vars, to read iterate over "std::map vars"
 *  to write add a DRW_Variant* into "std::map vars" (do not delete it, they are cleared in dtor)
 *  or use add* helper functions.
 */
class DRW_Header {
 public:
  DRW_Header() { m_version = DRW::Version::AC1021; }

  DRW_Header(const DRW_Header& other);

  DRW_Header& operator=(const DRW_Header& other);

  ~DRW_Header() { ClearVariants(); }

  void AddDouble(const std::string& key, double value, int code);
  void AddInteger(const std::string& key, int value, int code);
  void AddString(const std::string& key, std::string value, int code);
  void AddCoord(const std::string& key, DRW_Coord value, int code);

  [[nodiscard]] std::string GetComments() const { return m_comments; }

  /** @brief Writes the header variables stored in this DRW_Header instance to the provided dxfWriter object in the format specified by the given DXF version.
   * This method iterates over the header variables stored in the m_variants map and writes them to the dxfWriter using the appropriate group codes and value types based on the DXF version. It handles writing of standard header variables such as $ACADVER, $DWGCODEPAGE, and others, ensuring that the output conforms to the specifications of the target DXF version.
   *
   * @param writer Pointer to a dxfWriter object used to write the header variables to the output DXF file.
   * @param version The version of the DXF format to use for writing (e.g., AC1009, AC1015, etc.).
   */
  void Write(dxfWriter* writer, DRW::Version version);
  void AddComment(std::string c);

 protected:
  /** @brief Parses a header variable from the given dxfReader object based on the provided group code.
   *  This method reads the value corresponding to the code from the reader and stores it in the m_variants map
   *  using the current variable name (m_name) as the key. The type of value read (string, double, int, or coord)
   *  is determined by the code and stored in a DRW_Variant object.
   *
   *  @param code The group code indicating the type of header variable being parsed.
   *  @param reader Pointer to dxfReader object to read values from.
   */
  void ParseCode(int code, dxfReader* reader);

 private:
  void WriteBase(dxfWriter* writer, DRW::Version version);

  /** @brief Writes header variables that were added in AC1009 (R11/R12) but not present in AC1006 (R10).
   * Maximum legacy compatibility (still the safest target for 30-year-old viewers.
   * @param writer The dxfWriter to write the header variables to.
   * @param version The DXF version being written, used to determine how to write string variables.
   */
  void WriteAC1009Additions(dxfWriter* writer, DRW::Version version);

  /** @brief Writes header variables that were added in AC1012 (R13)
   * Writes AC1012 additions to the header section.
   * @param writer The dxfWriter to write the header variables to.
   * @param version The DXF version being written, used to determine how to write string variables.
   */
  void WriteAC1012Additions(dxfWriter* writer, DRW::Version version);

  /** @brief Writes header variables that were added in AC1014 (R14)
   * Big internal redesign after R13; the first truly stable “modern” DWG
   * @param writer The dxfWriter to write the header variables to.
   * @param version The DXF version being written, used to determine how to write string variables.
   */
  void WriteAC1014Additions(dxfWriter* writer, DRW::Version version);

  /** @brief Writes header variables that were added in AC1015 (2000)
   * The gold-standard safe default — almost every CAD program on the planet opens it perfectly
   * @param writer The dxfWriter to write the header variables to.
   * @param version The DXF version being written, used to determine how to write string variables.
   */
  void WriteAC1015Additions(dxfWriter* writer, DRW::Version version) noexcept;

  /** @brief Writes header variables that were added in AC1018 (2004)
   * Handle/security improvements; still the most common corporate baseline
   * @param writer The dxfWriter to write the header variables to.
   * @param version The DXF version being written, used to determine how to write string variables.
   */
  void WriteAC1018Additions(dxfWriter* writer, DRW::Version version) noexcept;
  void WriteAC1021Additions(dxfWriter* writer, DRW::Version version) noexcept;

  /** @brief Writes header variables that were added in AC1018 (2004)
   * Current long-term format (no data loss, smallest modern files)
   * @param writer The dxfWriter to write the header variables to.
   * @param version The DXF version being written, used to determine how to write string variables.
   */
  void WriteAC1024Additions(dxfWriter* writer, DRW::Version version) noexcept;

  [[nodiscard]] bool GetDouble(const std::string& key, double* varDouble);
  [[nodiscard]] bool GetInteger(const std::string& key, int* varInt);

  /** @brief Retrieves a String variant from the map and removes it if found. If the variant's string pointer is null, the output string is set to empty.
   * @param key The key to look up in the map.
   * @param variantString Pointer to the output string.
   * @return True if the variant was found and removed, false otherwise.
   */
  [[nodiscard]] bool GetString(const std::string& key, std::string* variantString);
  [[nodiscard]] bool GetCoord(const std::string& key, DRW_Coord* varStr);

  /** @brief Clears all header variables stored in the m_variants map, deleting the associated DRW_Variant objects and emptying the map.
   *  This method is used to free memory allocated for header variables and reset the state of the DRW_Header instance.
   */
  void ClearVariants();

 public:
  std::map<std::string, DRW_Variant*> m_variants;

 private:
  std::string m_comments{};
  std::string m_name{};
  DRW_Variant* m_currentVariant{};
  int m_version{};

  friend class dxfRW;
};
