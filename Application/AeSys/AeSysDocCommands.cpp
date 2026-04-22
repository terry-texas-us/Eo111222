#include "Stdafx.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "AeSys.h"
#include "AeSysDoc.h"
#include "AeSysView.h"
#include "Eo.h"
#include "EoDb.h"
#include "EoDbBlock.h"
#include "EoDbBlockFile.h"
#include "EoDbBlockReference.h"
#include "EoDbCharacterCellDefinition.h"
#include "EoDbFontDefinition.h"
#include "EoDbGroup.h"
#include "EoDbGroupList.h"
#include "EoDbJobFile.h"
#include "EoDbLabeledLine.h"
#include "EoDbLayer.h"
#include "EoDbLine.h"
#include "EoDbLineType.h"
#include "EoDbPoint.h"
#include "EoDbPolygon.h"
#include "EoDbPolyline.h"
#include "EoDbPrimitive.h"
#include "EoDbText.h"
#include "EoDlgDrawOptions.h"
#include "EoDlgEditTrapCommandsQuery.h"
#include "EoDlgFileManageBlocks.h"
#include "EoDlgFileManageLayers.h"
#include "EoDlgLineTypesSelection.h"
#include "EoDlgSelectGotoHomePoint.h"
#include "EoDlgSetHomePoint.h"
#include "EoDlgSetPastePosition.h"
#include "EoDlgSetupColor.h"
#include "EoDlgSetupHatch.h"
#include "EoDlgSetupNote.h"
#include "EoDlgSetupPointStyle.h"
#include "EoDlgTrapFilter.h"
#include "EoGePoint3d.h"
#include "EoGePoint4d.h"
#include "EoGeReferenceSystem.h"
#include "EoGeVector3d.h"
#include "EoGsRenderDeviceGdi.h"
#include "EoGsRenderState.h"
#include "Hatch.h"
#include "Lex.h"
#include "Resource.h"

#if defined(USING_DDE)
#include "ddeGItms.h"
#endif

// AeSysDoc commands

void AeSysDoc::OnClearActiveLayers() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsActive()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      auto position = layer->GetHeadPosition();
      while (position != nullptr) { UnregisterGroupHandles(layer->GetNext(position)); }
      layer->DeleteGroupsAndRemoveAll();
    }
  }
}

void AeSysDoc::OnClearAllLayers() {
  InitializeGroupAndPrimitiveEdit();

  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsInternal()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      auto position = layer->GetHeadPosition();
      while (position != nullptr) { UnregisterGroupHandles(layer->GetNext(position)); }
      layer->DeleteGroupsAndRemoveAll();
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnClearWorkingLayer() {
  InitializeGroupAndPrimitiveEdit();
  InitializeWorkLayer();
}
void AeSysDoc::OnClearAllTracings() {
  InitializeGroupAndPrimitiveEdit();

  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (!layer->IsInternal()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      auto position = layer->GetHeadPosition();
      while (position != nullptr) { UnregisterGroupHandles(layer->GetNext(position)); }
      layer->DeleteGroupsAndRemoveAll();
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnClearMappedTracings() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsMapped()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);

      if (layer->IsResident()) {
        layer->ClearTracingStateBit(static_cast<std::uint16_t>(EoDbLayer::TracingState::isMapped));
        layer->SetStateOff();
      } else {
        RemoveLayerTableLayerAt(i);
      }
    }
  }
}

void AeSysDoc::OnClearViewedTracings() {
  InitializeGroupAndPrimitiveEdit();
  for (int i = GetLayerTableSize() - 1; i > 0; i--) {
    auto* layer = GetLayerTableLayerAt(i);

    if (layer->IsViewed()) {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);

      if (layer->IsResident()) {
        layer->ClearTracingStateBit(static_cast<std::uint16_t>(EoDbLayer::TracingState::isViewed));
        layer->SetStateOff();
      } else {
        RemoveLayerTableLayerAt(i);
      }
    }
  }
}

void AeSysDoc::OnPrimBreak() {
  auto* activeView = AeSysView::GetActiveView();

  auto* group = activeView->SelectGroupAndPrimitive(activeView->GetCursorPosition());
  if (group != nullptr && activeView->EngagedPrimitive() != nullptr) {
    auto* primitive = activeView->EngagedPrimitive();

    if (primitive->Is(EoDb::kPolylinePrimitive)) {
      group->FindAndRemovePrim(primitive);
      UnregisterHandle(primitive->Handle());

      auto* polyline = static_cast<EoDbPolyline*>(primitive);

      EoGePoint3dArray points;
      polyline->GetAllPoints(points);

      auto color = primitive->Color();
      const auto& lineTypeName = primitive->LineTypeName();
      auto lineWeight = primitive->LineWeight();

      for (auto i = 0; i < points.GetSize() - 1; i++) {
        auto* line = EoDbLine::CreateLine(points[i], points[i + 1])->WithProperties(color, lineTypeName, lineWeight);
        RegisterHandle(line);
        group->AddTail(line);
      }
      if (polyline->IsLooped()) {
        auto* line =
            EoDbLine::CreateLine(points[points.GetUpperBound()], points[0])->WithProperties(color, lineTypeName, lineWeight);
        RegisterHandle(line);
        group->AddTail(line);
      }
      delete primitive;
      ResetAllViews();
    } else if (primitive->Is(EoDb::kGroupReferencePrimitive)) {
      auto* blockReference = static_cast<EoDbBlockReference*>(primitive);

      EoDbBlock* block{};

      if (LookupBlock(blockReference->BlockName(), block) != 0) {
        group->FindAndRemovePrim(primitive);
        UnregisterHandle(primitive->Handle());

        auto transformMatrix = blockReference->BuildTransformMatrix(block->BasePoint());

        EoDbGroup* pSegT = new EoDbGroup(*block);
        pSegT->Transform(transformMatrix);
        RegisterGroupHandles(pSegT);
        group->AddTail(pSegT);

        delete primitive;
        ResetAllViews();
      }
    }
  }
}

void AeSysDoc::OnEditSegToWork() {
  auto cursorPosition = app.GetCursorPosition();

  auto* layer = LayersSelUsingPoint(cursorPosition);
  if (layer == nullptr) { return; }

  if (layer->IsInternal()) {
    auto* group = layer->SelectGroupUsingPoint(cursorPosition);

    if (group == nullptr) { return; }
    layer->Remove(group);
    UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, group);
    AddWorkLayerGroup(group);
    UpdateAllViews(nullptr, EoDb::kGroup, group);
  }
}

void AeSysDoc::OnFileQuery() {
  auto cursorPosition = app.GetCursorPosition();

  auto* layer = LayersSelUsingPoint(cursorPosition);

  if (layer != nullptr) {
    CPoint currentPosition;
    ::GetCursorPos(&currentPosition);

    m_IdentifiedLayerName = layer->Name();

    int MenuResource = (layer->IsInternal()) ? IDR_LAYER : IDR_TRACING;

    auto layerTracingMenu = ::LoadMenuW(AeSys::GetInstance(), MAKEINTRESOURCE(MenuResource));
    auto* subMenu = CMenu::FromHandle(::GetSubMenu(layerTracingMenu, 0));

    subMenu->ModifyMenu(0, MF_BYPOSITION | MF_STRING, 0, m_IdentifiedLayerName);

    if (MenuResource == IDR_LAYER) {
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_WORK),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsWork() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_ACTIVE),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsActive() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_STATIC),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsStatic() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_LAYER_HIDDEN),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsOff() ? MF_CHECKED : MF_UNCHECKED)));
    } else {
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_OPEN),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsOpened() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_MAP),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsMapped() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_VIEW),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsViewed() ? MF_CHECKED : MF_UNCHECKED)));
      subMenu->CheckMenuItem(static_cast<UINT>(ID_TRACING_CLOAK),
          static_cast<UINT>(MF_BYCOMMAND | (layer->IsOff() ? MF_CHECKED : MF_UNCHECKED)));
    }
    subMenu->TrackPopupMenuEx(0, currentPosition.x, currentPosition.y, AfxGetMainWnd(), 0);
    ::DestroyMenu(layerTracingMenu);
  }
}

void AeSysDoc::OnLayerActive() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer == nullptr) {
  } else {
    if (layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_ACTIVE, m_IdentifiedLayerName);
    } else {
      layer->SetStateActive();
      UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
    }
  }
}

void AeSysDoc::OnLayerStatic() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer != nullptr) {
    if (layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_STATIC, m_IdentifiedLayerName);
    } else {
      layer->SetStateStatic();
      UpdateAllViews(nullptr, EoDb::kLayerSafe, layer);
    }
  }
}

void AeSysDoc::OnLayerHidden() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer != nullptr) {
    if (layer->IsWork()) {
      app.WarningMessageBox(IDS_MSG_LAYER_NO_HIDDEN, m_IdentifiedLayerName);
    } else {
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->SetStateOff();
    }
  }
}

void AeSysDoc::OnLayerMelt() { LayerMelt(m_IdentifiedLayerName); }

void AeSysDoc::OnLayerWork() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);
  if (layer == nullptr) { return; }

  SetWorkLayer(layer);
}

void AeSysDoc::OnTracingMap() { TracingMap(m_IdentifiedLayerName); }

void AeSysDoc::OnTracingView() { TracingView(m_IdentifiedLayerName); }

void AeSysDoc::OnTracingCloak() {
  auto* layer = GetLayerTableLayer(m_IdentifiedLayerName);

  if (layer->IsOpened()) {
    CFile File(m_IdentifiedLayerName, CFile::modeWrite | CFile::modeCreate);
    if (File != CFile::hFileNull) {
      EoDbJobFile JobFile;
      JobFile.WriteHeader(File);
      JobFile.WriteLayer(File, layer);
      SetWorkLayer(GetLayerTableLayerAt(0));
      m_saveAsType = EoDb::FileTypes::Unknown;
      UpdateAllViews(nullptr, EoDb::kLayerErase, layer);
      layer->SetStateOff();
    } else {
      app.WarningMessageBox(IDS_MSG_TRACING_WRITE_FAILURE, m_IdentifiedLayerName);
    }
  }
}

void AeSysDoc::OnTracingFuse() { TracingFuse(m_IdentifiedLayerName); }
void AeSysDoc::OnTracingOpen() { TracingOpen(m_IdentifiedLayerName); }

void AeSysDoc::OnLayersActiveAll() {
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (!layer->IsWork()) { layer->SetStateActive(); }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnLayersStaticAll() {
  for (auto i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (!layer->IsWork()) { layer->SetStateStatic(); }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnLayersRemoveEmpty() { RemoveEmptyLayers(); }
void AeSysDoc::OnToolsGroupUndelete() { DeletedGroupsRestore(); }
void AeSysDoc::OnPensRemoveUnusedStyles() { m_LineTypeTable.RemoveUnused(); }
void AeSysDoc::OnBlocksLoad() {
  CFileDialog dlg(TRUE, L"blk", L"*.blk");
  dlg.m_ofn.lpstrTitle = L"Load Blocks";

  if (dlg.DoModal() == IDOK) {
    if ((dlg.m_ofn.Flags & OFN_EXTENSIONDIFFERENT) == 0) {
      EoDbBlockFile fb;
      fb.ReadFile(dlg.GetPathName(), m_BlocksTable);
    } else {
      app.WarningMessageBox(IDS_MSG_FILE_TYPE_ERROR);
    }
  }
}
void AeSysDoc::OnBlocksRemoveUnused() { RemoveUnusedBlocks(); }
void AeSysDoc::OnBlocksUnload() {
  CFileDialog dlg(FALSE, L"blk", L"*.blk");
  dlg.m_ofn.lpstrTitle = L"Unload Blocks As";

  if (dlg.DoModal() == IDOK) {
    if ((dlg.m_ofn.Flags & OFN_EXTENSIONDIFFERENT) == 0) {
      EoDbBlockFile fb;
      fb.WriteFile(dlg.GetPathName(), m_BlocksTable);
    }
  }
}
void AeSysDoc::OnEditImageToClipboard() {
  auto* activeView = AeSysView::GetActiveView();

  HDC hdcEMF = ::CreateEnhMetaFile(0, 0, 0, 0);
  CDC* emfDC = CDC::FromHandle(hdcEMF);
  EoGsRenderDeviceGdi renderDevice(emfDC);
  DisplayAllLayers(activeView, &renderDevice);
  HENHMETAFILE hemf = ::CloseEnhMetaFile(hdcEMF);

  ::OpenClipboard(nullptr);
  ::EmptyClipboard();
  ::SetClipboardData(CF_ENHMETAFILE, hemf);
  ::CloseClipboard();
}
void AeSysDoc::OnEditTrace() {
  if (::OpenClipboard(nullptr)) {
    wchar_t sBuf[16]{};

    UINT ClipboardFormat;
    UINT Format = 0;

    while ((ClipboardFormat = EnumClipboardFormats(Format)) != 0) {
      GetClipboardFormatName(ClipboardFormat, sBuf, 16);

      if (wcscmp(sBuf, L"EoGroups") == 0) {
        HGLOBAL ClipboardDataHandle = GetClipboardData(ClipboardFormat);
        if (ClipboardDataHandle != 0) {
          CMemFile MemFile;
          EoGeVector3d vTrns;

          LPCSTR ClipboardData = (LPCSTR)GlobalLock(ClipboardDataHandle);
          if (ClipboardData != nullptr) {
            DWORD dwSizeOfBuffer = *((DWORD*)ClipboardData);
            MemFile.Write(ClipboardData, UINT(dwSizeOfBuffer));
            GlobalUnlock(ClipboardDataHandle);

            MemFile.Seek(96, CFile::begin);
            EoDbJobFile JobFile;
            JobFile.ReadMemFile(MemFile, vTrns);
          }
          break;
        }
      }
      Format = ClipboardFormat;
    }
    CloseClipboard();
  } else {
    app.WarningMessageBox(IDS_MSG_CLIPBOARD_LOCKED);
  }
}
void AeSysDoc::OnEditTrapDelete() {
  DeleteAllTrappedGroups();
  UpdateAllViews(nullptr, 0L, nullptr);
  OnEditTrapQuit();
}
void AeSysDoc::OnEditTrapQuit() {
  RemoveAllTrappedGroups();
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void AeSysDoc::OnEditTrapCopy() {
  auto* activeView = AeSysView::GetActiveView();
  CopyTrappedGroupsToClipboard(activeView);
}
void AeSysDoc::OnEditTrapCut() {
  auto* activeView = AeSysView::GetActiveView();
  CopyTrappedGroupsToClipboard(activeView);
  DeleteAllTrappedGroups();
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnEditTrapPaste() {
  if (::OpenClipboard(nullptr)) {
    UINT clipboardFormat = app.ClipboardFormatIdentifierForEoGroups();

    if (IsClipboardFormatAvailable(clipboardFormat)) {
      EoDlgSetPastePosition dialog;
      if (dialog.DoModal() == IDOK) {
        HGLOBAL globalHandle = GetClipboardData(clipboardFormat);
        if (globalHandle != 0) {
          EoGePoint3d minPoint;

          EoGePoint3d pivotPoint(app.GetCursorPosition());
          SetTrapPivotPoint(pivotPoint);

          LPCSTR buffer = (LPCSTR)GlobalLock(globalHandle);

          DWORD sizeOfBuffer{};
          if (buffer == nullptr) {
            GlobalUnlock(globalHandle);
            CloseClipboard();
            app.WarningMessageBox(IDS_MSG_CLIPBOARD_LOCKED);
            return;
          }
          sizeOfBuffer = *((DWORD*)buffer);

          CMemFile memoryFile;
          memoryFile.Write(buffer, UINT(sizeOfBuffer));

          memoryFile.Seek(sizeof(DWORD), CFile::begin);
          minPoint.Read(memoryFile);

          EoGeVector3d translateVector(minPoint, pivotPoint);

          GlobalUnlock(globalHandle);

          memoryFile.Seek(96, CFile::begin);
          EoDbJobFile jobFile;
          jobFile.ReadMemFile(memoryFile, translateVector);
        }
      }
    } else if (IsClipboardFormatAvailable(CF_TEXT)) {
      HGLOBAL clipboardDataHandle = GetClipboardData(CF_TEXT);

      LPWSTR clipboardText = new wchar_t[GlobalSize(clipboardDataHandle)];

      LPCWSTR clipboardData = (LPCWSTR)GlobalLock(clipboardDataHandle);
      if (clipboardData != nullptr) {
        lstrcpyW(clipboardText, clipboardData);
      } else {
        clipboardText[0] = L'\0';
      }
      GlobalUnlock(clipboardDataHandle);

      AddTextBlock(clipboardText);

      delete[] clipboardText;
    }
    CloseClipboard();
  } else {
    app.WarningMessageBox(IDS_MSG_CLIPBOARD_LOCKED);
  }
}

void AeSysDoc::OnEditTrapWork() {
  RemoveAllTrappedGroups();
  AddGroupsToTrap(GetWorkLayer());
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}
void AeSysDoc::OnEditTrapWorkAndActive() {
  RemoveAllTrappedGroups();

  for (int i = 0; i < GetLayerTableSize(); i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork() || layer->IsActive()) { AddGroupsToTrap(layer); }
  }
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::TrapCount);
}

void AeSysDoc::OnTrapCommandsCompress() { CompressTrappedGroups(); }

void AeSysDoc::OnTrapCommandsExpand() {
  try {
    ExpandTrappedGroups();
  } catch (...) { ATLTRACE2(traceGeneral, 3, L"AeSysDoc::OnTrapCommandsExpand: Failed to expand trapped groups.\n"); }
}

void AeSysDoc::OnTrapCommandsInvert() {
  int layerTableSize = GetLayerTableSize();
  for (int i = 0; i < layerTableSize; i++) {
    auto* layer = GetLayerTableLayerAt(i);
    if (layer->IsWork() || layer->IsActive()) {
      auto LayerPosition = layer->GetHeadPosition();
      while (LayerPosition != 0) {
        auto* Group = layer->GetNext(LayerPosition);
        auto GroupPosition = FindTrappedGroup(Group);
        if (GroupPosition != nullptr) {
          m_trappedGroups.RemoveAt(GroupPosition);
        } else {
          AddGroupToTrap(Group);
        }
      }
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnTrapCommandsSquare() {
  auto* activeView = AeSysView::GetActiveView();
  SquareTrappedGroups(activeView);
}
void AeSysDoc::OnTrapCommandsQuery() {
  EoDlgEditTrapCommandsQuery Dialog;

  if (Dialog.DoModal() == IDOK) {}
}
void AeSysDoc::OnTrapCommandsFilter() {
  EoDlgTrapFilter Dialog(this);
  if (Dialog.DoModal() == IDOK) {}
}

void AeSysDoc::OnTrapCommandsBlock() {
  if (m_trappedGroups.GetCount() == 0) { return; }

  EoDbBlock* block{};
  auto blockTableSize = BlockTableSize();
  wchar_t name[16]{};

  do { swprintf_s(name, 16, L"_%.3i", ++blockTableSize); } while (LookupBlock(name, block));

  block = new EoDbBlock;

  auto position = GetFirstTrappedGroupPosition();
  while (position != nullptr) {
    auto* group = GetNextTrappedGroup(position);

    auto* newGroup = new EoDbGroup(*group);

    block->AddTail(newGroup);

    newGroup->RemoveAll();

    delete newGroup;
  }
  block->SetBasePoint(m_trapPivotPoint);
  InsertBlock(CString(name), block);
}

void AeSysDoc::OnTrapCommandsUnblock() { m_trappedGroups.ExplodeBlockReferences(); }
void AeSysDoc::OnSetupPenColor() {
  EoDlgSetupColor Dialog;
  Dialog.m_ColorIndex = static_cast<std::uint16_t>(Gs::renderState.Color());

  if (Dialog.DoModal() == IDOK) {
    Gs::renderState.SetColor(static_cast<CDC*>(nullptr), static_cast<std::int16_t>(Dialog.m_ColorIndex));

    AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Pen);
  }
}

/** @brief Handles the command to set up the line type for drawing.
  This function checks if there are any line types defined in the document's line type table.
  If none are defined, it displays a warning message. Otherwise, it opens a dialog to allow
  the user to select a line type. If the user selects a line type and confirms, it updates
  the current drawing state with the selected line type and refreshes the view to reflect
  the change.
*/
void AeSysDoc::OnSetupLineType() {
  if (m_LineTypeTable.IsEmpty()) {
    app.WarningMessageBox(IDS_MSG_NO_LINETYPES_DEFINED);
    return;
  }
  EoDlgLineTypesSelection dialog(m_LineTypeTable);

  EoDbLineType* currentLineType{};

  // Prefer name-based lookup when a linetype name is set on the render state
  const auto& currentName = Gs::renderState.LineTypeName();
  if (!currentName.empty()) {
    [[maybe_unused]] const auto found = m_LineTypeTable.Lookup(CString(currentName.c_str()), currentLineType);
  }
  if (currentLineType == nullptr) {
    m_LineTypeTable.LookupUsingLegacyIndex(static_cast<std::uint16_t>(Gs::renderState.LineTypeIndex()), currentLineType);
  }
  dialog.SetSelectedLineType(currentLineType);

  if (dialog.DoModal() != IDOK) { return; }

  EoDbLineType* selectedLineType = dialog.GetSelectedLineType();
  if (selectedLineType == nullptr) {
    ATLTRACE2(traceGeneral, 3, L"AeSysDoc::OnSetupLineType: No line type selected.\n");
    return;
  }

  // When the selection came from the file-loaded list, copy the linetype into the
  // document's table so the pointer remains valid after the dialog is destroyed.
  if (dialog.IsSelectedFromFileList()) {
    EoDbLineType* existingLineType{};
    if (!m_LineTypeTable.Lookup(selectedLineType->Name(), existingLineType)) {
      auto* clonedLineType = new EoDbLineType(*selectedLineType);
      m_LineTypeTable.SetAt(clonedLineType->Name(), clonedLineType);
      selectedLineType = clonedLineType;
    } else {
      selectedLineType = existingLineType;
    }
  }

  Gs::renderState.SetLineType(static_cast<CDC*>(nullptr), static_cast<std::int16_t>(selectedLineType->Index()));
  Gs::renderState.SetLineTypeName(std::wstring(selectedLineType->Name()));
  AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Line);
}

void AeSysDoc::OnSetupFillHollow() { Gs::renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Hollow); }
void AeSysDoc::OnSetupFillSolid() { Gs::renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Solid); }
void AeSysDoc::OnSetupFillPattern() {}
void AeSysDoc::OnSetupFillHatch() {
  EoDlgSetupHatch Dialog;
  Dialog.m_HatchXScaleFactor = hatch::dXAxRefVecScal;
  Dialog.m_HatchYScaleFactor = hatch::dYAxRefVecScal;
  Dialog.m_HatchRotationAngle = Eo::RadianToDegree(hatch::dOffAng);

  if (Dialog.DoModal() == IDOK) {
    Gs::renderState.SetPolygonIntStyle(EoDb::PolygonStyle::Hatch);
    hatch::dXAxRefVecScal = std::max(0.01, Dialog.m_HatchXScaleFactor);
    hatch::dYAxRefVecScal = std::max(0.01, Dialog.m_HatchYScaleFactor);
    hatch::dOffAng = Eo::DegreeToRadian(Dialog.m_HatchRotationAngle);
  }
}

void AeSysDoc::OnSetupNote() {
  EoDbFontDefinition fontDefinition = Gs::renderState.FontDefinition();

  EoDlgSetupNote Dialog(&fontDefinition);

  auto characterCellDefinition = Gs::renderState.CharacterCellDefinition();

  Dialog.m_height = characterCellDefinition.Height();
  Dialog.m_rotationAngle = Eo::RadianToDegree(characterCellDefinition.RotationAngle());
  Dialog.m_expansionFactor = characterCellDefinition.ExpansionFactor();
  Dialog.m_slantAngle = Eo::RadianToDegree(characterCellDefinition.SlantAngle());

  if (Dialog.DoModal() == IDOK) {
    characterCellDefinition.SetHeight(Dialog.m_height);
    characterCellDefinition.SetRotationAngle(Eo::DegreeToRadian(Dialog.m_rotationAngle));
    characterCellDefinition.SetExpansionFactor(Dialog.m_expansionFactor);
    characterCellDefinition.SetSlantAngle(Eo::DegreeToRadian(Dialog.m_slantAngle));
    Gs::renderState.SetCharacterCellDefinition(characterCellDefinition);

    auto* activeView = AeSysView::GetActiveView();
    CDC* DeviceContext = (activeView == nullptr) ? nullptr : activeView->GetDC();

    Gs::renderState.SetFontDefinition(DeviceContext, fontDefinition);
  }
}

void AeSysDoc::OnSetupPointStyle() {
  CDlgSetPointStyle dlg;
  // Preload dialog from global state and document
  dlg.m_pointStyle = Gs::renderState.PointStyle();
  dlg.m_pointSize = GetPointSize();

  if (dlg.DoModal() == IDOK) {
    // Apply to global primitive state
    Gs::renderState.SetPointStyle(static_cast<short>(dlg.m_pointStyle));
    // Store into document
    SetPointSize(dlg.m_pointSize);

    UpdateAllViews(nullptr, 0L, nullptr);
  }
}

void AeSysDoc::OnToolsGroupBreak() {
  auto* activeView = AeSysView::GetActiveView();

  activeView->BreakAllPolylines();
  activeView->ExplodeAllBlockReferences();
}

void AeSysDoc::OnToolsGroupDelete() {
  auto* activeView = AeSysView::GetActiveView();
  auto cursorPosition = activeView->GetCursorPosition();

  auto* Group = activeView->SelectGroupAndPrimitive(cursorPosition);

  if (Group != nullptr) {
    AnyLayerRemove(Group);
    RemoveGroupFromAllViews(Group);
    if (RemoveTrappedGroup(Group) != nullptr) { activeView->UpdateStateInformation(AeSysView::TrapCount); }
    UpdateAllViews(nullptr, EoDb::kGroupEraseSafe, Group);
    DeletedGroupsAddTail(Group);
    app.AddStringToMessageList(IDS_SEG_DEL_TO_RESTORE);
  }
}
void AeSysDoc::OnToolsGroupDeletelast() {
  auto* activeView = AeSysView::GetActiveView();

  activeView->DeleteLastGroup();
}
void AeSysDoc::OnToolsGroupExchange() {
  if (!DeletedGroupsIsEmpty()) {
    EoDbGroup* TailGroup = DeletedGroupsRemoveTail();
    EoDbGroup* HeadGroup = DeletedGroupsRemoveHead();
    DeletedGroupsAddTail(HeadGroup);
    DeletedGroupsAddHead(TailGroup);
  }
}

void AeSysDoc::OnToolsPrimitiveSnaptoendpoint() {
  auto* activeView = AeSysView::GetActiveView();

  EoGePoint4d ndcPoint(activeView->GetCursorPosition());
  activeView->ModelViewTransformPoint(ndcPoint);

  if (activeView->GroupIsEngaged()) {
    auto* primitive = activeView->EngagedPrimitive();

    if (primitive->PivotOnControlPoint(activeView, ndcPoint)) {
      EoGePoint3d ptEng = activeView->DetPt();
      primitive->AddReportToMessageList(ptEng);
      activeView->SetCursorPosition(ptEng);
      return;
    }
    // Did not pivot on engaged primitive
    if (primitive->IsPointOnControlPoint(activeView, ndcPoint)) { EoDbGroup::SetPrimitiveToIgnore(primitive); }
  }
  if (activeView->SelSegAndPrimAtCtrlPt(ndcPoint) != 0) {
    EoGePoint3d ptEng = activeView->DetPt();
    activeView->EngagedPrimitive()->AddReportToMessageList(ptEng);
    activeView->SetCursorPosition(ptEng);
  }
  EoDbGroup::SetPrimitiveToIgnore(static_cast<EoDbPrimitive*>(nullptr));
}

void AeSysDoc::OnPrimGotoCenterPoint() {
  auto* activeView = AeSysView::GetActiveView();
  if (activeView->GroupIsEngaged()) {
    EoGePoint3d pt = activeView->EngagedPrimitive()->GetControlPoint();
    activeView->SetCursorPosition(pt);
  }
}

void AeSysDoc::OnToolsPrimitiveDelete() {
  EoGePoint3d pt = app.GetCursorPosition();

  auto* activeView = AeSysView::GetActiveView();

  auto* group = activeView->SelectGroupAndPrimitive(pt);

  if (group != 0) {
    auto position = FindTrappedGroup(group);

    LPARAM lHint = (position != nullptr) ? EoDb::kGroupEraseSafeTrap : EoDb::kGroupEraseSafe;
    // erase entire group even if group has more than one primitive
    UpdateAllViews(nullptr, lHint, group);

    if (group->GetCount() > 1) {  // remove primitive from group
      auto* primitive = activeView->EngagedPrimitive();
      group->FindAndRemovePrim(primitive);
      lHint = (position != nullptr) ? EoDb::kGroupSafeTrap : EoDb::kGroupSafe;
      // display the group with the primitive removed
      UpdateAllViews(nullptr, lHint, group);
      // new group required to allow primitive to be placed into deleted group list
      group = new EoDbGroup(primitive);
    } else {  // deleting an entire group
      AnyLayerRemove(group);
      RemoveGroupFromAllViews(group);

      if (RemoveTrappedGroup(group) != 0) { activeView->UpdateStateInformation(AeSysView::TrapCount); }
    }
    DeletedGroupsAddTail(group);
    app.AddStringToMessageList(IDS_MSG_PRIM_ADDED_TO_DEL_GROUPS);
  }
}

void AeSysDoc::OnPrimModifyAttributes() {
  auto* activeView = AeSysView::GetActiveView();

  auto cursorPosition = activeView->GetCursorPosition();

  auto* Group = activeView->SelectGroupAndPrimitive(cursorPosition);

  if (Group != nullptr) {
    activeView->EngagedPrimitive()->ModifyState();
    UpdateAllViews(nullptr, EoDb::kPrimitiveSafe, activeView->EngagedPrimitive());
  }
}
void AeSysDoc::OnSetupSavePoint() {
  EoDlgSetHomePoint Dialog(AeSysView::GetActiveView());

  if (Dialog.DoModal() == IDOK) {}
}
void AeSysDoc::OnSetupGotoPoint() {
  EoDlgSelectGotoHomePoint Dialog(AeSysView::GetActiveView());

  if (Dialog.DoModal() == IDOK) {}
}
void AeSysDoc::OnSetupOptionsDraw() {
  EoDlgDrawOptions Dialog;

  if (Dialog.DoModal() == IDOK) { AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::All); }
}

void AeSysDoc::OnFileManageBlocks() {
  EoDlgFileManageBlocks dlg(this);
  if (dlg.DoModal() == IDOK) {}
}

void AeSysDoc::OnFileManageLayers() {
  EoDlgFileManageLayers dlg(this);

  if (dlg.DoModal() == IDOK) {}
}

void AeSysDoc::OnMaintenanceRemoveEmptyNotes() {
  int NumberOfEmptyNotes = RemoveEmptyNotesAndDelete();
  int NumberOfEmptyGroups = RemoveEmptyGroups();
  CString str;
  str.Format(L"%d notes were removed resulting in %d empty groups which were also removed.", NumberOfEmptyNotes,
      NumberOfEmptyGroups);
  app.AddStringToMessageList(str);
}
void AeSysDoc::OnMaintenanceRemoveEmptyGroups() {
  int NumberOfEmptyGroups = RemoveEmptyGroups();
  CString str;
  str.Format(L"%d were removed.", NumberOfEmptyGroups);
  app.AddStringToMessageList(str);
}
void AeSysDoc::OnPensEditColors() { app.EditColorPalette(); }

void AeSysDoc::OnPensLoadColors() {
  auto filter = App::LoadStringResource(IDS_OPENFILE_FILTER_PENCOLORS);
  auto title = App::LoadStringResource(IDS_OPENFILE_LOAD_PENCOLORS_TITLE);
  auto initialDir = App::PathFromCommandLine();

  CString file;
  file.GetBufferSetLength(MAX_PATH);

  OPENFILENAME of{};
  of.lStructSize = sizeof(OPENFILENAME);
  of.hInstance = AeSys::GetInstance();
  of.lpstrFilter = filter;
  of.lpstrFile = file.GetBuffer();
  of.nMaxFile = MAX_PATH;
  of.lpstrTitle = title;
  of.lpstrInitialDir = initialDir;
  of.Flags = OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
  of.lpstrDefExt = L"txt";

  if (GetOpenFileNameW(&of)) {
    if ((of.Flags & OFN_EXTENSIONDIFFERENT) == 0) {
      app.LoadPenColorsFromFile(of.lpstrFile);
      UpdateAllViews(nullptr, 0L, nullptr);
    } else {
      app.WarningMessageBox(IDS_MSG_FILE_TYPE_ERROR);
    }
  } else {
    auto error = CommDlgExtendedError();
    if (error != 0) { app.WarningMessageBox(IDS_MSG_OPENFILE_DIALOG_ERROR); }
  }
}

void AeSysDoc::OnPensTranslate() {
  CStdioFile fl;

  if (fl.Open(App::PathFromCommandLine() + L"\\Pens\\xlate.txt", CFile::modeRead | CFile::typeText)) {
    wchar_t pBuf[128]{};
    std::uint16_t wCols{};

    while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != 0) { wCols++; }

    if (wCols > 0) {
      auto* pColNew = new std::int16_t[wCols];
      auto* pCol = new std::int16_t[wCols];

      std::uint16_t w{};

      fl.SeekToBegin();

      LPWSTR NextToken;
      while (fl.ReadString(pBuf, sizeof(pBuf) / sizeof(wchar_t) - 1) != 0) {
        NextToken = nullptr;
        pCol[w] = std::int16_t(_wtoi(wcstok_s(pBuf, L",", &NextToken)));
        pColNew[w++] = std::int16_t(_wtoi(wcstok_s(0, L"\n", &NextToken)));
      }
      PenTranslation(wCols, pColNew, pCol);

      delete[] pColNew;
      delete[] pCol;
    }
  }
  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnFile() {
  CPoint Position(8, 8);

  AfxGetApp()->GetMainWnd()->ClientToScreen(&Position);
  CMenu* FileSubMenu = CMenu::FromHandle(app.GetSubMenu(0));
  FileSubMenu->TrackPopupMenuEx(TPM_LEFTALIGN, Position.x, Position.y, AfxGetMainWnd(), 0);
}

void AeSysDoc::OnPrimExtractNum() {
  auto* activeView = AeSysView::GetActiveView();

  auto cursorPosition = activeView->GetCursorPosition();

  if (activeView->SelectGroupAndPrimitive(cursorPosition)) {
    auto* primitive = activeView->EngagedPrimitive();

    CString number;

    if (primitive->Is(EoDb::kTextPrimitive)) {
      number = static_cast<EoDbText*>(primitive)->Text();
    } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
      number = static_cast<EoDbLabeledLine*>(primitive)->Text();
    } else {
      return;
    }
    double value[32]{};
    int iTyp{};
    long lDef{};
    int iTokId{};

    try {
      lex::Parse(number);
      lex::EvalTokenStream(&iTokId, &lDef, &iTyp, value);

      if (iTyp != lex::ArchitecturalUnitsLengthToken && iTyp != lex::EngineeringUnitsLengthToken &&
          iTyp != lex::SimpleUnitsLengthToken) {
        lex::ConvertValTyp(iTyp, lex::RealToken, &lDef, value);
      }
      wchar_t Message[64]{};
      swprintf_s(Message, 64, L"%10.4f ", value[0]);
      wcscat_s(Message, 64, L"was extracted from drawing");
      app.AddStringToMessageList(std::wstring(Message));
    } catch (...) {
      app.WarningMessageBox(IDS_MSG_INVALID_NUMBER_EXTRACT);
      return;
    }
#if defined(USING_DDE)
    app.SetExtractedNumber(value[0]);
    dde::PostAdvise(dde::ExtNumInfo);
#endif
  }
}

void AeSysDoc::OnPrimExtractStr() {
  auto* activeView = AeSysView::GetActiveView();

  auto cursorPosition = activeView->GetCursorPosition();

  if (activeView->SelectGroupAndPrimitive(cursorPosition)) {
    auto* primitive = activeView->EngagedPrimitive();

    CString String;

    if (primitive->Is(EoDb::kTextPrimitive)) {
      String = static_cast<EoDbText*>(primitive)->Text();
    } else if (primitive->Is(EoDb::kDimensionPrimitive)) {
      String = static_cast<EoDbLabeledLine*>(primitive)->Text();
    } else {
      return;
    }
    String += L" was extracted from drawing";
    app.AddStringToMessageList(String);
#if defined(USING_DDE)
    app.SetExtractedString(String);
    dde::PostAdvise(dde::ExtStrInfo);
#endif
  }
  return;
}

void AeSysDoc::OnHelpKey() {
  switch (app.CurrentMode()) {
    case ID_MODE_DRAW:
      WinHelpW(AeSys::GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"DRAW"));
      break;

    case ID_MODE_EDIT: {
      WinHelpW(AeSys::GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"EDIT"));
      break;
    }
    case ID_MODE_TRAP:
    case ID_MODE_TRAPR: {
      WinHelpW(AeSys::GetSafeHwnd(), L"peg.hlp", HELP_KEY, reinterpret_cast<DWORD_PTR>(L"TRAP"));
      break;
    }
  }
}

void AeSysDoc::OnViewModelSpace() {
  m_activeSpace = (m_activeSpace == EoDxf::Space::ModelSpace) ? EoDxf::Space::PaperSpace : EoDxf::Space::ModelSpace;

  // Switch the work layer to layer "0" in the newly active space
  auto* layer0 = GetLayerTableLayer(L"0");
  if (layer0 != nullptr) { SetWorkLayer(layer0); }

  // Ensure a default paper-space viewport exists when switching to PaperSpace
  if (m_activeSpace == EoDxf::Space::PaperSpace) {
    auto viewPosition = GetFirstViewPosition();
    if (viewPosition != nullptr) {
      auto* view = static_cast<AeSysView*>(GetNextView(viewPosition));
      CreateDefaultPaperSpaceViewport(view);
    }
  }

  UpdateAllViews(nullptr, 0L, nullptr);
}

void AeSysDoc::OnUpdateViewModelSpace(CCmdUI* cmdUI) { cmdUI->SetCheck(m_activeSpace == EoDxf::Space::ModelSpace); }
