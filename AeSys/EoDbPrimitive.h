#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"
#include "drw_entities.h"

HTREEITEM tvAddItem(HWND tree, HTREEITEM parent, LPWSTR pszText, LPCVOID object);

class AeSysDoc;
class AeSysView;
class EoDbGroupList;
class EoDbGroup;
class EoGeTransformMatrix;

class EoDbPrimitive : public CObject {
 public:
  static constexpr std::uint16_t BUFFER_SIZE{2048};

  static constexpr std::int16_t COLOR_BYBLOCK{0};
  static constexpr std::int16_t COLOR_BYLAYER{256};
  static constexpr std::int16_t LINETYPE_BYBLOCK{32766};
  static constexpr std::int16_t LINETYPE_BYLAYER{32767};

 protected:
  std::int16_t m_color{COLOR_BYLAYER};
  std::int16_t m_lineTypeIndex{LINETYPE_BYLAYER};
  std::wstring m_lineTypeName{};
  std::wstring m_layerName{};

  static std::int16_t sm_layerColor;
  static std::int16_t sm_layerLineTypeIndex;

  static std::int16_t sm_specialColor;
  static int sm_controlPointIndex;
  static double sm_RelationshipOfPoint;
  static double sm_SelectApertureSize;

 public:
  EoDbPrimitive() = default;
  EoDbPrimitive(std::int16_t penColor, std::int16_t lineType);

 protected:
  EoDbPrimitive(const EoDbPrimitive& other)
      : m_color(other.m_color),
        m_lineTypeIndex(other.m_lineTypeIndex),
        m_lineTypeName(other.m_lineTypeName),
        m_layerName(other.m_layerName) {}

  EoDbPrimitive& operator=(const EoDbPrimitive& other) {
    if (this != &other) {
      m_color = other.m_color;
      m_lineTypeIndex = other.m_lineTypeIndex;
      m_lineTypeName = other.m_lineTypeName;
      m_layerName = other.m_layerName;
    }
    return *this;
  }

 public:
  ~EoDbPrimitive() override;

  virtual void AddReportToMessageList(const EoGePoint3d&) = 0;
  virtual void AddToTreeViewControl(HWND, HTREEITEM) = 0;
  virtual void Assign(EoDbPrimitive* primitive) = 0;
  virtual EoDbPrimitive*& Copy(EoDbPrimitive*&) = 0;
  virtual void Display(AeSysView* view, CDC* deviceContext) = 0;
  virtual void FormatExtra(CString& extra) = 0;
  virtual void FormatGeometry(CString& str) = 0;
  virtual void GetAllPoints(EoGePoint3dArray& points) = 0;
  virtual EoGePoint3d GetControlPoint() = 0;
  virtual void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, const EoGeTransformMatrix&) = 0;
  virtual EoGePoint3d GoToNextControlPoint() = 0;
  virtual bool Identical(EoDbPrimitive*) = 0;
  virtual bool Is(std::uint16_t type) noexcept = 0;
  virtual bool IsInView(AeSysView* view) = 0;
  virtual bool IsPointOnControlPoint(AeSysView* view, const EoGePoint4d& point) = 0;
  virtual EoGePoint3d SelectAtControlPoint(AeSysView* view, const EoGePoint4d& point) = 0;
  virtual bool SelectUsingLine(AeSysView* view, EoGeLine line, EoGePoint3dArray& intersections) = 0;
  virtual bool SelectUsingPoint(AeSysView* view, EoGePoint4d point, EoGePoint3d&) = 0;
  virtual bool SelectUsingRectangle(AeSysView* view, EoGePoint3d, EoGePoint3d) = 0;
  virtual void Transform(const EoGeTransformMatrix&) = 0;
  virtual void Translate(const EoGeVector3d&) = 0;
  virtual void TranslateUsingMask(EoGeVector3d, const DWORD) = 0;
  virtual bool Write(CFile& file) = 0;
  virtual void Write(CFile& file, std::uint8_t* buffer) = 0;

  virtual void CutAt2Points(
      const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*, EoDbGroupList*);
  virtual void CutAtPoint(const EoGePoint3d& point, EoDbGroup*);
  virtual int IsWithinArea(const EoGePoint3d& lowerLeft, const EoGePoint3d& upperRight, EoGePoint3d*);
  virtual void ModifyState();
  virtual bool PivotOnControlPoint(AeSysView*, const EoGePoint4d&);

  void SetBaseProperties(const DRW_Entity* entity, AeSysDoc* document);

  CString FormatPenColor() const;
  CString FormatLineType() const;
  std::int16_t LogicalColor() const noexcept;
  std::int16_t LogicalLineType() const noexcept;

  [[nodiscard]] std::int16_t Color() const noexcept { return m_color; }
  [[nodiscard]] std::int16_t LineTypeIndex() const noexcept { return m_lineTypeIndex; }
  [[nodiscard]] const std::wstring& LineTypeName() const noexcept { return m_lineTypeName; }
  [[nodiscard]] const std::wstring& LayerName() const noexcept { return m_layerName; }

  void SetColor(std::int16_t color) noexcept { m_color = color; }
  void SetLineTypeIndex(std::int16_t lineTypeIndex) noexcept { m_lineTypeIndex = lineTypeIndex; }
  void SetLineTypeName(std::wstring name) noexcept { m_lineTypeName = std::move(name); }
  void SetLayerName(std::wstring name) noexcept { m_layerName = std::move(name); }
  void SetProperties(std::int16_t color, std::int16_t lineTypeIndex) noexcept {
    m_color = color;
    m_lineTypeIndex = lineTypeIndex;
  }

  [[nodiscard]] EoDbPrimitive* WithProperties(std::int16_t color, std::int16_t lineTypeIndex) noexcept {
    m_color = color;
    m_lineTypeIndex = lineTypeIndex;
    return this;
  }

  static int ControlPointIndex() noexcept;
  static bool IsSupportedTyp(int type) noexcept;
  static std::int16_t LayerColor() noexcept;
  static void SetLayerColor(std::int16_t layerColor) noexcept;
  static std::int16_t LayerLineTypeIndex() noexcept;
  static void SetLayerLineTypeIndex(std::int16_t lineTypeIndex) noexcept;
  static double& Rel() noexcept;
  static std::int16_t SpecialColor() noexcept;
  static void SetSpecialColor(std::int16_t specialColor) noexcept;
};
