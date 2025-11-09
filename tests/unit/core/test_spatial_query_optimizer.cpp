#include <gtest/gtest.h>
#include "core/SpatialQueryOptimizer.h"
#include "core/Mesh.h"

using namespace koomesh::core;

class SpatialQueryOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mesh = std::make_unique<Mesh>();
        createTestMesh();
    }

    void createTestMesh() {
        // Create a 5x5x5 grid mesh
        NodeId nodeId = 1;
        for (int k = 0; k < 6; ++k) {
            for (int j = 0; j < 6; ++j) {
                for (int i = 0; i < 6; ++i) {
                    Node node(nodeId++, Eigen::Vector3d(i, j, k));
                    mesh->addNode(node);
                }
            }
        }

        // Add hexahedral elements
        ElementId elemId = 1;
        int nodesPerSide = 6;
        for (int k = 0; k < 5; ++k) {
            for (int j = 0; j < 5; ++j) {
                for (int i = 0; i < 5; ++i) {
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
// Basic Functionality Tests
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, Construction) {
    SpatialQueryOptimizer optimizer;
    EXPECT_EQ(optimizer.getCurrentIndexType(), SpatialIndexType::OCTREE);
}

TEST_F(SpatialQueryOptimizerTest, SetMesh) {
    SpatialQueryOptimizer optimizer;
    ASSERT_NO_THROW(optimizer.setMesh(*mesh));

    // Verify query works after setting mesh
    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    auto results = optimizer.queryBoundingBox(box);
    EXPECT_GT(results.size(), 0);
}

TEST_F(SpatialQueryOptimizerTest, SetMeshWithCustomIndexType) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh, SpatialIndexType::KDTREE);
    EXPECT_EQ(optimizer.getCurrentIndexType(), SpatialIndexType::KDTREE);
}

// ============================================================================
// Query Tests
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, BoundingBoxQuery) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    BoundingBox box(Eigen::Vector3d(1, 1, 1), Eigen::Vector3d(3, 3, 3));
    auto results = optimizer.queryBoundingBox(box);

    EXPECT_GT(results.size(), 0);
    EXPECT_GT(optimizer.getQueryPattern().boundingBoxQueries, 0);
}

TEST_F(SpatialQueryOptimizerTest, PointQuery) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    Eigen::Vector3d point(2.5, 2.5, 2.5);
    auto results = optimizer.queryPoint(point);

    EXPECT_GT(optimizer.getQueryPattern().pointQueries, 0);
}

TEST_F(SpatialQueryOptimizerTest, NearestNeighborQuery) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    Eigen::Vector3d point(2.5, 2.5, 2.5);
    ElementId nearest = optimizer.findNearest(point);

    EXPECT_NE(nearest, 0);
    EXPECT_GT(optimizer.getQueryPattern().nearestNeighborQueries, 0);
}

TEST_F(SpatialQueryOptimizerTest, KNearestQuery) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    Eigen::Vector3d point(2.5, 2.5, 2.5);
    auto results = optimizer.findKNearest(point, 5);

    EXPECT_LE(results.size(), 5);
    EXPECT_GT(optimizer.getQueryPattern().kNearestQueries, 0);
}

TEST_F(SpatialQueryOptimizerTest, RadiusSearch) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    Eigen::Vector3d point(2.5, 2.5, 2.5);
    auto results = optimizer.findWithinRadius(point, 2.0);

    EXPECT_GT(optimizer.getQueryPattern().radiusQueries, 0);
}

// ============================================================================
// Cache Tests
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, EnableDisableCache) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    optimizer.enableCache(100, 30.0);
    optimizer.disableCache();

    // Cache should not affect queries
    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    auto results = optimizer.queryBoundingBox(box);
    EXPECT_GT(results.size(), 0);
}

TEST_F(SpatialQueryOptimizerTest, CacheHit) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);
    optimizer.enableCache(100, 30.0);

    BoundingBox box(Eigen::Vector3d(1, 1, 1), Eigen::Vector3d(2, 2, 2));

    // First query - cache miss
    auto results1 = optimizer.queryBoundingBox(box);

    // Second query - should hit cache
    auto results2 = optimizer.queryBoundingBox(box);

    EXPECT_EQ(results1.size(), results2.size());

    // Check cache statistics
    std::string stats = optimizer.getCacheStatistics();
    EXPECT_NE(stats.find("Hit Rate"), std::string::npos);
}

TEST_F(SpatialQueryOptimizerTest, CacheClear) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);
    optimizer.enableCache(100, 30.0);

    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    optimizer.queryBoundingBox(box);

    optimizer.clearCache();

    // After clear, query should work normally
    auto results = optimizer.queryBoundingBox(box);
    EXPECT_GT(results.size(), 0);
}

TEST_F(SpatialQueryOptimizerTest, CacheEviction) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);
    optimizer.enableCache(5, 30.0);  // Small cache

    // Fill cache beyond capacity
    for (int i = 0; i < 10; ++i) {
        BoundingBox box(Eigen::Vector3d(i, i, i), Eigen::Vector3d(i+1, i+1, i+1));
        optimizer.queryBoundingBox(box);
    }

    // Cache should not exceed max size
    // (We can't directly check cache size, but the optimizer should handle it)
    std::string stats = optimizer.getCacheStatistics();
    EXPECT_NE(stats.find("Entries"), std::string::npos);
}

// ============================================================================
// Pattern Analysis Tests
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, QueryPatternTracking) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    // Perform various queries
    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    optimizer.queryBoundingBox(box);
    optimizer.queryBoundingBox(box);

    Eigen::Vector3d point(2.5, 2.5, 2.5);
    optimizer.findNearest(point);
    optimizer.findKNearest(point, 5);
    optimizer.findKNearest(point, 10);

    const auto& pattern = optimizer.getQueryPattern();
    EXPECT_EQ(pattern.boundingBoxQueries, 2);
    EXPECT_EQ(pattern.nearestNeighborQueries, 1);
    EXPECT_EQ(pattern.kNearestQueries, 2);
    EXPECT_NEAR(pattern.avgKValue, 7.5, 0.1);
}

TEST_F(SpatialQueryOptimizerTest, ResetQueryPattern) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    optimizer.queryBoundingBox(box);

    optimizer.resetQueryPattern();

    const auto& pattern = optimizer.getQueryPattern();
    EXPECT_EQ(pattern.boundingBoxQueries, 0);
}

// ============================================================================
// Recommendation Tests
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, RecommendIndexTypeForPointQueries) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    // Perform many point queries
    for (int i = 0; i < 100; ++i) {
        Eigen::Vector3d point(i % 5, (i/5) % 5, (i/25) % 5);
        optimizer.queryPoint(point);
    }

    auto recommended = optimizer.recommendIndexType();
    // Point queries should recommend Uniform Grid
    EXPECT_EQ(recommended, SpatialIndexType::UNIFORM_GRID);
}

TEST_F(SpatialQueryOptimizerTest, RecommendIndexTypeForNearestNeighbor) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    // Perform many nearest neighbor queries
    for (int i = 0; i < 100; ++i) {
        Eigen::Vector3d point(i % 5, (i/5) % 5, (i/25) % 5);
        optimizer.findNearest(point);
    }

    auto recommended = optimizer.recommendIndexType();
    // Nearest neighbor queries should recommend K-d Tree
    EXPECT_EQ(recommended, SpatialIndexType::KDTREE);
}

TEST_F(SpatialQueryOptimizerTest, RecommendIndexTypeForRangeQueries) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    // Perform many range queries
    for (int i = 0; i < 100; ++i) {
        BoundingBox box(Eigen::Vector3d(i % 3, (i/3) % 3, 0),
                       Eigen::Vector3d((i % 3) + 2, ((i/3) % 3) + 2, 2));
        optimizer.queryBoundingBox(box);
    }

    auto recommended = optimizer.recommendIndexType();
    // Range queries should recommend R-Tree
    EXPECT_EQ(recommended, SpatialIndexType::RTREE);
}

// ============================================================================
// Adaptation Tests
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, AdaptIndexType) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh, SpatialIndexType::OCTREE);

    EXPECT_EQ(optimizer.getCurrentIndexType(), SpatialIndexType::OCTREE);

    // Perform many point queries to trigger adaptation
    for (int i = 0; i < 150; ++i) {
        Eigen::Vector3d point(i % 5, (i/5) % 5, (i/25) % 5);
        optimizer.queryPoint(point);
    }

    // Try to adapt
    optimizer.adaptIndexType();

    // Index type should have changed based on query pattern
    auto newType = optimizer.getCurrentIndexType();
    EXPECT_NE(newType, SpatialIndexType::OCTREE);
}

// ============================================================================
// Report Tests
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, OptimizationReport) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    // Perform some queries
    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    optimizer.queryBoundingBox(box);

    Eigen::Vector3d point(2.5, 2.5, 2.5);
    optimizer.findNearest(point);

    std::string report = optimizer.getOptimizationReport();

    EXPECT_NE(report.find("Spatial Query Optimizer Report"), std::string::npos);
    EXPECT_NE(report.find("Query Pattern Statistics"), std::string::npos);
    EXPECT_NE(report.find("Recommended Index Type"), std::string::npos);
}

TEST_F(SpatialQueryOptimizerTest, CacheStatistics) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);
    optimizer.enableCache(100, 30.0);

    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    optimizer.queryBoundingBox(box);
    optimizer.queryBoundingBox(box);  // Cache hit

    std::string stats = optimizer.getCacheStatistics();

    EXPECT_NE(stats.find("Cache Statistics"), std::string::npos);
    EXPECT_NE(stats.find("Hit Rate"), std::string::npos);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(SpatialQueryOptimizerTest, QueriesWithoutMesh) {
    SpatialQueryOptimizer optimizer;

    // Queries without mesh should return empty/zero
    BoundingBox box(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
    auto results = optimizer.queryBoundingBox(box);
    EXPECT_EQ(results.size(), 0);

    ElementId nearest = optimizer.findNearest(Eigen::Vector3d(1, 1, 1));
    EXPECT_EQ(nearest, 0);
}

TEST_F(SpatialQueryOptimizerTest, RecommendationWithNoQueries) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    auto recommended = optimizer.recommendIndexType();
    // Should default to Octree with no query history
    EXPECT_EQ(recommended, SpatialIndexType::OCTREE);
}

TEST_F(SpatialQueryOptimizerTest, MixedQueryWorkload) {
    SpatialQueryOptimizer optimizer;
    optimizer.setMesh(*mesh);

    // Mix of different query types
    for (int i = 0; i < 20; ++i) {
        BoundingBox box(Eigen::Vector3d(i % 3, 0, 0), Eigen::Vector3d((i % 3) + 1, 1, 1));
        optimizer.queryBoundingBox(box);

        Eigen::Vector3d point(i % 5, i % 5, i % 5);
        optimizer.queryPoint(point);
        optimizer.findNearest(point);
        optimizer.findKNearest(point, 3);
        optimizer.findWithinRadius(point, 1.5);
    }

    // Should handle mixed workload
    std::string report = optimizer.getOptimizationReport();
    EXPECT_NE(report.find("Total Queries: 100"), std::string::npos);
}
