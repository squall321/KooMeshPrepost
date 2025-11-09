#pragma once

#include "io/IFileReader.h"
#include "io/FileFormatDetector.h"
#include <memory>
#include <string>
#include <map>
#include <functional>

namespace koomesh {
namespace io {

/**
 * @brief Factory for creating file readers
 *
 * Factory pattern implementation for creating appropriate file readers
 * based on file format. Supports:
 * - Static creation by format type
 * - Auto-detection and creation from filepath
 * - Registration of new readers
 *
 * Example usage:
 * @code
 * // Create reader by format
 * auto reader = FileReaderFactory::create(FileFormat::LSDYNA);
 *
 * // Create reader with auto-detection
 * auto reader = FileReaderFactory::createFromFile("model.k");
 *
 * // Read file (auto-detect format)
 * core::Mesh mesh;
 * bool success = FileReaderFactory::readFile("model.stl", mesh);
 * @endcode
 *
 * Registration example:
 * @code
 * // Register a custom reader
 * FileReaderFactory::registerReader(FileFormat::CUSTOM,
 *     []() { return std::make_unique<CustomReader>(); });
 * @endcode
 */
class FileReaderFactory {
public:
    /**
     * @brief Reader creation function type
     */
    using ReaderCreator = std::function<std::unique_ptr<IFileReader>()>;

    /**
     * @brief Create reader for specified format
     *
     * @param format File format
     * @return Unique pointer to reader, or nullptr if format not supported
     */
    static std::unique_ptr<IFileReader> create(FileFormat format);

    /**
     * @brief Create reader based on file path (auto-detect format)
     *
     * Automatically detects file format and creates appropriate reader
     *
     * @param filepath Path to the file
     * @return Unique pointer to reader, or nullptr if format cannot be detected
     */
    static std::unique_ptr<IFileReader> createFromFile(const std::string& filepath);

    /**
     * @brief Convenience method: Read file with auto-detection
     *
     * Automatically detects format, creates reader, and reads file
     *
     * @param filepath Path to the file
     * @param mesh Mesh object to populate
     * @param options Read options
     * @param progressCallback Optional progress callback
     * @return Read result
     *
     * @throws FileIOException if format detection or reading fails
     */
    static ReadResult readFile(
        const std::string& filepath,
        core::Mesh& mesh,
        const ReadOptions& options = ReadOptions(),
        ProgressCallback progressCallback = nullptr);

    /**
     * @brief Register a custom reader
     *
     * Allows registration of new readers for custom or additional formats
     *
     * @param format File format enum value
     * @param creator Function that creates reader instance
     */
    static void registerReader(FileFormat format, ReaderCreator creator);

    /**
     * @brief Check if format is supported
     *
     * @param format File format
     * @return true if reader is available for this format
     */
    static bool isFormatSupported(FileFormat format);

    /**
     * @brief Get list of supported formats
     *
     * @return Vector of supported file formats
     */
    static std::vector<FileFormat> getSupportedFormats();

private:
    /**
     * @brief Get reader registry
     *
     * Singleton pattern for reader registry
     */
    static std::map<FileFormat, ReaderCreator>& getRegistry();

    /**
     * @brief Initialize default readers
     *
     * Called once to register built-in readers
     */
    static void initializeDefaultReaders();

    /**
     * @brief Flag to track initialization
     */
    static bool s_initialized;
};

} // namespace io
} // namespace koomesh
