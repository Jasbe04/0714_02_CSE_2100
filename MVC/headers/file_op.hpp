/**
 * @file file_op.hpp
 * @brief Model layer — OCP + LSP file-operation hierarchy.
 *
 * In the MVC structure this belongs entirely to the Model:
 *   - No GTK widget references.
 *   - No UI logic.
 *   - Depends only on IProgressReporter (an interface, not GTK code).
 *
 * SOLID principles:
 *   O (OCP) — FileOperation is closed for modification, open for extension.
 *   L (LSP) — All subclasses are safely substitutable.
 *   S (SRP) — Each subclass does exactly one thing.
 *   D (DIP) — execute() accepts IProgressReporter*, not the GTK reporter.
 */

#ifndef FILE_OPERATION_HPP
#define FILE_OPERATION_HPP

#include "interfaces.hpp"
#include <string>

// ── Abstract base ─────────────────────────────────────────────────────────────

class FileOperation
{
public:
    virtual void        execute(IProgressReporter *reporter) = 0;
    virtual const char *operationName() const = 0;
    virtual ~FileOperation() {}
    static long long calculateTotalSize(const std::string &path);
    
};

// ── CopyOperation ─────────────────────────────────────────────────────────────

class CopyOperation : public FileOperation
{
public:
    // New constructor with optional custom name
    CopyOperation(const std::string &src, const std::string &dest, const char* customName = nullptr);
    void        execute(IProgressReporter *reporter) override;
    const char *operationName() const override {
        return m_customName ? m_customName : "Copying...";
    }
private:
    void copyRecursive(const std::string &src,
                       const std::string &dest,
                       long long          total,
                       long long         &copied,
                       IProgressReporter *reporter);
    std::string m_src, m_dest;
    const char* m_customName;           // new
};

// ── MoveOperation ─────────────────────────────────────────────────────────────

class MoveOperation : public FileOperation
{
public:
    MoveOperation(const std::string &src, const std::string &dest);
    void        execute(IProgressReporter *reporter) override;
    const char *operationName() const override { return "Moving..."; }

private:
    std::string m_src, m_dest;
};

// ── DeleteOperation ───────────────────────────────────────────────────────────

class DeleteOperation : public FileOperation
{
public:
    // New constructor with optional custom name
    explicit DeleteOperation(const std::string &path, const char* customName = nullptr);
    void        execute(IProgressReporter *reporter) override;
    const char *operationName() const override {
        return m_customName ? m_customName : "Deleting...";
    }
    static void deleteRecursive(const std::string &path,
                                long long          total,
                                long long         &deleted,
                                IProgressReporter *reporter);
private:
    std::string m_path;
    const char* m_customName;           // new
};

#endif // FILE_OPERATION_HPP
