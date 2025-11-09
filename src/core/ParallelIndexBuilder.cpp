/**
 * @file ParallelIndexBuilder.cpp
 * @brief Implementation of parallel spatial index construction
 */

#include "core/ParallelIndexBuilder.h"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace koomesh {
namespace core {

// ============================================================================
// ParallelIndexBuilder Implementation
// ============================================================================

std::unique_ptr<ISpatialIndex> ParallelIndexBuilder::buildParallel(
    SpatialIndexType type,
    const Mesh& mesh,
    const ParallelBuildConfig& config,
    ParallelBuildStats* stats
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Determine number of threads
    size_t numThreads = config.numThreads;
    if (numThreads == 0) {
        numThreads = getOptimalThreadCount(
            mesh.elementCount(),
            config.minElementsPerThread
        );
    }

    // For small meshes, use serial building
    if (!shouldUseParallel(mesh.elementCount()) || numThreads == 1) {
        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);

        if (stats) {
            auto endTime = std::chrono::high_resolution_clock::now();
            double buildTime = std::chrono::duration<double>(endTime - startTime).count();

            stats->numThreadsUsed = 1;
            stats->totalBuildTime = buildTime;
            stats->avgThreadTime = buildTime;
            stats->maxThreadTime = buildTime;
            stats->minThreadTime = buildTime;
            stats->elementsProcessed = mesh.elementCount();
            stats->elementsPerSecond = mesh.elementCount() / buildTime;
        }

        return index;
    }

    // Create index
    auto index = SpatialIndexFactory::create(type);

    // Build index (note: actual parallel building would require
    // modification of ISpatialIndex interface to support incremental building)
    // For now, we use the standard build method
    // TODO: Implement true parallel building when index supports it
    index->build(mesh);

    // Calculate statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    double buildTime = std::chrono::duration<double>(endTime - startTime).count();

    if (stats) {
        stats->numThreadsUsed = numThreads;
        stats->totalBuildTime = buildTime;
        stats->avgThreadTime = buildTime;
        stats->maxThreadTime = buildTime;
        stats->minThreadTime = buildTime;
        stats->elementsProcessed = mesh.elementCount();
        stats->elementsPerSecond = mesh.elementCount() / buildTime;
    }

    return index;
}

std::map<SpatialIndexType, std::unique_ptr<ISpatialIndex>>
ParallelIndexBuilder::buildMultiple(
    const std::vector<SpatialIndexType>& types,
    const Mesh& mesh,
    const ParallelBuildConfig& config
) {
    std::map<SpatialIndexType, std::unique_ptr<ISpatialIndex>> results;

    if (types.empty()) {
        return results;
    }

    // Build indexes concurrently
    std::vector<std::future<std::pair<SpatialIndexType, std::unique_ptr<ISpatialIndex>>>> futures;

    for (auto type : types) {
        futures.push_back(std::async(std::launch::async, [type, &mesh, &config]() {
            auto index = buildParallel(type, mesh, config);
            return std::make_pair(type, std::move(index));
        }));
    }

    // Collect results
    for (auto& future : futures) {
        auto result = future.get();
        results[result.first] = std::move(result.second);
    }

    return results;
}

std::future<std::unique_ptr<ISpatialIndex>> ParallelIndexBuilder::buildAsync(
    SpatialIndexType type,
    const Mesh& mesh,
    const ParallelBuildConfig& config
) {
    return std::async(std::launch::async, [type, &mesh, config]() {
        return buildParallel(type, mesh, config);
    });
}

size_t ParallelIndexBuilder::getOptimalThreadCount(
    size_t elementCount,
    size_t minElementsPerThread
) {
    if (elementCount < minElementsPerThread) {
        return 1;
    }

    size_t hardwareConcurrency = std::thread::hardware_concurrency();
    if (hardwareConcurrency == 0) {
        hardwareConcurrency = 4;  // Fallback
    }

    // Calculate based on element count
    size_t threadsForElements = (elementCount + minElementsPerThread - 1) / minElementsPerThread;

    // Don't use more threads than hardware supports
    return std::min(threadsForElements, hardwareConcurrency);
}

bool ParallelIndexBuilder::shouldUseParallel(size_t elementCount) {
    // Parallel building is beneficial for meshes with > 10k elements
    return elementCount > 10000;
}

std::vector<std::pair<ElementId, ElementId>> ParallelIndexBuilder::partitionMesh(
    const Mesh& mesh,
    size_t numPartitions
) {
    std::vector<std::pair<ElementId, ElementId>> partitions;

    if (numPartitions == 0 || mesh.elementCount() == 0) {
        return partitions;
    }

    // Get all element IDs
    std::vector<ElementId> elementIds;
    elementIds.reserve(mesh.elementCount());

    for (const auto& pair : mesh.elements()) {
        elementIds.push_back(pair.first);
    }

    // Sort for consistent partitioning
    std::sort(elementIds.begin(), elementIds.end());

    // Calculate partition size
    size_t elementsPerPartition = (elementIds.size() + numPartitions - 1) / numPartitions;

    // Create partitions
    for (size_t i = 0; i < numPartitions; ++i) {
        size_t startIdx = i * elementsPerPartition;
        size_t endIdx = std::min(startIdx + elementsPerPartition, elementIds.size());

        if (startIdx >= elementIds.size()) {
            break;
        }

        partitions.emplace_back(elementIds[startIdx], elementIds[endIdx - 1]);
    }

    return partitions;
}

void ParallelIndexBuilder::buildPartition(
    ISpatialIndex* index,
    const Mesh& mesh,
    ElementId startId,
    ElementId endId,
    std::atomic<size_t>& processedCount,
    std::atomic<double>& threadTime
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Process elements in range
    // Note: This requires index to support incremental insertion
    // For now, this is a placeholder for future implementation

    size_t count = 0;
    for (const auto& pair : mesh.elements()) {
        if (pair.first >= startId && pair.first <= endId) {
            ++count;
        }
    }

    processedCount += count;

    auto endTime = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration<double>(endTime - startTime).count();

    // Update thread time (atomic add)
    double currentTime = threadTime.load();
    while (!threadTime.compare_exchange_weak(currentTime, currentTime + time)) {
        // Retry if another thread modified it
    }
}

void ParallelIndexBuilder::reportProgress(
    const std::function<void(float)>& callback,
    const std::atomic<size_t>& processedCount,
    size_t totalCount
) {
    if (!callback || totalCount == 0) {
        return;
    }

    float progress = static_cast<float>(processedCount.load()) / static_cast<float>(totalCount);
    callback(std::min(progress, 1.0f));
}

ParallelBuildStats ParallelIndexBuilder::calculateStats(
    const std::vector<double>& threadTimes,
    size_t elementsProcessed,
    double totalTime
) {
    ParallelBuildStats stats;

    stats.numThreadsUsed = threadTimes.size();
    stats.totalBuildTime = totalTime;
    stats.elementsProcessed = elementsProcessed;

    if (totalTime > 0.0) {
        stats.elementsPerSecond = static_cast<double>(elementsProcessed) / totalTime;
    }

    if (!threadTimes.empty()) {
        stats.avgThreadTime = std::accumulate(threadTimes.begin(), threadTimes.end(), 0.0)
                             / static_cast<double>(threadTimes.size());
        stats.maxThreadTime = *std::max_element(threadTimes.begin(), threadTimes.end());
        stats.minThreadTime = *std::min_element(threadTimes.begin(), threadTimes.end());
    }

    return stats;
}

// ============================================================================
// ParallelIndexBuildGuard Implementation
// ============================================================================

ParallelIndexBuildGuard::ParallelIndexBuildGuard(const ParallelBuildConfig& config)
    : m_config(config) {
}

ParallelIndexBuildGuard::~ParallelIndexBuildGuard() {
    m_cancelled = true;

    // Wait for all threads to finish
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

std::unique_ptr<ISpatialIndex> ParallelIndexBuildGuard::build(
    SpatialIndexType type,
    const Mesh& mesh,
    ParallelBuildStats* stats
) {
    if (m_cancelled) {
        return nullptr;
    }

    return ParallelIndexBuilder::buildParallel(type, mesh, m_config, stats);
}

// ============================================================================
// ParallelBuildBenchmark Implementation
// ============================================================================

ParallelBuildBenchmark::BenchmarkResult ParallelBuildBenchmark::compare(
    SpatialIndexType type,
    const Mesh& mesh,
    const ParallelBuildConfig& config
) {
    BenchmarkResult result;

    // Serial build
    {
        auto startTime = std::chrono::high_resolution_clock::now();

        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);

        auto endTime = std::chrono::high_resolution_clock::now();
        result.serialTime = std::chrono::duration<double>(endTime - startTime).count();
    }

    // Parallel build
    {
        ParallelBuildStats stats;
        auto startTime = std::chrono::high_resolution_clock::now();

        auto index = ParallelIndexBuilder::buildParallel(type, mesh, config, &stats);

        auto endTime = std::chrono::high_resolution_clock::now();
        result.parallelTime = std::chrono::duration<double>(endTime - startTime).count();
        result.threadsUsed = stats.numThreadsUsed;
    }

    // Calculate speedup
    if (result.parallelTime > 0.0) {
        result.speedup = result.serialTime / result.parallelTime;
    }

    // Calculate efficiency
    if (result.threadsUsed > 0) {
        result.efficiency = result.speedup / static_cast<double>(result.threadsUsed);
    }

    return result;
}

std::map<size_t, double> ParallelBuildBenchmark::benchmarkScaling(
    const Mesh& mesh,
    const std::vector<size_t>& threadCounts
) {
    std::map<size_t, double> results;

    // Measure serial time first
    auto startTime = std::chrono::high_resolution_clock::now();
    auto serialIndex = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    serialIndex->build(mesh);
    auto endTime = std::chrono::high_resolution_clock::now();
    double serialTime = std::chrono::duration<double>(endTime - startTime).count();

    results[1] = 1.0;  // Serial speedup is 1.0

    // Test each thread count
    for (size_t numThreads : threadCounts) {
        if (numThreads == 1) {
            continue;  // Already have serial result
        }

        ParallelBuildConfig config;
        config.numThreads = numThreads;

        startTime = std::chrono::high_resolution_clock::now();
        auto index = ParallelIndexBuilder::buildParallel(
            SpatialIndexType::OCTREE,
            mesh,
            config
        );
        endTime = std::chrono::high_resolution_clock::now();

        double parallelTime = std::chrono::duration<double>(endTime - startTime).count();

        if (parallelTime > 0.0) {
            results[numThreads] = serialTime / parallelTime;
        }
    }

    return results;
}

} // namespace core
} // namespace koomesh
