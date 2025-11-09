#pragma once

#include "core/Mesh.h"
#include "FileIOException.h"
#include <string>
#include <memory>
#include <functional>

namespace koomesh {
namespace io {

/**
 * @brief Progress callback function
 *
 * @param current Current progress (0.0 to 1.0)
 * @param message Optional progress message
 * @return true to continue, false to cancel
 */
using ProgressCallback = std::function<bool(double current, const std::string& message)>;

/**
 * @brief Read options for file reader
 */
struct ReadOptions {
    bool validateOnRead = true;        ///< Validate mesh after reading
    bool strictMode = false;           ///< Abort on any warning
    bool skipInvalidElements = false;  ///< Skip elements with invalid nodes
    bool mergeNodes = false;           ///< Merge duplicate nodes
    double mergeTolerance = 1e-6;      ///< Tolerance for node merging
    size_t progressUpdateFrequency = 1000;  ///< Update progress every N lines
};

/**
 * @brief Read result information
 */
struct ReadResult {
    bool success = false;
    std::string message;

    size_t nodesRead = 0;
    size_t elementsRead = 0;
    size_t partsRead = 0;

    size_t nodesSkipped = 0;
    size_t elementsSkipped = 0;

    size_t warningCount = 0;
    size_t errorCount = 0;

    std::vector<std::string> warnings;
    std::vector<std::string> errors;

    double readTimeSeconds = 0.0;

    /**
     * @brief Check if read was successful with no errors
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
 * @brief Abstract file reader interface
 *
 * Strategy pattern: Different file formats implement this interface
 */
class IFileReader {
public:
    virtual ~IFileReader() = default;

    /**
     * @brief Check if this reader can handle the given file
     *
     * @param filename Path to file
     * @return true if reader can handle this file format
     */
    virtual bool canRead(const std::string& filename) const = 0;

    /**
     * @brief Get supported file extensions
     *
     * @return List of extensions (e.g., {".k", ".key", ".dyn"})
     */
    virtual std::vector<std::string> supportedExtensions() const = 0;

    /**
     * @brief Get reader format name
     *
     * @return Format name (e.g., "LS-DYNA Keyword")
     */
    virtual std::string formatName() const = 0;

    /**
     * @brief Read mesh from file
     *
     * @param filename Path to file
     * @param options Read options
     * @param progressCallback Optional progress callback
     * @return Read result with mesh data
     *
     * @throws FileOpenException if file cannot be opened
     * @throws FileReadException if read operation fails
     * @throws InvalidFormatException if file format is invalid
     * @throws OperationCancelledException if user cancels
     */
    virtual ReadResult read(
        const std::string& filename,
        core::Mesh& mesh,
        const ReadOptions& options = ReadOptions(),
        ProgressCallback progressCallback = nullptr) = 0;

    /**
     * @brief Get current progress (0.0 to 1.0)
     *
     * Thread-safe method to query current progress
     */
    virtual double progress() const = 0;

    /**
     * @brief Cancel current read operation
     *
     * Thread-safe method to request cancellation
     */
    virtual void cancel() = 0;

    /**
     * @brief Check if operation is currently running
     */
    virtual bool isReading() const = 0;
};

/**
 * @brief Factory for creating file readers
 */
class FileReaderFactory {
public:
    /**
     * @brief Create appropriate reader for file
     *
     * @param filename Path to file
     * @return Unique pointer to reader, or nullptr if no reader found
     */
    static std::unique_ptr<IFileReader> createReader(const std::string& filename);

    /**
     * @brief Register a reader implementation
     *
     * @param creator Function that creates reader instance
     */
    using ReaderCreator = std::function<std::unique_ptr<IFileReader>()>;
    static void registerReader(ReaderCreator creator);

    /**
     * @brief Get all registered readers
     */
    static std::vector<std::unique_ptr<IFileReader>> getAllReaders();

private:
    static std::vector<ReaderCreator>& getCreators();
};

} // namespace io
} // namespace koomesh
