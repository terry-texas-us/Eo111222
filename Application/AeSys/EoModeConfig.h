#pragma once

/// @file EoModeConfig.h
/// Plain-data configuration structs for Fixup and Edit modes.
/// Owned by AeSysView as single members; persisted across mode switches and
/// read/written by their respective options dialogs.

#include "EoGeVector3d.h"

/// Configuration for Fixup mode (fillet radius, chamfer/corner size, axis snap tolerance).
struct FixupConfig {
  double axisTolerance{2.0};
  double cornerSize{0.25};
};

/// Configuration for Edit mode (rotation, scale, mirror transform parameters).
struct EditConfig {
  EoGeVector3d mirrorScale{-1.0, 1.0, 1.0};
  EoGeVector3d rotationAngles{0.0, 0.0, 45.0};
  EoGeVector3d scale{2.0, 2.0, 2.0};
};
