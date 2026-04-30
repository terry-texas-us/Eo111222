#pragma once

/// @file EoPipeGeometry.h
/// Pure geometry helpers for Pipe mode — no view or document dependencies.

class EoDbGroup;
class EoDbLine;
class EoGePoint3d;

/// Configuration parameters for Pipe mode. Persists across mode switches; owned
/// by AeSysView as a single member and read/written via EoDlgPipeOptions.
struct PipeConfig {
  double ticSize{0.03125};
  double riseDropRadius{0.03125};
  int currentSymbolIndex{0};
};

namespace Pipe {

/// Generates a tick-mark perpendicular cross at a point along [begin, end] at
/// the given distance from begin. Returns false if the projection falls beyond end.
bool GenerateTickMark(const EoGePoint3d& begin,
    const EoGePoint3d& end,
    double distance,
    double ticSize,
    EoDbGroup* group);

/// Adds a line segment between begin and end, with optional tick-mark fittings at
/// either end. beginType / endType are ID_OP2–ID_OP9 fitting codes.
void GenerateLineWithFittings(int beginType,
    const EoGePoint3d& begin,
    int endType,
    const EoGePoint3d& end,
    double riseDropRadius,
    double ticSize,
    EoDbGroup* group);

}  // namespace Pipe
