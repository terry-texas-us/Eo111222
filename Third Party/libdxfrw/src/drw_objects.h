#pragma once

#include <map>
#include <string>
#include <vector>

class dxfReader;
class dxfWriter;

/** @brief Base class for DXF OBJECTS section entries.
 *
 *  Handles common group codes shared by all DXF object types in the OBJECTS section,
 *  analogous to DRW_TableEntry for TABLE section entries.
 *
 *  Common group codes handled:
 *  - 5: Handle
 *  - 102: Application-defined groups (ACAD_REACTORS, ACAD_XDICTIONARY)
 *  - 330: Soft-pointer ID/handle to owner dictionary (or reactor handle within ACAD_REACTORS)
 *  - 360: Hard-owner ID/handle (extension dictionary handle within ACAD_XDICTIONARY)
 *  - 1000-1071: Extended data
 */
class DRW_ObjectEntry {
 public:
  DRW_ObjectEntry() = default;

  virtual ~DRW_ObjectEntry() {
    for (auto* variant : m_extensionData) { delete variant; }
    m_extensionData.clear();
  }

  DRW_ObjectEntry(const DRW_ObjectEntry& e) {
    m_handle = e.m_handle;
    m_ownerHandle = e.m_ownerHandle;
    m_extensionDictionaryHandle = e.m_extensionDictionaryHandle;
    m_reactorHandles = e.m_reactorHandles;
    for (auto* variant : e.m_extensionData) { m_extensionData.push_back(new EoDxfGroupCodeValuesVariant(*variant)); }
  }

 protected:
  void ParseCode(int code, dxfReader* reader);
  void Reset();

 public:
  std::uint32_t m_handle{};  // Group code 5
  std::uint32_t m_ownerHandle{};  // Group code 330 (soft-pointer to owner dictionary)
  std::uint32_t m_extensionDictionaryHandle{};  // Group code 360 (hard-owner, extension dictionary)
  std::vector<std::uint32_t> m_reactorHandles;  // Group code 330 handles within ACAD_REACTORS group
  std::vector<EoDxfGroupCodeValuesVariant*> m_extensionData;  // Group codes 1000 to 1071

 private:
  EoDxfGroupCodeValuesVariant* m_currentVariant{};
  bool m_inReactors{};
  bool m_inXDictionary{};
};

// ACDBPLACEHOLDER, DICTIONARYVAR, FIELD, GROUP, IDBUFFER, IMAGEDEF_REACTOR, LAYER_FILTER, LAYER_INDEX, LAYOUT,
// MATERIAL, MLINESTYLE, PLOTSETTINGS, RASTERVARIABLES, SORTENTSTABLE, SPATIAL_FILTER, SPATIAL_INDEX,
// TABLESTYLE, VBA_PROJECT, VISUALSTYLE, XRECORD

/** @brief Class to handle image definition entries
 *
 *  An image definition object entry represents an image definition in a drawing, which defines the properties of an
 *  image that can be inserted into the drawing. It is defined by its name (code 1) and various properties such as the
 *  image size in pixels (code 10 and 20), default size of one pixel (code 11 and 12), loaded flag (code 280), and
 *  resolution units (code 281). The image definition can also include properties such as the group class version
 *  (code 90) and a map of reactors, which are objects that react to changes in the image definition.
 */
class DRW_ImageDef : public DRW_ObjectEntry {
  friend class dxfRW;

 public:
  DRW_ImageDef() { Reset(); }

 protected:
  void ParseCode(int code, dxfReader* reader);

  void Reset();

 public:
  UTF8STRING m_fileNameOfImage;  // Group code 1
  int imgVersion;  // Group code 90, 0=R14 version
  double m_uImageSizeInPixels{};  // Group code 10
  double m_vImageSizeInPixels{};  // Group code 20
  double m_uSizeOfOnePixel{};  // Group code 11
  double m_vSizeOfOnePixel{};  // Group code 21
  int m_imageIsLoadedFlag{};  // Group code 280, 0=unloaded, 1=loaded
  int m_resolutionUnits{};  // Group code 281, 0=no, 2=centimeters, 5=inch

  std::map<std::string, std::string> reactors;
};
