#pragma once

#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"

class AeSysView;

class CPrimState {
 private:
  EoInt16 m_pointStyle{};
  EoInt16 m_color{7};
  EoInt16 m_LineType{};
  EoDbFontDefinition m_fd;
  EoDbCharacterCellDefinition m_ccd;
  EoInt16 m_PolygonInteriorStyle{};
  EoInt16 m_PolygonInteriorStyleIndex{};

 public:
  CPrimState() : m_pointStyle(0), m_color(7), m_LineType(0), m_PolygonInteriorStyle(0), m_PolygonInteriorStyleIndex(0) {}

  const CPrimState& operator=(const CPrimState&);

 public:
  void GetCharCellDef(EoDbCharacterCellDefinition& ccd) const { ccd = m_ccd; }
  void GetFontDef(EoDbFontDefinition& fd) const { fd = m_fd; }
  const EoInt16& PointStyle() { return m_pointStyle; }
  const EoInt16& PenColor() { return (m_color); }
  const EoInt16& LineType() { return (m_LineType); }
  const EoInt16& PolygonIntStyle() { return (m_PolygonInteriorStyle); }
  const EoInt16& PolygonIntStyleId() { return (m_PolygonInteriorStyleIndex); }
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
  int SetROP2(CDC* deviceContext, int drawMode);
  void SetTxtAlign(CDC* deviceContext, EoUInt16 horizontalAlignment, EoUInt16 verticalAlignment);
};
extern CPrimState pstate;
