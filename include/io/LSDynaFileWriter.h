#pragma once

#include "io/IFileWriter.h"
#include "io/IFileReader.h"  // For ProgressCallback definition
#include "io/LSDynaKeywordParser.h"
#include <memory>
#include <fstream>

namespace koomesh {
namespace io {

/**
 * @brief Write options specific to LS-DYNA format
 */
struct LSDynaWriteOptions {
    KeywordFormat format = KeywordFormat::FREE;  ///< Output format
    size_t fieldWidth = 10;                      ///< Field width for fixed format
    int precision = 6;                           ///< Floating point precision
    bool writeHeader = true;                     ///< Write *KEYWORD header
    bool writeEnd = true;                        ///< Write *END footer
    bool writeComments = true;                   ///< Write descriptive comments
    bool sortById = false;                       ///< Sort nodes/elements by ID
};

/**
 * @brief Write context for tracking write operations
 */
struct WriteContext {
    std::string filename;
    size_t lineNumber = 0;
    std::string currentKeyword;
    KeywordFormat format = KeywordFormat::FREE;

    // Statistics
    size_t nodesWritten = 0;
    size_t elementsWritten = 0;
    size_t partsWritten = 0;

    // Errors and warnings
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void addError(const std::string& message) {
        errors.push_back("Line " + std::to_string(lineNumber) + ": " + message);
    }

    void addWarning(const std::string& message) {
        warnings.push_back("Line " + std::to_string(lineNumber) + ": " + message);
    }
};

/**
 * @brief LS-DYNA Keyword file writer
 *
 * Implements IFileWriter for LS-DYNA Keyword format files.
 *
 * Features:
 * - Supports both fixed and free format output
 * - Progressive writing with progress tracking
 * - Detailed error and warning reporting
 * - Configurable field width and precision
 * - Optional comments and headers
 *
 * Example:
 * @code
 * LSDynaFileWriter writer;
 * Mesh mesh;
 * // ... populate mesh ...
 *
 * LSDynaWriteOptions options;
 * options.format = KeywordFormat::FREE;
 * options.writeComments = true;
 *
 * auto result = writer.write("output.k", mesh, options);
 *
 * if (result.success) {
 *     std::cout << "Wrote " << result.nodesWritten << " nodes\n";
 * }
 * @endcode
 */
class LSDynaFileWriter : public IFileWriter {
public:
    /**
     * @brief Constructor
     */
    LSDynaFileWriter();

    /**
     * @brief Destructor
     */
    ~LSDynaFileWriter() override;

    // IFileWriter interface
    bool canWrite(const std::string& filename) const override;
    std::vector<std::string> supportedExtensions() const override;
    std::string formatName() const override;

    WriteResult write(
        const std::string& filename,
        const core::Mesh& mesh,
        const WriteOptions& options = WriteOptions(),
        ProgressCallback progressCallback = nullptr) override;

    /**
     * @brief Write with LS-DYNA specific options
     */
    WriteResult write(
        const std::string& filename,
        const core::Mesh& mesh,
        const LSDynaWriteOptions& options,
        ProgressCallback progressCallback = nullptr);

    double progress() const override;
    void cancel() override;
    bool isWriting() const override;

private:
    // Progress tracking
    mutable std::atomic<double> m_progress{0.0};
    std::atomic<bool> m_isWriting{false};
    std::atomic<bool> m_cancelRequested{false};

    /**
     * @brief Write file header
     */
    void writeHeader(std::ofstream& out, const LSDynaWriteOptions& options);

    /**
     * @brief Write nodes
     */
    void writeNodes(
        std::ofstream& out,
        const core::Mesh& mesh,
        const LSDynaWriteOptions& options,
        WriteContext& context,
        ProgressCallback progressCallback);

    /**
     * @brief Write elements
     */
    void writeElements(
        std::ofstream& out,
        const core::Mesh& mesh,
        const LSDynaWriteOptions& options,
        WriteContext& context,
        ProgressCallback progressCallback);

    /**
     * @brief Write parts
     */
    void writeParts(
        std::ofstream& out,
        const core::Mesh& mesh,
        const LSDynaWriteOptions& options,
        WriteContext& context);

    /**
     * @brief Write footer
     */
    void writeFooter(std::ofstream& out, const LSDynaWriteOptions& options);

    /**
     * @brief Write single node in fixed format
     */
    void writeNodeFixed(
        std::ofstream& out,
        const core::Node& node,
        const LSDynaWriteOptions& options);

    /**
     * @brief Write single node in free format
     */
    void writeNodeFree(
        std::ofstream& out,
        const core::Node& node,
        const LSDynaWriteOptions& options);

    /**
     * @brief Write single element
     */
    void writeElement(
        std::ofstream& out,
        const core::Element& element,
        const LSDynaWriteOptions& options);

    /**
     * @brief Write single part
     */
    void writePart(
        std::ofstream& out,
        const core::Part& part,
        const LSDynaWriteOptions& options);

    /**
     * @brief Get keyword name for element type
     */
    std::string getElementKeyword(core::ElementType type) const;

    /**
     * @brief Update progress and check for cancellation
     */
    bool updateProgress(
        double progress,
        const std::string& message,
        ProgressCallback callback);
};

/**
 * @brief Register LS-DYNA writer with factory
 */
void registerLSDynaWriter();

} // namespace io
} // namespace koomesh
