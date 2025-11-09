#include "core/SpatialIndexBenchmark.h"
#include "core/SpatialIndexFactory.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

namespace koomesh {
namespace core {

std::vector<BenchmarkResult> SpatialIndexBenchmark::benchmarkBuildTime(
    const Mesh& mesh,
    const std::vector<SpatialIndexType>& types
) const {
    std::vector<BenchmarkResult> results;

    for (auto type : types) {
        BenchmarkResult result;
        result.indexType = SpatialIndexFactory::typeToString(type);
        result.operationType = "build";
        result.meshSize = mesh.elementCount();
        result.queryCount = 1;

        // Create index and measure build time
        auto index = SpatialIndexFactory::create(type);
        result.timeMilliseconds = measureTime([&]() {
            index->build(mesh);
        });

        result.memoryBytes = estimateMemoryUsage(*index, mesh);

        results.push_back(result);
    }

    return results;
}

std::vector<BenchmarkResult> SpatialIndexBenchmark::benchmarkBoundingBoxQuery(
    const Mesh& mesh,
    const std::vector<SpatialIndexType>& types,
    const BoundingBox& queryBox,
    size_t iterations
) const {
    std::vector<BenchmarkResult> results;

    for (auto type : types) {
        BenchmarkResult result;
        result.indexType = SpatialIndexFactory::typeToString(type);
        result.operationType = "bounding_box_query";
        result.meshSize = mesh.elementCount();
        result.queryCount = iterations;

        // Build index
        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);

        // Measure query time
        size_t totalResults = 0;
        result.timeMilliseconds = measureTime([&]() {
            for (size_t i = 0; i < iterations; ++i) {
                auto queryResults = index->query(queryBox);
                totalResults += queryResults.size();
            }
        });

        result.resultCount = totalResults / iterations;
        result.memoryBytes = estimateMemoryUsage(*index, mesh);

        results.push_back(result);
    }

    return results;
}

std::vector<BenchmarkResult> SpatialIndexBenchmark::benchmarkNearestNeighbor(
    const Mesh& mesh,
    const std::vector<SpatialIndexType>& types,
    const Eigen::Vector3d& queryPoint,
    size_t iterations
) const {
    std::vector<BenchmarkResult> results;

    for (auto type : types) {
        BenchmarkResult result;
        result.indexType = SpatialIndexFactory::typeToString(type);
        result.operationType = "nearest_neighbor";
        result.meshSize = mesh.elementCount();
        result.queryCount = iterations;

        // Build index
        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);

        // Measure query time
        size_t foundCount = 0;
        result.timeMilliseconds = measureTime([&]() {
            for (size_t i = 0; i < iterations; ++i) {
                ElementId nearest = index->findNearest(queryPoint);
                if (nearest != 0) {
                    foundCount++;
                }
            }
        });

        result.resultCount = foundCount;
        result.memoryBytes = estimateMemoryUsage(*index, mesh);

        results.push_back(result);
    }

    return results;
}

std::vector<BenchmarkResult> SpatialIndexBenchmark::benchmarkKNearestNeighbor(
    const Mesh& mesh,
    const std::vector<SpatialIndexType>& types,
    const Eigen::Vector3d& queryPoint,
    size_t k,
    size_t iterations
) const {
    std::vector<BenchmarkResult> results;

    for (auto type : types) {
        BenchmarkResult result;
        result.indexType = SpatialIndexFactory::typeToString(type);
        result.operationType = "k_nearest_neighbor";
        result.meshSize = mesh.elementCount();
        result.queryCount = iterations;

        // Build index
        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);

        // Measure query time
        size_t totalResults = 0;
        result.timeMilliseconds = measureTime([&]() {
            for (size_t i = 0; i < iterations; ++i) {
                auto neighbors = index->findKNearest(queryPoint, k);
                totalResults += neighbors.size();
            }
        });

        result.resultCount = totalResults / iterations;
        result.memoryBytes = estimateMemoryUsage(*index, mesh);

        results.push_back(result);
    }

    return results;
}

std::vector<BenchmarkResult> SpatialIndexBenchmark::benchmarkRadiusSearch(
    const Mesh& mesh,
    const std::vector<SpatialIndexType>& types,
    const Eigen::Vector3d& queryPoint,
    double radius,
    size_t iterations
) const {
    std::vector<BenchmarkResult> results;

    for (auto type : types) {
        BenchmarkResult result;
        result.indexType = SpatialIndexFactory::typeToString(type);
        result.operationType = "radius_search";
        result.meshSize = mesh.elementCount();
        result.queryCount = iterations;

        // Build index
        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);

        // Measure query time
        size_t totalResults = 0;
        result.timeMilliseconds = measureTime([&]() {
            for (size_t i = 0; i < iterations; ++i) {
                auto elementsInRadius = index->findWithinRadius(queryPoint, radius);
                totalResults += elementsInRadius.size();
            }
        });

        result.resultCount = totalResults / iterations;
        result.memoryBytes = estimateMemoryUsage(*index, mesh);

        results.push_back(result);
    }

    return results;
}

std::vector<BenchmarkResult> SpatialIndexBenchmark::benchmarkScalability(
    const std::vector<size_t>& meshSizes,
    const std::vector<SpatialIndexType>& types
) const {
    std::vector<BenchmarkResult> results;

    for (size_t meshSize : meshSizes) {
        // Create test mesh
        auto mesh = createTestMesh(meshSize);

        // Benchmark each type
        for (auto type : types) {
            BenchmarkResult result;
            result.indexType = SpatialIndexFactory::typeToString(type);
            result.operationType = "scalability_build";
            result.meshSize = mesh->elementCount();
            result.queryCount = 1;

            // Create index and measure build time
            auto index = SpatialIndexFactory::create(type);
            result.timeMilliseconds = measureTime([&]() {
                index->build(*mesh);
            });

            result.memoryBytes = estimateMemoryUsage(*index, *mesh);

            results.push_back(result);
        }
    }

    return results;
}

std::vector<BenchmarkResult> SpatialIndexBenchmark::runComprehensiveBenchmark(
    const Mesh& mesh,
    const std::vector<SpatialIndexType>& types
) const {
    std::vector<BenchmarkResult> results;

    // 1. Build time benchmark
    auto buildResults = benchmarkBuildTime(mesh, types);
    results.insert(results.end(), buildResults.begin(), buildResults.end());

    // 2. Bounding box query benchmark
    // Use a box in the center of the mesh
    Eigen::Vector3d center(0.5, 0.5, 0.5);
    BoundingBox queryBox(center - Eigen::Vector3d(0.2, 0.2, 0.2),
                         center + Eigen::Vector3d(0.2, 0.2, 0.2));
    auto bbResults = benchmarkBoundingBoxQuery(mesh, types, queryBox, 100);
    results.insert(results.end(), bbResults.begin(), bbResults.end());

    // 3. Nearest neighbor benchmark
    auto nnResults = benchmarkNearestNeighbor(mesh, types, center, 100);
    results.insert(results.end(), nnResults.begin(), nnResults.end());

    // 4. K-nearest neighbor benchmark
    auto knnResults = benchmarkKNearestNeighbor(mesh, types, center, 10, 50);
    results.insert(results.end(), knnResults.begin(), knnResults.end());

    // 5. Radius search benchmark
    auto rsResults = benchmarkRadiusSearch(mesh, types, center, 0.5, 50);
    results.insert(results.end(), rsResults.begin(), rsResults.end());

    return results;
}

void SpatialIndexBenchmark::printResults(const std::vector<BenchmarkResult>& results) {
    std::cout << "\n";
    std::cout << "==================================================================" << std::endl;
    std::cout << " Spatial Index Benchmark Results" << std::endl;
    std::cout << "==================================================================" << std::endl;

    for (const auto& result : results) {
        std::cout << "\nIndex Type: " << result.indexType << std::endl;
        std::cout << "  Operation: " << result.operationType << std::endl;
        std::cout << "  Mesh Size: " << result.meshSize << " elements" << std::endl;
        std::cout << "  Time: " << std::fixed << std::setprecision(3)
                  << result.timeMilliseconds << " ms" << std::endl;
        std::cout << "  Queries: " << result.queryCount << std::endl;
        if (result.queryCount > 0) {
            std::cout << "  Avg Time/Query: " << std::fixed << std::setprecision(4)
                      << (result.timeMilliseconds / result.queryCount) << " ms" << std::endl;
        }
        if (result.resultCount > 0) {
            std::cout << "  Results: " << result.resultCount << std::endl;
        }
        std::cout << "  Memory: " << (result.memoryBytes / 1024) << " KB" << std::endl;
    }

    std::cout << "==================================================================" << std::endl;
}

void SpatialIndexBenchmark::printComparison(const std::vector<BenchmarkResult>& results) {
    if (results.empty()) {
        return;
    }

    // Group results by operation type
    std::map<std::string, std::vector<BenchmarkResult>> groupedResults;
    for (const auto& result : results) {
        groupedResults[result.operationType].push_back(result);
    }

    std::cout << "\n";
    std::cout << "==================================================================" << std::endl;
    std::cout << " Spatial Index Performance Comparison" << std::endl;
    std::cout << "==================================================================" << std::endl;

    for (const auto& group : groupedResults) {
        std::cout << "\nOperation: " << group.first << std::endl;
        std::cout << std::string(66, '-') << std::endl;
        std::cout << std::left << std::setw(15) << "Index Type"
                  << std::right << std::setw(12) << "Time (ms)"
                  << std::setw(15) << "Avg (ms/q)"
                  << std::setw(12) << "Memory (KB)"
                  << std::setw(12) << "Speedup" << std::endl;
        std::cout << std::string(66, '-') << std::endl;

        // Find fastest time for speedup calculation
        double fastestTime = std::numeric_limits<double>::max();
        for (const auto& result : group.second) {
            if (result.timeMilliseconds < fastestTime) {
                fastestTime = result.timeMilliseconds;
            }
        }

        for (const auto& result : group.second) {
            double avgTime = result.queryCount > 0 ?
                           result.timeMilliseconds / result.queryCount : 0.0;
            double speedup = fastestTime > 0.0 ?
                           result.timeMilliseconds / fastestTime : 1.0;

            std::cout << std::left << std::setw(15) << result.indexType
                      << std::right << std::fixed << std::setprecision(3)
                      << std::setw(12) << result.timeMilliseconds
                      << std::setw(15) << std::setprecision(4) << avgTime
                      << std::setw(12) << std::setprecision(0) << (result.memoryBytes / 1024)
                      << std::setw(12) << std::setprecision(2) << speedup << "x"
                      << std::endl;
        }
    }

    std::cout << "==================================================================" << std::endl;
}

void SpatialIndexBenchmark::exportToCSV(const std::vector<BenchmarkResult>& results,
                                       const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // Write header
    file << "IndexType,Operation,MeshSize,Time_ms,QueryCount,AvgTime_ms,ResultCount,Memory_KB\n";

    // Write data
    for (const auto& result : results) {
        double avgTime = result.queryCount > 0 ?
                       result.timeMilliseconds / result.queryCount : 0.0;

        file << result.indexType << ","
             << result.operationType << ","
             << result.meshSize << ","
             << std::fixed << std::setprecision(4) << result.timeMilliseconds << ","
             << result.queryCount << ","
             << avgTime << ","
             << result.resultCount << ","
             << (result.memoryBytes / 1024) << "\n";
    }

    file.close();
    std::cout << "Results exported to: " << filename << std::endl;
}

size_t SpatialIndexBenchmark::estimateMemoryUsage(const ISpatialIndex& index,
                                                   const Mesh& mesh) const {
    // Parse statistics to estimate memory usage
    std::string stats = index.getStatistics();

    size_t memoryEstimate = 0;

    // Base overhead per element (rough estimate)
    size_t elementCount = mesh.elementCount();
    memoryEstimate += elementCount * sizeof(ElementId);

    // Add per-index overhead (rough estimates)
    // These are approximations based on typical data structure sizes
    memoryEstimate += elementCount * 32; // Pointers, metadata, etc.

    // Try to parse actual statistics if available
    // Look for memory-related keywords in stats
    std::istringstream iss(stats);
    std::string line;
    while (std::getline(iss, line)) {
        // Simple parsing - can be enhanced
        if (line.find("Node") != std::string::npos ||
            line.find("Cell") != std::string::npos) {
            // Count nodes/cells and estimate their size
            memoryEstimate += 256; // Rough estimate per node/cell
        }
    }

    return memoryEstimate;
}

std::unique_ptr<Mesh> SpatialIndexBenchmark::createTestMesh(size_t gridSize) const {
    auto mesh = std::make_unique<Mesh>();

    // Create nodes in a grid pattern
    NodeId nodeId = 1;
    for (size_t k = 0; k < gridSize + 1; ++k) {
        for (size_t j = 0; j < gridSize + 1; ++j) {
            for (size_t i = 0; i < gridSize + 1; ++i) {
                double x = static_cast<double>(i) / gridSize;
                double y = static_cast<double>(j) / gridSize;
                double z = static_cast<double>(k) / gridSize;
                Node node(nodeId++, Eigen::Vector3d(x, y, z));
                mesh->addNode(node);
            }
        }
    }

    // Create hexahedral elements
    ElementId elemId = 1;
    size_t nodesPerSide = gridSize + 1;
    for (size_t k = 0; k < gridSize; ++k) {
        for (size_t j = 0; j < gridSize; ++j) {
            for (size_t i = 0; i < gridSize; ++i) {
                NodeId n000 = 1 + i + j * nodesPerSide + k * nodesPerSide * nodesPerSide;
                NodeId n100 = n000 + 1;
                NodeId n010 = n000 + nodesPerSide;
                NodeId n110 = n010 + 1;
                NodeId n001 = n000 + nodesPerSide * nodesPerSide;
                NodeId n101 = n001 + 1;
                NodeId n011 = n001 + nodesPerSide;
                NodeId n111 = n011 + 1;

                std::vector<NodeId> nodes = {n000, n100, n110, n010, n001, n101, n111, n011};
                HexahedronElement elem(elemId++, 1, nodes);  // PartId = 1
                mesh->addElement(std::make_unique<HexahedronElement>(elem));
            }
        }
    }

    return mesh;
}

} // namespace core
} // namespace koomesh
