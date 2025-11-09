/**
 * @file ParallelIndexBuilder.h
 * @brief Parallel spatial index construction utilities
 */

#pragma once

#include "core/ISpatialIndex.h"
#include "core/SpatialIndexFactory.h"
#include "core/Mesh.h"
#include <thread>
#include <future>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <map>

namespace koomesh {
namespace core {

/**
 * @brief Configuration for parallel index building
 */
struct ParallelBuildConfig {
    size_t numThreads = 0;  // 0 = auto-detect
    size_t minElementsPerThread = 10000;
    bool enableProgressReporting = false;
    std::function<void(float)> progressCallback = nullptr;
};

/**
 * @brief Statistics from parallel build operation
 */
struct ParallelBuildStats {
    size_t numThreadsUsed = 0;
    double totalBuildTime = 0.0;
    double avgThreadTime = 0.0;
    double maxThreadTime = 0.0;
    double minThreadTime = 0.0;
    size_t elementsProcessed = 0;
    double elementsPerSecond = 0.0;
};

/**
 * @brief Utility class for building spatial indexes in parallel
 *
 * This class provides methods to build spatial indexes using multiple threads
 * to improve performance on large meshes. It supports:
 * - Parallel construction of single index types
 * - Concurrent building of multiple index types
 * - Thread-safe progress reporting
 * - Performance statistics
 */
class ParallelIndexBuilder {
public:
    /**
     * @brief Build spatial index using multiple threads
     * @param type Type of spatial index to build
     * @param mesh Mesh to index
     * @param config Build configuration
     * @param stats Optional pointer to receive build statistics
     * @return Built spatial index
     */
    static std::unique_ptr<ISpatialIndex> buildParallel(
        SpatialIndexType type,
        const Mesh& mesh,
        const ParallelBuildConfig& config = ParallelBuildConfig(),
        ParallelBuildStats* stats = nullptr
    );

    /**
     * @brief Build multiple spatial indexes concurrently
     * @param types Types of spatial indexes to build
     * @param mesh Mesh to index
     * @param config Build configuration
     * @return Map of index type to built index
     */
    static std::map<SpatialIndexType, std::unique_ptr<ISpatialIndex>> buildMultiple(
        const std::vector<SpatialIndexType>& types,
        const Mesh& mesh,
        const ParallelBuildConfig& config = ParallelBuildConfig()
    );

    /**
     * @brief Build index asynchronously
     * @param type Type of spatial index to build
     * @param mesh Mesh to index
     * @param config Build configuration
     * @return Future that will contain the built index
     */
    static std::future<std::unique_ptr<ISpatialIndex>> buildAsync(
        SpatialIndexType type,
        const Mesh& mesh,
        const ParallelBuildConfig& config = ParallelBuildConfig()
    );

    /**
     * @brief Get optimal number of threads for given mesh size
     * @param elementCount Number of elements in mesh
     * @param minElementsPerThread Minimum elements per thread
     * @return Recommended number of threads
     */
    static size_t getOptimalThreadCount(
        size_t elementCount,
        size_t minElementsPerThread = 10000
    );

    /**
     * @brief Check if parallel building would be beneficial
     * @param elementCount Number of elements in mesh
     * @return True if parallel building is recommended
     */
    static bool shouldUseParallel(size_t elementCount);

    /**
     * @brief Partition mesh elements for parallel processing
     * @param mesh Mesh to partition
     * @param numPartitions Number of partitions
     * @return Vector of element ID ranges
     */
    static std::vector<std::pair<ElementId, ElementId>> partitionMesh(
        const Mesh& mesh,
        size_t numPartitions
    );

private:
    /**
     * @brief Build index for a partition of elements
     */
    static void buildPartition(
        ISpatialIndex* index,
        const Mesh& mesh,
        ElementId startId,
        ElementId endId,
        std::atomic<size_t>& processedCount,
        std::atomic<double>& threadTime
    );

    /**
     * @brief Report progress from multiple threads
     */
    static void reportProgress(
        const std::function<void(float)>& callback,
        const std::atomic<size_t>& processedCount,
        size_t totalCount
    );

    /**
     * @brief Calculate statistics from thread execution data
     */
    static ParallelBuildStats calculateStats(
        const std::vector<double>& threadTimes,
        size_t elementsProcessed,
        double totalTime
    );
};

/**
 * @brief RAII wrapper for parallel index building with automatic cleanup
 */
class ParallelIndexBuildGuard {
public:
    explicit ParallelIndexBuildGuard(const ParallelBuildConfig& config);
    ~ParallelIndexBuildGuard();

    std::unique_ptr<ISpatialIndex> build(
        SpatialIndexType type,
        const Mesh& mesh,
        ParallelBuildStats* stats = nullptr
    );

private:
    ParallelBuildConfig m_config;
    std::vector<std::thread> m_threads;
    std::atomic<bool> m_cancelled{false};
};

/**
 * @brief Benchmark parallel vs serial index building
 */
class ParallelBuildBenchmark {
public:
    struct BenchmarkResult {
        double serialTime = 0.0;
        double parallelTime = 0.0;
        double speedup = 0.0;
        size_t threadsUsed = 0;
        double efficiency = 0.0;  // speedup / threadsUsed
    };

    /**
     * @brief Compare serial vs parallel build performance
     * @param type Index type to benchmark
     * @param mesh Mesh to use
     * @param config Parallel configuration
     * @return Benchmark results
     */
    static BenchmarkResult compare(
        SpatialIndexType type,
        const Mesh& mesh,
        const ParallelBuildConfig& config = ParallelBuildConfig()
    );

    /**
     * @brief Run comprehensive parallel build benchmark
     * @param mesh Test mesh
     * @param threadCounts Thread counts to test
     * @return Map of thread count to speedup
     */
    static std::map<size_t, double> benchmarkScaling(
        const Mesh& mesh,
        const std::vector<size_t>& threadCounts = {1, 2, 4, 8}
    );
};

} // namespace core
} // namespace koomesh
