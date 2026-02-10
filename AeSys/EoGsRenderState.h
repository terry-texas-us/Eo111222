#pragma once

#include "Eo.h"
#include "EoDb.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"

class AeSysView;

class EoGsRenderState {
  EoDbFontDefinition m_fontDefinition{};
  EoDbCharacterCellDefinition m_characterCellDefinition{};
  std::int16_t m_pointStyle{};
  std::int16_t m_color{7};
  std::int16_t m_LineTypeIndex{Eo::continuousLineTypeIndex};
  EoDb::PolygonStyle m_PolygonStyle{EoDb::PolygonStyle::Hollow};
  std::int16_t m_PolygonInteriorStyleIndex{};

 public:
  EoGsRenderState() = default;
  EoGsRenderState(const EoGsRenderState&) = default;
  EoGsRenderState(EoGsRenderState&&) noexcept = default;

  EoGsRenderState& operator=(const EoGsRenderState&) = default;
  EoGsRenderState& operator=(EoGsRenderState&&) noexcept = default;

  ~EoGsRenderState() = default;

 public:
  void Restore(CDC* deviceContext, int saveIndex);
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
  
  void SetPen(AeSysView* view, CDC* deviceContext, std::int16_t penColor, std::int16_t lineType);
  
  void SetColor(CDC* deviceContext, std::int16_t color);
  [[nodiscard]] std::int16_t Color() const { return m_color; }
  
  void SetLineType(CDC* deviceContext, std::int16_t lineType);
  [[nodiscard]] std::int16_t LineTypeIndex() const { return m_LineTypeIndex; }

  void SetCharacterCellDefinition(EoDbCharacterCellDefinition& characterCellDefinition) {
    m_characterCellDefinition = characterCellDefinition;
  }
  void SetFontDefinition(CDC* deviceContext, const EoDbFontDefinition& fontDefinition);
  [[nodiscard]] EoDbCharacterCellDefinition CharacterCellDefinition() const noexcept {
    return m_characterCellDefinition;
  }
  [[nodiscard]] const EoDbFontDefinition& FontDefinition() const noexcept { return m_fontDefinition; }
  
  
  void SetPointStyle(std::int16_t pointStyle) { m_pointStyle = pointStyle; }
  const std::int16_t& PointStyle() { return m_pointStyle; }
  
  void SetPolygonIntStyle(EoDb::PolygonStyle interiorStyle) { m_PolygonStyle = interiorStyle; }
  void SetPolygonIntStyleId(std::int16_t styleIndex) { m_PolygonInteriorStyleIndex = styleIndex; }
  [[nodiscard]] const EoDb::PolygonStyle& PolygonIntStyle() const { return m_PolygonStyle; }
  [[nodiscard]] std::int16_t PolygonIntStyleId() const { return m_PolygonInteriorStyleIndex; }
  
  /** @brief Sets the current foreground mix mode. GDI uses the foreground mix mode to combine pens and
   * interiors of filled objects with the colors already on the screen. The foreground mix mode
   * defines how colors from the brush or pen and the colors in the existing image are to be combined.
   * @param deviceContext The device context to set the mix mode on.
   * @param drawMode The drawing mode to set.
   * @return The previous drawing mode.
   * @note This function is typically used to change the drawing mode for a specific device context.
   */
  int SetROP2(CDC* deviceContext, int drawMode);

  void SetAlignment(CDC* deviceContext, EoDb::HorizontalAlignment horizontalAlignment, EoDb::VerticalAlignment verticalAlignment);
};
extern EoGsRenderState pstate;
