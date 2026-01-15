#include "stdafx.h"

#include <Windows.h>
#include <afx.h>
#include <afxdlgs.h>
#include <afxext.h>
#include <afxmdichildwndex.h>
#include <afxmdiframewndex.h>
#include <afxmsg_.h>
#include <afxpropertygridctrl.h>
#include <afxres.h>
#include <afxstatusbar.h>
#include <afxstr.h>
#include <afxtoolbar.h>
#include <afxtoolbarcomboboxbutton.h>
#include <afxwin.h>
#include <algorithm>
#include <atltrace.h>
#include <atltypes.h>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <wchar.h>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbBitmapFile.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbEllipse.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbPoint.h"
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
#include "EoGsViewTransform.h"
#include "MainFrm.h"
#include "PrimState.h"
#include "Resource.h"
#include "Section.h"

#if defined(USING_DDE)
#include "Dde.h"
#include "DdeGItms.h"
#endif  // USING_DDE

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const double AeSysView::m_MaximumWindowRatio = 999.0;
const double AeSysView::m_MinimumWindowRatio = 0.001;

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
    : m_ModelTransform(),
      m_Viewport(),
      m_ViewTransform(),
      m_backgroundImageBitmap(),
      m_backgroundImagePalette(),
      m_EngagedPrimitive(nullptr),
      m_EngagedGroup(nullptr),
      m_OpHighlighted(0),
      m_OverviewViewTransform(),
      m_Plot(false),
      m_PlotScaleFactor(1.0),
      m_PreviewGroup(),
      m_PreviousViewTransform(),
      m_PreviousOp(0),
      m_PreviousPnt(),
      m_SelectApertureSize(0.005),
      m_viewBackgroundImage(false),
      m_ViewOdometer(true),
      m_ViewPenWidths(false),
      m_Viewports(),
      m_ViewRendered(false),
      m_ViewTransforms(),
      m_ViewTrueTypeFonts(true),
      m_ViewWireframe(false),
      m_VisibleGroupList(),
      m_WorldScale(1.0),
      m_ptDet(),
      m_vRelPos(),
      // Grid constraints
      m_GridOrigin(),
      m_MaximumDotsPerLine(64),
      m_XGridLineSpacing(12.0),
      m_YGridLineSpacing(12.0),
      m_ZGridLineSpacing(12.0),
      m_XGridSnapSpacing(1.0),
      m_YGridSnapSpacing(1.0),
      m_ZGridSnapSpacing(1.0),
      m_XGridPointSpacing(3.0),
      m_YGridPointSpacing(3.0),
      m_ZGridPointSpacing(0.0),
      m_AxisConstraintInfluenceAngle(5.0),
      m_AxisConstraintOffsetAngle(0.0),
      m_DisplayGridWithLines(false),
      m_DisplayGridWithPoints(false),
      m_GridSnap(false),
      // Cursor/Selection
      m_ViewStateInformation(true),
      m_middleButtonPanStartPoint(),
      m_middleButtonPanInProgress{false},
      m_RubberbandType(None),
      m_RubberbandBeginPoint(),
      m_RubberbandLogicalBeginPoint(),
      m_RubberbandLogicalEndPoint(),
      m_ptCursorPosDev(),
      m_ptCursorPosWorld(),
      // Sub-mode edit
      m_SubModeEditGroup(nullptr),
      m_SubModeEditPrimitive(nullptr),
      m_SubModeEditBeginPoint(),
      m_SubModeEditEndPoint(),
      m_tmEditSeg(),
      // Mend
      m_MendPrimitiveBegin(),
      m_MendPrimitiveVertexIndex(0),
      m_PrimitiveToMend(nullptr),
      m_PrimitiveToMendCopy(nullptr),
      // Annotate
      m_GapSpaceFactor(0.5),
      m_CircleRadius(0.03125),
      m_EndItemType(1),
      m_EndItemSize(0.1),
      m_BubbleRadius(0.125),
      m_NumberOfSides(0),
      m_DefaultText(),
      /// Draw2 Mode Interface
      m_CenterLineEccentricity(0.5),
      m_ContinueCorner(false),
      m_DistanceBetweenLines(0.0625),
      m_CurrentLeftLine(),
      m_CurrentRightLine(),
      m_PreviousReferenceLine(),
      m_CurrentReferenceLine(),
      m_AssemblyGroup(nullptr),
      m_EndSectionGroup(nullptr),
      m_BeginSectionGroup(nullptr),
      m_BeginSectionLine(nullptr),
      m_EndSectionLine(nullptr),
      /// Fixup Mode Interface
      m_FixupModeAxisTolerance(2.0),
      m_FixupModeCornerSize(0.25),
      // Edit Mode interface
      m_EditModeMirrorScale(-1.0, 1.0, 1.0),
      m_EditModeRotationAngles(0.0, 0.0, 45.0),
      m_EditModeScale(2.0, 2.0, 2.0),

      /// Low Pressure Duct Mode Interface
      m_InsideRadiusFactor(1.5),
      m_DuctSeamSize(0.03125),
      m_DuctTapSize(0.09375),
      m_GenerateTurningVanes(true),
      m_ElbowType(Mittered),
      m_DuctJustification(Center),
      m_TransitionSlope(4.0),
      m_BeginWithTransition(false),
      m_ContinueSection(false),
      m_EndCapLocation(0),
      m_EndCapPoint(nullptr),
      m_EndCapGroup(nullptr),
      m_OriginalPreviousGroupDisplayed(true),
      m_OriginalPreviousGroup(nullptr),
      m_PreviousSection(0.125, 0.0625, Section::Rectangular),
      m_CurrentSection(0.125, 0.0625, Section::Rectangular),

      /// Pipe Mode Interface
      m_CurrentPipeSymbolIndex(0),
      m_PipeTicSize(0.03125),
      m_PipeRiseDropRadius(0.03125),

      /// Power Mode Interface (initializers)
      m_PowerArrow(false),
      m_PowerConductor(false),
      m_PowerConductorSpacing(0.04),
      m_CircuitEndPoint(),
      m_PreviousRadius(0) {
  m_Viewport.SetDeviceWidthInPixels(app.DeviceWidthInPixels());
  m_Viewport.SetDeviceHeightInPixels(app.DeviceHeightInPixels());
  m_Viewport.SetDeviceWidthInInches(app.DeviceWidthInMillimeters() / Eo::MmPerInch);
  m_Viewport.SetDeviceHeightInInches(app.DeviceHeightInMillimeters() / Eo::MmPerInch);
}

AeSysView::~AeSysView() {}

// AeSysView diagnostics //////////////////////////////////////////////////////

#ifdef _DEBUG
void AeSysView::AssertValid() const { CView::AssertValid(); }
void AeSysView::Dump(CDumpContext& dc) const { CView::Dump(dc); }
AeSysDoc* AeSysView::GetDocument() const {  // non-debug version is inline
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(AeSysDoc)));
  return (AeSysDoc*)m_pDocument;
}
#endif  //_DEBUG

// Base class overides ////////////////////////////////////////////////////////

void AeSysView::OnActivateFrame(UINT state, CFrameWnd* deactivateFrame) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnActivateFrame(%i, %08.8lx)\n", this, state, deactivateFrame);

  CView::OnActivateFrame(state, deactivateFrame);
}
void AeSysView::OnActivateView(BOOL activate, CView* activateView, CView* deactiveView) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnActivateView(%i, %p, %p))\n", this, activate, activateView, deactiveView);

  CMainFrame* MainFrame = (CMainFrame*)(AfxGetMainWnd());
  if (activate) {
    if (CopyAcceleratorTableW(MainFrame->m_hAccelTable, nullptr, 0) == 0) {  // Accelerator table was destroyed when keyboard focus was killed - reload resource
      app.BuildModifiedAcceleratorTable();
    }
  }
  CMFCPropertyGridProperty& ActiveViewScaleProperty = MainFrame->GetPropertiesPane().GetActiveViewScaleProperty();
  ActiveViewScaleProperty.SetValue(m_WorldScale);
  ActiveViewScaleProperty.Enable(activate);

  CView::OnActivateView(activate, activateView, deactiveView);
}
BOOL AeSysView::PreCreateWindow(CREATESTRUCT& createStructure) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::PreCreateWindow(%08.8lx) ", this, createStructure);

  // TODO: Modify the Window class or styles here by modifying the CREATESTRUCT
  return CView::PreCreateWindow(createStructure);
}
int AeSysView::OnCreate(LPCREATESTRUCT createStructure) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnCreate(%08.8lx)\n", this, createStructure);

  if (CView::OnCreate(createStructure) == -1) { return -1; }
  return 0;
}
void AeSysView::OnSetFocus(CWnd* oldWindow) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnSetFocus(%08.8lx)\n", this, oldWindow);

  CMainFrame* MainFrame = (CMainFrame*)(AfxGetMainWnd());
  if (CopyAcceleratorTableW(MainFrame->m_hAccelTable, nullptr, 0) == 0) {
    // Accelerator table was destroyed when keyboard focus was killed - reload resource
    app.BuildModifiedAcceleratorTable();
  }
  CView::OnSetFocus(oldWindow);
}
void AeSysView::OnKillFocus(CWnd* newWindow) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnKillFocus(%08.8lx)\n", this, newWindow);

  HACCEL AcceleratorTableHandle = ((CMainFrame*)AfxGetMainWnd())->m_hAccelTable;

  ::DestroyAcceleratorTable(AcceleratorTableHandle);

  CView::OnKillFocus(newWindow);
}
void AeSysView::OnPaint() {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnPaint()\n", this);
  CView::OnPaint();
}
void AeSysView::OnDraw(CDC* deviceContext) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnDraw(%08.8lx) +", this, deviceContext);

  CRect Rect;
  deviceContext->GetClipBox(Rect);
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L" ClipBox(%i, %i, %i, %i)\n", Rect.left, Rect.top, Rect.right, Rect.bottom);

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

      D2D1_RECT_F rectangle1 = D2D1::RectF(rtSize.width / 2 - 50.0f, rtSize.height / 2 - 50.0f, rtSize.width / 2 + 50.0f, rtSize.height / 2 + 50.0f);
      D2D1_RECT_F rectangle2 = D2D1::RectF(rtSize.width / 2 - 100.0f, rtSize.height / 2 - 100.0f, rtSize.width / 2 + 100.0f, rtSize.height / 2 + 100.0f);

      m_RenderTarget->FillRectangle(&rectangle1, m_RedBrush);
      m_RenderTarget->DrawRectangle(&rectangle2, m_LightSlateGrayBrush);

      hr = m_RenderTarget->EndDraw();
    }
    if (hr == D2DERR_RECREATE_TARGET) {
      hr = S_OK;
      DiscardDeviceResources();
    }
#endif  // USING_Direct2D

    auto* Document = GetDocument();
    ASSERT_VALID(Document);
    if (m_ViewRendered) {
    } else {
      BackgroundImageDisplay(deviceContext);
      DisplayGrid(deviceContext);

      Document->DisplayAllLayers(this, deviceContext);
      Document->DisplayUniquePoints();
    }
    UpdateStateInformation(All);
    ModeLineDisplay();
    ValidateRect(nullptr);
  } catch (CException* e) { e->Delete(); }
}
/// <remarks>
///The default implementation of this function calls the OnUpdate member function with no hint information
///Override this function to perform any one-time initialization that requires information about the document.
/// </remarks>
void AeSysView::OnInitialUpdate() {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnInitialUpdate()\n", this);

  SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, (LONG_PTR)::CreateSolidBrush(ViewBackgroundColor));

#if defined(USING_Direct2D)
  m_RenderTarget = nullptr;
  CreateDeviceResources();
#endif  // USING_Direct2D

  CView::OnInitialUpdate();
}

void AeSysView::OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>::OnUpdate(%p, %p, %p)\n", this, sender, hint, hintObject);

  CDC* DeviceContext = GetDC();
  COLORREF BackgroundColor = DeviceContext->GetBkColor();
  DeviceContext->SetBkColor(ViewBackgroundColor);

  int PrimitiveState = 0;
  int iDrawMode = 0;

  if ((hint & EoDb::kSafe) == EoDb::kSafe) { PrimitiveState = pstate.Save(); }
  if ((hint & EoDb::kErase) == EoDb::kErase) { iDrawMode = pstate.SetROP2(DeviceContext, R2_XORPEN); }
  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialPenColorIndex(app.TrapHighlightColor()); }
  switch (hint) {
    case EoDb::kPrimitive:
    case EoDb::kPrimitiveSafe:
    case EoDb::kPrimitiveEraseSafe:
      ((EoDbPrimitive*)hintObject)->Display(this, DeviceContext);
      break;

    case EoDb::kGroup:
    case EoDb::kGroupSafe:
    case EoDb::kGroupEraseSafe:
    case EoDb::kGroupSafeTrap:
    case EoDb::kGroupEraseSafeTrap:
      ((EoDbGroup*)hintObject)->Display(this, DeviceContext);
      break;

    case EoDb::kGroups:
    case EoDb::kGroupsSafe:
    case EoDb::kGroupsSafeTrap:
    case EoDb::kGroupsEraseSafeTrap:
      ((EoDbGroupList*)hintObject)->Display(this, DeviceContext);
      break;

    case EoDb::kLayer:
    case EoDb::kLayerErase:
      ((EoDbLayer*)hintObject)->Display(this, DeviceContext);
      break;

    default:
      CView::OnUpdate(sender, hint, hintObject);
  }
  if ((hint & EoDb::kTrap) == EoDb::kTrap) { EoDbPrimitive::SetSpecialPenColorIndex(0); }
  if ((hint & EoDb::kErase) == EoDb::kErase) { pstate.SetROP2(DeviceContext, iDrawMode); }
  if ((hint & EoDb::kSafe) == EoDb::kSafe) { pstate.Restore(DeviceContext, PrimitiveState); }
  DeviceContext->SetBkColor(BackgroundColor);
  ReleaseDC(DeviceContext);
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

void AeSysView::OnEndPrinting(CDC* /* deviceContext */, CPrintInfo* /* printInformation */) {
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
      double HorizontalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;
      double VerticalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch / m_PlotScaleFactor;

      m_ViewTransform.Initialize(m_Viewport);
      m_ViewTransform.SetWindow(0.0, 0.0, HorizontalSizeInInches, VerticalSizeInInches);

      UINT nHorzPages;
      UINT nVertPages;

      NumPages(deviceContext, m_PlotScaleFactor, nHorzPages, nVertPages);

      double dX = ((pInfo->m_nCurPage - 1) % nHorzPages) * HorizontalSizeInInches;
      double dY = ((pInfo->m_nCurPage - 1) / nHorzPages) * VerticalSizeInInches;

      m_ViewTransform.SetTarget(EoGePoint3d(dX, dY, 0.0));
      m_ViewTransform.SetPosition(EoGeVector3d::kZAxis);
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

void AeSysView::OnLButtonDown(UINT flags, CPoint point) {
  if (AeSys::CustomLButtonDownCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnLButtonDown(flags, point);
  } else {
    DoCustomMouseClick(AeSys::CustomLButtonDownCharacters);
  }
}

void AeSysView::OnLButtonUp(UINT flags, CPoint point) {
  if (AeSys::CustomLButtonUpCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnLButtonUp(flags, point);
  } else {
    DoCustomMouseClick(AeSys::CustomLButtonUpCharacters);
  }
}

void AeSysView::OnRButtonDown(UINT flags, CPoint point) {
  if (AeSys::CustomRButtonDownCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnRButtonDown(flags, point);
  } else {
    DoCustomMouseClick(AeSys::CustomRButtonDownCharacters);
  }
}

void AeSysView::OnRButtonUp(UINT flags, CPoint point) {
  if (AeSys::CustomRButtonUpCharacters.IsEmpty() || !(GetKeyState(VK_SHIFT) & 0x8000)) {
    CView::OnRButtonUp(flags, point);
  } else {
    DoCustomMouseClick(AeSys::CustomRButtonUpCharacters);
  }
}

void AeSysView::OnMButtonDown(UINT /* flags */, CPoint point) {
  m_middleButtonPanStartPoint = point;
  m_middleButtonPanInProgress = true;
  SetCapture();
}

void AeSysView::OnMButtonUp(UINT /* flags */, CPoint /* point */) {
  if (m_middleButtonPanInProgress) {
    m_middleButtonPanInProgress = false;
    ReleaseCapture();
  }
}

void AeSysView::OnMouseMove(UINT, CPoint point) {
  if (m_middleButtonPanInProgress) {
    auto delta = point - m_middleButtonPanStartPoint;
    m_middleButtonPanStartPoint = point;

    EoGePoint3d target = m_ViewTransform.Target();
  
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
  if (m_RubberbandType == Lines) {
    CDC* DeviceContext = GetDC();
    int DrawMode = DeviceContext->SetROP2(R2_XORPEN);
    CPen GreyPen(PS_SOLID, 0, RubberbandColor);
    CPen* Pen = DeviceContext->SelectObject(&GreyPen);

    DeviceContext->MoveTo(m_RubberbandLogicalBeginPoint);
    DeviceContext->LineTo(m_RubberbandLogicalEndPoint);

    m_RubberbandLogicalEndPoint = point;
    DeviceContext->MoveTo(m_RubberbandLogicalBeginPoint);
    DeviceContext->LineTo(m_RubberbandLogicalEndPoint);
    DeviceContext->SelectObject(Pen);
    DeviceContext->SetROP2(DrawMode);
    ReleaseDC(DeviceContext);
  } else if (m_RubberbandType == Rectangles) {
    CDC* DeviceContext = GetDC();
    int DrawMode = DeviceContext->SetROP2(R2_XORPEN);
    CPen GreyPen(PS_SOLID, 0, RubberbandColor);
    CPen* Pen = DeviceContext->SelectObject(&GreyPen);
    CBrush* Brush = (CBrush*)DeviceContext->SelectStockObject(NULL_BRUSH);

    DeviceContext->Rectangle(m_RubberbandLogicalBeginPoint.x, m_RubberbandLogicalBeginPoint.y, m_RubberbandLogicalEndPoint.x, m_RubberbandLogicalEndPoint.y);

    m_RubberbandLogicalEndPoint = point;
    DeviceContext->Rectangle(m_RubberbandLogicalBeginPoint.x, m_RubberbandLogicalBeginPoint.y, m_RubberbandLogicalEndPoint.x, m_RubberbandLogicalEndPoint.y);
    DeviceContext->SelectObject(Brush);
    DeviceContext->SelectObject(Pen);
    DeviceContext->SetROP2(DrawMode);
    ReleaseDC(DeviceContext);
  }
}

BOOL AeSysView::OnMouseWheel(UINT flags, EoInt16 zDelta, CPoint point) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"AeSysView<%p>OnMouseWheel(%i, %i, %08.8lx)\n", this, flags, zDelta, point);

  if (zDelta > 0) {
    OnWindowZoomIn();
  } else {
    OnWindowZoomOut();
  }
  return __super::OnMouseWheel(flags, zDelta, point);
}

void AeSysView::OnSize(UINT type, int cx, int cy) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 3, L"AeSysView<%p>OnSize(%i, %i, %i)\n", this, type, cx, cy);

  if (cx && cy) {
    SetViewportSize(cx, cy);
    m_ViewTransform.Initialize(m_Viewport);
#if defined(USING_Direct2D)
    if (m_RenderTarget) { m_RenderTarget->Resize(D2D1::SizeU(cx, cy)); }
#endif  // USING_Direct2D
    m_OverviewViewTransform = m_ViewTransform;
  }
}

void AeSysView::OnTimer(UINT_PTR nIDEvent) {
  ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"AeSysView<%p>::OnTimer(%i)\n", this, nIDEvent);

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

  double UExtent = fabs(static_cast<double>(uMax - uMin));
  double VExtent = fabs(static_cast<double>(vMax - vMin));

  double XAdjustment = 0.0;
  double YAdjustment = 0.0;

  double Scale = 1.0 - (m_Viewport.WidthInInches() / UExtent) / ratio;

  if (fabs(Scale) > FLT_EPSILON) {
    XAdjustment = Scale * UExtent;
    YAdjustment = Scale * VExtent;
  }
  if (UExtent <= FLT_EPSILON || VExtent / UExtent > AspectRatio) {
    XAdjustment += (VExtent / AspectRatio - UExtent) * 0.5;
  } else {
    YAdjustment += (UExtent * AspectRatio - VExtent) * 0.5f;
  }
  uMin -= XAdjustment;
  uMax += XAdjustment;
  vMin -= YAdjustment;
  vMax += YAdjustment;
}

void AeSysView::InvokeNewModelTransform() { m_ModelTransform.InvokeNew(); }

void AeSysView::SetLocalModelTransform(EoGeTransformMatrix& transformation) { m_ModelTransform.SetLocalTM(transformation); }

void AeSysView::ReturnModelTransform() { m_ModelTransform.Return(); }

void AeSysView::BackgroundImageDisplay(CDC* deviceContext) {
  if (m_viewBackgroundImage && (static_cast<HBITMAP>(m_backgroundImageBitmap) != 0)) {
    int iWidDst = int(m_Viewport.Width());
    int iHgtDst = int(m_Viewport.Height());

    BITMAP bm;
    m_backgroundImageBitmap.GetBitmap(&bm);
    CDC dcMem;
    dcMem.CreateCompatibleDC(nullptr);
    CBitmap* pBitmap = dcMem.SelectObject(&m_backgroundImageBitmap);
    CPalette* pPalette = deviceContext->SelectPalette(&m_backgroundImagePalette, FALSE);
    deviceContext->RealizePalette();

    EoGePoint3d Target = m_ViewTransform.Target();
    EoGePoint3d ptTargetOver = m_OverviewViewTransform.Target();
    double dU = Target.x - ptTargetOver.x;
    double dV = Target.y - ptTargetOver.y;

    // Determine the region of the bitmap to tranfer to display
    CRect rcWnd;
    rcWnd.left = Eo::Round((m_ViewTransform.UMin() - OverviewUMin() + dU) / OverviewUExt() * static_cast<double>(bm.bmWidth));
    rcWnd.top = Eo::Round((1.0 - (m_ViewTransform.VMax() - OverviewVMin() + dV) / OverviewVExt()) * static_cast<double>(bm.bmHeight));
    rcWnd.right = Eo::Round((m_ViewTransform.UMax() - OverviewUMin() + dU) / OverviewUExt() * static_cast<double>(bm.bmWidth));
    rcWnd.bottom = Eo::Round((1.0 - (m_ViewTransform.VMin() - OverviewVMin() + dV) / OverviewVExt()) * static_cast<double>(bm.bmHeight));

    int iWidSrc = rcWnd.Width();
    int iHgtSrc = rcWnd.Height();

    deviceContext->StretchBlt(0, 0, iWidDst, iHgtDst, &dcMem, (int)rcWnd.left, (int)rcWnd.top, iWidSrc, iHgtSrc, SRCCOPY);

    dcMem.SelectObject(pBitmap);
    deviceContext->SelectPalette(pPalette, FALSE);
  }
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
  EoGeTransformMatrix tm;
  auto* document = GetDocument();

  document->GetExtents(this, ptMin, ptMax, tm);

  double HorizontalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(HORZSIZE)) / Eo::MmPerInch;
  double VerticalSizeInInches = static_cast<double>(deviceContext->GetDeviceCaps(VERTSIZE)) / Eo::MmPerInch;

  horizontalPages = static_cast<UINT>(Eo::Round(((ptMax.x - ptMin.x) * scaleFactor / HorizontalSizeInInches) + 0.5f));
  verticalPages = static_cast<UINT>(Eo::Round(((ptMax.y - ptMin.y) * scaleFactor / VerticalSizeInInches) + 0.5f));

  return horizontalPages * verticalPages;
}

void AeSysView::DisplayPixel(CDC* deviceContext, COLORREF cr, const EoGePoint3d& point) {
  EoGePoint4d ptView(point);

  ModelViewTransformPoint(ptView);

  if (ptView.IsInView()) { deviceContext->SetPixel(DoProjection(ptView), cr); }
}

void AeSysView::DoCameraRotate(int iDir) {
  EoGeVector3d vN = m_ViewTransform.Target() - m_ViewTransform.Position();
  vN.Normalize();

  EoGeVector3d vU = EoGeCrossProduct(ViewUp(), vN);
  vU.Normalize();

  EoGeVector3d vV = EoGeCrossProduct(vN, vU);
  vU.Normalize();

  EoGePoint3d Position = m_ViewTransform.Position();
  EoGePoint3d Target = m_ViewTransform.Target();
  switch (iDir) {
    case ID_CAMERA_ROTATELEFT: {
      Position = Position.RotateAboutAxis(Target, vV, Eo::DegreeToRadian(-10.0));
      break;
    }
    case ID_CAMERA_ROTATERIGHT:
      Position = Position.RotateAboutAxis(Target, vV, Eo::DegreeToRadian(10.0));
      break;

    case ID_CAMERA_ROTATEUP:
      Position = Position.RotateAboutAxis(Target, vU, Eo::DegreeToRadian(-10.0));
      break;

    case ID_CAMERA_ROTATEDOWN:
      Position = Position.RotateAboutAxis(Target, vU, Eo::DegreeToRadian(10.0));
      break;
  }
  m_ViewTransform.SetPosition(Position);
  m_ViewTransform.SetViewUp(vV);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::DoWindowPan(double ratio) {
  ratio = std::min(std::max(ratio, m_MinimumWindowRatio), m_MaximumWindowRatio);

  double UExtent = m_Viewport.WidthInInches() / ratio;
  double VExtent = m_Viewport.HeightInInches() / ratio;

  m_ViewTransform.SetCenteredWindow(m_Viewport, UExtent, VExtent);

  EoGePoint3d CursorPosition = GetCursorPosition();

  EoGeVector3d Direction = m_ViewTransform.Direction();
  EoGePoint3d Target = m_ViewTransform.Target();

  EoGeLine::IntersectionWithPln(CursorPosition, Direction, Target, Direction, &CursorPosition);

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(Direction);
  m_ViewTransform.BuildTransformMatrix();

  SetCursorPosition(CursorPosition);
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnSetupScale() {
  EoDlgSetScale dlg;
  dlg.m_Scale = GetWorldScale();
  if (dlg.DoModal() == IDOK) { SetWorldScale(dlg.m_Scale); }
}

void AeSysView::On3dViewsTop() {
  m_ViewTransform.SetPosition(EoGeVector3d::kZAxis);
  m_ViewTransform.SetDirection(EoGeVector3d::kZAxis);
  m_ViewTransform.SetViewUp(EoGeVector3d::kYAxis);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsBottom() {
  m_ViewTransform.SetPosition(-EoGeVector3d::kZAxis);
  m_ViewTransform.SetDirection(-EoGeVector3d::kZAxis);
  m_ViewTransform.SetViewUp(EoGeVector3d::kYAxis);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsLeft() {
  m_ViewTransform.SetPosition(-EoGeVector3d::kXAxis);
  m_ViewTransform.SetDirection(-EoGeVector3d::kXAxis);
  m_ViewTransform.SetViewUp(EoGeVector3d::kZAxis);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsRight() {
  m_ViewTransform.SetPosition(EoGeVector3d::kXAxis);
  m_ViewTransform.SetDirection(EoGeVector3d::kXAxis);
  m_ViewTransform.SetViewUp(EoGeVector3d::kZAxis);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsFront() {
  m_ViewTransform.SetPosition(-EoGeVector3d::kYAxis);
  m_ViewTransform.SetDirection(-EoGeVector3d::kYAxis);
  m_ViewTransform.SetViewUp(EoGeVector3d::kZAxis);
  m_ViewTransform.EnablePerspective(false);
  m_ViewTransform.BuildTransformMatrix();
  InvalidateRect(nullptr, TRUE);
}

void AeSysView::On3dViewsBack() {
  m_ViewTransform.SetPosition(EoGeVector3d::kYAxis);
  m_ViewTransform.SetDirection(EoGeVector3d::kYAxis);
  m_ViewTransform.SetViewUp(EoGeVector3d::kZAxis);
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

    EoGeVector3d ViewUp = EoGeCrossProduct(Direction, EoGeVector3d::kZAxis);
    ViewUp = EoGeCrossProduct(ViewUp, Direction);
    ViewUp.Normalize();

    m_ViewTransform.SetViewUp(ViewUp);
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
  HMENU WindowMenu = ::LoadMenu(app.GetInstance(), MAKEINTRESOURCE(IDR_WINDOW));
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

    EoGeTransformMatrix tm;
    document->GetExtents(this, ptMin, ptMax, tm);

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
  EoGePoint3d Target = m_ViewTransform.Target();

  Target.x -= 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowPanRight() {
  EoGePoint3d Target = m_ViewTransform.Target();

  Target.x += 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowPanUp() {
  EoGePoint3d Target = m_ViewTransform.Target();

  Target.y += 1.0 / (m_Viewport.WidthInInches() / m_ViewTransform.UExtent());

  m_ViewTransform.SetTarget(Target);
  m_ViewTransform.SetPosition(m_ViewTransform.Direction());
  m_ViewTransform.BuildTransformMatrix();

  InvalidateRect(nullptr, TRUE);
}

void AeSysView::OnWindowPanDown() {
  EoGePoint3d Target = m_ViewTransform.Target();

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
  EoDlgSetLength dlg;

  dlg.m_strTitle = L"Set Dimension Length";
  dlg.m_dLength = app.DimensionLength();
  if (dlg.DoModal() == IDOK) {
    app.SetDimensionLength(dlg.m_dLength);
    UpdateStateInformation(DimLen);
#if defined(USING_DDE)
    dde::PostAdvise(dde::DimLenInfo);
#endif  // USING_DDE
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
  EoDlgSetupCustomMouseCharacters Dialog;
  if (Dialog.DoModal() == IDOK) {}
}

void AeSysView::OnRelativeMovesEngDown() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.y -= app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle());
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngDownRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.y -= app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngIn() {
  EoGePoint3d pt = GetCursorPosition();
  pt.z -= app.EngagedLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngLeft() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.x -= app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle());
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngLeftRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.x -= app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngOut() {
  EoGePoint3d pt = GetCursorPosition();
  pt.z += app.EngagedLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngRight() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.x += app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle());
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngRightRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.x += app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngUp() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.y += app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle());
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesEngUpRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.y += app.EngagedLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, app.EngagedAngle() + Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesRight() {
  EoGePoint3d pt = GetCursorPosition();
  pt.x += app.DimensionLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesUp() {
  EoGePoint3d pt = GetCursorPosition();
  pt.y += app.DimensionLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesLeft() {
  EoGePoint3d pt = GetCursorPosition();
  pt.x -= app.DimensionLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesDown() {
  EoGePoint3d pt = GetCursorPosition();
  pt.y -= app.DimensionLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesIn() {
  EoGePoint3d pt = GetCursorPosition();
  pt.z -= app.DimensionLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesOut() {
  EoGePoint3d pt = GetCursorPosition();
  pt.z += app.DimensionLength();
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesRightRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.x += app.DimensionLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesUpRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.y += app.DimensionLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesLeftRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.x -= app.DimensionLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnRelativeMovesDownRotate() {
  EoGePoint3d pt = GetCursorPosition();
  EoGePoint3d ptSec = pt;
  ptSec.y -= app.DimensionLength();
  pt = ptSec.RotateAboutAxis(pt, EoGeVector3d::kZAxis, Eo::DegreeToRadian(app.DimensionAngle()));
  SetCursorPosition(pt);
}

void AeSysView::OnToolsPrimitiveSnapto() {
  EoGePoint3d pt = GetCursorPosition();

  EoGePoint3d ptDet;

  if (GroupIsEngaged()) {
    EoDbPrimitive* Primitive = m_EngagedPrimitive;

    EoGePoint4d ptView(pt);
    ModelViewTransformPoint(ptView);

    EoDbPolygon::EdgeToEvaluate() = EoDbPolygon::Edge();

    if (Primitive->SelectUsingPoint(this, ptView, ptDet)) {
      ptDet = Primitive->GoToNxtCtrlPt();
      m_ptDet = ptDet;

      Primitive->AddReportToMessageList(ptDet);
      SetCursorPosition(ptDet);
      return;
    }
  }
  if (SelectGroupAndPrimitive(pt) != 0) {
    ptDet = m_ptDet;
    m_EngagedPrimitive->AddReportToMessageList(ptDet);
    SetCursorPosition(ptDet);
  }
}

void AeSysView::OnPrimPerpJump() {
  EoGePoint3d pt = GetCursorPosition();

  if (SelectGroupAndPrimitive(pt) != 0) {
    if (m_EngagedPrimitive->Is(EoDb::kLinePrimitive)) {
      EoDbLine* pPrimLine = static_cast<EoDbLine*>(m_EngagedPrimitive);
      pt = pPrimLine->ProjPt(m_ptCursorPosWorld);
      SetCursorPosition(pt);
    }
  }
}

void AeSysView::OnHelpKey() { ::WinHelpW(GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"READY")); }

AeSysView* AeSysView::GetActiveView() {
  CMDIFrameWndEx* MDIFrameWnd = (CMDIFrameWndEx*)AfxGetMainWnd();

  if (MDIFrameWnd == nullptr) { return nullptr; }
  CMDIChildWndEx* MDIChildWnd = DYNAMIC_DOWNCAST(CMDIChildWndEx, MDIFrameWnd->MDIGetActive());

  if (MDIChildWnd == nullptr) { return nullptr; }
  CView* View = MDIChildWnd->GetActiveView();

  if (!View->IsKindOf(RUNTIME_CLASS(AeSysView))) {  // View is the wrong kind (this could occur with splitter windows, or additional views in a single document.
    return nullptr;
  }
  return (AeSysView*)View;
}

void AeSysView::OnUpdateViewOdometer(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewOdometer); }

#define LEGACY_ODOMETER
#if defined(LEGACY_ODOMETER)
static void DrawOdometerInView(AeSysView* view, CDC* context, AeSys::Units Units, EoGeVector3d& position) {
  auto* oldFont = static_cast<CFont*>(context->SelectStockObject(DEFAULT_GUI_FONT));
  auto oldTextAlign = context->SetTextAlign(TA_LEFT | TA_TOP);
  auto oldTextColor = context->SetTextColor(AppGetTextCol());
  auto oldBackgroundColor = context->SetBkColor(~AppGetTextCol() & 0x00ffffff);

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
#endif  // defined(LEGACY_ODOMETER)

/// @brief Displays the odometer information showing the relative position from the grid origin to the current cursor position, and optionally the line length and angle if in rubber band line mode.
void AeSysView::DisplayOdometer() {
  EoGePoint3d position = GetCursorPosition();

  m_vRelPos = GridOrign() - position;

  if (m_ViewOdometer) {
    AeSys::Units Units = app.GetUnits();

    CString lengthText;

    app.FormatLength(lengthText, Units, m_vRelPos.x);
    CString Position = lengthText.TrimLeft();
    app.FormatLength(lengthText, Units, m_vRelPos.y);
    Position.Append(L", " + lengthText.TrimLeft());
    app.FormatLength(lengthText, Units, m_vRelPos.z);
    Position.Append(L", " + lengthText.TrimLeft());

    if (m_RubberbandType == Lines) {
      EoGeLine line(m_RubberbandBeginPoint, position);

      auto lineLength = line.Length();
      auto angleInXYPlane = line.AngleFromXAxisXY();
      app.FormatLength(lengthText, Units, lineLength);

      CString angle;
      app.FormatAngle(angle, angleInXYPlane, 8, 3);
      angle.ReleaseBuffer();
      Position.Append(L" [" + lengthText.TrimLeft() + L" @ " + angle + L"]");
    }
    auto* mainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
    mainFrame->SetPaneText(1, Position);
#if defined(LEGACY_ODOMETER)
    DrawOdometerInView(this, GetDC(), Units, m_vRelPos);
#endif  // defined(LEGACY_ODOMETER)
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

void AeSysView::OnUpdateBackgroundimageLoad(CCmdUI* pCmdUI) { pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) == 0); }

void AeSysView::OnUpdateBackgroundimageRemove(CCmdUI* pCmdUI) { pCmdUI->Enable(static_cast<HBITMAP>(m_backgroundImageBitmap) != 0); }

void AeSysView::OnUpdateViewPenwidths(CCmdUI* pCmdUI) { pCmdUI->SetCheck(m_ViewPenWidths); }

void AeSysView::DeleteLastGroup() {
  if (m_VisibleGroupList.IsEmpty()) {
    app.AddStringToMessageList(IDS_MSG_NO_DET_GROUPS_IN_VIEW);
  } else {
    auto* Document = GetDocument();
    EoDbGroup* Group = m_VisibleGroupList.RemoveTail();

    Document->AnyLayerRemove(Group);
    if (Document->RemoveTrappedGroup(Group) != 0) {  // Display it normal color so the erase xor will work
      Document->UpdateAllViews(nullptr, EoDb::kGroupSafe, Group);
      UpdateStateInformation(TrapCount);
    }
    Document->UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
    Document->DeletedGroupsAddHead(Group);
    app.AddStringToMessageList(IDS_SEG_DEL_TO_RESTORE);
  }
}

void AeSysView::BreakAllPolylines() {
  auto Position = GetFirstVisibleGroupPosition();
  while (Position != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(Position);
    Group->BreakPolylines();
  }
}

void AeSysView::BreakAllSegRefs() {
  auto Position = GetFirstVisibleGroupPosition();
  while (Position != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(Position);
    Group->BreakSegRefs();
  }
}

void AeSysView::ResetView() {
  m_EngagedGroup = 0;
  m_EngagedPrimitive = 0;
}

EoDbGroup* AeSysView::SelSegAndPrimAtCtrlPt(const EoGePoint4d& pt) {
  EoDbPrimitive* Primitive;
  EoGePoint3d ptEng;

  m_EngagedGroup = 0;
  m_EngagedPrimitive = 0;

  EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

  auto Position = GetFirstVisibleGroupPosition();
  while (Position != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(Position);
    Primitive = Group->SelPrimAtCtrlPt(this, pt, &ptEng);
    if (Primitive != 0) {
      m_ptDet = ptEng;
      m_ptDet = tm * m_ptDet;
      m_EngagedGroup = Group;
      m_EngagedPrimitive = Primitive;
    }
  }
  return (m_EngagedGroup);
}

EoDbGroup* AeSysView::SelectGroupAndPrimitive(const EoGePoint3d& pt) {
  EoGePoint3d ptEng;

  m_EngagedGroup = 0;
  m_EngagedPrimitive = 0;

  EoGePoint4d ptView(pt);
  ModelViewTransformPoint(ptView);

  EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

  double dPicApert = m_SelectApertureSize;

  EoDbPolygon::EdgeToEvaluate() = 0;

  auto Position = GetFirstVisibleGroupPosition();
  while (Position != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(Position);
    EoDbPrimitive* Primitive = Group->SelPrimUsingPoint(this, ptView, dPicApert, ptEng);
    if (Primitive != 0) {
      m_ptDet = ptEng;
      m_ptDet = tm * m_ptDet;
      m_EngagedGroup = Group;
      m_EngagedPrimitive = Primitive;
      return (Group);
    }
  }
  return 0;
}

EoDbGroup* AeSysView::SelectCircleUsingPoint(EoGePoint3d& point, double tolerance, EoDbEllipse*& circle) {
  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);
    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->Is(EoDb::kEllipsePrimitive)) {
        EoDbEllipse* Arc = static_cast<EoDbEllipse*>(Primitive);

        if (fabs(Arc->GetSwpAng() - Eo::TwoPi) <= DBL_EPSILON && (Arc->GetMajAx().SquaredLength() - Arc->GetMinAx().SquaredLength()) <= DBL_EPSILON) {
          if (point.DistanceTo(Arc->Center()) <= tolerance) {
            circle = Arc;
            return Group;
          }
        }
      }
    }
  }
  return 0;
}

EoDbGroup* AeSysView::SelectLineUsingPoint(EoGePoint3d& point, EoDbLine*& line) {
  EoGePoint4d ptView(point);
  ModelViewTransformPoint(ptView);

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);
    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->Is(EoDb::kLinePrimitive)) {
        EoGePoint3d PointOnLine;
        if (Primitive->SelectUsingPoint(this, ptView, PointOnLine)) {
          line = static_cast<EoDbLine*>(Primitive);
          return Group;
        }
      }
    }
  }
  return 0;
}

EoDbGroup* AeSysView::SelectPointUsingPoint(EoGePoint3d& point, double tolerance, EoInt16 pointColor, EoInt16 pointStyle, EoDbPoint*& primitive) {
  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);
    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->Is(EoDb::kPointPrimitive)) {
        EoDbPoint* Point = static_cast<EoDbPoint*>(Primitive);

        if (Point->PenColor() == pointColor && Point->PointStyle() == pointStyle) {
          if (point.DistanceTo(Point->GetPt()) <= tolerance) {
            primitive = Point;
            return Group;
          }
        }
      }
    }
  }
  return 0;
}

EoDbGroup* AeSysView::SelectLineUsingPoint(const EoGePoint3d& pt) {
  m_EngagedGroup = 0;
  m_EngagedPrimitive = 0;

  EoGePoint3d ptEng;

  EoGePoint4d ptView(pt);
  ModelViewTransformPoint(ptView);

  double tol = m_SelectApertureSize;

  EoGeTransformMatrix tm = ModelViewGetMatrixInverse();

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);
    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->Is(EoDb::kLinePrimitive)) {
        if (Primitive->SelectUsingPoint(this, ptView, ptEng)) {
          tol = ptView.DistanceToPointXY(EoGePoint4d(ptEng));

          m_ptDet = ptEng;
          m_ptDet = tm * m_ptDet;
          m_EngagedGroup = Group;
          m_EngagedPrimitive = Primitive;
        }
      }
    }
  }
  return (m_EngagedGroup);
}

EoDbText* AeSysView::SelectTextUsingPoint(const EoGePoint3d& pt) {
  EoGePoint4d ptView(pt);
  ModelViewTransformPoint(ptView);

  auto GroupPosition = GetFirstVisibleGroupPosition();
  while (GroupPosition != 0) {
    EoDbGroup* Group = GetNextVisibleGroup(GroupPosition);
    auto PrimitivePosition = Group->GetHeadPosition();
    while (PrimitivePosition != 0) {
      EoDbPrimitive* Primitive = Group->GetNext(PrimitivePosition);
      if (Primitive->Is(EoDb::kTextPrimitive)) {
        EoGePoint3d ptProj;
        if (static_cast<EoDbText*>(Primitive)->SelectUsingPoint(this, ptView, ptProj)) return static_cast<EoDbText*>(Primitive);
      }
    }
  }
  return 0;
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

void AeSysView::OnFind() {
  CString findComboText;

  CWnd* mainWnd = AfxGetMainWnd();
  CMainFrame* mainFrame = mainWnd ? DYNAMIC_DOWNCAST(CMainFrame, mainWnd) : nullptr;
  if (!mainFrame) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"AeSysView::OnFind() - main frame is null\n");
    return;
  }

  auto* findCombo = mainFrame->GetFindCombo();
  if (!findCombo) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"AeSysView::OnFind() - find combo is null\n");
    return;
  }

  VerifyFindString(findCombo, findComboText);

  if (!findComboText.IsEmpty()) {
    ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"AeSysView::OnFind() ComboText = %ls\n", static_cast<LPCTSTR>(findComboText));
  }
}

void AeSysView::VerifyFindString(CMFCToolBarComboBoxButton* findComboBox, CString& findText) {
  if (findComboBox == nullptr) { return; }
  BOOL IsLastCommandFromButton = CMFCToolBar::IsLastCommandFromButton(findComboBox);

  if (IsLastCommandFromButton) { findText = findComboBox->GetText(); }
  CComboBox* ComboBox = findComboBox->GetComboBox();

  if (!findText.IsEmpty()) {
    const int Count = ComboBox->GetCount();
    int Position = 0;

    while (Position < Count) {
      CString LBText;
      ComboBox->GetLBText(Position, LBText);

      if (LBText.GetLength() == findText.GetLength()) {
        if (LBText == findText) { break; }
      }
      Position++;
    }
    if (Position < Count) {  // Text need to move to initial position
      ComboBox->DeleteString(static_cast<UINT>(Position));
    }
    ComboBox->InsertString(0, findText);
    ComboBox->SetCurSel(0);

    if (!IsLastCommandFromButton) { findComboBox->SetText(findText); }
  }
}

void AeSysView::OnEditFind() { ATLTRACE2(static_cast<int>(atlTraceGeneral), 1, L"AeSysView::OnEditFind() - Entering\n"); }

// Disables rubberbanding.
void AeSysView::RubberBandingDisable() {
  if (m_RubberbandType != None) {
    CDC* DeviceContext = GetDC();
    int DrawMode = DeviceContext->SetROP2(R2_XORPEN);
    CPen GreyPen(PS_SOLID, 0, RubberbandColor);
    CPen* Pen = DeviceContext->SelectObject(&GreyPen);

    if (m_RubberbandType == Lines) {
      DeviceContext->MoveTo(m_RubberbandLogicalBeginPoint);
      DeviceContext->LineTo(m_RubberbandLogicalEndPoint);
    } else if (m_RubberbandType == Rectangles) {
      CBrush* Brush = (CBrush*)DeviceContext->SelectStockObject(NULL_BRUSH);
      DeviceContext->Rectangle(m_RubberbandLogicalBeginPoint.x, m_RubberbandLogicalBeginPoint.y, m_RubberbandLogicalEndPoint.x, m_RubberbandLogicalEndPoint.y);
      DeviceContext->SelectObject(Brush);
    }
    DeviceContext->SelectObject(Pen);
    DeviceContext->SetROP2(DrawMode);
    ReleaseDC(DeviceContext);
    m_RubberbandType = None;
  }
}

void AeSysView::RubberBandingStartAtEnable(EoGePoint3d pt, ERubs type) {
  EoGePoint4d ptView(pt);

  ModelViewTransformPoint(ptView);

  if (ptView.IsInView()) {
    m_RubberbandBeginPoint = pt;

    m_RubberbandLogicalBeginPoint = DoProjection(ptView);
    m_RubberbandLogicalEndPoint = m_RubberbandLogicalBeginPoint;
  }
  m_RubberbandType = type;
}

EoGePoint3d AeSysView::GetCursorPosition() {
  CPoint CursorPosition;

  ::GetCursorPos(&CursorPosition);
  ScreenToClient(&CursorPosition);

  EoGePoint3d pt(double(CursorPosition.x), double(CursorPosition.y), m_ptCursorPosDev.z);
  if (pt != m_ptCursorPosDev) {
    m_ptCursorPosDev = pt;
    m_ptCursorPosWorld = m_ptCursorPosDev;

    DoProjectionInverse(m_ptCursorPosWorld);

    m_ptCursorPosWorld = ModelViewGetMatrixInverse() * m_ptCursorPosWorld;
    m_ptCursorPosWorld = SnapPointToGrid(m_ptCursorPosWorld);
  }
  return (m_ptCursorPosWorld);
}

void AeSysView::SetCursorPosition(EoGePoint3d cursorPosition) {
  EoGePoint4d ptView(cursorPosition);

  ModelViewTransformPoint(ptView);

  if (!ptView.IsInView()) {  // Redefine the view so targeted position becomes center
    m_ViewTransform.SetTarget(cursorPosition);
    m_ViewTransform.SetPosition(m_ViewTransform.Direction());
    m_ViewTransform.BuildTransformMatrix();

    InvalidateRect(nullptr, TRUE);

    ptView = cursorPosition;
    ModelViewTransformPoint(ptView);
  }
  // Move the cursor to specified position.
  CPoint pntCurPos = DoProjection(ptView);
  m_ptCursorPosDev(pntCurPos.x, pntCurPos.y, ptView.z / ptView.w);
  m_ptCursorPosWorld = cursorPosition;

  ClientToScreen(&pntCurPos);
  ::SetCursorPos(pntCurPos.x, pntCurPos.y);
}

void AeSysView::SetModeCursor(int mode) {
  EoUInt16 ResourceIdentifier;

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
  auto cursorHandle = static_cast<HCURSOR>(LoadImageW(app.GetInstance(), MAKEINTRESOURCE(ResourceIdentifier), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE));
  VERIFY(cursorHandle);
  SetCursor(cursorHandle);
  SetClassLongPtr(this->GetSafeHwnd(), GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursorHandle));
}

void AeSysView::SetWorldScale(const double scale) {
  if (scale > FLT_EPSILON) {
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
  if (m_ViewStateInformation) {
    auto* document = AeSysDoc::GetDoc();
    CDC* DeviceContext = GetDC();

    CFont* Font = (CFont*)DeviceContext->SelectStockObject(DEFAULT_GUI_FONT);
    UINT nTextAlign = DeviceContext->SetTextAlign(TA_LEFT | TA_TOP);
    COLORREF crText = DeviceContext->SetTextColor(AppGetTextCol());
    COLORREF crBk = DeviceContext->SetBkColor(~AppGetTextCol() & 0x00ffffff);

    TEXTMETRIC tm;
    DeviceContext->GetTextMetrics(&tm);

    CRect ClientRect;
    GetClientRect(&ClientRect);

    CRect rc;

    WCHAR szBuf[32];

    if ((item & WorkCount) == WorkCount) {
      rc.SetRect(0, ClientRect.top, 8 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      swprintf_s(szBuf, 32, L"%-4i", document->NumberOfGroupsInWorkLayer() + document->NumberOfGroupsInActiveLayers());
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT)wcslen(szBuf), 0);
    }
    if ((item & TrapCount) == TrapCount) {
      rc.SetRect(8 * tm.tmAveCharWidth, ClientRect.top, 16 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      long trapCount = static_cast<long>(document->TrapGroupCount());
      swprintf_s(szBuf, 32, L"%-4ld", trapCount);
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT)wcslen(szBuf), 0);
    }
    if ((item & Pen) == Pen) {
      rc.SetRect(16 * tm.tmAveCharWidth, ClientRect.top, 22 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      swprintf_s(szBuf, 32, L"P%-4i", pstate.PenColor());
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT)wcslen(szBuf), 0);
    }
    if ((item & Line) == Line) {
      rc.SetRect(22 * tm.tmAveCharWidth, ClientRect.top, 28 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      swprintf_s(szBuf, 32, L"L%-4i", pstate.LineType());
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT)wcslen(szBuf), 0);
    }
    if ((item & TextHeight) == TextHeight) {
      EoDbCharacterCellDefinition ccd;
      pstate.GetCharCellDef(ccd);
      rc.SetRect(28 * tm.tmAveCharWidth, ClientRect.top, 38 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      swprintf_s(szBuf, 32, L"T%-6.2f", ccd.ChrHgtGet());
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT)wcslen(szBuf), 0);
    }
    if ((item & Scale) == Scale) {
      rc.SetRect(38 * tm.tmAveCharWidth, ClientRect.top, 48 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      swprintf_s(szBuf, 32, L"1:%-6.2f", GetWorldScale());
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, szBuf, (UINT)wcslen(szBuf), 0);
    }
    if ((item & WndRatio) == WndRatio) {
      rc.SetRect(48 * tm.tmAveCharWidth, ClientRect.top, 58 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      double Ratio = WidthInInches() / UExtent();
      CString RatioAsString;
      RatioAsString.Format(L"=%-8.3f", Ratio);
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, RatioAsString, (UINT)RatioAsString.GetLength(), 0);
    }
    if ((item & DimLen) == DimLen || (item & DimAng) == DimAng) {
      rc.SetRect(58 * tm.tmAveCharWidth, ClientRect.top, 90 * tm.tmAveCharWidth, ClientRect.top + tm.tmHeight);
      CString LengthAndAngle;
      app.FormatLength(LengthAndAngle, app.GetUnits(), app.DimensionLength());
      LengthAndAngle.TrimLeft();
      CString Angle;
      app.FormatAngle(Angle, Eo::DegreeToRadian(app.DimensionAngle()), 8, 3);
      Angle.ReleaseBuffer();
      LengthAndAngle.Append(L" @ " + Angle);
      DeviceContext->ExtTextOutW(rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE, &rc, LengthAndAngle, (UINT)LengthAndAngle.GetLength(), 0);
    }
    DeviceContext->SetBkColor(crBk);
    DeviceContext->SetTextColor(crText);
    DeviceContext->SetTextAlign(nTextAlign);
    DeviceContext->SelectObject(Font);
    ReleaseDC(DeviceContext);
  }
}

#if defined(USING_Direct2D)
HRESULT AeSysView::CreateDeviceResources() {
  HRESULT hr = S_OK;

  if (!m_RenderTarget) {
    RECT rc;
    GetClientRect(&rc);

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    HWND WindowHandle = GetSafeHwnd();
    hr = app.m_Direct2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(WindowHandle, size), &m_RenderTarget);

    if (SUCCEEDED(hr)) { hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &m_LightSlateGrayBrush); }
    if (SUCCEEDED(hr)) { hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 0.3f), &m_RedBrush); }
  }
  return hr;
}

void AeSysView::DiscardDeviceResources() {
  SafeRelease(&m_RenderTarget);
  SafeRelease(&m_LightSlateGrayBrush);
  SafeRelease(&m_RedBrush);
}
#endif  // USING_Direct2D
