#include "Stdafx.h"

#include <cassert>
#include <memory>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoGeVector3d.h"
#include "MainFrm.h"
#include "Resource.h"
#include "Section.h"

#if defined(USING_STATE_PATTERN)
#include "AeSysState.h"
#include "DrawModeState.h"
#include "IdleState.h"
#endif

IMPLEMENT_DYNCREATE(AeSysView, CView)

BEGIN_MESSAGE_MAP(AeSysView, CView)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_WM_CONTEXTMENU()
ON_WM_CREATE()
ON_WM_ERASEBKGND()
ON_WM_KILLFOCUS()
ON_WM_LBUTTONDOWN()
ON_WM_LBUTTONUP()
ON_WM_MBUTTONDOWN()
ON_WM_MBUTTONUP()
ON_WM_MOUSEMOVE()
ON_WM_MOUSEWHEEL()
ON_WM_RBUTTONDOWN()
ON_WM_RBUTTONUP()
ON_WM_SETFOCUS()
ON_WM_SIZE()
ON_WM_TIMER()
ON_WM_PAINT()
#pragma warning(pop)

// Mode switching commands
ON_COMMAND(ID_MODE_ANNOTATE, &AeSysView::OnModeAnnotate)
ON_COMMAND(ID_MODE_CUT, &AeSysView::OnModeCut)
ON_COMMAND(ID_MODE_DIMENSION, &AeSysView::OnModeDimension)
ON_COMMAND(ID_MODE_DRAW, &AeSysView::OnModeDraw)
ON_COMMAND(ID_MODE_DRAW2, &AeSysView::OnModeDraw2)
ON_COMMAND(ID_MODE_EDIT, &AeSysView::OnModeEdit)
ON_COMMAND(ID_MODE_FIXUP, &AeSysView::OnModeFixup)
ON_COMMAND(ID_MODE_LPD, &AeSysView::OnModeLPD)
ON_COMMAND(ID_MODE_NODAL, &AeSysView::OnModeNodal)
ON_COMMAND(ID_MODE_PIPE, &AeSysView::OnModePipe)
ON_COMMAND(ID_MODE_POWER, &AeSysView::OnModePower)
ON_COMMAND(ID_MODE_TRAP, &AeSysView::OnModeTrap)

ON_COMMAND(ID_TRAPCOMMANDS_ADDGROUPS, &AeSysView::OnTrapCommandsAddGroups)

// Update UI handlers
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_MODE_ANNOTATE, &AeSysView::OnUpdateModeAnnotate)
ON_UPDATE_COMMAND_UI(ID_MODE_CUT, &AeSysView::OnUpdateModeCut)
ON_UPDATE_COMMAND_UI(ID_MODE_DIMENSION, &AeSysView::OnUpdateModeDimension)
ON_UPDATE_COMMAND_UI(ID_MODE_DRAW, &AeSysView::OnUpdateModeDraw)
ON_UPDATE_COMMAND_UI(ID_MODE_DRAW2, &AeSysView::OnUpdateModeDraw2)
ON_UPDATE_COMMAND_UI(ID_MODE_EDIT, &AeSysView::OnUpdateModeEdit)
ON_UPDATE_COMMAND_UI(ID_MODE_FIXUP, &AeSysView::OnUpdateModeFixup)
ON_UPDATE_COMMAND_UI(ID_MODE_LPD, &AeSysView::OnUpdateModeLpd)
ON_UPDATE_COMMAND_UI(ID_MODE_NODAL, &AeSysView::OnUpdateModeNodal)
ON_UPDATE_COMMAND_UI(ID_MODE_PIPE, &AeSysView::OnUpdateModePipe)
ON_UPDATE_COMMAND_UI(ID_MODE_POWER, &AeSysView::OnUpdateModePower)
ON_UPDATE_COMMAND_UI(ID_MODE_TRAP, &AeSysView::OnUpdateModeTrap)
#pragma warning(pop)

ON_COMMAND(ID_OP0, OnOp0)
ON_COMMAND(ID_OP2, OnOp2)
ON_COMMAND(ID_OP3, OnOp3)
ON_COMMAND(ID_OP4, OnOp4)
ON_COMMAND(ID_OP5, OnOp5)
ON_COMMAND(ID_OP6, OnOp6)
ON_COMMAND(ID_OP7, OnOp7)
ON_COMMAND(ID_OP8, OnOp8)
ON_COMMAND(IDM_RETURN, OnReturn)
ON_COMMAND(IDM_ESCAPE, OnEscape)

ON_COMMAND(ID_3DVIEWS_BACK, On3dViewsBack)
ON_COMMAND(ID_3DVIEWS_BOTTOM, On3dViewsBottom)
ON_COMMAND(ID_3DVIEWS_FRONT, On3dViewsFront)
ON_COMMAND(ID_3DVIEWS_ISOMETRIC, On3dViewsIsometric)
ON_COMMAND(ID_3DVIEWS_LEFT, On3dViewsLeft)
ON_COMMAND(ID_3DVIEWS_RIGHT, On3dViewsRight)
ON_COMMAND(ID_3DVIEWS_TOP, On3dViewsTop)

ON_COMMAND(ID_BACKGROUNDIMAGE_LOAD, OnBackgroundImageLoad)
ON_COMMAND(ID_BACKGROUNDIMAGE_REMOVE, OnBackgroundImageRemove)
ON_COMMAND(ID_CAMERA_ROTATELEFT, OnCameraRotateLeft)
ON_COMMAND(ID_CAMERA_ROTATERIGHT, OnCameraRotateRight)
ON_COMMAND(ID_CAMERA_ROTATEUP, OnCameraRotateUp)
ON_COMMAND(ID_CAMERA_ROTATEDOWN, OnCameraRotateDown)
ON_COMMAND(ID_FILE_PLOT_QUARTER, OnFilePlotQuarter)
ON_COMMAND(ID_FILE_PLOT_HALF, OnFilePlotHalf)
ON_COMMAND(ID_FILE_PLOT_FULL, OnFilePlotFull)
// Standard printing commands
ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
ON_COMMAND(ID_FILE_PRINTCURRENTVIEW, OnFilePrint)
ON_COMMAND(ID_HELP_KEY, OnHelpKey)
ON_COMMAND(ID_MODE_GROUP_EDIT, OnModeGroupEdit)
ON_COMMAND(ID_MODE_PRIMITIVE_EDIT, OnModePrimitiveEdit)
ON_COMMAND(ID_MODE_PRIMITIVE_MEND, OnModePrimitiveMend)
ON_COMMAND(ID_PRIM_PERPJUMP, OnPrimPerpJump)
ON_COMMAND(ID_RELATIVEMOVES_ENG_DOWN, OnRelativeMovesEngDown)
ON_COMMAND(ID_RELATIVEMOVES_ENG_DOWNROTATE, OnRelativeMovesEngDownRotate)
ON_COMMAND(ID_RELATIVEMOVES_ENG_IN, OnRelativeMovesEngIn)
ON_COMMAND(ID_RELATIVEMOVES_ENG_LEFT, OnRelativeMovesEngLeft)
ON_COMMAND(ID_RELATIVEMOVES_ENG_LEFTROTATE, OnRelativeMovesEngLeftRotate)
ON_COMMAND(ID_RELATIVEMOVES_ENG_OUT, OnRelativeMovesEngOut)
ON_COMMAND(ID_RELATIVEMOVES_ENG_RIGHT, OnRelativeMovesEngRight)
ON_COMMAND(ID_RELATIVEMOVES_ENG_RIGHTROTATE, OnRelativeMovesEngRightRotate)
ON_COMMAND(ID_RELATIVEMOVES_ENG_UP, OnRelativeMovesEngUp)
ON_COMMAND(ID_RELATIVEMOVES_ENG_UPROTATE, OnRelativeMovesEngUpRotate)
ON_COMMAND(ID_RELATIVEMOVES_DOWN, OnRelativeMovesDown)
ON_COMMAND(ID_RELATIVEMOVES_DOWNROTATE, OnRelativeMovesDownRotate)
ON_COMMAND(ID_RELATIVEMOVES_IN, OnRelativeMovesIn)
ON_COMMAND(ID_RELATIVEMOVES_LEFT, OnRelativeMovesLeft)
ON_COMMAND(ID_RELATIVEMOVES_LEFTROTATE, OnRelativeMovesLeftRotate)
ON_COMMAND(ID_RELATIVEMOVES_OUT, OnRelativeMovesOut)
ON_COMMAND(ID_RELATIVEMOVES_RIGHT, OnRelativeMovesRight)
ON_COMMAND(ID_RELATIVEMOVES_RIGHTROTATE, OnRelativeMovesRightRotate)
ON_COMMAND(ID_RELATIVEMOVES_UP, OnRelativeMovesUp)
ON_COMMAND(ID_RELATIVEMOVES_UPROTATE, OnRelativeMovesUpRotate)
ON_COMMAND(ID_SETUP_CONSTRAINTS, OnSetupConstraints)
ON_COMMAND(ID_SETUP_DIMANGLE, OnSetupDimAngle)
ON_COMMAND(ID_SETUP_DIMLENGTH, OnSetupDimLength)
ON_COMMAND(ID_SETUP_MOUSEBUTTONS, OnSetupMouseButtons)
ON_COMMAND(ID_SETUP_SCALE, OnSetupScale)
ON_COMMAND(ID_SETUP_UNITS, OnSetupUnits)
ON_COMMAND(ID_TOOLS_PRIMITIVE_SNAPTO, OnToolsPrimitiveSnapto)
ON_COMMAND(ID_VIEW_BACKGROUNDIMAGE, OnViewBackgroundImage)
ON_COMMAND(ID_VIEW_LIGHTING, OnViewLighting)
ON_COMMAND(ID_VIEW_PARAMETERS, OnViewParameters)
ON_COMMAND(ID_VIEW_PENWIDTHS, OnViewPenWidths)
ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
ON_COMMAND(ID_VIEW_RENDERED, OnViewRendered)
ON_COMMAND(ID_VIEW_SOLID, OnViewSolid)
ON_COMMAND(ID_VIEW_TRUETYPEFONTS, OnViewTrueTypeFonts)
ON_COMMAND(ID_VIEW_WINDOW, OnViewWindow)
ON_COMMAND(ID_VIEW_WIREFRAME, OnViewWireframe)
ON_COMMAND(ID_VIEW_DIRECT2D, OnViewDirect2D)
ON_COMMAND(ID_VIEW_ALIASED, OnViewAliased)
ON_COMMAND(ID_VIEW_BACKGROUND_TOGGLE, OnViewBackgroundToggle)
ON_COMMAND(ID_WINDOW_BEST, OnWindowBest)
ON_COMMAND(ID_WINDOW_NORMAL, OnWindowNormal)
ON_COMMAND(ID_WINDOW_LAST, OnWindowLast)
ON_COMMAND(ID_WINDOW_PAN, OnWindowPan)
ON_COMMAND(ID_WINDOW_PAN_LEFT, OnWindowPanLeft)
ON_COMMAND(ID_WINDOW_PAN_RIGHT, OnWindowPanRight)
ON_COMMAND(ID_WINDOW_PAN_UP, OnWindowPanUp)
ON_COMMAND(ID_WINDOW_PAN_DOWN, OnWindowPanDown)
ON_COMMAND(ID_WINDOW_SHEET, OnWindowSheet)
ON_COMMAND(ID_WINDOW_ZOOMIN, OnWindowZoomIn)
ON_COMMAND(ID_WINDOW_ZOOMOUT, OnWindowZoomOut)
ON_COMMAND(ID_VIEW_WINDOW_KEYPLAN, &AeSysView::OnViewWindowKeyplan)
ON_COMMAND(ID_WINDOW_ZOOMSPECIAL, &AeSysView::OnWindowZoomSpecial)
#pragma warning(push)
#pragma warning(disable : 4191)
ON_UPDATE_COMMAND_UI(ID_BACKGROUNDIMAGE_LOAD, OnUpdateBackgroundimageLoad)
ON_UPDATE_COMMAND_UI(ID_BACKGROUNDIMAGE_REMOVE, OnUpdateBackgroundimageRemove)
ON_UPDATE_COMMAND_UI(ID_VIEW_BACKGROUNDIMAGE, OnUpdateViewBackgroundImage)
ON_UPDATE_COMMAND_UI(ID_VIEW_PENWIDTHS, OnUpdateViewPenwidths)
ON_UPDATE_COMMAND_UI(ID_VIEW_RENDERED, OnUpdateViewRendered)
ON_UPDATE_COMMAND_UI(ID_VIEW_TRUETYPEFONTS, OnUpdateViewTrueTypeFonts)
ON_UPDATE_COMMAND_UI(ID_VIEW_WIREFRAME, OnUpdateViewWireframe)
ON_UPDATE_COMMAND_UI(ID_VIEW_DIRECT2D, OnUpdateViewDirect2D)
ON_UPDATE_COMMAND_UI(ID_VIEW_ALIASED, OnUpdateViewAliased)
ON_UPDATE_COMMAND_UI(ID_VIEW_BACKGROUND_TOGGLE, OnUpdateViewBackgroundToggle)
#if defined(USING_STATE_PATTERN)
ON_COMMAND_RANGE(ID_DRAW_MODE_OPTIONS, ID_DRAW_MODE_SHIFT_RETURN, &AeSysView::OnDrawCommand)
#endif
#pragma warning(pop)
ON_COMMAND(ID_DRAW_MODE_OPTIONS, &AeSysView::OnDrawModeOptions)
ON_COMMAND(ID_DRAW_MODE_POINT, &AeSysView::OnDrawModePoint)
ON_COMMAND(ID_DRAW_MODE_LINE, &AeSysView::OnDrawModeLine)
ON_COMMAND(ID_DRAW_MODE_POLYGON, &AeSysView::OnDrawModePolygon)
ON_COMMAND(ID_DRAW_MODE_QUAD, &AeSysView::OnDrawModeQuad)
ON_COMMAND(ID_DRAW_MODE_ARC, &AeSysView::OnDrawModeArc)
ON_COMMAND(ID_DRAW_MODE_BSPLINE, &AeSysView::OnDrawModeBspline)
ON_COMMAND(ID_DRAW_MODE_CIRCLE, &AeSysView::OnDrawModeCircle)
ON_COMMAND(ID_DRAW_MODE_ELLIPSE, &AeSysView::OnDrawModeEllipse)
ON_COMMAND(ID_DRAW_MODE_INSERT, &AeSysView::OnDrawModeInsert)
ON_COMMAND(ID_DRAW_MODE_RETURN, &AeSysView::OnDrawModeReturn)
ON_COMMAND(ID_DRAW_MODE_ESCAPE, &AeSysView::OnDrawModeEscape)
ON_COMMAND(ID_DRAW_MODE_SHIFT_RETURN, &AeSysView::OnDrawModeShiftReturn)

ON_COMMAND(ID_ANNOTATE_MODE_OPTIONS, &AeSysView::OnAnnotateModeOptions)
ON_COMMAND(ID_ANNOTATE_MODE_LINE, &AeSysView::OnAnnotateModeLine)
ON_COMMAND(ID_ANNOTATE_MODE_ARROW, &AeSysView::OnAnnotateModeArrow)
ON_COMMAND(ID_ANNOTATE_MODE_BUBBLE, &AeSysView::OnAnnotateModeBubble)
ON_COMMAND(ID_ANNOTATE_MODE_HOOK, &AeSysView::OnAnnotateModeHook)
ON_COMMAND(ID_ANNOTATE_MODE_UNDERLINE, &AeSysView::OnAnnotateModeUnderline)
ON_COMMAND(ID_ANNOTATE_MODE_BOX, &AeSysView::OnAnnotateModeBox)
ON_COMMAND(ID_ANNOTATE_MODE_CUT_IN, &AeSysView::OnAnnotateModeCutIn)
ON_COMMAND(ID_ANNOTATE_MODE_CONSTRUCTION_LINE, &AeSysView::OnAnnotateModeConstructionLine)
ON_COMMAND(ID_ANNOTATE_MODE_RETURN, &AeSysView::OnAnnotateModeReturn)
ON_COMMAND(ID_ANNOTATE_MODE_ESCAPE, &AeSysView::OnAnnotateModeEscape)

ON_COMMAND(ID_PIPE_MODE_OPTIONS, &AeSysView::OnPipeModeOptions)
ON_COMMAND(ID_PIPE_MODE_LINE, &AeSysView::OnPipeModeLine)
ON_COMMAND(ID_PIPE_MODE_FITTING, &AeSysView::OnPipeModeFitting)
ON_COMMAND(ID_PIPE_MODE_RISE, &AeSysView::OnPipeModeRise)
ON_COMMAND(ID_PIPE_MODE_DROP, &AeSysView::OnPipeModeDrop)
ON_COMMAND(ID_PIPE_MODE_SYMBOL, &AeSysView::OnPipeModeSymbol)
ON_COMMAND(ID_PIPE_MODE_WYE, &AeSysView::OnPipeModeWye)
ON_COMMAND(ID_PIPE_MODE_RETURN, &AeSysView::OnPipeModeReturn)
ON_COMMAND(ID_PIPE_MODE_ESCAPE, &AeSysView::OnPipeModeEscape)

ON_COMMAND(ID_POWER_MODE_OPTIONS, &AeSysView::OnPowerModeOptions)
ON_COMMAND(ID_POWER_MODE_CIRCUIT, &AeSysView::OnPowerModeCircuit)
ON_COMMAND(ID_POWER_MODE_GROUND, &AeSysView::OnPowerModeGround)
ON_COMMAND(ID_POWER_MODE_HOT, &AeSysView::OnPowerModeHot)
ON_COMMAND(ID_POWER_MODE_SWITCH, &AeSysView::OnPowerModeSwitch)
ON_COMMAND(ID_POWER_MODE_NEUTRAL, &AeSysView::OnPowerModeNeutral)
ON_COMMAND(ID_POWER_MODE_HOME, &AeSysView::OnPowerModeHome)
ON_COMMAND(ID_POWER_MODE_RETURN, &AeSysView::OnPowerModeReturn)
ON_COMMAND(ID_POWER_MODE_ESCAPE, &AeSysView::OnPowerModeEscape)

ON_COMMAND(ID_DRAW2_MODE_OPTIONS, &AeSysView::OnDraw2ModeOptions)
ON_COMMAND(ID_DRAW2_MODE_JOIN, &AeSysView::OnDraw2ModeJoin)
ON_COMMAND(ID_DRAW2_MODE_WALL, &AeSysView::OnDraw2ModeWall)
ON_COMMAND(ID_DRAW2_MODE_RETURN, &AeSysView::OnDraw2ModeReturn)
ON_COMMAND(ID_DRAW2_MODE_ESCAPE, &AeSysView::OnDraw2ModeEscape)

ON_COMMAND(ID_LPD_MODE_OPTIONS, &AeSysView::OnLpdModeOptions)
ON_COMMAND(ID_LPD_MODE_JOIN, &AeSysView::OnLpdModeJoin)
ON_COMMAND(ID_LPD_MODE_DUCT, &AeSysView::OnLpdModeDuct)
ON_COMMAND(ID_LPD_MODE_TRANSITION, &AeSysView::OnLpdModeTransition)
ON_COMMAND(ID_LPD_MODE_TAP, &AeSysView::OnLpdModeTap)
ON_COMMAND(ID_LPD_MODE_ELL, &AeSysView::OnLpdModeEll)
ON_COMMAND(ID_LPD_MODE_TEE, &AeSysView::OnLpdModeTee)
ON_COMMAND(ID_LPD_MODE_UP_DOWN, &AeSysView::OnLpdModeUpDown)
ON_COMMAND(ID_LPD_MODE_SIZE, &AeSysView::OnLpdModeSize)
ON_COMMAND(ID_LPD_MODE_RETURN, &AeSysView::OnLpdModeReturn)
ON_COMMAND(ID_LPD_MODE_ESCAPE, &AeSysView::OnLpdModeEscape)

ON_COMMAND(ID_EDIT_MODE_OPTIONS, &AeSysView::OnEditModeOptions)
ON_COMMAND(ID_EDIT_MODE_PIVOT, &AeSysView::OnEditModePivot)
ON_COMMAND(ID_EDIT_MODE_ROTCCW, &AeSysView::OnEditModeRotccw)
ON_COMMAND(ID_EDIT_MODE_ROTCW, &AeSysView::OnEditModeRotcw)
ON_COMMAND(ID_EDIT_MODE_MOVE, &AeSysView::OnEditModeMove)
ON_COMMAND(ID_EDIT_MODE_COPY, &AeSysView::OnEditModeCopy)
ON_COMMAND(ID_EDIT_MODE_FLIP, &AeSysView::OnEditModeFlip)
ON_COMMAND(ID_EDIT_MODE_REDUCE, &AeSysView::OnEditModeReduce)
ON_COMMAND(ID_EDIT_MODE_ENLARGE, &AeSysView::OnEditModeEnlarge)
ON_COMMAND(ID_EDIT_MODE_RETURN, &AeSysView::OnEditModeReturn)
ON_COMMAND(ID_EDIT_MODE_ESCAPE, &AeSysView::OnEditModeEscape)

ON_COMMAND(ID_TRAP_MODE_REMOVE_ADD, &AeSysView::OnTrapModeRemoveAdd)
ON_COMMAND(ID_TRAP_MODE_POINT, &AeSysView::OnTrapModePoint)
ON_COMMAND(ID_TRAP_MODE_STITCH, &AeSysView::OnTrapModeStitch)
ON_COMMAND(ID_TRAP_MODE_FIELD, &AeSysView::OnTrapModeField)
ON_COMMAND(ID_TRAP_MODE_LAST, &AeSysView::OnTrapModeLast)
ON_COMMAND(ID_TRAP_MODE_ENGAGE, &AeSysView::OnTrapModeEngage)
ON_COMMAND(ID_TRAP_MODE_MODIFY, &AeSysView::OnTrapModeModify)
ON_COMMAND(ID_TRAP_MODE_ESCAPE, &AeSysView::OnTrapModeEscape)
ON_COMMAND(ID_TRAP_MODE_MENU, &AeSysView::OnTrapModeMenu)

ON_COMMAND(ID_TRAPR_MODE_REMOVE_ADD, &AeSysView::OnTraprModeRemoveAdd)
ON_COMMAND(ID_TRAPR_MODE_POINT, &AeSysView::OnTraprModePoint)
ON_COMMAND(ID_TRAPR_MODE_STITCH, &AeSysView::OnTraprModeStitch)
ON_COMMAND(ID_TRAPR_MODE_FIELD, &AeSysView::OnTraprModeField)
ON_COMMAND(ID_TRAPR_MODE_LAST, &AeSysView::OnTraprModeLast)
ON_COMMAND(ID_TRAPR_MODE_ENGAGE, &AeSysView::OnTraprModeEngage)
ON_COMMAND(ID_TRAPR_MODE_MENU, &AeSysView::OnTraprModeMenu)
ON_COMMAND(ID_TRAPR_MODE_MODIFY, &AeSysView::OnTraprModeModify)
ON_COMMAND(ID_TRAPR_MODE_ESCAPE, &AeSysView::OnTraprModeEscape)

ON_COMMAND(ID_DIMENSION_MODE_OPTIONS, &AeSysView::OnDimensionModeOptions)
ON_COMMAND(ID_DIMENSION_MODE_ARROW, &AeSysView::OnDimensionModeArrow)
ON_COMMAND(ID_DIMENSION_MODE_LINE, &AeSysView::OnDimensionModeLine)
ON_COMMAND(ID_DIMENSION_MODE_DLINE, &AeSysView::OnDimensionModeDLine)
ON_COMMAND(ID_DIMENSION_MODE_DLINE2, &AeSysView::OnDimensionModeDLine2)
ON_COMMAND(ID_DIMENSION_MODE_EXTEN, &AeSysView::OnDimensionModeExten)
ON_COMMAND(ID_DIMENSION_MODE_RADIUS, &AeSysView::OnDimensionModeRadius)
ON_COMMAND(ID_DIMENSION_MODE_DIAMETER, &AeSysView::OnDimensionModeDiameter)
ON_COMMAND(ID_DIMENSION_MODE_ANGLE, &AeSysView::OnDimensionModeAngle)
ON_COMMAND(ID_DIMENSION_MODE_CONVERT, &AeSysView::OnDimensionModeConvert)
ON_COMMAND(ID_DIMENSION_MODE_RETURN, &AeSysView::OnDimensionModeReturn)
ON_COMMAND(ID_DIMENSION_MODE_ESCAPE, &AeSysView::OnDimensionModeEscape)

ON_COMMAND(ID_CUT_MODE_OPTIONS, &AeSysView::OnCutModeOptions)
ON_COMMAND(ID_CUT_MODE_TORCH, &AeSysView::OnCutModeTorch)
ON_COMMAND(ID_CUT_MODE_SLICE, &AeSysView::OnCutModeSlice)
ON_COMMAND(ID_CUT_MODE_FIELD, &AeSysView::OnCutModeField)
ON_COMMAND(ID_CUT_MODE_CLIP, &AeSysView::OnCutModeClip)
ON_COMMAND(ID_CUT_MODE_DIVIDE, &AeSysView::OnCutModeDivide)
ON_COMMAND(ID_CUT_MODE_RETURN, &AeSysView::OnCutModeReturn)
ON_COMMAND(ID_CUT_MODE_ESCAPE, &AeSysView::OnCutModeEscape)

ON_COMMAND(ID_FIXUP_MODE_OPTIONS, &AeSysView::OnFixupModeOptions)
ON_COMMAND(ID_FIXUP_MODE_REFERENCE, &AeSysView::OnFixupModeReference)
ON_COMMAND(ID_FIXUP_MODE_MEND, &AeSysView::OnFixupModeMend)
ON_COMMAND(ID_FIXUP_MODE_CHAMFER, &AeSysView::OnFixupModeChamfer)
ON_COMMAND(ID_FIXUP_MODE_FILLET, &AeSysView::OnFixupModeFillet)
ON_COMMAND(ID_FIXUP_MODE_SQUARE, &AeSysView::OnFixupModeSquare)
ON_COMMAND(ID_FIXUP_MODE_PARALLEL, &AeSysView::OnFixupModeParallel)
ON_COMMAND(ID_FIXUP_MODE_RETURN, &AeSysView::OnFixupModeReturn)
ON_COMMAND(ID_FIXUP_MODE_ESCAPE, &AeSysView::OnFixupModeEscape)

ON_COMMAND(ID_NODAL_MODE_ADDREMOVE, &AeSysView::OnNodalModeAddRemove)
ON_COMMAND(ID_NODAL_MODE_POINT, &AeSysView::OnNodalModePoint)
ON_COMMAND(ID_NODAL_MODE_LINE, &AeSysView::OnNodalModeLine)
ON_COMMAND(ID_NODAL_MODE_AREA, &AeSysView::OnNodalModeArea)
ON_COMMAND(ID_NODAL_MODE_MOVE, &AeSysView::OnNodalModeMove)
ON_COMMAND(ID_NODAL_MODE_COPY, &AeSysView::OnNodalModeCopy)
ON_COMMAND(ID_NODAL_MODE_TOLINE, &AeSysView::OnNodalModeToLine)
ON_COMMAND(ID_NODAL_MODE_TOPOLYGON, &AeSysView::OnNodalModeToPolygon)
ON_COMMAND(ID_NODAL_MODE_EMPTY, &AeSysView::OnNodalModeEmpty)
ON_COMMAND(ID_NODAL_MODE_ENGAGE, &AeSysView::OnNodalModeEngage)
ON_COMMAND(ID_NODAL_MODE_RETURN, &AeSysView::OnNodalModeReturn)
ON_COMMAND(ID_NODAL_MODE_ESCAPE, &AeSysView::OnNodalModeEscape)
END_MESSAGE_MAP()

/// AeSysView construction/destruction ////////////////////////////////////////

AeSysView::AeSysView() {
  m_Viewport.SetDeviceWidthInPixels(app.DeviceWidthInPixels());
  m_Viewport.SetDeviceHeightInPixels(app.DeviceHeightInPixels());
  m_Viewport.SetDeviceWidthInInches(app.DeviceWidthInMillimeters() / Eo::MmPerInch);
  m_Viewport.SetDeviceHeightInInches(app.DeviceHeightInMillimeters() / Eo::MmPerInch);
}

AeSysView::~AeSysView() {}
#if defined(USING_STATE_PATTERN)
void AeSysView::PushState(std::unique_ptr<AeSysState> newState) {
  if (!m_stateStack.empty()) { m_stateStack.top()->OnExit(this); }
  newState->OnEnter(this);
  m_stateStack.push(std::move(newState));
  Invalidate();  // Trigger redraw
}

void AeSysView::PopState() {
  if (!m_stateStack.empty()) {
    m_stateStack.top()->OnExit(this);
    m_stateStack.pop();
    if (!m_stateStack.empty()) { m_stateStack.top()->OnEnter(this); }
    Invalidate();
  }
}

AeSysState* AeSysView::GetCurrentState() const { return m_stateStack.empty() ? nullptr : m_stateStack.top().get(); }
#endif
AeSysDoc* AeSysView::GetDocument() const {
#ifdef _DEBUG
  auto* document = dynamic_cast<AeSysDoc*>(m_pDocument);
  assert(document != nullptr && "Invalid document type in AeSysView::GetDocument()");
  return document;
#else
  return static_cast<AeSysDoc*>(m_pDocument);
#endif
}

void AeSysView::OnActivateFrame(UINT state, CFrameWnd* deactivateFrame) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnActivateFrame(%i, %08.8lx)\n", this, state, deactivateFrame);

  CView::OnActivateFrame(state, deactivateFrame);
}

void AeSysView::OnActivateView(BOOL activate, CView* activateView, CView* deactiveView) {
  ATLTRACE2(
      traceGeneral, 3, L"AeSysView<%p>::OnActivateView(%i, %p, %p))\n", this, activate, activateView, deactiveView);

  CMainFrame* MainFrame = (CMainFrame*)(AfxGetMainWnd());
  if (activate) {
    if (CopyAcceleratorTableW(MainFrame->m_hAccelTable, nullptr, 0) ==
        0) {  // Accelerator table was destroyed when keyboard focus was killed - reload resource
      app.BuildModifiedAcceleratorTable();
    }
    // D2D HWND render targets retain their last frame, but the window content may have been
    // obscured by MFC tab/MDI child switching. Force a full re-render so the view repaints.
    if (m_useD2D) { InvalidateScene(); }
  }
  CMFCPropertyGridProperty& ActiveViewScaleProperty = MainFrame->GetPropertiesPane().GetActiveViewScaleProperty();
  ActiveViewScaleProperty.SetValue(m_WorldScale);
  ActiveViewScaleProperty.Enable(activate);

  CView::OnActivateView(activate, activateView, deactiveView);
}

BOOL AeSysView::PreCreateWindow(CREATESTRUCT& createStructure) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::PreCreateWindow(%08.8lx) ", this, createStructure);

  // Remove the sunken 3D client edge — gives a flat, modern border-less document view
  createStructure.dwExStyle &= ~WS_EX_CLIENTEDGE;
  return CView::PreCreateWindow(createStructure);
}

int AeSysView::OnCreate(LPCREATESTRUCT createStructure) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnCreate(%08.8lx)\n", this, createStructure);

  if (CView::OnCreate(createStructure) == -1) { return -1; }
  return 0;
}

void AeSysView::OnSetFocus(CWnd* oldWindow) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnSetFocus(%08.8lx)\n", this, oldWindow);

  CMainFrame* MainFrame = (CMainFrame*)(AfxGetMainWnd());
  if (CopyAcceleratorTableW(MainFrame->m_hAccelTable, nullptr, 0) == 0) {
    // Accelerator table was destroyed when keyboard focus was killed - reload resource
    app.BuildModifiedAcceleratorTable();
  }
  CView::OnSetFocus(oldWindow);
}

void AeSysView::OnKillFocus(CWnd* newWindow) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnKillFocus(%08.8lx)\n", this, newWindow);

  HACCEL AcceleratorTableHandle = ((CMainFrame*)AfxGetMainWnd())->m_hAccelTable;

  ::DestroyAcceleratorTable(AcceleratorTableHandle);

  CView::OnKillFocus(newWindow);
}

void AeSysView::OnPaint() {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnPaint()\n", this);
  CView::OnPaint();
}

