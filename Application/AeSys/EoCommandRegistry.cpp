#include "Stdafx.h"

#include "EoCommandRegistry.h"

#include <algorithm>

#include "AeSys.h"
#include "AeSysView.h"
#include "Resource.h"

EoCommandRegistry& EoCommandRegistry::Instance() {
  static EoCommandRegistry instance;
  return instance;
}

EoCommandRegistry::EoCommandRegistry() {
  RegisterDefaults();
}

void EoCommandRegistry::Register(const EoCommandEntry& entry, const std::vector<std::wstring>& aliases) {
  const auto index = m_entries.size();
  m_entries.push_back(entry);
  m_lookup[entry.canonicalName] = index;
  for (const auto& alias : aliases) { m_lookup[alias] = index; }
}

const EoCommandEntry* EoCommandRegistry::Find(const std::wstring& name) const noexcept {
  const auto it = m_lookup.find(name);
  if (it == m_lookup.end()) { return nullptr; }
  return &m_entries[it->second];
}

std::vector<std::wstring> EoCommandRegistry::AllKeys() const {
  std::vector<std::wstring> keys;
  keys.reserve(m_lookup.size());
  for (const auto& [key, _] : m_lookup) { keys.push_back(key); }
  std::sort(keys.begin(), keys.end());
  return keys;
}

void EoCommandRegistry::RegisterDefaults() {
  // Draw mode primitives
  Register({L"LINE", ID_MODE_DRAW, ID_DRAW_MODE_LINE, L"Draw line segments"}, {L"L"});
  Register({L"POLYLINE", ID_MODE_DRAW, ID_DRAW_MODE_POLYLINE, L"Draw open polyline"}, {L"PL"});
  Register({L"POLYGON", ID_MODE_DRAW, ID_DRAW_MODE_POLYGON, L"Draw closed polygon"}, {L"POL"});
  Register({L"QUAD", ID_MODE_DRAW, ID_DRAW_MODE_QUAD, L"Draw quadrilateral"}, {});
  Register({L"ARC", ID_MODE_DRAW, ID_DRAW_MODE_ARC, L"Draw arc"}, {L"A"});
  Register({L"CIRCLE", ID_MODE_DRAW, ID_DRAW_MODE_CIRCLE, L"Draw circle"}, {L"C"});
  Register({L"ELLIPSE", ID_MODE_DRAW, ID_DRAW_MODE_ELLIPSE, L"Draw ellipse"}, {L"EL"});
  Register({L"BSPLINE", ID_MODE_DRAW, ID_DRAW_MODE_BSPLINE, L"Draw b-spline"}, {L"SPL", L"SPLINE"});
  Register({L"INSERT", ID_MODE_DRAW, ID_DRAW_MODE_INSERT, L"Insert block reference"}, {L"I"});

  // Edit mode operations
  Register({L"MOVE", ID_MODE_EDIT, ID_EDIT_MODE_MOVE, L"Move trapped groups"}, {L"M"});
  Register({L"COPY", ID_MODE_EDIT, ID_EDIT_MODE_COPY, L"Copy trapped groups"}, {L"CO", L"CP"});
  Register({L"ROTATE", ID_MODE_EDIT, ID_EDIT_MODE_ROTCCW, L"Rotate CCW"}, {L"RO", L"ROT"});
  Register({L"ROTCW", ID_MODE_EDIT, ID_EDIT_MODE_ROTCW, L"Rotate CW"}, {});
  Register({L"FLIP", ID_MODE_EDIT, ID_EDIT_MODE_FLIP, L"Flip trapped groups"}, {});
  Register({L"REDUCE", ID_MODE_EDIT, ID_EDIT_MODE_REDUCE, L"Reduce/scale down"}, {});
  Register({L"ENLARGE", ID_MODE_EDIT, ID_EDIT_MODE_ENLARGE, L"Enlarge/scale up"}, {});
  Register({L"PIVOT", ID_MODE_EDIT, ID_EDIT_MODE_PIVOT, L"Pivot/rotate about point"}, {});

  // Trap (selection) operations
  Register({L"TRAP", ID_MODE_TRAP, 0, L"Enter trap (selection) mode"}, {});
  Register({L"FIELD", ID_MODE_TRAP, ID_TRAP_MODE_FIELD, L"Field/window trap"}, {L"WIN"});
  Register({L"STITCH", ID_MODE_TRAP, ID_TRAP_MODE_STITCH, L"Stitch trap"}, {});

  // Cut mode
  Register({L"CUT", ID_MODE_CUT, 0, L"Enter cut mode"}, {});
  Register({L"SLICE", ID_MODE_CUT, ID_CUT_MODE_SLICE, L"Slice/break primitives"}, {});

  // Dimension mode
  Register({L"DIMENSION", ID_MODE_DIMENSION, 0, L"Enter dimension mode"}, {L"DIM"});
  Register({L"DIMLINE", ID_MODE_DIMENSION, ID_DIMENSION_MODE_LINE, L"Linear dimension"}, {});
  Register({L"DIMRADIUS", ID_MODE_DIMENSION, ID_DIMENSION_MODE_RADIUS, L"Radius dimension"}, {});
  Register({L"DIMDIAMETER", ID_MODE_DIMENSION, ID_DIMENSION_MODE_DIAMETER, L"Diameter dimension"}, {});
  Register({L"DIMANGLE", ID_MODE_DIMENSION, ID_DIMENSION_MODE_ANGLE, L"Angular dimension"}, {});

  // Annotation mode
  Register({L"ANNOTATE", ID_MODE_ANNOTATE, 0, L"Enter annotation mode"}, {L"ANN"});

  // Fixup mode (modify existing geometry)
  Register({L"FIXUP", ID_MODE_FIXUP, 0, L"Enter fixup mode"}, {});
  Register({L"MEND", ID_MODE_FIXUP, ID_FIXUP_MODE_MEND, L"Mend primitive vertex"}, {});
  Register({L"CHAMFER", ID_MODE_FIXUP, ID_FIXUP_MODE_CHAMFER, L"Chamfer corner"}, {L"CHA"});
  Register({L"FILLET", ID_MODE_FIXUP, ID_FIXUP_MODE_FILLET, L"Fillet corner"}, {L"F"});
  Register({L"PARALLEL", ID_MODE_FIXUP, ID_FIXUP_MODE_PARALLEL, L"Parallel offset"}, {});

  // Domain modes
  Register({L"PIPE", ID_MODE_PIPE, 0, L"Enter pipe drawing mode"}, {});
  Register({L"LPD", ID_MODE_LPD, 0, L"Enter low-pressure duct mode"}, {L"DUCT"});
  Register({L"POWER", ID_MODE_POWER, 0, L"Enter power/electrical mode"}, {});
  Register({L"NODAL", ID_MODE_NODAL, 0, L"Enter nodal mode"}, {});
  Register({L"DRAW2", ID_MODE_DRAW2, 0, L"Enter parallel-wall draw mode"}, {L"WALL"});

  // Group/primitive editing
  Register({L"PEDIT", ID_MODE_PRIMITIVE_EDIT, 0, L"Primitive edit (grip drag)"}, {});
  Register({L"GEDIT", ID_MODE_GROUP_EDIT, 0, L"Group edit"}, {});

  // ---------------------------------------------------------------
  // Segment / group operations (global, mode-independent)
  // ---------------------------------------------------------------
  Register({L"ERASE", 0, 0, L"Delete group at cursor",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_TOOLS_GROUP_DELETE, 0), 0); } }},
      {L"E", L"DELETE", L"DEL"});

  Register({L"DELETELAST", 0, 0, L"Delete last group created",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_TOOLS_GROUP_DELETELAST, 0), 0); } }},
      {L"OOPS"});

  Register({L"DELPRIM", 0, 0, L"Delete primitive at cursor",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_TOOLS_PRIMITVE_DELETE, 0), 0); } }},
      {L"ERASEPRIM"});

  Register({L"BREAK", 0, 0, L"Break group into primitives",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_TOOLS_GROUP_BREAK, 0), 0); } }},
      {});

  Register({L"EXCHANGE", 0, 0, L"Exchange group to work layer",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_TOOLS_GROUP_EXCHANGE, 0), 0); } }},
      {});

  // ---------------------------------------------------------------
  // Undo / Redo
  // ---------------------------------------------------------------
  Register({L"UNDO", 0, 0, L"Undo last operation",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_EDIT_UNDO, 0), 0); } }},
      {L"U"});

  Register({L"REDO", 0, 0, L"Redo last undone operation",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_EDIT_REDO, 0), 0); } }},
      {});

  // ---------------------------------------------------------------
  // Snap / Engage
  // ---------------------------------------------------------------
  Register({L"SNAP", 0, 0, L"Engage (snap to) nearest primitive",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_TOOLS_PRIMITIVE_SNAPTO, 0), 0); } }},
      {L"Q", L"ENGAGE"});

  Register({L"SNAPEND", 0, 0, L"Snap to primitive endpoint",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_TOOLS_PRIMITIVE_SNAPTOENDPOINT, 0), 0); } }},
      {L"ENDPOINT"});

  // ---------------------------------------------------------------
  // Render / display properties
  // ---------------------------------------------------------------
  Register({L"COLOR", 0, 0, L"Set pen color",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_PENCOLOR, 0), 0); } }},
      {L"PENCOLOR", L"COL"});

  Register({L"LINETYPE", 0, 0, L"Set line type",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_LINETYPE, 0), 0); } }},
      {L"LT", L"LTYPE"});

  Register({L"FILL", 0, 0, L"Set fill option (HOLLOW SOLID HATCH PATTERN)",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_FILL_HOLLOW, 0), 0); } }},
      {});

  Register({L"FILLSOLID", 0, 0, L"Set fill solid",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_FILL_SOLID, 0), 0); } }},
      {});

  Register({L"FILLHATCH", 0, 0, L"Set fill hatch",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_FILL_HATCH, 0), 0); } }},
      {});

  Register({L"FILLPATTERN", 0, 0, L"Set fill pattern",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_FILL_PATTERN, 0), 0); } }},
      {});

  Register({L"SCALE", 0, 0, L"Set world scale",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_SCALE, 0), 0); } }},
      {L"WORLDSCALE"});

  Register({L"DIMLENGTH", 0, 0, L"Set dimension length",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_DIMLENGTH, 0), 0); } }},
      {});

  Register({L"DIMANGLE", 0, 0, L"Set dimension angle",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_DIMANGLE, 0), 0); } }},
      {});

  // ---------------------------------------------------------------
  // Home points
  // ---------------------------------------------------------------
  Register({L"HOMEPOINT", 0, 0, L"Save a numbered home point",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_SAVEPOINT, 0), 0); } }},
      {L"H", L"SAVEHOME"});

  Register({L"GOHOME", 0, 0, L"Go to a specific home point",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_GOTOPOINT, 0), 0); } }},
      {L"G"});

  // ---------------------------------------------------------------
  // View / display
  // ---------------------------------------------------------------
  Register({L"REFRESH", 0, 0, L"Refresh / redraw display",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_VIEW_REFRESH, 0), 0); } }},
      {L"REDRAW", L"R"});

  Register({L"ZOOMIN", 0, 0, L"Zoom in",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_WINDOW_ZOOMIN, 0), 0); } }},
      {L"ZI"});

  Register({L"ZOOMOUT", 0, 0, L"Zoom out",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_WINDOW_ZOOMOUT, 0), 0); } }},
      {L"ZO"});

  Register({L"ZOOMLAST", 0, 0, L"Restore previous zoom",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_WINDOW_LAST, 0), 0); } }},
      {L"ZL"});

  Register({L"ZOOMSHEET", 0, 0, L"Zoom to sheet/paper extents",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_WINDOW_SHEET, 0), 0); } }},
      {L"ZS"});

  Register({L"PAN", 0, 0, L"Pan display",
      []() { if (auto* v = AeSysView::GetActiveView()) { v->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_WINDOW_PAN, 0), 0); } }},
      {});

  // ---------------------------------------------------------------
  // File operations
  // ---------------------------------------------------------------
  Register({L"NEW", 0, 0, L"New drawing",
      []() { if (auto* w = AfxGetMainWnd()) { w->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_FILE_NEW, 0), 0); } }},
      {});

  Register({L"OPEN", 0, 0, L"Open drawing",
      []() { if (auto* w = AfxGetMainWnd()) { w->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_FILE_OPEN, 0), 0); } }},
      {});

  Register({L"SAVE", 0, 0, L"Save drawing",
      []() { if (auto* w = AfxGetMainWnd()) { w->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0), 0); } }},
      {});

  Register({L"SAVEAS", 0, 0, L"Save drawing as",
      []() { if (auto* w = AfxGetMainWnd()) { w->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE_AS, 0), 0); } }},
      {});

  Register({L"PLOT", 0, 0, L"Plot / print full",
      []() { if (auto* w = AfxGetMainWnd()) { w->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_FILE_PLOT_FULL, 0), 0); } }},
      {L"PRINT"});

  // ---------------------------------------------------------------
  // Functor commands - no WM_COMMAND dispatch; direct invocation.
  // ---------------------------------------------------------------

  // ZOOM EXTENTS - fit model-space extents into the active view.
  Register({L"ZOOM", 0, 0, L"Zoom (EXTENTS sub-command supported)",
      []() {
        auto* view = AeSysView::GetActiveView();
        if (view != nullptr) { view->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_WINDOW_BEST, 0), 0); }
      }},
      {L"Z"});

  Register({L"ZE", 0, 0, L"Zoom to extents",
      []() {
        auto* view = AeSysView::GetActiveView();
        if (view != nullptr) { view->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_WINDOW_BEST, 0), 0); }
      }},
      {L"ZOOMEXTENTS", L"EXTENTS"});

  // UNITS - open the units / precision dialog.
  Register({L"UNITS", 0, 0, L"Set drawing units and precision",
      []() {
        auto* view = AeSysView::GetActiveView();
        if (view != nullptr) { view->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_UNITS, 0), 0); }
      }},
      {L"UN"});

  // PURGE - remove unused blocks, line types, and empty layers.
  Register({L"PURGE", 0, 0, L"Remove unused blocks, line types, and empty layers",
      []() {
        auto* mainFrame = AfxGetMainWnd();
        if (mainFrame == nullptr) { return; }
        mainFrame->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_BLOCKS_REMOVEUNUSED, 0), 0);
        mainFrame->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_PENS_REMOVEUNUSEDSTYLES, 0), 0);
        mainFrame->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_LAYERS_REMOVEEMPTY, 0), 0);
      }},
      {L"PU"});

  // ---------------------------------------------------------------
  // AutoCAD power-user compatibility aliases
  // These map common AutoCAD commands to the closest AeSys equivalent.
  // ---------------------------------------------------------------

  // TR (TRIM) and EX (EXTEND) -> Fixup > Mend.
  // Mend lets you drag an endpoint to a new position; trim/extend semantics
  // require picking the cutting edge — a future Mend extension can handle this.
  Register({L"TRIM", ID_MODE_FIXUP, ID_FIXUP_MODE_MEND,
      L"Trim (uses Fixup > Mend -- pick endpoint and drag to cutting line)"},
      {L"TR"});

  Register({L"EXTEND", ID_MODE_FIXUP, ID_FIXUP_MODE_MEND,
      L"Extend (uses Fixup > Mend -- pick endpoint and drag to extension line)"},
      {L"EX"});

  // OFFSET -> Fixup > Parallel (nearest equivalent for offset-by-distance).
  Register({L"OFFSET", ID_MODE_FIXUP, ID_FIXUP_MODE_PARALLEL,
      L"Offset (uses Fixup > Parallel -- select line and specify distance)"},
      {L"O"});

  // MIRROR -> Edit > Flip (single-axis flip about the trap centroid).
  Register({L"MIRROR", ID_MODE_EDIT, ID_EDIT_MODE_FLIP,
      L"Mirror (uses Edit > Flip -- flips trapped groups about centroid axis)"},
      {L"MI"});

  // HATCH -> note alias redirecting to FILLHATCH. H is reserved for HOMEPOINT.
  // AutoCAD users typing HATCH will activate FILLHATCH; H remains HOMEPOINT.
  Register({L"HATCH", 0, 0,
      L"Hatch fill: activates FILLHATCH (use FILL, FILLSOLID, FILLPATTERN for other fill modes)",
      []() {
        auto* view = AeSysView::GetActiveView();
        if (view != nullptr) {
          view->SendMessageW(WM_COMMAND, MAKEWPARAM(ID_SETUP_FILL_HATCH, 0), 0);
        }
      }},
      {});  // No H alias -- H stays with HOMEPOINT
}
