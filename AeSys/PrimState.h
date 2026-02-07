#pragma once

#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"

class AeSysView;

class CPrimState {
 private:
  EoInt16 m_pointStyle{};
  EoInt16 m_color{7};
  EoInt16 m_LineTypeIndex{};
  EoDbFontDefinition m_fontDefinition;
  EoDbCharacterCellDefinition m_ccd;
  EoInt16 m_PolygonInteriorStyle{};
  EoInt16 m_PolygonInteriorStyleIndex{};

 public:
  CPrimState() : m_pointStyle(0), m_color(7), m_LineTypeIndex(0), m_PolygonInteriorStyle(0), m_PolygonInteriorStyleIndex(0) {}

  const CPrimState& operator=(const CPrimState&);

 public:
  void GetCharCellDef(EoDbCharacterCellDefinition& ccd) const { ccd = m_ccd; }
  void GetFontDef(EoDbFontDefinition& fd) const { fd = m_fontDefinition; }
  const EoInt16& PointStyle() { return m_pointStyle; }
  [[nodiscard]] const EoInt16& PenColor() const { return m_color; }
  [[nodiscard]] const EoInt16& LineType() const { return m_LineTypeIndex; }
  [[nodiscard]] const EoInt16& PolygonIntStyle() const { return m_PolygonInteriorStyle; }
  [[nodiscard]] const EoInt16& PolygonIntStyleId() const { return m_PolygonInteriorStyleIndex; }
  void Restore(CDC* deviceContext, int saveIndex);
  int Save();
  void SetCharCellDef(EoDbCharacterCellDefinition& ccd) { m_ccd = ccd; }
  void SetFontDef(CDC* deviceContext, const EoDbFontDefinition& fd);
  void SetPointStyle(EoInt16 pointStyle) { m_pointStyle = pointStyle; }
  void SetPolygonIntStyle(EoInt16 interiorStyle) { m_PolygonInteriorStyle = interiorStyle; }
  void SetPolygonIntStyleId(EoInt16 styleIndex) { m_PolygonInteriorStyleIndex = styleIndex; }
  void SetPen(AeSysView* view, CDC* deviceContext, EoInt16 penColor, EoInt16 lineType);
  /// <summary>Manages a small set of pen definitions.</summary>
  void ManagePenResources(CDC* deviceContext, EoInt16 penColor, int penWidth, EoInt16 lineType);
  void SetColor(CDC* deviceContext, EoInt16 color);
  void SetLineType(CDC* deviceContext, EoInt16 lineType);

  /** @brief Sets the current foreground mix mode. GDI uses the foreground mix mode to combine pens and
   * interiors of filled objects with the colors already on the screen. The foreground mix mode
   * defines how colors from the brush or pen and the colors in the existing image are to be combined.
   * @param deviceContext The device context to set the mix mode on.
   * @param drawMode The drawing mode to set.
   * @return The previous drawing mode.
   * @note This function is typically used to change the drawing mode for a specific device context.
   */
  int SetROP2(CDC* deviceContext, int drawMode);

  void SetTxtAlign(CDC* deviceContext, EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment);
};
extern CPrimState pstate;
