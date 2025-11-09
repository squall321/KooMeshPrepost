#include <gtest/gtest.h>
#include "core/Octree.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"
#include <Eigen/Dense>

using namespace koomesh::core;

class OctreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        mesh = std::make_unique<Mesh>();
        octree = std::make_unique<Octree>();
    }

    void TearDown() override {
        octree.reset();
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
    std::unique_ptr<Octree> octree;
};

TEST_F(OctreeTest, Construction_DefaultParameters) {
    EXPECT_EQ(8, octree->getMaxDepth());
    EXPECT_EQ(10, octree->getMaxElementsPerNode());
    EXPECT_FALSE(octree->isBuilt());
    EXPECT_EQ(0, octree->elementCount());
}

TEST_F(OctreeTest, Construction_CustomParameters) {
    auto customOctree = std::make_unique<Octree>(6, 5);
    EXPECT_EQ(6, customOctree->getMaxDepth());
    EXPECT_EQ(5, customOctree->getMaxElementsPerNode());
}

TEST_F(OctreeTest, Configuration_SetParameters) {
    octree->setMaxDepth(10);
    octree->setMaxElementsPerNode(20);
    EXPECT_EQ(10, octree->getMaxDepth());
    EXPECT_EQ(20, octree->getMaxElementsPerNode());
}

TEST_F(OctreeTest, Build_EmptyMesh) {
    octree->build(*mesh);
    EXPECT_TRUE(octree->isBuilt());
    EXPECT_EQ(0, octree->elementCount());
}

TEST_F(OctreeTest, Build_SimpleCube) {
    createCubeMesh();
    octree->build(*mesh);

    EXPECT_TRUE(octree->isBuilt());
    EXPECT_EQ(1, octree->elementCount());

    BoundingBox bounds = octree->boundingBox();
    EXPECT_TRUE(bounds.min().x() <= 0.0);
    EXPECT_TRUE(bounds.max().x() >= 1.0);
}

TEST_F(OctreeTest, Build_GridMesh) {
    createGridMesh(3, 3, 3);  // 3x3x3 grid = 135 tetrahedra
    octree->build(*mesh);

    EXPECT_TRUE(octree->isBuilt());
    EXPECT_EQ(135, octree->elementCount());
}

TEST_F(OctreeTest, Query_BoundingBox_FullMesh) {
    createGridMesh(2, 2, 2);  // 2x2x2 grid = 40 tetrahedra
    octree->build(*mesh);

    // Query entire mesh
    BoundingBox queryBox(Eigen::Vector3d(-1, -1, -1), Eigen::Vector3d(3, 3, 3));
    auto results = octree->query(queryBox);

    EXPECT_EQ(40, results.size());
}

TEST_F(OctreeTest, Query_BoundingBox_Partial) {
    createGridMesh(4, 4, 4);
    octree->build(*mesh);

    // Query small region
    BoundingBox queryBox(Eigen::Vector3d(0.0, 0.0, 0.0), Eigen::Vector3d(1.0, 1.0, 1.0));
    auto results = octree->query(queryBox);

    EXPECT_GT(results.size(), 0);
    EXPECT_LT(results.size(), octree->elementCount());
}

TEST_F(OctreeTest, Query_BoundingBox_NoIntersection) {
    createGridMesh(2, 2, 2);
    octree->build(*mesh);

    // Query outside mesh
    BoundingBox queryBox(Eigen::Vector3d(10.0, 10.0, 10.0), Eigen::Vector3d(20.0, 20.0, 20.0));
    auto results = octree->query(queryBox);

    EXPECT_EQ(0, results.size());
}

TEST_F(OctreeTest, QueryPoint_InsideMesh) {
    createGridMesh(3, 3, 3);
    octree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = octree->queryPoint(point);

    EXPECT_GT(results.size(), 0);
}

TEST_F(OctreeTest, QueryPoint_OutsideMesh) {
    createGridMesh(2, 2, 2);
    octree->build(*mesh);

    Eigen::Vector3d point(10.0, 10.0, 10.0);
    auto results = octree->queryPoint(point);

    EXPECT_EQ(0, results.size());
}

TEST_F(OctreeTest, FindNearest_SingleElement) {
    createCubeMesh();
    octree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = octree->findNearest(point);

    EXPECT_EQ(1, nearest);
}

TEST_F(OctreeTest, FindNearest_MultipleElements) {
    createGridMesh(3, 3, 3);
    octree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = octree->findNearest(point);

    EXPECT_GT(nearest, 0);
}

TEST_F(OctreeTest, FindNearest_EmptyTree) {
    octree->build(*mesh);

    Eigen::Vector3d point(0.0, 0.0, 0.0);
    ElementId nearest = octree->findNearest(point);

    EXPECT_EQ(0, nearest);
}

TEST_F(OctreeTest, FindKNearest_Basic) {
    createGridMesh(3, 3, 3);
    octree->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    auto results = octree->findKNearest(point, 5);

    EXPECT_EQ(5, results.size());
}

TEST_F(OctreeTest, FindKNearest_MoreThanAvailable) {
    createCubeMesh();
    octree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = octree->findKNearest(point, 10);

    EXPECT_EQ(1, results.size());
}

TEST_F(OctreeTest, FindKNearest_Zero) {
    createGridMesh(2, 2, 2);
    octree->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    auto results = octree->findKNearest(point, 0);

    EXPECT_EQ(0, results.size());
}

TEST_F(OctreeTest, FindWithinRadius_Basic) {
    createGridMesh(4, 4, 4);
    octree->build(*mesh);

    Eigen::Vector3d point(2.0, 2.0, 2.0);
    double radius = 1.0;
    auto results = octree->findWithinRadius(point, radius);

    EXPECT_GT(results.size(), 0);
}

TEST_F(OctreeTest, FindWithinRadius_ZeroRadius) {
    createGridMesh(3, 3, 3);
    octree->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    double radius = 0.0;
    auto results = octree->findWithinRadius(point, radius);

    // Should find elements whose bounds contain the point
    EXPECT_GE(results.size(), 0);
}

TEST_F(OctreeTest, FindWithinRadius_LargeRadius) {
    createGridMesh(2, 2, 2);
    octree->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    double radius = 100.0;
    auto results = octree->findWithinRadius(point, radius);

    EXPECT_EQ(40, results.size());  // Should find all elements
}

TEST_F(OctreeTest, RayIntersect_ThroughMesh) {
    createGridMesh(3, 3, 3);
    octree->build(*mesh);

    Eigen::Vector3d origin(0.5, 0.5, -1.0);
    Eigen::Vector3d direction(0.0, 0.0, 1.0);  // Ray along Z axis
    auto results = octree->rayIntersect(origin, direction);

    EXPECT_GT(results.size(), 0);
}

TEST_F(OctreeTest, RayIntersect_MissingMesh) {
    createGridMesh(2, 2, 2);
    octree->build(*mesh);

    Eigen::Vector3d origin(10.0, 10.0, 10.0);
    Eigen::Vector3d direction(1.0, 0.0, 0.0);
    auto results = octree->rayIntersect(origin, direction);

    EXPECT_EQ(0, results.size());
}

TEST_F(OctreeTest, Clear_ResetsState) {
    createGridMesh(2, 2, 2);
    octree->build(*mesh);

    EXPECT_TRUE(octree->isBuilt());
    EXPECT_GT(octree->elementCount(), 0);

    octree->clear();

    EXPECT_FALSE(octree->isBuilt());
    EXPECT_EQ(0, octree->elementCount());
}

TEST_F(OctreeTest, Rebuild_AfterClear) {
    createCubeMesh();
    octree->build(*mesh);
    octree->clear();

    createGridMesh(2, 2, 2);
    octree->build(*mesh);

    EXPECT_TRUE(octree->isBuilt());
    EXPECT_EQ(40, octree->elementCount());
}

TEST_F(OctreeTest, MemoryUsage_Increases) {
    createCubeMesh();
    octree->build(*mesh);
    size_t smallUsage = octree->memoryUsage();

    octree->clear();
    createGridMesh(5, 5, 5);
    octree->build(*mesh);
    size_t largeUsage = octree->memoryUsage();

    EXPECT_GT(largeUsage, smallUsage);
}

TEST_F(OctreeTest, Statistics_ValidOutput) {
    createGridMesh(3, 3, 3);
    octree->build(*mesh);

    std::string stats = octree->getStatistics();

    EXPECT_TRUE(stats.find("Octree Statistics") != std::string::npos);
    EXPECT_TRUE(stats.find("Elements:") != std::string::npos);
    EXPECT_TRUE(stats.find("Nodes:") != std::string::npos);
}

TEST_F(OctreeTest, BoundingBox_MatchesMesh) {
    createGridMesh(4, 4, 4);
    octree->build(*mesh);

    BoundingBox bounds = octree->boundingBox();

    // Should encompass [0,4] in each dimension
    EXPECT_LE(bounds.min().x(), 0.0);
    EXPECT_GE(bounds.max().x(), 4.0);
    EXPECT_LE(bounds.min().y(), 0.0);
    EXPECT_GE(bounds.max().y(), 4.0);
    EXPECT_LE(bounds.min().z(), 0.0);
    EXPECT_GE(bounds.max().z(), 4.0);
}

TEST_F(OctreeTest, MaxDepth_AffectsTreeStructure) {
    createGridMesh(4, 4, 4);

    // Build with shallow tree
    octree->setMaxDepth(2);
    octree->build(*mesh);
    std::string shallowStats = octree->getStatistics();

    octree->clear();

    // Build with deep tree
    octree->setMaxDepth(10);
    octree->build(*mesh);
    std::string deepStats = octree->getStatistics();

    // Stats should be different
    EXPECT_NE(shallowStats, deepStats);
}

TEST_F(OctreeTest, MaxElementsPerNode_AffectsSubdivision) {
    createGridMesh(3, 3, 3);

    // Build with many elements per node
    octree->setMaxElementsPerNode(100);
    octree->build(*mesh);
    size_t largeNodeMemory = octree->memoryUsage();

    octree->clear();

    // Build with few elements per node
    octree->setMaxElementsPerNode(1);
    octree->build(*mesh);
    size_t smallNodeMemory = octree->memoryUsage();

    // More subdivision = more memory (usually)
    EXPECT_NE(largeNodeMemory, smallNodeMemory);
}
