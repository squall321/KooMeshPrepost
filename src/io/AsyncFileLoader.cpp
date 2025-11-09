#include "io/AsyncFileLoader.h"
#include "io/FileReaderFactory.h"
#include "io/FileIOException.h"
#include <chrono>

namespace koomesh {
namespace io {

// ======================================================================
// AsyncFileLoader Implementation
// ======================================================================

AsyncFileLoader::AsyncFileLoader()
    : m_isLoading(false)
    , m_cancelRequested(false)
    , m_progress(0.0)
{
}

AsyncFileLoader::~AsyncFileLoader() {
    // Wait for any pending operations
    wait();
}

std::future<AsyncLoadResult> AsyncFileLoader::loadAsync(
    const std::string& filepath,
    core::Mesh& mesh,
    ProgressCallback progressCallback)
{
    return loadAsync(filepath, mesh, ReadOptions(), progressCallback);
}

std::future<AsyncLoadResult> AsyncFileLoader::loadAsync(
    const std::string& filepath,
    core::Mesh& mesh,
    const ReadOptions& options,
    ProgressCallback progressCallback)
{
    // Check if already loading
    if (m_isLoading.load()) {
        // Return a failed future
        std::promise<AsyncLoadResult> promise;
        AsyncLoadResult result;
        result.success = false;
        result.message = "Another load operation is already in progress";
        promise.set_value(result);
        return promise.get_future();
    }

    // Reset state
    m_isLoading = true;
    m_cancelRequested = false;
    m_progress = 0.0;

    // Launch async operation
    m_currentOperation = std::async(
        std::launch::async,
        [this, filepath, &mesh, options, progressCallback]() {
            return loadImpl(filepath, mesh, options, progressCallback);
        }
    );

    return std::async(std::launch::deferred, [this]() {
        return m_currentOperation.get();
    });
}

void AsyncFileLoader::cancel() {
    m_cancelRequested = true;
}

bool AsyncFileLoader::isLoading() const {
    return m_isLoading.load();
}

double AsyncFileLoader::progress() const {
    return m_progress.load();
}

void AsyncFileLoader::wait() {
    if (m_currentOperation.valid()) {
        m_currentOperation.wait();
    }
}

// ======================================================================
// Private Methods
// ======================================================================

AsyncLoadResult AsyncFileLoader::loadImpl(
    const std::string& filepath,
    core::Mesh& mesh,
    const ReadOptions& options,
    ProgressCallback userCallback)
{
    AsyncLoadResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Create wrapped callback that tracks progress and cancellation
        auto wrappedCallback = createWrappedCallback(userCallback);

        // Use factory to auto-detect and load
        result.readResult = FileReaderFactory::readFile(
            filepath,
            mesh,
            options,
            wrappedCallback);

        result.success = result.readResult.success;
        result.message = result.readResult.message;

    } catch (const OperationCancelledException&) {
        result.success = false;
        result.message = "Load operation cancelled by user";
    } catch (const FileIOException& e) {
        result.success = false;
        result.message = e.message();
    } catch (const std::exception& e) {
        result.success = false;
        result.message = std::string("Unexpected error: ") + e.what();
    }

    // Calculate elapsed time
    auto endTime = std::chrono::high_resolution_clock::now();
    result.elapsedSeconds = std::chrono::duration<double>(endTime - startTime).count();

    // Clear loading state
    m_isLoading = false;
    m_progress = result.success ? 1.0 : 0.0;

    return result;
}

ProgressCallback AsyncFileLoader::createWrappedCallback(ProgressCallback userCallback) {
    return [this, userCallback](double progress, const std::string& message) -> bool {
        // Update internal progress
        m_progress = progress;

        // Check for cancellation
        if (m_cancelRequested.load()) {
            return false;  // Signal cancellation to reader
        }

        // Call user callback if provided
        if (userCallback) {
            return userCallback(progress, message);
        }

        return true;  // Continue operation
    };
}

} // namespace io
} // namespace koomesh
