#pragma once

#include <string>

#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"
#include "EoDxfLineWeights.h"

class AeSysView;
class EoGsRenderDevice;

class EoGsRenderState {
  EoDbFontDefinition m_fontDefinition{};
  EoDbCharacterCellDefinition m_characterCellDefinition{};
  std::int16_t m_pointStyle{};
  std::int16_t m_color{7};
  std::int16_t m_LineTypeIndex{Eo::continuousLineTypeIndex};
  std::wstring m_lineTypeName{};
  EoDb::PolygonStyle m_PolygonStyle{EoDb::PolygonStyle::Hollow};
  std::int16_t m_PolygonInteriorStyleIndex{};
  double m_lineTypeScale{1.0};
  EoDxfLineWeights::LineWeight m_lineWeight{EoDxfLineWeights::LineWeight::kLnWtByLwDefault};
  std::wstring m_textStyleName{L"Standard"};

 public:
  EoGsRenderState() = default;
  EoGsRenderState(const EoGsRenderState&) = default;
  EoGsRenderState(EoGsRenderState&&) noexcept = default;

  EoGsRenderState& operator=(const EoGsRenderState&) = default;
  EoGsRenderState& operator=(EoGsRenderState&&) noexcept = default;

  ~EoGsRenderState() = default;

 public:
  void Restore(CDC* deviceContext, int saveIndex);
  void Restore(EoGsRenderDevice* renderDevice, int saveIndex);
  int Save();

  /** @brief This function is responsible for managing the resources associated with pen definitions,
   * including creating, deleting, and reusing pen objects as needed.
   *
   * @param deviceContext The device context to manage pen resources for.
   * @param color The color of the pen to create or manage.
   * @param width The width of the pen to create or manage.
   * @param lineType The line type of the pen to create or manage.
   */
  void ManagePenResources(CDC* deviceContext, std::int16_t color, int width, std::int16_t lineType);

  /// @brief Backend-agnostic overload — routes through EoGsRenderDevice::SelectPen.
  void ManagePenResources(EoGsRenderDevice* renderDevice, std::int16_t color, int width, std::int16_t lineType);

  void SetPen(AeSysView* view, CDC* deviceContext, std::int16_t penColor, std::int16_t lineType);
  
  void SetPen(AeSysView* view,
      CDC* deviceContext,
      std::int16_t penColor,
      std::int16_t lineType,
      const std::wstring& lineTypeName);
  
  void SetPen(const AeSysView* view,
      CDC* deviceContext,
      std::int16_t penColor,
      std::int16_t lineType,
      const std::wstring& lineTypeName,
      EoDxfLineWeights::LineWeight lineWeight,
      double lineTypeScale);

  void SetPen(AeSysView* view, EoGsRenderDevice* renderDevice, std::int16_t penColor, std::int16_t lineType);
  
  void SetPen(AeSysView* view,
      EoGsRenderDevice* renderDevice,
      std::int16_t penColor,
      std::int16_t lineType,
      const std::wstring& lineTypeName);
  
  void SetPen(const AeSysView* view,
      EoGsRenderDevice* renderDevice,
      std::int16_t penColor,
      std::int16_t lineType,
      const std::wstring& lineTypeName,
      EoDxfLineWeights::LineWeight lineWeight,
      double lineTypeScale);

  void SetColor(CDC* deviceContext, std::int16_t color);
  void SetColor(EoGsRenderDevice* renderDevice, std::int16_t color);
  [[nodiscard]] std::int16_t Color() const noexcept { return m_color; }

  void SetLineType(CDC* deviceContext, std::int16_t lineType);
  void SetLineType(EoGsRenderDevice* renderDevice, std::int16_t lineType);
  [[nodiscard]] std::int16_t LineTypeIndex() const noexcept { return m_LineTypeIndex; }

  void SetLineTypeName(std::wstring name) noexcept { m_lineTypeName = std::move(name); }
  [[nodiscard]] const std::wstring& LineTypeName() const noexcept { return m_lineTypeName; }

  [[nodiscard]] double LineTypeScale() const noexcept { return m_lineTypeScale; }

  void SetLineWeight(EoDxfLineWeights::LineWeight lineWeight) noexcept { m_lineWeight = lineWeight; }
  [[nodiscard]] EoDxfLineWeights::LineWeight LineWeight() const noexcept { return m_lineWeight; }

  void SetTextStyleName(std::wstring name) noexcept { m_textStyleName = std::move(name); }
  [[nodiscard]] const std::wstring& TextStyleName() const noexcept { return m_textStyleName; }

  void SetCharacterCellDefinition(const EoDbCharacterCellDefinition& characterCellDefinition) noexcept {
    m_characterCellDefinition = characterCellDefinition;
  }

  void SetFontDefinition(CDC* deviceContext, const EoDbFontDefinition& fontDefinition);
  void SetFontDefinition(EoGsRenderDevice* renderDevice, const EoDbFontDefinition& fontDefinition);
  [[nodiscard]] EoDbCharacterCellDefinition CharacterCellDefinition() const noexcept {
    return m_characterCellDefinition;
  }
  [[nodiscard]] const EoDbFontDefinition& FontDefinition() const noexcept { return m_fontDefinition; }

  void SetPointStyle(std::int16_t pointStyle) noexcept { m_pointStyle = pointStyle; }
  const std::int16_t& PointStyle() noexcept { return m_pointStyle; }

  void SetPolygonIntStyle(EoDb::PolygonStyle interiorStyle) noexcept { m_PolygonStyle = interiorStyle; }
  void SetPolygonIntStyleId(std::int16_t styleIndex) noexcept { m_PolygonInteriorStyleIndex = styleIndex; }
  [[nodiscard]] const EoDb::PolygonStyle& PolygonIntStyle() const noexcept { return m_PolygonStyle; }
  [[nodiscard]] std::int16_t PolygonIntStyleId() const noexcept { return m_PolygonInteriorStyleIndex; }

  /** @brief Sets the current foreground mix mode. GDI uses the foreground mix mode to combine pens and
   * interiors of filled objects with the colors already on the screen. The foreground mix mode
   * defines how colors from the brush or pen and the colors in the existing image are to be combined.
   * @param deviceContext The device context to set the mix mode on.
   * @param drawMode The drawing mode to set.
   * @return The previous drawing mode.
   * @note This function is typically used to change the drawing mode for a specific device context.
   */
  int SetROP2(CDC* deviceContext, int drawMode);
  int SetROP2(EoGsRenderDevice* renderDevice, int drawMode);

  void SetAlignment(CDC* deviceContext,
      EoDb::HorizontalAlignment horizontalAlignment,
      EoDb::VerticalAlignment verticalAlignment);
  void SetAlignment(EoGsRenderDevice* renderDevice,
      EoDb::HorizontalAlignment horizontalAlignment,
      EoDb::VerticalAlignment verticalAlignment);
};

/// @brief Graphics State — application-wide singletons for the Gs rendering layer.
namespace Gs {
/// @brief The application-wide render state singleton (color, line type, line weight, font).
extern EoGsRenderState renderState;
}  // namespace Gs
