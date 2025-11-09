#pragma once

#include "core/ISpatialIndex.h"
#include "core/Mesh.h"
#include "core/SpatialIndexFactory.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

namespace koomesh {
namespace core {

/**
 * @brief Benchmark results for a single operation
 */
struct BenchmarkResult {
    std::string indexType;            ///< Type of spatial index tested
    std::string operationType;        ///< Type of operation (build, query, nearest, etc.)
    size_t meshSize;                  ///< Number of elements in mesh
    double timeMilliseconds;          ///< Execution time in milliseconds
    size_t queryCount;                ///< Number of queries performed
    size_t resultCount;               ///< Number of results returned
    size_t memoryBytes;               ///< Estimated memory usage in bytes

    BenchmarkResult()
        : meshSize(0)
        , timeMilliseconds(0.0)
        , queryCount(0)
        , resultCount(0)
        , memoryBytes(0) {}
};

/**
 * @brief Comprehensive benchmark comparison for spatial indexes
 *
 * This class provides utilities for benchmarking and comparing the performance
 * of different spatial index implementations. It measures build time, query
 * performance, memory usage, and scalability.
 *
 * Example usage:
 * @code
 * SpatialIndexBenchmark benchmark;
 *
 * // Benchmark build performance
 * auto buildResults = benchmark.benchmarkBuildTime(mesh, {
 *     SpatialIndexType::OCTREE,
 *     SpatialIndexType::RTREE,
 *     SpatialIndexType::KDTREE,
 *     SpatialIndexType::UNIFORM_GRID
 * });
 *
 * // Print comparison
 * benchmark.printComparison(buildResults);
 * @endcode
 */
class SpatialIndexBenchmark {
public:
    /**
     * @brief Benchmark build time for multiple spatial index types
     * @param mesh The mesh to build indexes for
     * @param types Vector of spatial index types to test
     * @return Vector of benchmark results
     */
    std::vector<BenchmarkResult> benchmarkBuildTime(
        const Mesh& mesh,
        const std::vector<SpatialIndexType>& types
    ) const;

    /**
     * @brief Benchmark bounding box query performance
     * @param mesh The mesh to query
     * @param types Vector of spatial index types to test
     * @param queryBox The bounding box to query
     * @param iterations Number of query iterations
     * @return Vector of benchmark results
     */
    std::vector<BenchmarkResult> benchmarkBoundingBoxQuery(
        const Mesh& mesh,
        const std::vector<SpatialIndexType>& types,
        const BoundingBox& queryBox,
        size_t iterations = 100
    ) const;

    /**
     * @brief Benchmark nearest neighbor query performance
     * @param mesh The mesh to query
     * @param types Vector of spatial index types to test
     * @param queryPoint The point to find nearest element to
     * @param iterations Number of query iterations
     * @return Vector of benchmark results
     */
    std::vector<BenchmarkResult> benchmarkNearestNeighbor(
        const Mesh& mesh,
        const std::vector<SpatialIndexType>& types,
        const Eigen::Vector3d& queryPoint,
        size_t iterations = 100
    ) const;

    /**
     * @brief Benchmark k-nearest neighbor query performance
     * @param mesh The mesh to query
     * @param types Vector of spatial index types to test
     * @param queryPoint The point to find nearest elements to
     * @param k Number of nearest neighbors to find
     * @param iterations Number of query iterations
     * @return Vector of benchmark results
     */
    std::vector<BenchmarkResult> benchmarkKNearestNeighbor(
        const Mesh& mesh,
        const std::vector<SpatialIndexType>& types,
        const Eigen::Vector3d& queryPoint,
        size_t k,
        size_t iterations = 100
    ) const;

    /**
     * @brief Benchmark radius search performance
     * @param mesh The mesh to query
     * @param types Vector of spatial index types to test
     * @param queryPoint The center point
     * @param radius Search radius
     * @param iterations Number of query iterations
     * @return Vector of benchmark results
     */
    std::vector<BenchmarkResult> benchmarkRadiusSearch(
        const Mesh& mesh,
        const std::vector<SpatialIndexType>& types,
        const Eigen::Vector3d& queryPoint,
        double radius,
        size_t iterations = 100
    ) const;

    /**
     * @brief Benchmark scalability with different mesh sizes
     * @param meshSizes Vector of mesh sizes to test (number of elements per dimension)
     * @param types Vector of spatial index types to test
     * @return Vector of benchmark results (one per mesh size per type)
     */
    std::vector<BenchmarkResult> benchmarkScalability(
        const std::vector<size_t>& meshSizes,
        const std::vector<SpatialIndexType>& types
    ) const;

    /**
     * @brief Run a comprehensive benchmark suite
     * @param mesh The mesh to benchmark
     * @param types Vector of spatial index types to test
     * @return Vector of all benchmark results
     */
    std::vector<BenchmarkResult> runComprehensiveBenchmark(
        const Mesh& mesh,
        const std::vector<SpatialIndexType>& types
    ) const;

    /**
     * @brief Print benchmark results in a readable format
     * @param results Vector of benchmark results to print
     */
    static void printResults(const std::vector<BenchmarkResult>& results);

    /**
     * @brief Print benchmark comparison table
     * @param results Vector of benchmark results to compare
     */
    static void printComparison(const std::vector<BenchmarkResult>& results);

    /**
     * @brief Export benchmark results to CSV format
     * @param results Vector of benchmark results
     * @param filename Output filename
     */
    static void exportToCSV(const std::vector<BenchmarkResult>& results,
                           const std::string& filename);

private:
    /**
     * @brief Measure execution time of a function
     * @param func Function to measure
     * @return Execution time in milliseconds
     */
    template<typename Func>
    double measureTime(Func func) const {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        return duration.count();
    }

    /**
     * @brief Estimate memory usage of a spatial index
     * @param index The spatial index
     * @param mesh The mesh it was built for
     * @return Estimated memory usage in bytes
     */
    size_t estimateMemoryUsage(const ISpatialIndex& index, const Mesh& mesh) const;

    /**
     * @brief Create a test mesh with specified grid size
     * @param gridSize Number of elements per dimension
     * @return Created mesh
     */
    std::unique_ptr<Mesh> createTestMesh(size_t gridSize) const;
};

} // namespace core
} // namespace koomesh
