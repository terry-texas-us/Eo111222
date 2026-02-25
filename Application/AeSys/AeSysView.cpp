#include "Stdafx.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <utility>
#include <wchar.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbBitmapFile.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbConic.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbPolygon.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDlgActiveViewKeyplan.h"
#include "EoDlgSelectIsometricView.h"
#include "EoDlgSetAngle.h"
#include "EoDlgSetLength.h"
#include "EoDlgSetScale.h"
#include "EoDlgSetUnitsAndPrecision.h"
#include "EoDlgSetupConstraints.h"
#include "EoDlgSetupCustomMouseCharacters.h"
#include "EoDlgViewParameters.h"
#include "EoDlgViewZoom.h"
#include "EoGeLine.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsModelTransform.h"
#include "EoGsRenderState.h"
#include "EoGsViewTransform.h"
#include "MainFrm.h"
#include "Resource.h"
#include "Section.h"

#if defined(USING_STATE_PATTERN)
#include "AeSysState.h"
#include "DrawModeState.h"
#include "IdleState.h"
#endif

#if defined(USING_DDE)
#include "Dde.h"
#include "DdeGItms.h"
#endif  // USING_DDE

namespace {

constexpr double maximumWindowRatio{999.0};
constexpr double minimumWindowRatio{0.001};

#if defined(LEGACY_ODOMETER)
/** @deprecated This code is only used for the legacy odometer display, which draws the odometer values directly in the view.
 The current implementation displays the odometer values in the status bar, so this code is no longer needed.*/
void DrawOdometerInView(AeSysView* view, CDC* context, Eo::Units Units, EoGeVector3d& position) {
  auto* oldFont = static_cast<CFont*>(context->SelectStockObject(DEFAULT_GUI_FONT));
  auto oldTextAlign = context->SetTextAlign(TA_LEFT | TA_TOP);
  auto oldTextColor = context->SetTextColor(App::ViewTextColor());
  auto oldBackgroundColor = context->SetBkColor(~App::ViewTextColor() & 0x00ffffff);

  CRect clientArea;
  view->GetClientRect(&clientArea);
  TEXTMETRIC metrics;
  context->GetTextMetrics(&metrics);

  CString length;

  int left = clientArea.right - 16 * metrics.tmAveCharWidth;

  CRect rc(left, clientArea.top, clientArea.right, clientArea.top + metrics.tmHeight);
  app.FormatLength(length, Units, position.x);
  length.TrimLeft();
  context->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, length, (UINT)length.GetLength(), 0);

  rc.SetRect(left, clientArea.top + 1 * metrics.tmHeight, clientArea.right, clientArea.top + 2 * metrics.tmHeight);
  app.FormatLength(length, Units, position.y);
  length.TrimLeft();
  context->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, length, (UINT)length.GetLength(), 0);

  rc.SetRect(left, clientArea.top + 2 * metrics.tmHeight, clientArea.right, clientArea.top + 3 * metrics.tmHeight);
  app.FormatLength(length, Units, position.z);
  length.TrimLeft();
  context->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, length, (UINT)length.GetLength(), 0);

  context->SetBkColor(oldBackgroundColor);
  context->SetTextColor(oldTextColor);
  context->SetTextAlign(oldTextAlign);
  context->SelectObject(oldFont);
}
#endif

}  // namespace

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

ON_COMMAND(ID_EDIT_FIND_COMBO, OnFind)
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
ON_COMMAND(ID_EDIT_FIND, &AeSysView::OnEditFind)
ON_COMMAND(ID_FILE_PLOT_QUARTER, OnFilePlotQuarter)
ON_COMMAND(ID_FILE_PLOT_HALF, OnFilePlotHalf)
ON_COMMAND(ID_FILE_PLOT_FULL, OnFilePlotFull)
// Standard printing commands
ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrint)
ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
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
ON_COMMAND(ID_VIEW_ODOMETER, OnViewOdometer)
ON_COMMAND(ID_VIEW_PARAMETERS, OnViewParameters)
ON_COMMAND(ID_VIEW_PENWIDTHS, OnViewPenWidths)
ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
ON_COMMAND(ID_VIEW_RENDERED, OnViewRendered)
ON_COMMAND(ID_VIEW_SOLID, OnViewSolid)
ON_COMMAND(ID_VIEW_STATEINFORMATION, OnViewStateInformation)
ON_COMMAND(ID_VIEW_TRUETYPEFONTS, OnViewTrueTypeFonts)
ON_COMMAND(ID_VIEW_WINDOW, OnViewWindow)
ON_COMMAND(ID_VIEW_WIREFRAME, OnViewWireframe)
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
ON_UPDATE_COMMAND_UI(ID_VIEW_ODOMETER, OnUpdateViewOdometer)
ON_UPDATE_COMMAND_UI(ID_VIEW_PENWIDTHS, OnUpdateViewPenwidths)
ON_UPDATE_COMMAND_UI(ID_VIEW_RENDERED, OnUpdateViewRendered)
ON_UPDATE_COMMAND_UI(ID_VIEW_STATEINFORMATION, OnUpdateViewStateinformation)
ON_UPDATE_COMMAND_UI(ID_VIEW_TRUETYPEFONTS, OnUpdateViewTrueTypeFonts)
ON_UPDATE_COMMAND_UI(ID_VIEW_WIREFRAME, OnUpdateViewWireframe)
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

AeSysView::AeSysView()
    : m_ModelTransform{},
      m_Viewport{},
      m_ViewTransform{},
      m_backgroundImageBitmap{},
      m_backgroundImagePalette{},
      m_EngagedPrimitive{},
      m_EngagedGroup{},
      m_OpHighlighted{},
      m_OverviewViewTransform{},
      m_Plot{},
      m_PlotScaleFactor{1.0},
      m_PreviewGroup{},
      m_PreviousViewTransform{},
      m_PreviousOp{0},
      m_PreviousPnt{},
      m_SelectApertureSize{0.005},
      m_viewBackgroundImage{},
      m_ViewOdometer{true},
      m_ViewPenWidths{},
      m_Viewports{},
      m_ViewRendered{},
      m_ViewTransforms{},
      m_ViewTrueTypeFonts{true},
      m_ViewWireframe{},
      m_VisibleGroupList{},
      m_WorldScale{1.0},
      m_ptDet{},
      m_vRelPos{},
      // Grid constraints
      m_GridOrigin{},
      m_MaximumDotsPerLine{64},
      m_XGridLineSpacing{12.0},
      m_YGridLineSpacing{12.0},
      m_ZGridLineSpacing{12.0},
      m_XGridSnapSpacing{1.0},
      m_YGridSnapSpacing{1.0},
      m_ZGridSnapSpacing{1.0},
      m_XGridPointSpacing{3.0},
      m_YGridPointSpacing{3.0},
      m_ZGridPointSpacing{},
      m_AxisConstraintInfluenceAngle{5.0},
      m_AxisConstraintOffsetAngle{},
      m_DisplayGridWithLines{},
      m_DisplayGridWithPoints{},
      m_GridSnap{},
      // Cursor/Selection
      m_ViewStateInformation{true},
      m_middleButtonPanStartPoint{},
      m_middleButtonPanInProgress{},
      m_rubberbandType{None},
      m_rubberbandBegin{},
      m_rubberbandLogicalBegin{},
      m_rubberbandLogicalEnd{},
      m_ptCursorPosDev{},
      m_ptCursorPosWorld{},
      // Sub-mode edit
      m_SubModeEditGroup{},
      m_SubModeEditPrimitive{},
      m_SubModeEditBeginPoint{},
      m_SubModeEditEndPoint{},
      m_tmEditSeg{},
      // Mend
      m_MendPrimitiveBegin{},
      m_MendPrimitiveVertexIndex{},
      m_PrimitiveToMend{},
      m_PrimitiveToMendCopy{},
      // Annotate
      m_GapSpaceFactor{0.5},
      m_CircleRadius{0.03125},
      m_EndItemType{1},
      m_EndItemSize{0.1},
      m_BubbleRadius{0.125},
      m_NumberOfSides{},
      m_DefaultText{},
      /// Draw2 Mode Interface
      m_centerLineEccentricity{0.5},
      m_continuingCorner{},
      m_distanceBetweenLines{0.0625},
      m_currentLeftLine{},
      m_currentRightLine{},
      m_previousReferenceLine{},
      m_currentReferenceLine{},
      m_assemblyGroup{},
      m_endSectionGroup{},
      m_beginSectionGroup{},
      m_beginSectionLinePrimitive{},
      m_endSectionLinePrimitive{},
      /// Fixup Mode Interface
      m_FixupModeAxisTolerance{2.0},
      m_FixupModeCornerSize{0.25},
      // Edit Mode interface
      m_EditModeMirrorScale{-1.0, 1.0, 1.0},
      m_editModeRotationAngles{0.0, 0.0, 45.0},
      m_EditModeScale{2.0, 2.0, 2.0},

      /// Low Pressure Duct Mode Interface
      m_InsideRadiusFactor{1.5},
      m_DuctSeamSize{0.03125},
      m_DuctTapSize{0.09375},
      m_GenerateTurningVanes{true},
      m_ElbowType{Mittered},
      m_DuctJustification{Center},
      m_TransitionSlope{4.0},
      m_BeginWithTransition{},
      m_ContinueSection{},
      m_EndCapLocation{0},
      m_EndCapPoint{},
      m_EndCapGroup{},
      m_OriginalPreviousGroupDisplayed{true},
      m_OriginalPreviousGroup{},
      m_PreviousSection{0.125, 0.0625, Section::Rectangular},
      m_CurrentSection{0.125, 0.0625, Section::Rectangular},

      /// Pipe Mode Interface
      m_CurrentPipeSymbolIndex{},
      m_PipeTicSize{0.03125},
      m_PipeRiseDropRadius{0.03125},

      /// Power Mode Interface (initializers)
      m_PowerArrow{},
      m_PowerConductor{},
      m_PowerConductorSpacing{0.04},
      m_CircuitEndPoint{},
      m_PreviousRadius{} {
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
inline AeSysDoc* AeSysView::GetDocument() const {
#ifdef _DEBUG
  auto* document = dynamic_cast<AeSysDoc*>(m_pDocument);
  assert(document != nullptr && "Invalid document type in AeSysView::GetDocument()");
  return document;
#else _DEBUG  // debug version in PegView.cpp
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
  }
  CMFCPropertyGridProperty& ActiveViewScaleProperty = MainFrame->GetPropertiesPane().GetActiveViewScaleProperty();
  ActiveViewScaleProperty.SetValue(m_WorldScale);
  ActiveViewScaleProperty.Enable(activate);

  CView::OnActivateView(activate, activateView, deactiveView);
}

BOOL AeSysView::PreCreateWindow(CREATESTRUCT& createStructure) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::PreCreateWindow(%08.8lx) ", this, createStructure);

  // TODO: Modify the Window class or styles here by modifying the CREATESTRUCT
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

void AeSysView::OnDraw(CDC* deviceContext) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnDraw(%08.8lx) +", this, deviceContext);

  CRect Rect;
  deviceContext->GetClipBox(Rect);
  ATLTRACE2(traceGeneral, 3, L" ClipBox(%i, %i, %i, %i)\n", Rect.left, Rect.top, Rect.right, Rect.bottom);

  if (Rect.IsRectEmpty()) { return; }

  try {
#if defined(USING_Direct2D)
    HRESULT hr = S_OK;

    hr = CreateDeviceResources();

    if (SUCCEEDED(hr)) {
      m_RenderTarget->BeginDraw();
      m_RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
      m_RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));

      D2D1_SIZE_F rtSize = m_RenderTarget->GetSize();

      m_RenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_ALIASED);

      D2D1_RECT_F rectangle1 = D2D1::RectF(
          rtSize.width / 2 - 50.0f, rtSize.height / 2 - 50.0f, rtSize.width / 2 + 50.0f, rtSize.height / 2 + 50.0f);
      D2D1_RECT_F rectangle2 = D2D1::RectF(
          rtSize.width / 2 - 100.0f, rtSize.height / 2 - 100.0f, rtSize.width / 2 + 100.0f, rtSize.height / 2 + 100.0f);

      m_RenderTarget->FillRectangle(&rectangle1, m_RedBrush);
      m_RenderTarget->DrawRectangle(&rectangle2, m_LightSlateGrayBrush);

      hr = m_RenderTarget->EndDraw();
    }
    if (hr == D2DERR_RECREATE_TARGET) {
      hr = S_OK;
      DiscardDeviceResources();
    }
#endif

    auto* document = GetDocument();
    assert(document != nullptr);
    if (m_ViewRendered) {
    } else {
      BackgroundImageDisplay(deviceContext);
      DisplayGrid(deviceContext);
#if defined(USING_STATE_PATTERN)
      // Delegate core drawing to the current state (e.g., for mode-specific overlays or primitives)
      auto* state = GetCurrentState();
      if (state) {
        state->OnDraw(this, deviceContext);
      } else {
        // Fallback to existing drawing if no state (e.g., during init)
        document->DisplayAllLayers(this, deviceContext);
        document->DisplayUniquePoints();
      }
#else
        document->DisplayAllLayers(this, deviceContext);
        document->DisplayUniquePoints();
#endif
    }
    UpdateStateInformation(All);
    ModeLineDisplay();
    ValidateRect(nullptr);
  } catch (CException* e) { e->Delete(); }
}

void AeSysView::OnInitialUpdate() {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnInitialUpdate()\n", this);

  SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR)::CreateSolidBrush(Eo::ViewBackgroundColor));

#if defined(USING_Direct2D)
  m_RenderTarget = nullptr;
  CreateDeviceResources();
#endif

  CView::OnInitialUpdate();
#if defined(USING_STATE_PATTERN)
  PushState(std::make_unique<IdleState>());
#endif
  OnModeDraw();
}

/** @brief Helper function to display content based on the provided hint, used by OnUpdate for delegation
* This centralizes the logic for interpreting hints and displaying the appropriate content, allowing OnUpdate to focus on setup/delegation/cleanup
* @param sender The view that sent the update notification
* @param hint A bitmask hint indicating what type of content needs to be updated (e.g., primitive, group, layer)
* @param hintObject The specific object related to the hint (e.g., the primitive or group that changed)
* @param deviceContext The device context to use for drawing
*/
void AeSysView::DisplayUsingHint(CView* sender, LPARAM hint, CObject* hintObject, CDC* deviceContext) {
  switch (hint) {
    case EoDb::kPrimitive:
    case EoDb::kPrimitiveSafe:
    case EoDb::kPrimitiveEraseSafe:
      static_cast<EoDbPrimitive*>(hintObject)->Display(this, deviceContext);
      break;

    case EoDb::kGroup:
    case EoDb::kGroupSafe:
    case EoDb::kGroupEraseSafe:
    case EoDb::kGroupSafeTrap:
    case EoDb::kGroupEraseSafeTrap:
      static_cast<EoDbGroup*>(hintObject)->Display(this, deviceContext);
      break;

    case EoDb::kGroups:
    case EoDb::kGroupsSafe:
    case EoDb::kGroupsSafeTrap:
    case EoDb::kGroupsEraseSafeTrap:
      static_cast<EoDbGroupList*>(hintObject)->Display(this, deviceContext);
      break;

    case EoDb::kLayer:
    case EoDb::kLayerErase:
      static_cast<EoDbLayer*>(hintObject)->Display(this, deviceContext);
      break;

    default:
      CView::OnUpdate(sender, hint, hintObject);
  }
}

void AeSysView::OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnUpdate(%p, %p, %p)\n", this, sender, hint, hintObject);

  // Pre-delegation setup: Configure device context based on hint (e.g., for safe/erase/trap rendering)
  auto* deviceContext = GetDC();
  auto backgroundColor = deviceContext->GetBkColor();
  deviceContext->SetBkColor(Eo::ViewBackgroundColor);
  int savedRenderState{};
  int drawMode{};
  if ((hint & EoDb::kSafe) == EoDb::kSafe) { savedRenderState = renderState.Save(); }
  if ((hint & EoDb::kErase) == EoDb::kErase) { drawMode = renderState.SetROP2(deviceContext, R2_XORPEN); }
  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialColor(app.TrapHighlightColor()); }

  bool isHandledByState{};
#if defined(USING_STATE_PATTERN)
  // Core delegation: Give the current state first crack at handling the update
  auto* state = GetCurrentState();
  if (state) { isHandledByState = state->OnUpdate(this, sender, hint, hintObject); }
#endif
  if (!isHandledByState) { DisplayUsingHint(sender, hint, hintObject, deviceContext); }

  // Post-delegation cleanup: Restore any modified device context settings
  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialColor(0); }
  if ((hint & EoDb::kErase) == EoDb::kErase) { renderState.SetROP2(deviceContext, drawMode); }
  if ((hint & EoDb::kSafe) == EoDb::kSafe) { renderState.Restore(deviceContext, savedRenderState); }
  deviceContext->SetBkColor(backgroundColor);
  ReleaseDC(deviceContext);
}

void AeSysView::OnBeginPrinting(CDC* deviceContext, CPrintInfo* pInfo) {
  ViewportPushActive();
  PushViewTransform();

  int HorizontalPixelWidth = deviceContext->GetDeviceCaps(HORZRES);
  int VerticalPixelWidth = deviceContext->GetDeviceCaps(VERTRES);

  SetViewportSize(HorizontalPixelWidth, VerticalPixelWidth);

  double HorizontalSize = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE));
  double VerticalSize = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE));

  SetDeviceWidthInInches(HorizontalSize / Eo::MmPerInch);
  SetDeviceHeightInInches(VerticalSize / Eo::MmPerInch);

  if (m_Plot) {
    UINT HorizontalPages;
    UINT VerticalPages;
    pInfo->SetMaxPage(NumPages(deviceContext, m_PlotScaleFactor, HorizontalPages, VerticalPages));
  } else {
    m_ViewTransform.AdjustWindow(static_cast<double>(VerticalPixelWidth) / static_cast<double>(HorizontalPixelWidth));
  }
}

void AeSysView::OnEndPrinting([[maybe_unused]] CDC* deviceContext, [[maybe_unused]] CPrintInfo* printInformation) {
  PopViewTransform();
  ViewportPopActive();
}

BOOL AeSysView::OnPreparePrinting(CPrintInfo* pInfo) {
  if (m_Plot) {
    CPrintInfo pi;
    if (AfxGetApp()->GetPrinterDeviceDefaults(&pi.m_pPD->m_pd)) {
      HDC hDC = pi.m_pPD->m_pd.hDC;
      if (hDC == nullptr) { hDC = pi.m_pPD->CreatePrinterDC(); }
      if (hDC != nullptr) {
        UINT nHorzPages;
        UINT nVertPages;
        CDC DeviceContext;
        DeviceContext.Attach(hDC);
        pInfo->SetMaxPage(NumPages(&DeviceContext, m_PlotScaleFactor, nHorzPages, nVertPages));
        ::DeleteDC(DeviceContext.Detach());
      }
    }
  }
  return DoPreparePrinting(pInfo);
}

void AeSysView::OnPrepareDC(CDC* deviceContext, CPrintInfo* pInfo) {
  CView::OnPrepareDC(deviceContext, pInfo);

  if (deviceContext->IsPrinting()) {
    if (m_Plot) {
      double HorizontalSizeInInches =
          static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;
      double VerticalSizeInInches =
          static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;

      m_ViewTransform.Initialize(m_Viewport);
      m_ViewTransform.SetWindow(0.0, 0.0, HorizontalSizeInInches, VerticalSizeInInches);

      UINT nHorzPages;
      UINT nVertPages;

      NumPages(deviceContext, m_PlotScaleFactor, nHorzPages, nVertPages);

      double dX = ((pInfo->m_nCurPage - 1) % nHorzPages) * HorizontalSizeInInches;
      double dY = ((pInfo->m_nCurPage - 1) / nHorzPages) * VerticalSizeInInches;

      m_ViewTransform.SetTarget(EoGePoint3d(dX, dY, 0.0));
      m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitZ);
      m_ViewTransform.BuildTransformMatrix();
    } else {
    }
  }
}

// Window messages ////////////////////////////////////////////////////////////
void AeSysView::OnContextMenu(CWnd*, CPoint point) { app.ShowPopupMenu(IDR_CONTEXT_MENU, point, this); }

BOOL AeSysView::OnEraseBkgnd(CDC* deviceContext) {
  // TODO: Add your message handler code here and/or call default

  return __super::OnEraseBkgnd(deviceContext);
}

void AeSysView::DoCustomMouseClick(const CString& characters) {
  int Position = 0;

  while (Position < characters.GetLength()) {
    if (characters.Find(L'{', Position) == Position) {
      Position++;
      CString VirtualKey = characters.Tokenize(L"}", Position);
      PostMessageW(WM_KEYDOWN, static_cast<WPARAM>(_wtoi(VirtualKey)), 0L);
    } else {
      PostMessageW(WM_CHAR, characters[Position++], 0L);
    }
  }
}
#if defined(USING_STATE_PATTERN)
BOOL AeSysView::PreTranslateMessage(MSG* pMsg) {
  if (pMsg->message == WM_KEYDOWN) {
    auto* state = GetCurrentState();
    if (state && state->HandleKeypad(this, static_cast<UINT>(pMsg->wParam), 1, static_cast<UINT>(pMsg->lParam))) {
      return TRUE;  // Handled by state
    }
  }
  return CView::PreTranslateMessage(pMsg);
}
#endif

void AeSysView::OnLButtonDown(UINT flags, CPoint point) {
#if defined(USING_STATE_PATTERN)
  auto* state = GetCurrentState();
  if (state) {
    state->OnLButtonDown(this, flags, point);
    /*if ( handled )*/ { return; }
  }
  // Fallback to existing logic
#endif
  if (app.CustomLButtonDownCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnLButtonDown(flags, point);
  } else {
    DoCustomMouseClick(app.CustomLButtonDownCharacters);
  }
}

void AeSysView::OnLButtonUp(UINT flags, CPoint point) {
  if (app.CustomLButtonUpCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnLButtonUp(flags, point);
  } else {
    DoCustomMouseClick(app.CustomLButtonUpCharacters);
  }
}

void AeSysView::OnRButtonDown(UINT flags, CPoint point) {
  if (app.CustomRButtonDownCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnRButtonDown(flags, point);
  } else {
    DoCustomMouseClick(app.CustomRButtonDownCharacters);
  }
}

void AeSysView::OnRButtonUp(UINT flags, CPoint point) {
  if (app.CustomRButtonUpCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnRButtonUp(flags, point);
  } else {
    DoCustomMouseClick(app.CustomRButtonUpCharacters);
  }
}

void AeSysView::OnMButtonDown([[maybe_unused]] UINT flags, CPoint point) {
  m_middleButtonPanStartPoint = point;
  m_middleButtonPanInProgress = true;
  SetCapture();
}

void AeSysView::OnMButtonUp([[maybe_unused]] UINT flags, [[maybe_unused]] CPoint point) {
  if (m_middleButtonPanInProgress) {
    m_middleButtonPanInProgress = false;
    ReleaseCapture();
  }
}

void AeSysView::OnMouseMove([[maybe_unused]] UINT flags, CPoint point) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView::OnMouseMove - flags: %u, point: (%d, %d)\n", flags, point.x, point.y);
#if defined(USING_STATE_PATTERN)
  auto* state = GetCurrentState();
  if (state) { state->OnMouseMove(this, flags, point); }
#endif
  if (m_middleButtonPanInProgress) {
    auto delta = point - m_middleButtonPanStartPoint;
    m_middleButtonPanStartPoint = point;

    auto target = m_ViewTransform.Target();

    // Convert delta to world coordinates (scale as needed)
    target.x += static_cast<double>(-delta.cx) * m_ViewTransform.UExtent() / m_Viewport.Width();
    target.y += static_cast<double>(delta.cy) * m_ViewTransform.VExtent() / m_Viewport.Height();

    m_ViewTransform.SetTarget(target);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    InvalidateRect(nullptr, TRUE);  // Redraw view
  }
  DisplayOdometer();

  switch (app.CurrentMode()) {
    case ID_MODE_ANNOTATE:
      DoAnnotateModeMouseMove();
      break;

    case ID_MODE_DRAW:
      DoDrawModeMouseMove();
      break;

    case ID_MODE_DRAW2:
      DoDraw2ModeMouseMove();
      break;

    case ID_MODE_LPD:
      DoDuctModeMouseMove();
      break;

    case ID_MODE_NODAL:
      DoNodalModeMouseMove();
      break;

    case ID_MODE_PIPE:
      DoPipeModeMouseMove();
      break;

    case ID_MODE_POWER:
      DoPowerModeMouseMove();
      break;

    case ID_MODE_PRIMITIVE_EDIT:
      PreviewPrimitiveEdit();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      PreviewMendPrimitive();
      break;

    case ID_MODE_GROUP_EDIT:
      PreviewGroupEdit();
      break;
  }
  if (m_rubberbandType == Lines) {
    auto* deviceContext = GetDC();
    auto drawMode = deviceContext->SetROP2(R2_XORPEN);
    CPen grayPen(PS_SOLID, 0, Eo::colorRubberband);
    auto* pen = deviceContext->SelectObject(&grayPen);

    deviceContext->MoveTo(m_rubberbandLogicalBegin);
    deviceContext->LineTo(m_rubberbandLogicalEnd);

    m_rubberbandLogicalEnd = point;
    deviceContext->MoveTo(m_rubberbandLogicalBegin);
    deviceContext->LineTo(m_rubberbandLogicalEnd);
    deviceContext->SelectObject(pen);
    deviceContext->SetROP2(drawMode);
    ReleaseDC(deviceContext);
  } else if (m_rubberbandType == Rectangles) {
    auto* deviceContext = GetDC();
    auto drawMode = deviceContext->SetROP2(R2_XORPEN);
    CPen grayPen(PS_SOLID, 0, Eo::colorRubberband);
    auto* pen = deviceContext->SelectObject(&grayPen);
    auto* brush = deviceContext->SelectStockObject(NULL_BRUSH);

    deviceContext->Rectangle(
        m_rubberbandLogicalBegin.x, m_rubberbandLogicalBegin.y, m_rubberbandLogicalEnd.x, m_rubberbandLogicalEnd.y);

    m_rubberbandLogicalEnd = point;
    deviceContext->Rectangle(
        m_rubberbandLogicalBegin.x, m_rubberbandLogicalBegin.y, m_rubberbandLogicalEnd.x, m_rubberbandLogicalEnd.y);
    deviceContext->SelectObject(brush);
    deviceContext->SelectObject(pen);
    deviceContext->SetROP2(drawMode);
    ReleaseDC(deviceContext);
  }
}

BOOL AeSysView::OnMouseWheel(UINT flags, std::int16_t zDelta, CPoint point) {
  ATLTRACE2(traceGeneral, 1, L"AeSysView<%p>OnMouseWheel(%i, %i, %08.8lx)\n", this, flags, zDelta, point);

  if (zDelta > 0) {
    OnWindowZoomIn();
  } else {
    OnWindowZoomOut();
  }
  return __super::OnMouseWheel(flags, zDelta, point);
}

void AeSysView::OnSize(UINT type, int cx, int cy) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>OnSize(%i, %i, %i)\n", this, type, cx, cy);

  if (cx && cy) {
    SetViewportSize(cx, cy);
    m_ViewTransform.Initialize(m_Viewport);
#if defined(USING_Direct2D)
    if (m_RenderTarget) { m_RenderTarget->Resize(D2D1::SizeU(cx, cy)); }
#endif
    m_OverviewViewTransform = m_ViewTransform;
  }
}

void AeSysView::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(traceGeneral, 3, L"AeSysView<%p>::OnTimer(%i)\n", this, nIDEvent);

  CView::OnTimer(nIDEvent);
}

///////////////////////////////////////////////////////////////////////////////

CMFCStatusBar& AeSysView::GetStatusBar() const { return (static_cast<CMainFrame*>(AfxGetMainWnd()))->GetStatusBar(); }

void AeSysView::PopViewTransform() {
  if (!m_ViewTransforms.IsEmpty()) { m_ViewTransform = m_ViewTransforms.RemoveTail(); }
  m_ViewTransform.BuildTransformMatrix();
}

void AeSysView::PushViewTransform() { m_ViewTransforms.AddTail(m_ViewTransform); }

void AeSysView::ModelViewAdjustWindow(double& uMin, double& vMin, double& uMax, double& vMax, double ratio) {
  double AspectRatio = static_cast<double>(m_Viewport.Height() / m_Viewport.Width());

  double UExtent = std::abs(static_cast<double>(uMax - uMin));
  double VExtent = std::abs(static_cast<double>(vMax - vMin));

  double XAdjustment = 0.0;
  double YAdjustment = 0.0;

  double Scale = 1.0 - (m_Viewport.WidthInInches() / UExtent) / ratio;

  if (std::abs(Scale) > Eo::geometricTolerance) {
    XAdjustment = Scale * UExtent;
    YAdjustment = Scale * VExtent;
  }
  if (UExtent < Eo::geometricTolerance || VExtent / UExtent > AspectRatio) {
    XAdjustment += (VExtent / AspectRatio - UExtent) * 0.5;
  } else {
    YAdjustment += (UExtent * AspectRatio - VExtent) * 0.5f;
  }
  uMin -= XAdjustment;
  uMax += XAdjustment;
  vMin -= YAdjustment;
  vMax += YAdjustment;
}

void AeSysView::PushModelTransform() { m_ModelTransform.Push(); }

void AeSysView::SetLocalModelTransform(EoGeTransformMatrix& transformation) {
  m_ModelTransform.SetLocalTM(transformation);
}

void AeSysView::PopModelTransform() { m_ModelTransform.Pop(); }

void AeSysView::BackgroundImageDisplay(CDC* deviceContext) {
  if (!m_viewBackgroundImage || (static_cast<HBITMAP>(m_backgroundImageBitmap) == 0)) { return; }

  int iWidDst = int(m_Viewport.Width());
  int iHgtDst = int(m_Viewport.Height());

  BITMAP bm{};
  m_backgroundImageBitmap.GetBitmap(&bm);
  CDC dcMem;
  dcMem.CreateCompatibleDC(nullptr);
  CBitmap* pBitmap = dcMem.SelectObject(&m_backgroundImageBitmap);
  CPalette* pPalette = deviceContext->SelectPalette(&m_backgroundImagePalette, FALSE);
  deviceContext->RealizePalette();

  auto Target = m_ViewTransform.Target();
  auto ptTargetOver = m_OverviewViewTransform.Target();
  double dU = Target.x - ptTargetOver.x;
  double dV = Target.y - ptTargetOver.y;

  // Determine the region of the bitmap to tranfer to display
  CRect rcWnd;
  rcWnd.left =
      Eo::Round((m_ViewTransform.UMin() - OverviewUMin() + dU) / OverviewUExt() * static_cast<double>(bm.bmWidth));
  rcWnd.top = Eo::Round(
      (1.0 - (m_ViewTransform.VMax() - OverviewVMin() + dV) / OverviewVExt()) * static_cast<double>(bm.bmHeight));
  rcWnd.right =
      Eo::Round((m_ViewTransform.UMax() - OverviewUMin() + dU) / OverviewUExt() * static_cast<double>(bm.bmWidth));
  rcWnd.bottom = Eo::Round(
      (1.0 - (m_ViewTransform.VMin() - OverviewVMin() + dV) / OverviewVExt()) * static_cast<double>(bm.bmHeight));

  int iWidSrc = rcWnd.Width();
  int iHgtSrc = rcWnd.Height();

  deviceContext->StretchBlt(0, 0, iWidDst, iHgtDst, &dcMem, (int)rcWnd.left, (int)rcWnd.top, iWidSrc, iHgtSrc, SRCCOPY);

  dcMem.SelectObject(pBitmap);
  deviceContext->SelectPalette(pPalette, FALSE);
}

void AeSysView::ViewportPopActive() {
  if (!m_Viewports.IsEmpty()) { m_Viewport = m_Viewports.RemoveTail(); }
}

void AeSysView::ViewportPushActive() { m_Viewports.AddTail(m_Viewport); }
// AeSysView printing

void AeSysView::OnFilePlotFull() {
  m_Plot = true;
  m_PlotScaleFactor = 1.0f;
  CView::OnFilePrint();
}

void AeSysView::OnFilePlotHalf() {
  m_Plot = true;
  m_PlotScaleFactor = 0.5f;
  CView::OnFilePrint();
}

void AeSysView::OnFilePlotQuarter() {
  m_Plot = true;
  m_PlotScaleFactor = 0.25f;
  CView::OnFilePrint();
}

void AeSysView::OnFilePrint() {
  m_Plot = false;
  m_PlotScaleFactor = 1.0f;
  CView::OnFilePrint();
}

UINT AeSysView::NumPages(CDC* deviceContext, double scaleFactor, UINT& horizontalPages, UINT& verticalPages) {
  EoGePoint3d ptMin;
  EoGePoint3d ptMax;
  EoGeTransformMatrix transformMatrix;
  auto* document = GetDocument();

  document->GetExtents(this, ptMin, ptMax, transformMatrix);

  double HorizontalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch;
  double VerticalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch;

  horizontalPages = static_cast<UINT>(Eo::Round(((ptMax.x - ptMin.x) * scaleFactor / HorizontalSizeInInches) + 0.5f));
  verticalPages = static_cast<UINT>(Eo::Round(((ptMax.y - ptMin.y) * scaleFactor / VerticalSizeInInches) + 0.5f));

  return horizontalPages * verticalPages;
}

void AeSysView::DisplayPixel(CDC* deviceContext, COLORREF cr, const EoGePoint3d& point) {
  EoGePoint4d ndcPoint(point);

  ModelViewTransformPoint(ndcPoint);

  if (ndcPoint.IsInView()) { deviceContext->SetPixel(ProjectToClient(ndcPoint), cr); }
}

void AeSysView::DoCameraRotate(int rotationDirection) {
  try {
    auto normal = m_ViewTransform.Position() - m_ViewTransform.Target();
    normal.Normalize();

    auto u = CrossProduct(ViewUp(), normal);
    u.Normalize();

    auto v = CrossProduct(normal, u);
    v.Normalize();

    auto position = m_ViewTransform.Position();
    auto target = m_ViewTransform.Target();
    switch (rotationDirection) {
      case ID_CAMERA_ROTATELEFT:
        position = position.RotateAboutAxis(target, v, Eo::DegreeToRadian(-10.0));
        break;
      case ID_CAMERA_ROTATERIGHT:
        position = position.RotateAboutAxis(target, v, Eo::DegreeToRadian(10.0));
        break;

      case ID_CAMERA_ROTATEUP:
        position = position.RotateAboutAxis(target, u, Eo::DegreeToRadian(-10.0));
        break;

      case ID_CAMERA_ROTATEDOWN:
        position = position.RotateAboutAxis(target, u, Eo::DegreeToRadian(10.0));
        break;
    }
    m_ViewTransform.SetPosition(position);
    m_ViewTransform.SetViewUp(v);
    m_ViewTransform.BuildTransformMatrix();
    InvalidateRect(nullptr, TRUE);
  } catch (const std::domain_error& error) {
    ::MessageBoxA(nullptr, error.what(), "Camera Rotate Error", MB_ICONWARNING | MB_OK);
    return;
  }
}

void AeSysView::DoWindowPan(double ratio) {
  ratio = std::min(std::max(ratio, minimumWindowRatio), maximumWindowRatio);

  double UExtent = m_Viewport.WidthInInches() / ratio;
  double VExtent = m_Viewport.HeightInInches() / ratio;

  m_ViewTransform.SetCenteredWindow(m_Viewport, UExtent, VExtent);

  auto cursorPosition = GetCursorPosition();

  auto direction = m_ViewTransform.Direction();
  auto target = m_ViewTransform.Target();

  EoGeLine::IntersectionWithPln(cursorPosition, direction, target, direction, &cursorPosition);

  m_ViewTransform.SetTarget(target);
  m_ViewTransform.SetPosition(direction);
  m_ViewTransform.BuildTransformMatrix();

  SetCursorPosition(cursorPosition);
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnSetupScale() {
  EoDlgSetScale dlg;
  dlg.m_Scale = GetWorldScale();
  if (dlg.DoModal() == IDOK) { SetWorldScale(dlg.m_Scale); }
}

void AeSysView::On3dViewsTop() {
  m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetDirection(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitY);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsBottom() {
  m_ViewTransform.SetPosition(-EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetDirection(-EoGeVector3d::positiveUnitZ);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitY);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsLeft() {
  m_ViewTransform.SetPosition(-EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetDirection(-EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsRight() {
  m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetDirection(EoGeVector3d::positiveUnitX);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsFront() {
  m_ViewTransform.SetPosition(-EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetDirection(-EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsBack() {
  m_ViewTransform.SetPosition(EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetDirection(EoGeVector3d::positiveUnitY);
  m_ViewTransform.SetViewUp(EoGeVector3d::positiveUnitZ);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsIsometric() {
  static int iLeftRight = 0;
  static int iFrontBack = 0;
  static int iAboveUnder = 0;

  EoDlgSelectIsometricView Dialog;
  Dialog.m_LeftRight = iLeftRight;
  Dialog.m_FrontBack = iFrontBack;
  Dialog.m_AboveUnder = iAboveUnder;
  if (Dialog.DoModal()) {
    iLeftRight = Dialog.m_LeftRight;
    iFrontBack = Dialog.m_FrontBack;
    iAboveUnder = Dialog.m_AboveUnder;

    EoGeVector3d Direction;

    Direction.x = iLeftRight == 0 ? 0.5773503 : -0.5773503;
    Direction.y = iFrontBack == 0 ? 0.5773503 : -0.5773503;
    Direction.z = iAboveUnder == 0 ? -0.5773503 : 0.5773503;

    m_ViewTransform.SetPosition(-Direction);
    m_ViewTransform.SetDirection(-Direction);
    m_ViewTransform.EnablePerspective(false);

    auto viewUp = CrossProduct(Direction, EoGeVector3d::positiveUnitZ);
    viewUp = CrossProduct(viewUp, Direction);
    viewUp.Normalize();

    m_ViewTransform.SetViewUp(viewUp);
    m_ViewTransform.SetCenteredWindow(m_Viewport, 0.0, 0.0);
  }
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnCameraRotateLeft() { DoCameraRotate(ID_CAMERA_ROTATELEFT); }

void AeSysView::OnCameraRotateRight() { DoCameraRotate(ID_CAMERA_ROTATERIGHT); }

void AeSysView::OnCameraRotateUp() { DoCameraRotate(ID_CAMERA_ROTATEUP); }

void AeSysView::OnCameraRotateDown() { DoCameraRotate(ID_CAMERA_ROTATEDOWN); }

void AeSysView::OnViewParameters() {
  EoDlgViewParameters Dialog;

  EoGsViewTransform ModelView(m_ViewTransform);

  Dialog.m_ModelView = reinterpret_cast<uintptr_t>(&ModelView);
  Dialog.m_PerspectiveProjection = m_ViewTransform.IsPerspectiveOn();

  if (Dialog.DoModal() == IDOK) { m_ViewTransform.EnablePerspective(Dialog.m_PerspectiveProjection == TRUE); }
}

void AeSysView::OnViewLighting() {}

void AeSysView::OnViewRendered() {
  m_ViewRendered = !m_ViewRendered;
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnViewTrueTypeFonts() {
  m_ViewTrueTypeFonts = !m_ViewTrueTypeFonts;
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnViewPenWidths() {
  m_ViewPenWidths = !m_ViewPenWidths;
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnViewSolid() {}

void AeSysView::OnViewWindow() {
  CPoint CurrentPosition;
  ::GetCursorPos(&CurrentPosition);
  HMENU WindowMenu = ::LoadMenu(AeSys::GetInstance(), MAKEINTRESOURCE(IDR_WINDOW));
  CMenu* SubMenu = CMenu::FromHandle(::GetSubMenu(WindowMenu, 0));
  SubMenu->TrackPopupMenuEx(TPM_LEFTALIGN, CurrentPosition.x, CurrentPosition.y, AfxGetMainWnd(), 0);
  ::DestroyMenu(WindowMenu);
}

void AeSysView::OnViewWireframe() {
  m_ViewWireframe = !m_ViewWireframe;
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnViewWindowKeyplan() {
  EoDlgActiveViewKeyplan dlg(this);
  dlg.m_dRatio = m_Viewport.WidthInInches() / m_ViewTransform.UExtent();

  if (dlg.DoModal() == IDOK) { InvalidateRect(nullptr, TRUE); }
}

void AeSysView::OnViewOdometer() {
  m_ViewOdometer = !m_ViewOdometer;
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnViewRefresh() { InvalidateRect(nullptr, TRUE); }

void AeSysView::OnUpdateViewRendered(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewRendered); }

void AeSysView::OnUpdateViewWireframe(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewWireframe); }

void AeSysView::OnWindowNormal() {
  CopyActiveModelViewToPreviousModelView();
  DoWindowPan(1.0);
}

void AeSysView::OnWindowBest() {
  auto* document = GetDocument();
  EoGePoint3d ptMin;
  EoGePoint3d ptMax;

  document->GetExtents(this, ptMin, ptMax, ModelViewGetMatrix());

  // extents return range - 1 to 1

  if (ptMin.x < ptMax.x) {
    m_PreviousViewTransform = m_ViewTransform;

    double UExtent = m_ViewTransform.UExtent() * (ptMax.x - ptMin.x) / 2.0;
    double VExtent = m_ViewTransform.VExtent() * (ptMax.y - ptMin.y) / 2.0;

    m_ViewTransform.SetCenteredWindow(m_Viewport, UExtent, VExtent);

    EoGeTransformMatrix transformMatrix;
    document->GetExtents(this, ptMin, ptMax, transformMatrix);

    EoGePoint3d Target = EoGePoint3d((ptMin.x + ptMax.x) / 2.0, (ptMin.y + ptMax.y) / 2.0, (ptMin.z + ptMax.z) / 2.0);

    m_ViewTransform.SetTarget(Target);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    ViewZoomExtents();

    SetCursorPosition(Target);
    InvalidateRect(nullptr, TRUE);
  }
}

void AeSysView::OnWindowLast() {
  ExchangeActiveAndPreviousModelViews();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowSheet() {
  ModelViewInitialize();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowZoomIn() { DoWindowPan(m_Viewport.WidthInInches() / m_ViewTransform.UExtent() / 0.9); }

void AeSysView::OnWindowZoomOut() { DoWindowPan(m_Viewport.WidthInInches() / m_ViewTransform.UExtent() * 0.9); }

void AeSysView::OnWindowPan() {
  CopyActiveModelViewToPreviousModelView();
  DoWindowPan(m_Viewport.WidthInInches() / m_ViewTransform.UExtent());
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowPanLeft() {
  auto Target = m_ViewTransform.Target();

  Target.x -= 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowPanRight() {
  auto Target = m_ViewTransform.Target();

  Target.x += 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowPanUp() {
  auto Target = m_ViewTransform.Target();

  Target.y += 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowPanDown() {
  auto Target = m_ViewTransform.Target();

  Target.y -= 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowZoomSpecial() {
  EoDlgViewZoom dlg(this);

  dlg.m_Ratio = m_Viewport.WidthInInches() / m_ViewTransform.UExtent();

  if (dlg.DoModal() == IDOK) {
    CopyActiveModelViewToPreviousModelView();
    DoWindowPan(dlg.m_Ratio);
    InvalidateRect(nullptr, TRUE);
  }
}

void AeSysView::OnSetupDimLength() {
  EoDlgSetLength dialog;

  dialog.SetTitle(L"Set Dimension Length");
  dialog.SetLength(app.DimensionLength());
  if (dialog.DoModal() == IDOK) {
    app.SetDimensionLength(dialog.Length());
    UpdateStateInformation(DimLen);
#if defined(USING_DDE)
    dde::PostAdvise(dde::DimLenInfo);
#endif
  }
}

void AeSysView::OnSetupDimAngle() {
  EoDlgSetAngle dlg;

  dlg.m_strTitle = L"Set Dimension Angle";
  dlg.m_dAngle = app.DimensionAngle();
  if (dlg.DoModal() == IDOK) {
    app.SetDimensionAngle(dlg.m_dAngle);
    UpdateStateInformation(DimAng);
#if defined(USING_DDE)
    dde::PostAdvise(dde::DimAngZInfo);
#endif  // USING_DDE
  }
}

void AeSysView::OnSetupUnits() {
  EoDlgSetUnitsAndPrecision Dialog;
  Dialog.m_Units = app.GetUnits();
  Dialog.m_Precision = app.GetArchitecturalUnitsFractionPrecision();

  if (Dialog.DoModal() == IDOK) {
    app.SetUnits(Dialog.m_Units);
    app.SetArchitecturalUnitsFractionPrecision(Dialog.m_Precision);
  }
}

void AeSysView::OnSetupConstraints() {
  EoDlgSetupConstraints Dialog(this);

  if (Dialog.DoModal() == IDOK) { UpdateStateInformation(All); }
}

void AeSysView::OnSetupMouseButtons() {
  EoDlgSetupCustomMouseCharacters dialog;
  if (dialog.DoModal() == IDOK) {}
}

void AeSysView::OnRelativeMovesEngDown() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngDownRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngIn() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z -= app.EngagedLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngLeft() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngLeftRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x -= app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngOut() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z += app.EngagedLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngRight() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngRightRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngUp() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle());
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesEngUpRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y += app.EngagedLength();
  cursorPosition = ptSec.RotateAboutAxis(
      cursorPosition, EoGeVector3d::positiveUnitZ, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesRight() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.x += app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesUp() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.y += app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesLeft() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.x -= app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesDown() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.y -= app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesIn() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z -= app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesOut() {
  auto cursorPosition = GetCursorPosition();
  cursorPosition.z += app.DimensionLength();
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesRightRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x += app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesUpRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y += app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesLeftRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.x -= app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnRelativeMovesDownRotate() {
  auto cursorPosition = GetCursorPosition();
  EoGePoint3d ptSec = cursorPosition;
  ptSec.y -= app.DimensionLength();
  cursorPosition =
      ptSec.RotateAboutAxis(cursorPosition, EoGeVector3d::positiveUnitZ, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(cursorPosition);
}

void AeSysView::OnToolsPrimitiveSnapto() {
  auto cursorPosition = GetCursorPosition();

  EoGePoint3d ptDet;

  if (GroupIsEngaged()) {
    auto* primitive = m_EngagedPrimitive;

    EoGePoint4d ptView(cursorPosition);
    ModelViewTransformPoint(ptView);

    EoDbPolygon::EdgeToEvaluate() = EoDbPolygon::Edge();

    if (primitive->SelectUsingPoint(this, ptView, ptDet)) {
      ptDet = primitive->GoToNextControlPoint();
      m_ptDet = ptDet;

      primitive->AddReportToMessageList(ptDet);
      SetCursorPosition(ptDet);
      return;
    }
  }
  if (SelectGroupAndPrimitive(cursorPosition) != nullptr) {
    ptDet = m_ptDet;
    m_EngagedPrimitive->AddReportToMessageList(ptDet);
    SetCursorPosition(ptDet);
  }
}

void AeSysView::OnPrimPerpJump() {
  auto cursorPosition = GetCursorPosition();

  if (SelectGroupAndPrimitive(cursorPosition) != nullptr) {
    if (m_EngagedPrimitive->Is(EoDb::kLinePrimitive)) {
      auto* engagedLine = static_cast<EoDbLine*>(m_EngagedPrimitive);
      cursorPosition = engagedLine->ProjectPointToLine(m_ptCursorPosWorld);
      SetCursorPosition(cursorPosition);
    }
  }
}

void AeSysView::OnHelpKey() { ::WinHelpW(GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"READY")); }

/** @brief Retrieves the active view in the MDI application.
 * @note This function assumes that the main window is a CMDIFrameWndEx and that the active child window is a CMDIChildWndEx containing an AeSysView.
 * @return A pointer to the active AeSysView, or nullptr if no active view is found.
 */
AeSysView* AeSysView::GetActiveView() {
  auto* frameWindow = dynamic_cast<CMDIFrameWndEx*>(AfxGetMainWnd());
  if (frameWindow == nullptr) {
    // Legitimately nullptr during CMainFrame::OnCreate() - not an error
    return nullptr;
  }
  auto* childWindow = dynamic_cast<CMDIChildWndEx*>(frameWindow->MDIGetActive());
#ifdef _DEBUG
  assert(childWindow != nullptr &&
         "No active MDI child window - should always have an active document after initialization");
#endif
  if (childWindow == nullptr) { return nullptr; }

  auto* view = childWindow->GetActiveView();
#ifdef _DEBUG
  assert(view != nullptr && "Active MDI child has no view - this indicates a framework error");
#endif
  if (view == nullptr) { return nullptr; }

  auto* activeView = dynamic_cast<AeSysView*>(view);
#ifdef _DEBUG
  assert(activeView != nullptr &&
         "Active view is not an AeSysView (possible splitter windows or multi-view configuration");
#endif
  return activeView;
}

void AeSysView::OnUpdateViewOdometer(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewOdometer); }

void AeSysView::DisplayOdometer() {
  auto cursorPosition = GetCursorPosition();

  m_vRelPos = cursorPosition - GridOrign();

  if (m_ViewOdometer) {
    auto units = app.GetUnits();

    CString lengthText;

    app.FormatLength(lengthText, units, m_vRelPos.x);
    CString Position = lengthText.TrimLeft();
    app.FormatLength(lengthText, units, m_vRelPos.y);
    Position.Append(L", " + lengthText.TrimLeft());
    app.FormatLength(lengthText, units, m_vRelPos.z);
    Position.Append(L", " + lengthText.TrimLeft());

    if (m_rubberbandType == Lines) {
      EoGeLine line(m_rubberbandBegin, cursorPosition);

      auto lineLength = line.Length();
      auto angleInXYPlane = line.AngleFromXAxisXY();
      app.FormatLength(lengthText, units, lineLength);

      CString angle;
      app.FormatAngle(angle, angleInXYPlane, 8, 3);
      angle.ReleaseBuffer();
      Position.Append(L" [" + lengthText.TrimLeft() + L" @ " + angle + L"]");
    }
    auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
    mainFrame->SetPaneText(1, Position);
#if defined(LEGACY_ODOMETER)
    DrawOdometerInView(this, GetDC(), units, m_vRelPos);
#endif
  }
#if defined(USING_DDE)
  dde::PostAdvise(dde::RelPosXInfo);
  dde::PostAdvise(dde::RelPosYInfo);
  dde::PostAdvise(dde::RelPosZInfo);
#endif  // USING_DDE
}

void AeSysView::OnUpdateViewTrueTypeFonts(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewTrueTypeFonts); }

void AeSysView::OnBackgroundImageLoad() {
  CFileDialog dlg(TRUE, L"bmp", L"*.bmp");
  dlg.m_ofn.lpstrTitle = L"Load Background Image";

  if (dlg.DoModal() == IDOK) {
    EoDbBitmapFile BitmapFile(dlg.GetPathName());

    BitmapFile.Load(dlg.GetPathName(), m_backgroundImageBitmap, m_backgroundImagePalette);
    m_viewBackgroundImage = true;
    InvalidateRect(nullptr, TRUE);
  }
}

void AeSysView::OnBackgroundImageRemove() {
  if (static_cast<HBITMAP>(m_backgroundImageBitmap) != 0) {
    m_backgroundImageBitmap.DeleteObject();
    m_backgroundImagePalette.DeleteObject();
    m_viewBackgroundImage = false;

    InvalidateRect(nullptr, TRUE);
  }
}

void AeSysView::OnViewBackgroundImage() {
  m_viewBackgroundImage = !m_viewBackgroundImage;
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnUpdateViewBackgroundImage(CCmdUI* pCmdUI) {
  pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) != 0);
  pCmdUI->SetCheck(m_viewBackgroundImage);
}

void AeSysView::OnUpdateBackgroundimageLoad(CCmdUI* pCmdUI) {
  pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) == 0);
}

void AeSysView::OnUpdateBackgroundimageRemove(CCmdUI* pCmdUI) {
  pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) != 0);
}

void AeSysView::OnUpdateViewPenwidths(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewPenWidths); }

void AeSysView::DeleteLastGroup() {
  if (m_VisibleGroupList.IsEmpty()) {
    app.AddStringToMessageList(IDS_MSG_NO_DET_GROUPS_IN_VIEW);
  } else {
    auto* document = GetDocument();
    auto* Group = m_VisibleGroupList.RemoveTail();

    document->AnyLayerRemove(Group);
    if (document->RemoveTrappedGroup(Group) != 0) {  // Display it normal color so the erase xor will work
      document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      UpdateStateInformation(TrapCount);
    }
    document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
    document->DeletedGroupsAddHead(Group);
    app.AddStringToMessageList(IDS_SEG_DEL_TO_RESTORE);
  }
}

void AeSysView::BreakAllPolylines() {
  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* Group = GetNextVisibleGroup(position);
    Group->BreakPolylines();
  }
}

void AeSysView::BreakAllSegRefs() {
  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* Group = GetNextVisibleGroup(position);
    Group->BreakSegRefs();
  }
}

void AeSysView::ResetView() {
  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;
}

EoDbGroup* AeSysView::SelSegAndPrimAtCtrlPt(const EoGePoint4d& pt) {
  EoDbPrimitive* primitive{};
  EoGePoint3d engagedPoint;

  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;

  EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextVisibleGroup(position);
    primitive = group->SelPrimAtCtrlPt(this, pt, &engagedPoint);
    if (primitive != nullptr) {
      m_ptDet = engagedPoint;
      m_ptDet = transformMatrix * m_ptDet;
      m_EngagedGroup = group;
      m_EngagedPrimitive = primitive;
    }
  }
  return m_EngagedGroup;
}

EoDbGroup* AeSysView::SelectGroupAndPrimitive(const EoGePoint3d& point) {
  EoGePoint3d engagedPoint;

  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;

  EoGePoint4d ptView(point);
  ModelViewTransformPoint(ptView);

  EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  double apertureSize = m_SelectApertureSize;

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto position = GetFirstVisibleGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextVisibleGroup(position);
    auto* primitive = group->SelPrimUsingPoint(this, ptView, apertureSize, engagedPoint);
    if (primitive != nullptr) {
      m_ptDet = engagedPoint;
      m_ptDet = transformMatrix * m_ptDet;
      m_EngagedGroup = group;
      m_EngagedPrimitive = primitive;
      return group;
    }
  }
  return nullptr;
}

EoDbGroup* AeSysView::SelectCircleUsingPoint(EoGePoint3d& point, double tolerance, EoDbConic*& circle) {
  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (!primitive->Is(EoDb::kConicPrimitive)) { continue; }
      auto* conic = static_cast<EoDbConic*>(primitive);
      if (conic->Subclass() != EoDbConic::ConicType::Circle) { continue; }
      if (point.DistanceTo(conic->Center()) <= tolerance) {
        circle = conic;
        return group;
      }
    }
  }
  return nullptr;
}

EoDbGroup* AeSysView::SelectLineUsingPoint(EoGePoint3d& point, EoDbLine*& line) {
  EoGePoint4d ptView(point);
  ModelViewTransformPoint(ptView);

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive->Is(EoDb::kLinePrimitive)) {
        EoGePoint3d PointOnLine;
        if (primitive->SelectUsingPoint(this, ptView, PointOnLine)) {
          line = static_cast<EoDbLine*>(primitive);
          return group;
        }
      }
    }
  }
  return nullptr;
}

EoDbGroup* AeSysView::SelectLineUsingPoint(const EoGePoint3d& pt) {
  m_EngagedGroup = nullptr;
  m_EngagedPrimitive = nullptr;

  EoGePoint3d engagedPoint;

  EoGePoint4d ptView(pt);
  ModelViewTransformPoint(ptView);

  double tol = m_SelectApertureSize;

  EoGeTransformMatrix transformMatrix = ModelViewGetMatrixInverse();

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(GroupPosition);
    auto PrimitivePosition = group->GetHeadPosition();
    while (PrimitivePosition != nullptr) {
      EoDbPrimitive* primitive = group->GetNext(PrimitivePosition);
      if (primitive->Is(EoDb::kLinePrimitive)) {
        if (primitive->SelectUsingPoint(this, ptView, engagedPoint)) {
          tol = ptView.DistanceToPointXY(EoGePoint4d(engagedPoint));

          m_ptDet = engagedPoint;
          m_ptDet = transformMatrix * m_ptDet;
          m_EngagedGroup = group;
          m_EngagedPrimitive = primitive;
        }
      }
    }
  }
  return m_EngagedGroup;
}

EoDbText* AeSysView::SelectTextUsingPoint(const EoGePoint3d& point) {
  EoGePoint4d ptView(point);
  ModelViewTransformPoint(ptView);

  auto groupPosition = GetFirstVisibleGroupPosition();
  while (groupPosition != nullptr) {
    auto* group = GetNextVisibleGroup(groupPosition);
    auto primitivePosition = group->GetHeadPosition();
    while (primitivePosition != nullptr) {
      auto* primitive = group->GetNext(primitivePosition);
      if (primitive->Is(EoDb::kTextPrimitive)) {
        EoGePoint3d ptProj;
        if (static_cast<EoDbText*>(primitive)->SelectUsingPoint(this, ptView, ptProj)) {
          return static_cast<EoDbText*>(primitive);
        }
      }
    }
  }
  return nullptr;
}

void AeSysView::OnOp0() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
    case ID_MODE_GROUP_EDIT:
      OnEditModeOptions();
      break;
  }
}

void AeSysView::OnOp2() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP2);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP2);
      break;
  }
}

void AeSysView::OnOp3() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP3);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP3);
      break;
  }
}

void AeSysView::OnOp4() {
  auto* document = GetDocument();
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      document->InitializeGroupAndPrimitiveEdit();
      break;

    case ID_MODE_GROUP_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      document->InitializeGroupAndPrimitiveEdit();
      break;
  }
}

void AeSysView::OnOp5() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveCopy();
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupCopy();
      break;
  }
}

void AeSysView::OnOp6() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP6);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP6);
      break;
  }
}

void AeSysView::OnOp7() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP7);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP7);
      break;
  }
}

void AeSysView::OnOp8() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveTransform(ID_OP8);
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupTransform(ID_OP8);
      break;
  }
}

void AeSysView::OnReturn() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      InitializeGroupAndPrimitiveEdit();
      break;

    case ID_MODE_GROUP_EDIT:
      app.LoadModeResources(app.PrimaryMode());
      InitializeGroupAndPrimitiveEdit();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      MendPrimitiveReturn();
      break;
  }
}

void AeSysView::OnEscape() {
  switch (app.CurrentMode()) {
    case ID_MODE_PRIMITIVE_EDIT:
      DoEditPrimitiveEscape();
      break;

    case ID_MODE_GROUP_EDIT:
      DoEditGroupEscape();
      break;

    case ID_MODE_PRIMITIVE_MEND:
      MendPrimitiveEscape();
      break;
  }
}

/** @brief Handler for the Find command, which retrieves the text from the Find combo box in the main frame and logs it.
 * @note Currently, this function only verifies the combo box text and logs it, but it is intended to be expanded with the actual Find command implementation in the future.
 */
void AeSysView::OnFind() {
  constexpr auto mainFrameErrorMsg = L"Main frame should exist when Find command is triggered";

  // Get the main frame window
  auto* mainFrame = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
#ifdef _DEBUG
  assert(mainFrame != nullptr && mainFrameErrorMsg);
#endif
  if (mainFrame == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"%ls\n", static_cast<const wchar_t*>(mainFrameErrorMsg));
    return;
  }
  // Get the find combo box control
  constexpr auto findComboErrorMsg = L"Find combo should exist in toolbar";

  auto* findComboBox = mainFrame->GetFindCombo();
#ifdef _DEBUG
  assert(findComboBox != nullptr && findComboErrorMsg);
#endif
  if (findComboBox == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"%ls\n", static_cast<const wchar_t*>(findComboErrorMsg));
    return;
  }

  CString findComboBoxText;
  VerifyFindString(findComboBox, findComboBoxText);

  if (findComboBoxText.IsEmpty()) { return; }

  // @todo Find command implementation should go here, currently just verifying the combo box text and logging it
  ATLTRACE2(
      traceGeneral, 1, L"Verifying the FindComboBox text and logging: `%ls`\n", static_cast<LPCWSTR>(findComboBoxText));
}

/** @brief Verifies the text in the Find combo box and updates it if necessary.
 * @param findComboBox The combo box button to verify.
 * @param findComboBoxText A reference to a CString that will be updated with the current text of the combo box if the last command was from the button.
 * @note This function checks if the last command was triggered by the provided combo box button. If it was, it retrieves the current text from the combo box. It then checks if this text already exists in the combo box's list of items. If it does, it removes it and re-inserts it at the top of the list to ensure that the most recently used search term is easily accessible. Finally, if the last command was not from the button, it sets the combo box text to the verified text.
 */
void AeSysView::VerifyFindString(CMFCToolBarComboBoxButton* findComboBox, CString& findComboBoxText) {
  if (findComboBox == nullptr) { return; }
  BOOL isLastCommandFromButton = CMFCToolBar::IsLastCommandFromButton(findComboBox);

  if (isLastCommandFromButton) { findComboBoxText = findComboBox->GetText(); }
  auto* ComboBox = findComboBox->GetComboBox();

  if (!findComboBoxText.IsEmpty()) {
    const int count = ComboBox->GetCount();
    int Position{};

    while (Position < count) {
      CString LBText;
      ComboBox->GetLBText(Position, LBText);

      if (LBText.GetLength() == findComboBoxText.GetLength()) {
        if (LBText == findComboBoxText) { break; }
      }
      Position++;
    }
    // Text need to move to initial position
    if (Position < count) { ComboBox->DeleteString(static_cast<UINT>(Position)); }
    ComboBox->InsertString(0, findComboBoxText);
    ComboBox->SetCurSel(0);

    if (!isLastCommandFromButton) { findComboBox->SetText(findComboBoxText); }
  }
}

void AeSysView::OnEditFind() { ATLTRACE2(traceGeneral, 1, L"AeSysView::OnEditFind() - Entering\n"); }

void AeSysView::RubberBandingDisable() {
  if (m_rubberbandType != None) {
    auto* deviceContext = GetDC();
    int drawMode = deviceContext->SetROP2(R2_XORPEN);
    CPen grayPen(PS_SOLID, 0, Eo::colorRubberband);
    auto* pen = deviceContext->SelectObject(&grayPen);

    if (m_rubberbandType == Lines) {
      deviceContext->MoveTo(m_rubberbandLogicalBegin);
      deviceContext->LineTo(m_rubberbandLogicalEnd);
    } else if (m_rubberbandType == Rectangles) {
      auto* brush = static_cast<CBrush*>(deviceContext->SelectStockObject(NULL_BRUSH));
      deviceContext->Rectangle(
          m_rubberbandLogicalBegin.x, m_rubberbandLogicalBegin.y, m_rubberbandLogicalEnd.x, m_rubberbandLogicalEnd.y);
      deviceContext->SelectObject(brush);
    }
    deviceContext->SelectObject(pen);
    deviceContext->SetROP2(drawMode);
    ReleaseDC(deviceContext);
    m_rubberbandType = None;
  }
}

void AeSysView::RubberBandingStartAtEnable(EoGePoint3d point, ERubs type) {
  EoGePoint4d ndcPoint(point);

  ModelViewTransformPoint(ndcPoint);

  if (ndcPoint.IsInView()) {
    m_rubberbandBegin = point;
    m_rubberbandLogicalBegin = ProjectToClient(ndcPoint);
    m_rubberbandLogicalEnd = m_rubberbandLogicalBegin;
  }
  m_rubberbandType = type;
}

EoGePoint3d AeSysView::GetCursorPosition() {
  CPoint cursorPosition;

  ::GetCursorPos(&cursorPosition);
  ScreenToClient(&cursorPosition);

  EoGePoint3d pt(double(cursorPosition.x), double(cursorPosition.y), m_ptCursorPosDev.z);
  if (pt != m_ptCursorPosDev) {
    m_ptCursorPosDev = pt;
    m_ptCursorPosWorld = m_ptCursorPosDev;

    DoProjectionInverse(m_ptCursorPosWorld);

    m_ptCursorPosWorld = ModelViewGetMatrixInverse() * m_ptCursorPosWorld;
    m_ptCursorPosWorld = SnapPointToGrid(m_ptCursorPosWorld);
  }
  return m_ptCursorPosWorld;
}

void AeSysView::SetCursorPosition(const EoGePoint3d& position) {
  EoGePoint4d ndcPoint(position);
  ModelViewTransformPoint(ndcPoint);

  if (!ndcPoint.IsInView()) {
    // Redefine the view so targeted position becomes center
    m_ViewTransform.SetTarget(position);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    InvalidateRect(nullptr, TRUE);

    ndcPoint = EoGePoint4d(position);
    ModelViewTransformPoint(ndcPoint);
  }
  // Move the cursor to specified position.
  CPoint clientPoint = ProjectToClient(ndcPoint);

  m_ptCursorPosDev =
      EoGePoint3d(static_cast<double>(clientPoint.x), static_cast<double>(clientPoint.y), ndcPoint.z / ndcPoint.w);

  m_ptCursorPosWorld = position;

  ClientToScreen(&clientPoint);
  ::SetCursorPos(clientPoint.x, clientPoint.y);
}

void AeSysView::SetModeCursor(int mode) {
  std::uint16_t ResourceIdentifier;

  switch (mode) {
    case ID_MODE_ANNOTATE:
      ResourceIdentifier = IDR_ANNOTATE_MODE;
      break;

    case ID_MODE_CUT:
      ResourceIdentifier = IDR_CUT_MODE;
      break;

    case ID_MODE_DIMENSION:
      ResourceIdentifier = IDR_DIMENSION_MODE;
      break;

    case ID_MODE_DRAW:
      ResourceIdentifier = IDR_DRAW_MODE;
      break;

    case ID_MODE_LPD:
      ResourceIdentifier = IDR_LPD_MODE;
      break;

    case ID_MODE_PIPE:
      ResourceIdentifier = IDR_PIPE_MODE;
      break;

    case ID_MODE_POWER:
      ResourceIdentifier = IDR_POWER_MODE;
      break;

    case ID_MODE_DRAW2:
      ResourceIdentifier = IDR_DRAW2_MODE;
      break;

    case ID_MODE_EDIT:
      ResourceIdentifier = IDR_EDIT_MODE;
      break;

    case ID_MODE_FIXUP:
      ResourceIdentifier = IDR_FIXUP_MODE;
      break;

    case ID_MODE_NODAL:
      ResourceIdentifier = IDR_NODAL_MODE;
      break;

    case ID_MODE_NODALR:
      ResourceIdentifier = IDR_NODALR_MODE;
      break;

    case ID_MODE_TRAP:
      ResourceIdentifier = IDR_TRAP_MODE;
      break;

    case ID_MODE_TRAPR:
      ResourceIdentifier = IDR_TRAPR_MODE;
      break;

    default:
      SetCursor(static_cast<HCURSOR>(LoadImageW(nullptr, IDC_CROSS, IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE)));
      return;
  }
  auto cursorHandle = static_cast<HCURSOR>(
      LoadImageW(AeSys::GetInstance(), MAKEINTRESOURCE(ResourceIdentifier), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  VERIFY(cursorHandle);
  SetCursor(cursorHandle);
  SetClassLongPtr(this->GetSafeHwnd(), GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursorHandle));
}

void AeSysView::SetWorldScale(double scale) {
  if (scale > Eo::geometricTolerance) {
    m_WorldScale = scale;
    UpdateStateInformation(Scale);

    CMainFrame* MainFrame = (CMainFrame*)(AfxGetMainWnd());
    MainFrame->GetPropertiesPane().GetActiveViewScaleProperty().SetValue(m_WorldScale);

#if defined(USING_DDE)
    dde::PostAdvise(dde::ScaleInfo);
#endif  // USING_DDE
  }
}

void AeSysView::OnViewStateInformation() {
  m_ViewStateInformation = !m_ViewStateInformation;
  AeSysDoc::GetDoc()->UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysView::OnUpdateViewStateinformation(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewStateInformation); }

void AeSysView::UpdateStateInformation(EStateInformationItem item) {
  if (!m_ViewStateInformation) { return; }

  auto* document = AeSysDoc::GetDoc();
  auto* deviceContext = GetDC();

  auto oldFont = deviceContext->SelectStockObject(DEFAULT_GUI_FONT);
  auto oldTextAlign = deviceContext->SetTextAlign(TA_LEFT | TA_TOP);
  auto oldTextColor = deviceContext->SetTextColor(App::ViewTextColor());
  auto oldBkColor = deviceContext->SetBkColor(~App::ViewTextColor() & 0x00ffffff);

  TEXTMETRIC textMetric{};
  deviceContext->GetTextMetrics(&textMetric);
  auto averageCharacterWidth = textMetric.tmAveCharWidth;
  auto height = textMetric.tmHeight;

  CRect clientRect{};
  GetClientRect(&clientRect);
  auto top = clientRect.top;

  CRect rectangle{};
  constexpr UINT options = ETO_CLIPPED | ETO_OPAQUE;
  wchar_t szBuf[32]{};

  if ((item & WorkCount) == WorkCount) {
    rectangle.SetRect(0, top, 8 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"%-4i", document->NumberOfGroupsInWorkLayer() + document->NumberOfGroupsInActiveLayers());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & TrapCount) == TrapCount) {
    rectangle.SetRect(8 * averageCharacterWidth, top, 16 * averageCharacterWidth, top + height);
    long trapCount = static_cast<long>(document->TrapGroupCount());
    swprintf_s(szBuf, 32, L"%-4ld", trapCount);
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & Pen) == Pen) {
    rectangle.SetRect(16 * averageCharacterWidth, top, 22 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"P%-4i", renderState.Color());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & Line) == Line) {
    rectangle.SetRect(22 * averageCharacterWidth, top, 28 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"L%-4i", renderState.LineTypeIndex());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & TextHeight) == TextHeight) {
    auto characterCellDefinition = renderState.CharacterCellDefinition();
    rectangle.SetRect(28 * averageCharacterWidth, top, 38 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"T%-6.2f", characterCellDefinition.Height());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & Scale) == Scale) {
    rectangle.SetRect(38 * averageCharacterWidth, top, 48 * averageCharacterWidth, top + height);
    swprintf_s(szBuf, 32, L"1:%-6.2f", GetWorldScale());
    deviceContext->ExtTextOutW(
        rectangle.left, rectangle.top, options, &rectangle, szBuf, static_cast<UINT>(wcslen(szBuf)), 0);
  }
  if ((item & WndRatio) == WndRatio) {
    rectangle.SetRect(48 * averageCharacterWidth, top, 58 * averageCharacterWidth, top + height);
    double Ratio = WidthInInches() / UExtent();
    CString RatioAsString;
    RatioAsString.Format(L"=%-8.3f", Ratio);
    deviceContext->ExtTextOutW(rectangle.left, rectangle.top, options, &rectangle, RatioAsString,
        static_cast<UINT>(RatioAsString.GetLength()), 0);
  }
  if ((item & DimLen) == DimLen || (item & DimAng) == DimAng) {
    rectangle.SetRect(58 * averageCharacterWidth, top, 90 * averageCharacterWidth, top + height);
    CString LengthAndAngle;
    app.FormatLength(LengthAndAngle, app.GetUnits(), app.DimensionLength());
    LengthAndAngle.TrimLeft();
    CString Angle;
    app.FormatAngle(Angle, Eo::DegreeToRadian(app.DimensionAngle()), 8, 3);
    Angle.ReleaseBuffer();
    LengthAndAngle.Append(L" @ " + Angle);
    deviceContext->ExtTextOutW(rectangle.left, rectangle.top, options, &rectangle, LengthAndAngle,
        static_cast<UINT>(LengthAndAngle.GetLength()), 0);
  }
  deviceContext->SetBkColor(oldBkColor);
  deviceContext->SetTextColor(oldTextColor);
  deviceContext->SetTextAlign(oldTextAlign);
  deviceContext->SelectObject(oldFont);
  ReleaseDC(deviceContext);
}

// Mode switching commands (moved from AeSys.cpp for proper command routing)

void AeSysView::OnModeAnnotate() {
  app.SetModeResourceIdentifier(IDR_ANNOTATE_MODE);
  app.SetPrimaryMode(ID_MODE_ANNOTATE);
  app.LoadModeResources(ID_MODE_ANNOTATE);
}

void AeSysView::OnModeCut() {
  app.SetModeResourceIdentifier(IDR_CUT_MODE);
  app.SetPrimaryMode(ID_MODE_CUT);
  app.LoadModeResources(ID_MODE_CUT);
}

void AeSysView::OnModeDimension() {
  app.SetModeResourceIdentifier(IDR_DIMENSION_MODE);
  app.SetPrimaryMode(ID_MODE_DIMENSION);
  app.LoadModeResources(ID_MODE_DIMENSION);
}

void AeSysView::OnModeDraw() {
  app.SetModeResourceIdentifier(IDR_DRAW_MODE);
  app.SetPrimaryMode(ID_MODE_DRAW);
  app.LoadModeResources(ID_MODE_DRAW, this);
#if defined(USING_STATE_PATTERN)
  PushState(std::make_unique<DrawModeState>());
#endif
}

void AeSysView::OnModeDraw2() {
  app.SetModeResourceIdentifier(IDR_DRAW2_MODE);
  app.SetPrimaryMode(ID_MODE_DRAW2);
  app.LoadModeResources(ID_MODE_DRAW2);
}

void AeSysView::OnModeEdit() {
  app.SetModeResourceIdentifier(IDR_EDIT_MODE);
  app.LoadModeResources(ID_MODE_EDIT);
}

void AeSysView::OnModeFixup() {
  app.SetModeResourceIdentifier(IDR_FIXUP_MODE);
  app.LoadModeResources(ID_MODE_FIXUP);
}

void AeSysView::OnModeLPD() {
  app.SetModeResourceIdentifier(IDR_LPD_MODE);
  app.LoadModeResources(ID_MODE_LPD);
}

void AeSysView::OnModeNodal() {
  app.SetModeResourceIdentifier(IDR_NODAL_MODE);
  app.LoadModeResources(ID_MODE_NODAL);
}

void AeSysView::OnModePipe() {
  app.SetModeResourceIdentifier(IDR_PIPE_MODE);
  app.LoadModeResources(ID_MODE_PIPE);
}

void AeSysView::OnModePower() {
  app.SetModeResourceIdentifier(IDR_POWER_MODE);
  app.LoadModeResources(ID_MODE_POWER);
}

void AeSysView::OnModeTrap() {
  if (app.TrapModeAddGroups()) {
    app.SetModeResourceIdentifier(IDR_TRAP_MODE);
    app.LoadModeResources(ID_MODE_TRAP);
  } else {
    app.SetModeResourceIdentifier(IDR_TRAPR_MODE);
    app.LoadModeResources(ID_MODE_TRAPR);
  }
}

void AeSysView::OnTrapCommandsAddGroups() {
  app.SetModeAddGroups(!app.TrapModeAddGroups());
  app.LoadModeResources(app.TrapModeAddGroups() ? ID_MODE_TRAP : ID_MODE_TRAPR);
  OnModeTrap();
}

// Update UI handlers
void AeSysView::OnUpdateModeAnnotate(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_ANNOTATE); }

void AeSysView::OnUpdateModeCut(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_CUT); }

void AeSysView::OnUpdateModeDimension(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_DIMENSION); }

void AeSysView::OnUpdateModeDraw(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_DRAW); }

void AeSysView::OnUpdateModeDraw2(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_DRAW2); }

void AeSysView::OnUpdateModeEdit(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_EDIT); }

void AeSysView::OnUpdateModeFixup(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_FIXUP); }

void AeSysView::OnUpdateModeLpd(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_LPD); }

void AeSysView::OnUpdateModeNodal(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_NODAL); }

void AeSysView::OnUpdateModePipe(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_PIPE); }

void AeSysView::OnUpdateModePower(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_POWER); }

void AeSysView::OnUpdateModeTrap(CCmdUI* pCmdUI) { pCmdUI->SetCheck(app.CurrentMode() == ID_MODE_TRAP); }

#if defined(USING_Direct2D)
HRESULT AeSysView::CreateDeviceResources() {
  HRESULT hr = S_OK;

  if (!m_RenderTarget) {
    RECT rc;
    GetClientRect(&rc);

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    HWND WindowHandle = GetSafeHwnd();
    hr = app.m_Direct2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(WindowHandle, size), &m_RenderTarget);

    if (SUCCEEDED(hr)) {
      hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &m_LightSlateGrayBrush);
    }
    if (SUCCEEDED(hr)) {
      hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 0.3f), &m_RedBrush);
    }
  }
  return hr;
}

void AeSysView::DiscardDeviceResources() {
  SafeRelease(&m_RenderTarget);
  SafeRelease(&m_LightSlateGrayBrush);
  SafeRelease(&m_RedBrush);
}
#endif
