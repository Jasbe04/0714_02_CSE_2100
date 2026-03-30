/**
 * @file file_op.hpp
 * @brief OCP + LSP — Open/Closed and Liskov Substitution principles.
 *
 * Original problem (file_op.c):
 *   - One giant copy_function() handled both copy AND cut (move) via an if-flag.
 *   - Adding a new operation (e.g. compress) meant editing existing code.
 *
 * Solution:
 *   - Abstract base class FileOperation defines the interface.
 *   - CopyOperation, MoveOperation (cut+paste), and DeleteOperation are
 *     independent subclasses — adding a new one never touches existing code.
 *   - All subclasses are safely substitutable (LSP): anywhere a FileOperation*
 *     is expected, any subclass works correctly.
 *
 * SOLID principles demonstrated here:
 *   O (OCP) — FileOperation is closed for modification but open for extension.
 *   L (LSP) — CopyOperation, MoveOperation, DeleteOperation are substitutable.
 *   S (SRP) — Each subclass does exactly one thing.
 *   D (DIP) — execute() accepts IProgressReporter*, not a concrete class.
 */

#ifndef FILE_OPERATION_HPP
#define FILE_OPERATION_HPP

#include "interfaces.hpp"
#include <string>

// ── Abstract base (OCP contract) ─────────────────────────────────────────────

/**
 * @brief Abstract base class for all file operations.
 *
 * To add a new operation (e.g. CompressOperation), derive from this class
 * and implement execute().  No existing code needs to change — OCP satisfied.
 */
class FileOperation
{
public:
    /**
     * @brief Execute the file operation.
     * @param reporter Used to show progress feedback during the operation.
     */
    virtual void execute(IProgressReporter *reporter) = 0;

    /** @brief Human-readable name shown in the progress window title. */
    virtual const char *operationName() const = 0;

    virtual ~FileOperation() {}

protected:
    /**
     * @brief Recursively calculates total byte-size of a file or directory.
     * @param path Path to measure.
     * @return Total size in bytes.
     */
    static long long calculateTotalSize(const std::string &path);
};

// ── CopyOperation ─────────────────────────────────────────────────────────────

/**
 * @brief Copies a file or directory tree from @p src to @p dest.
 *
 * LSP guarantee: can replace any FileOperation* without changing caller behaviour.
 */
class CopyOperation : public FileOperation
{
public:
    CopyOperation(const std::string &src, const std::string &dest);

    void        execute(IProgressReporter *reporter) override;
    const char *operationName() const override { return "Copying..."; }

private:
    /**
     * @brief Recursive copy helper.
     * @param src      Source path.
     * @param dest     Destination path.
     * @param total    Total bytes in the whole operation (for progress fraction).
     * @param copied   Running tally of bytes copied so far (in/out).
     * @param reporter Progress reporter to update after each chunk.
     */
    void copyRecursive(const std::string &src,
                       const std::string &dest,
                       long long          total,
                       long long         &copied,
                       IProgressReporter *reporter);

    std::string m_src;
    std::string m_dest;
};

// ── MoveOperation ─────────────────────────────────────────────────────────────

/**
 * @brief Moves a file or directory (copy then delete source).
 *
 * LSP guarantee: substitutable for any FileOperation*.
 * SRP: delegates actual copy to CopyOperation, deletion to DeleteOperation.
 */
class MoveOperation : public FileOperation
{
public:
    MoveOperation(const std::string &src, const std::string &dest);

    void        execute(IProgressReporter *reporter) override;
    const char *operationName() const override { return "Moving..."; }

private:
    std::string m_src;
    std::string m_dest;
};

// ── DeleteOperation ───────────────────────────────────────────────────────────

/**
 * @brief Recursively deletes a file or directory.
 *
 * LSP guarantee: substitutable for any FileOperation*.
 * Can be used standalone (menu delete) or called by MoveOperation.
 */
class DeleteOperation : public FileOperation
{
public:
    explicit DeleteOperation(const std::string &path);

    void        execute(IProgressReporter *reporter) override;
    const char *operationName() const override { return "Deleting..."; }

    /**
     * @brief Static helper so MoveOperation can reuse deletion logic.
     * @param path     Path to delete.
     * @param total    Total bytes (for progress fraction).
     * @param deleted  Running tally of bytes deleted (in/out).
     * @param reporter Progress reporter to update.
     */
    static void deleteRecursive(const std::string &path,
                                long long          total,
                                long long         &deleted,
                                IProgressReporter *reporter);

private:
    std::string m_path;
};

#endif // FILE_OPERATION_HPP
