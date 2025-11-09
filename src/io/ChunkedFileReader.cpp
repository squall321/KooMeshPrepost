#include "io/ChunkedFileReader.h"
#include "io/FileIOException.h"
#include "io/MemoryMappedFile.h"
#include "io/LSDynaKeywordParser.h"
#include <chrono>
#include <algorithm>
#include <sstream>

namespace koomesh {
namespace io {

// ======================================================================
// ThreadPool Implementation
// ======================================================================

ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;  // Fallback
    }

    m_threads.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        m_threads.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool() {
    m_stop = true;
    m_condition.notify_all();

    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

template<typename F>
void ThreadPool::enqueue(F&& task) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tasks.push(std::forward<F>(task));
    }
    m_condition.notify_one();
}

// Explicit instantiation for std::function<void()>
template void ThreadPool::enqueue(std::function<void()>&&);

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] {
        return m_tasks.empty() && m_activeTasks == 0;
    });
}

void ThreadPool::workerThread() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });

            if (m_stop && m_tasks.empty()) {
                return;
            }

            if (!m_tasks.empty()) {
                task = std::move(m_tasks.front());
                m_tasks.pop();
                ++m_activeTasks;
            }
        }

        if (task) {
            task();
            --m_activeTasks;
            m_condition.notify_all();
        }
    }
}

// ======================================================================
// ChunkedFileReader Implementation
// ======================================================================

ChunkedFileReader::ChunkedFileReader()
    : m_registry(KeywordParserRegistry::createDefault())
{
}

ChunkedFileReader::~ChunkedFileReader() = default;

bool ChunkedFileReader::canRead(const std::string& filename) const {
    auto extensions = supportedExtensions();
    for (const auto& ext : extensions) {
        if (filename.length() >= ext.length()) {
            std::string fileExt = filename.substr(filename.length() - ext.length());
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

std::vector<std::string> ChunkedFileReader::supportedExtensions() const {
    return {".k", ".key", ".dyn", ".keyword"};
}

std::string ChunkedFileReader::formatName() const {
    return "LS-DYNA Keyword (Chunked)";
}

ReadResult ChunkedFileReader::read(
    const std::string& filename,
    core::Mesh& mesh,
    const ReadOptions& options,
    ProgressCallback progressCallback)
{
    ChunkedReadOptions chunkedOptions(options);
    return readChunked(filename, mesh, chunkedOptions, progressCallback);
}

ReadResult ChunkedFileReader::readChunked(
    const std::string& filename,
    core::Mesh& mesh,
    const ChunkedReadOptions& options,
    ProgressCallback progressCallback)
{
    ReadResult result;
    result.success = false;

    m_isReading = true;
    m_cancelRequested = false;
    m_progress = 0.0;
    m_lastStats = ChunkStats();

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

        mmf.advise(MemoryMappedFile::Advice::SEQUENTIAL);

        // Update progress: Reading content
        if (!updateProgress(0.1, "Reading file content...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Split into lines
        std::vector<std::string> lines;
        lines.reserve(mmf.size() / 80);

        const char* data = mmf.data();
        size_t size = mmf.size();
        const char* lineStart = data;
        const char* current = data;
        const char* end = data + size;

        while (current < end) {
            if (*current == '\n' || *current == '\r') {
                lines.emplace_back(lineStart, current - lineStart);

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

        if (lineStart < end) {
            lines.emplace_back(lineStart, end - lineStart);
        }

        if (lines.empty()) {
            result.message = "File is empty";
            result.errorCount = 1;
            result.errors.push_back("File contains no data");
            return result;
        }

        m_lastStats.totalLines = lines.size();

        // Update progress: Chunking
        if (!updateProgress(0.15, "Splitting file into chunks...", progressCallback)) {
            throw OperationCancelledException();
        }

        auto chunkStartTime = std::chrono::high_resolution_clock::now();
        std::vector<FileChunk> chunks = splitIntoChunks(lines, options.chunkSize);
        auto chunkEndTime = std::chrono::high_resolution_clock::now();

        m_lastStats.chunkingTimeSeconds =
            std::chrono::duration<double>(chunkEndTime - chunkStartTime).count();
        m_lastStats.totalChunks = chunks.size();

        // Update progress: Parsing
        if (!updateProgress(0.2, "Parsing chunks in parallel...", progressCallback)) {
            throw OperationCancelledException();
        }

        // Setup base parse context
        ParseContext baseContext;
        baseContext.filename = filename;

        // Detect format from first chunk
        if (!chunks.empty() && !chunks[0].lines.empty()) {
            size_t dataLinesChecked = 0;
            size_t linesWithCommas = 0;

            for (const auto& line : chunks[0].lines) {
                if (dataLinesChecked >= 100) break;

                if (LineParser::isKeyword(line) ||
                    LineParser::isComment(line) ||
                    LineParser::isBlank(line)) {
                    continue;
                }

                ++dataLinesChecked;

                if (line.find(',') != std::string::npos) {
                    ++linesWithCommas;
                }
            }

            if (dataLinesChecked > 0 && linesWithCommas > dataLinesChecked / 2) {
                baseContext.format = KeywordFormat::FREE;
            } else {
                baseContext.format = KeywordFormat::FIXED;
            }
        }

        auto parseStartTime = std::chrono::high_resolution_clock::now();

        // Parse all lines sequentially (parallel parsing requires Mesh API changes)
        // We still benefit from chunk splitting for progress tracking
        std::vector<std::string> allErrors;
        std::vector<std::string> allWarnings;

        size_t currentLine = 0;
        size_t totalLines = lines.size();

        while (currentLine < totalLines) {
            if (m_cancelRequested) {
                throw OperationCancelledException();
            }

            const std::string& line = lines[currentLine];
            baseContext.lineNumber = currentLine + 1;

            // Update progress periodically
            if (currentLine % 1000 == 0) {
                double progress = 0.2 + (0.6 * currentLine / totalLines);
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

                if (keyword == "END") {
                    break;
                }

                IKeywordParser* parser = m_registry->getParser(keyword);

                if (parser) {
                    std::vector<std::string> remainingLines(
                        lines.begin() + currentLine + 1,
                        lines.end());

                    try {
                        size_t consumed = parser->parse(remainingLines, mesh, baseContext);
                        currentLine += 1 + consumed;
                    } catch (const std::exception& e) {
                        allErrors.push_back(std::string("Parser error for keyword ") +
                                          keyword + ": " + e.what());
                        ++currentLine;
                    }
                } else {
                    allWarnings.push_back("Unsupported keyword: " + keyword);
                    ++currentLine;
                }
            } else {
                ++currentLine;
            }
        }

        auto parseEndTime = std::chrono::high_resolution_clock::now();
        m_lastStats.parsingTimeSeconds =
            std::chrono::duration<double>(parseEndTime - parseStartTime).count();
        m_lastStats.chunksProcessed = chunks.size();
        m_lastStats.mergingTimeSeconds = 0.0;  // No merging needed for sequential parsing

        // Update progress: Complete
        updateProgress(1.0, "Complete", progressCallback);

        // Build result
        result.success = true;
        result.nodesRead = baseContext.nodesProcessed;
        result.elementsRead = baseContext.elementsProcessed;
        result.partsRead = baseContext.partsProcessed;
        result.warningCount = baseContext.warnings.size() + allWarnings.size();
        result.errorCount = baseContext.errors.size() + allErrors.size();
        result.warnings = baseContext.warnings;
        result.warnings.insert(result.warnings.end(), allWarnings.begin(), allWarnings.end());
        result.errors = baseContext.errors;
        result.errors.insert(result.errors.end(), allErrors.begin(), allErrors.end());

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

double ChunkedFileReader::progress() const {
    return m_progress.load();
}

void ChunkedFileReader::cancel() {
    m_cancelRequested = true;
}

bool ChunkedFileReader::isReading() const {
    return m_isReading.load();
}

// ======================================================================
// Private Methods
// ======================================================================

std::vector<FileChunk> ChunkedFileReader::splitIntoChunks(
    const std::vector<std::string>& lines,
    size_t chunkSize)
{
    std::vector<FileChunk> chunks;

    if (lines.empty()) {
        return chunks;
    }

    // Calculate approximate bytes per line
    size_t totalBytes = 0;
    for (const auto& line : lines) {
        totalBytes += line.length() + 1;  // +1 for newline
    }
    double bytesPerLine = static_cast<double>(totalBytes) / lines.size();

    // Calculate approximate lines per chunk
    size_t linesPerChunk = static_cast<size_t>(chunkSize / bytesPerLine);
    if (linesPerChunk == 0) linesPerChunk = 100;  // Minimum chunk size

    size_t chunkId = 0;
    size_t currentLine = 0;
    size_t currentOffset = 0;

    while (currentLine < lines.size()) {
        FileChunk chunk;
        chunk.id = chunkId++;
        chunk.startLine = currentLine;
        chunk.startOffset = currentOffset;

        size_t linesInChunk = 0;
        size_t bytesInChunk = 0;

        // Add lines until we reach chunk size or find a keyword boundary
        while (currentLine < lines.size() &&
               (linesInChunk < linesPerChunk || !LineParser::isKeyword(lines[currentLine]))) {

            const std::string& line = lines[currentLine];
            chunk.lines.push_back(line);

            bytesInChunk += line.length() + 1;
            ++linesInChunk;
            ++currentLine;

            // If we've exceeded chunk size and found a keyword, break
            if (bytesInChunk >= chunkSize &&
                currentLine < lines.size() &&
                LineParser::isKeyword(lines[currentLine])) {
                break;
            }

            // Hard limit: don't let chunks get too large
            if (bytesInChunk >= chunkSize * 2) {
                break;
            }
        }

        chunk.endLine = currentLine;
        chunk.endOffset = currentOffset + bytesInChunk;
        currentOffset = chunk.endOffset;

        if (!chunk.lines.empty()) {
            chunks.push_back(std::move(chunk));
        }
    }

    // Calculate average chunk size
    if (!chunks.empty()) {
        size_t totalSize = 0;
        for (const auto& chunk : chunks) {
            totalSize += chunk.size();
        }
        m_lastStats.avgChunkSize = totalSize / chunks.size();
    }

    return chunks;
}

void ChunkedFileReader::processChunk(
    const FileChunk& chunk,
    const ParseContext& baseContext,
    std::vector<core::Node>& nodes,
    std::vector<core::Element>& elements,
    std::vector<std::string>& errors,
    std::vector<std::string>& warnings,
    std::mutex& resultMutex)
{
    // Create local context for this chunk
    ParseContext context = baseContext;
    context.lineNumber = chunk.startLine;

    size_t currentLine = 0;

    while (currentLine < chunk.lines.size()) {
        if (m_cancelRequested) {
            return;
        }

        const std::string& line = chunk.lines[currentLine];
        context.lineNumber = chunk.startLine + currentLine + 1;

        // Skip comments and blank lines
        if (LineParser::isComment(line) || LineParser::isBlank(line)) {
            ++currentLine;
            continue;
        }

        // Handle keywords
        if (LineParser::isKeyword(line)) {
            std::string keyword = LineParser::extractKeyword(line);

            if (keyword == "END") {
                break;
            }

            IKeywordParser* parser = m_registry->getParser(keyword);

            if (parser) {
                // Gather remaining lines for parser
                std::vector<std::string> remainingLines(
                    chunk.lines.begin() + currentLine + 1,
                    chunk.lines.end());

                // Parse directly into the chunk's storage
                // Note: We can't use a temp mesh because nodes/elements aren't publicly accessible
                // Instead, rely on the parser to populate context statistics

                // For now, skip chunk-based parallel parsing of keywords
                // This will be addressed in a future enhancement
                // Just track the lines for this chunk

                try {
                    // Consume lines but don't parse in parallel for now
                    // This is a limitation that will be fixed when we refactor
                    // the keyword parsers to support direct vector output
                    currentLine += 1;
                } catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lock(resultMutex);
                    errors.push_back(std::string("Chunk ") + std::to_string(chunk.id) +
                                   ", Parser error for keyword " + keyword + ": " + e.what());
                    ++currentLine;
                }
            } else {
                std::lock_guard<std::mutex> lock(resultMutex);
                warnings.push_back(std::string("Chunk ") + std::to_string(chunk.id) +
                                 ", Unsupported keyword: " + keyword);
                ++currentLine;
            }
        } else {
            ++currentLine;
        }
    }
}

void ChunkedFileReader::mergeResults(
    core::Mesh& mesh,
    const std::vector<std::vector<core::Node>>& chunkNodes,
    const std::vector<std::vector<core::Element>>& chunkElements)
{
    // Currently not used - parsing is sequential
    // This method is reserved for future enhancement when
    // parallel parsing is fully supported
    (void)mesh;
    (void)chunkNodes;
    (void)chunkElements;
}

bool ChunkedFileReader::updateProgress(
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

size_t ChunkedFileReader::detectOptimalThreadCount() const {
    size_t hardwareThreads = std::thread::hardware_concurrency();

    if (hardwareThreads == 0) {
        return 4;  // Fallback
    }

    // Use hardware threads, but cap at 8 for typical workloads
    return std::min(hardwareThreads, size_t(8));
}

} // namespace io
} // namespace koomesh
