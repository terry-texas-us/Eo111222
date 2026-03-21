#pragma once

namespace hatch {

constexpr int maxPatterns = 128;
constexpr int maxTableValues = 4096;

extern double dOffAng;
extern double dXAxRefVecScal;
extern double dYAxRefVecScal;

extern int tableOffset[maxPatterns];
extern float tableValue[maxTableValues];

}  // namespace hatch
