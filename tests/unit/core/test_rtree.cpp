#include <gtest/gtest.h>
#include "core/RTree.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"
#include <Eigen/Dense>

using namespace koomesh::core;

class RTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        mesh = std::make_unique<Mesh>();
        rtree = std::make_unique<RTree>();
    }

    void TearDown() override {
        rtree.reset();
        mesh.reset();
    }

    // Helper: Create a simple cube mesh
    void createCubeMesh() {
        // 8 nodes forming a unit cube
        mesh->addNode(Node(1, 0.0, 0.0, 0.0));
        mesh->addNode(Node(2, 1.0, 0.0, 0.0));
        mesh->addNode(Node(3, 1.0, 1.0, 0.0));
        mesh->addNode(Node(4, 0.0, 1.0, 0.0));
        mesh->addNode(Node(5, 0.0, 0.0, 1.0));
        mesh->addNode(Node(6, 1.0, 0.0, 1.0));
        mesh->addNode(Node(7, 1.0, 1.0, 1.0));
        mesh->addNode(Node(8, 0.0, 1.0, 1.0));

        // 1 hex element
        std::vector<NodeId> nodeIds = {1, 2, 3, 4, 5, 6, 7, 8};
        auto elem = std::make_unique<HexahedronElement>(1, 1, nodeIds);
        mesh->addElement(std::move(elem));
    }

    // Helper: Create a grid of tetrahedra
    void createGridMesh(int nx, int ny, int nz) {
        NodeId nodeId = 1;
        ElementId elemId = 1;

        // Create nodes
        for (int k = 0; k <= nz; ++k) {
            for (int j = 0; j <= ny; ++j) {
                for (int i = 0; i <= nx; ++i) {
                    mesh->addNode(Node(nodeId++, i * 1.0, j * 1.0, k * 1.0));
                }
            }
        }

        // Create tetrahedral elements
        int nxNodes = nx + 1;
        int nyNodes = ny + 1;

        for (int k = 0; k < nz; ++k) {
            for (int j = 0; j < ny; ++j) {
                for (int i = 0; i < nx; ++i) {
                    // Calculate node IDs for this cell
                    NodeId n000 = 1 + i + j * nxNodes + k * nxNodes * nyNodes;
                    NodeId n100 = n000 + 1;
                    NodeId n010 = n000 + nxNodes;
                    NodeId n110 = n010 + 1;
                    NodeId n001 = n000 + nxNodes * nyNodes;
                    NodeId n101 = n001 + 1;
                    NodeId n011 = n001 + nxNodes;
                    NodeId n111 = n011 + 1;

                    // Create 5 tetrahedra per cell
                    std::vector<NodeId> tet1 = {n000, n100, n110, n001};
                    std::vector<NodeId> tet2 = {n100, n110, n111, n001};
                    std::vector<NodeId> tet3 = {n110, n010, n011, n001};
                    std::vector<NodeId> tet4 = {n110, n111, n011, n001};
                    std::vector<NodeId> tet5 = {n100, n101, n111, n001};

                    mesh->addElement(std::make_unique<TetrahedronElement>(elemId++, 1, tet1));
                    mesh->addElement(std::make_unique<TetrahedronElement>(elemId++, 1, tet2));
                    mesh->addElement(std::make_unique<TetrahedronElement>(elemId++, 1, tet3));
                    mesh->addElement(std::make_unique<TetrahedronElement>(elemId++, 1, tet4));
                    mesh->addElement(std::make_unique<TetrahedronElement>(elemId++, 1, tet5));
                }
            }
        }
    }

    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<RTree> rtree;
};

TEST_F(RTreeTest, Construction_DefaultParameters) {
    EXPECT_EQ(16, rtree->getMaxChildren());
    EXPECT_EQ(8, rtree->getMinChildren());
    EXPECT_FALSE(rtree->isBuilt());
    EXPECT_EQ(0, rtree->elementCount());
}

TEST_F(RTreeTest, Construction_CustomParameters) {
    auto customRTree = std::make_unique<RTree>(32);
    EXPECT_EQ(32, customRTree->getMaxChildren());
    EXPECT_EQ(16, customRTree->getMinChildren());
}

TEST_F(RTreeTest, Configuration_SetMaxChildren) {
    rtree->setMaxChildren(24);
    EXPECT_EQ(24, rtree->getMaxChildren());
    EXPECT_EQ(12, rtree->getMinChildren());
}

TEST_F(RTreeTest, Build_EmptyMesh) {
    rtree->build(*mesh);
    EXPECT_TRUE(rtree->isBuilt());
    EXPECT_EQ(0, rtree->elementCount());
}

TEST_F(RTreeTest, Build_SimpleCube) {
    createCubeMesh();
    rtree->build(*mesh);

    EXPECT_TRUE(rtree->isBuilt());
    EXPECT_EQ(1, rtree->elementCount());

    BoundingBox bounds = rtree->boundingBox();
    EXPECT_TRUE(bounds.min().x() <= 0.0);
    EXPECT_TRUE(bounds.max().x() >= 1.0);
}

TEST_F(RTreeTest, Build_GridMesh) {
    createGridMesh(3, 3, 3);  // 3x3x3 grid = 135 tetrahedra
    rtree->build(*mesh);

    EXPECT_TRUE(rtree->isBuilt());
    EXPECT_EQ(135, rtree->elementCount());
}

TEST_F(RTreeTest, Query_BoundingBox_FullMesh) {
    createGridMesh(2, 2, 2);  // 2x2x2 grid = 40 tetrahedra
    rtree->build(*mesh);

    // Query entire mesh
    BoundingBox queryBox(Eigen::Vector3d(-1, -1, -1), Eigen::Vector3d(3, 3, 3));
    auto results = rtree->query(queryBox);

    EXPECT_EQ(40, results.size());
}

TEST_F(RTreeTest, Query_BoundingBox_Partial) {
    createGridMesh(4, 4, 4);
    rtree->build(*mesh);

    // Query small region
    BoundingBox queryBox(Eigen::Vector3d(0.0, 0.0, 0.0), Eigen::Vector3d(1.0, 1.0, 1.0));
    auto results = rtree->query(queryBox);

    EXPECT_GT(results.size(), 0);
    EXPECT_LT(results.size(), rtree->elementCount());
}

TEST_F(RTreeTest, Query_BoundingBox_NoIntersection) {
    createGridMesh(2, 2, 2);
    rtree->build(*mesh);

    // Query outside mesh
    BoundingBox queryBox(Eigen::Vector3d(10.0, 10.0, 10.0), Eigen::Vector3d(20.0, 20.0, 20.0));
    auto results = rtree->query(queryBox);

    EXPECT_EQ(0, results.size());
}

TEST_F(RTreeTest, QueryPoint_InsideMesh) {
    createGridMesh(3, 3, 3);
    rtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = rtree->queryPoint(point);

    EXPECT_GT(results.size(), 0);
}

TEST_F(RTreeTest, QueryPoint_OutsideMesh) {
    createGridMesh(2, 2, 2);
    rtree->build(*mesh);

    Eigen::Vector3d point(10.0, 10.0, 10.0);
    auto results = rtree->queryPoint(point);

    EXPECT_EQ(0, results.size());
}

TEST_F(RTreeTest, FindNearest_SingleElement) {
    createCubeMesh();
    rtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = rtree->findNearest(point);

    EXPECT_EQ(1, nearest);
}

TEST_F(RTreeTest, FindNearest_MultipleElements) {
    createGridMesh(3, 3, 3);
    rtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = rtree->findNearest(point);

    EXPECT_GT(nearest, 0);
}

TEST_F(RTreeTest, FindNearest_EmptyTree) {
    rtree->build(*mesh);

    Eigen::Vector3d point(0.0, 0.0, 0.0);
    ElementId nearest = rtree->findNearest(point);

    EXPECT_EQ(0, nearest);
}

TEST_F(RTreeTest, FindKNearest_Basic) {
    createGridMesh(3, 3, 3);
    rtree->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    auto results = rtree->findKNearest(point, 5);

    EXPECT_EQ(5, results.size());
}

TEST_F(RTreeTest, FindKNearest_MoreThanAvailable) {
    createCubeMesh();
    rtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = rtree->findKNearest(point, 10);

    EXPECT_EQ(1, results.size());
}

TEST_F(RTreeTest, FindKNearest_Zero) {
    createGridMesh(2, 2, 2);
    rtree->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    auto results = rtree->findKNearest(point, 0);

    EXPECT_EQ(0, results.size());
}

TEST_F(RTreeTest, FindWithinRadius_Basic) {
    createGridMesh(4, 4, 4);
    rtree->build(*mesh);

    Eigen::Vector3d point(2.0, 2.0, 2.0);
    double radius = 1.0;
    auto results = rtree->findWithinRadius(point, radius);

    EXPECT_GT(results.size(), 0);
}

TEST_F(RTreeTest, FindWithinRadius_ZeroRadius) {
    createGridMesh(3, 3, 3);
    rtree->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    double radius = 0.0;
    auto results = rtree->findWithinRadius(point, radius);

    // Should find elements whose bounds contain the point
    EXPECT_GE(results.size(), 0);
}

TEST_F(RTreeTest, FindWithinRadius_LargeRadius) {
    createGridMesh(2, 2, 2);
    rtree->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    double radius = 100.0;
    auto results = rtree->findWithinRadius(point, radius);

    EXPECT_EQ(40, results.size());  // Should find all elements
}

TEST_F(RTreeTest, RayIntersect_ThroughMesh) {
    createGridMesh(3, 3, 3);
    rtree->build(*mesh);

    Eigen::Vector3d origin(0.5, 0.5, -1.0);
    Eigen::Vector3d direction(0.0, 0.0, 1.0);  // Ray along Z axis
    auto results = rtree->rayIntersect(origin, direction);

    EXPECT_GT(results.size(), 0);
}

TEST_F(RTreeTest, RayIntersect_MissingMesh) {
    createGridMesh(2, 2, 2);
    rtree->build(*mesh);

    Eigen::Vector3d origin(10.0, 10.0, 10.0);
    Eigen::Vector3d direction(1.0, 0.0, 0.0);
    auto results = rtree->rayIntersect(origin, direction);

    EXPECT_EQ(0, results.size());
}

TEST_F(RTreeTest, Clear_ResetsState) {
    createGridMesh(2, 2, 2);
    rtree->build(*mesh);

    EXPECT_TRUE(rtree->isBuilt());
    EXPECT_GT(rtree->elementCount(), 0);

    rtree->clear();

    EXPECT_FALSE(rtree->isBuilt());
    EXPECT_EQ(0, rtree->elementCount());
}

TEST_F(RTreeTest, Rebuild_AfterClear) {
    createCubeMesh();
    rtree->build(*mesh);
    rtree->clear();

    createGridMesh(2, 2, 2);
    rtree->build(*mesh);

    EXPECT_TRUE(rtree->isBuilt());
    EXPECT_EQ(40, rtree->elementCount());
}

TEST_F(RTreeTest, MemoryUsage_Increases) {
    createCubeMesh();
    rtree->build(*mesh);
    size_t smallUsage = rtree->memoryUsage();

    rtree->clear();
    createGridMesh(5, 5, 5);
    rtree->build(*mesh);
    size_t largeUsage = rtree->memoryUsage();

    EXPECT_GT(largeUsage, smallUsage);
}

TEST_F(RTreeTest, Statistics_ValidOutput) {
    createGridMesh(3, 3, 3);
    rtree->build(*mesh);

    std::string stats = rtree->getStatistics();

    EXPECT_TRUE(stats.find("RTree Statistics") != std::string::npos);
    EXPECT_TRUE(stats.find("Elements:") != std::string::npos);
    EXPECT_TRUE(stats.find("Nodes:") != std::string::npos);
    EXPECT_TRUE(stats.find("Max children:") != std::string::npos);
}

TEST_F(RTreeTest, BoundingBox_MatchesMesh) {
    createGridMesh(4, 4, 4);
    rtree->build(*mesh);

    BoundingBox bounds = rtree->boundingBox();

    // Should encompass [0,4] in each dimension
    EXPECT_LE(bounds.min().x(), 0.0);
    EXPECT_GE(bounds.max().x(), 4.0);
    EXPECT_LE(bounds.min().y(), 0.0);
    EXPECT_GE(bounds.max().y(), 4.0);
    EXPECT_LE(bounds.min().z(), 0.0);
    EXPECT_GE(bounds.max().z(), 4.0);
}

TEST_F(RTreeTest, MaxChildren_AffectsTreeStructure) {
    createGridMesh(4, 4, 4);

    // Build with few children
    rtree->setMaxChildren(4);
    rtree->build(*mesh);
    std::string smallStats = rtree->getStatistics();

    rtree->clear();

    // Build with many children
    rtree->setMaxChildren(32);
    rtree->build(*mesh);
    std::string largeStats = rtree->getStatistics();

    // Stats should be different
    EXPECT_NE(smallStats, largeStats);
}

TEST_F(RTreeTest, DifferentMaxChildren_DifferentPerformance) {
    createGridMesh(5, 5, 5);

    // Build with M=8
    rtree->setMaxChildren(8);
    rtree->build(*mesh);
    size_t memory8 = rtree->memoryUsage();

    rtree->clear();

    // Build with M=16
    rtree->setMaxChildren(16);
    rtree->build(*mesh);
    size_t memory16 = rtree->memoryUsage();

    // Different configurations should have different memory footprints
    EXPECT_NE(memory8, memory16);
}
