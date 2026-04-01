# MFC Custom Color Selection Control

## Overview

`EoCtrlColorComboBox` is a dark-theme-aware, owner-draw ACI (AutoCAD Color Index) color selection combo box embedded in the AeSys standard toolbar. It replaces the default MFC toolbar button for `ID_PENCOLOR_COMBO` with a dropdown showing color swatches, named colors, and a "More Colors..." entry that opens the full `EoDlgSetupColor` dialog.

The control is built from two tightly coupled classes:

| Class | Base | Role |
|-------|------|------|
| `EoCtrlColorComboBox` | `CMFCToolBarComboBoxButton` | Toolbar button — owns the item list, handles serialization, draws the closed combo on the toolbar surface |
| `EoCtrlColorOwnerDrawCombo` | `CComboBox` | Child window — owner-draw dropdown list, flat custom painting, DPI-scaled arrow glyph |

## Class Architecture

### EoCtrlColorComboBox (Toolbar Button)

The toolbar button is the **data owner**. MFC's `CMFCToolBarComboBoxButton` maintains an internal item list (`m_lstItems`) separate from the `CComboBox` control's items. The button's list is the authoritative source — the control may not exist or be populated when the toolbar first paints.

**Key design decisions:**
- `CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS` — non-editable, owner-draw with fixed item height, stores display strings
- Image index = `-1` (no toolbar bitmap — the combo itself is the visual)
- Items are always rebuilt dynamically via `BuildItemList()`, never persisted as a list

### EoCtrlColorOwnerDrawCombo (Dropdown Control)

Created by `CreateCombo()` override. The control is stripped of all system chrome:
- `WS_EX_CLIENTEDGE` removed (no 3D border)
- `WS_BORDER` removed
- `SetWindowTheme(L"", L"")` disables visual styles entirely
- `OnNcCalcSize` is a no-op — entire window is client area

This gives full painting control to `OnPaint()` (closed state) and `DrawItem()` (dropdown items).

## Item List Design

### Standard Items (Always Present)
| Display Text | Item Data | Meaning |
|-------------|-----------|---------|
| By Layer | `COLOR_BYLAYER` (256) | Inherit from layer |
| By Block | `COLOR_BYBLOCK` (0) | Inherit from block reference |
| red | 1 | ACI 1 |
| yellow | 2 | ACI 2 |
| green | 3 | ACI 3 |
| cyan | 4 | ACI 4 |
| blue | 5 | ACI 5 |
| magenta | 6 | ACI 6 |
| white | 7 | ACI 7 |
| More Colors... | `kMoreColors` (`(DWORD_PTR)(-1)`) | Opens `EoDlgSetupColor` |

### Dynamic Custom Entry (Conditional)
When the active pen color is ACI 8–255, `BuildItemList()` inserts an additional entry **before** "More Colors..." showing the numeric index (e.g., "42") with a filled color swatch from `Eo::ColorPalette`. Only one custom entry exists at a time — selecting a different custom color via the dialog replaces it.

### BuildItemList / SetCurrentColor Interaction
```
SetCurrentColor(aciIndex)
  ├── SelectItem(aciIndex)  ← fast path: item already in list
  │   └── found? → done
  └── BuildItemList(aciIndex)  ← rebuild with custom entry
      ├── RemoveAllItems()
      ├── add standard items
      ├── add custom entry if 8 ≤ aciIndex ≤ 255
      ├── add "More Colors..."
      └── SelectItem(aciIndex)
```

## Dark Theme Integration

### Painting Layers (from bottom to top)

1. **Toolbar background** — `EoMfVisualManager::OnFillBarBackground` fills with `toolbarBackground`
2. **Combo border** — `EoMfVisualManager::OnDrawComboBorder` draws 1px `borderColor` frame (no 3D effect)
3. **Combo interior** — `EoCtrlColorComboBox::OnDraw` fills with `paneBackground` (deflated 2px inside border)
4. **Dropdown button** — `EoMfVisualManager::OnDrawComboDropButton` fills with `paneBackground`, draws DPI-scaled arrow in `paneText`
5. **Selected item content** — `OnDraw` renders color swatch + text directly on the toolbar DC using the button's cached item list
6. **Dropdown list background** — `OnCtlColor(CTLCOLOR_LISTBOX)` returns `menuBackground` brush
7. **Dropdown items** — `DrawItem` uses `menuBackground` (normal), `menuHighlightBackground` (hover), `menuHighlightBorder` (hover frame)

### Why OnDraw Renders Directly

MFC's default `CMFCToolBarComboBoxButton::OnDraw` calls `FillSolidRect(clrWindow)` — hardcoded white, invisible in dark theme. The override replaces this with scheme-aware rendering and reads from the **button's** internal item list (`CMFCToolBarComboBoxButton::GetCurSel()` / `GetItem()` / `GetItemData()`), not the `CComboBox` control. This is critical because the control may not be populated when the toolbar first paints after deserialization.

## Toolbar State Serialization

### The Problem

MFC persists toolbar state to the Windows registry via `CMFCToolBar::SaveState()` / `LoadState()`. Each button is serialized independently into its own `CMemFile` blob:

```
SaveState:  CMemFile → CArchive::WriteClass(pRuntimeClass) → button->Serialize(ar) → registry
LoadState:  registry → CMemFile → CArchive::ReadClass() → CreateObject() → button->Serialize(ar)
```

`CMFCToolBarComboBoxButton::Serialize` writes combo items as:
```
CMFCToolBarButton base data
m_iWidth          (int)
m_dwStyle         (DWORD)
nCount            (int)        ← number of items
{ text (CString) + data (DWORD) } × nCount
m_iSelEntry       (int)        ← selected index
```

This causes **three problems** for `EoCtrlColorComboBox`:

1. **DWORD_PTR truncation**: Item data is cast to `DWORD` on save — on x64, `kMoreColors` (`0xFFFFFFFFFFFFFFFF`) becomes `0xFFFFFFFF`. While the truncation is consistent (both read and write use DWORD), the sentinel comparison `itemData == kMoreColors` fails after deserialization because `0x00000000FFFFFFFF ≠ 0xFFFFFFFFFFFFFFFF`.

2. **Item duplication**: The default constructor calls `PopulateItems()` → `BuildItemList()` adding ~10 items. When MFC deserializes, `CreateObject()` calls the constructor (adding items), then `Serialize()` adds the same items again from the archive. The list grows: 10 → 20 → 30 items across successive save/load cycles.

3. **Cascading toolbar corruption**: When the color combo's deserialization produces bad state, MFC's toolbar restoration fails for subsequent buttons too. `CMFCToolBarButton` objects created via `CreateObject()` during the failed load are leaked — the debug output shows ~13 leaked `CMFCToolBarButton` objects (136 bytes each) plus one `EoCtrlFindComboBox` (336 bytes).

### The Fix: Custom Serialize Override

`EoCtrlColorComboBox` overrides `Serialize` to bypass `CMFCToolBarComboBoxButton::Serialize` entirely:

```cpp
void EoCtrlColorComboBox::Serialize(CArchive& ar) {
  CMFCToolBarButton::Serialize(ar);   // grandparent — skips combo item serialization

  if (ar.IsStoring()) {
    ar << m_iWidth;
    ar << (DWORD)m_dwStyle;
    ar << selectedAciColor;            // int32_t — no item list, no DWORD_PTR
  } else {
    // Read width + style + color, then:
    BuildItemList(activeColor);        // rebuild fresh — no duplicates
    SelectItem(activeColor);
  }
}
```

**Schema versioning**: `IMPLEMENT_SERIAL(EoCtrlColorComboBox, CMFCToolBarComboBoxButton, VERSIONABLE_SCHEMA | 2)` allows reading old schema 1 data (written by the base class before the override existed). The load path detects the schema:
- **Schema ≤ 1** (old format): Consumes all base-class fields (width, style, items, selection index) to advance the archive position, then rebuilds items fresh.
- **Schema 2+** (new format): Reads width, style, and the saved ACI color directly.

In both cases, `BuildItemList()` starts with `RemoveAllItems()`, clearing any items added by the constructor.

### Toolbar Initialization Flow

```
First run (clean registry):
  LoadState() → no saved state → RestoreOriginalState()
  → AFX_WM_RESETTOOLBAR → OnToolbarReset()
  → ReplaceButton(ID_PENCOLOR_COMBO, EoCtrlColorComboBox())

Subsequent runs:
  LoadState() → saved state found → CreateObject() + Serialize()
  → EoCtrlColorComboBox reconstructed from archive
  (OnToolbarReset is NOT called)
```

## Key Files

| File | Role |
|------|------|
| `EoCtrlColorComboBox.h` | Class declarations for `EoCtrlColorComboBox` and `EoCtrlColorOwnerDrawCombo` |
| `EoCtrlColorComboBox.cpp` | Full implementation: item management, serialization, owner-draw painting, selection handling |
| `EoMfVisualManager.h/.cpp` | `OnDrawComboBorder` and `OnDrawComboDropButton` overrides for flat dark theme |
| `MainFrm.cpp` | `OnToolbarReset()` — `ReplaceButton` insertion; `SyncColorCombo()` — syncs combo to render state |
| `AeSysViewRender.cpp` | `UpdateStateInformation(Pen)` → calls `SyncColorCombo()` |
| `EoDlgSetupColor.h/.cpp` | Full ACI color selection dialog (256 colors + By Layer + By Block) |
| `EoCtrlColorsButton.h/.cpp` | Owner-draw button control used inside `EoDlgSetupColor` for color grid display |

## Adaptability to Other Contexts

### Current Coupling

`EoCtrlColorComboBox` is tightly coupled to the **toolbar infrastructure**:
- Inherits `CMFCToolBarComboBoxButton` — carries toolbar-specific members (`m_iWidth`, `m_nID`, `m_rectCombo`, `m_rectButton`)
- `OnDraw` renders into the toolbar's DC, reading from the button's internal item list
- `Serialize` handles MFC toolbar state persistence
- `DECLARE_SERIAL` / `IMPLEMENT_SERIAL` enables `CRuntimeClass::CreateObject()` for toolbar deserialization

`EoCtrlColorOwnerDrawCombo` is the reusable visual layer — it handles owner-draw painting, color swatches, dark theme colors, and DPI-scaled arrow glyphs using only `CComboBox` APIs.

### Target Use Cases

#### 1. Setup → Pen Color (accelerator `P`)

**Current implementation** (`AeSysDocCommands.cpp`):
```cpp
void AeSysDoc::OnSetupPenColor() {
  EoDlgSetupColor Dialog;
  Dialog.m_ColorIndex = renderState.Color();
  if (Dialog.DoModal() == IDOK) {
    renderState.SetColor(nullptr, Dialog.m_ColorIndex);
    AeSysView::GetActiveView()->UpdateStateInformation(AeSysView::Pen);
  }
}
```

Opens the full 256-color `EoDlgSetupColor` dialog. This is a **command handler**, not a persistent UI element. An `EoCtrlColorOwnerDrawCombo` could replace or supplement this:
- Embed a combo in a small modal dialog as a quick-pick, with "More Colors..." still opening the full dialog
- Or: replace the menu command with a non-modal dropdown anchored at the cursor position

**Adaptation path**: Create a standalone dialog class containing an `EoCtrlColorOwnerDrawCombo`, pre-populated with the same items as `BuildItemList()`. The item-building logic and `AciToColorRef()` / `AciToName()` helpers are static or free functions — no toolbar dependency.

#### 2. File Manager Dialog — Layer Color Column

**Current implementation** (`EoDlgFileManage.cpp`):
```cpp
case Color: {
  EoDlgSetupColor dialog;
  dialog.m_ColorIndex = layer->ColorIndex();
  if (dialog.DoModal() == IDOK) {
    layer->SetColorIndex(dialog.m_ColorIndex);
  }
  break;
}
```

Clicking the Color column opens the full dialog. A dropdown combo embedded in the list control cell (or a popup combo anchored at the click point) would be faster for common colors.

**Adaptation path**: On `NM_CLICK` to the Color column, create a temporary `EoCtrlColorOwnerDrawCombo` positioned over the cell, populate it, and handle `CBN_SELCHANGE`. This is the "edit-in-place" pattern already used by `EoMfStatusBar` for the Length/Angle panes.

### Reuse Strategy

The toolbar button class (`EoCtrlColorComboBox`) is **not directly reusable** in dialogs — it depends on `CMFCToolBarComboBoxButton` infrastructure. The reusable components are:

| Component | Reusable? | Notes |
|-----------|-----------|-------|
| `EoCtrlColorOwnerDrawCombo` | ✅ Yes | Standard `CComboBox` subclass — works in any dialog or popup |
| `BuildItemList()` logic | ✅ Extract | Item population (standard colors + custom entry + More Colors) |
| `AciToColorRef()` | ✅ Already static | Maps ACI index → `COLORREF` via `Eo::ColorPalette` |
| `AciToName()` | ✅ Already static | Maps ACI index → display name |
| `DrawItem()` | ✅ In combo class | Color swatch + text rendering, dark theme aware |
| `kMoreColors` sentinel | ✅ Already static constexpr | Item data marker for "More Colors..." |
| `Serialize` | ❌ Toolbar-only | MFC toolbar state persistence |
| `OnDraw` | ❌ Toolbar-only | Renders on toolbar DC surface |

### Recommended Refactoring Path

1. **Extract a helper function** (free or static) for item population:
   ```cpp
   void PopulateColorCombo(CComboBox& combo, std::int16_t activeAciIndex);
   ```
   This encapsulates the `RemoveAllItems` + standard items + optional custom entry + "More Colors..." pattern. Both `EoCtrlColorComboBox::BuildItemList()` and dialog-hosted combos call it.

2. **Use `EoCtrlColorOwnerDrawCombo` directly** in dialogs — it's a plain `CComboBox` subclass. Create it with `CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS`, call the populate helper, and handle `CBN_SELCHANGE` in the dialog's message map.

3. **For popup/in-place use** (File Manager column click), create a temporary `EoCtrlColorOwnerDrawCombo` window positioned at the click point, populate it, show the dropdown, and destroy on selection or focus loss — same pattern as `EoMfStatusBar`'s edit-in-place.

### Dark Theme Considerations for Dialog Use

`EoCtrlColorOwnerDrawCombo::DrawItem()` already reads from `Eo::SchemeColors(Eo::activeColorScheme)` — it will render correctly in both dark and light schemes without modification. The `OnPaint`, `OnNcPaint`, and `OnCtlColor` overrides also use scheme colors. The only adjustment needed for dialog hosting:
- Call `SetWindowTheme(combo.m_hWnd, L"", L"")` after creation to disable visual styles (matches the toolbar version)
- Remove `WS_EX_CLIENTEDGE` and `WS_BORDER` if a flat borderless look is desired, or keep them for a standard dialog combo appearance with dark-themed dropdown
