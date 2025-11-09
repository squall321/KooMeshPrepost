#pragma once

#include "io/IFileReader.h"
#include "io/FileFormatDetector.h"
#include "core/Mesh.h"
#include <future>
#include <atomic>
#include <memory>
#include <functional>

namespace koomesh {
namespace io {

/**
 * @brief Async file loading result
 */
struct AsyncLoadResult {
    bool success = false;
    std::string message;
    ReadResult readResult;
    double elapsedSeconds = 0.0;
};

/**
 * @brief Asynchronous file loader
 *
 * Provides non-blocking file loading capabilities using std::async.
 * Allows UI to remain responsive while loading large files.
 *
 * Features:
 * - Background file loading
 * - Progress tracking
 * - Cancellation support
 * - Thread-safe operations
 *
 * Example usage:
 * @code
 * AsyncFileLoader loader;
 *
 * // Start async load
 * auto future = loader.loadAsync("large_model.k", mesh,
 *     [](double progress, const std::string& msg) {
 *         std::cout << "Progress: " << (progress * 100) << "% - " << msg << "\n";
 *         return true;  // Continue loading
 *     });
 *
 * // Do other work while loading...
 *
 * // Wait for completion
 * AsyncLoadResult result = future.get();
 * if (result.success) {
 *     std::cout << "Loaded in " << result.elapsedSeconds << " seconds\n";
 * }
 * @endcode
 *
 * With cancellation:
 * @code
 * AsyncFileLoader loader;
 * auto future = loader.loadAsync("model.k", mesh, progressCallback);
 *
 * // ... later, if user cancels:
 * loader.cancel();
 *
 * AsyncLoadResult result = future.get();  // Will be cancelled
 * @endcode
 */
class AsyncFileLoader {
public:
    /**
     * @brief Constructor
     */
    AsyncFileLoader();

    /**
     * @brief Destructor
     *
     * Waits for any pending operations to complete
     */
    ~AsyncFileLoader();

    /**
     * @brief Load file asynchronously
     *
     * Starts loading in background thread. Returns immediately.
     *
     * @param filepath Path to file
     * @param mesh Mesh object to populate
     * @param progressCallback Optional progress callback
     * @return Future that will contain load result
     *
     * @note Mesh reference must remain valid until future completes
     */
    std::future<AsyncLoadResult> loadAsync(
        const std::string& filepath,
        core::Mesh& mesh,
        ProgressCallback progressCallback = nullptr);

    /**
     * @brief Load file asynchronously with options
     *
     * @param filepath Path to file
     * @param mesh Mesh object to populate
     * @param options Read options
     * @param progressCallback Optional progress callback
     * @return Future that will contain load result
     */
    std::future<AsyncLoadResult> loadAsync(
        const std::string& filepath,
        core::Mesh& mesh,
        const ReadOptions& options,
        ProgressCallback progressCallback = nullptr);

    /**
     * @brief Cancel ongoing load operation
     *
     * Requests cancellation of current load. The operation may not
     * stop immediately but will terminate at next progress check.
     *
     * Thread-safe method.
     */
    void cancel();

    /**
     * @brief Check if load is in progress
     *
     * @return true if async load is currently running
     */
    bool isLoading() const;

    /**
     * @brief Get current progress (0.0 to 1.0)
     *
     * Thread-safe method to query progress of current load
     *
     * @return Progress value, or 0.0 if not loading
     */
    double progress() const;

    /**
     * @brief Wait for current operation to complete
     *
     * Blocks until async load finishes. If no load is in progress,
     * returns immediately.
     */
    void wait();

private:
    /**
     * @brief Internal async load implementation
     *
     * This is the actual worker function executed in background thread
     */
    AsyncLoadResult loadImpl(
        const std::string& filepath,
        core::Mesh& mesh,
        const ReadOptions& options,
        ProgressCallback userCallback);

    /**
     * @brief Wrap user callback with progress tracking
     *
     * Creates a callback that updates internal progress state
     * and checks for cancellation
     */
    ProgressCallback createWrappedCallback(ProgressCallback userCallback);

    // State tracking
    std::atomic<bool> m_isLoading{false};
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<double> m_progress{0.0};

    // Current operation future
    std::future<AsyncLoadResult> m_currentOperation;
};

} // namespace io
} // namespace koomesh
