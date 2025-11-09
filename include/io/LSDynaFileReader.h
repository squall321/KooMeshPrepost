#pragma once

#include "io/IFileReader.h"
#include "io/LSDynaKeywordParser.h"
#include "io/MemoryMappedFile.h"
#include <atomic>
#include <memory>

namespace koomesh {
namespace io {

/**
 * @brief LS-DYNA Keyword file reader
 *
 * Implements IFileReader for LS-DYNA Keyword format files.
 * Uses memory-mapped I/O for efficient large file reading.
 *
 * Features:
 * - Memory-mapped file I/O for performance
 * - Progressive parsing with cancellation support
 * - Detailed error and warning reporting
 * - Format auto-detection (fixed vs free)
 * - Thread-safe progress tracking
 *
 * Example:
 * @code
 * LSDynaFileReader reader;
 * Mesh mesh;
 * ReadOptions options;
 *
 * auto result = reader.read("model.k", mesh, options,
 *     [](double progress, const std::string& msg) {
 *         std::cout << "Progress: " << (progress * 100) << "%\n";
 *         return true;  // Continue
 *     });
 *
 * if (result.success) {
 *     std::cout << "Read " << result.nodesRead << " nodes\n";
 * }
 * @endcode
 */
class LSDynaFileReader : public IFileReader {
public:
    /**
     * @brief Constructor
     */
    LSDynaFileReader();

    /**
     * @brief Destructor
     */
    ~LSDynaFileReader() override;

    // IFileReader interface
    bool canRead(const std::string& filename) const override;
    std::vector<std::string> supportedExtensions() const override;
    std::string formatName() const override;

    ReadResult read(
        const std::string& filename,
        core::Mesh& mesh,
        const ReadOptions& options = ReadOptions(),
        ProgressCallback progressCallback = nullptr) override;

    double progress() const override;
    void cancel() override;
    bool isReading() const override;

private:
    // Parser registry
    std::unique_ptr<KeywordParserRegistry> m_registry;

    // Progress tracking
    mutable std::atomic<double> m_progress{0.0};
    std::atomic<bool> m_isReading{false};
    std::atomic<bool> m_cancelRequested{false};

    /**
     * @brief Split file content into lines
     */
    std::vector<std::string> splitIntoLines(const char* data, size_t size);

    /**
     * @brief Detect file format (fixed vs free)
     */
    KeywordFormat detectFormat(const std::vector<std::string>& lines);

    /**
     * @brief Parse all lines into mesh
     */
    void parseLines(
        const std::vector<std::string>& lines,
        core::Mesh& mesh,
        ParseContext& context,
        ProgressCallback progressCallback);

    /**
     * @brief Update progress and check for cancellation
     *
     * @return true to continue, false if cancelled
     */
    bool updateProgress(
        double progress,
        const std::string& message,
        ProgressCallback callback);

    /**
     * @brief Validate mesh after reading
     */
    void validateMesh(
        const core::Mesh& mesh,
        ParseContext& context,
        const ReadOptions& options);
};

/**
 * @brief Register LS-DYNA reader with factory
 */
void registerLSDynaReader();

} // namespace io
} // namespace koomesh
