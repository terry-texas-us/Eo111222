#pragma once

#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbHeaderSection.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbMaskedPrimitive.h"
#include "EoDbPrimitive.h"
#include "EoGePoint3d.h"
#include "EoGeUniquePoint.h"
#include "EoGeVector3d.h"

class EoGeTransformMatrix;

class AeSysDoc : public CDocument {
 protected:
  AeSysDoc();
  AeSysDoc(const AeSysDoc&) = delete;
  AeSysDoc& operator=(const AeSysDoc&) = delete;

  DECLARE_DYNCREATE(AeSysDoc)

 private:
  CString m_IdentifiedLayerName;
  EoDb::FileTypes m_SaveAsType;

  EoDbHeaderSection m_HeaderSection;
  EoDbLineTypeTable m_LineTypeTable;
  EoDbLineType* m_ContinuousLineType;
  EoDbBlocks m_BlocksTable;
  CLayers m_LayerTable;
  EoDbLayer* m_workLayer;
  EoDbGroupList m_DeletedGroupList;
  EoDbGroupList m_TrappedGroupList;
  EoGePoint3d m_TrapPivotPoint;

  EoDbGroupList m_NodalGroupList;
  CObList m_MaskedPrimitives;
  CObList m_UniquePoints;
  double m_pointSize{0.0};  // in drawing units when greater than zero; in pixels when less than zero; default otherwise

  // Overrides
 public:
  BOOL DoSave(LPCWSTR lpszPathName, BOOL bReplace = TRUE) override;
  void SetCommonTableEntries();
  BOOL OnNewDocument() override;
  BOOL OnOpenDocument(LPCWSTR lpszPathName) override;
  BOOL OnSaveDocument(LPCWSTR lpszPathName) override;

  /** @brief Deletes the contents of the document.
   *
   * This method is called to clear the document's data without destroying the CDocument object itself.
   * It is typically invoked before the document is destroyed or when reusing the document.
   * It can also be called to implement commands like "Edit Clear All" that require deleting all of the document's data.
   */
  void DeleteContents() override;

  ~AeSysDoc() override;

 public:
  void InitializeGroupAndPrimitiveEdit();

  /** @brief Provides access to the document's header section.
   *
   * @return A reference to the EoDbHeaderSection object.
   */
  EoDbHeaderSection& HeaderSection() { return m_HeaderSection; }
  const EoDbHeaderSection& HeaderSection() const { return m_HeaderSection; }

  /// <summary>Constructs 0 to many seperate text primitives for each "\r\n" delimited substr.</summary>
  void AddTextBlock(LPWSTR pszText);

  // Block Table interface
  EoDbBlocks* BlocksTable() { return (&m_BlocksTable); }
  bool BlockTableIsEmpty() { return m_BlocksTable.IsEmpty() == TRUE; }
  EoUInt16 BlockTableSize() { return (EoUInt16(m_BlocksTable.GetSize())); }
  int GetBlockReferenceCount(const CString& name);
  auto GetFirstBlockPosition() { return m_BlocksTable.GetStartPosition(); }
  void GetNextBlock(POSITION& position, CString& name, EoDbBlock*& block) {
    m_BlocksTable.GetNextAssoc(position, name, block);
  }
  bool LookupBlock(CString name, EoDbBlock*& block);

  /** @brief Removes all blocks from the block table. */
  void RemoveAllBlocks();

  /** @brief Removes all blocks which have no references. */
  void RemoveUnusedBlocks();

  /** @brief Inserts a block into the block table.
   *
   * If a block with the same name already exists, it is replaced.
   *
   * @param name The name of the block.
   * @param block A pointer to the EoDbBlock object to insert.
   */
  void InsertBlock(const CString& name, EoDbBlock* block) {
    EoDbBlock* existing = nullptr;
    if (m_BlocksTable.Lookup(name, existing)) {
      ATLTRACE2(static_cast<int>(atlTraceGeneral), 0, L"InsertBlock: Replacing existing block '%s'\n",
                name.GetString());
      existing->DeletePrimitivesAndRemoveAll();
      delete existing;
    }
    m_BlocksTable.SetAt(name, block);
  }

  void LayerBlank(const CString& strName);
  /// <summary>A layer is converted to a tracing or a job file</summary>
  bool LayerMelt(CString& strName);

  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
  int NumberOfGroupsInWorkLayer();
  int NumberOfGroupsInActiveLayers();
  /// <summary>Displays drawing and determines which groups are detectable.</summary>
  void DisplayAllLayers(AeSysView* view, CDC* deviceContext);

  // Layer Table interface
  EoDbLayer* GetLayerTableLayer(const CString& layerName);
  EoDbLayer* GetLayerTableLayerAt(int layer);
  int GetLayerTableSize() { return (int)m_LayerTable.GetSize(); }
  int FindLayerTableLayer(const CString& layerName) const;
  void RemoveLayerTableLayer(const CString& strName);
  void RemoveAllLayerTableLayers();
  void RemoveLayerTableLayerAt(int);
  void AddLayerTableLayer(EoDbLayer* layer) { m_LayerTable.Add(layer); }
  void RemoveEmptyLayers();

  EoDbLayer* LayersSelUsingPoint(const EoGePoint3d&);

  /// Get the stored point size. Positive => drawing units; negative => pixels.
  double GetPointSize() const noexcept { return m_pointSize; }
  void SetPointSize(double size) noexcept { m_pointSize = size; }

  // Line Type Table interface
  EoDbLineTypeTable* LineTypeTable() { return &m_LineTypeTable; }
  EoDbLineType* ContinuousLineType() { return m_ContinuousLineType; }

  void PenTranslation(EoUInt16, EoInt16*, EoInt16*);

  int RemoveEmptyNotesAndDelete();
  int RemoveEmptyGroups();

  void ResetAllViews();
  void AddGroupToAllViews(EoDbGroup* group);
  void AddGroupsToAllViews(EoDbGroupList* groups);
  void RemoveGroupFromAllViews(EoDbGroup* group);
  void RemoveAllGroupsFromAllViews();

  EoDbLayer* AnyLayerRemove(EoDbGroup* group);

  // Work Layer interface
  void AddWorkLayerGroup(EoDbGroup* group);
  void AddWorkLayerGroups(EoDbGroupList* groups);
  auto FindWorkLayerGroup(EoDbGroup* group) const { return (m_workLayer->Find(group)); }
  auto GetFirstWorkLayerGroupPosition() const { return m_workLayer->GetHeadPosition(); }
  EoDbGroup* GetLastWorkLayerGroup() const;
  auto GetLastWorkLayerGroupPosition() const { return m_workLayer->GetTailPosition(); }
  EoDbGroup* GetNextWorkLayerGroup(POSITION& position) const { return m_workLayer->GetNext(position); }
  EoDbGroup* GetPreviousWorkLayerGroup(POSITION& position) const { return m_workLayer->GetPrev(position); }
  EoDbLayer* GetWorkLayer() const { return m_workLayer; }
  void InitializeWorkLayer();
  EoDbLayer* SetWorkLayer(EoDbLayer* layer);

  void WriteShadowFile();

  // Deleted groups interface
  auto DeletedGroupsAddHead(EoDbGroup* group) { return (m_DeletedGroupList.AddHead(group)); }
  auto DeletedGroupsAddTail(EoDbGroup* group) { return (m_DeletedGroupList.AddTail(group)); }
  bool DeletedGroupsIsEmpty() { return (m_DeletedGroupList.IsEmpty() == TRUE); }
  // CobList asserts on calls to RemoveHead & RemoveTail if list is empty!
  EoDbGroup* DeletedGroupsRemoveHead() { return (m_DeletedGroupList.RemoveHead()); }
  void DeletedGroupsRemoveGroups() { m_DeletedGroupList.DeleteGroupsAndRemoveAll(); }
  EoDbGroup* DeletedGroupsRemoveTail() { return (m_DeletedGroupList.RemoveTail()); }
  /// <summary>Restores the last group added to the deleted group list.</summary>
  void DeletedGroupsRestore();

 public:  // trap interface
  void AddGroupsToTrap(EoDbGroupList* groups);
  POSITION AddGroupToTrap(EoDbGroup* group);
  /// <summary>Builds a single group from two or more groups in trap.</summary>
  /// <remarks>The new group is added to the hot layer even if the trap contained groups from one or more warm layers.</remarks>
  void CompressTrappedGroups();
  void CopyTrappedGroups(EoGeVector3d translate);
  void DeleteAllTrappedGroups();

  /** @brief Copies the trapped groups to the clipboard in various formats.
   *
   * This method opens the clipboard, empties its current contents, and then copies the trapped groups
   * to the clipboard in text, image, or group formats based on the application's clipboard settings.
   * After copying the data, it closes the clipboard.
   *
   * @param view A pointer to the AeSysView object used for rendering the groups.
   * @note This is done with two independent clipboard formats. The standard enhanced metafile and the private EoDbGroupList which is read exclusively by Peg.
   */
  void CopyTrappedGroupsToClipboard(AeSysView* view);

  /** @brief Expands each trapped group into separate groups for each primitive.
   *
   * This method iterates through the list of trapped groups and for each group,
   * it creates new groups containing individual primitives from the original group.
   * The original groups are removed from the document and deleted.
   * The new groups are added to the work layer and the trapped group list.
   */
  void ExpandTrappedGroups();
  auto FindTrappedGroup(EoDbGroup* group) { return m_TrappedGroupList.Find(group); }
  auto GetFirstTrappedGroupPosition() const { return m_TrappedGroupList.GetHeadPosition(); }
  EoDbGroup* GetNextTrappedGroup(POSITION& position) { return m_TrappedGroupList.GetNext(position); }
  EoGePoint3d GetTrapPivotPoint() const { return m_TrapPivotPoint; }
  BOOL IsTrapEmpty() const { return m_TrappedGroupList.IsEmpty(); }
  void ModifyTrappedGroupsColor(EoInt16 color) { m_TrappedGroupList.ModifyColor(color); }
  void ModifyTrappedGroupsLineType(EoInt16 lineType) { m_TrappedGroupList.ModifyLineType(lineType); }
  void ModifyTrappedGroupsNoteAttributes(EoDbFontDefinition& fontDef, EoDbCharacterCellDefinition& cellDef,
                                         int attributes);
  void RemoveAllTrappedGroups();
  EoDbGroup* RemoveLastTrappedGroup() { return m_TrappedGroupList.RemoveTail(); }
  auto RemoveTrappedGroup(EoDbGroup* group) { return m_TrappedGroupList.Remove(group); }
  void RemoveTrappedGroupAt(POSITION position) { m_TrappedGroupList.RemoveAt(position); }
  void SetTrapPivotPoint(const EoGePoint3d& pt) { m_TrapPivotPoint = pt; }
  void SquareTrappedGroups(AeSysView* view);
  void TransformTrappedGroups(EoGeTransformMatrix& tm);
  void TranslateTrappedGroups(EoGeVector3d translate);
  INT_PTR TrapGroupCount() { return m_TrappedGroupList.GetCount(); }
  EoDbGroupList* GroupsInTrap() { return &m_TrappedGroupList; }

  // Nodal list interface (includes list of groups, primitives and unique points)
  void DeleteNodalResources();
  /// <summary>Maintains a list of the primatives with at least one identified node.</summary>
  void UpdateNodalList(EoDbGroup* group, EoDbPrimitive* primitive, DWORD mask, int bit, EoGePoint3d point);

  auto AddNodalGroup(EoDbGroup* group) { return m_NodalGroupList.AddTail(group); }
  auto FindNodalGroup(EoDbGroup* group) { return m_NodalGroupList.Find(group); }
  auto GetFirstNodalGroupPosition() const { return m_NodalGroupList.GetHeadPosition(); }
  EoDbGroup* GetNextNodalGroup(POSITION& position) { return m_NodalGroupList.GetNext(position); }
  void RemoveAllNodalGroups() { m_NodalGroupList.RemoveAll(); }
  POSITION AddMaskedPrimitive(EoDbMaskedPrimitive* maskedPrimitive) {
    return m_MaskedPrimitives.AddTail((CObject*)maskedPrimitive);
  }
  auto GetFirstMaskedPrimitivePosition() const { return m_MaskedPrimitives.GetHeadPosition(); }
  EoDbMaskedPrimitive* GetNextMaskedPrimitive(POSITION& position) {
    return (EoDbMaskedPrimitive*)m_MaskedPrimitives.GetNext(position);
  }
  void RemoveAllMaskedPrimitives() { m_MaskedPrimitives.RemoveAll(); }
  DWORD GetPrimitiveMask(EoDbPrimitive* primitive);
  void AddPrimitiveBit(EoDbPrimitive* primitive, int bit);
  void RemovePrimitiveBit(EoDbPrimitive* primitive, int bit);

  int AddUniquePoint(const EoGePoint3d& point);
  auto AddUniquePoint(EoGeUniquePoint* uniquePoint) { return m_UniquePoints.AddTail((CObject*)uniquePoint); }
  void DisplayUniquePoints();
  auto GetFirstUniquePointPosition() const { return m_UniquePoints.GetHeadPosition(); }
  EoGeUniquePoint* GetNextUniquePoint(POSITION& position) { return (EoGeUniquePoint*)m_UniquePoints.GetNext(position); }
  void RemoveUniquePointAt(POSITION position) { m_UniquePoints.RemoveAt(position); }
  void RemoveAllUniquePoints() { m_UniquePoints.RemoveAll(); }
  int RemoveUniquePoint(const EoGePoint3d& point);

 public:  // Generated message map functions
  afx_msg void OnBlocksLoad();
  afx_msg void OnBlocksRemoveUnused();
  afx_msg void OnBlocksUnload();
  afx_msg void OnClearActiveLayers();
  afx_msg void OnClearAllLayers();
  afx_msg void OnClearAllTracings();
  afx_msg void OnClearMappedTracings();
  afx_msg void OnClearViewedTracings();
  afx_msg void OnClearWorkingLayer();
  /// <summary>The current view is copied to the clipboard as an enhanced metafile.</summary>
  afx_msg void OnEditImageToClipboard();
  afx_msg void OnEditSegToWork();
  /// <summary>Pastes clipboard to drawing. Only EoGroups format handled and no translation is performed.</summary>
  afx_msg void OnEditTrace();
  afx_msg void OnEditTrapCopy();
  afx_msg void OnEditTrapCut();
  afx_msg void OnEditTrapDelete();
  /// <summary>Initializes current trap and all trap component lists.</summary>
  afx_msg void OnEditTrapQuit();

  /** @brief Pastes the contents of the clipboard into the current drawing.
   *
   * This method checks the clipboard for data in the EoGroups format, which is specific to the application or CF_TEXT format.
   * If EoGroups format is found, it is pasted into the current drawing, and any other formats present in the clipboard are ignored.
   * This ensures that only compatible data is processed and integrated into the drawing.
   */
  afx_msg void OnEditTrapPaste();

  /// <summary>Adds all groups in the work layer to the trap.</summary>
  afx_msg void OnEditTrapWork();
  /// <summary>Add all groups in all work and active layers to the trap.</summary>
  afx_msg void OnEditTrapWorkAndActive();
  afx_msg void OnFile();
  afx_msg void OnFileManage();
  afx_msg void OnFileQuery();
  afx_msg void OnFileTracing();
  afx_msg void OnLayerActive();
  afx_msg void OnLayerHidden();
  afx_msg void OnLayerMelt();
  afx_msg void OnLayerStatic();
  afx_msg void OnLayerWork();

  /** @brief Activates all layers, except for the current working layer.
   * This is a warm state meaning the layer is displayed using hot color set, is detectable,
   * and may have its groups modified or deleted. No new groups are added to an active layer.
   * Zero or more layers may be active.
   */
  afx_msg void OnLayersActiveAll();
  afx_msg void OnLayersStaticAll();
  afx_msg void OnLayersRemoveEmpty();
  afx_msg void OnMaintenanceRemoveEmptyNotes();
  afx_msg void OnMaintenanceRemoveEmptyGroups();
  afx_msg void OnPensEditColors();
  afx_msg void OnPensLoadColors();
  afx_msg void OnPensRemoveUnusedStyles();
  afx_msg void OnPensTranslate();
  /// <summary>Breaks a primitive into a simpler set of primitives.</summary>
  afx_msg void OnPrimBreak();
  /// <summary>Searches for closest detectible primitive. If found, primitive is lifted from its group, inserted into a new group which is added to deleted group list. The primitive resources are not freed.</summary>
  afx_msg void OnToolsPrimitiveDelete();
  afx_msg void OnPrimExtractNum();
  afx_msg void OnPrimExtractStr();
  /// <summary>Positions the cursor at a "control" point on the current engaged group.</summary>
  afx_msg void OnPrimGotoCenterPoint();
  /// <summary>Picks a primative and modifies its attributes to current settings.</summary>
  afx_msg void OnPrimModifyAttributes();

  /** @brief Snaps the cursor to the nearest endpoint of a primitive in the active view.
   * 
   * This function first retrieves the active view and transforms the current cursor position
   * into model coordinates. If a group is currently engaged, it attempts to pivot on the
   * control points of the engaged primitive. If successful, it updates the cursor position
   * to that control point. If not, it checks if the cursor is on any control point of the
   * engaged primitive and ignores it if so. If no group is engaged, it selects a segment
   * and primitive at the control point closest to the cursor position and updates the cursor
   * position accordingly. Finally, it resets any ignored primitive state.
   */
  afx_msg void OnToolsPrimitiveSnaptoendpoint();
  /// <summary>Reduces complex primitives and group references to a simpler form</summary>
  afx_msg void OnToolsGroupBreak();

  /** @brief Handles the deletion of a group in the active view.
    This function retrieves the current cursor position from the active view and attempts to select a group and its primitive at that position.
    If a group is found, it removes the group from any layer it belongs to, removes it from all views, and checks if it is trapped.
    If it is trapped, it updates the state information of the active view. The function then updates all views to reflect the deletion of the group and adds the deleted group to a list for potential restoration.
    Finally, it adds a message to inform the user that the segment has been deleted and can be restored.
  */
  afx_msg void OnToolsGroupDelete();
  afx_msg void OnToolsGroupDeletelast();
  /// <summary>Exchanges the first and last groups on the deleted group list.</summary>
  afx_msg void OnToolsGroupExchange();
  afx_msg void OnToolsGroupUndelete();
  afx_msg void OnSetupFillHatch();
  afx_msg void OnSetupFillHollow();
  afx_msg void OnSetupFillPattern();
  afx_msg void OnSetupFillSolid();
  afx_msg void OnSetupGotoPoint();
  afx_msg void OnSetupPointStyle();
  afx_msg void OnSetupNote();
  afx_msg void OnSetupOptionsDraw();
  afx_msg void OnSetupPenColor();
  afx_msg void OnSetupLineType();
  afx_msg void OnSetupSavePoint();
  afx_msg void OnTracingCloak();
  afx_msg void OnTracingFuse();
  afx_msg void OnTracingMap();
  afx_msg void OnTracingOpen();
  afx_msg void OnTracingView();
  afx_msg void OnTrapCommandsBlock();
  afx_msg void OnTrapCommandsCompress();
  afx_msg void OnTrapCommandsExpand();
  afx_msg void OnTrapCommandsFilter();
  afx_msg void OnTrapCommandsInvert();
  afx_msg void OnTrapCommandsQuery();
  afx_msg void OnTrapCommandsSquare();
  afx_msg void OnTrapCommandsUnblock();
  // Returns a pointer to the currently active document.
  static AeSysDoc* GetDoc();

 protected:
  DECLARE_MESSAGE_MAP()
 public:
  afx_msg void OnHelpKey();

  /** @brief Fuses a tracing layer into the main document converting it to a static layer.
   *
   * This method modifies the specified tracing layer by removing its tracing flags,
   * making it resident and internal, and setting its state to static. It also updates
   * the layer's name based on the provided file name.
   *
   * @param nameAndLocation A reference to a CString containing the name and location of the tracing layer file.
   * @note This is a cold state meaning the layer is displayed using warm color set, is not detectable, and may not have its groups modified or deleted. No new groups may be added to the layer.
   */
  void TracingFuse(CString& nameAndLocation);
  bool TracingLoadLayer(const CString& pathName, EoDbLayer* layer);

  /** @brief Maps a tracing file as a layer in the document.
   * @param pathName The file path of the tracing file to be mapped.
   * @return true if the tracing file was successfully mapped; false otherwise.
   * @note This method checks if the specified tracing file is already loaded as a layer. If it is, it prompts the user to close it first. If not, it attempts to load the tracing file into a new layer and adds it to the layer table. If successful, it sets the appropriate flags on the layer and updates all views to reflect the change.
   * This is a cold state meaning the tracing is displayed using warm color set, is not detectable,
   * and may not have its groups modified or deleted.
   * No new groups may be added to the tracing. Any number of tracings may be mapped.
   */
  bool TracingMap(const CString& pathName);
  bool TracingOpen(const CString& pathName);

  /** @brief Displays a tracing file as a layer in the current document.
   * This method checks if the specified tracing file is already loaded as a layer. 
   * If it is, it prompts the user to close it first. If not, it creates a new layer and loads the tracing file into it. The layer is then marked as mapped and the views are updated to display it.
   * @param pathName The path of the tracing file to be displayed.
   * @return true if the tracing file was successfully loaded and displayed; false otherwise.
   */
  bool TracingView(const CString& pathName);
};
