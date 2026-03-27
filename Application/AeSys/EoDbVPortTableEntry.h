#pragma once

#include <cstdint>
#include <string>

#include "EoDb.h"
#include "EoGePoint3d.h"
#include "EoGeVector3d.h"

/** @brief Represents a single VPORT table entry for the model-space active viewport.
 *
 * Stores the DXF VPORT table fields needed to preserve the active viewport configuration
 * across PEG V2 (AE2026) save/load and DXF round-trip. The field names and semantics match
 * the DXF VPORT table entry (AcDbViewportTableRecord) group codes.
 *
 * On-disk PEG V2 layout (inside kViewPortTable):
 *   handle (uint64)
 *   ownerHandle (uint64)
 *   name (tab-terminated string)
 *   lowerLeftCorner (Point3d — z ignored, stored as 0)
 *   upperRightCorner (Point3d — z ignored, stored as 0)
 *   viewCenter (Point3d — z ignored, stored as 0)
 *   snapBasePoint (Point3d — z ignored, stored as 0)
 *   snapSpacing (Point3d — z ignored, stored as 0)
 *   gridSpacing (Point3d — z ignored, stored as 0)
 *   viewDirection (Vector3d)
 *   viewTargetPoint (Vector3d)
 *   viewHeight (double)
 *   viewAspectRatio (double)
 *   lensLength (double)
 *   frontClipPlane (double)
 *   backClipPlane (double)
 *   snapRotationAngle (double)
 *   viewTwistAngle (double)
 *   viewMode (int16)
 *   circleZoomPercent (int16)
 *   fastZoom (int16)
 *   ucsIcon (int16)
 *   snapOn (int16)
 *   gridOn (int16)
 *   snapStyle (int16)
 *   snapIsopair (int16)
 *   gridBehavior (int16)
 */
class EoDbVPortTableEntry {
 public:
  EoDbVPortTableEntry() = default;

  /// @brief DXF entity handle (group code 5). Zero = no handle assigned.
  std::uint64_t m_handle{};

  /// @brief DXF owner handle (group code 330). Zero = no owner assigned.
  std::uint64_t m_ownerHandle{};

  /// @brief Viewport name — typically "*ACTIVE" for the active model-space viewport.
  std::wstring m_name{L"*ACTIVE"};

  /// @brief Lower-left corner of viewport (DXF group 10/20).
  EoGePoint3d m_lowerLeftCorner{};

  /// @brief Upper-right corner of viewport (DXF group 11/21).
  EoGePoint3d m_upperRightCorner{1.0, 1.0, 0.0};

  /// @brief View center point in DCS (DXF group 12/22).
  EoGePoint3d m_viewCenter{};

  /// @brief Snap base point in DCS (DXF group 13/23).
  EoGePoint3d m_snapBasePoint{};

  /// @brief Snap spacing X and Y (DXF group 14/24).
  EoGePoint3d m_snapSpacing{10.0, 10.0, 0.0};

  /// @brief Grid spacing X and Y (DXF group 15/25).
  EoGePoint3d m_gridSpacing{10.0, 10.0, 0.0};

  /// @brief View direction from target point, WCS (DXF group 16/26/36).
  EoGeVector3d m_viewDirection{0.0, 0.0, 1.0};

  /// @brief View target point, WCS (DXF group 17/27/37).
  EoGeVector3d m_viewTargetPoint{};

  /// @brief View height (DXF group 40).
  double m_viewHeight{10.0};

  /// @brief Viewport aspect ratio (DXF group 41).
  double m_viewAspectRatio{1.0};

  /// @brief Lens length for perspective views (DXF group 42).
  double m_lensLength{50.0};

  /// @brief Front clipping plane offset (DXF group 43).
  double m_frontClipPlane{};

  /// @brief Back clipping plane offset (DXF group 44).
  double m_backClipPlane{};

  /// @brief Snap rotation angle in radians (DXF group 50, stored as degrees in DXF).
  double m_snapRotationAngle{};

  /// @brief View twist angle in radians (DXF group 51, stored as degrees in DXF).
  double m_viewTwistAngle{};

  /// @brief VIEWMODE system variable (DXF group 71).
  std::int16_t m_viewMode{};

  /// @brief Circle zoom percent (DXF group 72).
  std::int16_t m_circleZoomPercent{100};

  /// @brief Fast zoom setting (DXF group 73).
  std::int16_t m_fastZoom{1};

  /// @brief UCSICON setting (DXF group 74).
  std::int16_t m_ucsIcon{3};

  /// @brief Snap on/off (DXF group 75).
  std::int16_t m_snapOn{};

  /// @brief Grid on/off (DXF group 76).
  std::int16_t m_gridOn{};

  /// @brief Snap style (DXF group 77).
  std::int16_t m_snapStyle{};

  /// @brief Snap isopair (DXF group 78).
  std::int16_t m_snapIsopair{};

  /// @brief Grid behavior flags (DXF group 60).
  std::int16_t m_gridBehavior{7};

  /// @brief Reads the entry from a PEG file.
  void Read(CFile& file);

  /// @brief Writes the entry to a PEG file.
  void Write(CFile& file) const;
};
