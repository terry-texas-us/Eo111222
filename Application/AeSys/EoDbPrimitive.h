#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "EoDxfEntities.h"
#include "EoDxfLineWeights.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeVector3d.h"

HTREEITEM tvAddItem(HWND tree, HTREEITEM parent, const wchar_t* text, LPCVOID object);

class AeSysDoc;
class AeSysView;
class EoDbGroupList;
class EoDbGroup;
class EoDbHandleManager;
class EoDxfInterface;
class EoGsRenderDevice;
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
  std::uint64_t m_handle{};
  std::uint64_t m_ownerHandle{};
  double m_thickness{};
  EoDxfLineWeights::LineWeight m_lineWeight{EoDxfLineWeights::LineWeight::kLnWtByLwDefault};
  double m_lineTypeScale{1.0};

  static std::int16_t sm_layerColor;
  static std::int16_t sm_layerLineTypeIndex;
  static std::wstring sm_layerLineTypeName;
  static EoDxfLineWeights::LineWeight sm_layerLineWeight;
  static double sm_layerLineTypeScale;

  static std::int16_t sm_specialColor;
  static int sm_controlPointIndex;
  static double sm_RelationshipOfPoint;
  static double sm_SelectApertureSize;
  static EoDbHandleManager* sm_handleManager;

 public:
  EoDbPrimitive();
  EoDbPrimitive(std::int16_t penColor, std::int16_t lineType);

 protected:
  /// @brief Copy constructor assigns a fresh handle — a copy is a new entity.
  /// All visual/spatial properties are copied from @p other, but the handle
  /// is unique to this instance.
  EoDbPrimitive(const EoDbPrimitive& other);

  /// @brief Copy-assignment overwrites visual/spatial properties but preserves
  /// the destination's handle — entity identity does not change on property update.
  EoDbPrimitive& operator=(const EoDbPrimitive& other);

 public:
  ~EoDbPrimitive() override;

  virtual void AddReportToMessageList(const EoGePoint3d&);
  virtual void AddToTreeViewControl(HWND, HTREEITEM) = 0;
  virtual void Assign(EoDbPrimitive* primitive) = 0;
  virtual EoDbPrimitive*& Copy(EoDbPrimitive*&) = 0;
  virtual void Display(AeSysView* view, EoGsRenderDevice* renderDevice) = 0;
  virtual void FormatExtra(CString& extra);
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

  /// @brief Writes per-primitive AE2026 V2 extension data after the generic V2 block.
  /// Default is no-op. Override for primitives that carry V2-specific relationships (e.g., attribute handles).
  virtual void WriteV2Extension(CFile& file) const;
  /// @brief Reads per-primitive AE2026 V2 extension data after the generic V2 block.
  virtual void ReadV2Extension(CFile& file);

  virtual void CutAt2Points(
      const EoGePoint3d& firstPoint, const EoGePoint3d& secondPoint, EoDbGroupList*, EoDbGroupList*);
  virtual void CutAtPoint(const EoGePoint3d& point, EoDbGroup*);
  virtual int IsWithinArea(const EoGePoint3d& lowerLeft, const EoGePoint3d& upperRight, EoGePoint3d*);
  virtual void ModifyState();
  virtual bool PivotOnControlPoint(AeSysView*, const EoGePoint4d&);

  void SetBaseProperties(const EoDxfGraphic* entity, AeSysDoc* document);

  /// @brief Populates base DXF entity properties from this primitive's members.
  /// Call this in every ExportToDxf override before writer->Add*().
  /// @param entity The DXF entity to populate with layer, linetype, color, and handle information.
  void PopulateDxfBaseProperties(EoDxfGraphic* entity) const;

  /// @brief Exports this primitive to DXF via the writer interface.
  /// Default implementation is a no-op for primitive types that have no DXF equivalent.
  /// @param writer The DXF interface to write the entity through.
  virtual void ExportToDxf(EoDxfInterface* writer) const;

  CString FormatPenColor() const;
  CString FormatLineType() const;
  std::int16_t LogicalColor() const noexcept;
  std::int16_t LogicalLineType() const noexcept;
  [[nodiscard]] const std::wstring& LogicalLineTypeName() const;

  [[nodiscard]] std::int16_t Color() const noexcept { return m_color; }
  [[nodiscard]] std::uint64_t Handle() const noexcept { return m_handle; }
  [[nodiscard]] const std::wstring& LayerName() const noexcept { return m_layerName; }
  [[nodiscard]] std::int16_t LineTypeIndex() const noexcept { return m_lineTypeIndex; }
  [[nodiscard]] const std::wstring& LineTypeName() const noexcept { return m_lineTypeName; }
  [[nodiscard]] double LineTypeScale() const noexcept { return m_lineTypeScale; }
  [[nodiscard]] EoDxfLineWeights::LineWeight LineWeight() const noexcept { return m_lineWeight; }
  [[nodiscard]] std::uint64_t OwnerHandle() const noexcept { return m_ownerHandle; }
  [[nodiscard]] double Thickness() const noexcept { return m_thickness; }

  void SetColor(std::int16_t color) noexcept { m_color = color; }
  void SetHandle(std::uint64_t handle) noexcept { m_handle = handle; }
  void SetLineTypeIndex(std::int16_t lineTypeIndex) noexcept { m_lineTypeIndex = lineTypeIndex; }
  void SetLineTypeName(std::wstring name) noexcept { m_lineTypeName = std::move(name); }
  void SetLineTypeScale(double scale) noexcept { m_lineTypeScale = scale; }
  void SetLineWeight(EoDxfLineWeights::LineWeight lineWeight) noexcept { m_lineWeight = lineWeight; }
  void SetLayerName(std::wstring name) noexcept { m_layerName = std::move(name); }
  void SetOwnerHandle(std::uint64_t ownerHandle) noexcept { m_ownerHandle = ownerHandle; }
  void SetThickness(double thickness) noexcept { m_thickness = thickness; }

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
  static const std::wstring& LayerLineTypeName() noexcept;
  static void SetLayerLineTypeName(const std::wstring& lineTypeName);
  static EoDxfLineWeights::LineWeight LayerLineWeight() noexcept;
  static void SetLayerLineWeight(EoDxfLineWeights::LineWeight lineWeight) noexcept;
  static double LayerLineTypeScale() noexcept;
  static void SetLayerLineTypeScale(double lineTypeScale) noexcept;
  static double& Rel() noexcept;
  static std::int16_t SpecialColor() noexcept;
  static void SetSpecialColor(std::int16_t specialColor) noexcept;

  /// @brief Wires the document-owned handle manager into the primitive base class.
  /// Called once from the AeSysDoc constructor so that every subsequently-created
  /// primitive receives a unique handle via AssignHandle().
  static void SetHandleManager(EoDbHandleManager* handleManager) noexcept;

  /// @brief Saves the current handle manager and sets it to nullptr, suppressing handle
  /// assignment in subsequently constructed primitives.  Returns the saved pointer
  /// for later restoration via ResumeHandleAssignment().
  [[nodiscard]] static EoDbHandleManager* SuspendHandleAssignment() noexcept;

  /// @brief Restores a previously saved handle manager, re-enabling handle assignment.
  static void ResumeHandleAssignment(EoDbHandleManager* saved) noexcept;
};

/// @brief RAII guard that suppresses handle assignment for ephemeral/preview primitives.
/// While active, newly constructed EoDbPrimitive instances receive handle 0 (kNoHandle)
/// and the handle counter does not advance.  Nest-safe: inner scopes are no-ops.
class EoDbHandleSuppressionScope {
  EoDbHandleManager* m_saved;

 public:
  EoDbHandleSuppressionScope() noexcept : m_saved(EoDbPrimitive::SuspendHandleAssignment()) {}
  ~EoDbHandleSuppressionScope() noexcept { EoDbPrimitive::ResumeHandleAssignment(m_saved); }
  EoDbHandleSuppressionScope(const EoDbHandleSuppressionScope&) = delete;
  EoDbHandleSuppressionScope& operator=(const EoDbHandleSuppressionScope&) = delete;
};
