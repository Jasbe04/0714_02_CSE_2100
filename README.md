# Advanced-Programming-Laboratory (071402CSE2100)

***

# File Manager Application (GTK+ 3.x)

**Advanced Programming Lab - CSE 2nd Year**
**Version 2.0 | February 2026**

## 1. Project Overview
This is a **GTK-based desktop file manager** designed for Windows that provides a professional graphical interface for essential file system operations. Originally built as a single-file "monolith," this project has been completely refactored into a **modular, maintainable, and scalable architecture** following industry-standard software engineering principles.

### Key Features
*   **Navigation:** Browser-style back/forward navigation and folder traversal.
*   **File Operations:** Copy, cut, paste, delete, and rename functionality.
*   **Search:** Recursive, case-insensitive file searching.
*   **Progress Tracking:** Visual progress bars for file operations with recursive size calculation.
*   **System Integration:** Drive management and automatic icon retrieval based on file type.

## 2. Technology Stack
*   **Language:** C.
*   **GUI Framework:** GTK+ 3.x.
*   **Platform:** Windows (utilizing Windows API for file attributes and operations).
*   **File System APIs:** POSIX (`dirent.h`) and Windows API.

## 3. Architecture Evolution: The Refactoring Journey
The core of this project was the **evolution from a messy codebase to a professional structure**.

| Aspect | Before Refactoring | After Refactoring (Current) |
| :--- | :--- | :--- |
| **Structure** | Monolithic (1,074 lines in `main.c`) | Modular (9 files across 8 modules) |
| **State Management** | 30+ scattered global variables | Centralized `Data` structure (Single Source of Truth) |
| **Naming** | Cryptic (e.g., `p`, `i`, `event1`) | Descriptive snake_case (e.g., `depthLevel`, `backEvent`) |
| **Documentation** | Minimal or none | Comprehensive function-level documentation blocks |
| **Maintainability** | Very Low | High (Separation of Concerns) |

## 4. Project Structure
The code is organized into focused modules, each with a specific responsibility:

```text
file_manager/
├── src/                    # Implementation files
│   ├── main.c              # Entry point & initialization
│   ├── ui.c                # UI setup & event handling
│   ├── nvign.c             # Navigation & directory traversal
│   ├── file_op.c           # File & folder operations
│   ├── menu_op.c           # Context menu logic
│   ├── search.c            # Recursive search functionality
│   ├── icon.c              # System icon management
│   └── prog.c              # Progress tracking utilities
└── include/                # Header files
    └── app_data.h          # Centralized State Structure (Data)
```

## 5. Design Patterns Applied
To ensure professional-grade code, several design patterns were implemented:
*   **Module Pattern:** Logic is encapsulated into `.c`/`.h` pairs.
*   **State Container Pattern:** A central `Data` structure is passed to all functions, eliminating global variables.
*   **Memento Pattern:** Linked lists (`NextPath` and `BackPath`) are used to manage navigation history.
*   **Strategy Pattern:** A unified `CopyProgress` structure handles progress for different operations (copy, move, delete).

## 6. Coding Standards
*   **Naming Conventions:** Follows a strict `snake_case` pattern for functions (e.g., `create_new_folder()`) and `PascalCase` for structures (e.g., `CopyProgress`).
*   **Documentation:** Every function includes a detailed header describing its purpose, parameters, return values, and memory management requirements.
*   **Code Quality:** Consistent 4-space indentation and a maximum line length of 120 characters for readability.

## 7. Conclusion
This project demonstrates the practical application of **SOLID principles** and professional software standards. By transforming a functional but "unclean" monolithic script into a well-documented modular system, the application is now ready for future extension and professional review.
