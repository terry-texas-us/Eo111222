#pragma once

#include <map>
#include <string>
#include <vector>

#include "EoDxfEntity.h"
#include "EoDxfGroupCodeValuesVariant.h"
#include "EoDxfReader.h"

class EoDxfWriter;

/** @brief Base class for DXF OBJECTS section entries.
 *
 *  Derives from EoDxfEntity to share the common database-identity fields (handle, owner handle,
 *  reactor handles, extension dictionary handle, extended data, and application-defined groups)
 *  with graphical entity types. This unifies all handle-bearing DXF objects under a single
 *  hierarchy, following the same pattern as ezdxf's DXFEntity base.
 *
 *  Common group codes inherited from EoDxfEntity:
 *  - 5: Handle
 *  - 102: Application-defined groups (ACAD_REACTORS, ACAD_XDICTIONARY, and others)
 *  - 330: Soft-pointer ID/handle to owner dictionary (or reactor handle within ACAD_REACTORS)
 *  - 360: Hard-owner ID/handle (extension dictionary handle within ACAD_XDICTIONARY)
 *  - 1000-1071: Extended data
 */
class EoDxfObjectEntry : public EoDxfEntity {
 public:
  EoDxfObjectEntry() = default;
  ~EoDxfObjectEntry() override = default;

  EoDxfObjectEntry(const EoDxfObjectEntry&) = default;
  EoDxfObjectEntry& operator=(const EoDxfObjectEntry&) = default;
  EoDxfObjectEntry(EoDxfObjectEntry&&) noexcept = default;
  EoDxfObjectEntry& operator=(EoDxfObjectEntry&&) noexcept = default;

 protected:
  /// Delegates to EoDxfEntity::ParseCode for all common group codes.
  void ParseCode(int code, EoDxfReader& reader);
  void Reset();
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
class EoDxfImageDefinition : public EoDxfObjectEntry {
  friend class EoDxfRead;
  friend class EoDxfWrite;

 public:
  EoDxfImageDefinition() { Reset(); }

 protected:
  void ParseCode(int code, EoDxfReader& reader);

  void Reset();

 public:
  std::wstring m_fileNameOfImage;  // Group code 1
  std::int32_t m_classVersion;  // Group code 90, 0=R14 version
  double m_uImageSizeInPixels{};  // Group code 10
  double m_vImageSizeInPixels{};  // Group code 20
  double m_uSizeOfOnePixel{};  // Group code 11
  double m_vSizeOfOnePixel{};  // Group code 21
  std::int16_t m_imageIsLoadedFlag{};  // Group code 280, 0=unloaded, 1=loaded
  std::int16_t m_resolutionUnits{};  // Group code 281, 0=no, 2=centimeters, 5=inch

  std::map<std::wstring, std::wstring> reactors;
};

class EoDxfUnsupportedObject {
 public:
  void Write(EoDxfWriter* writer) const;

  std::wstring m_objectType;
  std::vector<EoDxfGroupCodeValuesVariant> m_values;
};
