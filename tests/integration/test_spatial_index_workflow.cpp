/**
 * @file test_spatial_index_workflow.cpp
 * @brief Integration tests for complete spatial index workflows
 */

#include <gtest/gtest.h>
#include "core/Mesh.h"
#include "core/Element.h"
#include "core/SpatialIndexFactory.h"
#include "core/SpatialIndexBenchmark.h"
#include "core/SpatialQueryOptimizer.h"
#include "core/SpatialIndexSerializer.h"
#include "core/ParallelIndexBuilder.h"
#include "selection/SelectionManager.h"
#include <Eigen/Dense>

using namespace koomesh::core;
using namespace koomesh::selection;

/**
 * @brief Test fixture for integration tests
 */
class SpatialIndexWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a moderately sized test mesh
        createTestMesh(1000);
    }

    void createTestMesh(size_t numElements) {
        mesh = std::make_unique<Mesh>();

        size_t nodesPerSide = static_cast<size_t>(std::ceil(std::cbrt(static_cast<double>(numElements)))) + 1;

        NodeId nodeId = 1;
        for (size_t k = 0; k < nodesPerSide; ++k) {
            for (size_t j = 0; j < nodesPerSide; ++j) {
                for (size_t i = 0; i < nodesPerSide; ++i) {
                    double x = static_cast<double>(i);
                    double y = static_cast<double>(j);
                    double z = static_cast<double>(k);
                    Node node(nodeId++, Eigen::Vector3d(x, y, z));
                    mesh->addNode(node);
                }
            }
        }

        ElementId elemId = 1;
        size_t elementsPerSide = nodesPerSide - 1;

        for (size_t k = 0; k < elementsPerSide && elemId <= numElements; ++k) {
            for (size_t j = 0; j < elementsPerSide && elemId <= numElements; ++j) {
                for (size_t i = 0; i < elementsPerSide && elemId <= numElements; ++i) {
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
    }

    std::unique_ptr<Mesh> mesh;
};

// ============================================================================
// Complete Workflow Tests
// ============================================================================

TEST_F(SpatialIndexWorkflowTest, CompleteWorkflow_BuildQuerySelect) {
    // 1. Build spatial index
    auto index = SpatialIndexFactory::createOptimal(*mesh);
    ASSERT_NE(index, nullptr);

    // 2. Perform spatial query
    BoundingBox queryBox;
    queryBox.minX = 2.0;
    queryBox.minY = 2.0;
    queryBox.minZ = 2.0;
    queryBox.maxX = 5.0;
    queryBox.maxY = 5.0;
    queryBox.maxZ = 5.0;

    auto elements = index->query(queryBox);
    EXPECT_GT(elements.size(), 0);

    // 3. Use selection manager
    SelectionManager selector;
    selector.setMesh(*mesh);
    selector.useSpatialIndex(SpatialIndexType::OCTREE);

    size_t selected = selector.selectByBox(
        Eigen::Vector3d(2, 2, 2),
        Eigen::Vector3d(5, 5, 5),
        SelectionMode::REPLACE
    );

    EXPECT_GT(selected, 0);
}

TEST_F(SpatialIndexWorkflowTest, BenchmarkCompareAll) {
    // Run comprehensive benchmark
    SpatialIndexBenchmark benchmark(*mesh);
    auto results = benchmark.runComprehensiveBenchmark();

    ASSERT_FALSE(results.empty());

    // Verify all index types were tested
    EXPECT_TRUE(results.find(SpatialIndexType::OCTREE) != results.end());
    EXPECT_TRUE(results.find(SpatialIndexType::RTREE) != results.end());
    EXPECT_TRUE(results.find(SpatialIndexType::KDTREE) != results.end());
    EXPECT_TRUE(results.find(SpatialIndexType::UNIFORM_GRID) != results.end());

    // Verify results are reasonable
    for (const auto& pair : results) {
        EXPECT_GT(pair.second.buildTime, 0.0);
        EXPECT_GT(pair.second.avgQueryTime, 0.0);
    }
}

TEST_F(SpatialIndexWorkflowTest, SerializeAndReuse) {
    std::string cachePath = "/tmp/test_workflow.sidx";

    // First run - build and cache
    auto index1 = SpatialIndexCache::getOrBuild(
        SpatialIndexType::OCTREE,
        *mesh,
        cachePath
    );
    ASSERT_NE(index1, nullptr);

    // Second run - load from cache
    auto index2 = SpatialIndexCache::getOrBuild(
        SpatialIndexType::OCTREE,
        *mesh,
        cachePath
    );
    ASSERT_NE(index2, nullptr);

    // Cleanup
    std::remove(cachePath.c_str());
}

TEST_F(SpatialIndexWorkflowTest, QueryOptimizerAdaptive) {
    // Create optimizer
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh, SpatialIndexType::OCTREE);
    optimizer.enableCache(100, 60.0);

    // Perform many point queries (should favor K-d Tree)
    Eigen::Vector3d queryPoint(5, 5, 5);
    for (int i = 0; i < 20; ++i) {
        auto nearest = optimizer.findNearest(queryPoint);
        EXPECT_GT(nearest, 0);
    }

    // Get recommendation
    auto recommended = optimizer.recommendIndexType();

    // Should recommend K-d Tree after many nearest neighbor queries
    // Note: May vary based on implementation
    EXPECT_TRUE(
        recommended == SpatialIndexType::KDTREE ||
        recommended == SpatialIndexType::OCTREE
    );

    // Get statistics
    auto stats = optimizer.getStatistics();
    EXPECT_GT(stats.totalQueries, 0);
}

TEST_F(SpatialIndexWorkflowTest, ParallelBuildAndQuery) {
    // Build index in parallel
    ParallelBuildConfig config;
    config.numThreads = 2;

    ParallelBuildStats stats;
    auto index = ParallelIndexBuilder::buildParallel(
        SpatialIndexType::OCTREE,
        *mesh,
        config,
        &stats
    );

    ASSERT_NE(index, nullptr);
    EXPECT_GT(stats.elementsProcessed, 0);

    // Query the parallel-built index
    BoundingBox queryBox;
    queryBox.minX = 0.0;
    queryBox.minY = 0.0;
    queryBox.minZ = 0.0;
    queryBox.maxX = 3.0;
    queryBox.maxY = 3.0;
    queryBox.maxZ = 3.0;

    auto elements = index->query(queryBox);
    EXPECT_GT(elements.size(), 0);
}

TEST_F(SpatialIndexWorkflowTest, SelectionWithHistory) {
    SelectionManager selector;
    selector.setMesh(*mesh);
    selector.useSpatialIndex(SpatialIndexType::OCTREE);
    selector.enableHistory();

    // Make selection
    selector.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(3, 3, 3),
        SelectionMode::REPLACE
    );
    size_t count1 = selector.getSelection().size();

    // Make another selection
    selector.selectByBox(
        Eigen::Vector3d(5, 5, 5),
        Eigen::Vector3d(8, 8, 8),
        SelectionMode::ADD
    );
    size_t count2 = selector.getSelection().size();

    EXPECT_GT(count2, count1);

    // Undo
    selector.undo();
    EXPECT_EQ(selector.getSelection().size(), count1);

    // Redo
    selector.redo();
    EXPECT_EQ(selector.getSelection().size(), count2);
}

TEST_F(SpatialIndexWorkflowTest, MultipleIndexTypesComparison) {
    // Build multiple indexes concurrently
    std::vector<SpatialIndexType> types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::RTREE
    };

    auto indexes = ParallelIndexBuilder::buildMultiple(types, *mesh);

    ASSERT_EQ(indexes.size(), 3);

    // Compare query performance
    BoundingBox queryBox;
    queryBox.minX = 2.0;
    queryBox.minY = 2.0;
    queryBox.minZ = 2.0;
    queryBox.maxX = 5.0;
    queryBox.maxY = 5.0;
    queryBox.maxZ = 5.0;

    for (const auto& pair : indexes) {
        auto elements = pair.second->query(queryBox);
        EXPECT_GT(elements.size(), 0);
    }
}

TEST_F(SpatialIndexWorkflowTest, EndToEndPerformance) {
    // This test verifies the entire pipeline performs acceptably

    auto startTotal = std::chrono::high_resolution_clock::now();

    // 1. Build index
    auto startBuild = std::chrono::high_resolution_clock::now();
    auto index = SpatialIndexFactory::createOptimal(*mesh);
    auto endBuild = std::chrono::high_resolution_clock::now();
    double buildTime = std::chrono::duration<double>(endBuild - startBuild).count();

    // 2. Perform queries
    auto startQuery = std::chrono::high_resolution_clock::now();
    BoundingBox box;
    box.minX = 0; box.maxX = 5;
    box.minY = 0; box.maxY = 5;
    box.minZ = 0; box.maxZ = 5;

    for (int i = 0; i < 100; ++i) {
        auto elements = index->query(box);
        (void)elements;  // Suppress unused warning
    }
    auto endQuery = std::chrono::high_resolution_clock::now();
    double queryTime = std::chrono::duration<double>(endQuery - startQuery).count();

    auto endTotal = std::chrono::high_resolution_clock::now();
    double totalTime = std::chrono::duration<double>(endTotal - startTotal).count();

    // Performance assertions
    EXPECT_LT(buildTime, 5.0);      // Build should take < 5 seconds
    EXPECT_LT(queryTime, 1.0);      // 100 queries should take < 1 second
    EXPECT_LT(totalTime, 10.0);     // Total should take < 10 seconds

    std::cout << "\nPerformance Summary:\n";
    std::cout << "  Build Time: " << buildTime << "s\n";
    std::cout << "  Query Time (100x): " << queryTime << "s\n";
    std::cout << "  Avg Query Time: " << (queryTime / 100.0) * 1000.0 << "ms\n";
    std::cout << "  Total Time: " << totalTime << "s\n";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
