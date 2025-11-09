#include "io/LSDynaFileReader.h"
#include "io/FileIOException.h"
#include "core/MeshValidator.h"
#include <chrono>
#include <algorithm>
#include <sstream>

namespace koomesh {
namespace io {

// ======================================================================
// LSDynaFileReader Implementation
// ======================================================================

LSDynaFileReader::LSDynaFileReader()
    : m_registry(KeywordParserRegistry::createDefault())
{
}

LSDynaFileReader::~LSDynaFileReader() = default;

bool LSDynaFileReader::canRead(const std::string& filename) const {
    // Check file extension
    auto extensions = supportedExtensions();
    for (const auto& ext : extensions) {
        if (filename.length() >= ext.length()) {
            std::string fileExt = filename.substr(filename.length() - ext.length());
            // Case-insensitive comparison
            std::string extLower = ext;
            std::string fileExtLower = fileExt;
            std::transform(extLower.begin(), extLower.end(), extLower.begin(), ::tolower);
            std::transform(fileExtLower.begin(), fileExtLower.end(), fileExtLower.begin(), ::tolower);
            if (extLower == fileExtLower) {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> LSDynaFileReader::supportedExtensions() const {
    return {".k", ".key", ".dyn", ".keyword"};
}

std::string LSDynaFileReader::formatName() const {
    return "LS-DYNA Keyword";
}

ReadResult LSDynaFileReader::read(
    const std::string& filename,
    core::Mesh& mesh,
    const ReadOptions& options,
    ProgressCallback progressCallback)
{
    ReadResult result;
    result.success = false;

    m_isReading = true;
    m_cancelRequested = false;
    m_progress = 0.0;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Update progress: Opening file
        if (!updateProgress(0.05, "Opening file...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Open file with memory mapping
        MemoryMappedFile mmf(filename, MemoryMapMode::READ_ONLY);

        if (!mmf.isValid()) {
            throw FileOpenException(filename, "Failed to open file");
        }

        // Advise sequential access
        mmf.advise(MemoryMappedFile::Advice::SEQUENTIAL);

        // Update progress: Reading file
        if (!updateProgress(0.1, "Reading file content...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Split into lines
        std::vector<std::string> lines = splitIntoLines(mmf.data(), mmf.size());

        if (lines.empty()) {
            result.message = "File is empty";
            result.errorCount = 1;
            result.errors.push_back("File contains no data");
            return result;
        }

        // Update progress: Detecting format
        if (!updateProgress(0.15, "Detecting file format...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Setup parse context
        ParseContext context;
        context.filename = filename;
        context.format = detectFormat(lines);

        // Update progress: Parsing
        if (!updateProgress(0.2, "Parsing keywords...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Parse lines
        parseLines(lines, mesh, context, progressCallback);

        // Update progress: Validating
        if (options.validateOnRead) {
            if (!updateProgress(0.9, "Validating mesh...", progressCallback)) {
                throw OperationCancelledException();
            }
            validateMesh(mesh, context, options);
        }

        // Update progress: Complete
        updateProgress(1.0, "Complete", progressCallback);

        // Build result
        result.success = true;
        result.nodesRead = context.nodesProcessed;
        result.elementsRead = context.elementsProcessed;
        result.partsRead = context.partsProcessed;
        result.warningCount = context.warnings.size();
        result.errorCount = context.errors.size();
        result.warnings = context.warnings;
        result.errors = context.errors;

        if (result.errorCount > 0) {
            result.message = "Parsing completed with errors";
        } else if (result.warningCount > 0) {
            result.message = "Parsing completed with warnings";
        } else {
            result.message = "Parsing completed successfully";
        }

    } catch (const OperationCancelledException&) {
        result.success = false;
        result.message = "Operation cancelled by user";
        throw;
    } catch (const FileIOException& e) {
        result.success = false;
        result.message = e.message();
        result.errorCount = 1;
        result.errors.push_back(e.message());
        throw;
    } catch (const std::exception& e) {
        result.success = false;
        result.message = std::string("Unexpected error: ") + e.what();
        result.errorCount = 1;
        result.errors.push_back(e.what());
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.readTimeSeconds = std::chrono::duration<double>(endTime - startTime).count();

    m_isReading = false;
    m_progress = 1.0;

    return result;
}

double LSDynaFileReader::progress() const {
    return m_progress.load();
}

void LSDynaFileReader::cancel() {
    m_cancelRequested = true;
}

bool LSDynaFileReader::isReading() const {
    return m_isReading.load();
}

// ======================================================================
// Private Methods
// ======================================================================

std::vector<std::string> LSDynaFileReader::splitIntoLines(
    const char* data,
    size_t size)
{
    std::vector<std::string> lines;
    lines.reserve(size / 80);  // Estimate: ~80 chars per line

    const char* lineStart = data;
    const char* current = data;
    const char* end = data + size;

    while (current < end) {
        if (*current == '\n' || *current == '\r') {
            // Found end of line
            lines.emplace_back(lineStart, current - lineStart);

            // Skip \r\n or \n\r
            if (current + 1 < end &&
                ((*current == '\n' && *(current + 1) == '\r') ||
                 (*current == '\r' && *(current + 1) == '\n'))) {
                ++current;
            }

            ++current;
            lineStart = current;
        } else {
            ++current;
        }
    }

    // Add last line if not empty
    if (lineStart < end) {
        lines.emplace_back(lineStart, end - lineStart);
    }

    return lines;
}

KeywordFormat LSDynaFileReader::detectFormat(
    const std::vector<std::string>& lines)
{
    // Heuristic: check first 100 data lines for commas
    size_t dataLinesChecked = 0;
    size_t linesWithCommas = 0;

    for (const auto& line : lines) {
        if (dataLinesChecked >= 100) break;

        // Skip keywords, comments, and blank lines
        if (LineParser::isKeyword(line) ||
            LineParser::isComment(line) ||
            LineParser::isBlank(line)) {
            continue;
        }

        ++dataLinesChecked;

        // Check for comma (free format indicator)
        if (line.find(',') != std::string::npos) {
            ++linesWithCommas;
        }
    }

    // If more than 50% of lines have commas, assume free format
    if (dataLinesChecked > 0 && linesWithCommas > dataLinesChecked / 2) {
        return KeywordFormat::FREE;
    }

    // Default to fixed format (most common for LS-DYNA)
    return KeywordFormat::FIXED;
}

void LSDynaFileReader::parseLines(
    const std::vector<std::string>& lines,
    core::Mesh& mesh,
    ParseContext& context,
    ProgressCallback progressCallback)
{
    size_t totalLines = lines.size();
    size_t currentLine = 0;

    while (currentLine < totalLines) {
        // Check for cancellation periodically
        if (m_cancelRequested) {
            throw OperationCancelledException();
        }

        const std::string& line = lines[currentLine];
        context.lineNumber = currentLine + 1;

        // Update progress periodically (every 1000 lines)
        if (currentLine % 1000 == 0) {
            double progress = 0.2 + (0.7 * currentLine / totalLines);
            std::ostringstream msg;
            msg << "Parsing line " << currentLine << "/" << totalLines;

            if (!updateProgress(progress, msg.str(), progressCallback)) {
                throw OperationCancelledException();
            }
        }

        // Skip comments and blank lines
        if (LineParser::isComment(line) || LineParser::isBlank(line)) {
            ++currentLine;
            continue;
        }

        // Handle keywords
        if (LineParser::isKeyword(line)) {
            std::string keyword = LineParser::extractKeyword(line);

            // Check for *END
            if (keyword == "END") {
                break;
            }

            // Find parser for this keyword
            IKeywordParser* parser = m_registry->getParser(keyword);

            if (parser) {
                // Gather remaining lines for parser
                std::vector<std::string> remainingLines(
                    lines.begin() + currentLine + 1,
                    lines.end());

                try {
                    size_t consumed = parser->parse(remainingLines, mesh, context);
                    currentLine += 1 + consumed;
                } catch (const std::exception& e) {
                    context.addError(std::string("Parser error for keyword ") +
                                   keyword + ": " + e.what());
                    ++currentLine;
                }
            } else {
                // Unknown keyword - just skip it
                context.addWarning("Unsupported keyword: " + keyword);
                ++currentLine;
            }
        } else {
            // Data line without keyword - skip
            ++currentLine;
        }
    }
}

bool LSDynaFileReader::updateProgress(
    double progress,
    const std::string& message,
    ProgressCallback callback)
{
    m_progress = progress;

    if (callback) {
        bool shouldContinue = callback(progress, message);
        if (!shouldContinue) {
            m_cancelRequested = true;
            return false;
        }
    }

    return !m_cancelRequested;
}

void LSDynaFileReader::validateMesh(
    const core::Mesh& mesh,
    ParseContext& context,
    const ReadOptions& options)
{
    core::MeshValidator validator;

    core::MeshValidator::ValidationOptions validationOptions;
    validationOptions.checkNodeReferences = true;
    validationOptions.checkConnectivity = true;
    validationOptions.checkDuplicateNodes = false;  // Too slow for large meshes
    validationOptions.checkDuplicateElements = false;
    validationOptions.checkDegenerateElements = true;
    validationOptions.checkElementQuality = false;  // Optional, can be slow

    auto validationResult = validator.validate(mesh, validationOptions);

    // Add validation errors to context
    for (const auto& error : validationResult.errors) {
        context.addError("Validation: " + error);
    }

    // Add validation warnings to context
    for (const auto& warning : validationResult.warnings) {
        context.addWarning("Validation: " + warning);
    }

    // In strict mode, treat warnings as errors
    if (options.strictMode && !validationResult.warnings.empty()) {
        context.errors.insert(context.errors.end(),
                            validationResult.warnings.begin(),
                            validationResult.warnings.end());
    }
}

// ======================================================================
// Registration
// ======================================================================

void registerLSDynaReader() {
    FileReaderFactory::registerReader([]() {
        return std::make_unique<LSDynaFileReader>();
    });
}

} // namespace io
} // namespace koomesh
