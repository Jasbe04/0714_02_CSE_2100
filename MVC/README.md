# File Manager — MVC Transformation Documentation

> **Project:** Updated New Project → MVC File Manager | C++ / GTK+3

| Property | Value |
|---|---|
| **Language** | C++17 |
| **UI Toolkit** | GTK+3 / GIO (Windows) |
| **Pattern** | Model-View-Controller (MVC) |
| **From** | SOLID-principled modular design (Updated New Project) |
| **To** | Formal MVC three-layer architecture (MVC File Manager) |
| **Build** | `g++ -std=c++17 ... $(pkg-config --cflags --libs gtk+-3.0 gio-2.0)` |

---

## Table of Contents

1. [Purpose](#1-purpose)
2. [Original Design (Updated New Project)](#2-original-design-updated-new-project)
3. [Class-by-Class Transformation Map](#3-class-by-class-transformation-map)
4. [New Classes Introduced by MVC](#4-new-classes-introduced-by-mvc)
5. [MVC Interface Reference](#5-mvc-interface-reference)
6. [SOLID Principles: Before and After](#6-solid-principles-before-and-after)
7. [MVC Data Flow](#7-mvc-data-flow)
8. [MVC File Inventory](#8-mvc-file-inventory)
9. [Build Instructions](#9-build-instructions)
10. [Summary](#10-summary)

---

## 1. Purpose

This document demonstrates exactly how the File Manager was transformed from its **Updated New Project** state — a SOLID-principled but architecturally flat design — into a formal **Model-View-Controller (MVC)** application. Every class, every interface, and every responsibility mapping is compared side-by-side so the transformation is completely traceable.

The document is structured in three parts:
- An explanation of the problems in the original design that MVC was applied to solve
- A detailed class-by-class mapping of what was removed, merged, split, or added
- A reference section covering the new MVC interfaces, data flow, and build instructions

### MVC Architecture Overview

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                           MVC THREE-LAYER ARCHITECTURE                                 │
│                                                                                        │
│  ┌──────────────────────┐    User Events →    ┌────────────────────────┐              │
│  │         VIEW         │ ──────────────────► │       CONTROLLER       │              │
│  │    FileManagerView   │                     │  FileManagerController │              │
│  │                      │ ◄────────────────── │                        │              │
│  │  • Owns all GTK      │    ← Update View    │  • Routes user events  │              │
│  │    widgets           │                     │  • Calls Model methods │              │
│  │  • Renders file      │                     │  • Calls View updates  │              │
│  │    listings          │                     │  • No business logic   │              │
│  │  • Displays icons    │                     │  • No GTK widgets      │              │
│  │    & paths           │                     │                        │              │
│  │  • Shows progress    │                     │  Files:                │              │
│  │    dialogs           │                     │  controller.hpp/.cpp   │              │
│  │  • Emits UI signals  │                     └───────────┬────────────┘              │
│  │                      │                                 │            ▲              │
│  │  Files:              │                    Calls Model ►│            │ ◄ Model      │
│  │  view.hpp/.cpp       │                                 │            │   Notify     │
│  └──────────────────────┘                                 ▼            │              │
│                                              ┌────────────┴────────────┐              │
│                                              │          MODEL          │              │
│                                              │     FileSystemModel     │              │
│                                              │                         │              │
│                                              │  • Navigation stacks    │              │
│                                              │  • Clipboard state      │              │
│                                              │  • Filesystem queries   │              │
│                                              │  • Business rules only  │              │
│                                              │  • No GTK dependency    │              │
│                                              │                         │              │
│                                              │  Files:                 │              │
│                                              │  model.hpp/.cpp         │              │
│                                              └─────────────────────────┘              │
│                                                                                        │
│  Data Flow: User → View → Controller → Model  |  Notify: Model → Controller → View    │
│  Key: Model has NO GTK dep · View has NO business logic · Controller has NO state      │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

*Figure 1: MVC three-layer architecture showing data flow between View, Controller, and Model*

---

## 2. Original Design (Updated New Project)

The original project was a well-written SOLID refactoring of an earlier C codebase. All five SOLID principles were applied and documented in the source headers. However, the design had **no formal architectural layer separation** — it was structured around services, not around the Model-View-Controller pattern.

### 2.1 File Inventory (Original)

| File | Role |
|---|---|
| `app_state.hpp` / (no .cpp) | Central data struct holding all GTK widgets, pixbufs, navigation stacks, and clipboard flags together. |
| `interfaces.hpp` | Four interfaces: `INavigable`, `IClipboard`, `ISearchable`, `IProgressReporter`. |
| `nvign.hpp` / `nvign.cpp` | `NavigationManager` — implements `INavigable`; navigates directories and updates AppState widgets. |
| `menu.hpp` / `menu.cpp` | `ClipboardManager` — implements `IClipboard`; manages copy/cut/paste using `FileOperation` hierarchy. |
| `search.hpp` / `search.cpp` | `SearchService` — implements `ISearchable`; recursive case-insensitive file search. |
| `menu_op.hpp` / `menu_op.cpp` | `MenuOperations` — rename and delete from the right-click context menu. |
| `folder_creator.hpp/.cpp` | `FolderCreator` — shows a GTK dialog and calls mkdir in the current directory. |
| `icon.hpp` / `icon.cpp` | `IconLoader` — returns a scaled `GdkPixbuf` for a given file path using GIO. |
| `ui.hpp` / `ui.cpp` | `UIController` — builds all GTK widgets, wires all signals, implements all handlers. |
| `prog.hpp` / `prog.cpp` | `ProgressReporter` — GTK progress window, implements `IProgressReporter`. |
| `file_op.hpp` / `file_op.cpp` | `FileOperation` hierarchy: `CopyOperation`, `MoveOperation`, `DeleteOperation`. |
| `main.cpp` | Composition root — creates all services and passes them into `UIController`. |

### 2.2 The AppState Problem

`AppState` was the central data structure passed to almost every class. It contained two fundamentally different categories of data that had no business living together:

| Category | Members |
|---|---|
| **GTK widgets** | `window`, `store`, `treeview`, `scrolledWindow`, `labelPath`, `labelStatus`, `searchEntry`, `searchButton`, `boxSearch`, `boxPath`, `box`, `fullBox`, `parentBox`, `image1`, `image2`, `event1–4` |
| **GdkPixbuf icons** | `iconPixbuf`, `scaledDriveIcon`, `pixbufBack`, `pixbufNext` |
| **Navigation state** | `depth`, `navForward`, `backStack` (std::stack), `forwardStack` (std::stack) |
| **Clipboard state** | `clipboardDir`, `clipboardName`, `isCopy`, `isCut` |

Because `AppState` mixed UI artefacts (GTK widget pointers) with business-logic state (navigation stacks, clipboard flags), it was **impossible to test or use the navigation and clipboard logic without creating a live GTK environment**. This is the exact problem MVC solves.

### 2.3 The UIController Problem

`UIController` had two jobs simultaneously — a violation of SRP that MVC makes structural rather than optional:

- **Job 1 — Widget construction:** `loadPixbufs()`, `buildToolbar()`, `buildTreeView()`, `connectSignals()`, the full GTK window layout.
- **Job 2 — Event routing:** `handleRowActivated()`, `handleRightClick()`, and all the static signal trampolines that called `INavigable`, `IClipboard`, `ISearchable`.

In MVC these two jobs belong to different layers: widget construction and rendering belong to the **View**, and event routing belongs to the **Controller**. Merging them into `UIController` made it impossible to change the routing logic without touching the widget code, and vice versa.

---

## 3. Class-by-Class Transformation Map

Every class in the original design maps to one of four outcomes in the MVC version: **Eliminated**, **Merged**, **Split**, or **Preserved**.

### 3.1 `AppState` → Eliminated

`AppState` is completely removed. Its contents are split by their nature:

| AppState member category | Destination in MVC |
|---|---|
| GTK widget pointers (window, store, treeview, boxes, labels, entries, event boxes) | Moved into `FileManagerView` as private member fields. The View is the only layer that owns widgets. |
| GdkPixbuf pointers (iconPixbuf, scaledDriveIcon, pixbufBack, pixbufNext) | Moved into `FileManagerView`. Pixbufs are display resources, not business data. |
| Navigation state (depth, backStack, forwardStack, navForward) | Moved into `FileSystemModel` as private members `m_depth`, `m_backStack`, `m_forwardStack`. |
| Clipboard state (clipboardDir, clipboardName, isCopy, isCut) | Replaced by `ClipboardState` struct, owned by `FileSystemModel` as `m_clipboard`. |

<table>
<tr>
<th>ORIGINAL (SOLID, no MVC)</th>
<th>MVC REFACTORED</th>
</tr>
<tr>
<td>

```cpp
// app_state.hpp
struct AppState {
  GtkWidget    *window;      // UI
  GtkListStore *store;       // UI
  GtkWidget    *treeview;    // UI
  // ...(12 more widgets)...
  std::stack<std::string> backStack;    // logic
  std::stack<std::string> forwardStack; // logic
  std::string clipboardDir;  // logic
  std::string clipboardName; // logic
  bool isCopy = false;       // logic
  bool isCut  = false;       // logic
  int  depth  = 0;           // logic
};
```

</td>
<td>

```cpp
// model.hpp — business logic only
struct ClipboardState {
  std::string sourceDir, sourceName;
  bool isCopy=false, isCut=false;
  bool hasPending() const;
};
// FileSystemModel owns:
//   m_depth, m_backStack,
//   m_forwardStack, m_clipboard

// view.hpp — GTK widgets only
// FileManagerView owns:
//   m_window, m_store,
//   m_treeview, m_iconPixbuf ...
```

</td>
</tr>
</table>

### 3.2 `NavigationManager` → Merged into `FileSystemModel`

`NavigationManager` implemented `INavigable` and held `openDirectory()`, `showDrives()`, `goToParent()`, `goForward()`. All of these are pure business logic. In MVC, navigation is **Model responsibility**.

| Original method | MVC equivalent |
|---|---|
| `NavigationManager::openDirectory()` | `FileSystemModel::openPath(path, recordHistory)` — takes an explicit path, pushes to back stack, calls `notifyDirectory()`. |
| `NavigationManager::showDrives()` | `FileSystemModel::loadDrives()` — enumerates logical drives, calls `notifyDrives()`. |
| `NavigationManager::goToParent()` | `FileSystemModel::goToParent()` — pops back stack, shows drives at depth 0, calls `notify*`. |
| `NavigationManager::goForward()` | `FileSystemModel::goForward()` — pops forward stack, calls `notifyDirectory()`. |

> The `INavigable` interface is retired. The Controller calls `FileSystemModel` methods directly.

### 3.3 `ClipboardManager` → Merged into `FileSystemModel`

`ClipboardManager` held clipboard state internally, mirroring it into `AppState`. This mirroring was the problem: clipboard state was stored in two places simultaneously. In MVC the Model owns clipboard state in a single `ClipboardState` struct.

| Original | MVC equivalent |
|---|---|
| `ClipboardManager::markCopy(dir,name)` | `FileSystemModel::markCopy(dir,name)` — sets `m_clipboard`, calls `onClipboardChanged()`. |
| `ClipboardManager::markCut(dir,name)` | `FileSystemModel::markCut(dir,name)` — sets `m_clipboard`, calls `onClipboardChanged()`. |
| `ClipboardManager::paste()` | `FileSystemModel::paste()` — executes `CopyOperation` or `MoveOperation`, clears `m_clipboard`, calls `notify*`. |
| `ClipboardManager::hasPending()` | `ClipboardState::hasPending()` — inline method on the plain data struct. |
| `IClipboard` interface | Retired. Controller calls `FileSystemModel` directly. |

### 3.4 `SearchService` → Merged into `FileSystemModel`

`SearchService` wrote results directly into `AppState`'s `GtkListStore` — touching a GTK widget inside an otherwise pure-logic service. In MVC, `FileSystemModel::search()` returns results through the observer callback `onSearchResults(entries)`. The View populates the list store — **the Model never touches a GTK type**.

| Original | MVC equivalent |
|---|---|
| `SearchService::search(query, basePath)` | `FileSystemModel::search(query)` — uses `m_currentPath` as base; calls `observer->onSearchResults(results)`. |
| `SearchService::searchRecursive(dir,q)` | `FileSystemModel::searchRecursive(dir,lowerQuery,results)` — private const helper; no GTK code. |
| `ISearchable` interface | Retired. Controller calls `FileSystemModel::search()` directly. |

### 3.5 `MenuOperations` → Split between Controller and Model

`MenuOperations` provided `rename()` and `deleteItem()` — each a mix of UI dialog code and filesystem calls. MVC draws a clear line: the dialog belongs in the **Controller**; the filesystem call belongs in the **Model**.

| Original method | MVC split |
|---|---|
| `MenuOperations::rename(treePath)` — show dialog + call rename() | **Controller:** `FileManagerController::onRename()` shows `promptRename()` dialog. **Model:** `FileSystemModel::renameEntry(oldName, newName)` calls POSIX `rename()`. |
| `MenuOperations::deleteItem(treePath)` — show confirm + call `DeleteOperation` | **Controller:** `FileManagerController::onDelete()` shows `confirmDelete()` dialog. **Model:** `FileSystemModel::deleteEntry(name)` runs `DeleteOperation`. |

### 3.6 `FolderCreator` → Split between Controller and Model

`FolderCreator::createFolder()` combined a GTK entry dialog with a `mkdir` call. The same MVC split applies:

| Original | MVC split |
|---|---|
| `FolderCreator::createFolder(parentWindow)` — show dialog + mkdir | **Controller:** `FileManagerController::onNewFolder()` calls `promptFolderName()`. **Model:** `FileSystemModel::createFolder(name)` performs `mkdir` with auto-numbering. |

### 3.7 `IconLoader` → Absorbed into `FileManagerView`

`IconLoader` was a pure utility returning a `GdkPixbuf` for a given file path. It was passed into `NavigationManager` and `SearchService`, giving those classes a display dependency they should not have had.

In MVC, icon loading belongs entirely in the **View layer**. The `populateStore()` method inside `FileManagerView::onDirectoryChanged()` handles icon resolution per-entry using GIO content types and GTK icon themes directly. No separate class is needed.

### 3.8 `UIController` → Split into `FileManagerView` + `FileManagerController`

`UIController` was the most impactful split. Its two clearly distinct jobs are now assigned to two different layers:

| UIController method | MVC destination |
|---|---|
| `loadPixbufs()` | `FileManagerView::loadPixbufs()` — View layer, unchanged role. |
| `buildToolbar()` | `FileManagerView::buildToolbar()` — View layer. |
| `buildTreeView()` | `FileManagerView::buildTreeView()` — View layer. |
| `connectSignals()` | `FileManagerView::connectSignals()` — View layer. |
| `buildAndRun()` | `FileManagerView::build()` + `view.run()` — View layer, split into two calls for correct init order. |
| `handleRowActivated()` | `FileManagerController::onItemActivated()` — Controller layer. |
| `handleRightClick()` | `FileManagerView::showContextMenu()` + Controller callbacks — shared by View (menu display) and Controller (action dispatch). |
| `onBackClicked` (static) | `FileManagerView::onBackClicked` (static trampoline) forwards to `controller->onBack()`. |
| `onForwardClicked` | `FileManagerView::onForwardClicked` → `controller->onForward()`. |
| `onNewFolderClicked` | `FileManagerView::onNewFolderClicked` → `controller->onNewFolder()`. |
| `onPasteClicked` | `FileManagerView::onPasteClicked` → `controller->onPaste()`. |
| `onSearchClicked` | `FileManagerView::onSearchClicked` → `controller->onSearch()`. |

### 3.9 `FileOperation` Hierarchy → Preserved, Layer Made Explicit + Minor Extension

The `FileOperation` hierarchy (`CopyOperation`, `MoveOperation`, `DeleteOperation`) was already well-designed following OCP and LSP. It is preserved completely and formally assigned to the **Model layer**. Two small extensions were added:

| Class / Method | Change |
|---|---|
| `CopyOperation` | Constructor gains optional `customName` parameter. |
| `DeleteOperation` | Constructor gains optional `customName` parameter. |
| `calculateTotalSize()` | Moved from `protected` to `public static` to allow `MoveOperation`'s progress tracking to work correctly. |

### 3.10 `ProgressReporter` → Preserved, Extended, Assigned to View Layer

`ProgressReporter` is formally assigned to the **View layer** (it creates a GTK window). Two additions were made:

| Addition | Purpose |
|---|---|
| `showCalculating()` | New `IProgressReporter` method. Shows a "Calculating size..." spinner dialog before a long-running operation begins. |
| `hideCalculating()` | Dismisses the calculating dialog. |
| `set_parent_window()` | Accepts the main window pointer after `view.build()` has run, so the progress dialog is correctly parented. |
| `IProgressReporter` | Gains `showCalculating()` and `hideCalculating()` as new pure virtuals. |

### 3.11 `interfaces.hpp` → Simplified

| Interface | Fate in MVC |
|---|---|
| `INavigable` | **Retired.** Navigation is now `FileSystemModel`'s internal concern. |
| `IClipboard` | **Retired.** Clipboard state lives in the Model. |
| `ISearchable` | **Retired.** Search is a Model method. |
| `IProgressReporter` | **Retained** and extended with `showCalculating()`/`hideCalculating()`. |

A new interface, `IModelObserver`, is added — the push-notification contract the View implements so the Model can broadcast state changes without knowing anything about GTK.

### 3.12 `main.cpp` → Simplified Composition Root

<table>
<tr>
<th>ORIGINAL (SOLID, no MVC)</th>
<th>MVC REFACTORED</th>
</tr>
<tr>
<td>

```cpp
AppState state;
ProgressReporter reporter;
IconLoader icons;
NavigationManager nav(&state, &icons);
ClipboardManager clipboard(&state,
    &reporter, &nav);
SearchService search(&state, &icons);
MenuOperations menuOps(&state,
    &reporter, &nav);
FolderCreator folderCreator;
UIController ui(&state, &nav,
    &clipboard, &search,
    &menuOps, &folderCreator);
ui.buildAndRun();
```

</td>
<td>

```cpp
ProgressReporter reporter;
FileSystemModel model(&reporter);
FileManagerView view;
FileManagerController ctrl(&model, &view);
model.setObserver(&view);
view.setController(&ctrl);
view.build();   // widgets first
reporter.set_parent_window(
    view.window());
model.loadDrives(); // initial state
view.run();    // gtk_main()
```

</td>
</tr>
</table>

> **Important:** `view.build()` must run before `model.loadDrives()` because the drive-list observer callback fires immediately and must find live GTK widgets in the View.

---

## 4. New Classes Introduced by MVC

Three classes are entirely new in the MVC version.

### 4.1 `FileSystemModel`

The **Model**. Consolidates all business logic previously scattered across `NavigationManager`, `ClipboardManager`, `SearchService`, `MenuOperations`, and `FolderCreator`. Contains **zero GTK code**.

| Method | Behaviour |
|---|---|
| `loadDrives()` | Enumerate Windows logical drives via `GetLogicalDrives()`; notify observer. |
| `openPath(path, history)` | `chdir`, push back stack, `listDirectory()`, call `onDirectoryChanged()`. |
| `goToParent()` | Pop back stack, show drives at depth 0, or navigate to previous dir. |
| `goForward()` | Pop forward stack, navigate forward. |
| `createFolder(name)` | `mkdir` with auto-numbering on collision. Returns `bool` success. |
| `markCopy(dir, name)` | Set `m_clipboard` `isCopy=true`; call `onClipboardChanged()`. |
| `markCut(dir, name)` | Set `m_clipboard` `isCut=true`; call `onClipboardChanged()`. |
| `paste()` | Execute `CopyOperation` or `MoveOperation`; clear clipboard; notify. |
| `renameEntry(old, new)` | POSIX `rename()`; refresh directory on success. |
| `deleteEntry(name)` | Run `DeleteOperation` on `CWD+name`; refresh directory. |
| `search(query)` | Recursive case-insensitive search; call `onSearchResults(results)`. |
| `setObserver(observer)` | Register the `IModelObserver` (the View) for push notifications. |

### 4.2 `FileManagerView`

The **View**. Owns every GTK widget and is the only layer that calls GTK functions. Implements `IModelObserver` so the Model can push state without knowing about GTK. **Never calls Model methods** — all input events are forwarded to the Controller.

| Method | Behaviour |
|---|---|
| `build()` | Creates all GTK widgets. Does not show the window or start the event loop. |
| `run()` | Calls `gtk_widget_show_all()` and `gtk_main()`. |
| `onDirectoryChanged()` | Populates `GtkListStore` with `FileEntry` data, updates path label, shows search bar, enables toolbar buttons. |
| `onDrivesChanged()` | Populates store with drive entries; hides search bar; disables directory-only buttons. |
| `onClipboardChanged(state)` | Calls `setPasteEnabled(state.hasPending())` — pure visual update, no logic. |
| `onSearchResults(results)` | Populates store with search results; updates path label with result count. |
| `showContextMenu()` | Builds and shows right-click GTK menu; forwards item activations to Controller. |
| `selectedName()` / `selectedFullPath()` | Reads column 1 or 2 from the currently selected tree row. |
| `nameAtPath(path)` | Returns the filename stored in column 1 at the given `GtkTreePath`. |

### 4.3 `FileManagerController`

The **Controller**. A thin routing layer — reads the minimum information from the View, shows any necessary input dialogs, and calls Model methods.

| Method | Behaviour |
|---|---|
| `onItemActivated(path)` | Reads full path from tree model. Calls `model->openPath()` for directories, `ShellExecute()` for files. |
| `onBack()` | Calls `model->goToParent()`. |
| `onForward()` | Calls `model->goForward()`. |
| `onNewFolder()` | Calls `promptFolderName()`; calls `model->createFolder(name)` if not cancelled. |
| `onPaste()` | Calls `model->paste()`. |
| `onSearch()` | Reads `gtk_entry_get_text(view->searchEntry())`; calls `model->search(query)`. |
| `onRename(path)` | Reads old name from `view->nameAtPath()`; shows `promptRename()`; calls `model->renameEntry()`. |
| `onCopy(path)` | Reads name; calls `model->markCopy(cwd, name)`. |
| `onCut(path)` | Reads name; calls `model->markCut(cwd, name)`. |
| `onDelete(path)` | Reads name; shows `confirmDelete()`; calls `model->deleteEntry(name)`. |
| `promptRename()` | *Private:* GTK entry dialog. Returns new name or empty string. |
| `promptFolderName()` | *Private:* GTK entry dialog. Returns name, empty for auto-name. |
| `confirmDelete()` | *Private:* `GTK_BUTTONS_YES_NO` dialog. Returns `true` if confirmed. |

---

## 5. MVC Interface Reference

### 5.1 `IProgressReporter` — `interfaces.hpp`

Unchanged in purpose, extended with two new methods. The DIP boundary between the Model's file operations and the GTK progress window in the View.

| Method | Contract |
|---|---|
| `show(title)` | Open a progress window labelled with the operation name. |
| `update(fraction)` | Update progress bar to 0.0–1.0; pump GTK event loop. |
| `hide()` | Destroy the progress window. |
| `showCalculating()` | **NEW.** Show a spinner dialog while total file size is computed. |
| `hideCalculating()` | **NEW.** Dismiss the calculating dialog. |

### 5.2 `IModelObserver` — `model.hpp`

New in MVC. The push-notification contract. `FileManagerView` implements it. `FileSystemModel` holds a pointer to it. The Model never knows the concrete type of its observer.

| Callback | Meaning to the View |
|---|---|
| `onDirectoryChanged(entries, path)` | New directory listing is ready; re-render the list store. |
| `onDrivesChanged(drives)` | Updated drive list is ready; switch to drive view. |
| `onClipboardChanged(state)` | Clipboard pending state changed; enable or disable Paste. |
| `onSearchResults(results)` | Search completed; display results in the list store. |

---

## 6. SOLID Principles: Before and After

| Principle | Original | MVC |
|---|---|---|
| **S — SRP** | `AppState` mixed UI and logic. `UIController` built widgets AND routed events. `NavigationManager` updated `AppState` widgets as a side-effect. | Model owns logic only. View owns widgets only. Controller owns routing only. Each class has exactly one reason to change. |
| **O — OCP** | `FileOperation` hierarchy already satisfied OCP. Adding a new file operation required only a new subclass. | `FileOperation` hierarchy unchanged and still satisfies OCP. `CopyOperation` and `DeleteOperation` gain an optional `customName` parameter without changing their interface. |
| **L — LSP** | `CopyOperation`, `MoveOperation`, `DeleteOperation` were substitutable for `FileOperation*` in all contexts. | Unchanged. The three concrete operations remain freely substitutable. |
| **I — ISP** | `INavigable`, `IClipboard`, `ISearchable`, `IProgressReporter` were four small focused interfaces. | `INavigable`, `IClipboard`, `ISearchable` are retired. `IProgressReporter` is retained and focused. `IModelObserver` is added as a new minimal interface. |
| **D — DIP** | `UIController` depended on `INavigable`, `IClipboard`, `ISearchable` — not on concrete managers. `FileOperation` depended on `IProgressReporter`. | `FileSystemModel` depends on `IProgressReporter`. `FileManagerView` depends on `IModelObserver` (via implementing it). |

---

## 7. MVC Data Flow

### 7.1 Navigation Example: User Opens a Folder

| Step | Action | Layer |
|---|---|---|
| 1 | User double-clicks a folder row in the tree view. | View — `GtkTreeView` emits `row-activated` signal. |
| 2 | Static trampoline `onRowActivated()` fires. | View — casts `gpointer` to `FileManagerView*`, calls `ctrl->onItemActivated(path)`. |
| 3 | Controller reads full path from column 2 of the tree model. | Controller |
| 4 | `stat()` shows it is a directory. | Controller — calls `model->openPath(fullPath)`. |
| 5 | Model pushes current path onto `m_backStack`, calls `chdir(path)`. | Model — pure filesystem logic, no GTK. |
| 6 | Model calls `listDirectory()`, builds `vector<FileEntry>`. | Model — no GTK. |
| 7 | Model calls `observer->onDirectoryChanged(entries, path)`. | Model — only knows `IModelObserver*`, not `FileManagerView`. |
| 8 | `View::onDirectoryChanged()` calls `populateStore()`, `setPathLabel()`. | View — populates `GtkListStore`, updates label, shows search bar. |

### 7.2 Clipboard Example: User Copies and Pastes a File

| Step | Action | Layer |
|---|---|---|
| 1 | User right-clicks a file; chooses Copy from context menu. | View — shows context menu; menu item activate calls `ctrl->onCopy(path)`. |
| 2 | Controller reads filename from View. | Controller — calls `view->nameAtPath(path)`, then `getcwd()`. |
| 3 | Controller calls `model->markCopy(cwd, name)`. | Controller |
| 4 | Model sets `m_clipboard`; calls `observer->onClipboardChanged(state)`. | Model — no GTK. |
| 5 | `View::onClipboardChanged()` sets Paste button opacity to 1.0. | View — pure display update. |
| 6 | User navigates to destination folder (see 7.1 flow). | — |
| 7 | User clicks Paste button. | View — calls `ctrl->onPaste()`. |
| 8 | Controller calls `model->paste()`. | Controller |
| 9 | Model runs `CopyOperation` with `m_reporter` for progress. | Model — no GTK (`IProgressReporter` is the abstraction). |
| 10 | Model clears clipboard; calls `onClipboardChanged` + `onDirectoryChanged`. | Model — notifies View of both changes in sequence. |

---

## 8. MVC File Inventory

| File | Layer | Contents |
|---|---|---|
| `headers/model.hpp` | Model | `FileSystemModel`, `FileEntry`, `ClipboardState`, `IModelObserver` declarations. |
| `src/model.cpp` | Model | All business logic: directory listing, drive enumeration, navigation stacks, clipboard, search, file operations. |
| `headers/file_op.hpp` | Model | `FileOperation` hierarchy (`Copy`, `Move`, `Delete`) with `customName` extension. |
| `src/file_op.cpp` | Model | File-operation implementations with per-chunk progress callbacks. |
| `headers/interfaces.hpp` | Shared | `IProgressReporter` (with `showCalculating`/`hideCalculating`). |
| `headers/view.hpp` | View | `FileManagerView`: all GTK widget declarations, `IModelObserver` implementation. |
| `src/view.cpp` | View | Widget construction, rendering (`populateStore`), signal trampolines, context menu. |
| `headers/prog.hpp` | View | `ProgressReporter`: GTK concrete `IProgressReporter` with calculating dialog. |
| `src/prog.cpp` | View | Progress window `show`/`update`/`hide`/`showCalculating`/`hideCalculating`. |
| `headers/controller.hpp` | Controller | `FileManagerController`: all input handler declarations, dialog helper declarations. |
| `src/controller.cpp` | Controller | All routing logic, `promptRename`, `promptFolderName`, `confirmDelete` dialogs. |
| `src/main.cpp` | Root | Composition root: constructs all three layers, wires them, starts event loop. |

---

## 9. Build Instructions

Compile all source files together using C++17 with GTK+3 and GIO pkg-config flags:

```bash
g++ -std=c++17 \
    src/main.cpp       \
    src/model.cpp      \
    src/view.cpp       \
    src/controller.cpp \
    src/file_op.cpp    \
    src/prog.cpp       \
    -Iheaders          \
    $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) \
    -lole32 -o file_manager
```

Run from the project root directory so the `icons/` folder is found at runtime:

```bash
.\file_manager.exe
```

---

## 10. Summary

### What MVC Changed

- `AppState` (god struct mixing widgets and logic) is **eliminated** entirely.
- `NavigationManager`, `ClipboardManager`, `SearchService` are **merged into `FileSystemModel`** — all business logic in one place with no GTK dependency.
- `FolderCreator` and `MenuOperations` are **split**: dialog prompts go to `FileManagerController`, filesystem calls go to `FileSystemModel`.
- `UIController` is **split**: widget construction becomes `FileManagerView`, event routing becomes `FileManagerController`.
- `IconLoader` is **absorbed into `FileManagerView`** — icon loading is a display concern.
- `INavigable`, `IClipboard`, `ISearchable` interfaces are **retired** (the Controller calls the Model directly).
- `IModelObserver` is **introduced** — the push-notification protocol from Model to View.
- `IProgressReporter` is **extended** with `showCalculating()`/`hideCalculating()` for better UX on large operations.
- The startup sequence is made explicit: `build()` → `set_parent_window()` → `loadDrives()` → `run()`.

### What MVC Did Not Change

- The `FileOperation` hierarchy (`CopyOperation`, `MoveOperation`, `DeleteOperation`) — OCP and LSP fully preserved.
- The `IProgressReporter` abstraction between file operations and the GTK progress window — DIP preserved.
- The `ProgressReporter` GTK implementation — only extended, not restructured.
- The GTK signal trampoline pattern (static methods casting `gpointer`) — technique is the same, now in the View.
- The overall user-facing behaviour of the application.

---

*End of Document*
