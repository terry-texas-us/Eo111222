
# EoDdeLib — DDE Server Integration in AeSys

## Overview

AeSys exposes a Windows DDEML (Dynamic Data Exchange Management Library) server under the service name **`AeSys`**. The DDE infrastructure is split between the **EoDdeLib** static library (generic server engine) and **AeSys** application code (domain-specific topics, items, and commands).

## Architecture

### EoDdeLib — `Core Libraries\EoDdeLib`

> Static library — no MFC, no app dependencies

| File | Responsibility |
|------|---------------|
| `dde.h` | Types, structs, callback typedefs, all function declarations (library-internal only — no app-specific declarations) |
| `Dde.cpp` | `Initialize` / `Uninitialize`, `StdCallback`, `DoCallback`, command parsing, `PostAdvise` |
| `DdeItem.cpp` | `ItemAdd` / `ItemFind` / `ItemRemove` |
| `DdeTopic.cpp` | `TopicAdd` / `TopicFind` / `TopicRemove` |
| `DdeConversation.cpp` | `ConversationAdd` / `ConversationFind` / `ConversationRemove` |
| `DdeSys.cpp` | System topic handlers (`SysReqItems`, `SysReqFormats`, etc.) |
| `DdeStringUtil.h` | Declarations for string utility functions |
| `DdeStringUtil.cpp` | `SkipWhiteSpace`, `ScanForChar`, `ScanForString` |

*Linked to AeSys via ProjectReference*

---

### AeSys — `Application\AeSys`

| File | Responsibility |
|------|---------------|
| `DdeAeSysRegistration.cpp` | `RegisterAeSysTopics()` — wires everything up (forward-declared locally, not in `dde.h`); `OnDdeInitError()` — MFC error callback; `OnDdeFallbackExec()` — `WM_CHAR` `PostMessage` |
| `DdeCmds.cpp` / `ddeCmds.h` | Execute command handlers (`ExecLine`, `ExecFill`, `ExecTracingOpen`, `ExecFileGet`, etc.) |
| `DdeGItms.cpp` / `ddeGItms.h` | Item request/poke handlers and `PITEMINFO` externs (`DimAngZInfo`, `ScaleInfo`, etc.) |

## Compile-Time Control

DDE support is controlled by `#define USING_DDE` in `Stdafx.h`. When undefined, all DDE code compiles out of AeSys and `<ddeml.h>` is not included. EoDdeLib always defines `USING_DDE` in its project preprocessor definitions.

## Lifecycle

| Event | Location | Call |
|-------|----------|------|
| Startup | `AeSys::InitInstance()` | `dde::RegisterAeSysTopics()` — calls `dde::Initialize(L"AeSys", ...)` then registers all topics, items, and commands |
| Shutdown | `AeSys::ExitInstance()` | `dde::Uninitialize()` — unregisters service, frees handles |

## Topics and Items

### System Topic (`SZDDESYS_TOPIC`)

Standard DDEML system topic with items: `SysItems`, `Formats`, `Help`, `Topics`, `Protocols`. Three execute commands are also registered here: `TracingOpen`, `TracingMap`, `TracingView`.

### General Topic

| Item | Request | Poke | Advised From |
|------|---------|------|--------------|
| `DimAngZ` | Current dimensioned angle (Z) | ✔ | `AeSysView` |
| `DimLen` | Current dimensioned length | ✔ | `AeSysView` |
| `EngAngZ` | Engaged angle (Z) | — | `EoDbLine`, `EoDbLabeledLine`, `EoDbPolygon`, `EoDbPolyline` |
| `EngLen` | Engaged length | — | Same as `EngAngZ` |
| `ExtNum` | External number | — | `AeSysDoc` |
| `ExtStr` | External string | — | `AeSysDoc` |
| `RelPosX/Y/Z` | Relative cursor position | — | `AeSysView` |
| `Scale` | Current view scale | ✔ | `AeSysView` |

Items marked **Poke ✔** accept values from a DDE client; the rest are read-only. `PostAdvise()` is called after the application updates the corresponding value to notify any advise-linked clients.

### Commands Topic

Execute commands available via `[CommandName(args)]` syntax:

| Command | Description |
|---------|-------------|
| `TracingBlank` | Blank a tracing |
| `TracingMap` | Map a tracing |
| `TracingOpen` | Open a tracing |
| `TracingView` | View a tracing |
| `FileGet` | Open a file |
| `GotoPoint` | Navigate to a point |
| `Line` | Draw a line |
| `Pen` | Set pen properties |
| `Note` / `NoteHT` | Place text / set note height |
| `Send` | Send data |
| `SetPoint` | Set current point |
| `DimAngZ` / `DimLen` | Set dimensioned angle / length |
| `Scale` | Set view scale |
| `Fill` | Fill command |

## Callback Decoupling

EoDdeLib uses two callback function pointers (stored in `SERVERINFO`) to avoid depending on MFC:

- **`PINITERRORFN`** — called on `DdeInitialize` failure; AeSys implements this with `app.WarningMessageBox()`.
- **`PFALLBACKEXECFN`** — called when a topic has no exec function and no command list; AeSys posts `WM_CHAR` to the main window.

## Implementation Notes

### Unicode Conventions

- All DDE string handles are created with **`CP_WINUNICODE`** (not `CP_WINANSI`), matching the wide-string (`const wchar_t*`) parameters throughout.
- All DDE data handles use **`CF_UNICODETEXT`** (not `CF_TEXT`). Byte sizes passed to `DdeCreateDataHandle` and `DdeAddData` account for `sizeof(wchar_t)` per character.
- The format lists (`SysFormatList`, `MyFormats`) and the standard format name table (`CFNames`) advertise `CF_UNICODETEXT`.

### Struct and Type Names

- The conversation-tracking struct is named **`_DDE_CONVERSATIONINFO`** (typedef `CONVERSATIONINFO`) to avoid collision with the Windows SDK `CONVINFO` type from `<ddeml.h>`.

### Allocation Pattern

- All heap-allocated structs (`TOPICINFO`, `ITEMINFO`, `CONVERSATIONINFO`, `EXECCMDFNINFO`) use **`new T{}`** (value-initialization) and **`delete`**. The legacy `new char[sizeof(T)]` + `ZeroMemory` + `delete[]` pattern has been removed.

### Uninitialize Cleanup

- `Uninitialize()` removes all topics by looping **`TopicRemove()`** on the head of the topic list. `TopicRemove` is responsible for freeing each topic's HSZ handles, items, commands, and conversations. This replaces a previous implementation that manually walked the list, freed HSZ handles inline, and then called `TopicRemove` for hardcoded topic names — a pattern that double-freed handles.

### RegisterAeSysTopics Boundary

- `RegisterAeSysTopics()` is defined in `DdeAeSysRegistration.cpp` (AeSys) and forward-declared locally in both `DdeAeSysRegistration.cpp` and `AeSys.cpp`. It is **not** declared in `dde.h` because it is application-specific, not part of the generic library API.
