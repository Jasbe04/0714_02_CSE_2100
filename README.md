# Advanced-Programming-Laboratory (071402CSE2100)

***

# **File Manager Application**

# Executive Summary

This document provides comprehensive documentation of the File Manager application's architectural evolution from a monolithic single-file implementation to a well-structured, modular codebase following professional software engineering principles.

The refactoring demonstrates practical application of:

- **Separation of Concerns** - functional modules with clear responsibilities
- **Consistent Naming Conventions** - descriptive snake_case naming throughout
- **Centralized State Management** - Data structure encapsulating application state
- **Modular Header-Based Organization** - logical grouping of related functionality
- **Comprehensive Documentation** - detailed function-level documentation blocks

# 1. Project Overview

## 1.1 Application Description

The File Manager is a GTK-based desktop application for Windows that provides a graphical interface for file system operations. It supports navigation, file operations (copy/cut/paste/delete), folder creation, search functionality, and drive management with visual progress tracking.

## 1.2 Technology Stack

- **Programming Language:** C
- **GUI Framework:** GTK+ 3.x
- **Platform:** Windows (with Windows API integration)
- **File System APIs:** POSIX (dirent.h) and Windows API

# 2. Architecture Evolution

## 2.1 Before: Monolithic Structure

The original implementation consisted of a single main.c file containing approximately 1074 lines of code with:

- All functionality in one file
- Global variables for state management
- Unclear variable names (p, i, k, l, event1, event2, etc.)
- Mixed concerns within functions
- No documentation or commenting
- Difficult to maintain and extend

## 2.2 After: Modular Architecture

The refactored implementation organizes code into focused modules, each with a specific responsibility:

| Module | Responsibility | LOC |
| --- | --- | --- |
| main.c | Entry point and initialization | 12 |
| ui.c | User interface setup and event handling | 200+ |
| nvign.c | Navigation operations (back/forward/open) | 210 |
| file_op.c | File operations (create folder, copy, paste, cut) | 183 |
| menu_op.c | Context menu operations (rename, delete) | 187 |
| search.c | File search functionality | 71 |
| icon.c | Icon loading and management | 54 |
| prog.c | Progress tracking utilities | 68 |

## 2.3 Directory Structure

```
file_manager/
├── src/                    # Source files
│   ├── main.c              # Entry point
│   ├── file_op.c           # File operations
│   ├── menu_op.c           # Menu operations
│   ├── ui.c                # User interface
│   ├── search.c            # Search functionality
│   ├── nvign.c             # Navigation
│   ├── icons.c             # Icons handling
│   └── prog.c              # Progress bar
├── headers/                # Header files
│   ├── app_data.h          # Structured data definitions
│   ├── file_op.h
│   ├── icon.h
│   ├── menu_op.h
│   ├── nvign.h
│   ├── prog.h
│   ├── search.h
│   └── ui.h
├── icons/                  # Icon assets
└── README.md
```

# 3. Naming Conventions

## 3.1 File Naming Convention

**Pattern:** snake_case with descriptive names

**Format:** `<functionality>_<type>.c` and corresponding `.h` header

| File Name | Purpose | Naming Rationale |
| --- | --- | --- |
| file_op.c/.h | File operations | Abbreviation 'op' for operations is clear and concise |
| menu_op.c/.h | Context menu operations | Distinguishes menu-triggered operations |
| nvign.c/.h | Navigation operations | Abbreviation for 'navigation', handles directory traversal |
| prog.c/.h | Progress tracking | Abbreviation for 'progress', manages progress bars |

## 3.2 Function Naming Convention

**Pattern:** snake_case with descriptive verb phrases

**Structure:** `<action>_<object>` or `<verb>_<noun>`

Examples of well-named functions:

| Function Name | Clear Purpose from Name |
| --- | --- |
| create_new_folder() | Creates a new folder with user input or auto-generated name |
| delete_function_for_cut() | Deletes source files after a cut/move operation |
| update_progress_bar() | Updates GTK progress bar with current copy/delete progress |
| calculate_total_size() | Recursively calculates total size of file/directory |
| get_file_icon() | Retrieves appropriate icon for a file based on its type |
| go_to_parent_folder() | Navigates up one directory level (back button) |
| search_files() | Recursively searches for files matching a query |

## 3.3 Variable Naming Convention

**Pattern:** snake_case for local variables, PascalCase for structures

**Before Refactoring (Unclear):**

```c
int p = 0;
int i = 0;
int k = 0;
int l = 0;
GtkWidget *event1, *event2, *event3, *event4;
GtkWidget *box1, *box2, *box3;
```

**After Refactoring (Descriptive):**

```c
int depthLevel = 0;
int forwardCount = 0;
int isCopyMode = 0;
int isCutMode = 0;
GtkWidget *backEvent, *nextEvent;
GtkWidget *newFolderEvent, *pasteEvent;
GtkWidget *boxSearch, *boxPath;
```

## 3.4 Structure Naming Convention

**Pattern:** PascalCase for type definitions

| Structure Name | Purpose |
| --- | --- |
| Data | Main application state container with all UI widgets and flags |
| CopyProgress | Tracks progress of file copy/delete operations |
| NextPath | Linked list node for forward navigation history |
| BackPath | Linked list node for backward navigation history |

# 4. State Management Architecture

## 4.1 Centralized State Container

The refactored design introduces a central Data structure that encapsulates all application state, eliminating scattered global variables and providing a single source of truth.

### Data Structure Composition

The Data structure is passed as a pointer to all callback functions, ensuring consistent state access throughout the application.

Key State Categories:

1. **UI Widget References:** window, treeview, scrolledWindow, labels, buttons, event boxes
2. **Navigation State:** depthLevel, forwardCount, head1, head2 (linked list pointers)
3. **Clipboard State:** isCopyMode, isCutMode, pathWay1, fName1
4. **Data Model:** store (GtkListStore), iconPixbuf, scaledPixbuf4
5. **Search Components:** searchEntry, searchButton, boxSearch

## 4.2 Navigation History Management

The application implements a dual linked-list system for browser-style back/forward navigation:

- **BackPath (head2):** Stores paths for backward navigation
- **NextPath (head1):** Stores paths for forward navigation
- **depthLevel:** Tracks current directory depth from drives
- **forwardCount:** Number of available forward navigation steps

When navigating backward, the current path is pushed to the forward history (head1). When navigating forward, paths are popped from head1 and pushed to head2.

# 5. Module Organization & Responsibilities

The codebase is organized into focused modules, each with a well-defined responsibility following the Single Responsibility Principle.

## 5.1 main.c - Application Entry Point

**Purpose:** Initialize GTK and application state, then launch the UI

**Lines of Code:** 12

**Responsibilities:**

- Initialize GTK framework
- Allocate and initialize Data structure
- Set initial state (navigation pointers, flags, counters)
- Call set_ui() to build and show the interface

## 5.2 ui.c/.h - User Interface Management

**Purpose:** Handle all GTK UI setup and user interaction events

**Lines of Code:** 200+

**Key Functions:**

- set_ui(): Creates window, loads icons, builds layout, connects signals
- select_directory(): Handles double-click on folders/files
- right_button_click(): Shows context menu on right-click

**Design Notes:**

- Separates UI construction from business logic
- Uses event boxes for clickable icons (back, forward, new folder, paste)
- Connects all callbacks to pass Data structure as user data

## 5.3 nvign.c/.h - Navigation Operations

**Purpose:** Manage directory navigation and drive display

**Lines of Code:** 210

**Key Functions:**

- show_drives(): Displays all available system drives
- open_directory(): Loads and displays contents of current directory
- go_to_parent_folder(): Navigate back one directory level
- go_to_previous_folder(): Navigate forward in history

**Design Features:**

- Maintains navigation history using linked lists
- Updates button opacity based on navigation availability
- Filters hidden and system files
- Loads appropriate icons for files and folders

## 5.4 file_op.c/.h - File Operations

**Purpose:** Handle file and folder creation, copying, cutting, and pasting

**Lines of Code:** 183

**Key Functions:**

- create_new_folder(): Creates folders with auto-naming (New Folder, New Folder (1), etc.)
- paste(): Recursively copies files and folders
- copy_function(): Handles paste button, shows progress window
- delete_function_for_cut(): Removes source after move operation

**Technical Implementation:**

- Uses Windows API for file attributes (GetFileAttributes, SetFileAttributes)
- Handles read-only attributes during deletion
- Progress tracking integrated with CopyProgress structure
- 8KB buffer for efficient file copying

## 5.5 menu_op.c/.h - Context Menu Operations

**Purpose:** Handle operations triggered from right-click context menu

**Lines of Code:** 187

**Key Functions:**

- folder_rename(): Renames selected file/folder
- delete(): Deletes selected item with confirmation
- copy(): Marks item for copy operation
- cut(): Marks item for move operation
- delete_function(): Recursive deletion implementation

**User Experience Features:**

- Confirmation dialog before deletion
- Progress window during deletion
- Error dialogs with system error messages
- Paste button enabled after copy/cut

## 5.6 search.c/.h - Search Functionality

**Purpose:** Implement recursive file search with case-insensitive matching

**Lines of Code:** 71

**Key Functions:**

- search_files(): Recursively searches directory tree
- on_search_clicked(): Initiates search from button click

**Implementation Details:**

- Case-insensitive search using g_utf8_strdown()
- Partial string matching with g_strstr_len()
- Searches subdirectories recursively
- Displays results with appropriate icons

## 5.7 icon.c/.h - Icon Management

**Purpose:** Load and scale file icons based on file types

**Lines of Code:** 54

**Key Function:**

- get_file_icon(): Returns scaled GdkPixbuf for file

**Features:**

- Uses GTK icon theme for system consistency
- Fallback icons for audio/video files
- Automatic scaling to requested size
- Proper memory management (caller frees pixbuf)

## 5.8 prog.c/.h - Progress Tracking

**Purpose:** Manage progress bar updates and size calculations

**Lines of Code:** 68

**Key Functions:**

- update_progress_bar(): Updates GTK progress bar widget
- calculate_total_size(): Recursively calculates total bytes
- on_progress_close(): Handles progress window close events

**Technical Details:**

- Uses _fseeki64() for large file support
- Processes GTK events to keep UI responsive
- Displays percentage in progress bar text

# 6. Documentation Standards

Every function in the refactored codebase includes a comprehensive documentation block following a consistent format.

## 6.1 Documentation Block Template

```c
/*
 * Brief:
 *   Short description of what the function does.
 *
 * Details:
 *   - Additional detail point 1
 *   - Additional detail point 2
 *   - Additional detail point 3
 *
 * Parameters:
 *   param1 - Description of parameter 1
 *   param2 - Description of parameter 2
 *
 * Returns:
 *   Description of return value (if applicable)
 */
```

## 6.2 Example: Comprehensive Documentation

From icon.c:

```c
/*
 * Brief:
 *   Returns the icon for a given file as a GdkPixbuf.
 *
 * Details:
 *   - Uses the system GtkIconTheme to fetch the standard file icon.
 *   - Falls back to generic audio/video/text icons if the file type is unknown.
 *   - Scales the icon to the requested size.
 *
 * Parameters:
 *   filePath - Path to the file
 *   size     - Desired width and height of the icon
 *
 * Returns:
 *   A GdkPixbuf pointer representing the icon.
 *   The caller is responsible for freeing the pixbuf.
 *
 * Notes:
 *   - Handles both uppercase and lowercase file extensions.
 *   - Returns a default "text-x-generic" icon if no theme icon is available.
 */
```

## 6.3 Documentation Benefits

- **Self-Documenting Code:** Functions are understandable without reading implementation
- **Maintenance:** Future developers can quickly understand purpose and usage
- **Debugging:** Clear contracts help identify where problems occur
- **Collaboration:** Team members can work independently on modules

# 7. Design Patterns Applied

The refactored architecture demonstrates several software design patterns commonly used in professional applications.

## 7.1 Module Pattern

**Implementation:** Each .c/.h pair represents a module with related functionality

**Benefit:** Encapsulation of related functions, easier testing and maintenance

## 7.2 State Container Pattern

**Implementation:** Data structure passed through all functions as application state

**Benefit:** Eliminates global variables, makes state dependencies explicit

## 7.3 Callback Pattern

**Implementation:** GTK event handlers receive Data pointer via gpointer

**Benefit:** Decouples UI events from state management

## 7.4 Memento Pattern (Navigation History)

**Implementation:** Linked lists (NextPath, BackPath) store navigation states

**Benefit:** Enables undo/redo-style navigation without coupling to UI

## 7.5 Strategy Pattern (Progress Tracking)

**Implementation:** CopyProgress structure passed to different operations

**Benefit:** Same progress tracking works for copy, delete, and move operations

## 7.6 Code Formatting

Consistent formatting improves readability:

- **Indentation:** 4 spaces per level
- **Braces:** Always used, even for single-line blocks
- **Line Length:** Generally kept under 120 characters
- **Spacing:** Blank lines separate logical sections

# 8. Conclusion

The refactoring of the File Manager application from a 1074-line monolithic main.c file to a well-organized, modular architecture represents a significant achievement in software engineering practice.

## 8.1 Key Achievements

| Aspect | Before | After |
| --- | --- | --- |
| File Count | 1 file (main.c) | 9 files (8 modules + entry) |
| State Management | 30+ global variables | 1 Data structure |
| Documentation | Minimal/None | Comprehensive blocks |
| Naming | Cryptic (p, i, k, event1) | Descriptive (depthLevel, backEvent) |
| Testability | Very Low | Good |

## 8.2 Learning Outcomes

This refactoring exercise demonstrates:

- **Practical Application of Theory:** SOLID principles and design patterns in real code
- **Professional Practices:** Documentation, naming, and organization standards
- **Iterative Improvement:** Code quality can be enhanced without changing functionality
- **Maintainable Software:** Structure matters as much as features

## 8.3 Final Thoughts

The journey from a working but monolithic codebase to a clean, modular architecture illustrates the value of software engineering principles. While the application's functionality remains unchanged, its maintainability, extensibility, and professional quality have improved dramatically.

This refactoring serves as a foundation for future development and a template for organizing complex C applications with GTK. The patterns and practices demonstrated here are applicable to projects of any scale and can be adapted to other frameworks and languages.

---
