#pragma once

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
  static constexpr EoUInt16 BUFFER_SIZE{2048};

  static constexpr EoInt16 COLOR_BYBLOCK{0};
  static constexpr EoInt16 COLOR_BYLAYER{256};
  static constexpr EoInt16 LINETYPE_BYBLOCK{32766};
  static constexpr EoInt16 LINETYPE_BYLAYER{32767};

 protected:
  EoInt16 m_color{COLOR_BYLAYER};
  EoInt16 m_lineTypeIndex{LINETYPE_BYLAYER};
  std::wstring m_lineTypeName{};
  std::wstring m_layerName{};

  static EoInt16 sm_layerColor;
  static EoInt16 sm_layerLineTypeIndex;

  static EoInt16 sm_specialColor;
  static EoUInt16 sm_ControlPointIndex;
  static double sm_RelationshipOfPoint;
  static double sm_SelectApertureSize;

 public:  // Constructors and destructor
  EoDbPrimitive();
  EoDbPrimitive(EoInt16 penColor, EoInt16 lineType);

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

 public:  // Methods - absolute virtuals
  virtual void AddReportToMessageList(EoGePoint3d) = 0;
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
  virtual bool Is(EoUInt16 type) = 0;
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
  virtual void Write(CFile& file, EoUInt8* buffer) = 0;

 public:
  virtual void CutAt2Points(const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*,
                            EoDbGroupList*);
  virtual void CutAtPoint(EoGePoint3d& point, EoDbGroup*);
  virtual int IsWithinArea(EoGePoint3d, EoGePoint3d, EoGePoint3d*);
  virtual void ModifyState();
  virtual bool PivotOnControlPoint(AeSysView*, const EoGePoint4d&);

 public:  // Methods
  void SetBaseProperties(const DRW_Entity* entity, AeSysDoc* document);

  CString FormatPenColor() const;
  CString FormatLineType() const;
  EoInt16 LogicalColor() const;
  EoInt16 LogicalLineType() const;

  EoInt16 Color() const noexcept { return m_color; }
  void SetColor(EoInt16 color) { m_color = color; }
  EoInt16 LineTypeIndex() const noexcept { return m_lineTypeIndex; }
  void SetLineTypeIndex(EoInt16 lineTypeIndex) { m_lineTypeIndex = lineTypeIndex; }

  const std::wstring& LineTypeName() const noexcept { return m_lineTypeName; }
  void SetLineTypeName(std::wstring name) { m_lineTypeName = std::move(name); }

  const std::wstring& LayerName() const noexcept { return m_layerName; }
  void SetLayerName(std::wstring name) { m_layerName = std::move(name); }

 public:  // Methods - static
  static EoUInt16 ControlPointIndex();
  static bool IsSupportedTyp(int type);
  static EoInt16 LayerColor();
  static void SetLayerColor(EoInt16 layerColor);
  static EoInt16 LayerLineTypeIndex();
  static void SetLayerLineTypeIndex(EoInt16 lineTypeIndex);
  static double& Rel();
  static EoInt16 SpecialColor();
  static void SetSpecialColor(EoInt16 specialColor);
};
