#pragma once

/// @file EoPowerGeometry.h
/// Pure geometry helpers for Power mode — no view or document dependencies.

#include <cstdint>

class EoDbGroup;
class EoGePoint3d;

/// Configuration parameters for Power mode. Persists across mode switches; owned
/// by AeSysView as a single member and read/written via EoDlgPowerOptions (future).
struct PowerConfig {
  double conductorSpacing{0.04};
};

namespace Power {

/// Populates group with the home-run arrow geometry pointing from pointOnCircuit toward endPoint.
/// pointOnCircuit is adjusted to the projected start offset.
void FillHomeRunArrow(EoDbGroup* group, EoGePoint3d& pointOnCircuit, const EoGePoint3d& endPoint);

/// Populates group with the conductor symbol geometry for the given conductorType (ID_OP4–ID_OP7).
/// Returns false if conductorType is unrecognized (group is left unmodified).
bool FillConductorSymbol(int conductorType, EoDbGroup* group,
    const EoGePoint3d& pointOnCircuit,
    const EoGePoint3d& endPoint);

}  // namespace Power
