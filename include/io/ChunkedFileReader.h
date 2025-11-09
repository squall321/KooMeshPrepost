#pragma once

#include "io/IFileReader.h"
#include "io/MemoryMappedFile.h"
#include "io/LSDynaKeywordParser.h"
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>

namespace koomesh {
namespace io {

/**
 * @brief Configuration for chunked file reading
 */
struct ChunkedReadOptions {
    ReadOptions baseOptions;        ///< Base read options
    size_t chunkSize = 100 * 1024 * 1024;  ///< Chunk size in bytes (default: 100MB)
    size_t numThreads = 0;          ///< Number of threads (0 = auto-detect)
    bool preserveOrder = true;      ///< Process chunks in order
    size_t maxQueueSize = 4;        ///< Max queued chunks

    ChunkedReadOptions() = default;
    explicit ChunkedReadOptions(const ReadOptions& opts) : baseOptions(opts) {}
};

/**
 * @brief File chunk information
 */
struct FileChunk {
    size_t id;              ///< Chunk ID
    size_t startOffset;     ///< Start byte offset in file
    size_t endOffset;       ///< End byte offset in file
    size_t startLine;       ///< Start line number
    size_t endLine;         ///< End line number
    std::vector<std::string> lines;  ///< Lines in this chunk

    size_t size() const { return endOffset - startOffset; }
    size_t lineCount() const { return lines.size(); }
};

/**
 * @brief Thread pool for parallel chunk processing
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Delete copy/move
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Enqueue a task to be executed
     */
    template<typename F>
    void enqueue(F&& task);

    /**
     * @brief Wait for all tasks to complete
     */
    void wait();

    /**
     * @brief Get number of threads
     */
    size_t size() const { return m_threads.size(); }

private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop{false};
    std::atomic<size_t> m_activeTasks{0};

    void workerThread();
};

/**
 * @brief Chunked file reader for large LS-DYNA files
 *
 * Splits large files into chunks and processes them in parallel
 * using multiple threads for improved performance.
 *
 * Features:
 * - Automatic chunk splitting at keyword boundaries
 * - Multi-threaded parallel processing
 * - Configurable chunk size and thread count
 * - Per-chunk and overall progress tracking
 * - Thread-safe result aggregation
 *
 * Example:
 * @code
 * ChunkedFileReader reader;
 * Mesh mesh;
 * ChunkedReadOptions options;
 * options.chunkSize = 100 * 1024 * 1024;  // 100MB chunks
 * options.numThreads = 4;
 *
 * auto result = reader.read("large_model.k", mesh, options,
 *     [](double progress, const std::string& msg) {
 *         std::cout << "Progress: " << (progress * 100) << "%\n";
 *         return true;
 *     });
 * @endcode
 */
class ChunkedFileReader : public IFileReader {
public:
    /**
     * @brief Constructor
     */
    ChunkedFileReader();

    /**
     * @brief Destructor
     */
    ~ChunkedFileReader() override;

    // IFileReader interface
    bool canRead(const std::string& filename) const override;
    std::vector<std::string> supportedExtensions() const override;
    std::string formatName() const override;

    /**
     * @brief Read file using chunked parallel processing
     */
    ReadResult read(
        const std::string& filename,
        core::Mesh& mesh,
        const ReadOptions& options = ReadOptions(),
        ProgressCallback progressCallback = nullptr) override;

    /**
     * @brief Read file with chunked-specific options
     */
    ReadResult readChunked(
        const std::string& filename,
        core::Mesh& mesh,
        const ChunkedReadOptions& options = ChunkedReadOptions(),
        ProgressCallback progressCallback = nullptr);

    double progress() const override;
    void cancel() override;
    bool isReading() const override;

    /**
     * @brief Get chunk statistics from last read
     */
    struct ChunkStats {
        size_t totalChunks = 0;
        size_t chunksProcessed = 0;
        size_t totalLines = 0;
        size_t avgChunkSize = 0;
        double chunkingTimeSeconds = 0.0;
        double parsingTimeSeconds = 0.0;
        double mergingTimeSeconds = 0.0;
    };

    const ChunkStats& getLastChunkStats() const { return m_lastStats; }

private:
    // Parser registry
    std::unique_ptr<KeywordParserRegistry> m_registry;

    // Progress tracking
    mutable std::atomic<double> m_progress{0.0};
    std::atomic<bool> m_isReading{false};
    std::atomic<bool> m_cancelRequested{false};

    // Chunk statistics
    ChunkStats m_lastStats;

    /**
     * @brief Split file into chunks at keyword boundaries
     */
    std::vector<FileChunk> splitIntoChunks(
        const std::vector<std::string>& lines,
        size_t chunkSize);

    /**
     * @brief Process a single chunk
     */
    void processChunk(
        const FileChunk& chunk,
        const ParseContext& baseContext,
        std::vector<core::Node>& nodes,
        std::vector<core::Element>& elements,
        std::vector<std::string>& errors,
        std::vector<std::string>& warnings,
        std::mutex& resultMutex);

    /**
     * @brief Merge chunk results into final mesh
     */
    void mergeResults(
        core::Mesh& mesh,
        const std::vector<std::vector<core::Node>>& chunkNodes,
        const std::vector<std::vector<core::Element>>& chunkElements);

    /**
     * @brief Update progress and check for cancellation
     */
    bool updateProgress(
        double progress,
        const std::string& message,
        ProgressCallback callback);

    /**
     * @brief Detect optimal number of threads
     */
    size_t detectOptimalThreadCount() const;
};

} // namespace io
} // namespace koomesh
