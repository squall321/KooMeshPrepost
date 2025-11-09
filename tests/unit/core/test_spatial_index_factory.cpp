#include <gtest/gtest.h>
#include "core/SpatialIndexFactory.h"
#include "core/Octree.h"
#include "core/RTree.h"
#include "core/KdTree.h"
#include "core/UniformGrid.h"
#include "core/Mesh.h"

using namespace koomesh::core;

class SpatialIndexFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test mesh
        mesh = std::make_unique<Mesh>();
    }

    void createSimpleCubeMesh(int gridSize = 2) {
        // Create a simple cubic mesh for testing
        NodeId nodeId = 1;
        for (int k = 0; k < gridSize; ++k) {
            for (int j = 0; j < gridSize; ++j) {
                for (int i = 0; i < gridSize; ++i) {
                    Node node(nodeId++, Eigen::Vector3d(i, j, k));
                    mesh->addNode(node);
                }
            }
        }

        // Add some hexahedral elements
        ElementId elemId = 1;
        int nodesPerSide = gridSize;
        for (int k = 0; k < gridSize - 1; ++k) {
            for (int j = 0; j < gridSize - 1; ++j) {
                for (int i = 0; i < gridSize - 1; ++i) {
                    NodeId n000 = 1 + i + j * nodesPerSide + k * nodesPerSide * nodesPerSide;
                    NodeId n100 = n000 + 1;
                    NodeId n010 = n000 + nodesPerSide;
                    NodeId n110 = n010 + 1;
                    NodeId n001 = n000 + nodesPerSide * nodesPerSide;
                    NodeId n101 = n001 + 1;
                    NodeId n011 = n001 + nodesPerSide;
                    NodeId n111 = n011 + 1;

                    std::vector<NodeId> nodes = {n000, n100, n110, n010, n001, n101, n111, n011};
                    HexahedronElement elem(elemId++, nodes);
                    mesh->addElement(std::make_unique<HexahedronElement>(elem));
                }
            }
        }
    }

    void createLargeMesh(int gridSize = 20) {
        createSimpleCubeMesh(gridSize);
    }

    void createElongatedMesh() {
        // Create an elongated mesh (like a beam or pipe)
        NodeId nodeId = 1;
        for (int i = 0; i < 100; ++i) {
            for (int j = 0; j < 3; ++j) {
                for (int k = 0; k < 3; ++k) {
                    Node node(nodeId++, Eigen::Vector3d(i * 10.0, j, k));
                    mesh->addNode(node);
                }
            }
        }

        // Add elements
        ElementId elemId = 1;
        for (int i = 0; i < 99; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    NodeId n000 = 1 + i * 9 + j * 3 + k;
                    NodeId n100 = n000 + 9;
                    NodeId n010 = n000 + 3;
                    NodeId n110 = n010 + 9;
                    NodeId n001 = n000 + 1;
                    NodeId n101 = n001 + 9;
                    NodeId n011 = n001 + 3;
                    NodeId n111 = n011 + 9;

                    std::vector<NodeId> nodes = {n000, n100, n110, n010, n001, n101, n111, n011};
                    HexahedronElement elem(elemId++, nodes);
                    mesh->addElement(std::make_unique<HexahedronElement>(elem));
                }
            }
        }
    }

    std::unique_ptr<Mesh> mesh;
};

// ============================================================================
// Basic Factory Tests
// ============================================================================

TEST_F(SpatialIndexFactoryTest, CreateOctreeByType) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    ASSERT_NE(index, nullptr);

    // Try to dynamic cast to verify it's actually an Octree
    auto* octree = dynamic_cast<Octree*>(index.get());
    EXPECT_NE(octree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateRTreeByType) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::RTREE);
    ASSERT_NE(index, nullptr);

    auto* rtree = dynamic_cast<RTree*>(index.get());
    EXPECT_NE(rtree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateKdTreeByType) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::KDTREE);
    ASSERT_NE(index, nullptr);

    auto* kdtree = dynamic_cast<KdTree*>(index.get());
    EXPECT_NE(kdtree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateUniformGridByType) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::UNIFORM_GRID);
    ASSERT_NE(index, nullptr);

    auto* grid = dynamic_cast<UniformGrid*>(index.get());
    EXPECT_NE(grid, nullptr);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(SpatialIndexFactoryTest, CreateWithCustomConfig) {
    SpatialIndexConfig config;
    config.type = SpatialIndexType::OCTREE;
    config.octreeMaxDepth = 12;
    config.octreeMaxElementsPerNode = 20;

    auto index = SpatialIndexFactory::create(config);
    ASSERT_NE(index, nullptr);

    auto* octree = dynamic_cast<Octree*>(index.get());
    ASSERT_NE(octree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateRTreeWithCustomConfig) {
    SpatialIndexConfig config;
    config.type = SpatialIndexType::RTREE;
    config.rtreeMaxChildren = 16;

    auto index = SpatialIndexFactory::create(config);
    ASSERT_NE(index, nullptr);

    auto* rtree = dynamic_cast<RTree*>(index.get());
    ASSERT_NE(rtree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateUniformGridWithCellSize) {
    SpatialIndexConfig config;
    config.type = SpatialIndexType::UNIFORM_GRID;
    config.uniformGridCellSize = 2.5;

    auto index = SpatialIndexFactory::create(config);
    ASSERT_NE(index, nullptr);

    auto* grid = dynamic_cast<UniformGrid*>(index.get());
    ASSERT_NE(grid, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateUniformGridWithCellCount) {
    SpatialIndexConfig config;
    config.type = SpatialIndexType::UNIFORM_GRID;
    config.uniformGridCellsX = 10;
    config.uniformGridCellsY = 10;
    config.uniformGridCellsZ = 10;

    auto index = SpatialIndexFactory::create(config);
    ASSERT_NE(index, nullptr);

    auto* grid = dynamic_cast<UniformGrid*>(index.get());
    ASSERT_NE(grid, nullptr);
}

// ============================================================================
// String-based Creation Tests
// ============================================================================

TEST_F(SpatialIndexFactoryTest, CreateFromNameOctree) {
    auto index = SpatialIndexFactory::createFromName("octree");
    ASSERT_NE(index, nullptr);

    auto* octree = dynamic_cast<Octree*>(index.get());
    EXPECT_NE(octree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameRTree) {
    auto index = SpatialIndexFactory::createFromName("rtree");
    ASSERT_NE(index, nullptr);

    auto* rtree = dynamic_cast<RTree*>(index.get());
    EXPECT_NE(rtree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameRTreeWithHyphen) {
    auto index = SpatialIndexFactory::createFromName("r-tree");
    ASSERT_NE(index, nullptr);

    auto* rtree = dynamic_cast<RTree*>(index.get());
    EXPECT_NE(rtree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameKdTree) {
    auto index = SpatialIndexFactory::createFromName("kdtree");
    ASSERT_NE(index, nullptr);

    auto* kdtree = dynamic_cast<KdTree*>(index.get());
    EXPECT_NE(kdtree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameKdTreeVariants) {
    auto index1 = SpatialIndexFactory::createFromName("kd-tree");
    EXPECT_NE(dynamic_cast<KdTree*>(index1.get()), nullptr);

    auto index2 = SpatialIndexFactory::createFromName("k-d-tree");
    EXPECT_NE(dynamic_cast<KdTree*>(index2.get()), nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameUniformGrid) {
    auto index = SpatialIndexFactory::createFromName("uniformgrid");
    ASSERT_NE(index, nullptr);

    auto* grid = dynamic_cast<UniformGrid*>(index.get());
    EXPECT_NE(grid, nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameUniformGridVariants) {
    auto index1 = SpatialIndexFactory::createFromName("uniform-grid");
    EXPECT_NE(dynamic_cast<UniformGrid*>(index1.get()), nullptr);

    auto index2 = SpatialIndexFactory::createFromName("grid");
    EXPECT_NE(dynamic_cast<UniformGrid*>(index2.get()), nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameCaseInsensitive) {
    auto index1 = SpatialIndexFactory::createFromName("OCTREE");
    EXPECT_NE(dynamic_cast<Octree*>(index1.get()), nullptr);

    auto index2 = SpatialIndexFactory::createFromName("RTree");
    EXPECT_NE(dynamic_cast<RTree*>(index2.get()), nullptr);

    auto index3 = SpatialIndexFactory::createFromName("KdTree");
    EXPECT_NE(dynamic_cast<KdTree*>(index3.get()), nullptr);
}

TEST_F(SpatialIndexFactoryTest, CreateFromNameInvalid) {
    EXPECT_THROW(SpatialIndexFactory::createFromName("invalid"), std::invalid_argument);
    EXPECT_THROW(SpatialIndexFactory::createFromName(""), std::invalid_argument);
    EXPECT_THROW(SpatialIndexFactory::createFromName("bvh"), std::invalid_argument);
}

// ============================================================================
// Type Conversion Tests
// ============================================================================

TEST_F(SpatialIndexFactoryTest, TypeToString) {
    EXPECT_EQ(SpatialIndexFactory::typeToString(SpatialIndexType::OCTREE), "octree");
    EXPECT_EQ(SpatialIndexFactory::typeToString(SpatialIndexType::RTREE), "rtree");
    EXPECT_EQ(SpatialIndexFactory::typeToString(SpatialIndexType::KDTREE), "kdtree");
    EXPECT_EQ(SpatialIndexFactory::typeToString(SpatialIndexType::UNIFORM_GRID), "uniformgrid");
}

TEST_F(SpatialIndexFactoryTest, StringToType) {
    EXPECT_EQ(SpatialIndexFactory::stringToType("octree"), SpatialIndexType::OCTREE);
    EXPECT_EQ(SpatialIndexFactory::stringToType("rtree"), SpatialIndexType::RTREE);
    EXPECT_EQ(SpatialIndexFactory::stringToType("kdtree"), SpatialIndexType::KDTREE);
    EXPECT_EQ(SpatialIndexFactory::stringToType("uniformgrid"), SpatialIndexType::UNIFORM_GRID);
}

TEST_F(SpatialIndexFactoryTest, StringToTypeRoundTrip) {
    auto types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::RTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::UNIFORM_GRID
    };

    for (auto type : types) {
        std::string name = SpatialIndexFactory::typeToString(type);
        SpatialIndexType roundTrip = SpatialIndexFactory::stringToType(name);
        EXPECT_EQ(type, roundTrip);
    }
}

// ============================================================================
// Optimal Selection Tests
// ============================================================================

TEST_F(SpatialIndexFactoryTest, OptimalSelectionSmallMesh) {
    createSimpleCubeMesh(2);  // Very small mesh
    auto index = SpatialIndexFactory::createOptimal(*mesh);
    ASSERT_NE(index, nullptr);

    // Small meshes should use Octree
    auto* octree = dynamic_cast<Octree*>(index.get());
    EXPECT_NE(octree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, OptimalSelectionLargeMesh) {
    createLargeMesh(20);  // Large mesh with many elements
    auto index = SpatialIndexFactory::createOptimal(*mesh);
    ASSERT_NE(index, nullptr);

    // Large regular meshes should use UniformGrid
    auto* grid = dynamic_cast<UniformGrid*>(index.get());
    EXPECT_NE(grid, nullptr);
}

TEST_F(SpatialIndexFactoryTest, OptimalSelectionElongatedMesh) {
    createElongatedMesh();  // Elongated mesh (high aspect ratio)
    auto index = SpatialIndexFactory::createOptimal(*mesh);
    ASSERT_NE(index, nullptr);

    // Elongated meshes should use KdTree
    auto* kdtree = dynamic_cast<KdTree*>(index.get());
    EXPECT_NE(kdtree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, OptimalSelectionEmptyMesh) {
    // Empty mesh should still create an index (Octree as default)
    auto index = SpatialIndexFactory::createOptimal(*mesh);
    ASSERT_NE(index, nullptr);
}

TEST_F(SpatialIndexFactoryTest, OptimalSelectionWithCustomConfig) {
    createSimpleCubeMesh(5);

    SpatialIndexConfig config;
    config.octreeMaxDepth = 15;  // Custom configuration

    auto index = SpatialIndexFactory::createOptimal(*mesh, config);
    ASSERT_NE(index, nullptr);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(SpatialIndexFactoryTest, CreateAndBuildOctree) {
    createSimpleCubeMesh(3);

    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    ASSERT_NO_THROW(index->build(*mesh));

    // Verify index works
    BoundingBox queryBox(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(1, 1, 1));
    auto results = index->query(queryBox);
    EXPECT_GT(results.size(), 0);
}

TEST_F(SpatialIndexFactoryTest, CreateAndBuildRTree) {
    createSimpleCubeMesh(3);

    auto index = SpatialIndexFactory::create(SpatialIndexType::RTREE);
    ASSERT_NO_THROW(index->build(*mesh));

    BoundingBox queryBox(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(1, 1, 1));
    auto results = index->query(queryBox);
    EXPECT_GT(results.size(), 0);
}

TEST_F(SpatialIndexFactoryTest, CreateAndBuildKdTree) {
    createSimpleCubeMesh(3);

    auto index = SpatialIndexFactory::create(SpatialIndexType::KDTREE);
    ASSERT_NO_THROW(index->build(*mesh));

    BoundingBox queryBox(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(1, 1, 1));
    auto results = index->query(queryBox);
    EXPECT_GT(results.size(), 0);
}

TEST_F(SpatialIndexFactoryTest, CreateAndBuildUniformGrid) {
    createSimpleCubeMesh(3);

    auto index = SpatialIndexFactory::create(SpatialIndexType::UNIFORM_GRID);
    ASSERT_NO_THROW(index->build(*mesh));

    BoundingBox queryBox(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(1, 1, 1));
    auto results = index->query(queryBox);
    EXPECT_GT(results.size(), 0);
}

TEST_F(SpatialIndexFactoryTest, OptimalIndexFunctional) {
    createSimpleCubeMesh(5);

    auto index = SpatialIndexFactory::createOptimal(*mesh);
    ASSERT_NO_THROW(index->build(*mesh));

    // Test that the index works
    Eigen::Vector3d queryPoint(0.5, 0.5, 0.5);
    auto nearest = index->findNearest(queryPoint, *mesh);
    EXPECT_NE(nearest, 0);
}

TEST_F(SpatialIndexFactoryTest, AllTypesProduceFunctionalIndexes) {
    createSimpleCubeMesh(4);

    auto types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::RTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::UNIFORM_GRID
    };

    for (auto type : types) {
        auto index = SpatialIndexFactory::create(type);
        ASSERT_NO_THROW(index->build(*mesh));

        BoundingBox queryBox(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(2, 2, 2));
        auto results = index->query(queryBox);
        EXPECT_GT(results.size(), 0) << "Type: " << SpatialIndexFactory::typeToString(type);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(SpatialIndexFactoryTest, CreateWithDefaultConfig) {
    SpatialIndexConfig config;  // All defaults
    auto index = SpatialIndexFactory::create(config);
    ASSERT_NE(index, nullptr);

    // Default type is OCTREE
    auto* octree = dynamic_cast<Octree*>(index.get());
    EXPECT_NE(octree, nullptr);
}

TEST_F(SpatialIndexFactoryTest, MultipleCreationsIndependent) {
    auto index1 = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    auto index2 = SpatialIndexFactory::create(SpatialIndexType::OCTREE);

    EXPECT_NE(index1.get(), index2.get());
    EXPECT_NE(index1, nullptr);
    EXPECT_NE(index2, nullptr);
}
