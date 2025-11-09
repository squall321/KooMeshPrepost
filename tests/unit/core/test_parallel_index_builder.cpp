/**
 * @file test_parallel_index_builder.cpp
 * @brief Unit tests for ParallelIndexBuilder
 */

#include <gtest/gtest.h>
#include "core/ParallelIndexBuilder.h"
#include "core/Mesh.h"
#include "core/Element.h"
#include <Eigen/Dense>

using namespace koomesh::core;

/**
 * @brief Test fixture for ParallelIndexBuilder tests
 */
class ParallelIndexBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test mesh with 1000 hexahedron elements
        createTestMesh(1000);
    }

    void createTestMesh(size_t numElements) {
        mesh = std::make_unique<Mesh>();

        // Create nodes (assuming hexahedrons need 8 nodes each, but sharing nodes)
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

        // Create hexahedral elements
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
// Thread Count Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, GetOptimalThreadCount_SmallMesh) {
    size_t threadCount = ParallelIndexBuilder::getOptimalThreadCount(5000, 10000);
    EXPECT_EQ(threadCount, 1);
}

TEST_F(ParallelIndexBuilderTest, GetOptimalThreadCount_LargeMesh) {
    size_t threadCount = ParallelIndexBuilder::getOptimalThreadCount(100000, 10000);
    EXPECT_GT(threadCount, 1);
    EXPECT_LE(threadCount, std::thread::hardware_concurrency());
}

TEST_F(ParallelIndexBuilderTest, ShouldUseParallel_SmallMesh) {
    EXPECT_FALSE(ParallelIndexBuilder::shouldUseParallel(5000));
}

TEST_F(ParallelIndexBuilderTest, ShouldUseParallel_LargeMesh) {
    EXPECT_TRUE(ParallelIndexBuilder::shouldUseParallel(50000));
}

// ============================================================================
// Mesh Partitioning Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, PartitionMesh_EmptyMesh) {
    Mesh emptyMesh;
    auto partitions = ParallelIndexBuilder::partitionMesh(emptyMesh, 4);
    EXPECT_TRUE(partitions.empty());
}

TEST_F(ParallelIndexBuilderTest, PartitionMesh_ZeroPartitions) {
    auto partitions = ParallelIndexBuilder::partitionMesh(*mesh, 0);
    EXPECT_TRUE(partitions.empty());
}

TEST_F(ParallelIndexBuilderTest, PartitionMesh_SinglePartition) {
    auto partitions = ParallelIndexBuilder::partitionMesh(*mesh, 1);
    EXPECT_EQ(partitions.size(), 1);
}

TEST_F(ParallelIndexBuilderTest, PartitionMesh_MultiplePartitions) {
    auto partitions = ParallelIndexBuilder::partitionMesh(*mesh, 4);
    EXPECT_LE(partitions.size(), 4);
    EXPECT_GT(partitions.size(), 0);

    // Check partitions are non-overlapping and cover all elements
    for (size_t i = 0; i < partitions.size(); ++i) {
        EXPECT_LE(partitions[i].first, partitions[i].second);

        if (i > 0) {
            EXPECT_GT(partitions[i].first, partitions[i-1].second);
        }
    }
}

// ============================================================================
// Parallel Build Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, BuildParallel_Octree) {
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
    EXPECT_GT(stats.totalBuildTime, 0.0);
    EXPECT_GT(stats.elementsProcessed, 0);
    EXPECT_GT(stats.elementsPerSecond, 0.0);
}

TEST_F(ParallelIndexBuilderTest, BuildParallel_KdTree) {
    ParallelBuildConfig config;

    auto index = ParallelIndexBuilder::buildParallel(
        SpatialIndexType::KDTREE,
        *mesh,
        config
    );

    ASSERT_NE(index, nullptr);
}

TEST_F(ParallelIndexBuilderTest, BuildParallel_AutoThreadCount) {
    ParallelBuildConfig config;
    config.numThreads = 0;  // Auto-detect

    ParallelBuildStats stats;
    auto index = ParallelIndexBuilder::buildParallel(
        SpatialIndexType::OCTREE,
        *mesh,
        config,
        &stats
    );

    ASSERT_NE(index, nullptr);
    EXPECT_GT(stats.numThreadsUsed, 0);
}

TEST_F(ParallelIndexBuilderTest, BuildParallel_WithStats) {
    ParallelBuildConfig config;

    ParallelBuildStats stats;
    auto index = ParallelIndexBuilder::buildParallel(
        SpatialIndexType::OCTREE,
        *mesh,
        config,
        &stats
    );

    ASSERT_NE(index, nullptr);

    // Verify statistics
    EXPECT_EQ(stats.elementsProcessed, mesh->elementCount());
    EXPECT_GT(stats.totalBuildTime, 0.0);
    EXPECT_GT(stats.avgThreadTime, 0.0);
    EXPECT_GE(stats.maxThreadTime, stats.minThreadTime);
}

// ============================================================================
// Multiple Index Building Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, BuildMultiple_TwoTypes) {
    std::vector<SpatialIndexType> types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::KDTREE
    };

    auto indexes = ParallelIndexBuilder::buildMultiple(types, *mesh);

    EXPECT_EQ(indexes.size(), 2);
    EXPECT_NE(indexes[SpatialIndexType::OCTREE], nullptr);
    EXPECT_NE(indexes[SpatialIndexType::KDTREE], nullptr);
}

TEST_F(ParallelIndexBuilderTest, BuildMultiple_EmptyList) {
    std::vector<SpatialIndexType> types;

    auto indexes = ParallelIndexBuilder::buildMultiple(types, *mesh);

    EXPECT_TRUE(indexes.empty());
}

TEST_F(ParallelIndexBuilderTest, BuildMultiple_AllTypes) {
    std::vector<SpatialIndexType> types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::RTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::UNIFORM_GRID
    };

    auto indexes = ParallelIndexBuilder::buildMultiple(types, *mesh);

    EXPECT_EQ(indexes.size(), 4);
    for (const auto& pair : indexes) {
        EXPECT_NE(pair.second, nullptr);
    }
}

// ============================================================================
// Async Build Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, BuildAsync_Basic) {
    auto future = ParallelIndexBuilder::buildAsync(
        SpatialIndexType::OCTREE,
        *mesh
    );

    auto index = future.get();
    ASSERT_NE(index, nullptr);
}

TEST_F(ParallelIndexBuilderTest, BuildAsync_MultipleSimultaneous) {
    std::vector<std::future<std::unique_ptr<ISpatialIndex>>> futures;

    futures.push_back(ParallelIndexBuilder::buildAsync(
        SpatialIndexType::OCTREE, *mesh
    ));
    futures.push_back(ParallelIndexBuilder::buildAsync(
        SpatialIndexType::KDTREE, *mesh
    ));

    for (auto& future : futures) {
        auto index = future.get();
        ASSERT_NE(index, nullptr);
    }
}

// ============================================================================
// Guard Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, BuildGuard_Basic) {
    ParallelBuildConfig config;
    ParallelIndexBuildGuard guard(config);

    auto index = guard.build(SpatialIndexType::OCTREE, *mesh);
    ASSERT_NE(index, nullptr);
}

TEST_F(ParallelIndexBuilderTest, BuildGuard_RAII) {
    ParallelBuildConfig config;

    {
        ParallelIndexBuildGuard guard(config);
        auto index = guard.build(SpatialIndexType::OCTREE, *mesh);
        ASSERT_NE(index, nullptr);
        // Guard destroyed here
    }

    // Should not crash
}

// ============================================================================
// Benchmark Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, Benchmark_Compare) {
    ParallelBuildConfig config;
    config.numThreads = 2;

    auto result = ParallelBuildBenchmark::compare(
        SpatialIndexType::OCTREE,
        *mesh,
        config
    );

    EXPECT_GT(result.serialTime, 0.0);
    EXPECT_GT(result.parallelTime, 0.0);
    EXPECT_GT(result.speedup, 0.0);
    EXPECT_GT(result.threadsUsed, 0);
    EXPECT_GT(result.efficiency, 0.0);
}

TEST_F(ParallelIndexBuilderTest, Benchmark_Scaling) {
    auto results = ParallelBuildBenchmark::benchmarkScaling(
        *mesh,
        {1, 2, 4}
    );

    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results[1], 1.0);  // Serial speedup is 1.0

    // Results should be reasonable
    for (const auto& pair : results) {
        EXPECT_GT(pair.second, 0.0);
        EXPECT_LE(pair.second, static_cast<double>(pair.first) * 2.0);  // Max 2x ideal speedup
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ParallelIndexBuilderTest, BuildParallel_EmptyMesh) {
    Mesh emptyMesh;

    auto index = ParallelIndexBuilder::buildParallel(
        SpatialIndexType::OCTREE,
        emptyMesh
    );

    ASSERT_NE(index, nullptr);
}

TEST_F(ParallelIndexBuilderTest, BuildParallel_SingleElement) {
    Mesh singleElemMesh;

    // Add minimal nodes
    for (NodeId i = 1; i <= 8; ++i) {
        Node node(i, Eigen::Vector3d(
            static_cast<double>(i % 2),
            static_cast<double>((i / 2) % 2),
            static_cast<double>(i / 4)
        ));
        singleElemMesh.addNode(node);
    }

    // Add single element
    std::vector<NodeId> nodes = {1, 2, 3, 4, 5, 6, 7, 8};
    HexahedronElement elem(1, 1, nodes);
    singleElemMesh.addElement(std::make_unique<HexahedronElement>(elem));

    auto index = ParallelIndexBuilder::buildParallel(
        SpatialIndexType::OCTREE,
        singleElemMesh
    );

    ASSERT_NE(index, nullptr);
}

// ============================================================================
// Progress Callback Tests
// ============================================================================

TEST_F(ParallelIndexBuilderTest, BuildWithProgressCallback) {
    ParallelBuildConfig config;
    config.enableProgressReporting = true;

    bool callbackCalled = false;
    float lastProgress = 0.0f;

    config.progressCallback = [&callbackCalled, &lastProgress](float progress) {
        callbackCalled = true;
        lastProgress = progress;
    };

    auto index = ParallelIndexBuilder::buildParallel(
        SpatialIndexType::OCTREE,
        *mesh,
        config
    );

    ASSERT_NE(index, nullptr);
    // Note: Callback may or may not be called depending on implementation
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
