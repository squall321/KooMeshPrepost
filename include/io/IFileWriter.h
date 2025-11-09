#pragma once

#include "core/Mesh.h"
#include "FileIOException.h"
#include <string>
#include <memory>
#include <functional>

namespace koomesh {
namespace io {

/**
 * @brief Write options for file writer
 */
struct WriteOptions {
    bool writeComments = true;         ///< Include comments in output
    bool prettyPrint = true;           ///< Format output for readability
    bool validateBeforeWrite = true;   ///< Validate mesh before writing
    bool strictMode = false;           ///< Abort on any warning
    int precision = 6;                 ///< Floating-point precision
    bool scientificNotation = false;   ///< Use scientific notation
    size_t progressUpdateFrequency = 1000;  ///< Update progress every N elements
};

/**
 * @brief Write result information
 */
struct WriteResult {
    bool success = false;
    std::string message;

    size_t nodesWritten = 0;
    size_t elementsWritten = 0;
    size_t partsWritten = 0;

    size_t warningCount = 0;
    size_t errorCount = 0;

    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    double writeTimeSeconds = 0.0;
    size_t bytesWritten = 0;

    /**
     * @brief Check if write was successful with no errors
     */
    bool isOk() const {
        return success && errorCount == 0;
    }

    /**
     * @brief Generate summary report
     */
    std::string generateReport() const;
};

/**
 * @brief Abstract file writer interface
 *
 * Strategy pattern: Different file formats implement this interface
 */
class IFileWriter {
public:
    virtual ~IFileWriter() = default;

    /**
     * @brief Check if this writer can write the given format
     *
     * @param filename Path to file
     * @return true if writer can handle this file format
     */
    virtual bool canWrite(const std::string& filename) const = 0;

    /**
     * @brief Get supported file extensions
     *
     * @return List of extensions (e.g., {".k", ".key", ".dyn"})
     */
    virtual std::vector<std::string> supportedExtensions() const = 0;

    /**
     * @brief Get writer format name
     *
     * @return Format name (e.g., "LS-DYNA Keyword")
     */
    virtual std::string formatName() const = 0;

    /**
     * @brief Write mesh to file
     *
     * @param filename Path to file
     * @param mesh Mesh to write
     * @param options Write options
     * @param progressCallback Optional progress callback
     * @return Write result
     *
     * @throws FileOpenException if file cannot be opened
     * @throws FileWriteException if write operation fails
     * @throws OperationCancelledException if user cancels
     */
    virtual WriteResult write(
        const std::string& filename,
        const core::Mesh& mesh,
        const WriteOptions& options = WriteOptions(),
        ProgressCallback progressCallback = nullptr) = 0;

    /**
     * @brief Get current progress (0.0 to 1.0)
     *
     * Thread-safe method to query current progress
     */
    virtual double progress() const = 0;

    /**
     * @brief Cancel current write operation
     *
     * Thread-safe method to request cancellation
     */
    virtual void cancel() = 0;

    /**
     * @brief Check if operation is currently running
     */
    virtual bool isWriting() const = 0;
};

/**
 * @brief Factory for creating file writers
 */
class FileWriterFactory {
public:
    /**
     * @brief Create appropriate writer for file
     *
     * @param filename Path to file
     * @return Unique pointer to writer, or nullptr if no writer found
     */
    static std::unique_ptr<IFileWriter> createWriter(const std::string& filename);

    /**
     * @brief Register a writer implementation
     *
     * @param creator Function that creates writer instance
     */
    using WriterCreator = std::function<std::unique_ptr<IFileWriter>()>;
    static void registerWriter(WriterCreator creator);

    /**
     * @brief Get all registered writers
     */
    static std::vector<std::unique_ptr<IFileWriter>> getAllWriters();

private:
    static std::vector<WriterCreator>& getCreators();
};

} // namespace io
} // namespace koomesh
