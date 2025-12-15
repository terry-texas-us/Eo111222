#pragma once

#include "EoDbLayer.h"
#include "EoDbLineTypeTable.h"
#include "EoDbMaskedPrimitive.h"

#if defined(USING_ODA)
#include "DbDatabase.h"
#endif  // USING_ODA

class AeSysDoc : public CDocument {
 protected:  // create from serialization only
  AeSysDoc();
  AeSysDoc(const AeSysDoc&) = delete;
  AeSysDoc& operator=(const AeSysDoc&) = delete;

  DECLARE_DYNCREATE(AeSysDoc)

 public:  // Attributes
#if defined(USING_ODA)
  OdDbDatabasePtr m_DatabasePtr;
  OdDb::DwgVersion m_SaveAsVersion;
#endif  // USING_ODA

 private:
  CString m_IdentifiedLayerName;
  EoDb::FileTypes m_SaveAsType;

  EoDbLineTypeTable m_LineTypeTable;
  EoDbLineType* m_ContinuousLineType;
  CBlocks m_BlocksTable;
  CLayers m_LayerTable;
  EoDbLayer* m_WorkLayer;
  EoDbGroupList m_DeletedGroupList;
  EoDbGroupList m_TrappedGroupList;
  EoGePoint3d m_TrapPivotPoint;

  EoDbGroupList m_NodalGroupList;
  CObList m_MaskedPrimitives;
  CObList m_UniquePoints;

  // Overrides
 public:
  virtual BOOL DoSave(LPCWSTR lpszPathName, BOOL bReplace = TRUE);
  virtual BOOL OnNewDocument();
  virtual BOOL OnOpenDocument(LPCWSTR lpszPathName);
  virtual BOOL OnSaveDocument(LPCWSTR lpszPathName);
  /// <summary>
  // Called by the framework to delete the document's data without destroying the CDocument object itself.
  // It is called just before the document is to be destroyed. It is also called to ensure that a document
  // is empty before it is reused. Call this function to implement an "Edit Clear All" or similar command
  // that deletes all of the document's data.
  /// </summary>
  virtual void DeleteContents();

  // Implementation
 public:
  virtual ~AeSysDoc();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

  // Operations
 public:
  void InitializeGroupAndPrimitiveEdit();

  /// <summary>Constructs 0 to many seperate text primitives for each "\r\n" delimited substr.</summary>
  void AddTextBlock(LPWSTR pszText);

  // Block Table interface
  CBlocks* BlocksTable() { return (&m_BlocksTable); }
  bool BlockTableIsEmpty() { return m_BlocksTable.IsEmpty() == TRUE; }
  EoUInt16 BlockTableSize() { return (EoUInt16(m_BlocksTable.GetSize())); }
  int GetBlockReferenceCount(const CString& name);
  POSITION GetFirstBlockPosition() { return m_BlocksTable.GetStartPosition(); }
  void GetNextBlock(POSITION& position, CString& name, EoDbBlock*& block) {
    m_BlocksTable.GetNextAssoc(position, name, block);
  }
  bool LookupBlock(CString name, EoDbBlock*& block);
  /// <summary>Removes all blocks and defining primitives.</summary>
  void RemoveAllBlocks();
  /// <summary>Removes all blocks which have no references.</summary>
  void RemoveUnusedBlocks();
  void InsertBlock(const CString& name, EoDbBlock* block) { m_BlocksTable.SetAt(name, block); }
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

  // Line Type Table interface
  EoDbLineTypeTable* LineTypeTable() { return &m_LineTypeTable; }
  EoDbLineType* ContinuousLineType() { return m_ContinuousLineType; }
  /// <summary>Loads the Line Type table.</summary>
  void LoadLineTypesFromXmlFile(const CString& pathName);

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
  POSITION FindWorkLayerGroup(EoDbGroup* group) const { return (m_WorkLayer->Find(group)); }
  POSITION GetFirstWorkLayerGroupPosition() const { return m_WorkLayer->GetHeadPosition(); }
  EoDbGroup* GetLastWorkLayerGroup() const;
  POSITION GetLastWorkLayerGroupPosition() const { return m_WorkLayer->GetTailPosition(); }
  EoDbGroup* GetNextWorkLayerGroup(POSITION& position) const { return m_WorkLayer->GetNext(position); }
  EoDbGroup* GetPreviousWorkLayerGroup(POSITION& position) const { return m_WorkLayer->GetPrev(position); }
  EoDbLayer* GetWorkLayer() const { return m_WorkLayer; }
  void InitializeWorkLayer();
  EoDbLayer* SetWorkLayer(EoDbLayer* layer);

  void WriteShadowFile();

  // Deleted groups interface
  POSITION DeletedGroupsAddHead(EoDbGroup* group) { return (m_DeletedGroupList.AddHead(group)); }
  POSITION DeletedGroupsAddTail(EoDbGroup* group) { return (m_DeletedGroupList.AddTail(group)); }
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
  /// <summary>The current trap is copied to the clipboard. This is done with two independent clipboard formats. The standard enhanced metafile and the private EoDbGroupList which is read exclusively by Peg.</summary>
  void CopyTrappedGroupsToClipboard(AeSysView* view);
  /// <summary>Expands compressed groups.</summary>
  // The new groups are added to the hot layer even if the trap contained
  // groups from one or more warm layers.
  void ExpandTrappedGroups();
  POSITION FindTrappedGroup(EoDbGroup* group) { return m_TrappedGroupList.Find(group); }
  POSITION GetFirstTrappedGroupPosition() const { return m_TrappedGroupList.GetHeadPosition(); }
  EoDbGroup* GetNextTrappedGroup(POSITION& position) { return m_TrappedGroupList.GetNext(position); }
  EoGePoint3d GetTrapPivotPoint() { return m_TrapPivotPoint; }
  BOOL IsTrapEmpty() const { return m_TrappedGroupList.IsEmpty(); }
  void ModifyTrappedGroupsPenColor(EoInt16 penColor) { m_TrappedGroupList.ModifyPenColor(penColor); }
  void ModifyTrappedGroupsLineType(EoInt16 lineType) { m_TrappedGroupList.ModifyLineType(lineType); }
  void ModifyTrappedGroupsNoteAttributes(EoDbFontDefinition& fontDef, EoDbCharacterCellDefinition& cellDef,
                                         int attributes);
  void RemoveAllTrappedGroups();
  EoDbGroup* RemoveLastTrappedGroup() { return m_TrappedGroupList.RemoveTail(); }
  POSITION RemoveTrappedGroup(EoDbGroup* group) { return m_TrappedGroupList.Remove(group); }
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

  POSITION AddNodalGroup(EoDbGroup* group) { return m_NodalGroupList.AddTail(group); }
  POSITION FindNodalGroup(EoDbGroup* group) { return m_NodalGroupList.Find(group); }
  POSITION GetFirstNodalGroupPosition() const { return m_NodalGroupList.GetHeadPosition(); }
  EoDbGroup* GetNextNodalGroup(POSITION& position) { return m_NodalGroupList.GetNext(position); }
  void RemoveAllNodalGroups() { m_NodalGroupList.RemoveAll(); }
  POSITION AddMaskedPrimitive(EoDbMaskedPrimitive* maskedPrimitive) {
    return m_MaskedPrimitives.AddTail((CObject*)maskedPrimitive);
  }
  POSITION GetFirstMaskedPrimitivePosition() const { return m_MaskedPrimitives.GetHeadPosition(); }
  EoDbMaskedPrimitive* GetNextMaskedPrimitive(POSITION& position) {
    return (EoDbMaskedPrimitive*)m_MaskedPrimitives.GetNext(position);
  }
  void RemoveAllMaskedPrimitives() { m_MaskedPrimitives.RemoveAll(); }
  DWORD GetPrimitiveMask(EoDbPrimitive* primitive);
  void AddPrimitiveBit(EoDbPrimitive* primitive, int bit);
  void RemovePrimitiveBit(EoDbPrimitive* primitive, int bit);

  int AddUniquePoint(const EoGePoint3d& point);
  POSITION AddUniquePoint(EoGeUniquePoint* uniquePoint) { return m_UniquePoints.AddTail((CObject*)uniquePoint); }
  void DisplayUniquePoints();
  POSITION GetFirstUniquePointPosition() const { return m_UniquePoints.GetHeadPosition(); }
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
  /// <summary>Pastes clipboard to drawing. If the clipboard has the EoGroups format, all other formats are ignored.</summary>
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
  /// <summary>
  ///All layers are made active with the exception of the current working layer.
  ///This is a warm state meaning the layer is displayed using hot color set, is detectable,
  ///and may have its groups modified or deleted. No new groups are added to an active layer.
  ///Zero or more layers may be active.
  /// </summary>
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
  afx_msg void OnToolsPrimitiveSnaptoendpoint();
  /// <summary>Reduces complex primitives and group references to a simpler form</summary>
  afx_msg void OnToolsGroupBreak();
  /// <summary>
  /// Searches for closest detectible group.  If found, group is removed
  /// from all general group lists and added to deleted group list.
  /// Notes: The group resources are not freed.
  /// </summary>
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

  /// <summary>Tracing is converted to a static layer.</summary>
  /// <remarks>
  ///This is a cold state meaning the layer is displayed using warm color set,
  ///is not detectable, and may not have its groups modified or deleted.
  /// No new groups may be added to the layer.
  /// </remarks>
  void TracingFuse(CString& nameAndLocation);
  bool TracingLoadLayer(const CString& pathName, EoDbLayer* layer);
  /// <summary>Selected tracing is mapped.</summary>
  /// <remarks>
  /// This is a cold state meaning the tracing is displayed using warm color set, is not detectable,
  /// and may not have its groups modified or deleted.
  /// No new groups may be added to the tracing. Any number of tracings may be mapped.
  /// </remarks>
  bool TracingMap(const CString& pathName);
  bool TracingOpen(const CString& pathName);
  bool TracingView(const CString& pathName);

 public:
#if defined(USING_ODA)
  void ConvertPegDocument();

  void ConvertHeaderSection();
  void ConvertViewportTable();
  void ConvertLinetypesTable();
  void ConvertLinetypesTableRecord(EoDbLineType* lineType);
  void ConvertLayerTable();
  void ConvertLayerTableRecord(EoDbLayer* layer);

  void ConvertBlockTable();

  void ConvertGroupsInLayers();
  void ConvertGroup(EoDbGroup* group, const OdDbObjectId& modelSpace);
  void ConvertGroupsInBlocks();

  afx_msg void OnSetupLayerproperties();
  BOOL DoPromptFileName(CString& fileName, UINT titleResourceIdentifier, DWORD flags);
#endif  // USING_ODA
};
