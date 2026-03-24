#pragma once

#include <array>
#include <cstdint>

/** @brief Class to handle conversion between DXF line weight codes and internal line weight enumeration. */
class EoDxfLineWeights {
 public:
  enum class LineWeight : std::int8_t {
    kLnWt000,  // 0   0.00 mm
    kLnWt005,  // 1   0.05 mm
    kLnWt009,  // 2   0.09 mm
    kLnWt013,  // 3   0.13 mm
    kLnWt015,  // 4   0.15 mm
    kLnWt018,  // 5   0.18 mm
    kLnWt020,  // 6   0.20 mm
    kLnWt025,  // 7   0.25 mm
    kLnWt030,  // 8   0.30 mm
    kLnWt035,  // 9   0.35 mm
    kLnWt040,  // 10  0.40 mm
    kLnWt050,  // 11  0.50 mm
    kLnWt053,  // 12  0.53 mm
    kLnWt060,  // 13  0.60 mm
    kLnWt070,  // 14  0.70 mm
    kLnWt080,  // 15  0.80 mm
    kLnWt090,  // 16  0.90 mm
    kLnWt100,  // 17  1.00 mm
    kLnWt106,  // 18  1.06 mm
    kLnWt120,  // 19  1.20 mm
    kLnWt140,  // 20  1.40 mm
    kLnWt158,  // 21  1.58 mm
    kLnWt200,  // 22  2.00 mm
    kLnWt211,  // 23  2.11 mm
    kLnWtByLayer = 29,  // by layer (DXF -1)
    kLnWtByBlock = 30,  // by block (DXF -2)
    kLnWtByLwDefault = 31  // by default (DXF -3)
  };

 public:
  /** @brief Converts internal `LineWeight` enum to the exact DXF line-weight code.
   *  @param lineWeightEnum The internal `LineWeight` enum value to convert.
   *  @return The corresponding DXF line-weight code.
   */
  static constexpr std::int16_t LineWeightToDxfIndex(LineWeight lineWeightEnum) noexcept {
    switch (lineWeightEnum) {
      case LineWeight::kLnWtByLayer:
        return -1;
      case LineWeight::kLnWtByBlock:
        return -2;
      case LineWeight::kLnWtByLwDefault:
        return -3;
      default: {
        const auto index = static_cast<std::size_t>(lineWeightEnum);
        if (index < kDxfValues.size()) { return kDxfValues[index]; }
        break;
      }
    }
    return -3;  // fallback to LnWtByLwDefault for any invalid enum value (should never happen)
  }

  /** @brief Converts DXF line-weight code to internal `LineWeight` enum.
   *
   *  Negative values map exactly to the special “by...” cases.
   *  Positive values must match one of the 24 exact valid codes defined by the DXF spec.
   *  Any other value (including malformed positive integers) returns `kLnWtByLwDefault`.
   *  @param lineWeightCode The DXF line-weight code to convert.
   *  @return The corresponding internal `LineWeight` enum value.
   */
  static constexpr LineWeight DxfIndexToLineWeight(std::int16_t lineWeightCode) noexcept {
    if (lineWeightCode < 0) {
      if (lineWeightCode == -1) { return LineWeight::kLnWtByLayer; }
      if (lineWeightCode == -2) { return LineWeight::kLnWtByBlock; }
      if (lineWeightCode == -3) { return LineWeight::kLnWtByLwDefault; }
      return LineWeight::kLnWtByLwDefault;  // any other negative
    }

    // Exact match only
    for (std::size_t index = 0; index < kDxfValues.size(); ++index) {
      if (lineWeightCode == kDxfValues[index]) { return static_cast<LineWeight>(index); }
    }
    return LineWeight::kLnWtByLwDefault;  // invalid positive value
  }

 private:
  // The ONLY 24 valid positive DXF lineweight codes (in 0.01 mm units). Order exactly matches enum values 0..23.
  static constexpr std::array<std::int16_t, 24> kDxfValues = {
      0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40, 50, 53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211};
};