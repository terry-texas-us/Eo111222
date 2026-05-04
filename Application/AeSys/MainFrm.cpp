
#include "Stdafx.h"

#include <algorithm>
#include <cassert>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoCtrlColorComboBox.h"
#include "EoCtrlLayerComboBox.h"
#include "EoCtrlLineTypeComboBox.h"
#include "EoCtrlLineWeightComboBox.h"
#include "EoCtrlTextStyleComboBox.h"
#include "EoDbAttrib.h"
#include "EoDbBlockReference.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoGeTransformMatrix.h"
#include "EoMfVisualManager.h"
#include "MainFrm.h"
#include "Resource.h"

namespace {

// ---------------------------------------------------------------------------
// Room-boundary query helpers
// ---------------------------------------------------------------------------

/// @brief Shoelace formula — returns signed 2-D area (positive = CCW).
static double SignedArea2D(const std::vector<EoGePoint3d>& pts) noexcept {
  const auto n = pts.size();
  if (n < 3) { return 0.0; }
  double sum = 0.0;
  for (size_t i = 0; i < n; ++i) {
    const auto& a = pts[i];
    const auto& b = pts[(i + 1) % n];
    sum += a.x * b.y - b.x * a.y;
  }
  return sum * 0.5;
}

/// @brief 2-D perimeter of a closed polygon (last→first edge included).
static double Perimeter2D(const std::vector<EoGePoint3d>& pts) noexcept {
  const auto n = pts.size();
  if (n < 2) { return 0.0; }
  double len = 0.0;
  for (size_t i = 0; i < n; ++i) {
    const auto& a = pts[i];
    const auto& b = pts[(i + 1) % n];
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    len += std::sqrt(dx * dx + dy * dy);
  }
  return len;
}

/// @brief Ray-cast point-in-polygon test (2-D, winding on XY plane).
static bool PointInPolygon2D(const std::vector<EoGePoint3d>& pts, double px, double py) noexcept {
  const auto n = pts.size();
  if (n < 3) { return false; }
  bool inside = false;
  for (size_t i = 0, j = n - 1; i < n; j = i++) {
    const double xi = pts[i].x, yi = pts[i].y;
    const double xj = pts[j].x, yj = pts[j].y;
    if (((yi > py) != (yj > py)) && (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) { inside = !inside; }
  }
  return inside;
}

/// @brief Describes one result from QueryRoomAtPoint.
struct RoomResult {
  bool found{false};
  double area{0.0};
  double perimeter{0.0};
  std::string layerUtf8;
  int vertexCount{0};
};

/// @brief Flatten EoGePoint3dArray (CArray-based) to std::vector<EoGePoint3d>.
static std::vector<EoGePoint3d> ToVector(const EoGePoint3dArray& arr) {
  std::vector<EoGePoint3d> v;
  const auto n = arr.GetSize();
  v.reserve(static_cast<size_t>(n));
  for (INT_PTR i = 0; i < n; ++i) { v.push_back(arr[i]); }
  return v;
}

/// @brief Encode a CString value for JSON (escape \ and ").
static std::string RoomEncodeJson(const CString& s) {
  std::string out;
  out.reserve(static_cast<size_t>(s.GetLength()));
  for (int i = 0; i < s.GetLength(); ++i) {
    const wchar_t wc = s[i];
    if (wc == L'\\') {
      out += "\\\\";
    } else if (wc == L'"') {
      out += "\\\"";
    } else {
      out += static_cast<char>(wc & 0xFF);
    }
  }
  return out;
}

/// @brief Overloads for std::wstring and EoDbAttrib tag (both used by batch query helpers).
static std::string EncodeJsonString(const CString& s) {
  return RoomEncodeJson(s);
}
static std::string EncodeJsonString(const std::wstring& s) {
  return RoomEncodeJson(CString(s.c_str()));
}

/// @brief Walk all model-space layers looking for a closed primitive that contains
///        (px, py).  Returns RoomResult with area, perimeter, layer, vertex count.
static RoomResult QueryRoomAtPoint(AeSysDoc* doc, double px, double py) {
  RoomResult result;

  const CLayers& layers = doc->SpaceLayers(EoDxf::Space::ModelSpace);
  const INT_PTR layerCount = layers.GetSize();

  for (INT_PTR li = 0; li < layerCount; ++li) {
    EoDbLayer* layer = layers[li];
    if (layer == nullptr) { continue; }

    // Layer name for reporting.
    const std::string layerName = RoomEncodeJson(layer->Name());

    // Iterate groups.
    auto pos = layer->GetHeadPosition();
    while (pos != nullptr) {
      EoDbGroup* group = layer->GetNext(pos);
      if (group == nullptr) { continue; }

      // Iterate primitives within the group.
      auto primPos = group->GetHeadPosition();
      while (primPos != nullptr) {
        EoDbPrimitive* prim = group->GetNext(primPos);
        if (prim == nullptr) { continue; }

        if (prim->Is(EoDb::kPolygonPrimitive)) {
          // Polygons are always closed.
          EoGePoint3dArray pts;
          prim->GetAllPoints(pts);
          auto v = ToVector(pts);
          if (PointInPolygon2D(v, px, py)) {
            result.found = true;
            result.area = std::abs(SignedArea2D(v));
            result.perimeter = Perimeter2D(v);
            result.layerUtf8 = layerName;
            result.vertexCount = static_cast<int>(v.size());
            return result;
          }
        } else if (prim->Is(EoDb::kPolylinePrimitive)) {
          auto* pl = static_cast<EoDbPolyline*>(prim);
          if (!pl->IsClosed()) { continue; }
          EoGePoint3dArray pts;
          prim->GetAllPoints(pts);
          auto v = ToVector(pts);
          if (PointInPolygon2D(v, px, py)) {
            result.found = true;
            result.area = std::abs(SignedArea2D(v));
            result.perimeter = Perimeter2D(v);
            result.layerUtf8 = layerName;
            result.vertexCount = static_cast<int>(v.size());
            return result;
          }
        }
      }
    }
  }

  return result;  // not found
}

// ---------------------------------------------------------------------------
// Batch room query helper
// ---------------------------------------------------------------------------

/// @brief Single room entry returned by QueryAllRooms.
struct RoomEntry {
  double area{0.0};
  double perimeter{0.0};
  double centroidX{0.0};
  double centroidY{0.0};
  int vertexCount{0};
  std::string layerUtf8;
};

/// @brief Walk all model-space layers collecting every closed polygon / closed polyline.
///        Pass a non-empty @p layerFilter to restrict results to one layer (case-insensitive).
static std::vector<RoomEntry> QueryAllRooms(AeSysDoc* doc, const std::wstring& layerFilter) {
  std::vector<RoomEntry> results;
  const CLayers& layers = doc->SpaceLayers(EoDxf::Space::ModelSpace);
  for (INT_PTR li = 0, ln = layers.GetSize(); li < ln; ++li) {
    EoDbLayer* layer = layers[li];
    if (layer == nullptr) { continue; }
    if (!layerFilter.empty()) {
      if (layer->Name().CompareNoCase(CString(layerFilter.c_str())) != 0) { continue; }
    }
    const std::string layerName = EncodeJsonString(layer->Name());

    auto pos = layer->GetHeadPosition();
    while (pos != nullptr) {
      EoDbGroup* group = layer->GetNext(pos);
      if (group == nullptr) { continue; }
      auto primPos = group->GetHeadPosition();
      while (primPos != nullptr) {
        EoDbPrimitive* prim = group->GetNext(primPos);
        if (prim == nullptr) { continue; }

        bool isClosed = prim->Is(EoDb::kPolygonPrimitive);
        if (!isClosed && prim->Is(EoDb::kPolylinePrimitive)) {
          isClosed = static_cast<EoDbPolyline*>(prim)->IsClosed();
        }
        if (!isClosed) { continue; }

        EoGePoint3dArray pts;
        prim->GetAllPoints(pts);
        const auto v = ToVector(pts);
        if (v.size() < 3) { continue; }

        RoomEntry entry;
        entry.area = std::abs(SignedArea2D(v));
        entry.perimeter = Perimeter2D(v);
        entry.vertexCount = static_cast<int>(v.size());
        entry.layerUtf8 = layerName;
        double cx = 0.0, cy = 0.0;
        for (const auto& p : v) {
          cx += p.x;
          cy += p.y;
        }
        const double invN = 1.0 / static_cast<double>(v.size());
        entry.centroidX = cx * invN;
        entry.centroidY = cy * invN;
        results.push_back(std::move(entry));
      }
    }
  }
  return results;
}

// ---------------------------------------------------------------------------
// Layer query helper
// ---------------------------------------------------------------------------

/// @brief Describes one result from QueryLayerByName.
struct LayerResult {
  bool found{false};
  int groupCount{0};
  int primitiveCount{0};
  bool visible{true};
  bool isWork{false};
  double minX{0.0}, minY{0.0}, maxX{0.0}, maxY{0.0};
  bool hasExtents{false};
};

/// @brief Search model-space layers for an exact case-insensitive name match
///        and return primitive/group counts, visibility, and extents.
static LayerResult QueryLayerByName(AeSysDoc* doc, AeSysView* view, const std::wstring& name) {
  LayerResult result;

  const CLayers& layers = doc->SpaceLayers(EoDxf::Space::ModelSpace);
  const INT_PTR layerCount = layers.GetSize();

  for (INT_PTR li = 0; li < layerCount; ++li) {
    EoDbLayer* layer = layers[li];
    if (layer == nullptr) { continue; }

    // Case-insensitive comparison.
    const CString layerName = layer->Name();
    CString searchName(name.c_str());
    if (layerName.CompareNoCase(searchName) != 0) { continue; }

    result.found = true;
    result.visible = !layer->IsOff();
    result.isWork = layer->IsWork();

    // Count groups and primitives.
    auto pos = layer->GetHeadPosition();
    while (pos != nullptr) {
      EoDbGroup* group = layer->GetNext(pos);
      if (group == nullptr) { continue; }
      ++result.groupCount;
      result.primitiveCount += static_cast<int>(group->GetCount());
    }

    // Compute extents if the layer has geometry.
    if (result.primitiveCount > 0 && view != nullptr) {
      EoGePoint3d ptMin(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
      EoGePoint3d ptMax(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);
      EoGeTransformMatrix identity;
      layer->GetExtents(view, ptMin, ptMax, identity);
      if (ptMin.x <= ptMax.x && ptMin.y <= ptMax.y) {
        result.hasExtents = true;
        result.minX = ptMin.x;
        result.minY = ptMin.y;
        result.maxX = ptMax.x;
        result.maxY = ptMax.y;
      }
    }
    return result;
  }
  return result;  // not found
}

// ---------------------------------------------------------------------------
// Batch block-reference query helper
// ---------------------------------------------------------------------------

/// @brief Single block-reference entry returned by QueryAllBlocks.
struct BlockEntry {
  std::string name;
  std::string layer;
  double x{0.0}, y{0.0}, z{0.0};
  double rotation{0.0};
  double scaleX{1.0}, scaleY{1.0}, scaleZ{1.0};
  std::string attribsJson{"[]"};
};

/// @brief Collect every EoDbBlockReference in model space, optionally filtered by layer name.
static std::vector<BlockEntry> QueryAllBlocks(AeSysDoc* doc, const std::wstring& layerFilter) {
  std::vector<BlockEntry> results;
  const CLayers& layers = doc->SpaceLayers(EoDxf::Space::ModelSpace);
  for (INT_PTR li = 0, ln = layers.GetSize(); li < ln; ++li) {
    EoDbLayer* layer = layers[li];
    if (layer == nullptr) { continue; }
    if (!layerFilter.empty()) {
      if (layer->Name().CompareNoCase(CString(layerFilter.c_str())) != 0) { continue; }
    }
    auto pos = layer->GetHeadPosition();
    while (pos != nullptr) {
      EoDbGroup* group = layer->GetNext(pos);
      if (group == nullptr) { continue; }
      auto primPos = group->GetHeadPosition();
      while (primPos != nullptr) {
        EoDbPrimitive* prim = group->GetNext(primPos);
        if (prim == nullptr || !prim->Is(EoDb::kGroupReferencePrimitive)) { continue; }
        auto* ref = static_cast<EoDbBlockReference*>(prim);

        BlockEntry entry;
        entry.name = EncodeJsonString(ref->BlockName());
        entry.layer = EncodeJsonString(CString(ref->LayerName().c_str()));
        const EoGePoint3d ip = ref->InsertionPoint();
        entry.x = ip.x;
        entry.y = ip.y;
        entry.z = ip.z;
        entry.rotation = ref->Rotation() * (180.0 / Eo::Pi);
        const EoGeVector3d sf = ref->ScaleFactors();
        entry.scaleX = sf.x;
        entry.scaleY = sf.y;
        entry.scaleZ = sf.z;

        std::string aJson = "[";
        bool firstA = true;
        for (const std::uint64_t h : ref->AttributeHandles()) {
          auto* ap = doc->FindPrimitiveByHandle(h);
          if (ap == nullptr || !ap->Is(EoDb::kAttribPrimitive)) { continue; }
          auto* attrib = static_cast<EoDbAttrib*>(ap);
          if (!firstA) { aJson += ','; }
          firstA = false;
          char buf[512];
          _snprintf_s(buf,
              _TRUNCATE,
              "{\"tag\":\"%s\",\"value\":\"%s\"}",
              EncodeJsonString(attrib->TagString()).c_str(),
              EncodeJsonString(attrib->Text()).c_str());
          aJson += buf;
        }
        aJson += ']';
        entry.attribsJson = std::move(aJson);
        results.push_back(std::move(entry));
      }
    }
  }
  return results;
}

// ---------------------------------------------------------------------------
// Batch layer-list query helper
// ---------------------------------------------------------------------------

/// @brief Single layer entry returned by QueryAllLayersList.
struct LayerListEntry {
  std::string nameUtf8;
  bool visible{true};
  bool isWork{false};
  int groupCount{0};
  int primitiveCount{0};
};

/// @brief Enumerate every model-space layer with its visibility, work flag, and counts.
static std::vector<LayerListEntry> QueryAllLayersList(AeSysDoc* doc) {
  std::vector<LayerListEntry> results;
  const CLayers& layers = doc->SpaceLayers(EoDxf::Space::ModelSpace);
  for (INT_PTR li = 0, ln = layers.GetSize(); li < ln; ++li) {
    EoDbLayer* layer = layers[li];
    if (layer == nullptr) { continue; }
    LayerListEntry entry;
    entry.nameUtf8 = EncodeJsonString(layer->Name());
    entry.visible = !layer->IsOff();
    entry.isWork = layer->IsWork();
    auto pos = layer->GetHeadPosition();
    while (pos != nullptr) {
      EoDbGroup* group = layer->GetNext(pos);
      if (group == nullptr) { continue; }
      ++entry.groupCount;
      entry.primitiveCount += static_cast<int>(group->GetCount());
    }
    results.push_back(std::move(entry));
  }
  return results;
}

/// @brief Trim leading and trailing ASCII whitespace from a wide string.
static std::wstring TrimWs(std::wstring s) {
  const auto first = s.find_first_not_of(L" \t\r\n");
  if (first == std::wstring::npos) { return {}; }
  const auto last = s.find_last_not_of(L" \t\r\n");
  return s.substr(first, last - first + 1);
}

constexpr int statusInfo = 0;
constexpr int statusLength = 1;
constexpr int statusAngle = 2;
constexpr int statusCmd = 3;  // mode name / CMD focus indicator
constexpr int statusScale = 14;
constexpr int statusZoom = 15;
constexpr int maxUserToolbars = 10;
constexpr unsigned int firstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
constexpr unsigned int lastUserToolBarId = firstUserToolBarId + maxUserToolbars - 1;
constexpr unsigned int indicators[] = {
    ID_SEPARATOR,  // 0: message pane (fixed ~36 characters)
    ID_INDICATOR_LENGTH,  // 1: dimension length display
    ID_INDICATOR_ANGLE,  // 2: dimension angle display
    ID_INDICATOR_CMD,  // 3: mode name when idle; "CMD" accent when command tab is focused
    ID_OP0,  // 4–13: mode key-command help panes
    ID_OP1,
    ID_OP2,
    ID_OP3,
    ID_OP4,
    ID_OP5,
    ID_OP6,
    ID_OP7,
    ID_OP8,
    ID_OP9,
    ID_INDICATOR_SCALE,  // 14: world scale display
    ID_INDICATOR_ZOOM,  // 15: zoom ratio display
    ID_SEPARATOR,  // 16: stretch filler — absorbs remaining space
};
}  // namespace

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_CREATE()
#pragma warning(pop)
ON_WM_DESTROY()
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_MDIACTIVATE()
#pragma warning(pop)
ON_COMMAND(ID_CONTEXT_HELP, &CMDIFrameWndEx::OnContextHelp)
ON_COMMAND(ID_DEFAULT_HELP, &CMDIFrameWndEx::OnHelpFinder)
ON_COMMAND(ID_HELP_FINDER, &CMDIFrameWndEx::OnHelpFinder)
ON_COMMAND(ID_HELP, &CMDIFrameWndEx::OnHelp)
ON_COMMAND(ID_MDI_TABBED, OnMdiTabbed)
ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullScreen)
ON_COMMAND(ID_WINDOW_MANAGER, &CMainFrame::OnWindowManager)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_REGISTERED_MESSAGE(AFX_WM_TOOLBARMENU, OnToolbarContextMenu)
ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, OnGetTabToolTip)
ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, OnToolbarReset)
#pragma warning(pop)
ON_MESSAGE(EoNamedPipeServer::WM_APP_PIPE_COMMAND, &CMainFrame::OnPipeCommand)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_MDI_TABBED, OnUpdateMdiTabbed)
ON_UPDATE_COMMAND_UI(ID_PENCOLOR_COMBO, OnUpdatePenColorCombo)
ON_UPDATE_COMMAND_UI(ID_LINETYPE_COMBO, OnUpdateLineTypeCombo)
ON_UPDATE_COMMAND_UI(ID_LINEWEIGHT_COMBO, OnUpdateLineWeightCombo)
ON_UPDATE_COMMAND_UI(ID_TEXTSTYLE_COMBO, OnUpdateTextStyleCombo)
ON_UPDATE_COMMAND_UI(ID_LAYER_COMBO, OnUpdateLayerCombo)
#pragma warning(pop)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() {}
CMainFrame::~CMainFrame() {}
int CMainFrame::OnCreate(LPCREATESTRUCT createStruct) {
  if (CMDIFrameWndEx::OnCreate(createStruct) == -1) { return -1; }

  CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(EoMfVisualManager));

  // Configure MDI tabbed groups with hardcoded settings (no longer user-configurable)
  {
    CMDITabInfo tabInfo;
    tabInfo.m_tabLocation = CMFCTabCtrl::LOCATION_TOP;
    tabInfo.m_style = CMFCTabCtrl::STYLE_3D;
    tabInfo.m_bTabIcons = FALSE;
    tabInfo.m_bTabCloseButton = FALSE;
    tabInfo.m_bTabCustomTooltips = TRUE;
    tabInfo.m_bAutoColor = FALSE;
    tabInfo.m_bDocumentMenu = TRUE;
    tabInfo.m_bEnableTabSwap = TRUE;
    tabInfo.m_bFlatFrame = TRUE;
    tabInfo.m_bActiveTabCloseButton = TRUE;
    tabInfo.m_nTabBorderSize = 0;
    EnableMDITabbedGroups(TRUE, tabInfo);
  }

  if (!m_menuBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create menubar\n");
    return -1;
  }
  m_menuBar.SetPaneStyle(m_menuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

  // Prevent the menu bar from taking the focus on activation
  CMFCPopupMenu::SetForceMenuFocus(FALSE);
  constexpr DWORD controlStyle(WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

  // Default button/image sizes for initial toolbar creation. The actual button height
  // is adjusted after the first RecalcLayout, which creates the combo HWNDs via
  // OnShow → CreateCombo. At that point we measure the real combo closed height and
  // set all toolbars to match, ensuring icon-only and combo-containing toolbars
  // report identical row heights to the docking manager.
  const CSize imageSize(24, 24);
  const CSize buttonSize(32, 32);

  if (!m_standardToolBar.CreateEx(this, TBSTYLE_FLAT, controlStyle, CRect(1, 1, 1, 1), IDR_MAINFRAME_24)
      || !m_standardToolBar.LoadToolBar(IDR_MAINFRAME_24, 0, 0, TRUE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create toolbar\n");
    return -1;
  }
  // SetSizes must be called AFTER LoadToolBar — LoadToolBar with bLocked=TRUE
  // recalculates m_sizeButton from resource dimensions, overriding any pre-set sizes.
  CMFCToolBar::SetSizes(buttonSize, imageSize);
  m_standardToolBar.SetWindowTextW(L"Standard");

  if (!m_renderPropertiesToolBar.CreateEx(this, TBSTYLE_FLAT, controlStyle)
      || !m_renderPropertiesToolBar.LoadToolBar(IDR_RENDER_PROPERTIES, 0, 0, TRUE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create render properties toolbar\n");
    return -1;
  }
  // Match cell height to the standard toolbar so all toolbars on the same docking row
  // report identical height to the docking manager.
  EoMfStatelessToolBar::SetSizes(buttonSize, imageSize);
  m_renderPropertiesToolBar.SetWindowTextW(L"Properties");

  if (!m_stylesToolBar.CreateEx(this, TBSTYLE_FLAT, controlStyle)
      || !m_stylesToolBar.LoadToolBar(IDR_STYLES, 0, 0, TRUE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create styles toolbar\n");
    return -1;
  }
  // Match cell height to the standard toolbar
  EoMfStatelessToolBar::SetSizes(buttonSize, imageSize);
  m_stylesToolBar.SetWindowTextW(L"Styles");

  if (!m_layerPropertiesToolBar.CreateEx(this, TBSTYLE_FLAT, controlStyle)
      || !m_layerPropertiesToolBar.LoadToolBar(IDR_LAYER_PROPERTIES, 0, 0, TRUE)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create layer properties toolbar\n");
    return -1;
  }
  EoMfStatelessToolBar::SetSizes(buttonSize, imageSize);
  m_layerPropertiesToolBar.SetWindowTextW(L"Layers");

  InitUserToolbars(nullptr, firstUserToolBarId, lastUserToolBarId);

  if (!m_statusBar.Create(this)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create status bar\n");
    return -1;
  }
  // Remove the size gripper — modern apps allow resizing from any window edge/corner
  m_statusBar.ModifyStyle(SBARS_SIZEGRIP, 0);
  m_statusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(unsigned int));

  // Message pane: fixed width (~36 characters at default font size)
  m_statusBar.SetPaneInfo(statusInfo, ID_SEPARATOR, SBPS_NOBORDERS, 288);

  // Length and Angle panes: fixed width for dimension display
  m_statusBar.SetPaneInfo(statusLength, ID_INDICATOR_LENGTH, SBPS_NOBORDERS, 120);
  m_statusBar.SetPaneInfo(statusAngle, ID_INDICATOR_ANGLE, SBPS_NOBORDERS, 100);

  // CMD pane: shows current mode name when idle; accent style when command tab is focused
  m_statusBar.SetPaneInfo(statusCmd, ID_INDICATOR_CMD, SBPS_NOBORDERS, 90);
  m_statusBar.SetPaneText(statusCmd, L"Ready");

  // World Scale and Zoom Ratio moved off the status bar — both zeroed out.
  // World Scale is now on the layout tab bar. Zoom Ratio is removed entirely.
  m_statusBar.SetPaneInfo(statusScale, ID_INDICATOR_SCALE, SBPS_NOBORDERS | SBPS_DISABLED, 0);
  m_statusBar.SetPaneInfo(statusZoom, ID_INDICATOR_ZOOM, SBPS_NOBORDERS | SBPS_DISABLED, 0);

  // Trailing stretch filler: absorbs remaining space after all fixed panes
  m_statusBar.SetPaneStyle(16, SBPS_STRETCH | SBPS_NOBORDERS);

  if (!CreateDockablePanes()) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create dockable panes\n");
    return -1;
  }
  m_menuBar.EnableDocking(CBRS_ALIGN_ANY);
  m_standardToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_renderPropertiesToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_layerPropertiesToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_stylesToolBar.EnableDocking(CBRS_ALIGN_ANY);
  m_propertiesPane.EnableDocking(CBRS_ALIGN_ANY);
  m_outputPane.EnableDocking(CBRS_ALIGN_ANY);

  EnableDocking(CBRS_ALIGN_ANY);

  // Single shared row: [MenuBar][Standard][RenderProps][Layers][Styles]
  // DockPane inserts at row 0, so dock the rightmost toolbar first,
  // then build leftward with DockPaneLeftOf, ending with the menu bar.
  m_menuBar.SetExclusiveRowMode(FALSE);
  DockPane(&m_stylesToolBar);
  DockPaneLeftOf(&m_layerPropertiesToolBar, &m_stylesToolBar);
  DockPaneLeftOf(&m_renderPropertiesToolBar, &m_layerPropertiesToolBar);
  DockPaneLeftOf(&m_standardToolBar, &m_renderPropertiesToolBar);
  DockPaneLeftOf(&m_menuBar, &m_standardToolBar);
  DockPane(&m_propertiesPane, AFX_IDW_DOCKBAR_LEFT);
  DockPane(&m_outputPane, AFX_IDW_DOCKBAR_BOTTOM);

  RecalcLayout();

  EnsureToolbarsVisible();

  ApplyColorScheme();

  // ApplyColorScheme calls LoadBitmap which can reset button sizes.
  // Re-apply combo-derived sizes after all bitmap loading is complete.
  AdjustToolbarSizesToMatchCombos();

  EnableAutoHidePanes(CBRS_ALIGN_ANY);

  EnableWindowsDialog(ID_WINDOW_MANAGER, IDS_WINDOWS_MANAGER, TRUE);

  // Enable automatic creation and management of the pop-up pane menu, which displays a list of application panes.
  EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, L"Customize...", ID_VIEW_TOOLBARS, FALSE, FALSE);
  EnableFullScreenMode(ID_VIEW_FULLSCREEN);
  EnableFullScreenMainMenu(TRUE);

  CMFCToolBar::EnableQuickCustomization();

  if (CMFCToolBar::GetUserImages() == nullptr) {
    // load user-defined toolbar images
    if (m_userImages.Load(L".\\res\\UserImages.bmp")) {
      m_userImages.SetImageSize(CSize(16, 16), FALSE);
      CMFCToolBar::SetUserImages(&m_userImages);
    }
  }
  // Shows the document name after thumbnail before the application name in a frame window title.
  ModifyStyle(0, FWS_PREFIXTITLE);

  // Explicitly load the best-resolution icon from the multi-size ICO (16/32/48/64/128/256 px).
  // MFC's default WNDCLASS registration uses LoadIcon which is capped at 32 px; LoadImage with
  // explicit metrics lets Windows pick the nearest larger size (e.g. 256 px for high-DPI taskbar).
  {
    const HINSTANCE resourceHandle = ::AfxGetResourceHandle();
    const auto largeIcon = static_cast<HICON>(::LoadImageW(resourceHandle,
        MAKEINTRESOURCE(IDR_MAINFRAME),
        IMAGE_ICON,
        ::GetSystemMetrics(SM_CXICON),
        ::GetSystemMetrics(SM_CYICON),
        LR_DEFAULTCOLOR));
    const auto smallIcon = static_cast<HICON>(::LoadImageW(resourceHandle,
        MAKEINTRESOURCE(IDR_MAINFRAME),
        IMAGE_ICON,
        ::GetSystemMetrics(SM_CXSMICON),
        ::GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR));
    SetIcon(largeIcon, TRUE);
    SetIcon(smallIcon, FALSE);
  }

  // Start the named-pipe automation server so Python scripts can drive the CLI
  // without a pywin32 DDE dependency.
  m_pipeServer.Start(GetSafeHwnd());

  return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
  if (!CMDIFrameWndEx::PreCreateWindow(cs)) { return FALSE; }

  return TRUE;
}

BOOL CMainFrame::CreateDockablePanes() {
  const CSize defaultSize(200, 200);

  constexpr DWORD sharedStyles(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_FLOAT_MULTI);

  auto windowName = App::LoadStringResource(IDS_OUTPUT);
  if (!m_outputPane.Create(windowName, this, defaultSize, TRUE, ID_VIEW_OUTPUTWND, sharedStyles | CBRS_BOTTOM)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create Output pane\n");
    return FALSE;
  }
  windowName = App::LoadStringResource(IDS_PROPERTIES);
  if (!m_propertiesPane.Create(windowName, this, defaultSize, TRUE, ID_VIEW_PROPERTIESWND, sharedStyles | CBRS_RIGHT)) {
    ATLTRACE2(traceGeneral, 3, L"Failed to create Properties pane\n");
    return FALSE;
  }
  SetDockablePanesIcons();
  return TRUE;
}

void CMainFrame::SetDockablePanesIcons() {
  const CSize smallIconSize(::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
  const HINSTANCE resourceHandle(::AfxGetResourceHandle());

  const auto propertiesPaneIcon = static_cast<HICON>(LoadImageW(
      resourceHandle, MAKEINTRESOURCE(IDI_PROPERTIES_WND_HC), IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_propertiesPane.SetIcon(propertiesPaneIcon, FALSE);

  const auto outputPaneIcon = static_cast<HICON>(LoadImageW(
      resourceHandle, MAKEINTRESOURCE(IDI_OUTPUT_WND_HC), IMAGE_ICON, smallIconSize.cx, smallIconSize.cy, 0));
  m_outputPane.SetIcon(outputPaneIcon, FALSE);

  UpdateMDITabbedBarsIcons();
}

void CMainFrame::OnWindowManager() {
  ShowWindowsDialog();
}

void CMainFrame::OnViewCustomize() {
  auto* dialog = new CMFCToolBarsCustomizeDialog(this, TRUE);
  dialog->EnableUserDefinedToolbars();
  dialog->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp, LPARAM name) {
  const auto result = CMDIFrameWndEx::OnToolbarCreateNew(wp, name);
  if (result == 0) { return 0; }
  auto* userToolbar = (CMFCToolBar*)result;
  assert(userToolbar != nullptr);

  const auto customize = App::LoadStringResource(IDS_TOOLBAR_CUSTOMIZE);

  userToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, customize);
  return result;
}

LRESULT CMainFrame::OnToolbarReset(WPARAM toolbarResourceId, LPARAM lparam) {
  ATLTRACE2(traceGeneral, 3, L"CMainFrame::OnToolbarReset(%i, %i)\n", toolbarResourceId, lparam);

  switch (toolbarResourceId) {
    case IDR_RENDER_PROPERTIES: {
      m_renderPropertiesToolBar.ReplaceButton(ID_PENCOLOR_COMBO, EoCtrlColorComboBox(), FALSE);
      m_renderPropertiesToolBar.ReplaceButton(ID_LINETYPE_COMBO, EoCtrlLineTypeComboBox(), FALSE);
      m_renderPropertiesToolBar.ReplaceButton(ID_LINEWEIGHT_COMBO, EoCtrlLineWeightComboBox(), FALSE);
      break;
    }
    case IDR_STYLES: {
      m_stylesToolBar.ReplaceButton(ID_TEXTSTYLE_COMBO, EoCtrlTextStyleComboBox(), FALSE);
      break;
    }
    case IDR_LAYER_PROPERTIES: {
      m_layerPropertiesToolBar.ReplaceButton(ID_LAYER_COMBO, EoCtrlLayerComboBox(), FALSE);
      break;
    }
    case IDR_PROPERTIES:
      break;
  }
  return 0;
}
BOOL CMainFrame::LoadFrame(UINT resourceId, DWORD defaultStyle, CWnd* parentWindow, CCreateContext* createContext) {
  if (!CMDIFrameWndEx::LoadFrame(resourceId, defaultStyle, parentWindow, createContext)) { return FALSE; }

  // Add some tools for example....
  auto* userToolsManager = app.GetUserToolsManager();
  if (userToolsManager != nullptr && userToolsManager->GetUserTools().IsEmpty()) {
    auto* tool1 = userToolsManager->CreateNewTool();
    tool1->m_strLabel = L"&Notepad";
    tool1->SetCommand(L"notepad.exe");

    auto* tool2 = userToolsManager->CreateNewTool();
    tool2->m_strLabel = L"Paint &Brush";
    tool2->SetCommand(L"mspaint.exe");

    auto* tool3 = userToolsManager->CreateNewTool();
    tool3->m_strLabel = L"&File Explorer";
    tool3->SetCommand(L"explorer.exe");

    auto* tool4 = userToolsManager->CreateNewTool();
    tool4->m_strLabel = L"Fanning, Fanning & Associates On-Line";
    tool4->SetCommand(L"http://www.fanningfanning.com");
  }

  // Enable customization button for user toolbars (standard and render properties are locked)
  const auto customize = App::LoadStringResource(IDS_TOOLBAR_CUSTOMIZE);
  for (int i = 0; i < maxUserToolbars; i++) {
    auto* userToolbar = GetUserToolBarByIndex(i);
    if (userToolbar != nullptr) { userToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, customize); }
  }

  // Re-apply toolbar sizing after CDockingManager::LoadState restores docking layout
  // from the registry. LoadState (called from CMDIFrameWndEx::LoadFrame → LoadMDIState)
  // restores toolbar HWND positions and sizes from a previous session, overriding the
  // SetSizes values applied in OnCreate. EnsureToolbarsVisible catches toolbars hidden
  // by stale registry state. RecalcLayout forces the docking manager to re-query each
  // toolbar's preferred height (based on the still-valid button sizes from OnCreate).
  EnsureToolbarsVisible();
  AdjustToolbarSizesToMatchCombos();

  return TRUE;
}
LRESULT CMainFrame::OnToolbarContextMenu(WPARAM, LPARAM point_) {
  CMenu popupToolbarMenu;
  VERIFY(popupToolbarMenu.LoadMenu(IDR_POPUP_TOOLBAR));

  auto* subMenu = popupToolbarMenu.GetSubMenu(0);
  assert(subMenu != nullptr);

  if (subMenu) {
    const CPoint point(AFX_GET_X_LPARAM(point_), AFX_GET_Y_LPARAM(point_));

    auto* popupMenu = new CMFCPopupMenu;
    popupMenu->Create(this, point.x, point.y, subMenu->Detach());
  }
  return 0;
}

BOOL CMainFrame::OnShowPopupMenu(CMFCPopupMenu* pMenuPopup) {
  CMDIFrameWndEx::OnShowPopupMenu(pMenuPopup);

  if (pMenuPopup != nullptr && pMenuPopup->GetMenuBar()->CommandToIndex(ID_VIEW_TOOLBARS) >= 0) {
    if (CMFCToolBar::IsCustomizeMode()) {
      // Don't show toolbars list in the cuztomization mode!
      return FALSE;
    }
    pMenuPopup->RemoveAllItems();

    CMenu menu;
    VERIFY(menu.LoadMenu(IDR_POPUP_TOOLBAR));

    const auto* const popupSubMenu = menu.GetSubMenu(0);
    assert(popupSubMenu != nullptr);

    if (popupSubMenu) { pMenuPopup->GetMenuBar()->ImportFromMenu(*popupSubMenu, TRUE); }
  }
  return TRUE;
}

void CMainFrame::UpdateMDITabs(BOOL resetMDIChild) {
  CList<UINT, UINT> lstCommands;
  if (AreMDITabs(nullptr)) {
    lstCommands.AddTail(ID_WINDOW_ARRANGE);
    lstCommands.AddTail(ID_WINDOW_CASCADE);
    lstCommands.AddTail(ID_WINDOW_TILE_HORZ);
    lstCommands.AddTail(ID_WINDOW_TILE_VERT);
  }
  CMFCToolBar::SetNonPermittedCommands(lstCommands);
  if (resetMDIChild) {
    HWND hwndT = ::GetWindow(m_hWndMDIClient, GW_CHILD);
    while (hwndT != nullptr) {
      auto* frame = DYNAMIC_DOWNCAST(CMDIChildWndEx, CWnd::FromHandle(hwndT));
      if (frame != nullptr) { frame->ModifyStyle(WS_SYSMENU, 0); }
      hwndT = ::GetWindow(hwndT, GW_HWNDNEXT);
    }
    m_menuBar.SetMaximizeMode(FALSE);
  }
  if (m_propertiesPane.IsAutoHideMode()) {
    m_propertiesPane.BringWindowToTop();
    auto* divider = m_propertiesPane.GetDefaultPaneDivider();
    if (divider != nullptr) { divider->BringWindowToTop(); }
  }
  CMDIFrameWndEx::m_bDisableSetRedraw = TRUE;

  RecalcLayout();
  RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

BOOL CMainFrame::OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL drop) {
  if (drop) { return FALSE; }
  CMenu menu;
  VERIFY(menu.LoadMenu(IDR_POPUP_MDITABS));

  auto* popupSubMenu = menu.GetSubMenu(0);
  assert(popupSubMenu != nullptr);

  if (popupSubMenu) {
    if ((dwAllowedItems & AFX_MDI_CAN_BE_DOCKED) == 0) { popupSubMenu->DeleteMenu(ID_MDI_TABBED, MF_BYCOMMAND); }
    auto* popupMenu = new CMFCPopupMenu;
    if (popupMenu) {
      popupMenu->SetAutoDestroy(FALSE);
      popupMenu->Create(this, point.x, point.y, popupSubMenu->GetSafeHmenu());
    }
  }
  return TRUE;
}

LRESULT CMainFrame::OnGetTabToolTip(WPARAM /*wp*/, LPARAM lp) {
  auto* toolTipInfo = (CMFCTabToolTipInfo*)lp;
  assert(toolTipInfo != nullptr);

  if (toolTipInfo) {
    assert(toolTipInfo->m_pTabWnd != nullptr);
    if (!toolTipInfo->m_pTabWnd->IsMDITab()) { return 0; }
    toolTipInfo->m_strText.Format(L"Tab #%d Custom Tooltip", toolTipInfo->m_nTabIndex + 1);
  }
  return 0;
}

void CMainFrame::OnMdiTabbed() {
  auto* pMDIChild = DYNAMIC_DOWNCAST(CMDIChildWndEx, MDIGetActive());
  if (pMDIChild == nullptr) {
    assert(FALSE);
    return;
  }
  TabbedDocumentToControlBar(pMDIChild);
}

void CMainFrame::OnUpdateMdiTabbed(CCmdUI* pCmdUI) {
  pCmdUI->SetCheck();
}
void CMainFrame::OnDestroy() {
  ATLTRACE2(traceGeneral, 3, L"CMainFrame::OnDestroy() - Entering\n");
  m_pipeServer.Stop();
  PostQuitMessage(0);  // Force WM_QUIT message to terminate message loop
}
CString CMainFrame::GetPaneText(int index) {
  return m_statusBar.GetPaneText(index);
}

void CMainFrame::SetPaneInfo(int index, UINT newId, UINT style, int width) {
  m_statusBar.SetPaneInfo(index, newId, style, width);
}

BOOL CMainFrame::SetPaneText(int index, const wchar_t* newText) {
  return m_statusBar.SetPaneText(index, newText);
}

void CMainFrame::SetPaneStyle(int index, UINT style) {
  m_statusBar.SetPaneStyle(index, style);
}

void CMainFrame::SetPaneTextColor(int index, COLORREF textColor) {
  m_statusBar.SetPaneTextColor(index, textColor);
}

void CMainFrame::SetPaneBackgroundColor(int index, COLORREF backgroundColor) {
  m_statusBar.SetPaneBackgroundColor(index, backgroundColor);
}

void CMainFrame::OnViewFullScreen() {
  ShowFullScreen();
}

void CMainFrame::EnsureToolbarsVisible() {
  // Safety net: after LoadMDIState() restores docking state from the registry,
  // verify that all application toolbars are visible. A stale or corrupted blob
  // (e.g. from a DPI change or structural toolbar change) can leave toolbars hidden.
  CMFCToolBar* toolbars[] = {
      &m_standardToolBar, &m_renderPropertiesToolBar, &m_layerPropertiesToolBar, &m_stylesToolBar};
  for (auto* toolbar : toolbars) {
    if (!toolbar->IsVisible()) {
      toolbar->ShowPane(TRUE, FALSE, TRUE);
      CString name;
      toolbar->GetWindowText(name);
      ATLTRACE2(
          traceGeneral, 3, L"EnsureToolbarsVisible: forced toolbar '%s' visible\n", static_cast<const wchar_t*>(name));
    }
  }
}
void CMainFrame::AdjustToolbarSizesToMatchCombos() {
  // Measure the actual combo HWND closed height and set all toolbar button sizes
  // to match. This ensures icon-only and combo-containing toolbars report identical
  // row heights to the docking manager. Must be called after any operation that can
  // reset m_sizeButton (LoadBitmap, LoadToolBar, docking state restore).
  const CSize kImageSize(24, 24);
  CObList buttons;
  if (CMFCToolBar::GetCommandButtons(ID_PENCOLOR_COMBO, buttons) > 0) {
    const auto* comboButton = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, buttons.GetHead());
    if (comboButton != nullptr) {
      const auto* combo = comboButton->GetComboBox();
      if (combo != nullptr && combo->GetSafeHwnd() != nullptr) {
        CRect comboRect;
        combo->GetWindowRect(&comboRect);
        constexpr int comboVertMargin = 4;  // CMFCToolBarComboBoxButton::m_nVertMargin
        const int targetHeight = std::max(32, static_cast<int>(comboRect.Height()) + 2 * comboVertMargin);
        const CSize adjustedSize(std::max(32, targetHeight), targetHeight);
        ATLTRACE2(traceGeneral,
            3,
            L"AdjustToolbarSizesToMatchCombos: combo=%d, button=%dx%d\n",
            comboRect.Height(),
            adjustedSize.cx,
            adjustedSize.cy);
        EoMfStatelessToolBar::SetSizes(adjustedSize, kImageSize);
        m_standardToolBar.SetLockedSizes(adjustedSize, kImageSize, TRUE);
        m_renderPropertiesToolBar.SetSizesAll(adjustedSize, kImageSize);
        m_layerPropertiesToolBar.SetSizesAll(adjustedSize, kImageSize);
        m_stylesToolBar.SetSizesAll(adjustedSize, kImageSize);
        RecalcLayout();
      }
    }
  }
}
void CMainFrame::ApplyColorScheme() {
  auto* visualManager = dynamic_cast<EoMfVisualManager*>(CMFCVisualManager::GetInstance());
  if (visualManager != nullptr) { visualManager->RefreshColors(); }

  // Set status bar text color for all panes
  const int paneCount = m_statusBar.GetCount();
  for (int i = 0; i < paneCount; i++) { m_statusBar.SetPaneTextColor(i, Eo::chromeColors.statusBarText, FALSE); }

  m_propertiesPane.ApplyColorScheme();
  m_outputPane.ApplyColorScheme();

  // LoadBitmap can reset button sizes. Re-apply combo-derived sizes.
  AdjustToolbarSizesToMatchCombos();

  RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}

void CMainFrame::OnUpdatePenColorCombo(CCmdUI* pCmdUI) {
  pCmdUI->Enable(TRUE);
}

void CMainFrame::SyncColorCombo(std::int16_t aciIndex) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_PENCOLOR_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlColorComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentColor(aciIndex);
        break;
      }
    }
  }
}

void CMainFrame::OnUpdateLineTypeCombo(CCmdUI* pCmdUI) {
  pCmdUI->Enable(TRUE);
}

void CMainFrame::SyncLineTypeCombo(std::int16_t lineTypeIndex, const std::wstring& lineTypeName) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_LINETYPE_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlLineTypeComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentLineType(lineTypeIndex, lineTypeName);
        break;
      }
    }
  }
}

void CMainFrame::OnUpdateLineWeightCombo(CCmdUI* pCmdUI) {
  pCmdUI->Enable(TRUE);
}

void CMainFrame::SyncLineWeightCombo(EoDxfLineWeights::LineWeight lineWeight) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_LINEWEIGHT_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlLineWeightComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentLineWeight(lineWeight);
        break;
      }
    }
  }
}

void CMainFrame::OnUpdateTextStyleCombo(CCmdUI* pCmdUI) {
  pCmdUI->Enable(TRUE);
}

void CMainFrame::SyncTextStyleCombo(const std::wstring& textStyleName) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_TEXTSTYLE_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlTextStyleComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentTextStyle(textStyleName);
        break;
      }
    }
  }
}

void CMainFrame::OnUpdateLayerCombo(CCmdUI* pCmdUI) {
  pCmdUI->Enable(TRUE);
}

void CMainFrame::SyncLayerCombo(const CString& layerName) {
  CObList buttonsList;
  if (CMFCToolBar::GetCommandButtons(ID_LAYER_COMBO, buttonsList) > 0) {
    for (auto pos = buttonsList.GetHeadPosition(); pos != nullptr;) {
      auto* button = DYNAMIC_DOWNCAST(EoCtrlLayerComboBox, buttonsList.GetNext(pos));
      if (button != nullptr) {
        button->SetCurrentLayer(layerName);
        break;
      }
    }
  }
}

/// @brief UI-thread handler for WM_APP_PIPE_COMMAND messages sent by EoNamedPipeServer.
/// Dispatches the command through the CLI pipeline and fills ctx->responseUtf8
/// with a complete EoPipe protocol response line so the server thread can relay
/// it verbatim to the Python client.
LRESULT CMainFrame::OnPipeCommand(WPARAM, LPARAM lp) {
  auto* ctx = reinterpret_cast<EoNamedPipeServer::CommandContext*>(lp);
  if (!ctx) { return 0; }

  // Validate pre-conditions that the server thread cannot check.
  if (ctx->commandLine.empty()) {
    ctx->responseUtf8 = EoPipe::ResponseOk();
    return 0;
  }

  if (!IsWindow(m_hWnd)) {
    ctx->responseUtf8 = EoPipe::ResponseError("INTERNAL", "main window destroyed");
    return 0;
  }

  // Route QUERY verb to the structured-response handler before falling through
  // to the fire-and-forget CLI pipeline.
  static const std::wstring kQueryPrefix = L"QUERY";
  if (ctx->commandLine.size() >= kQueryPrefix.size()) {
    std::wstring upper = ctx->commandLine.substr(0, kQueryPrefix.size());
    for (auto& c : upper) { c = towupper(c); }
    if (upper == kQueryPrefix) {
      std::wstring args;
      if (ctx->commandLine.size() > kQueryPrefix.size() + 1) {
        args = ctx->commandLine.substr(kQueryPrefix.size() + 1);
      }
      if (HandleQueryCommand(ctx, args)) { return 0; }
    }
  }

  m_outputPane.ExecuteCommandLine(ctx->commandLine);

  // ExecuteCommandLine is currently fire-and-forget: it dispatches through
  // EoCommandRegistry which validates the verb before sending WM_COMMAND.
  // Errors detectable at dispatch time are reported via AppendHistory but
  // not yet returned as structured results.  Leave the response as OK here;
  // a future iteration will route EoCommandRegistry::ExecuteResult back
  // through this context when argFunctor-based commands support it.
  ctx->responseUtf8 = EoPipe::ResponseOk();

  return 0;
}

/// @brief Handles QUERY verb pipe requests and returns JSON via ctx->responseUtf8.
///
/// Supported sub-commands:
///   QUERY TRAP   — returns groups count, total primitive count, aggregate bounding box,
///                  bounding-box area (XY plane), and per-group primitive counts + layer names.
///
/// All geometry is computed in world space using an identity transform so the numbers
/// match what the user sees in the drawing.  ptMin/ptMax are initialised to
/// Eo::boundsMax/Eo::boundsMin sentinels so that un-trapped queries yield zero area cleanly.
bool CMainFrame::HandleQueryCommand(EoNamedPipeServer::CommandContext* ctx, const std::wstring& args) {
  // Trim leading whitespace from args.
  std::wstring verb = args;
  auto firstNonSpace = verb.find_first_not_of(L" \t");
  if (firstNonSpace != std::wstring::npos) { verb = verb.substr(firstNonSpace); }
  for (auto& c : verb) { c = towupper(c); }

  // ---- QUERY TRAP --------------------------------------------------------
  static const std::wstring kTrap = L"TRAP";
  if (verb == kTrap || verb.substr(0, kTrap.size()) == kTrap) {
    auto* doc = AeSysDoc::GetDoc();
    if (doc == nullptr) {
      ctx->responseUtf8 = EoPipe::ResponseError("NO_DOC", "no active document");
      return true;
    }

    const auto groupCount = static_cast<int>(doc->TrapGroupCount());

    // Aggregate bounding box and total primitive count across all trapped groups.
    EoGePoint3d ptMin(Eo::boundsMax, Eo::boundsMax, Eo::boundsMax);
    EoGePoint3d ptMax(Eo::boundsMin, Eo::boundsMin, Eo::boundsMin);
    int totalPrimitives = 0;

    auto* view = AeSysView::GetActiveView();
    EoGeTransformMatrix identity;  // default-constructed identity matrix

    auto position = doc->GetFirstTrappedGroupPosition();
    while (position != nullptr) {
      auto* group = doc->GetNextTrappedGroup(position);
      if (group == nullptr) { continue; }
      totalPrimitives += static_cast<int>(group->GetCount());
      group->GetExtents(view, ptMin, ptMax, identity);
    }

    // Compute bounding-box area on the XY plane.
    double bboxArea = 0.0;
    double bboxWidth = 0.0;
    double bboxHeight = 0.0;
    bool hasGeometry = (groupCount > 0 && ptMin.x <= ptMax.x && ptMin.y <= ptMax.y);
    if (hasGeometry) {
      bboxWidth = ptMax.x - ptMin.x;
      bboxHeight = ptMax.y - ptMin.y;
      bboxArea = bboxWidth * bboxHeight;
    }

    // Build the per-group JSON array: [{"primitives":n,"layer":"name"}, ...]
    // We keep it compact — no spaces after colons/commas.
    std::string groupsJson = "[";
    position = doc->GetFirstTrappedGroupPosition();
    bool firstGroup = true;
    while (position != nullptr) {
      auto* group = doc->GetNextTrappedGroup(position);
      if (group == nullptr) { continue; }
      if (!firstGroup) { groupsJson += ','; }
      firstGroup = false;

      // Collect unique layer names from the group's primitives.
      std::wstring layerName;
      auto primPos = group->GetHeadPosition();
      if (primPos != nullptr) {
        auto* prim = group->GetNext(primPos);
        if (prim != nullptr) { layerName = prim->LayerName(); }
      }

      // Encode layer name as UTF-8, escaping backslash and double-quote.
      std::string layerUtf8;
      for (wchar_t wc : layerName) {
        if (wc == L'\\' || wc == L'"') { layerUtf8 += '\\'; }
        layerUtf8 += static_cast<char>(wc & 0xFF);
      }

      char buf[256];
      _snprintf_s(buf,
          _TRUNCATE,
          "{\"primitives\":%d,\"layer\":\"%s\"}",
          static_cast<int>(group->GetCount()),
          layerUtf8.c_str());
      groupsJson += buf;
    }
    groupsJson += ']';

    // Assemble the top-level JSON object.
    char json[1024];
    _snprintf_s(json,
        _TRUNCATE,
        "{\"groups\":%d,\"primitives\":%d,"
        "\"bbox\":{\"min_x\":%.6f,\"min_y\":%.6f,\"max_x\":%.6f,\"max_y\":%.6f},"
        "\"bbox_width\":%.6f,\"bbox_height\":%.6f,\"bbox_area\":%.6f,"
        "\"items\":%s}",
        groupCount,
        totalPrimitives,
        hasGeometry ? ptMin.x : 0.0,
        hasGeometry ? ptMin.y : 0.0,
        hasGeometry ? ptMax.x : 0.0,
        hasGeometry ? ptMax.y : 0.0,
        bboxWidth,
        bboxHeight,
        bboxArea,
        groupsJson.c_str());

    ctx->responseUtf8 = EoPipe::ResponseOkValue(json);
    return true;
  }

  // ---- QUERY BLOCKS [layer] — batch block-reference enumeration ---------
  // Must be checked before QUERY BLOCK AT to avoid prefix-match collision.
  static const std::wstring kBlocks = L"BLOCKS";
  {
    const bool isExact = (verb == kBlocks);
    const bool hasArg =
        (verb.size() > kBlocks.size() && verb[kBlocks.size()] == L' ' && verb.substr(0, kBlocks.size()) == kBlocks);
    if (isExact || hasArg) {
      auto* doc = AeSysDoc::GetDoc();
      if (doc == nullptr) {
        ctx->responseUtf8 = EoPipe::ResponseError("NO_DOC", "no active document");
        return true;
      }
      // Optional layer name after "BLOCKS " — trim surrounding whitespace.
      const std::wstring layerFilter = hasArg ? TrimWs(verb.substr(kBlocks.size() + 1)) : L"";
      const auto blocks = QueryAllBlocks(doc, layerFilter);

      std::string json = "{\"count\":";
      json += std::to_string(static_cast<int>(blocks.size()));
      json += ",\"blocks\":[";
      bool first = true;
      for (const auto& b : blocks) {
        if (!first) { json += ','; }
        first = false;
        // Build fixed numeric fields into a small stack buffer; append the variable-length attribsJson directly into
        // the std::string to avoid any size cap on the combined entry.
        char numBuf[256];
        _snprintf_s(numBuf,
            _TRUNCATE,
            "{\"name\":\"%s\",\"layer\":\"%s\","
            "\"x\":%.6f,\"y\":%.6f,\"z\":%.6f,"
            "\"rotation\":%.6f,\"scale_x\":%.6f,\"scale_y\":%.6f,\"scale_z\":%.6f,"
            "\"attributes\":",
            b.name.c_str(),
            b.layer.c_str(),
            b.x,
            b.y,
            b.z,
            b.rotation,
            b.scaleX,
            b.scaleY,
            b.scaleZ);
        json += numBuf;
        json += b.attribsJson;
        json += '}';
      }
      json += "]}";  // close blocks array and root object
      ctx->responseUtf8 = EoPipe::ResponseOkValue(json);
      return true;
    }
  }

  // ---- QUERY BLOCK AT x,y[,z] ----------------------------------------
  static const std::wstring kBlock = L"BLOCK";
  // Require a space after BLOCK so this branch never fires for "BLOCKS".
  if (verb.size() > kBlock.size() && verb[kBlock.size()] == L' ' && verb.substr(0, kBlock.size()) == kBlock) {
    auto* doc = AeSysDoc::GetDoc();
    if (doc == nullptr) {
      ctx->responseUtf8 = EoPipe::ResponseError("NO_DOC", "no active document");
      return true;
    }
    // Parse coordinates from "BLOCK AT x,y[,z]"
    std::wstring rest = (verb.size() > kBlock.size() + 1) ? verb.substr(kBlock.size() + 1) : L"";
    // Strip optional "AT " prefix.
    static const std::wstring kAt = L"AT ";
    if (rest.size() >= kAt.size() && rest.substr(0, kAt.size()) == kAt) { rest = rest.substr(kAt.size()); }

    double qx = 0.0, qy = 0.0, qz = 0.0;
    const int parsed = swscanf_s(rest.c_str(), L"%lf,%lf,%lf", &qx, &qy, &qz);
    if (parsed < 2) {
      ctx->responseUtf8 = EoPipe::ResponseError("BAD_ARGS", "expected: QUERY BLOCK AT x,y[,z]");
      return true;
    }

    // Find the nearest block reference whose insertion point is within
    // a small world-space tolerance of the query point.  Using insertion-
    // point distance avoids view-dependent aperture issues with
    // SelectGroupAndPrimitive.
    constexpr double kBlockPickTol = 0.5;  // world-space units
    EoGePoint3d queryPoint(qx, qy, qz);
    EoDbBlockReference* ref = nullptr;
    double bestDist = kBlockPickTol;
    const CLayers& blkLayers = doc->SpaceLayers(EoDxf::Space::ModelSpace);
    for (INT_PTR li = 0, ln = blkLayers.GetSize(); li < ln; ++li) {
      EoDbLayer* layer = blkLayers[li];
      if (layer == nullptr) { continue; }
      auto groupPos = layer->GetHeadPosition();
      while (groupPos != nullptr) {
        EoDbGroup* grp = layer->GetNext(groupPos);
        if (grp == nullptr) { continue; }
        auto primPos = grp->GetHeadPosition();
        while (primPos != nullptr) {
          EoDbPrimitive* prim = grp->GetNext(primPos);
          if (prim == nullptr || !prim->Is(EoDb::kGroupReferencePrimitive)) { continue; }
          auto* candidate = static_cast<EoDbBlockReference*>(prim);
          const double dist = queryPoint.DistanceTo(candidate->InsertionPoint());
          if (dist < bestDist) {
            bestDist = dist;
            ref = candidate;
          }
        }
      }
    }
    if (ref == nullptr) {
      ctx->responseUtf8 = EoPipe::ResponseError("NO_BLOCK", "no block reference at the specified point");
      return true;
    }

    // Helper: encode a wide string as JSON-safe UTF-8 bytes (Latin-1 plane).
    auto encodeJson = [](const CString& wide) -> std::string {
      std::string out;
      for (int i = 0; i < wide.GetLength(); ++i) {
        wchar_t wc = wide[i];
        if (wc == L'\\') {
          out += "\\\\";
        } else if (wc == L'"') {
          out += "\\\"";
        } else {
          out += static_cast<char>(wc & 0xFF);
        }
      }
      return out;
    };

    // Build the attributes JSON array.
    std::string attribsJson = "[";
    bool firstAttrib = true;
    for (const std::uint64_t attribHandle : ref->AttributeHandles()) {
      auto* attribPrim = doc->FindPrimitiveByHandle(attribHandle);
      if (attribPrim == nullptr || !attribPrim->Is(EoDb::kAttribPrimitive)) { continue; }
      auto* attrib = static_cast<EoDbAttrib*>(attribPrim);

      // Encode tag (wstring) and value (CString).
      std::string tagUtf8;
      for (wchar_t wc : attrib->TagString()) {
        if (wc == L'\\') {
          tagUtf8 += "\\\\";
        } else if (wc == L'"') {
          tagUtf8 += "\\\"";
        } else {
          tagUtf8 += static_cast<char>(wc & 0xFF);
        }
      }
      const std::string valueUtf8 = encodeJson(attrib->Text());

      if (!firstAttrib) { attribsJson += ','; }
      firstAttrib = false;

      char attribBuf[512];
      _snprintf_s(attribBuf, _TRUNCATE, "{\"tag\":\"%s\",\"value\":\"%s\"}", tagUtf8.c_str(), valueUtf8.c_str());
      attribsJson += attribBuf;
    }
    attribsJson += ']';

    const EoGePoint3d insertPt = ref->InsertionPoint();
    const EoGeVector3d scale = ref->ScaleFactors();
    const double rotationDeg = ref->Rotation() * (180.0 / Eo::Pi);
    const std::string nameUtf8 = encodeJson(ref->BlockName());
    const std::string layerUtf8 = encodeJson(CString(ref->LayerName().c_str()));

    char json[2048];
    _snprintf_s(json,
        _TRUNCATE,
        "{\"name\":\"%s\",\"layer\":\"%s\","
        "\"x\":%.6f,\"y\":%.6f,\"z\":%.6f,"
        "\"rotation\":%.6f,\"scale_x\":%.6f,\"scale_y\":%.6f,\"scale_z\":%.6f,"
        "\"attributes\":%s}",
        nameUtf8.c_str(),
        layerUtf8.c_str(),
        insertPt.x,
        insertPt.y,
        insertPt.z,
        rotationDeg,
        scale.x,
        scale.y,
        scale.z,
        attribsJson.c_str());

    ctx->responseUtf8 = EoPipe::ResponseOkValue(json);
    return true;
  }

  // ---- QUERY ROOMS [layer] — batch room enumeration -------------------
  // Must be checked before QUERY ROOM x,y to avoid prefix-match collision.
  static const std::wstring kRooms = L"ROOMS";
  {
    const bool isExact = (verb == kRooms);
    const bool hasArg =
        (verb.size() > kRooms.size() && verb[kRooms.size()] == L' ' && verb.substr(0, kRooms.size()) == kRooms);
    if (isExact || hasArg) {
      auto* doc = AeSysDoc::GetDoc();
      if (doc == nullptr) {
        ctx->responseUtf8 = EoPipe::ResponseError("NO_DOC", "no active document");
        return true;
      }
      // Optional layer filter after "ROOMS " — trim surrounding whitespace.
      const std::wstring layerFilter = hasArg ? TrimWs(verb.substr(kRooms.size() + 1)) : L"";
      const auto rooms = QueryAllRooms(doc, layerFilter);

      std::string json = "{\"count\":";
      json += std::to_string(static_cast<int>(rooms.size()));
      json += ",\"rooms\":[";
      bool first = true;
      for (const auto& r : rooms) {
        if (!first) { json += ','; }
        first = false;
        char buf[512];
        _snprintf_s(buf,
            _TRUNCATE,
            "{\"area\":%.6f,\"perimeter\":%.6f,"
            "\"centroid_x\":%.6f,\"centroid_y\":%.6f,"
            "\"vertex_count\":%d,\"layer\":\"%s\"}",
            r.area,
            r.perimeter,
            r.centroidX,
            r.centroidY,
            r.vertexCount,
            r.layerUtf8.c_str());
        json += buf;
      }
      json += "]}";  // close rooms array and root object
      ctx->responseUtf8 = EoPipe::ResponseOkValue(json);
      return true;
    }
  }

  // ---- QUERY ROOM x,y[,z] --------------------------------------------
  static const std::wstring kRoom = L"ROOM";
  // Require a space after ROOM so this branch never fires for "ROOMS".
  if (verb.size() > kRoom.size() && verb[kRoom.size()] == L' ' && verb.substr(0, kRoom.size()) == kRoom) {
    auto* doc = AeSysDoc::GetDoc();
    if (doc == nullptr) {
      ctx->responseUtf8 = EoPipe::ResponseError("NO_DOC", "no active document");
      return true;
    }

    // Parse coordinates: "ROOM x,y[,z]"
    std::wstring coordStr = (verb.size() > kRoom.size() + 1) ? verb.substr(kRoom.size() + 1) : L"";
    // Trim leading whitespace.
    const auto firstNs = coordStr.find_first_not_of(L" \t");
    if (firstNs != std::wstring::npos) { coordStr = coordStr.substr(firstNs); }

    // Split on commas.
    double qx = 0.0, qy = 0.0;
    const auto comma1 = coordStr.find(L',');
    if (comma1 == std::wstring::npos) {
      ctx->responseUtf8 = EoPipe::ResponseError("BAD_ARGS", "usage: QUERY ROOM x,y");
      return true;
    }
    qx = _wtof(coordStr.substr(0, comma1).c_str());
    const std::wstring rest2 = coordStr.substr(comma1 + 1);
    const auto comma2 = rest2.find(L',');
    qy = _wtof(rest2.substr(0, (comma2 != std::wstring::npos) ? comma2 : rest2.size()).c_str());

    const RoomResult room = QueryRoomAtPoint(doc, qx, qy);

    if (!room.found) {
      ctx->responseUtf8 = EoPipe::ResponseError("NOT_FOUND", "no closed boundary contains the specified point");
      return true;
    }

    char json[512];
    _snprintf_s(json,
        _TRUNCATE,
        "{\"found\":true,\"area\":%.6f,\"perimeter\":%.6f,"
        "\"vertex_count\":%d,\"layer\":\"%s\"}",
        room.area,
        room.perimeter,
        room.vertexCount,
        room.layerUtf8.c_str());

    ctx->responseUtf8 = EoPipe::ResponseOkValue(json);
    return true;
  }

  // ---- QUERY LAYERS — enumerate all model-space layers ----------------
  // Exact match only — no arguments accepted.
  static const std::wstring kLayers = L"LAYERS";
  if (verb == kLayers
      || (verb.size() > kLayers.size() && verb[kLayers.size()] == L' ' && verb.substr(0, kLayers.size()) == kLayers)) {
    auto* doc = AeSysDoc::GetDoc();
    if (doc == nullptr) {
      ctx->responseUtf8 = EoPipe::ResponseError("NO_DOC", "no active document");
      return true;
    }
    const auto layerList = QueryAllLayersList(doc);

    std::string json = "{\"count\":";
    json += std::to_string(static_cast<int>(layerList.size()));
    json += ",\"layers\":[";
    bool first = true;
    for (const auto& l : layerList) {
      if (!first) { json += ','; }
      first = false;
      char buf[256];
      _snprintf_s(buf,
          _TRUNCATE,
          "{\"name\":\"%s\",\"visible\":%s,\"is_work\":%s,"
          "\"groups\":%d,\"primitives\":%d}",
          l.nameUtf8.c_str(),
          l.visible ? "true" : "false",
          l.isWork ? "true" : "false",
          l.groupCount,
          l.primitiveCount);
      json += buf;
    }
    json += "]}";  // close layers array and root object
    ctx->responseUtf8 = EoPipe::ResponseOkValue(json);
    return true;
  }

  // ---- QUERY LAYER name -----------------------------------------------
  static const std::wstring kLayer = L"LAYER";
  // Bare "LAYER" with no name: return BAD_ARGS immediately.
  if (verb == kLayer) {
    ctx->responseUtf8 = EoPipe::ResponseError("BAD_ARGS", "usage: QUERY LAYER <name>");
    return true;
  }
  // Require a space after LAYER so this branch never fires for "LAYERS".
  if (verb.size() > kLayer.size() && verb[kLayer.size()] == L' ' && verb.substr(0, kLayer.size()) == kLayer) {
    auto* doc = AeSysDoc::GetDoc();
    if (doc == nullptr) {
      ctx->responseUtf8 = EoPipe::ResponseError("NO_DOC", "no active document");
      return true;
    }
    auto* view = AeSysView::GetActiveView();

    // Trim leading and trailing whitespace from the layer name.
    std::wstring layerName = TrimWs(verb.substr(kLayer.size() + 1));
    if (layerName.empty()) {
      ctx->responseUtf8 = EoPipe::ResponseError("BAD_ARGS", "usage: QUERY LAYER <name>");
      return true;
    }

    const LayerResult lr = QueryLayerByName(doc, view, layerName);

    if (!lr.found) {
      ctx->responseUtf8 = EoPipe::ResponseError("NOT_FOUND", "no layer with that name in model space");
      return true;
    }

    // Encode the layer name safely for JSON.
    const std::string nameUtf8 = RoomEncodeJson(CString(layerName.c_str()));

    char json[512];
    _snprintf_s(json,
        _TRUNCATE,
        "{\"name\":\"%s\",\"visible\":%s,\"is_work\":%s,"
        "\"groups\":%d,\"primitives\":%d,"
        "\"extents\":{\"min_x\":%.6f,\"min_y\":%.6f,"
        "\"max_x\":%.6f,\"max_y\":%.6f}}",
        nameUtf8.c_str(),
        lr.visible ? "true" : "false",
        lr.isWork ? "true" : "false",
        lr.groupCount,
        lr.primitiveCount,
        lr.hasExtents ? lr.minX : 0.0,
        lr.hasExtents ? lr.minY : 0.0,
        lr.hasExtents ? lr.maxX : 0.0,
        lr.hasExtents ? lr.maxY : 0.0);

    ctx->responseUtf8 = EoPipe::ResponseOkValue(json);
    return true;
  }

  // Unknown QUERY sub-command.
  ctx->responseUtf8 = EoPipe::ResponseError("UNKNOWN_QUERY",
      "supported: QUERY TRAP, "
      "QUERY BLOCK AT x,y, QUERY BLOCKS [layer], "
      "QUERY ROOM x,y, QUERY ROOMS [layer], "
      "QUERY LAYER name, QUERY LAYERS");
  return true;
}
void CMainFrame::FocusCommandLine() {
  // Make sure the Output pane is visible before activating the Command tab.
  ShowPane(&m_outputPane, TRUE, FALSE, TRUE);
  m_outputPane.FocusCommandLine();
  SetCommandLineActive(true);
}
void CMainFrame::ExecuteCommandLine(const std::wstring& commandLine) {
  m_outputPane.ExecuteCommandLine(commandLine);
}

void CMainFrame::SetCommandLineActive(bool active) {
  if (active) {
    // Soft highlight: menuHighlightBackground is less visually loud than captionActive,
    // correctly conveying "mode ops suspended while typing" rather than "you are editing."
    m_statusBar.SetPaneBackgroundColor(statusCmd, Eo::chromeColors.menuHighlightBackground);
    m_statusBar.SetPaneTextColor(statusCmd, Eo::chromeColors.statusBarText);
    m_statusBar.SetPaneText(statusCmd, L"CMD");
  } else {
    m_statusBar.SetPaneBackgroundColor(statusCmd);  // reset to default
    m_statusBar.SetPaneTextColor(statusCmd, Eo::chromeColors.statusBarText);
    // Restore the mode name so the pane always shows the current context.
    UpdateCmdPane();
  }
}

void CMainFrame::UpdateCmdPane(const AeSysView* hint) {
  // Display the current mode name (e.g. "Draw", "Trap", "Edit") in the CMD pane.
  // hint is used when the view is not yet the MDI active child (e.g. during OnInitialUpdate).
  const AeSysView* view = AeSysView::GetActiveView();
  if (view == nullptr) { view = hint; }
  CString modeLabel;
  if (view != nullptr) {
    if (const auto* state = view->GetCurrentState(); state != nullptr) {
      const wchar_t* label = state->ModeLabel();
      if (label != nullptr && *label != L'\0') { modeLabel = label; }
    }
  }
  m_statusBar.SetPaneText(statusCmd, modeLabel);
  m_statusBar.SetPaneBackgroundColor(statusCmd);
  m_statusBar.SetPaneTextColor(statusCmd, Eo::chromeColors.statusBarText);
}
