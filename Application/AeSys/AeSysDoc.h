#pragma once

#include <unordered_map>
#include <variant>

#include "EoDb.h"
#include "EoDbAppIdEntry.h"
#include "EoDbBlock.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbClassEntry.h"
#include "EoDbDimStyle.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbHandleManager.h"
#include "EoDbHeaderSection.h"
#include "EoDbLayer.h"
#include "EoDbLineType.h"
#include "EoDbLineTypeTable.h"
#include "EoDbMaskedPrimitive.h"
#include "EoDbPrimitive.h"
#include "EoDbTextStyle.h"
#include "EoDbVPortTableEntry.h"
#include "EoDxfBase.h"
#include "EoDxfObjects.h"
#include "EoGePoint3d.h"
#include "EoGeUniquePoint.h"
#include "EoGeVector3d.h"

class EoDbViewport;
class EoGeTransformMatrix;

/// @brief Tagged union of all handle-bearing object types for unified handle graph lookup.
///
/// DXF handles are globally unique integers. A single map keyed by handle can resolve
/// any handle to its owning object regardless of type. The four variant alternatives
/// cover all heap-allocated, pointer-stable handle-bearing types in the document.
using HandleObject = std::variant<EoDbPrimitive*, EoDbLayer*, EoDbLineType*, EoDbBlock*>;

class AeSysDoc : public CDocument {
 protected:
  AeSysDoc();
  AeSysDoc(const AeSysDoc&) = delete;
  AeSysDoc& operator=(const AeSysDoc&) = delete;

  DECLARE_DYNCREATE(AeSysDoc)

 private:
  EoDbHeaderSection m_HeaderSection{};
  EoDbLineTypeTable m_LineTypeTable{};
  std::vector<EoDbAppIdEntry> m_appIdTable{};
  std::vector<EoDbClassEntry> m_classTable{};
  std::vector<EoDbDimStyle> m_dimStyleTable{};
  std::vector<EoDbTextStyle> m_textStyleTable{};
  std::vector<EoDxfUnsupportedObject> m_unsupportedObjects{};
  std::vector<EoDxfLayout> m_layouts{};
  std::vector<EoDbVPortTableEntry> m_vportTable{};
  EoDbBlocks m_BlocksTable{};
  EoDbGroupList m_DeletedGroupList{};
  EoDbGroupList m_trappedGroups{};
  EoDbGroupList m_NodalGroupList{};
  CObList m_MaskedPrimitives{};
  CObList m_UniquePoints{};
  CLayers m_modelSpaceLayers{};
  std::unordered_map<std::uint64_t, CLayers> m_paperSpaceLayoutLayers{};
  std::uint64_t m_activeLayoutHandle{EoDxf::Handles::PaperSpaceBlockRecord};
  EoDxf::Space m_activeSpace{EoDxf::Space::ModelSpace};
  EoGePoint3d m_trapPivotPoint{};
  EoDbLineType* m_continuousLineType{};
  EoDbLayer* m_workLayer{};
  CString m_IdentifiedLayerName{};
  double m_pointSize{};  // in drawing units when greater than zero; in pixels when less than zero; default otherwise
  EoDb::FileTypes m_saveAsType{EoDb::FileTypes::Unknown};
  EoDxf::Version m_dxfVersion{EoDxf::Version::AC1032};
  std::wstring m_originalDwgPath{};  ///< Original .dwg path when opened via ODAFileConverter; empty otherwise
  EoDbHandleManager m_handleManager{};
  std::unordered_map<std::uint64_t, HandleObject> m_handleMap{};

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

  /** @brief Provides access to the document's handle manager.
   *
   * @return A reference to the EoDbHandleManager used to assign and accommodate entity handles.
   */
  [[nodiscard]] EoDbHandleManager& HandleManager() noexcept { return m_handleManager; }
  [[nodiscard]] const EoDbHandleManager& HandleManager() const noexcept { return m_handleManager; }

  // Handle → Object reverse lookup map

  /** @brief Registers a primitive's handle in the lookup map.
   * Skips zero-handle primitives (no handle assigned yet).
   * @param primitive Non-null pointer to the primitive to register.
   */
  void RegisterHandle(EoDbPrimitive* primitive);

  /** @brief Registers a layer's handle in the lookup map.
   * Skips zero-handle layers.
   * @param layer Non-null pointer to the layer to register.
   */
  void RegisterHandle(EoDbLayer* layer);

  /** @brief Registers a linetype's handle in the lookup map.
   * Skips zero-handle linetypes.
   * @param lineType Non-null pointer to the linetype to register.
   */
  void RegisterHandle(EoDbLineType* lineType);

  /** @brief Registers a block's handle in the lookup map.
   * Skips zero-handle blocks.
   * @param block Non-null pointer to the block to register.
   */
  void RegisterHandle(EoDbBlock* block);

  /** @brief Removes a handle from the lookup map.
   * @param handle The handle to unregister. Zero handles are silently ignored.
   */
  void UnregisterHandle(std::uint64_t handle) noexcept;

  /** @brief Finds any handle-bearing object by its handle.
   * @param handle The handle to search for.
   * @return Pointer to the HandleObject variant, or nullptr if not found.
   */
  [[nodiscard]] const HandleObject* FindObjectByHandle(std::uint64_t handle) const noexcept;

  /** @brief Finds a primitive by its handle.
   * @param handle The handle to search for.
   * @return Pointer to the primitive, or nullptr if the handle is absent or maps to a non-primitive object.
   */
  [[nodiscard]] EoDbPrimitive* FindPrimitiveByHandle(std::uint64_t handle) const noexcept;

  /** @brief Finds a layer by its handle.
   * @param handle The handle to search for.
   * @return Pointer to the layer, or nullptr if the handle is absent or maps to a non-layer object.
   */
  [[nodiscard]] EoDbLayer* FindLayerByHandle(std::uint64_t handle) const noexcept;

  /** @brief Finds a linetype by its handle.
   * @param handle The handle to search for.
   * @return Pointer to the linetype, or nullptr if the handle is absent or maps to a non-linetype object.
   */
  [[nodiscard]] EoDbLineType* FindLineTypeByHandle(std::uint64_t handle) const noexcept;

  /** @brief Finds a block by its handle.
   * @param handle The handle to search for.
   * @return Pointer to the block, or nullptr if the handle is absent or maps to a non-block object.
   */
  [[nodiscard]] EoDbBlock* FindBlockByHandle(std::uint64_t handle) const noexcept;

  /** @brief Registers all primitive handles in a group.
   * @param group Non-null pointer to the group whose primitives are registered.
   */
  void RegisterGroupHandles(EoDbGroup* group);

  /** @brief Unregisters all primitive handles in a group.
   * @param group Non-null pointer to the group whose primitives are unregistered.
   */
  void UnregisterGroupHandles(EoDbGroup* group);

  /** @brief Adds text to the document
   *
   * The input text is expected to be a single string with lines separated by "\r\n".
   * Each line of text is added as a separate EoDbText primitive to the document.
   * The position of the first line of text is determined by the current cursor position in the application,
   * and subsequent lines are positioned based on the font definition and character cell settings.
   *
   * @param textBlock A pointer to a wide string containing the text to add. Lines should be separated by "\r\n".
   */
  void AddTextBlock(LPWSTR textBlock);

  // Block Table interface
  [[nodiscard]] EoDbBlocks* BlocksTable() { return (&m_BlocksTable); }
  [[nodiscard]] bool BlockTableIsEmpty() const { return m_BlocksTable.IsEmpty() == TRUE; }
  [[nodiscard]] std::uint16_t BlockTableSize() const { return (std::uint16_t(m_BlocksTable.GetSize())); }
  int GetBlockReferenceCount(const CString& name);
  [[nodiscard]] auto GetFirstBlockPosition() const { return m_BlocksTable.GetStartPosition(); }
  void GetNextBlock(POSITION& position, CString& name, EoDbBlock*& block) {
    m_BlocksTable.GetNextAssoc(position, name, block);
  }
  bool LookupBlock(CString name, EoDbBlock*& block);

  /** @brief Renames a block in the block table and patches all block references that use the old name.
   *
   * @param oldName The current name of the block.
   * @param newName The new name to assign. Must be non-empty and not already in use.
   * @return true on success; false if oldName does not exist or newName is invalid/already taken.
   */
  bool RenameBlock(const CString& oldName, const CString& newName);

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
      ATLTRACE2(traceGeneral, 3, L"InsertBlock: Replacing existing block '%s'\n", name.GetString());
      UnregisterHandle(existing->Handle());
      UnregisterGroupHandles(existing);
      existing->DeletePrimitivesAndRemoveAll();
      delete existing;
    }
    m_BlocksTable.SetAt(name, block);
    RegisterHandle(block);
  }

  // Editor Mode — unified state for Block Editor and Tracing Editor
  enum class EditorMode { None, Block, Tracing };

  /// @brief True when the user is editing a block definition in-place.
  [[nodiscard]] bool IsEditingBlock() const noexcept { return m_editorMode == EditorMode::Block; }
  [[nodiscard]] bool IsEditingTracing() const noexcept { return m_editorMode == EditorMode::Tracing; }
  [[nodiscard]] bool IsInEditor() const noexcept { return m_editorMode != EditorMode::None; }
  [[nodiscard]] EditorMode CurrentEditorMode() const noexcept { return m_editorMode; }
  [[nodiscard]] const CString& EditingBlockName() const noexcept { return m_editingBlockName; }
  bool EnterBlockEditMode(const CString& blockName);
  void ExitBlockEditMode(bool commit);

  // Tracing Editor
  [[nodiscard]] const CString& EditingTracingPath() const noexcept { return m_editingTracingPath; }
  [[nodiscard]] bool IsTracingSession() const noexcept { return m_isTracingSession; }
  [[nodiscard]] bool IsClosing() const noexcept { return m_isClosing; }
  [[nodiscard]] bool IsTracingEditDirty() const noexcept { return m_tracingEditDirty; }
  void SetTracingEditDirty(bool dirty) noexcept { m_tracingEditDirty = dirty; }
  bool EnterTracingEditMode(const CString& pathName);
  bool EnterEmbeddedTracingEditMode(EoDbLayer* tracingLayer);
  void ExitTracingEditMode(bool commit);
  bool SaveTracingLayerToFile(const CString& filePath, EoDbLayer* layer);
  void OnToolsSaveTracingEdit();
  void OnToolsSaveAsTracingEdit();
  void OnToolsExitTracingEdit();
  void OnToolsCancelTracingEdit();
  void InsertTracingLayer(const std::wstring& absolutePath);
  void ReloadTracingLayer(EoDbLayer* layer);

 private:
  EditorMode m_editorMode{EditorMode::None};
  bool m_isTracingSession{};  ///< True when this document was created solely to edit a .tra file
  bool m_isClosing{};  ///< True during document teardown to suppress paint/update cycles

  // Block Editor state
  CString m_editingBlockName;
  EoDbBlock* m_editingBlock{};  ///< The block being edited (original in block table)
  EoDbLayer* m_blockEditLayer{};  ///< Temporary layer holding editable copies of block primitives

  // Tracing Editor state
  CString m_editingTracingPath;  ///< Full path of the .tra file being edited
  bool m_tracingEditDirty{};  ///< True when unsaved edits exist in the current tracing editor session
  EoDbLayer* m_tracingEditLayer{};  ///< Temporary layer holding editable copies of tracing primitives (solo mode)
  EoDbLayer* m_embeddedTracingLayer{};  ///< The |name layer being edited in embedded mode (not a temp copy)

  // Shared editor state
  EoDbLayer* m_savedEditorWorkLayer{};  ///< Saved work layer to restore on exit
  EoDxf::Space m_savedEditorSpace{EoDxf::Space::ModelSpace};  ///< Saved active space to restore on exit
  CString m_savedEditorTitle;  ///< Saved document title to restore on exit
  std::vector<std::pair<EoDbPrimitive*, EoDbGroup*>> m_blockEditSnapshot{};  ///< Snapshot for cancel
  std::vector<std::pair<EoDbLayer*, std::uint16_t>> m_savedTracingLayerVisibility{};  ///< Layers hidden during tracing ed

 public:
  void LayerBlank(const CString& strName);
  /// @brief A layer is converted to a tracing or a job file
  bool LayerMelt(CString& strName);

  void GetExtents(AeSysView* view, EoGePoint3d&, EoGePoint3d&, EoGeTransformMatrix&);
  int NumberOfGroupsInWorkLayer();
  int NumberOfGroupsInActiveLayers();
  /// @brief Displays drawing and determines which groups are detectable.
  void DisplayAllLayers(AeSysView* view, EoGsRenderDevice* renderDevice);

  /** @brief Draws the paper-space sheet background and viewport boundary outlines.
   *
   *  When in paper space, this method renders a light gray filled rectangle
   *  representing the layout sheet area, and thin outlined rectangles for each
   *  viewport boundary.  This provides a visual reference for the relative
   *  positioning of viewports on the sheet — useful for debugging and for
   *  understanding multi-viewport layouts.
   *
   *  The sheet rectangle is derived from the active EoDxfLayout extents when
   *  available (DXF-sourced files), or from the bounding box of all viewports
   *  with a small margin (PEG files).
   *
   *  Called from DisplayAllLayers() before rendering paper-space layer content.
   *
   *  @param view  The active AeSysView providing transform and projection services.
   *  @param renderDevice  The render device to draw into.
   */
  void DisplayPaperSpaceSheet(AeSysView* view, EoGsRenderDevice* renderDevice);

  /** @brief Dims paper-space content when a viewport is active.
   *
   *  Draws a semi-transparent overlay across the entire visible area using
   *  GdiAlphaBlend, reducing the visual prominence of paper-space entities
   *  so that model-space content rendered through the active viewport stands out.
   *  No-op when no viewport is active or when the render device has no GDI DC.
   *
   *  @param view  The active AeSysView (checked for IsViewportActive()).
   *  @param renderDevice  The render device to draw the overlay into.
   */
  void DimPaperSpaceOverlay(AeSysView* view, EoGsRenderDevice* renderDevice);

  /** @brief Finds the paper-space viewport primitive containing the given world-space point.
   *
   *  Walks the active paper-space layout's layers, searching for EoDbViewport
   *  primitives (ID >= 2 with valid dimensions) whose rectangular boundary
   *  contains the specified point.
   *
   *  @param worldPoint  A point in paper-space world coordinates.
   *  @return Pointer to the viewport primitive containing the point, or nullptr if none.
   */
  [[nodiscard]] EoDbViewport* HitTestViewport(const EoGePoint3d& worldPoint);

  /** @brief Finds the first valid paper-space viewport in the active layout.
   *
   *  Walks the active paper-space layout's layers, returning the first EoDbViewport
   *  primitive with ID >= 2 and valid dimensions. Used when the tab bar needs to
   *  activate a viewport without a specific hit-test point.
   *
   *  @return Pointer to the first viewport primitive, or nullptr if none found.
   */
  [[nodiscard]] EoDbViewport* FindFirstViewport();

  /** @brief Renders model-space entities through each paper-space viewport.
   *
   *  For each EoDbViewport primitive found in paper-space layers (excluding the
   *  overall paper-space viewport, id 1, and viewports with no model-space view),
   *  this method:
   *  1. Computes the viewport boundary in device coordinates and applies a GDI clip rectangle.
   *  2. Saves and reconfigures the view transform to match the viewport's model-space view parameters.
   *  3. Renders all model-space layers through that transform.
   *  4. Restores the view transform and removes the clip.
   *
   *  @param view  The active AeSysView providing transform and projection services.
   *  @param renderDevice  The render device to render into.
   */
  void DisplayModelSpaceThroughViewports(AeSysView* view, EoGsRenderDevice* renderDevice);

  /** @brief Creates a default paper-space viewport if none exists.
   *
   *  When a PEG file (or any document) has model-space content but no paper-space
   *  viewport primitives with a valid model-space view (ID > 1, viewHeight > 0),
   *  this method creates one. The viewport is sized to encompass the full
   *  model-space extents at 1:1 scale, producing a layout equivalent to the
   *  DXF paper-space structure. This unifies the plot pipeline: both DXF and PEG
   *  documents go through the same paper-space → viewport → model-space rendering.
   *
   *  @param view  The active AeSysView (needed for model-space extent computation).
   */
  void CreateDefaultPaperSpaceViewport(AeSysView* view);

  /** @brief Renders model-space layers directly (bypasses active-space routing).
   *
   *  Used internally by DisplayModelSpaceThroughViewports to draw model-space content
   *  through each viewport's clipped/transformed window.  Does not register groups
   *  for detectability or highlight trapped groups.
   *
   *  @param view  The active AeSysView.
   *  @param renderDevice  The render device to render into.
   */
  void DisplayModelSpaceLayers(AeSysView* view, EoGsRenderDevice* renderDevice);

  // Layer Table interface

  /// @brief Returns the layer table for the active space (model or active paper-space layout).
  [[nodiscard]] CLayers& ActiveSpaceLayers();
  [[nodiscard]] const CLayers& ActiveSpaceLayers() const;

  /// @brief Returns the layer table for the specified space (paper-space routes through active layout).
  [[nodiscard]] CLayers& SpaceLayers(EoDxf::Space space);

  /// @brief Returns the active drawing space.
  [[nodiscard]] EoDxf::Space ActiveSpace() const noexcept { return m_activeSpace; }

  /// @brief Sets the active drawing space (model or paper).
  void SetActiveSpace(EoDxf::Space space) noexcept { m_activeSpace = space; }

  /// @brief Returns the original .dwg path when the document was opened via ODAFileConverter.
  [[nodiscard]] const std::wstring& OriginalDwgPath() const noexcept { return m_originalDwgPath; }

  /** @brief Retrieves a layer from the active space layer table by name.
   * This method searches the layer table for a layer with the specified name and returns a pointer to it.
   * If no such layer exists, it returns nullptr.
   * @param name The name of the layer to retrieve.
   * @return A pointer to the EoDbLayer with the specified name, or nullptr if not found.
   */
  [[nodiscard]] EoDbLayer* GetLayerTableLayer(const CString& name);
  [[nodiscard]] EoDbLayer* GetLayerTableLayerAt(int layer);
  [[nodiscard]] int GetLayerTableSize() const;

  /** @brief Finds the index of a layer in the active space layer table by its name.
   * This method performs a case-insensitive search for a layer with the specified name in the layer table.
   * @param name The name of the layer to find.
   * @return The index of the layer if found; otherwise, -1.
   */
  [[nodiscard]] int FindLayerTableLayer(const CString& name) const;
  void RemoveLayerTableLayer(const CString& strName);
  void RemoveAllLayerTableLayers();
  void RemoveLayerTableLayerAt(int);
  void AddLayerTableLayer(EoDbLayer* layer);
  void RemoveEmptyLayers();

  /// @brief Adds a layer to a specific space's layer table (paper-space routes through active layout).
  void AddLayerToSpace(EoDbLayer* layer, EoDxf::Space space);

  /// @brief Adds a layer to a specific paper-space layout by block record handle.
  void AddLayerToLayout(EoDbLayer* layer, std::uint64_t blockRecordHandle);

  /** @brief Finds a layer by name in a specific space's layer table.
   * @param name The layer name to search for.
   * @param space The target space (ModelSpace or PaperSpace — paper routes through active layout).
   * @return Pointer to the layer if found; nullptr otherwise.
   */
  [[nodiscard]] EoDbLayer* FindLayerInSpace(const CString& name, EoDxf::Space space);

  /// @brief Finds a layer by name in a specific paper-space layout.
  [[nodiscard]] EoDbLayer* FindLayerInLayout(const CString& name, std::uint64_t blockRecordHandle);

  /// @brief Returns the active paper-space layout's layer collection.
  CLayers& PaperSpaceLayers() { return m_paperSpaceLayoutLayers[m_activeLayoutHandle]; }
  const CLayers& PaperSpaceLayers() const;

  /// @brief Returns the active paper-space layout's block record handle.
  [[nodiscard]] std::uint64_t ActiveLayoutHandle() const noexcept { return m_activeLayoutHandle; }

  /// @brief Sets the active paper-space layout by block record handle.
  void SetActiveLayoutHandle(std::uint64_t blockRecordHandle) noexcept { m_activeLayoutHandle = blockRecordHandle; }

  /// @brief Returns the full map of paper-space layout layer collections.
  [[nodiscard]] auto& PaperSpaceLayoutLayers() noexcept { return m_paperSpaceLayoutLayers; }
  [[nodiscard]] const auto& PaperSpaceLayoutLayers() const noexcept { return m_paperSpaceLayoutLayers; }

  /// @brief Returns the layer collection for a specific paper-space layout by block record handle.
  CLayers& LayoutLayers(std::uint64_t blockRecordHandle) { return m_paperSpaceLayoutLayers[blockRecordHandle]; }

  EoDbLayer* LayersSelUsingPoint(const EoGePoint3d&);

  /// Get the stored point size. Positive => drawing units; negative => pixels.
  double GetPointSize() const noexcept { return m_pointSize; }
  void SetPointSize(double size) noexcept { m_pointSize = size; }

  // Line Type Table interface
  [[nodiscard]] auto* LineTypeTable() { return &m_LineTypeTable; }
  [[nodiscard]] auto* ContinuousLineType() { return m_continuousLineType; }

  // Class Table interface (CLASSES section passthrough)
  [[nodiscard]] auto& ClassTable() noexcept { return m_classTable; }
  [[nodiscard]] const auto& ClassTable() const noexcept { return m_classTable; }
  void AddClassEntry(const EoDbClassEntry& entry) { m_classTable.push_back(entry); }
  void AddClassEntry(EoDbClassEntry&& entry) { m_classTable.push_back(std::move(entry)); }
  void ClearClassTable() noexcept { m_classTable.clear(); }

  // Dimension Style Table interface
  [[nodiscard]] auto& DimStyleTable() noexcept { return m_dimStyleTable; }
  [[nodiscard]] const auto& DimStyleTable() const noexcept { return m_dimStyleTable; }
  void AddDimStyleEntry(const EoDbDimStyle& entry) { m_dimStyleTable.push_back(entry); }
  void AddDimStyleEntry(EoDbDimStyle&& entry) { m_dimStyleTable.push_back(std::move(entry)); }
  void ClearDimStyleTable() noexcept { m_dimStyleTable.clear(); }

  /// @brief Finds a dimension style by name (case-insensitive).
  /// @return Pointer to the matching entry, or nullptr if not found.
  [[nodiscard]] const EoDbDimStyle* FindDimStyle(const std::wstring& name) const noexcept {
    for (const auto& style : m_dimStyleTable) {
      if (_wcsicmp(style.m_name.c_str(), name.c_str()) == 0) { return &style; }
    }
    return nullptr;
  }

  // Text Style Table interface
  [[nodiscard]] auto& TextStyleTable() noexcept { return m_textStyleTable; }
  [[nodiscard]] const auto& TextStyleTable() const noexcept { return m_textStyleTable; }
  void AddTextStyleEntry(const EoDbTextStyle& entry) { m_textStyleTable.push_back(entry); }
  void AddTextStyleEntry(EoDbTextStyle&& entry) { m_textStyleTable.push_back(std::move(entry)); }
  void ClearTextStyleTable() noexcept { m_textStyleTable.clear(); }

  /// @brief Finds a text style by name (case-insensitive).
  /// @return Pointer to the matching entry, or nullptr if not found.
  [[nodiscard]] const EoDbTextStyle* FindTextStyle(const std::wstring& name) const noexcept {
    for (const auto& style : m_textStyleTable) {
      if (_wcsicmp(style.m_name.c_str(), name.c_str()) == 0) { return &style; }
    }
    return nullptr;
  }

  // Viewport Table interface (AE2026)
  [[nodiscard]] auto& VPortTable() noexcept { return m_vportTable; }
  [[nodiscard]] const auto& VPortTable() const noexcept { return m_vportTable; }
  void AddVPortTableEntry(const EoDbVPortTableEntry& entry) { m_vportTable.push_back(entry); }
  void AddVPortTableEntry(EoDbVPortTableEntry&& entry) { m_vportTable.push_back(std::move(entry)); }
  void ClearVPortTable() noexcept { m_vportTable.clear(); }

  /// @brief Finds the *ACTIVE VPORT table entry (case-insensitive).
  /// @return Pointer to the active entry, or nullptr if not found.
  [[nodiscard]] const EoDbVPortTableEntry* FindActiveVPort() const noexcept {
    for (const auto& entry : m_vportTable) {
      if (_wcsicmp(entry.m_name.c_str(), L"*ACTIVE") == 0) { return &entry; }
    }
    return nullptr;
  }

  // AppId Table interface (round-trip passthrough)
  [[nodiscard]] auto& AppIdTable() noexcept { return m_appIdTable; }
  [[nodiscard]] const auto& AppIdTable() const noexcept { return m_appIdTable; }
  void AddAppIdEntry(const EoDbAppIdEntry& entry) { m_appIdTable.push_back(entry); }
  void AddAppIdEntry(EoDbAppIdEntry&& entry) { m_appIdTable.push_back(std::move(entry)); }
  void ClearAppIdTable() noexcept { m_appIdTable.clear(); }

  // Unsupported Objects interface (round-trip passthrough)
  [[nodiscard]] auto& UnsupportedObjects() noexcept { return m_unsupportedObjects; }
  [[nodiscard]] const auto& UnsupportedObjects() const noexcept { return m_unsupportedObjects; }
  void AddUnsupportedObject(const EoDxfUnsupportedObject& object) { m_unsupportedObjects.push_back(object); }
  void AddUnsupportedObject(EoDxfUnsupportedObject&& object) { m_unsupportedObjects.push_back(std::move(object)); }
  void ClearUnsupportedObjects() noexcept { m_unsupportedObjects.clear(); }

  /** @brief Resolves dynamic block INSERT entities to their correct anonymous block definitions.
   *
   *  Dynamic blocks in DXF use an extension dictionary chain:
   *  INSERT → extension dictionary (DICTIONARY) → ACDB_BLOCKREPRESENTATION_DATA → group code 340 →
   *  anonymous block BLOCK_RECORD handle → block name.
   *
   *  This method walks that chain for every INSERT primitive with a non-zero extension dictionary handle,
   *  updating the INSERT's block name to the resolved anonymous block. Must be called after DXF import
   *  completes and before the document is rendered.
   */
  void ResolveDynamicBlockReferences();

  // Layout storage (structured DXF LAYOUT objects)
  [[nodiscard]] auto& Layouts() noexcept { return m_layouts; }
  [[nodiscard]] const auto& Layouts() const noexcept { return m_layouts; }
  void AddLayout(const EoDxfLayout& layout) { m_layouts.push_back(layout); }
  void AddLayout(EoDxfLayout&& layout) { m_layouts.push_back(std::move(layout)); }
  void ClearLayouts() noexcept { m_layouts.clear(); }

  void PenTranslation(std::uint16_t, std::int16_t*, std::int16_t*);

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
  auto FindWorkLayerGroup(EoDbGroup* group) const {
    return (m_workLayer != nullptr ? m_workLayer->Find(group) : nullptr);
  }
  [[nodiscard]] auto GetFirstWorkLayerGroupPosition() const {
    return (m_workLayer != nullptr ? m_workLayer->GetHeadPosition() : POSITION{});
  }

  /** @brief Retrieves the last group in the work layer.
   * @return Pointer to the last EoDbGroup in the work layer, or nullptr if the work layer is not defined or contains no
   * groups.
   */
  [[nodiscard]] EoDbGroup* GetLastWorkLayerGroup() const;
  [[nodiscard]] auto GetLastWorkLayerGroupPosition() const {
    return (m_workLayer != nullptr ? m_workLayer->GetTailPosition() : POSITION{});
  }
  EoDbGroup* GetNextWorkLayerGroup(POSITION& position) const {
    return (m_workLayer != nullptr ? m_workLayer->GetNext(position) : nullptr);
  }
  EoDbGroup* GetPreviousWorkLayerGroup(POSITION& position) const {
    return (m_workLayer != nullptr ? m_workLayer->GetPrev(position) : nullptr);
  }
  [[nodiscard]] auto* GetWorkLayer() const { return m_workLayer; }
  void InitializeWorkLayer();
  EoDbLayer* SetWorkLayer(EoDbLayer* layer);

  void WriteShadowFile(EoDb::PegFileVersion fileVerion);

  // Deleted groups interface
  auto DeletedGroupsAddHead(EoDbGroup* group) { return (m_DeletedGroupList.AddHead(group)); }
  auto DeletedGroupsAddTail(EoDbGroup* group) { return (m_DeletedGroupList.AddTail(group)); }
  bool DeletedGroupsIsEmpty() { return (m_DeletedGroupList.IsEmpty() == TRUE); }
  // CobList asserts on calls to RemoveHead & RemoveTail if list is empty!
  EoDbGroup* DeletedGroupsRemoveHead() { return (m_DeletedGroupList.RemoveHead()); }
  void DeletedGroupsRemoveGroups() { m_DeletedGroupList.DeleteGroupsAndRemoveAll(); }
  EoDbGroup* DeletedGroupsRemoveTail() { return (m_DeletedGroupList.RemoveTail()); }
  /// @brief Restores the last group added to the deleted group list.
  void DeletedGroupsRestore();

 public:  // trap interface
  void AddGroupsToTrap(EoDbGroupList* groups);
  POSITION AddGroupToTrap(EoDbGroup* group);

  /** @brief Combines multiple groups in the trap into a single group.
   *
   * This method creates a new group and adds all primitives from the existing trapped groups to it.
   * The original groups are removed from the document and deleted, while the new combined group is added to the work
   * layer and the trapped group list. The new group is added to the hot layer even if the trap contained groups from
   * one or more warm layers.
   * @note Text primitives in the new group are sorted by their Y coordinate for proper display.
   */
  void CompressTrappedGroups();
  void CopyTrappedGroups(const EoGeVector3d& translate);
  void DeleteAllTrappedGroups();

  /** @brief Copies the trapped groups to the clipboard in various formats.
   *
   * This method opens the clipboard, empties its current contents, and then copies the trapped groups
   * to the clipboard in text, image, or group formats based on the application's clipboard settings.
   * After copying the data, it closes the clipboard.
   *
   * @param view A pointer to the AeSysView object used for rendering the groups.
   * @note This is done with two independent clipboard formats. The standard enhanced metafile and the private
   * EoDbGroupList which is read exclusively by Peg.
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

  auto FindTrappedGroup(EoDbGroup* group) { return m_trappedGroups.Find(group); }
  auto GetFirstTrappedGroupPosition() const { return m_trappedGroups.GetHeadPosition(); }
  EoDbGroup* GetNextTrappedGroup(POSITION& position) { return m_trappedGroups.GetNext(position); }
  EoGePoint3d GetTrapPivotPoint() const { return m_trapPivotPoint; }
  BOOL IsTrapEmpty() const { return m_trappedGroups.IsEmpty(); }
  void ModifyTrappedGroupsColor(std::int16_t color) { m_trappedGroups.ModifyColor(color); }
  void ModifyTrappedGroupsLineType(const std::wstring& lineTypeName) { m_trappedGroups.ModifyLineType(lineTypeName); }
  void ModifyTrappedGroupsNoteAttributes(const EoDbFontDefinition& fontDefinition,
      const EoDbCharacterCellDefinition& characterCellDefinition, int attributes);
  void RemoveAllTrappedGroups();
  EoDbGroup* RemoveLastTrappedGroup() { return m_trappedGroups.RemoveTail(); }
  auto RemoveTrappedGroup(EoDbGroup* group) { return m_trappedGroups.Remove(group); }
  void RemoveTrappedGroupAt(POSITION position) { m_trappedGroups.RemoveAt(position); }
  void SetTrapPivotPoint(const EoGePoint3d& pt) { m_trapPivotPoint = pt; }
  void SquareTrappedGroups(AeSysView* view);
  void TransformTrappedGroups(EoGeTransformMatrix& transformMatrix);
  void TranslateTrappedGroups(const EoGeVector3d& translate);
  [[nodiscard]] auto TrapGroupCount() { return m_trappedGroups.GetCount(); }
  [[nodiscard]] auto* GroupsInTrap() { return &m_trappedGroups; }

  /// @brief Moves all trapped groups to the specified target space.
  /// Each group is removed from its current layer (model or paper) and added to the
  /// corresponding layer (by name) in the target space. If the target layer does not
  /// exist, a minimal layer is created. The trap is cleared after the transfer.
  void MoveTrappedGroupsToSpace(EoDxf::Space targetSpace);

  // Nodal list interface (includes list of groups, primitives and unique points)
  void DeleteNodalResources();
  /// @brief Maintains a list of the primatives with at least one identified node.
  void UpdateNodalList(EoDbGroup* group, EoDbPrimitive* primitive, DWORD mask, int bit, EoGePoint3d point);

  auto AddNodalGroup(EoDbGroup* group) { return m_NodalGroupList.AddTail(group); }
  auto FindNodalGroup(EoDbGroup* group) { return m_NodalGroupList.Find(group); }
  [[nodiscard]] auto GetFirstNodalGroupPosition() const { return m_NodalGroupList.GetHeadPosition(); }
  EoDbGroup* GetNextNodalGroup(POSITION& position) { return m_NodalGroupList.GetNext(position); }
  void RemoveAllNodalGroups() { m_NodalGroupList.RemoveAll(); }
  POSITION AddMaskedPrimitive(EoDbMaskedPrimitive* maskedPrimitive) {
    return m_MaskedPrimitives.AddTail((CObject*)maskedPrimitive);
  }
  [[nodiscard]] auto GetFirstMaskedPrimitivePosition() const { return m_MaskedPrimitives.GetHeadPosition(); }
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
  [[nodiscard]] auto GetFirstUniquePointPosition() const { return m_UniquePoints.GetHeadPosition(); }
  EoGeUniquePoint* GetNextUniquePoint(POSITION& position) { return (EoGeUniquePoint*)m_UniquePoints.GetNext(position); }
  void RemoveUniquePointAt(POSITION position) { m_UniquePoints.RemoveAt(position); }
  void RemoveAllUniquePoints() { m_UniquePoints.RemoveAll(); }
  int RemoveUniquePoint(const EoGePoint3d& point);

 public:  // Generated message map functions
  afx_msg void OnBlocksLoad();
  afx_msg void OnBlocksRemoveUnused();
  afx_msg void OnBlocksUnload();
  afx_msg void OnToolsEditBlockDefinition();
  void OnToolsSaveAsBlockEdit();
  afx_msg void OnUpdateToolsEditBlockDefinition(CCmdUI* cmdUI);
  afx_msg void OnClearActiveLayers();
  afx_msg void OnClearAllLayers();
  afx_msg void OnClearAllTracings();
  afx_msg void OnClearMappedTracings();
  afx_msg void OnClearViewedTracings();
  afx_msg void OnClearWorkingLayer();
  /// @brief The current view is copied to the clipboard as an enhanced metafile.
  afx_msg void OnEditImageToClipboard();
  afx_msg void OnEditSegToWork();
  /// @brief Pastes clipboard to drawing. Only EoGroups format handled and no translation is performed.
  afx_msg void OnEditTrace();
  afx_msg void OnEditTrapCopy();
  afx_msg void OnEditTrapCut();
  afx_msg void OnEditTrapDelete();
  /// @brief Initializes current trap and all trap component lists.
  afx_msg void OnEditTrapQuit();

  /** @brief Pastes the contents of the clipboard into the current drawing.
   *
   * This method checks the clipboard for data in the EoGroups format, which is specific to the application or CF_TEXT
   * format. If EoGroups format is found, it is pasted into the current drawing, and any other formats present in the
   * clipboard are ignored. This ensures that only compatible data is processed and integrated into the drawing.
   */
  afx_msg void OnEditTrapPaste();

  /// @brief Adds all groups in the work layer to the trap.
  afx_msg void OnEditTrapWork();
  /// @brief Add all groups in all work and active layers to the trap.
  afx_msg void OnEditTrapWorkAndActive();
  afx_msg void OnFile();
  afx_msg void OnFileManageBlocks();
  afx_msg void OnFileManageLayers();
  afx_msg void OnFileQuery();
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
  /// @brief Breaks a primitive into a simpler set of primitives.
  afx_msg void OnPrimBreak();
  /// @brief Searches for closest detectible primitive. If found, primitive is lifted from its group, inserted into a
  /// new group which is added to deleted group list. The primitive resources are not freed.
  afx_msg void OnToolsPrimitiveDelete();
  afx_msg void OnPrimExtractNum();
  afx_msg void OnPrimExtractStr();
  /// @brief Positions the cursor at a "control" point on the current engaged group.
  afx_msg void OnPrimGotoCenterPoint();
  /// @brief Picks a primative and modifies its attributes to current settings.
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
  /// @brief Reduces complex primitives and group references to a simpler form
  afx_msg void OnToolsGroupBreak();

  /** @brief Handles the deletion of a group in the active view.
    This function retrieves the current cursor position from the active view and attempts to select a group and its
    primitive at that position. If a group is found, it removes the group from any layer it belongs to, removes it from
    all views, and checks if it is trapped. If it is trapped, it updates the state information of the active view. The
    function then updates all views to reflect the deletion of the group and adds the deleted group to a list for
    potential restoration. Finally, it adds a message to inform the user that the segment has been deleted and can be
    restored.
  */
  afx_msg void OnToolsGroupDelete();
  afx_msg void OnToolsGroupDeletelast();
  /// @brief Exchanges the first and last groups on the deleted group list.
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
  afx_msg void OnViewModelSpace();
  afx_msg void OnUpdateViewModelSpace(CCmdUI* cmdUI);

  /** @brief Fuses a tracing layer into the main document converting it to a static layer.
   *
   * This method modifies the specified tracing layer by removing its tracing flags,
   * making it resident and internal, and setting its state to static. It also updates
   * the layer's name based on the provided file name.
   *
   * @param nameAndLocation The name and location of the tracing layer file.
   * @note This is a cold state meaning the layer is displayed using warm color set, is not detectable, and may not have
   * its groups modified or deleted. No new groups may be added to the layer.
   */
  void TracingFuse(CString& nameAndLocation);
  bool TracingLoadLayer(const CString& pathName, EoDbLayer* layer);

  /** @brief Maps a tracing file as a layer in the document.
   * @param pathName The file path of the tracing file to be mapped.
   * @return true if the tracing file was successfully mapped; false otherwise.
   * @note This method checks if the specified tracing file is already loaded as a layer. If it is, it prompts the user
   * to close it first. If not, it attempts to load the tracing file into a new layer and adds it to the layer table. If
   * successful, it sets the appropriate flags on the layer and updates all views to reflect the change. This is a cold
   * state meaning the tracing is displayed using warm color set, is not detectable, and may not have its groups
   * modified or deleted. No new groups may be added to the tracing. Any number of tracings may be mapped.
   */
  bool TracingMap(const CString& pathName);

  /** @brief Opens a tracing file as a layer. If the file is already loaded, it will be re-opened if it is not currently
   * opened. If the file is not already loaded, a new layer will be created and the tracing file will be loaded into it.
   * The layer will be set to the opened state and made active. All views will be updated to reflect the changes.
   * @param pathName The name of the tracing file to open.
   * @return true if the tracing file was successfully opened; otherwise, false.
   */
  bool TracingOpen(const CString& pathName);

  /** @brief Displays a tracing file as a layer in the current document.
   * This method checks if the specified tracing file is already loaded as a layer.
   * If it is, it prompts the user to close it first. If not, it creates a new layer and loads the tracing file into it.
   * The layer is then marked as mapped and the views are updated to display it.
   * @param pathName The path of the tracing file to be displayed.
   * @return true if the tracing file was successfully loaded and displayed; false otherwise.
   */
  bool TracingView(const CString& pathName);
};
