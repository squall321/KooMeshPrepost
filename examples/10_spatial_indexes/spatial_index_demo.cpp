/**
 * @file spatial_index_demo.cpp
 * @brief Comprehensive demonstration of spatial index functionality
 *
 * This example demonstrates:
 * - Creating and using different spatial index types
 * - Performing various query operations
 * - Comparing performance of different indexes
 * - Using the factory pattern for index creation
 */

#include "core/Mesh.h"
#include "core/SpatialIndexFactory.h"
#include "core/SpatialIndexBenchmark.h"
#include "core/SpatialQueryOptimizer.h"
#include "core/PerformanceDocGenerator.h"
#include <iostream>
#include <iomanip>

using namespace koomesh::core;

/**
 * @brief Create a simple test mesh
 */
std::unique_ptr<Mesh> createTestMesh(int gridSize = 10) {
    auto mesh = std::make_unique<Mesh>();

    // Create nodes in a grid pattern
    NodeId nodeId = 1;
    for (int k = 0; k <= gridSize; ++k) {
        for (int j = 0; j <= gridSize; ++j) {
            for (int i = 0; i <= gridSize; ++i) {
                double x = static_cast<double>(i);
                double y = static_cast<double>(j);
                double z = static_cast<double>(k);
                Node node(nodeId++, Eigen::Vector3d(x, y, z));
                mesh->addNode(node);
            }
        }
    }

    // Create hexahedral elements
    ElementId elemId = 1;
    int nodesPerSide = gridSize + 1;
    for (int k = 0; k < gridSize; ++k) {
        for (int j = 0; j < gridSize; ++j) {
            for (int i = 0; i < gridSize; ++i) {
                NodeId n000 = 1 + i + j * nodesPerSide + k * nodesPerSide * nodesPerSide;
                NodeId n100 = n000 + 1;
                NodeId n010 = n000 + nodesPerSide;
                NodeId n110 = n010 + 1;
                NodeId n001 = n000 + nodesPerSide * nodesPerSide;
                NodeId n101 = n001 + 1;
                NodeId n011 = n001 + nodesPerSide;
                NodeId n111 = n011 + 1;

                std::vector<NodeId> nodes = {n000, n100, n110, n010, n001, n101, n111, n011};
                HexahedronElement elem(elemId++, 1, nodes);
                mesh->addElement(std::make_unique<HexahedronElement>(elem));
            }
        }
    }

    std::cout << "Created test mesh with " << mesh->nodeCount()
              << " nodes and " << mesh->elementCount() << " elements\n";

    return mesh;
}

/**
 * @brief Demo 1: Basic spatial index usage
 */
void demo1_BasicUsage(const Mesh& mesh) {
    std::cout << "\n=== Demo 1: Basic Spatial Index Usage ===\n\n";

    // Create an Octree index
    auto octree = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    octree->build(mesh);

    std::cout << "Built Octree index\n";
    std::cout << octree->getStatistics() << "\n";

    // Perform bounding box query
    BoundingBox queryBox(Eigen::Vector3d(2, 2, 2), Eigen::Vector3d(5, 5, 5));
    auto results = octree->query(queryBox);

    std::cout << "Bounding box query [2,2,2]-[5,5,5] found "
              << results.size() << " elements\n";

    // Find nearest neighbor
    Eigen::Vector3d queryPoint(5.5, 5.5, 5.5);
    ElementId nearest = octree->findNearest(queryPoint);

    std::cout << "Nearest element to point (5.5, 5.5, 5.5) is element #"
              << nearest << "\n";
}

/**
 * @brief Demo 2: Comparing different index types
 */
void demo2_CompareIndexTypes(const Mesh& mesh) {
    std::cout << "\n=== Demo 2: Comparing Index Types ===\n\n";

    std::vector<SpatialIndexType> types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::RTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::UNIFORM_GRID
    };

    BoundingBox queryBox(Eigen::Vector3d(3, 3, 3), Eigen::Vector3d(7, 7, 7));

    for (auto type : types) {
        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);

        auto results = index->query(queryBox);

        std::cout << std::setw(15) << SpatialIndexFactory::typeToString(type)
                  << ": Found " << std::setw(4) << results.size() << " elements\n";
    }
}

/**
 * @brief Demo 3: Using the factory with automatic selection
 */
void demo3_AutomaticSelection(const Mesh& mesh) {
    std::cout << "\n=== Demo 3: Automatic Index Selection ===\n\n";

    // Let the factory choose the optimal index
    auto index = SpatialIndexFactory::createOptimal(mesh);

    std::cout << "Factory automatically selected optimal index for this mesh\n";

    // Note: The actual index type is determined internally by the factory
    // based on mesh characteristics

    index->build(mesh);

    // Use the index
    Eigen::Vector3d point(5, 5, 5);
    auto kNearest = index->findKNearest(point, 5);

    std::cout << "Found " << kNearest.size() << " nearest neighbors\n";
}

/**
 * @brief Demo 4: Query optimizer with caching
 */
void demo4_QueryOptimizer(const Mesh& mesh) {
    std::cout << "\n=== Demo 4: Query Optimizer with Caching ===\n\n";

    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(mesh);
    optimizer.enableCache(100, 30.0);  // 100 entries, 30 second TTL

    std::cout << "Performing queries...\n";

    // Perform some queries
    BoundingBox box1(Eigen::Vector3d(2, 2, 2), Eigen::Vector3d(4, 4, 4));
    BoundingBox box2(Eigen::Vector3d(5, 5, 5), Eigen::Vector3d(7, 7, 7));

    // First queries - cache misses
    auto results1a = optimizer.queryBoundingBox(box1);
    auto results2a = optimizer.queryBoundingBox(box2);

    // Repeat queries - cache hits
    auto results1b = optimizer.queryBoundingBox(box1);
    auto results2b = optimizer.queryBoundingBox(box2);

    std::cout << optimizer.getCacheStatistics() << "\n";

    // Perform many nearest neighbor queries
    for (int i = 0; i < 20; ++i) {
        Eigen::Vector3d point(i * 0.5, i * 0.5, i * 0.5);
        optimizer.findNearest(point);
    }

    // Get recommendation
    std::cout << "\nRecommended index type based on query patterns: "
              << SpatialIndexFactory::typeToString(optimizer.recommendIndexType())
              << "\n";

    std::cout << optimizer.getOptimizationReport();
}

/**
 * @brief Demo 5: Performance benchmarking
 */
void demo5_Benchmarking(const Mesh& mesh) {
    std::cout << "\n=== Demo 5: Performance Benchmarking ===\n\n";

    SpatialIndexBenchmark benchmark;

    std::vector<SpatialIndexType> types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::RTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::UNIFORM_GRID
    };

    // Benchmark build time
    std::cout << "Benchmarking build time...\n";
    auto buildResults = benchmark.benchmarkBuildTime(mesh, types);
    SpatialIndexBenchmark::printComparison(buildResults);

    // Benchmark nearest neighbor query
    std::cout << "\nBenchmarking nearest neighbor queries...\n";
    Eigen::Vector3d queryPoint(5.0, 5.0, 5.0);
    auto nnResults = benchmark.benchmarkNearestNeighbor(mesh, types, queryPoint, 100);
    SpatialIndexBenchmark::printComparison(nnResults);
}

/**
 * @brief Demo 6: Generate performance documentation
 */
void demo6_Documentation(const Mesh& mesh) {
    std::cout << "\n=== Demo 6: Generate Performance Documentation ===\n\n";

    SpatialIndexBenchmark benchmark;

    std::vector<SpatialIndexType> types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::RTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::UNIFORM_GRID
    };

    // Run comprehensive benchmark
    auto results = benchmark.runComprehensiveBenchmark(mesh, types);

    // Generate documentation
    PerformanceDocGenerator generator;
    generator.setSystemInfo("Linux 4.4.0", "Example CPU", "Example RAM");
    generator.setProjectInfo("Spatial Index Demo", "1.0.0");

    // Generate markdown
    std::string markdown = generator.generateMarkdown(results);
    if (PerformanceDocGenerator::saveToFile(markdown, "spatial_index_performance.md")) {
        std::cout << "Saved Markdown report to: spatial_index_performance.md\n";
    }

    // Generate HTML
    std::string html = generator.generateHTML(results);
    if (PerformanceDocGenerator::saveToFile(html, "spatial_index_performance.html")) {
        std::cout << "Saved HTML report to: spatial_index_performance.html\n";
    }

    // Generate JSON
    std::string json = generator.generateJSON(results);
    if (PerformanceDocGenerator::saveToFile(json, "spatial_index_performance.json")) {
        std::cout << "Saved JSON data to: spatial_index_performance.json\n";
    }

    std::cout << "\nPerformance reports generated successfully!\n";
}

/**
 * @brief Main function
 */
int main() {
    std::cout << "=================================================\n";
    std::cout << "  Spatial Index Integration Demo\n";
    std::cout << "=================================================\n";

    try {
        // Create test mesh
        auto mesh = createTestMesh(10);  // 10x10x10 grid = 1000 elements

        // Run demos
        demo1_BasicUsage(*mesh);
        demo2_CompareIndexTypes(*mesh);
        demo3_AutomaticSelection(*mesh);
        demo4_QueryOptimizer(*mesh);
        demo5_Benchmarking(*mesh);
        demo6_Documentation(*mesh);

        std::cout << "\n=================================================\n";
        std::cout << "  All demos completed successfully!\n";
        std::cout << "=================================================\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
