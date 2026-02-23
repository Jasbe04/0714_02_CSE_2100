# Advanced-Programming-Laboratory (071402CSE2100)

***

# **File Manager Application**

## Design Pattern & Refactoring Documentation

*Comparing Original vs. Refactored Architecture*

---

# 1. Project Overview

This document describes the full design and refactoring journey of a GTK-based Windows file manager written in C. Starting from a single monolithic source file with global variables and inconsistent naming, the project was progressively transformed through three distinct refactoring prompts into a clean, modular, well-documented multi-file C project.

The documentation covers every design decision made, every naming convention applied, every structural pattern introduced, and every prompt used to drive the transformation — so that the complete reasoning and evolution of the codebase is fully traceable.

---

**Scope**

Three refactoring passes were applied to the original main.c: (1) naming conventions + global variable elimination, (2) Doxygen documentation, and (3) modular multi-file decomposition into 8 header/source pairs.

---

# 2. The Original Codebase

The original codebase was a single file, main.c, containing approximately 1,138 lines. It implemented a complete GTK file manager for Windows with the following capabilities:

- Drive enumeration and navigation (Windows API)
- Directory listing with file-type icons from GIO / GTK icon themes
- Forward and backward navigation using two singly-linked-list stacks
- Recursive file copy, cut, and paste with a live progress bar
- Recursive file and folder deletion with progress tracking
- Folder creation with collision-safe auto-numbering
- Inline rename via a modal dialog
- Right-click context menu (Rename / Copy / Cut / Delete)
- Recursive filename search with case-insensitive substring matching

## 2.1 Structural Problems

The original file suffered from several significant structural anti-patterns that made it difficult to maintain, test, and extend:

### 2.1.1 Global Variables

Over 30 variables were declared at file scope, including all GTK widgets, pixbufs, navigation stacks, and state flags. This meant any function could read or modify any piece of state at any time, with no encapsulation or clear ownership.

| **Before (Original)** | **After (Refactored)** |
|---|---|
| `GtkWidget *window;` | `/* removed -- now data->window */` |
| `GtkListStore *store;` | `/* removed -- now data->store */` |
| `int i = 0; /* depth */` | `/* removed -- now data->depth */` |
| `int k = 0; /* copy flag */` | `/* removed -- now data->is_copy*/` |
| `NextPath *head1 = NULL;` | `/* removed -- now data->head_next*/` |

### 2.1.2 Inconsistent Naming

Identifiers mixed several naming styles within the same file: PascalCase for some functions (Paste, Copy, Cut, Delete, Rename), snake_case for others (update_progress_bar, calculate_total_size), cryptic single-letter variables (i, k, l, p), and Hungarian-style prefixes for some widgets (labelPath, boxSearch).

### 2.1.3 Monolithic Structure

All 1,138 lines — struct definitions, utility helpers, navigation logic, UI construction, file I/O, and the main entry point — lived in a single translation unit. There were no headers, no module boundaries, and no enforced interface between concerns.

### 2.1.4 No Documentation

No function carried a comment describing its purpose, parameters, or return value. Maintainers had to read the full body of every function to understand its contract.

# 3. Prompts Used

All three refactoring passes were driven by the following exact prompts provided to the AI assistant. Each prompt is preserved verbatim.

## 3.1 Prompt 1 — Naming Conventions & Global Variable Elimination

---

**PROMPT 1**

*Design the code in such a way that it follows the following naming conventions. Remove the global variables and use them as structure elements instead. Modify the code accordingly.*

*Local variables snake_case*

*Parameters snake_case*

*Functions camelCase*

*Struct names PascalCase*

*Struct members snake_case*

---

## 3.2 Prompt 2 — Doxygen Documentation

---

**PROMPT 2**

*Now add comments for each function that follow the following convention:*

```c
/**
 * @brief Creates a new folder.
 *
 * @param path Path of the parent directory.
 * @param folder_name Name of the folder to create.
 * @return int Returns 0 on success, -1 on failure.
 */
int create_folder(const char *path, const char *folder_name);
```

---

## 3.3 Prompt 3 — Modular Decomposition into Header/Source Pairs

---

**PROMPT 3**

*Now, create eight header files with the following names, each containing the specified function declarations. Also create the corresponding .c source files for each header:*

- **app_data.h** - holds the structures
- **file_op.h** - `void create_new_folder(...); void delete_function_for_cut(...); void paste(...); void copy_function(...);`
- **icon.h** - `GdkPixbuf *get_file_icon(...);`
- **menu_op.h** - `void folder_rename(...); void delete_function(...); void delete(...); void copy(...); void cut(...);`
- **nvign.h** - `void show_drives(...); void open_directory(...); void go_to_parent_folder(...); void go_to_previous_folder(...);`
- **prog.h** - `void update_progress_bar(...); long long calculate_total_size(...); gboolean on_progress_close(...);`
- **search.h** - `void search_files(...); void on_search_clicked(...);`
- **ui.h** - `void select_directory(...); gboolean right_button_click(...); void set_ui(...);`

---

# 4. Design Patterns Applied

## 4.1 Context Object Pattern (Prompt 1)

The most impactful structural change introduced in Prompt 1 was replacing 30+ global variables with a single heap-allocated struct called AppState (later renamed Data). Every callback and helper receives a pointer to this struct instead of reading/writing global state.

---

**Pattern Name**

Context Object (also called State Object or Parameter Object). The pattern bundles related state into a single struct and passes it by pointer through the call graph, eliminating global mutable state.

---

In the original code, any function could silently depend on or mutate globals like `i` (depth), `k` (copy flag), `head1`/`head2` (navigation stacks). The Context Object makes these dependencies explicit — a function that needs navigation state must accept a `Data*` parameter, making the dependency visible at the call site.

**Before (Global State):**

```c
// Global variables anyone can modify
int i = 0;  // depth
int k = 0;  // is_copy
NextPath *head1 = NULL;

void someFunction() {
    // Function silently depends on globals
    if (k) { /* ... */ }
    i++;
}
```

**After (Context Object):**

```c
// All state in one struct
typedef struct {
    int depth;
    int is_copy;
    NextPath *head_next;
    // ... 30+ other fields
} Data;

void someFunction(Data *data) {
    // Dependencies explicit at call site
    if (data->is_copy) { /* ... */ }
    data->depth++;
}
```

## 4.2 Module Pattern (Prompt 3)

The third refactoring pass applied a variant of the Module pattern by splitting the monolithic file into eight cohesive header/source pairs. Each module exposes a small set of related functions and hides implementation details in the .c file.

**Modules Created:**

| Module | Responsibility | Lines |
|---|---|---|
| **app_data.h** | Struct definitions (Data, CopyProgress, NextPath, BackPath) | ~80 |
| **ui.h/.c** | GTK window construction and event wiring | ~350 |
| **nvign.h/.c** | Directory navigation and drive enumeration | ~280 |
| **file_op.h/.c** | File copy, paste, cut, and folder creation | ~250 |
| **menu_op.h/.c** | Context menu actions (rename, delete, copy, cut) | ~220 |
| **search.h/.c** | Recursive file search | ~120 |
| **icon.h/.c** | File icon loading via GIO content-type lookup | ~90 |
| **prog.h/.c** | Progress bar updates and size calculations | ~110 |

## 4.3 Facade Pattern (ui.h)

The ui.h module serves as a facade that integrates all other modules. It includes nvign.h, menu_op.h, file_op.h, and search.h, and provides a single entry point `set_ui(Data*)` that constructs the full interface. This shields main.c from knowing about the internal modules.

## 4.4 Separation of Concerns

Each module has a single, well-defined responsibility:

- **nvign** knows only about directory traversal — it never touches file I/O
- **file_op** knows only about file operations — it never constructs UI widgets
- **icon** is a pure utility — it has no dependency on app_data.h or global state
- **prog** handles progress tracking — it's reusable for any long-running operation

# 5. Before-and-After Comparison

## 5.1 File Structure

| **Before (Original)** | **After (Refactored)** |
|---|---|
| 1 file (main.c) | 17 files (8 headers + 8 sources + 1 main) |
| ~1,138 lines in one file | ~1,500 lines total, split across modules |
| No module boundaries | Clear module interfaces via headers |

## 5.2 State Management

| **Before (Original)** | **After (Refactored)** |
|---|---|
| 30+ global variables | 1 Data struct passed by pointer |
| Silent side effects everywhere | State changes visible at call site |
| No ownership — anyone can mutate | Owner is caller who holds Data* |

## 5.3 Documentation

| **Before (Original)** | **After (Refactored)** |
|---|---|
| Zero function comments | Every function has @brief/@param/@return |
| Purpose guessed from body | Purpose described at declaration |
| No parameter descriptions | All parameters documented with types |
| No return-value descriptions | Return values and error states documented |

## 5.4 Naming

| **Before (Original)** | **After (Refactored)** |
|---|---|
| Mixed: Paste, update_progress_bar | Uniform camelCase for all functions |
| `int i, int k, int l, int p` | `depth, is_copy, is_cut, nav_forward` |
| `FName1, pathway1` | `fname1, pathway1` |
| `struct nextPath` (lowercase) | `typedef struct NextPath` (PascalCase) |

# 6. Module Dependency Graph

The diagram below shows which modules each source file includes. Arrows represent `#include` dependencies. `app_data.h` is included by every module and sits at the base of the dependency tree.

```
main.c
  |
  v
ui.h/.c
  |
  +----------+----------+
  |          |          |
  v          v          v
nvign.h  menu_op.h  file_op.h  search.h
  |          |          |          |
  +----------+----------+----------+
             |          |
             v          v
          prog.h     icon.h
             |
             v
     app_data.h (foundation — included by all)
```

Notable dependency rules enforced during decomposition:

- `app_data.h` has no local dependencies — it includes only GTK and Windows headers.
- `icon.h` has no dependency on `app_data.h` — it operates on file paths and GTK types only.
- `prog.h` depends only on `app_data.h` (for CopyProgress) and GTK.
- `ui.c` is the integration layer — it includes nvign.h, menu_op.h, file_op.h, and search.h.
- `main.c` includes only `app_data.h` and `ui.h`, keeping the entry point minimal.

# 7. Step-by-Step Evolution

## 7.1 Pass 1: Context Object + Naming Conventions

Applied by Prompt 1. The codebase remained a single file but was substantially restructured:

1. All 30+ global variables removed.
2. A new struct AppState (Data) created to hold all former globals.
3. All functions updated to accept `AppState*` / `Data*` as a parameter.
4. All identifiers renamed to follow snake_case / camelCase / PascalCase rules.
5. Cryptic abbreviations (i, k, l, p) replaced with descriptive names.
6. GTK callbacks now receive state through `gpointer data` instead of globals.

## 7.2 Pass 2: Doxygen Documentation

Applied by Prompt 2. No logic was changed — only comments were added:

1. Every function declaration received a `@brief/@param/@return` block.
2. Every function definition received the same block immediately above it.
3. GTK signal-handler parameters that are unused were annotated with '(unused)'.
4. Struct members received inline `/** ... */` documentation.

## 7.3 Pass 3: Modular Decomposition

Applied by Prompt 3. The single documented file was split into 17 files:

1. 8 header files (.h) — one per module, each with include guards and Doxygen comments.
2. 8 source files (.c) — each including only its own header and its direct dependencies.
3. 1 minimal main.c — `gtk_init`, `g_new0(Data)`, `set_ui()`, `g_free`. Under 40 lines.
4. The struct name AppState was renamed to Data for consistency with the header signatures specified in the prompt.
5. The function `set_ui()` was introduced in ui.h to encapsulate the entire UI construction that was previously inlined in main().
6. The `onPasteClicked` wrapper was merged into `copy_function()` by changing its signature to match a `GdkEventButton` callback directly.

# 8. Benefits Achieved

## 8.1 Maintainability

- A developer looking for navigation logic goes directly to `nvign.c`.
- A developer looking for icon loading goes directly to `icon.c`.
- Changes to one module require recompiling only that module and its dependents.

## 8.2 Readability

- Every function name describes what the function does (`open_directory` vs `openDirectory` is equivalent, but `copy` vs `Copy`, `delete` vs `Delete`, `cut` vs `Cut` now follow the same convention as the rest).
- Every parameter name describes what the parameter represents.
- Every function has a one-sentence description visible at the declaration.

## 8.3 Testability

- `calculate_total_size` and `get_file_icon` are now pure utility functions in standalone modules — they can be unit-tested independently of the GTK event loop.
- `search_files` can be tested by passing a temporary directory and a mock GtkListStore.

## 8.4 Extensibility

- Adding a new file operation requires only adding a new function to `file_op.h` and `file_op.c`.
- Adding a new navigation feature requires only modifying `nvign.h` and `nvign.c`.
- The Data struct can be extended with new fields without touching any function signatures.

---
