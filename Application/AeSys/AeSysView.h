#pragma once

#include <cstdint>
#include <d2d1.h>
#include <wrl/client.h>

#include "Eo.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoDlgPlot.h"
#include "EoGeLine.h"
#include "EoGeMatrix.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsModelTransform.h"
#include "EoGsViewTransform.h"
#include "EoGsViewport.h"
#include "EoLpdGeometry.h"
#include "EoMfLayoutTabBar.h"
#include "EoMfPrimitiveTooltip.h"
#include "EoModeConfig.h"
#include "EoPipeGeometry.h"
#include "EoPowerGeometry.h"
#include "Section.h"

#include <stack>

#include "AeSysState.h"
class AeSysDoc;
class EoDbBlock;
class EoDbConic;
class EoDbLayer;
class EoDbLine;
class EoDbPoint;
class EoDbText;
class EoDbViewport;
class EoMfOutputDockablePane;

class AeSysView : public CView {
 protected:
  AeSysView();
  AeSysView(const AeSysView&) = delete;
  AeSysView& operator=(const AeSysView&) = delete;

  DECLARE_DYNCREATE(AeSysView)

 public:
  enum EStateInformationItem {
    WorkCount = 0x0001,
    TrapCount = 0x0002,
    BothCounts = WorkCount | TrapCount,
    Pen = 0x0004,
    Line = 0x0008,
    TextHeight = 0x0010,
    WndRatio = 0x0020,
    Scale = 0x0040,
    DimLen = 0x0080,
    DimAng = 0x0100,
    Layer = 0x0200,
    All = BothCounts | Pen | Line | TextHeight | WndRatio | Scale | DimLen | DimAng | Layer
  };
  enum ERubs { None, Lines, Rectangles, RectanglesRemove, RectanglesWindow, RectanglesWindowRemove };

private:
 std::stack<std::unique_ptr<AeSysState>> m_stateStack;

public:
 /**
  * @brief Pushes a new state onto the state stack.
  * This function exits the current state (if any), enters the new state, and pushes it onto the stack.
  * It also triggers a redraw of the view.
  * @param newState A unique pointer to the new state to be pushed.
  */
 void PushState(std::unique_ptr<AeSysState> newState);

 /**
  * @brief Pops the current state from the state stack.
  * This function exits the current state, pops it from the stack, and enters the new top state (if any).
  * It also triggers a redraw of the view.
  */
 void PopState();

 /// @brief Empties the state stack, calling OnExit on each state in LIFO order.
 /// Call at the top of every OnMode*() switching command to ensure clean teardown
 /// regardless of which mode state was active.
 void PopAllModeStates();

 /**
  * @brief Gets a pointer to the current state.
  * @return A pointer to the current state, or nullptr if the stack is empty.
  */
 [[nodiscard]] AeSysState* GetCurrentState() const noexcept;

 /// Accessor for state classes that need the preview group.
 [[nodiscard]] EoDbGroup& PreviewGroup() noexcept { return m_PreviewGroup; }
 private:
  EoGsModelTransform m_ModelTransform;
  EoGsViewport m_Viewport;
  EoGsViewTransform m_ViewTransform;

  // Off-screen back buffer
  CDC m_backBufferDC;
  CBitmap m_backBuffer;
  CSize m_backBufferSize{0, 0};
  // Overlay composite buffer — holds (scene + transient overlays). Composited from
  // m_backBuffer on every overlay-dirty paint; used as the source of the screen BitBlt
  // so simple expose events become a single BitBlt with no GDI primitive calls.
  CDC m_overlayDC;
  CBitmap m_overlayBuffer;
  bool m_sceneInvalid{true};
  bool m_overlayDirty{};

  // Direct2D render target
  Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_d2dRenderTarget;
  // Cached scene bitmap target — DisplayAllLayers renders into this once per scene
  // change. Per-frame composition copies the bitmap to the HWND target and draws
  // transient overlays (unique points, preview, rubberband) over it. Eliminates
  // per-frame entity rendering when only overlays change (e.g. rubberband drag).
  Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> m_d2dSceneTarget;
  bool m_useD2D{true};
  bool m_d2dAliased{};
  bool m_useD2DForPrint{true};  ///< Use D2D DC render target for printing (GDI fallback if false)

  CBitmap m_backgroundImageBitmap{};
  CPalette m_backgroundImagePalette{};
  EoDbPrimitive* m_EngagedPrimitive{};
  EoDbGroup* m_EngagedGroup{};
  std::uint16_t m_OpHighlighted{};
  EoGsViewTransform m_OverviewViewTransform{};
  bool m_Plot{};
  double m_PlotScaleFactor{1.0};
  PlotSettings m_plotSettings;  ///< Last-used plot dialog output; consumed by the print lifecycle.
  EoDbGroup m_PreviewGroup{};
  EoGsViewTransform m_PreviousViewTransform{};
  double m_SelectApertureSize{0.005};
  bool m_viewBackgroundImage{};
  bool m_ViewPenWidths{};
  CViewports m_Viewports{};
  bool m_ViewRendered{};
  EoGsViewTransforms m_ViewTransforms{};
  bool m_ViewTrueTypeFonts{true};
  bool m_ViewWireframe{};
  EoDbGroupList m_VisibleGroupList{};
  double m_WorldScale{1.0};

  EoGePoint3d m_ptDet{};

  EoGeVector3d m_vRelPos{};

  EoMfLayoutTabBar m_layoutTabBar;

  /// @brief The currently activated viewport in paper space (double-click to enter/exit).
  /// When non-null, the view is in viewport-activated mode: the accent border is drawn
  /// around this viewport and entity creation is routed to model-space.
  EoDbViewport* m_activeViewportPrimitive{};

  /// @brief Saved paper-space work layer during viewport activation.
  /// When a viewport is activated, the current paper-space work layer is saved here
  /// and restored when the viewport is deactivated.
  EoDbLayer* m_savedWorkLayerForViewport{};

 private:  // grid and axis constraints
  EoGePoint3d m_GridOrigin{};
  int m_MaximumDotsPerLine{64};

  double m_XGridLineSpacing{12.0};
  double m_YGridLineSpacing{12.0};
  double m_ZGridLineSpacing{12.0};

  double m_XGridSnapSpacing{1.0};
  double m_YGridSnapSpacing{1.0};
  double m_ZGridSnapSpacing{1.0};

  double m_XGridPointSpacing{3.0};
  double m_YGridPointSpacing{3.0};
  double m_ZGridPointSpacing{};

  double m_AxisConstraintInfluenceAngle{5.0};
  double m_AxisConstraintOffsetAngle{};
  bool m_DisplayGridWithLines{};
  bool m_DisplayGridWithPoints{};
  bool m_GridSnap{};

 public:
  double AxisConstraintInfluenceAngle() const noexcept;
  void SetAxisConstraintInfluenceAngle(double angle) noexcept;
  double AxisConstraintOffsetAngle() const noexcept;
  void SetAxisConstraintOffsetAngle(double angle) noexcept;
  void InitializeConstraints() noexcept;
  /// @brief Generates a point display centered about the user origin in one or more of the three orthogonal planes
  /// for the current user grid.
  void DisplayGrid(CDC* deviceContext);
  EoGePoint3d GridOrign() const noexcept;
  void GridOrign(const EoGePoint3d& origin) noexcept;
  void GetGridLineSpacing(double& x, double& y, double& z) const noexcept;
  void SetGridLineSpacing(double x, double y, double z) noexcept;
  void GetGridPointSpacing(double& x, double& y, double& z) const noexcept;
  void SetGridPointSpacing(double x, double y, double z) noexcept;
  void GetGridSnapSpacing(double& x, double& y, double& z) const noexcept;
  void SetGridSnapSpacing(double x, double y, double z) noexcept;

  /** @brief Snaps a point to the nearest grid point based on the current grid snap spacing.
   * This function calculates the nearest grid point to the input point by rounding the coordinates of the input point
   * to the nearest multiple of the grid snap spacing. The resulting snapped point is returned.
   * @param point The point to be snapped to the grid.
   * @return The snapped point that is aligned with the nearest grid intersection.
   */
  [[nodiscard]] EoGePoint3d SnapPointToGrid(const EoGePoint3d& point) const;

  /** @brief Snaps a point to the nearest axis if it is within the influence angle of that axis.
   * This function checks the angle between the line from the begin point to the end point and each of the three
   * orthogonal axes. If the angle is less than the sum of the influence angle and offset angle, the end point is
   * snapped to that axis. If multiple axes are within the influence angle, the one with the smallest angle is chosen.
   * @param begin The starting point from which to measure angles.
   * @param end The point to be potentially snapped.
   * @return The new snapped point if snapping occurred, or the original end point if no snapping was applied.
   * @note Offset angle only support about z-axis
   */
  [[nodiscard]] EoGePoint3d SnapPointToAxis(const EoGePoint3d& begin, const EoGePoint3d& end) const;

  bool DisplayGridWithLines() const noexcept;
  void EnableDisplayGridWithLines(bool display) noexcept;
  void EnableDisplayGridWithPoints(bool display) noexcept;
  bool DisplayGridWithPoints() const noexcept;
  bool GridSnap() const noexcept;
  void EnableGridSnap(bool snap) noexcept;
  void ViewZoomExtents() noexcept {}

  /** @brief Applies the *ACTIVE VPORT table entry to the view transform.
   *
   * Maps the DXF VPORT fields (viewCenter, viewHeight, viewDirection, viewTargetPoint,
   * lensLength, viewTwistAngle, viewMode) onto EoGsViewTransform, establishing the
   * camera position, projection window, and perspective mode so the view matches
   * the saved viewport configuration without requiring a manual zoom-to-extents.
   *
   * Called from OnInitialUpdate() after the viewport device dimensions are established.
   * No-op if the document has no *ACTIVE VPORT entry or if viewHeight is non-positive.
   */
  void ApplyActiveViewport();

 public:
  AeSysDoc* GetDocument() const;

 protected:  // Overrides
  void OnActivateFrame(UINT nState, CFrameWnd* pDeactivateFrame) override;
  void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;

 public:
  void OnDraw(CDC* deviceContext) override;  // overridden to draw this view

  /** @brief Initialize view-specific resources and settings when the view is first created.
   * This includes setting the background color and preparing any necessary rendering resources.
   * @note This method is called by the MFC framework after the view window has been created but before it is displayed.
   * The default implementation of this function calls the OnUpdate member function with no hint information
   * Override this function to perform any one-time initialization that requires information about the document.
   */
  void OnInitialUpdate() override;

  BOOL PreCreateWindow(CREATESTRUCT& createStructure) override;

 protected:
  BOOL OnPreparePrinting(CPrintInfo* printInformation) override;
  void OnBeginPrinting(CDC* deviceContext, CPrintInfo* printInformation) override;
  void OnPrepareDC(CDC* deviceContext, CPrintInfo* printInformation) override;
  void OnEndPrinting(CDC* deviceContext, CPrintInfo* printInformation) override;

  /** @brief Display the view using hint information to optimize rendering.
   * This method is called by the MFC framework when the document or other views call UpdateAllViews with a hint
   * parameter. The hint and hintObject parameters can be used to determine what type of update occurred and optimize
   * the rendering accordingly. For example, if only a specific primitive was updated, the view can choose to redraw
   * only that primitive instead of the entire scene.
   * @param sender The view that sent the update notification, or nullptr if the document sent the notification.
   * @param hint An application-defined value that indicates the type of update. This can be used to optimize the
   * response.
   * @param hintObject An optional pointer to an object that provides additional information about the update. The
   * interpretation of this pointer depends on the value of hint.
   * @param renderDevice A pointer to the rendering device context, which can be used for drawing operations.
   * @note Override this function to handle specific update notifications and refresh only the necessary parts of the
   * view. If you do not handle a particular hint, call the base class implementation to ensure default processing
   * occurs.
   */
  void DisplayUsingHint(CView* sender, LPARAM hint, CObject* hintObject, EoGsRenderDevice* renderDevice);

  /** @brief Respond to updates from the document or other views.
   * This method is called by the MFC framework when the document or other views call UpdateAllViews.
   * The hint parameter can be used to determine what type of update occurred and respond accordingly.
   * @param sender The view that sent the update notification, or nullptr if the document sent the notification.
   * @param hint An application-defined value that indicates the type of update. This can be used to optimize the
   * response.
   * @param hintObject An optional pointer to an object that provides additional information about the update. The
   * interpretation of this pointer depends on the value of hint.
   * @note Override this function to handle specific update notifications and refresh the view as needed. If you do not
   * handle a particular hint, call the base class implementation to ensure default processing occurs.
   */
  void OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) override;

 public:
  virtual ~AeSysView();

 protected:
  afx_msg void OnContextMenu(CWnd*, CPoint point);
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg LRESULT OnLayoutTabChange(WPARAM wParam, LPARAM lParam);

  afx_msg void OnModeAnnotate();
  afx_msg void OnModeCut();
  afx_msg void OnModeDimension();
  afx_msg void OnModeDraw();
  afx_msg void OnModeDraw2();
  afx_msg void OnModeEdit();
  afx_msg void OnModeFixup();
  afx_msg void OnModeLPD();
  afx_msg void OnModeNodal();
  afx_msg void OnModePipe();
  afx_msg void OnModePower();
  afx_msg void OnModeTrap();

  afx_msg void OnTrapCommandsAddGroups();

  afx_msg void OnUpdateModeAnnotate(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeCut(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeDimension(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeDraw(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeDraw2(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeEdit(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeFixup(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeLpd(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeNodal(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModePipe(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModePower(CCmdUI* pCmdUI);
  afx_msg void OnUpdateModeTrap(CCmdUI* pCmdUI);

 public:
  /// @brief Rebuilds the layout tab bar from the document's layout list.
  /// Called from OnInitialUpdate and OnUpdate when document content changes.
  void UpdateLayoutTabs();

  /// @brief Returns the layout tab bar for state control updates.
  [[nodiscard]] EoMfLayoutTabBar& LayoutTabBar() noexcept { return m_layoutTabBar; }

  /// @brief Returns the currently activated paper-space viewport, or nullptr if none.
  [[nodiscard]] EoDbViewport* ActiveViewportPrimitive() const noexcept { return m_activeViewportPrimitive; }

  /// @brief Activates a paper-space viewport for model-space editing.
  /// Pass nullptr to deactivate. Invalidates the scene to update the accent border.
  void SetActiveViewportPrimitive(EoDbViewport* viewport);

  /// @brief Returns true if a viewport is currently activated in paper space.
  [[nodiscard]] bool IsViewportActive() const noexcept { return m_activeViewportPrimitive != nullptr; }

  /// @brief Deactivates the current viewport (if any) and invalidates the scene.
  void DeactivateViewport();

  /// @brief Configures the view transform for model-space coordinate mapping through the given viewport.
  /// Pushes the current (paper-space) view transform, then sets camera and off-center projection
  /// so that DoProjectionInverse + ModelViewGetMatrixInverse map device coordinates to model-space.
  /// Caller must call RestoreViewportTransform() when done.
  /// @return true if the viewport transform was successfully configured; false if the viewport is
  ///         invalid or the device clip rect is degenerate (view transform is NOT pushed on failure).
  bool ConfigureViewportTransform(const EoDbViewport* viewport);

  /// @brief Restores the view transform saved by ConfigureViewportTransform().
  void RestoreViewportTransform();

  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg BOOL OnEraseBkgnd(CDC* deviceContext);
  afx_msg void OnKillFocus(CWnd* newWindow);
afx_msg BOOL PreTranslateMessage(MSG* pMsg);
afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint pnt);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint pnt);
  afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnMouseMove(UINT nFlags, CPoint pnt);

  /**
   * @brief Handle mouse wheel events for zooming in and out.
   * This function is called when the mouse wheel is scrolled. It checks the zDelta value to determine the direction of
   * the scroll and calls the appropriate zoom function (zoom in or zoom out).
   * @param flags Indicates whether various virtual keys are down.
   * @param zDelta The distance the wheel is rotated, expressed in multiples of WHEEL_DELTA.
   * @param point The current position of the cursor, in screen coordinates.
   * @return TRUE if the message was processed; otherwise, FALSE.
   */
  afx_msg BOOL OnMouseWheel(UINT flags, std::int16_t zDelta, CPoint point);

  afx_msg void OnPaint();
  afx_msg void OnRButtonDown(UINT nFlags, CPoint pnt);
  afx_msg void OnRButtonUp(UINT nFlags, CPoint pnt);
  afx_msg void OnSetFocus(CWnd* oldWindow);
  afx_msg void OnSize(UINT type, int cx, int cy);

  /// @brief Returns a pointer to the currently active view.
  static AeSysView* GetActiveView();

  void UpdateStateInformation(EStateInformationItem item);

  CPoint m_middleButtonPanStartPoint{};
  bool m_middleButtonPanInProgress{};

  // Hover tooltip — fires kHoverTimerId ms after the last mouse-move with no hit.
  static constexpr UINT_PTR kHoverTimerId{1};
  static constexpr UINT kHoverDelayMs{1500};
  EoMfPrimitiveTooltip m_hoverTooltip;

  ERubs m_rubberbandType{None};
  EoGePoint3d m_rubberbandBegin{};
  CPoint m_rubberbandLogicalBegin{};
  CPoint m_rubberbandLogicalEnd{};

  // True after the first corner of a two-click field-trap rectangle has been placed.
  // m_fieldTrapIsRemove captures the shift state at first-corner time; the second
  // click does not need shift held to complete a removal.
  bool m_fieldTrapAnchorSet{};
  bool m_fieldTrapIsRemove{};

  /** @brief Disables rubber banding by erasing the current rubber band from the view.
   * @note When Direct2D is active, simply clears the rubberband type and invalidates the scene — the next
   * OnDraw omits the rubberband overlay. When GDI is active, erases the rubber band using R2_XORPEN by
   * redrawing it. Sets m_rubberbandType to None in both cases.
   */
  void RubberBandingDisable();
  /** @brief Initializes rubber banding for a given point and type.
   * @param point The starting point for rubber banding in world coordinates.
   * @param type The type of rubber banding (Lines or Rectangles).
   * @note This function transforms the given point from world coordinates to view coordinates.
   * If the transformed point is within the view, it sets the starting point for rubber banding and initializes
   * the logical begin and end points to the projected view coordinates. The rubber banding type is also set to the
   * specified type.
   */
  void RubberBandingStartAtEnable(EoGePoint3d point, ERubs type);

  EoGePoint3d m_ptCursorPosDev{};
  EoGePoint3d m_ptCursorPosWorld{};

  /** @brief Retrieves the current cursor position in world coordinates.
   * @return The current cursor position world coordinates.
   * @note This function gets the current cursor position in device coordinates, converts it to world coordinates using
   * the inverse of the model-view   matrix, and snaps it to the grid if necessary. It also updates the member variables
   * that store the cursor position in both device and world   coordinates.
   */
  [[nodiscard]] EoGePoint3d GetCursorPosition();

  /** @brief Sets the cursor position to the specified world-coordinate point.
   * @param position The point in world coordinates to use for the cursor position.
   */
  void SetCursorPosition(const EoGePoint3d& position);

  void SetModeCursor(int mode);

  /** @brief Selects a circle primitive using a point and tolerance.
   * @param point The point to use for selection.
   * @param tolerance The tolerance distance for selection.
   * @param circle A reference to a pointer that will receive the selected circle primitive.
   * @return A pointer to the group containing the selected circle, or nullptr if no circle was found.
   */
  [[nodiscard]] EoDbGroup* SelectCircleUsingPoint(const EoGePoint3d& point, double tolerance, EoDbConic*& circle);

  /**
   * Select a line primitive using a point.
   * @param point The point to use for selection.
   * @param line  The line primitive that is selected.
   * @return The group containing the selected line primitive, or nullptr if no line is selected.
   */
  [[nodiscard]] EoDbGroup* SelectLineUsingPoint(const EoGePoint3d& point, EoDbLine*& line);

  [[nodiscard]] EoDbGroup* SelSegAndPrimAtCtrlPt(const EoGePoint4d& pt);
  [[nodiscard]] EoDbGroup* SelectLineUsingPoint(const EoGePoint3d& pt);
  [[nodiscard]] EoDbText* SelectTextUsingPoint(const EoGePoint3d& pt);
  [[nodiscard]] EoDbGroup* SelectGroupAndPrimitive(const EoGePoint3d& pt);
  [[nodiscard]] EoGePoint3d& DetPt() noexcept { return m_ptDet; }
  [[nodiscard]] EoDbPrimitive*& EngagedPrimitive() noexcept { return m_EngagedPrimitive; }
  [[nodiscard]] EoDbGroup*& EngagedGroup() noexcept { return m_EngagedGroup; }

  /** @brief Displays a pixel at the specified 3D point with the given color using the provided device context.
   * This function is typically used for drawing temporary graphics such as rubber band lines or selection highlights.
   * The z-coordinate of the point may be ignored depending on the context in which this function is called.
   * @param deviceContext The device context to use for drawing the pixel.
   * @param colorReference The color to set the pixel to, specified as a COLORREF value.
   * @param point The 3D point representing the location of the pixel to set. The z-coordinate may be ignored depending
   * on the context.
   */
  void DisplayPixel(CDC* deviceContext, COLORREF colorReference, const EoGePoint3d& point);

  bool GroupIsEngaged() noexcept { return m_EngagedGroup != nullptr; }
  [[nodiscard]] double& SelectApertureSize() noexcept { return m_SelectApertureSize; }
  void BreakAllPolylines();

  /** @brief Explodes all block references in the view by iterating through each visible group and calling the
   * ExplodeBlockReferences method on it.
   * @note This method modifies the groups in place, so any block references within the groups will be exploded into
   * their constituent primitives. After calling this method, the view will need to be refreshed to reflect the changes.
   */
  void ExplodeAllBlockReferences();

  /// @brief Marks the off-screen scene buffer as invalid, triggering a full re-render on the next paint.
  void InvalidateScene();

  /// @brief Marks the overlay (preview group) as dirty, triggering a repaint without re-rendering the scene.
  void InvalidateOverlay();

  bool PenWidthsOn() const noexcept { return m_ViewPenWidths; }
  [[nodiscard]] double GetWorldScale() const noexcept { return m_WorldScale; }
  void SetWorldScale(double scale);
  bool RenderAsWireframe() const noexcept { return m_ViewWireframe; }
  auto AddGroup(EoDbGroup* group) { return m_VisibleGroupList.AddTail(group); }
  void AddGroups(EoDbGroupList* groups) { return m_VisibleGroupList.AddTail(groups); }
  auto RemoveGroup(EoDbGroup* group) { return m_VisibleGroupList.Remove(group); }
  void RemoveAllGroups() { m_VisibleGroupList.RemoveAll(); }
  void ResetView();

 private:
  /// @brief Recreates the off-screen back buffer to match the given dimensions.
  void RecreateBackBuffer(int width, int height);

  /// @brief Creates (or recreates) the Direct2D HWND render target for this view.
  void CreateD2DRenderTarget();

  /// @brief Creates the cached scene bitmap render target sized to the current viewport.
  void CreateD2DSceneTarget();

  /// @brief Releases the Direct2D render target and all device-dependent resources.
  void DiscardD2DResources();

 public:
  /// @brief Deletes last group detectable in the this view.
  void DeleteLastGroup();
  auto GetFirstVisibleGroupPosition() const { return m_VisibleGroupList.GetHeadPosition(); }
  auto GetLastGroupPosition() const { return m_VisibleGroupList.GetTailPosition(); }
  [[nodiscard]] EoDbGroup* GetNextVisibleGroup(POSITION& position) { return m_VisibleGroupList.GetNext(position); }
  [[nodiscard]] EoDbGroup* GetPreviousGroup(POSITION& position) { return m_VisibleGroupList.GetPrev(position); }

  /** @brief This method handles the rendering of the background image in the view.
   *
   * It calculates the appropriate portion of the
   * background image to display based on the current view and overview transforms, and then stretches that portion to
   * fit the viewport. It uses GDI functions to perform the drawing, ensuring that the palette is correctly selected and
   * realized for accurate color representation.
   * @param deviceContext The device context to draw on, typically obtained from the view's OnDraw method.
   */
  void BackgroundImageDisplay(CDC* deviceContext);

  [[nodiscard]] EoGeVector3d GetRelPos() const noexcept { return m_vRelPos; }
  [[nodiscard]] bool ViewTrueTypeFonts() const noexcept { return m_ViewTrueTypeFonts; }

  /** @brief Displays the odometer information showing the relative position from the grid origin to the current cursor
   * position, and optionally the line length and angle if in rubber band line mode.
   */
  void DisplayOdometer();

  void DoCameraRotate(int iDir);
  void DoWindowPan(double ratio);

  void CopyActiveModelViewToPreviousModelView();
  EoGsViewTransform PreviousModelView();
  void ExchangeActiveAndPreviousModelViews();
  void PushModelTransform();
  void SetLocalModelTransform(const EoGeTransformMatrix& transformation);
  void PopModelTransform();

  void ModelTransformPoint(EoGePoint4d& point) noexcept;
  void ModelTransformPoint(EoGePoint3d& point) noexcept;

  void ModelTransformVector(EoGeVector3d vector) noexcept;
  void ModelViewAdjustWindow(double& uMin, double& vMin, double& uMax, double& vMax, double ratio);
  void ModelViewGetViewport(EoGsViewport& viewport);
  [[nodiscard]] EoGeVector3d CameraDirection() const;
  [[nodiscard]] EoGeTransformMatrix& ModelViewGetMatrix() noexcept;
  [[nodiscard]] EoGeTransformMatrix& ModelViewGetMatrixInverse() noexcept;
  [[nodiscard]] EoGePoint3d CameraTarget() const noexcept;
  [[nodiscard]] double UExtent() const noexcept;
  [[nodiscard]] double UMax() const noexcept;
  [[nodiscard]] double UMin() const noexcept;
  [[nodiscard]] double VExtent() const noexcept;
  [[nodiscard]] double VMax() const noexcept;
  [[nodiscard]] double VMin() const noexcept;
  [[nodiscard]] EoGeVector3d ViewUp() const noexcept;
  void ModelViewInitialize();

  void PopViewTransform();
  void PushViewTransform();
  void ModelViewTransformPoint(EoGePoint4d& ndcPoint);
  void ModelViewTransformPoints(EoGePoint4dArray& pointsArray);
  void ModelViewTransformPoints(int numberOfPoints, EoGePoint4d* points);
  void ModelViewTransformVector(EoGeVector3d& vector);
  void SetViewTransform(const EoGsViewTransform& viewTransform);
  void SetCenteredWindow(double uExtent, double vExtent);
  void SetCameraPosition(const EoGeVector3d& direction);
  void SetCameraTarget(const EoGePoint3d& target);
  void SetCameraViewUp(const EoGeVector3d& viewUp);
  void SetViewWindow(double uMin, double vMin, double uMax, double vMax);

  /// @brief Determines the number of pages for 1 to 1 print
  UINT NumPages(const CDC* deviceContext, double dScaleFactor, UINT& nHorzPages, UINT& nVertPages);

  [[nodiscard]] double OverviewUExt() const noexcept { return m_OverviewViewTransform.UExtent(); }
  [[nodiscard]] double OverviewUMin() const noexcept { return m_OverviewViewTransform.UMin(); }
  [[nodiscard]] double OverviewVExt() const noexcept { return m_OverviewViewTransform.VExtent(); }
  [[nodiscard]] double OverviewVMin() const noexcept { return m_OverviewViewTransform.VMin(); }

  [[nodiscard]] CPoint ProjectToClient(const EoGePoint4d& ndcPoint) { return m_Viewport.ProjectToClient(ndcPoint); }

  void ProjectToClient(CPoint* clientPoints, int numberOfPoints, const EoGePoint4d* ndcPoints) {
    m_Viewport.ProjectToClient(clientPoints, numberOfPoints, ndcPoints);
  }

  void ProjectToClient(CPoint* clientPoints, EoGePoint4dArray& ndcPoints) {
    m_Viewport.ProjectToClient(clientPoints, ndcPoints);
  }

  void DoProjectionInverse(EoGePoint3d& pt) { m_Viewport.DoProjectionInverse(pt); }

  [[nodiscard]] double HeightInInches() { return m_Viewport.HeightInInches(); }
  [[nodiscard]] double WidthInInches() { return m_Viewport.WidthInInches(); }
  void ViewportPopActive();
  void ViewportPushActive();
  void SetViewportSize(const int width, const int height) { m_Viewport.SetSize(width, height); }
  void SetDeviceHeightInInches(double height) { m_Viewport.SetDeviceHeightInInches(height); }
  void SetDeviceWidthInInches(double width) { m_Viewport.SetDeviceWidthInInches(width); }

 public:  // Group and Primitive operations
  void InitializeGroupAndPrimitiveEdit();
  void DoEditGroupCopy();
  void DoEditGroupEscape();
  void DoEditGroupTransform(std::uint16_t operation);
  void DoEditPrimitiveTransform(std::uint16_t operation);
  void DoEditPrimitiveCopy();
  void DoEditPrimitiveEscape();
  void PreviewPrimitiveEdit();
  void PreviewGroupEdit();

  void MendStateReturn();
  void MendStateEscape();

  /// Annotate Mode Interface ///////////////////////////////////////////////////
 private:  // Annotate and Dimension interface
  double m_GapSpaceFactor{0.5};  // Edge space factor 25 percent of character height
  double m_CircleRadius{0.03125};
  int m_EndItemType{1};
  double m_EndItemSize{0.1};
  double m_BubbleRadius{0.125};
  int m_NumberOfSides{};  // Number of sides on bubble (0 indicating circle)
  CString m_DefaultText{};

 public:
  [[nodiscard]] double BubbleRadius() const noexcept { return m_BubbleRadius; }
  void SetBubbleRadius(double radius) noexcept { m_BubbleRadius = radius; }
  [[nodiscard]] double CircleRadius() const noexcept { return m_CircleRadius; }
  void SetCircleRadius(double radius) noexcept { m_CircleRadius = radius; }
  [[nodiscard]] CString DefaultText() const { return m_DefaultText; }
  void SetDefaultText(const CString& text) { m_DefaultText = text; }
  [[nodiscard]] double EndItemSize() const noexcept { return m_EndItemSize; }
  void SetEndItemSize(double size) noexcept { m_EndItemSize = size; }
  [[nodiscard]] int EndItemType() const noexcept { return m_EndItemType; }
  void SetEndItemType(int type) noexcept { m_EndItemType = type; }
  [[nodiscard]] double GapSpaceFactor() const noexcept { return m_GapSpaceFactor; }
  void SetGapSpaceFactor(double factor) noexcept { m_GapSpaceFactor = factor; }
  [[nodiscard]] int NumberOfSides() const noexcept { return m_NumberOfSides; }
  void SetNumberOfSides(int number) noexcept { m_NumberOfSides = number; }

 public:  // Annotate mode interface
  void DoAnnotateModeMouseMove();

  afx_msg void OnAnnotateModeOptions();
  afx_msg void OnAnnotateModeLine();
  afx_msg void OnAnnotateModeArrow();
  afx_msg void OnAnnotateModeBubble();
  afx_msg void OnAnnotateModeHook();
  afx_msg void OnAnnotateModeUnderline();
  afx_msg void OnAnnotateModeBox();
  afx_msg void OnAnnotateModeCutIn();
  afx_msg void OnAnnotateModeConstructionLine();
  afx_msg void OnAnnotateModeReturn();
  afx_msg void OnAnnotateModeEscape();

  /// @brief Generates arrow heads for annotation mode.
  /// type type of end item
  /// size size of end item
  /// beginPoint tail of line segment defining arrow head
  /// endPoint head of line segment defining arrow head
  /// group group where primitives are placed
  void GenerateLineEndItem(int type,
      double size,
      const EoGePoint3d& beginPoint,
      const EoGePoint3d& endPoint,
      EoDbGroup* group) const;

  bool CorrectLeaderEndpoints(int beginType, int endType, EoGePoint3d& beginPoint, EoGePoint3d& endPoint) const;

  /// Draw Mode Interface ///////////////////////////////////////////////////////
 public:
  afx_msg void OnInsertBlock();
  afx_msg void OnInsertTracing();

afx_msg void OnDrawCommand(UINT nID);
afx_msg void OnDrawModeOptions();
  afx_msg void OnDrawModePoint();
  afx_msg void OnDrawModeLine();
  afx_msg void OnDrawModePolygon();
  afx_msg void OnDrawModeQuad();
  afx_msg void OnDrawModeArc();
  afx_msg void OnDrawModeBspline();
  afx_msg void OnDrawModeCircle();
  afx_msg void OnDrawModeEllipse();
  afx_msg void OnDrawModeInsert();
  afx_msg void OnDrawModeReturn();
  afx_msg void OnDrawModeEscape();
  afx_msg void OnDrawModeShiftReturn();
  afx_msg void OnDrawModeUndoLast();
  afx_msg void OnDrawModeFinish();
  afx_msg void OnDrawModeFinishPolyline();

  /// Draw Mode2 Interface //////////////////////////////////////////////////////

 private:
  double m_centerLineEccentricity{0.5};  // Center line eccentricity for parallel lines
  bool m_continuingCorner{};
  double m_distanceBetweenLines{0.0625};
  EoGeLine m_currentLeftLine{};
  EoGeLine m_currentRightLine{};
  EoGeLine m_previousReferenceLine{};  // Previous line used as reference for parallel line and corner construction
  EoGeLine m_currentReferenceLine{};  // Current line used as reference for parallel line and corner construction
  EoDbGroup* m_assemblyGroup{};
  EoDbGroup* m_endSectionGroup{};
  EoDbGroup* m_beginSectionGroup{};
  EoDbLine* m_beginSectionLinePrimitive{};
  EoDbLine* m_endSectionLinePrimitive{};

 public:
  void DoDraw2ModeMouseMove();

  [[nodiscard]] double DistanceBetweenLines() const noexcept { return m_distanceBetweenLines; }
  [[nodiscard]] double CenterLineEccentricity() const noexcept { return m_centerLineEccentricity; }

  afx_msg void OnDraw2ModeOptions();
  /// @brief Searches for an existing wall side or endcap
  afx_msg void OnDraw2ModeJoin();
  afx_msg void OnDraw2ModeWall();
  afx_msg void OnDraw2ModeReturn();
  afx_msg void OnDraw2ModeEscape();

  bool CleanPreviousLines();
  bool StartAssemblyFromLine();

  public:
   afx_msg void OnDimensionModeOptions();
  afx_msg void OnDimensionModeArrow();
  afx_msg void OnDimensionModeLine();
  afx_msg void OnDimensionModeDLine();
  afx_msg void OnDimensionModeDLine2();
  afx_msg void OnDimensionModeExten();
  afx_msg void OnDimensionModeRadius();
  afx_msg void OnDimensionModeDiameter();
  afx_msg void OnDimensionModeAngle();
  afx_msg void OnDimensionModeConvert();
  afx_msg void OnDimensionModeReturn();
  afx_msg void OnDimensionModeEscape();

  FixupConfig m_fixupConfig;

  afx_msg void OnFixupModeOptions();
  afx_msg void OnFixupModeReference();
  afx_msg void OnFixupModeMend();
  afx_msg void OnFixupModeChamfer();
  afx_msg void OnFixupModeFillet();
  afx_msg void OnFixupModeSquare();
  afx_msg void OnFixupModeParallel();
  afx_msg void OnFixupModeReturn();
  afx_msg void OnFixupModeEscape();

 public:  // Nodal mode interface
  void DoNodalModeMouseMove();

  afx_msg void OnNodalModeAddRemove();
  afx_msg void OnNodalModePoint();
  afx_msg void OnNodalModeLine();
  afx_msg void OnNodalModeArea();
  /// @brief Translate all control points identified
  afx_msg void OnNodalModeMove();
  afx_msg void OnNodalModeCopy();
  afx_msg void OnNodalModeToLine();
  afx_msg void OnNodalModeToPolygon();
  afx_msg void OnNodalModeEmpty();
  /// @brief Handles the engagement of nodal mode by updating the nodal list with all points from the currently engaged
  /// primitive.
  afx_msg void OnNodalModeEngage();
  afx_msg void OnNodalModeReturn();
  afx_msg void OnNodalModeEscape();

  void ConstructPreviewGroup();
  void ConstructPreviewGroupForNodalGroups();

 public:  // Cut mode interface
  afx_msg void OnCutModeOptions();
  /// @brief Cuts a primative at cursor position.
  afx_msg void OnCutModeTorch();
  /// @brief Cuts all primatives which intersect with line defined by two points.
  // Notes: Colinear fill area edges are not considered to intersect.
  afx_msg void OnCutModeSlice();
  afx_msg void OnCutModeField();
  /// @brief Cuts a primative at two pnts and puts non-null middle piece in trap.
  // Notes:	Accuracy of arc section cuts diminishes with high
  //			eccentricities. if two cut points are coincident
  //			nothing happens.
  afx_msg void OnCutModeClip();
  afx_msg void OnCutModeDivide();
  afx_msg void OnCutModeReturn();
  afx_msg void OnCutModeEscape();

 public:  // Edit mode interface
  EditConfig m_editConfig;

  [[nodiscard]] EoGeVector3d EditModeRotationAngles() const noexcept { return m_editConfig.rotationAngles; }
  EoGeTransformMatrix EditModeInvertedRotationTMat() const {
    EoGeTransformMatrix matrix;
    matrix = matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
    matrix.Inverse();
    return matrix;
  }
  EoGeTransformMatrix EditModeRotationTMat() const {
    EoGeTransformMatrix matrix;
    matrix = matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
    return matrix;
  }
  [[nodiscard]] EoGeVector3d EditModeInvertedScaleFactors() const noexcept {
    EoGeVector3d invertedScaleFactors;
    invertedScaleFactors.x = Eo::IsGeometricallyNonZero(m_editConfig.scale.x) ? 1.0 / m_editConfig.scale.x : 1.0;
    invertedScaleFactors.y = Eo::IsGeometricallyNonZero(m_editConfig.scale.y) ? 1.0 / m_editConfig.scale.y : 1.0;
    invertedScaleFactors.z = Eo::IsGeometricallyNonZero(m_editConfig.scale.z) ? 1.0 / m_editConfig.scale.z : 1.0;
    return invertedScaleFactors;
  }

  [[nodiscard]] EoGeVector3d EditModeScaleFactors() const noexcept { return m_editConfig.scale; }
  void SetEditModeScaleFactors(double x, double y, double z) noexcept { m_editConfig.scale.Set(x, y, z); }
  void SetEditModeRotationAngles(double x, double y, double z) noexcept { m_editConfig.rotationAngles.Set(x, y, z); }
  [[nodiscard]] EoGeVector3d EditModeMirrorScale() const noexcept { return m_editConfig.mirrorScale; }
  void SetMirrorScale(double x, double y, double z) noexcept { m_editConfig.mirrorScale.Set(x, y, z); }

  afx_msg void OnEditModeOptions();
  afx_msg void OnEditModePivot();
  afx_msg void OnEditModeRotccw();
  afx_msg void OnEditModeRotcw();
  afx_msg void OnEditModeMove();
  afx_msg void OnEditModeCopy();
  afx_msg void OnEditModeFlip();
  afx_msg void OnEditModeReduce();
  afx_msg void OnEditModeEnlarge();
  afx_msg void OnEditModeReturn();
  afx_msg void OnEditModeEscape();

  afx_msg void OnTrapModeRemoveAdd();
  afx_msg void OnTrapModePoint();
  /// @brief Identifies groups which intersect with a line and adds them to the trap.
  afx_msg void OnTrapModeStitch();
  /// @brief Identifies groups which lie wholly or partially within a rectangular area.
  /// <remarks>Needs to be generalized to quad.</remarks>
  afx_msg void OnTrapModeField();
  /// @brief Adds last detectable group which is not already in trap to trap
  afx_msg void OnTrapModeLast();
  afx_msg void OnTrapModeEngage();
  afx_msg void OnTrapModeMenu();
  afx_msg void OnTrapModeModify();
  afx_msg void OnTrapModeEscape();

  afx_msg void OnTraprModeRemoveAdd();
  afx_msg void OnTraprModePoint();
  /// @brief Identifies groups which intersect with a line and removes them from the trap.
  afx_msg void OnTraprModeStitch();
  /// @brief Identifies groups which lie wholly or partially within a orthogonal rectangular area.
  // Notes: This routine fails in all but top view. !!
  // Parameters:	pt1 	one corner of the area
  //				pt2 	other corner of the area
  afx_msg void OnTraprModeField();
  afx_msg void OnTraprModeLast();
  afx_msg void OnTraprModeEngage();
  afx_msg void OnTraprModeMenu();
  afx_msg void OnTraprModeModify();
  afx_msg void OnTraprModeEscape();

 private:  // Low Pressure Duct (rectangular) interface
  LpdConfig m_lpdConfig;
  bool m_ContinueSection{};
  int m_EndCapLocation{0};
  EoDbPoint* m_EndCapPoint{};
  EoDbGroup* m_EndCapGroup{};
  bool m_OriginalPreviousGroupDisplayed{true};
  EoDbGroup* m_OriginalPreviousGroup{};

  Section m_PreviousSection{0.125, 0.0625, Section::Rectangular};
  Section m_CurrentSection{0.125, 0.0625, Section::Rectangular};

 public:
  void DoDuctModeMouseMove();

  [[nodiscard]] double DuctSeamSize() const noexcept { return m_lpdConfig.ductSeamSize; }
  [[nodiscard]] bool BeginWithTransition() const noexcept { return m_lpdConfig.beginWithTransition; }
  [[nodiscard]] EJust DuctJustification() const noexcept { return m_lpdConfig.justification; }
  [[nodiscard]] double TransitionSlope() const noexcept { return m_lpdConfig.transitionSlope; }
  [[nodiscard]] bool GenerateTurningVanes() const noexcept { return m_lpdConfig.generateTurningVanes; }
  [[nodiscard]] double PipeTicSize() const noexcept { return m_pipeConfig.ticSize; }
  [[nodiscard]] double PipeRiseDropRadius() const noexcept { return m_pipeConfig.riseDropRadius; }

  afx_msg void OnLpdModeOptions();
  afx_msg void OnLpdModeJoin();
  afx_msg void OnLpdModeDuct();
  afx_msg void OnLpdModeTransition();
  afx_msg void OnLpdModeTap();
  afx_msg void OnLpdModeEll();
  afx_msg void OnLpdModeTee();
  afx_msg void OnLpdModeUpDown();
  afx_msg void OnLpdModeSize();
  afx_msg void OnLpdModeReturn();
  afx_msg void OnLpdModeEscape();

  /**
   * Selects a point primitive using a given point, tolerance, color, and style.
   * @param cursorPosition The position to check for selection.
   * @param tolerance The maximum distance from the cursor position to consider a point selected.
   * @param color The color of the point primitive to match.
   * @param pointStyle The style of the point primitive to match.
   * @param endCapPoint A reference to store the selected point primitive if found.
   * @return The group containing the selected point primitive, or nullptr if none is found.
   */
  [[nodiscard]] EoDbGroup* SelectPointUsingPoint(const EoGePoint3d& point,
      double tolerance,
      std::int16_t color,
      std::int16_t pointStyle,
      EoDbPoint*& endCapPoint);

  /** Locates and returns the first two lines that have an endpoint which coincides with
   * the endpoints of the specified line.
   * @param testLinePrimitive The test line primitive to check against.
   * @param angularTolerance The angle tolerance for qualifying lines (in radians).
   * @param leftLine The endpoints of the left line.
   * @param rightLine The endpoints of the right line.
   * @return true if qualifying lines located, else false.
   * @note The lines are oriented so direction vectors defined by each point to the specified line.
   * No check is made to see if lines are colinear.
   * Lines are normal to to test line (and therefore parallel to each other) if acceptance angle is 0.
   */
  bool Find2LinesUsingLineEndpoints(const EoDbLine* testLinePrimitive,
      double angularTolerance,
      EoGeLine& leftLine,
      EoGeLine& rightLine);
  /// @brief Generates an end-cap.
  /// <remarks>
  /// End-caps are groups containing a line and a point.  The line defines the orientation of the end-cap.
  /// The point contains information about the cross-section (width, depth)
  /// and optionally a number which might be used for something like cfm.
  /// </remarks>
  /// beginPoint begin point of the line
  /// endPoint end point of the line
  /// section width and depth data
  /// group
  void GenerateEndCap(const EoGePoint3d& beginPoint, const EoGePoint3d& endPoint, Section section, EoDbGroup* group);

  /** @brief Generates the rise/drop lines and the associated section for a vertical section.
   * @param indicatorLineTypeName is the line type name for the rise/drop lines which are generated as a separate
   * group to allow for different line types.
   * @param section section for which rise/drop is being generated (used for width and depth)
   * @param referenceLine centerline of the rise/drop. Rise/drop indicator lines will be generated perpendicular to this
   * line. This line will be shortened by m_DuctSeamSize at the end if it is long enough to accommodate the seam.
   * @param group group to which primitives will be added
   * @note need to allow continuation perpendicular to vertical section ?
   *      need to add end-cap if rise/drop line is long enough ?
   */
  void GenerateRiseDrop(const std::wstring& indicatorLineTypeName,
      Section section,
      EoGeLine& referenceLine,
      EoDbGroup* group);
  /// @brief Generates rectangular section using a set of parallel lines.
  /// section width and depth of section
  /// group
  void GenerateRectangularSection(const EoGeLine& referenceLine, double eccentricity, Section section, EoDbGroup* group);
  /// @brief Generates text segment representing width and depth of a piece of duct.
  void GenSizeNote(EoGePoint3d, double angle, Section section);
  /// previousReferenceLine
  /// previousSection
  /// currentReferenceLine on exit the begin point is the same as the point on the endcap
  /// currentSection
  /// group
  void GenerateRectangularElbow(EoGeLine& previousReferenceLine,
      Section previousSection,
      EoGeLine& currentReferenceLine,
      Section currentSection,
      EoDbGroup* group);
  /// @brief Generates rectangular tap fitting.
  /// justification
  /// section
  bool GenerateRectangularTap(EJust justification, Section section);

  /** @brief Generates a mitered bullhead tee fitting. (placeholder)
   *  @param existingGroup group containing rectangular section to join
   *  @param existingSectionReferenceLine
   *  @param existingSectionWidth
   *  @param existingSectionDepth
   *  @param group
   *  @return Center point of end cap of exit transition.
   *  @note Requires current operation to be a regular rectangular section. The selection based on the current cursor
   * location identifies the second section, and the direction from the point to the cursor location defines the
   * direction for the two elbow turns.
   *  @note Placeholder until implementation is return of (0.0, 0.0, 0.0)
   */
  [[nodiscard]] EoGePoint3d GenerateBullheadTee(const EoDbGroup* existingGroup,
      const EoGeLine& existingSectionReferenceLine,
      double existingSectionWidth,
      double existingSectionDepth,
      const EoDbGroup* group) noexcept;

  /** @brief Generates a full elbow takeoff from an existing section to the current section.
   * @param existingSectionReferenceLine Reference line of the existing section.
   * @param existingSection Cross sectional data of the existing section.
   * @param group Group to add the generated primitives to.
   */
  void GenerateFullElbowTakeoff(EoDbGroup* existingGroup,
      EoGeLine& existingSectionReferenceLine,
      Section existingSection,
      EoDbGroup* group);

  /// @brief Generates section which transitions from one rectangle to another
  /// @param referenceLine line defining the begin point and direction of the transition
  /// @param eccentricity
  /// @param justification
  /// @param slope slope of the transition
  /// @param previousSection width and depth at begin of the transition
  /// @param currentSection width and depth at end of the transition
  /// group group receiving the primitives
  void GenerateTransition(const EoGeLine& referenceLine,
      double eccentricity,
      EJust justification,
      double slope,
      Section previousSection,
      Section currentSection,
      EoDbGroup* group);
  /// @brief Sets the width and depth of ductwork.
  void SetDuctOptions(Section& section);
  /// @brief Determines the total length required to transition duct from one size to another
  /// @param justification justification: 0 centered, %gt 0 taper to right, %lt 0 taper to left
  /// @param slope slope of the section sides
  /// @param previousSection width and depth of begin section
  /// @param currentSection width and depth of end section
  /// @return length of the transition
  [[nodiscard]] double LengthOfTransition(EJust justification,
      double slope,
      Section previousSection,
      Section currentSection);

 private:  // Pipe mode interface
  PipeConfig m_pipeConfig;

  /** @brief Generates a tick mark at a specified distance along a line defined by two points, and adds it to the given
   * group.
   * @param begin The starting point of the line segment.
   * @param end The ending point of the line segment.
   * @param distance The distance from the beginPoint along the line segment where the tick mark should be placed.
   * @param group The group to which the generated tick mark will be added.
   * @return true if the tick mark was successfully generated and added to the group; false otherwise (e.g., if the
   * distance is greater than the length of the line segment).
   */
  bool GenerateTickMark(const EoGePoint3d& begin, const EoGePoint3d& end, double distance, EoDbGroup* group) const;
  void DropFromOrRiseIntoHorizontalSection(const EoGePoint3d& point, EoDbGroup* group, EoDbLine* section) const;
  void DropIntoOrRiseFromHorizontalSection(const EoGePoint3d& point, EoDbGroup* group, EoDbLine* section);

 public:
  /// @brief Adds a fitting indication to horizontal pipe section as required by previous fitting type.
  void GenerateLineWithFittings(int beginType,
      const EoGePoint3d& beginPoint,
      int endType,
      const EoGePoint3d& endPoint,
      EoDbGroup* group);

  void DoPipeModeMouseMove();

  afx_msg void OnPipeModeOptions();
  afx_msg void OnPipeModeLine();
  afx_msg void OnPipeModeFitting();
  afx_msg void OnPipeModeRise();
  afx_msg void OnPipeModeDrop();
  /// @brief Generates a piping symbol at point specified if pipe section located.
  afx_msg void OnPipeModeSymbol();
  afx_msg void OnPipeModeWye();
  afx_msg void OnPipeModeReturn();
  afx_msg void OnPipeModeEscape();

 private:  // Power mode interface
  PowerConfig m_powerConfig;

 public:
  void DoPowerModeMouseMove();

  afx_msg void OnPowerModeOptions();
  afx_msg void OnPowerModeCircuit();
  afx_msg void OnPowerModeGround();
  afx_msg void OnPowerModeHot();
  afx_msg void OnPowerModeSwitch();
  afx_msg void OnPowerModeNeutral();
  afx_msg void OnPowerModeHome();
  afx_msg void OnPowerModeReturn();
  afx_msg void OnPowerModeEscape();

  void GeneratePowerConductorSymbol(std::uint16_t conductorType,
      const EoGePoint3d& pointOnCircuit,
      const EoGePoint3d& endPoint) const;
  void GenerateHomeRunArrow(EoGePoint3d& pointOnCircuit, const EoGePoint3d& endPoint) const;
  void DoPowerModeConductor(std::uint16_t conductorType);

 public:
  /// @brief Updates the status bar mode line display with mode-specific operation information and optionally draws the
  /// pane text in the active view.
  void ModeLineDisplay();

  /// @brief Highlights a mode line operation by setting its text color to red in the status bar and optionally in the
  /// active view.
  /// @param command The operation command identifier to highlight. A value of 0 indicates no operation should be
  /// highlighted.
  /// @return The command identifier that was highlighted, or 0 if no operation was highlighted.
  [[nodiscard]] std::uint16_t ModeLineHighlightOp(std::uint16_t command);

  /// @brief Removes highlighting from a mode line operation pane and updates the display.
  /// @param command Reference to the command identifier to unhighlight. Set to 0 after unhighlighting is complete.
  void ModeLineUnhighlightOp(std::uint16_t& command);

  /// @brief Gets a reference to the application's status bar.
  /// @return A reference to the CMFCStatusBar object from the main frame window.
  CMFCStatusBar& GetStatusBar() const;

 public:
  afx_msg void OnBackgroundImageLoad();
  afx_msg void OnBackgroundImageRemove();
  afx_msg void OnFilePlot();
  afx_msg void OnFilePlotHalf();
  afx_msg void OnFilePlotFull();
  afx_msg void OnFilePlotQuarter();
  afx_msg void OnFilePrint();
  afx_msg void On3dViewsBack();
  afx_msg void On3dViewsBottom();
  afx_msg void On3dViewsFront();

  /** @brief Sets the view to an isometric view based on user-selected left/right, front/back, and above/under
   * options. This function displays a dialog for the user to select the isometric view orientation, calculates the
   * corresponding direction vector, and updates the view transform accordingly.
   */
  afx_msg void On3dViewsIsometric();
  afx_msg void On3dViewsLeft();
  afx_msg void On3dViewsRight();
  afx_msg void On3dViewsTop();
  afx_msg void OnRelativeMovesEngDown();
  afx_msg void OnRelativeMovesEngDownRotate();
  afx_msg void OnRelativeMovesEngIn();
  afx_msg void OnRelativeMovesEngOut();
  afx_msg void OnRelativeMovesEngLeft();
  afx_msg void OnRelativeMovesEngLeftRotate();
  afx_msg void OnRelativeMovesEngRight();
  afx_msg void OnRelativeMovesEngRightRotate();
  afx_msg void OnRelativeMovesEngUp();
  afx_msg void OnRelativeMovesEngUpRotate();
  afx_msg void OnRelativeMovesRight();
  afx_msg void OnRelativeMovesUp();
  afx_msg void OnRelativeMovesLeft();
  afx_msg void OnRelativeMovesDown();
  afx_msg void OnRelativeMovesIn();
  afx_msg void OnRelativeMovesOut();
  afx_msg void OnRelativeMovesRightRotate();
  afx_msg void OnRelativeMovesUpRotate();
  afx_msg void OnRelativeMovesLeftRotate();
  afx_msg void OnRelativeMovesDownRotate();
  afx_msg void OnSetupScale();
  afx_msg void OnSheetSetupFormFactor();
  afx_msg void OnToolsPrimitiveSnapto();
  afx_msg void OnUpdateViewTrueTypeFonts(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewBackgroundImage(CCmdUI* pCmdUI);
  afx_msg void OnUpdateBackgroundimageLoad(CCmdUI* pCmdUI);
  afx_msg void OnUpdateBackgroundimageRemove(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewPenwidths(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewRendered(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewWireframe(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewDirect2D(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewAliased(CCmdUI* pCmdUI);
  afx_msg void OnViewBackgroundImage();
  afx_msg void OnViewTrueTypeFonts();
  afx_msg void OnViewPenWidths();
  afx_msg void OnViewRefresh();
  afx_msg void OnViewParameters();
  afx_msg void OnViewLighting();
  afx_msg void OnViewSolid();
  afx_msg void OnViewRendered();
  afx_msg void OnViewWindowKeyplan();
  afx_msg void OnViewWireframe();
  afx_msg void OnViewDirect2D();
  afx_msg void OnViewAliased();
  afx_msg void OnViewBackgroundToggle();
  afx_msg void OnUpdateViewBackgroundToggle(CCmdUI* pCmdUI);
  afx_msg void OnWindowZoomSpecial();
  afx_msg void OnWindowNormal();
  afx_msg void OnWindowBest();
  afx_msg void OnWindowLast();
  afx_msg void OnWindowSheet();
  afx_msg void OnWindowZoomIn();
  afx_msg void OnWindowZoomOut();
  afx_msg void OnWindowPan();
  afx_msg void OnWindowPanLeft();
  afx_msg void OnWindowPanRight();
  afx_msg void OnWindowPanUp();
  afx_msg void OnWindowPanDown();
  afx_msg void OnCameraRotateLeft();
  afx_msg void OnCameraRotateRight();
  afx_msg void OnCameraRotateUp();
  afx_msg void OnCameraRotateDown();
  afx_msg void OnViewWindow();
  afx_msg void OnSetupDimLength();
  afx_msg void OnSetupDimAngle();
  afx_msg void OnSetupUnits();
  afx_msg void OnSetupConstraints();
  afx_msg void OnModePrimitiveEdit();
  afx_msg void OnModeGroupEdit();
  afx_msg void OnModePrimitiveMend();
  afx_msg void OnPrimPerpJump();
  afx_msg void OnHelpKey();
  afx_msg void OnOp0();
  afx_msg void OnOp2();
  afx_msg void OnOp3();
  afx_msg void OnOp4();
  afx_msg void OnOp5();
  afx_msg void OnOp6();
  afx_msg void OnOp7();
  afx_msg void OnOp8();
  afx_msg void OnReturn();
  afx_msg void OnEscape();

 protected:
  DECLARE_MESSAGE_MAP()
};
