#pragma once

#include <cmath>

#include "Eo.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbPrimitive.h"
#include "EoGeLine.h"
#include "EoGeMatrix.h"

#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeTransformMatrix.h"
#include "EoGeVector3d.h"
#include "EoGsModelTransform.h"
#include "EoGsViewTransform.h"
#include "EoGsViewport.h"
#include "Section.h"

class AeSysDoc;
class EoDbBlock;
class EoDbConic;
class EoDbLine;
class EoDbPoint;
class EoDbText;
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
    All = BothCounts | Pen | Line | TextHeight | WndRatio | Scale | DimLen | DimAng
  };
  enum ERubs { None, Lines, Rectangles };

 private:
  static const double m_MaximumWindowRatio;
  static const double m_MinimumWindowRatio;

  EoGsModelTransform m_ModelTransform;
  EoGsViewport m_Viewport;
  EoGsViewTransform m_ViewTransform;

  CBitmap m_backgroundImageBitmap;
  CPalette m_backgroundImagePalette;
  EoDbPrimitive* m_EngagedPrimitive;
  EoDbGroup* m_EngagedGroup;
  EoUInt16 m_OpHighlighted;
  EoGsViewTransform m_OverviewViewTransform;
  bool m_Plot;
  double m_PlotScaleFactor;
  EoDbGroup m_PreviewGroup;
  EoGsViewTransform m_PreviousViewTransform;
  EoUInt16 m_PreviousOp;
  EoGePoint3d m_PreviousPnt;
  double m_SelectApertureSize;
  bool m_viewBackgroundImage;
  bool m_ViewOdometer;
  bool m_ViewPenWidths;
  CViewports m_Viewports;
  bool m_ViewRendered;
  EoGsViewTransforms m_ViewTransforms;
  bool m_ViewTrueTypeFonts;
  bool m_ViewWireframe;
  EoDbGroupList m_VisibleGroupList;
  double m_WorldScale;

  EoGePoint3d m_ptDet;

  EoGeVector3d m_vRelPos;

  EoGePoint3dArray pts;

 private:  // grid and axis constraints
  EoGePoint3d m_GridOrigin;
  int m_MaximumDotsPerLine;

  double m_XGridLineSpacing;
  double m_YGridLineSpacing;
  double m_ZGridLineSpacing;

  double m_XGridSnapSpacing;
  double m_YGridSnapSpacing;
  double m_ZGridSnapSpacing;

  double m_XGridPointSpacing;
  double m_YGridPointSpacing;
  double m_ZGridPointSpacing;

  double m_AxisConstraintInfluenceAngle;
  double m_AxisConstraintOffsetAngle;
  bool m_DisplayGridWithLines;
  bool m_DisplayGridWithPoints;
  bool m_GridSnap;

 public:
  double AxisConstraintInfluenceAngle() const;
  void SetAxisConstraintInfluenceAngle(const double angle);
  double AxisConstraintOffsetAngle() const;
  void SetAxisConstraintOffsetAngle(const double angle);
  void InitializeConstraints();
  /// <summary>Generates a point display centered about the user origin in one or more of the three orthogonal planes for the current user grid.</summary>
  void DisplayGrid(CDC* deviceContext);
  EoGePoint3d GridOrign() const;
  void GridOrign(const EoGePoint3d& origin);
  void GetGridLineSpacing(double& x, double& y, double& z) const;
  void SetGridLineSpacing(double x, double y, double z);
  void GetGridPointSpacing(double& x, double& y, double& z) const;
  void SetGridPointSpacing(double x, double y, double z);
  void GetGridSnapSpacing(double& x, double& y, double& z) const;
  void SetGridSnapSpacing(double x, double y, double z);
  /// <summary>Determines the nearest point on system constraining grid.</summary>
  EoGePoint3d SnapPointToGrid(EoGePoint3d& pt) const;
  /// <summary>Set Axis constraint tolerance angle and offset axis constraint offset angle. Constrains a line to nearest axis pivoting on first endpoint.</summary>
  /// <remarks>Offset angle only support about z-axis</remarks>
  /// <returns>Point after snap</returns>
  EoGePoint3d SnapPointToAxis(EoGePoint3d& beginPoint, EoGePoint3d& endPoint) const;

  bool DisplayGridWithLines() const;
  void EnableDisplayGridWithLines(bool display);
  void EnableDisplayGridWithPoints(bool display);
  bool DisplayGridWithPoints() const;
  bool GridSnap() const;
  void EnableGridSnap(bool snap);
  void ViewZoomExtents() {}

#if defined(USING_Direct2D)
  ID2D1HwndRenderTarget* m_RenderTarget;
  ID2D1SolidColorBrush* m_LightSlateGrayBrush;
  ID2D1SolidColorBrush* m_RedBrush;

  HRESULT CreateDeviceResources();
  void DiscardDeviceResources();
#endif  // USING_Direct2D

 public:
  AeSysDoc* GetDocument() const;

 protected:  // Overrides
  void OnActivateFrame(UINT nState, CFrameWnd* pDeactivateFrame) override;
  void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;

 public:
  void OnDraw(CDC* deviceContext) override;  // overridden to draw this view
  void OnInitialUpdate() override;
  BOOL PreCreateWindow(CREATESTRUCT& createStructure) override;

 protected:
  BOOL OnPreparePrinting(CPrintInfo* printInformation) override;
  void OnBeginPrinting(CDC* deviceContext, CPrintInfo* printInformation) override;
  void OnPrepareDC(CDC* deviceContext, CPrintInfo* printInformation) override;
  void OnEndPrinting(CDC* deviceContext, CPrintInfo* printInformation) override;
  void OnUpdate(CView* sender, LPARAM hint, CObject* hintObject) override;

 public:
  virtual ~AeSysView();

#ifdef _DEBUG
  void AssertValid() const override;
  void Dump(CDumpContext& dc) const override;
#endif

 protected:  // Windows messages
  afx_msg void OnContextMenu(CWnd*, CPoint point);
  afx_msg void OnTimer(UINT_PTR nIDEvent);

 public:
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg BOOL OnEraseBkgnd(CDC* deviceContext);
  afx_msg void OnKillFocus(CWnd* newWindow);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint pnt);
  afx_msg void OnLButtonUp(UINT nFlags, CPoint pnt);
  afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
  afx_msg void OnMouseMove(UINT nFlags, CPoint pnt);

  /**
 * @brief Handle mouse wheel events for zooming in and out.
 * 
 * This function is called when the mouse wheel is scrolled. It checks the zDelta value to determine the direction of the scroll
 * and calls the appropriate zoom function (zoom in or zoom out).
 * 
 * @param flags Indicates whether various virtual keys are down.
 * @param zDelta The distance the wheel is rotated, expressed in multiples of WHEEL_DELTA.
 * @param point The current position of the cursor, in screen coordinates.
 * @return TRUE if the message was processed; otherwise, FALSE.
 */
  afx_msg BOOL OnMouseWheel(UINT flags, EoInt16 zDelta, CPoint point);
  afx_msg void OnPaint();
  afx_msg void OnRButtonDown(UINT nFlags, CPoint pnt);
  afx_msg void OnRButtonUp(UINT nFlags, CPoint pnt);
  afx_msg void OnSetFocus(CWnd* oldWindow);
  afx_msg void OnSize(UINT type, int cx, int cy);
  afx_msg void OnViewStateInformation();
  afx_msg void OnUpdateViewStateinformation(CCmdUI* pCmdUI);

 public:  // Operations
  /// <summary>Returns a pointer to the currently active view.</summary>
  static AeSysView* GetActiveView();

  void VerifyFindString(CMFCToolBarComboBoxButton* findCombo, CString& findText);

  bool m_ViewStateInformation;
  void UpdateStateInformation(EStateInformationItem item);

  CPoint m_middleButtonPanStartPoint;
  bool m_middleButtonPanInProgress;

  ERubs m_RubberbandType;
  EoGePoint3d m_RubberbandBeginPoint;
  CPoint m_RubberbandLogicalBeginPoint;
  CPoint m_RubberbandLogicalEndPoint;

  void RubberBandingDisable();
  void RubberBandingStartAtEnable(EoGePoint3d point3d, ERubs type);

  EoGePoint3d m_ptCursorPosDev;
  EoGePoint3d m_ptCursorPosWorld;

  EoGePoint3d GetCursorPosition();
  /// <summary> Positions cursor at targeted position.</summary>
  void SetCursorPosition(EoGePoint3d point3d);
  void SetModeCursor(int mode);

  /** @brief Selects a circle primitive using a point and tolerance.
   * @param point The point to use for selection.
   * @param tolerance The tolerance distance for selection.
   * @param circle A reference to a pointer that will receive the selected circle primitive.
   * @return A pointer to the group containing the selected circle, or nullptr if no circle was found.
   */
  EoDbGroup* SelectCircleUsingPoint(EoGePoint3d& point, double tolerance, EoDbConic*& circle);

  /**
   * Select a line primitive using a point.
   *
   * @param point The point to use for selection.
   * @param line  The line primitive that is selected.
   * @return The group containing the selected line primitive, or nullptr if no line is selected.
   */
  EoDbGroup* SelectLineUsingPoint(EoGePoint3d& point, EoDbLine*& line);

  EoDbGroup* SelSegAndPrimAtCtrlPt(const EoGePoint4d& pt);
  EoDbGroup* SelectLineUsingPoint(const EoGePoint3d& pt);
  EoDbText* SelectTextUsingPoint(const EoGePoint3d& pt);
  [[nodiscard]] EoDbGroup* SelectGroupAndPrimitive(const EoGePoint3d& pt);
  EoGePoint3d& DetPt() { return m_ptDet; }
  EoDbPrimitive*& EngagedPrimitive() { return m_EngagedPrimitive; }
  EoDbGroup*& EngagedGroup() { return m_EngagedGroup; }
  /// <summary>Set a pixel.</summary>
  void DisplayPixel(CDC* deviceContext, COLORREF colorReference, const EoGePoint3d& point);

  bool GroupIsEngaged() { return m_EngagedGroup != 0; }
  double& SelectApertureSize() { return m_SelectApertureSize; }
  void BreakAllPolylines();
  void BreakAllSegRefs();

  bool PenWidthsOn() const { return m_ViewPenWidths; }
  double GetWorldScale() const { return m_WorldScale; }
  void SetWorldScale(const double scale);
  bool RenderAsWireframe() const { return m_ViewWireframe; }
  auto AddGroup(EoDbGroup* group) { return m_VisibleGroupList.AddTail(group); }
  void AddGroups(EoDbGroupList* groups) { return m_VisibleGroupList.AddTail(groups); }
  auto RemoveGroup(EoDbGroup* group) { return m_VisibleGroupList.Remove(group); }
  void RemoveAllGroups() { m_VisibleGroupList.RemoveAll(); }
  void ResetView();
  /// <summary> Deletes last group detectable in the this view.</summary>
  void DeleteLastGroup();
  auto GetFirstVisibleGroupPosition() const { return m_VisibleGroupList.GetHeadPosition(); }
  auto GetLastGroupPosition() const { return m_VisibleGroupList.GetTailPosition(); }
  EoDbGroup* GetNextVisibleGroup(POSITION& position) { return m_VisibleGroupList.GetNext(position); }
  EoDbGroup* GetPreviousGroup(POSITION& position) { return m_VisibleGroupList.GetPrev(position); }
  void BackgroundImageDisplay(CDC* deviceContext);
  EoGeVector3d GetRelPos() const { return m_vRelPos; }
  bool ViewTrueTypeFonts() const { return m_ViewTrueTypeFonts; }

  /** @brief Displays the odometer information showing the relative position from the grid origin to the current cursor position, 
   * and optionally the line length and angle if in rubber band line mode.
   */
  void DisplayOdometer();

  /** Streams a sequence of characters as WM_KEYDOWN or WM_CHAR window messages.
  */
  void DoCustomMouseClick(const CString& characters);
  void DoCameraRotate(int iDir);
  void DoWindowPan(double ratio);

  void CopyActiveModelViewToPreviousModelView();
  EoGsViewTransform PreviousModelView();
  void ExchangeActiveAndPreviousModelViews();
  void InvokeNewModelTransform();
  void SetLocalModelTransform(EoGeTransformMatrix& transformation);
  void ReturnModelTransform();
  void ModelTransformPoint(EoGePoint4d& point);
  void ModelTransformPoint(EoGePoint3d& point);
  void ModelTransformPoints(int numberOfPoints, EoGePoint4d* points);
  void ModelTransformPoints(EoGePoint4dArray& pointsArray);
  void ModelTransformVector(EoGeVector3d vector);
  void ModelViewAdjustWindow(double& uMin, double& vMin, double& uMax, double& vMax, double ratio);
  void ModelViewGetViewport(EoGsViewport& viewport);
  EoGeVector3d CameraDirection() const;
  EoGeTransformMatrix& ModelViewGetMatrix();
  EoGeTransformMatrix& ModelViewGetMatrixInverse();
  EoGePoint3d CameraTarget() const;
  double UExtent() const;
  double UMax() const;
  double UMin() const;
  double VExtent() const;
  double VMax() const;
  double VMin() const;
  EoGeVector3d ViewUp() const;
  void ModelViewInitialize();

  void PopViewTransform();
  void PushViewTransform();
  void ModelViewTransformPoint(EoGePoint4d& point);
  void ModelViewTransformPoints(EoGePoint4dArray& pointsArray);
  void ModelViewTransformPoints(int numberOfPoints, EoGePoint4d* points);
  void ModelViewTransformVector(EoGeVector3d& vector);
  void SetViewTransform(EoGsViewTransform& viewTransform);
  void SetCenteredWindow(const double uExtent, const double vExtent);
  void SetCameraPosition(const EoGeVector3d& direction);
  void SetCameraTarget(const EoGePoint3d& target);
  void SetViewWindow(const double uMin, const double vMin, const double uMax, const double vMax);

  /// <summary>Determines the number of pages for 1 to 1 print</summary>
  UINT NumPages(CDC* deviceContext, double dScaleFactor, UINT& nHorzPages, UINT& nVertPages);

  double OverviewUExt() { return m_OverviewViewTransform.UExtent(); }
  double OverviewUMin() { return m_OverviewViewTransform.UMin(); }
  double OverviewVExt() { return m_OverviewViewTransform.VExtent(); }
  double OverviewVMin() { return m_OverviewViewTransform.VMin(); }
  CPoint DoProjection(const EoGePoint4d& pt) { return m_Viewport.DoProjection(pt); }
  void DoProjection(CPoint* pnt, int iPts, EoGePoint4d* pt) { m_Viewport.DoProjection(pnt, iPts, pt); }
  void DoProjection(CPoint* pnt, EoGePoint4dArray& pointsArray) { m_Viewport.DoProjection(pnt, pointsArray); }
  void DoProjectionInverse(EoGePoint3d& pt) { m_Viewport.DoProjectionInverse(pt); }
  double HeightInInches() { return m_Viewport.HeightInInches(); }
  double WidthInInches() { return m_Viewport.WidthInInches(); }
  void ViewportPopActive();
  void ViewportPushActive();
  void SetViewportSize(const int width, const int height) { m_Viewport.SetSize(width, height); }
  void SetDeviceHeightInInches(double height) { m_Viewport.SetDeviceHeightInInches(height); }
  void SetDeviceWidthInInches(double width) { m_Viewport.SetDeviceWidthInInches(width); }

 public:  // Group and Primitive operations
  EoDbGroup* m_SubModeEditGroup;
  EoDbPrimitive* m_SubModeEditPrimitive;
  EoGePoint3d m_SubModeEditBeginPoint;
  EoGePoint3d m_SubModeEditEndPoint;
  EoGeTransformMatrix m_tmEditSeg;

  void InitializeGroupAndPrimitiveEdit();
  void DoEditGroupCopy();
  void DoEditGroupEscape();
  void DoEditGroupTransform(EoUInt16 operation);
  void DoEditPrimitiveTransform(EoUInt16 operation);
  void DoEditPrimitiveCopy();
  void DoEditPrimitiveEscape();
  void PreviewPrimitiveEdit();
  void PreviewGroupEdit();

  EoGePoint3d m_MendPrimitiveBegin;
  DWORD m_MendPrimitiveVertexIndex;
  EoDbPrimitive* m_PrimitiveToMend;
  EoDbPrimitive* m_PrimitiveToMendCopy;

  void PreviewMendPrimitive();
  void MendPrimitiveEscape();
  void MendPrimitiveReturn();

  /// Annotate Mode Interface ///////////////////////////////////////////////////
 private:                   // Annotate and Dimension interface
  double m_GapSpaceFactor;  // Edge space factor 25 percent of character height
  double m_CircleRadius;
  int m_EndItemType;
  double m_EndItemSize;
  double m_BubbleRadius;
  int m_NumberOfSides;  // Number of sides on bubble (0 indicating circle)
  CString m_DefaultText;

 public:
  double BubbleRadius() const { return m_BubbleRadius; }
  void SetBubbleRadius(double radius) { m_BubbleRadius = radius; }
  double CircleRadius() const { return m_CircleRadius; }
  void SetCircleRadius(double radius) { m_CircleRadius = radius; }
  CString DefaultText() const { return m_DefaultText; }
  void SetDefaultText(const CString& text) { m_DefaultText = text; }
  double EndItemSize() const { return m_EndItemSize; }
  void SetEndItemSize(double size) { m_EndItemSize = size; }
  int EndItemType() const { return m_EndItemType; }
  void SetEndItemType(int type) { m_EndItemType = type; }
  double GapSpaceFactor() const { return m_GapSpaceFactor; }
  void SetGapSpaceFactor(double factor) { m_GapSpaceFactor = factor; }
  int NumberOfSides() const { return m_NumberOfSides; }
  void SetNumberOfSides(int number) { m_NumberOfSides = number; }

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

  /// <summary>Generates arrow heads for annotation mode.</summary>
  /// <param name="type">type of end item</param>
  /// <param name="size">size of end item</param>
  /// <param name="beginPoint">tail of line segment defining arrow head</param>
  /// <param name="endPoint">head of line segment defining arrow head</param>
  /// <param name="group">group where primitives are placed</param>
  void GenerateLineEndItem(int type, double size, EoGePoint3d& beginPoint, EoGePoint3d& endPoint,
                           EoDbGroup* group) const;
  bool CorrectLeaderEndpoints(int beginType, int endType, EoGePoint3d& beginPoint, EoGePoint3d& endPoint) const;

  /// Draw Mode Interface ///////////////////////////////////////////////////////
 public:
  void DoDrawModeMouseMove();

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

  /// Draw Mode2 Interface //////////////////////////////////////////////////////

 private:
  double m_CenterLineEccentricity;  // Center line eccentricity for parallel lines
  bool m_ContinueCorner;
  double m_DistanceBetweenLines;
  EoGeLine m_CurrentLeftLine;
  EoGeLine m_CurrentRightLine;
  EoGeLine m_PreviousReferenceLine;
  EoGeLine m_CurrentReferenceLine;
  EoDbGroup* m_AssemblyGroup;
  EoDbGroup* m_EndSectionGroup;
  EoDbGroup* m_BeginSectionGroup;
  EoDbLine* m_BeginSectionLine;
  EoDbLine* m_EndSectionLine;

 public:
  void DoDraw2ModeMouseMove();

  afx_msg void OnDraw2ModeOptions();
  /// <summary>Searches for an existing wall side or endcap</summary>
  afx_msg void OnDraw2ModeJoin();
  afx_msg void OnDraw2ModeWall();
  afx_msg void OnDraw2ModeReturn();
  afx_msg void OnDraw2ModeEscape();

  bool CleanPreviousLines();
  bool StartAssemblyFromLine();

  enum EJust { Left = -1, Center, Right };
  enum EElbow { Mittered, Radial };

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

  double m_FixupModeAxisTolerance;
  double m_FixupModeCornerSize;

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
  /// <summary>Translate all control points identified</summary>
  afx_msg void OnNodalModeMove();
  afx_msg void OnNodalModeCopy();
  afx_msg void OnNodalModeToLine();
  afx_msg void OnNodalModeToPolygon();
  afx_msg void OnNodalModeEmpty();
  /// @brief Handles the engagement of nodal mode by updating the nodal list with all points from the currently engaged primitive.
  afx_msg void OnNodalModeEngage();
  afx_msg void OnNodalModeReturn();
  afx_msg void OnNodalModeEscape();

  void ConstructPreviewGroup();
  void ConstructPreviewGroupForNodalGroups();

 public:  // Cut mode interface
  afx_msg void OnCutModeOptions();
  /// <summary>Cuts a primative at cursor position.</summary>
  afx_msg void OnCutModeTorch();
  /// <summary>Cuts all primatives which intersect with line defined by two points.</summary>
  // Notes: Colinear fill area edges are not considered to intersect.
  afx_msg void OnCutModeSlice();
  afx_msg void OnCutModeField();
  /// <summary>Cuts a primative at two pnts and puts non-null middle piece in trap.</summary>
  // Notes:	Accuracy of arc section cuts diminishes with high
  //			eccentricities. if two cut points are coincident
  //			nothing happens.
  afx_msg void OnCutModeClip();
  afx_msg void OnCutModeDivide();
  afx_msg void OnCutModeReturn();
  afx_msg void OnCutModeEscape();

 public:  // Edit mode interface
  EoGeVector3d m_EditModeMirrorScale;
  EoGeVector3d m_editModeRotationAngles;
  EoGeVector3d m_EditModeScale;

  [[nodiscard]] EoGeVector3d EditModeRotationAngles() const { return m_editModeRotationAngles; }
  EoGeTransformMatrix EditModeInvertedRotationTMat() const {
    EoGeTransformMatrix Matrix;
    Matrix = Matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
    Matrix.Inverse();
    return Matrix;
  }
  EoGeTransformMatrix EditModeRotationTMat() const {
    EoGeTransformMatrix Matrix;
    Matrix = Matrix.BuildRotationTransformMatrix(EditModeRotationAngles());
    return Matrix;
  }
  EoGeVector3d EditModeInvertedScaleFactors() const {
    EoGeVector3d InvertedScaleFactors;

    InvertedScaleFactors.x = fabs(m_EditModeScale.x) > Eo::geometricTolerance ? 1.0 / m_EditModeScale.x : 1.0;
    InvertedScaleFactors.y = fabs(m_EditModeScale.y) > Eo::geometricTolerance ? 1.0 / m_EditModeScale.y : 1.0;
    InvertedScaleFactors.z = fabs(m_EditModeScale.z) > Eo::geometricTolerance ? 1.0 / m_EditModeScale.z : 1.0;

    return InvertedScaleFactors;
  }
  EoGeVector3d EditModeScaleFactors() const { return m_EditModeScale; }
  void SetEditModeScaleFactors(const double x, const double y, const double z) { m_EditModeScale.Set(x, y, z); }
  void SetEditModeRotationAngles(double x, double y, double z) { m_editModeRotationAngles.Set(x, y, z); }
  EoGeVector3d EditModeMirrorScale() const { return m_EditModeMirrorScale; }
  void SetMirrorScale(double x, double y, double z) { m_EditModeMirrorScale.Set(x, y, z); }

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
  /// <summary>Identifies groups which intersect with a line and adds them to the trap.</summary>
  afx_msg void OnTrapModeStitch();
  /// <summary>Identifies groups which lie wholly or partially within a rectangular area.</summary>
  /// <remarks>Needs to be generalized to quad.</remarks>
  afx_msg void OnTrapModeField();
  /// <summary>Adds last detectable group which is not already in trap to trap</summary>
  afx_msg void OnTrapModeLast();
  afx_msg void OnTrapModeEngage();
  afx_msg void OnTrapModeMenu();
  afx_msg void OnTrapModeModify();
  afx_msg void OnTrapModeEscape();

  afx_msg void OnTraprModeRemoveAdd();
  afx_msg void OnTraprModePoint();
  /// <summary>Identifies groups which intersect with a line and removes them from the trap.</summary>
  afx_msg void OnTraprModeStitch();
  /// <summary>Identifies groups which lie wholly or partially within a orthoganal rectangular area.</summary>
  // Notes: This routine fails in all but top view. !!
  // Parameters:	pt1 	one corner of the area
  //				pt2 	other corner of the area
  afx_msg void OnTraprModeField();
  afx_msg void OnTraprModeLast();
  afx_msg void OnTraprModeEngage();
  afx_msg void OnTraprModeMenu();
  afx_msg void OnTraprModeModify();
  afx_msg void OnTraprModeEscape();

 private:  // Low Pressure Duct (retangular) interface
  double m_InsideRadiusFactor;
  double m_DuctSeamSize;
  double m_DuctTapSize;
  bool m_GenerateTurningVanes;
  EElbow m_ElbowType;
  EJust m_DuctJustification;
  double m_TransitionSlope;
  bool m_BeginWithTransition;
  bool m_ContinueSection;
  int m_EndCapLocation;
  EoDbPoint* m_EndCapPoint;
  EoDbGroup* m_EndCapGroup;
  bool m_OriginalPreviousGroupDisplayed;
  EoDbGroup* m_OriginalPreviousGroup;

  Section m_PreviousSection;
  Section m_CurrentSection;

 public:
  void DoDuctModeMouseMove();

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
  EoDbGroup* SelectPointUsingPoint(EoGePoint3d& point, double tolerance, EoInt16 color, EoInt16 pointStyle,
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
  bool Find2LinesUsingLineEndpoints(EoDbLine* testLinePrimitive, double angularTolerance, EoGeLine& leftLine,
                                    EoGeLine& rightLine);
  /// <summary>Generates an end-cap.</summary>
  /// <remarks>
  ///End-caps are groups containing a line and a point.  The line defines the orientation of the end-cap.
  ///The point contains information about the cross-section (width, depth)
  ///and optionally a number which might be used for something like cfm.
  /// </remarks>
  /// <param name="beginPoint">begin point of the line</param>
  /// <param name="endPoint">end point of the line</param>
  /// <param name="section">width and depth data</param>
  /// <param name="group"></param>
  void GenerateEndCap(EoGePoint3d& beginPoint, EoGePoint3d& endPoint, Section section, EoDbGroup* group);
  /// <summary>Generates rise or drop fitting.</summary>
  /// <param name="riseDropIndicator">	rise or drop indicator; 1 rise, 2 drop</param>
  /// <param name="section">horizontal section width and depth</param>
  void GenerateRiseDrop(EoUInt16 riseDropIndicator, Section section, EoGeLine& referenceLine, EoDbGroup* group);
  /// <summary>Generates rectangular section using a set of parallel lines.</summary>
  /// <param name="section">width and depth of section</param>
  /// <param name="group"></param>
  void GenerateRectangularSection(EoGeLine& referenceLine, double eccentricity, Section section, EoDbGroup* group);
  /// <summary> Generates text segment representing width and depth of a piece of duct. </summary>
  void GenSizeNote(EoGePoint3d, double angle, Section section);
  /// <param name="previousReferenceLine"></param>
  /// <param name="previousSection"></param>
  /// <param name="currentReferenceLine">on exit the begin point is the same as the point on the endcap</param>
  /// <param name="currentSection"></param>
  /// <param name="group"></param>
  void GenerateRectangularElbow(EoGeLine& previousReferenceLine, Section previousSection,
                                EoGeLine& currentReferenceLine, Section currentSection, EoDbGroup* group);
  /// <summary>Generates rectangular tap fitting.</summary>
  /// <param name="justification"></param>
  /// <param name="section"></param>
  bool GenerateRectangularTap(EJust justification, Section section);

  /** @brief Generates a mitered bullhead tee fitting. (placeholder)
   *  @param existingGroup group containing rectangular section to join
   *  @param existingSectionReferenceLine
   *  @param existingSectionWidth
   *  @param existingSectionDepth
   *  @param group
   *  @return Center point of end cap of exit transition.
   *  @note Requires current operation to be a regular rectangular section. The selection based on the current cursor location
   *  identifies the second section, and the direction from the point to the cursor location defines the direction for the two elbow turns.
   *  @note Placeholder until implementation is return of (0.0, 0.0, 0.0)
   */
  EoGePoint3d GenerateBullheadTee(EoDbGroup* existingGroup, EoGeLine& existingSectionReferenceLine,
                                  double existingSectionWidth, double existingSectionDepth, EoDbGroup* group);

  /** @brief Generates a full elbow takeoff from an existing section to the current section.
   * @param existingSectionReferenceLine Reference line of the existing section.
   * @param existingSection Cross sectional data of the existing section.
   * @param group Group to add the generated primitives to.
   */
  void GenerateFullElbowTakeoff(EoDbGroup* existingGroup, EoGeLine& existingSectionReferenceLine,
                                Section existingSection, EoDbGroup* group);

  /// <summary>Generates section which transitions from one rectangle to another</summary>
  /// <param name="referenceLine">line defining the begin point and direction of the transition</param>
  /// <param name="eccentricity"></param>
  /// <param name="justification"></param>
  /// <param name="slope">slope of the transition</param>
  /// <param name="previousSection">width and depth at begin of the transition</param>
  /// <param name="currentSection">width and depth at end of the transition</param>
  /// <param name="group">group receiving the primitives</param>
  void GenerateTransition(EoGeLine& referenceLine, double eccentricity, EJust justification, double slope,
                          Section previousSection, Section currentSection, EoDbGroup* group);
  /// <summary>Sets the width and depth of ductwork.</summary>
  void SetDuctOptions(Section& section);
  /// <summary>Determines the total length required to transition duct from one size to another</summary>
  /// <param name="justification">justification: 0 centered, %gt 0 taper to right, %lt 0 taper to left</param>
  /// <param name="slope">slope of the section sides</param>
  /// <param name="previousSection">width and depth of begin section</param>
  /// <param name="currentSection">width and depth of end section</param>
  /// <returns>length of the transition</returns>
  double LengthOfTransition(EJust justification, double slope, Section previousSection, Section currentSection);

 private:  // Pipe mode interface
  int m_CurrentPipeSymbolIndex;
  double m_PipeTicSize;
  double m_PipeRiseDropRadius;

  /// <summary>Adds a fitting indication to horizontal pipe section as required by previous fitting type.</summary>
  void GenerateLineWithFittings(int beginType, EoGePoint3d& beginPoint, int endType, EoGePoint3d& endPoint,
                                EoDbGroup* group);
  /// <summary>Draws tic mark at a point distance from begin point on the line defined by begin and end points.</summary>
  bool GenerateTicMark(EoGePoint3d& beginPoint, EoGePoint3d& endPoint, double distance, EoDbGroup* group) const;
  void DropFromOrRiseIntoHorizontalSection(EoGePoint3d& point, EoDbGroup* group, EoDbLine* section);
  void DropIntoOrRiseFromHorizontalSection(EoGePoint3d& point, EoDbGroup* group, EoDbLine* section);

 public:
  void DoPipeModeMouseMove();

  afx_msg void OnPipeModeOptions();
  afx_msg void OnPipeModeLine();
  afx_msg void OnPipeModeFitting();
  afx_msg void OnPipeModeRise();
  afx_msg void OnPipeModeDrop();
  /// <summary>Generates a piping symbol at point specified if pipe section located.</summary>
  afx_msg void OnPipeModeSymbol();
  afx_msg void OnPipeModeWye();
  afx_msg void OnPipeModeReturn();
  afx_msg void OnPipeModeEscape();

 private:  // Power mode interface
  bool m_PowerArrow;
  bool m_PowerConductor;
  double m_PowerConductorSpacing;
  EoGePoint3d m_CircuitEndPoint;
  double m_PreviousRadius;

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

  void GeneratePowerConductorSymbol(EoUInt16 conductorType, EoGePoint3d& pointOnCircuit, EoGePoint3d& endPoint) const;
  void GenerateHomeRunArrow(EoGePoint3d& pointOnCircuit, EoGePoint3d& endPoint) const;
  void DoPowerModeConductor(EoUInt16 conductorType);

 public:
  /// @brief Updates the status bar mode line display with mode-specific operation information and optionally draws the pane text in the active view.
  void ModeLineDisplay();

  /// @brief Highlights a mode line operation by setting its text color to red in the status bar and optionally in the active view.
  /// @param command The operation command identifier to highlight. A value of 0 indicates no operation should be highlighted.
  /// @return The command identifier that was highlighted, or 0 if no operation was highlighted.
  EoUInt16 ModeLineHighlightOp(EoUInt16 command);

  /// @brief Removes highlighting from a mode line operation pane and updates the display.
  /// @param command Reference to the command identifier to unhighlight. Set to 0 after unhighlighting is complete.
  void ModeLineUnhighlightOp(EoUInt16& command);

  /// @brief Gets a reference to the application's status bar.
  /// @return A reference to the CMFCStatusBar object from the main frame window.
  CMFCStatusBar& GetStatusBar() const;

 public:
  afx_msg void OnBackgroundImageLoad();
  afx_msg void OnBackgroundImageRemove();
  afx_msg void OnFilePlotHalf();
  afx_msg void OnFilePlotFull();
  afx_msg void OnFilePlotQuarter();
  afx_msg void OnFilePrint();
  afx_msg void OnFind();
  afx_msg void On3dViewsBack();
  afx_msg void On3dViewsBottom();
  afx_msg void On3dViewsFront();
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
  afx_msg void OnToolsPrimitiveSnapto();
  afx_msg void OnUpdateViewOdometer(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewTrueTypeFonts(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewBackgroundImage(CCmdUI* pCmdUI);
  afx_msg void OnUpdateBackgroundimageLoad(CCmdUI* pCmdUI);
  afx_msg void OnUpdateBackgroundimageRemove(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewPenwidths(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewRendered(CCmdUI* pCmdUI);
  afx_msg void OnUpdateViewWireframe(CCmdUI* pCmdUI);
  afx_msg void OnViewBackgroundImage();
  afx_msg void OnViewTrueTypeFonts();
  afx_msg void OnViewPenWidths();
  afx_msg void OnViewOdometer();
  afx_msg void OnViewRefresh();
  afx_msg void OnViewParameters();
  afx_msg void OnViewLighting();
  afx_msg void OnViewSolid();
  afx_msg void OnViewRendered();
  afx_msg void OnViewWindowKeyplan();
  afx_msg void OnViewWireframe();
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
  afx_msg void OnSetupMouseButtons();
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
  afx_msg void OnEditFind();

 protected:
  DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in PegView.cpp
inline AeSysDoc* AeSysView::GetDocument() const { return reinterpret_cast<AeSysDoc*>(m_pDocument); }
#endif
