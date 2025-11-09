#include <gtest/gtest.h>
#include "core/KdTree.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"
#include <Eigen/Dense>

using namespace koomesh::core;

class KdTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        mesh = std::make_unique<Mesh>();
        kdtree = std::make_unique<KdTree>();
    }

    void TearDown() override {
        kdtree.reset();
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
    std::unique_ptr<KdTree> kdtree;
};

TEST_F(KdTreeTest, Construction_DefaultParameters) {
    EXPECT_EQ(20, kdtree->getMaxDepth());
    EXPECT_EQ(5, kdtree->getMaxElementsPerNode());
    EXPECT_FALSE(kdtree->isBuilt());
    EXPECT_EQ(0, kdtree->elementCount());
}

TEST_F(KdTreeTest, Construction_CustomParameters) {
    auto customKdTree = std::make_unique<KdTree>(15, 10);
    EXPECT_EQ(15, customKdTree->getMaxDepth());
    EXPECT_EQ(10, customKdTree->getMaxElementsPerNode());
}

TEST_F(KdTreeTest, Configuration_SetParameters) {
    kdtree->setMaxDepth(25);
    kdtree->setMaxElementsPerNode(8);
    EXPECT_EQ(25, kdtree->getMaxDepth());
    EXPECT_EQ(8, kdtree->getMaxElementsPerNode());
}

TEST_F(KdTreeTest, Build_EmptyMesh) {
    kdtree->build(*mesh);
    EXPECT_TRUE(kdtree->isBuilt());
    EXPECT_EQ(0, kdtree->elementCount());
}

TEST_F(KdTreeTest, Build_SimpleCube) {
    createCubeMesh();
    kdtree->build(*mesh);

    EXPECT_TRUE(kdtree->isBuilt());
    EXPECT_EQ(1, kdtree->elementCount());

    BoundingBox bounds = kdtree->boundingBox();
    EXPECT_TRUE(bounds.min().x() <= 0.0);
    EXPECT_TRUE(bounds.max().x() >= 1.0);
}

TEST_F(KdTreeTest, Build_GridMesh) {
    createGridMesh(3, 3, 3);  // 3x3x3 grid = 135 tetrahedra
    kdtree->build(*mesh);

    EXPECT_TRUE(kdtree->isBuilt());
    EXPECT_EQ(135, kdtree->elementCount());
}

TEST_F(KdTreeTest, Query_BoundingBox_FullMesh) {
    createGridMesh(2, 2, 2);  // 2x2x2 grid = 40 tetrahedra
    kdtree->build(*mesh);

    // Query entire mesh
    BoundingBox queryBox(Eigen::Vector3d(-1, -1, -1), Eigen::Vector3d(3, 3, 3));
    auto results = kdtree->query(queryBox);

    EXPECT_EQ(40, results.size());
}

TEST_F(KdTreeTest, Query_BoundingBox_Partial) {
    createGridMesh(4, 4, 4);
    kdtree->build(*mesh);

    // Query small region
    BoundingBox queryBox(Eigen::Vector3d(0.0, 0.0, 0.0), Eigen::Vector3d(1.0, 1.0, 1.0));
    auto results = kdtree->query(queryBox);

    EXPECT_GT(results.size(), 0);
    EXPECT_LT(results.size(), kdtree->elementCount());
}

TEST_F(KdTreeTest, Query_BoundingBox_NoIntersection) {
    createGridMesh(2, 2, 2);
    kdtree->build(*mesh);

    // Query outside mesh
    BoundingBox queryBox(Eigen::Vector3d(10.0, 10.0, 10.0), Eigen::Vector3d(20.0, 20.0, 20.0));
    auto results = kdtree->query(queryBox);

    EXPECT_EQ(0, results.size());
}

TEST_F(KdTreeTest, QueryPoint_InsideMesh) {
    createGridMesh(3, 3, 3);
    kdtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = kdtree->queryPoint(point);

    EXPECT_GT(results.size(), 0);
}

TEST_F(KdTreeTest, QueryPoint_OutsideMesh) {
    createGridMesh(2, 2, 2);
    kdtree->build(*mesh);

    Eigen::Vector3d point(10.0, 10.0, 10.0);
    auto results = kdtree->queryPoint(point);

    EXPECT_EQ(0, results.size());
}

TEST_F(KdTreeTest, FindNearest_SingleElement) {
    createCubeMesh();
    kdtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = kdtree->findNearest(point);

    EXPECT_EQ(1, nearest);
}

TEST_F(KdTreeTest, FindNearest_MultipleElements) {
    createGridMesh(3, 3, 3);
    kdtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = kdtree->findNearest(point);

    EXPECT_GT(nearest, 0);
}

TEST_F(KdTreeTest, FindNearest_EmptyTree) {
    kdtree->build(*mesh);

    Eigen::Vector3d point(0.0, 0.0, 0.0);
    ElementId nearest = kdtree->findNearest(point);

    EXPECT_EQ(0, nearest);
}

TEST_F(KdTreeTest, FindKNearest_Basic) {
    createGridMesh(3, 3, 3);
    kdtree->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    auto results = kdtree->findKNearest(point, 5);

    EXPECT_EQ(5, results.size());
}

TEST_F(KdTreeTest, FindKNearest_MoreThanAvailable) {
    createCubeMesh();
    kdtree->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = kdtree->findKNearest(point, 10);

    EXPECT_EQ(1, results.size());
}

TEST_F(KdTreeTest, FindKNearest_Zero) {
    createGridMesh(2, 2, 2);
    kdtree->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    auto results = kdtree->findKNearest(point, 0);

    EXPECT_EQ(0, results.size());
}

TEST_F(KdTreeTest, FindWithinRadius_Basic) {
    createGridMesh(4, 4, 4);
    kdtree->build(*mesh);

    Eigen::Vector3d point(2.0, 2.0, 2.0);
    double radius = 1.0;
    auto results = kdtree->findWithinRadius(point, radius);

    EXPECT_GT(results.size(), 0);
}

TEST_F(KdTreeTest, FindWithinRadius_ZeroRadius) {
    createGridMesh(3, 3, 3);
    kdtree->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    double radius = 0.0;
    auto results = kdtree->findWithinRadius(point, radius);

    // Should find elements whose bounds contain the point
    EXPECT_GE(results.size(), 0);
}

TEST_F(KdTreeTest, FindWithinRadius_LargeRadius) {
    createGridMesh(2, 2, 2);
    kdtree->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    double radius = 100.0;
    auto results = kdtree->findWithinRadius(point, radius);

    EXPECT_EQ(40, results.size());  // Should find all elements
}

TEST_F(KdTreeTest, RayIntersect_ThroughMesh) {
    createGridMesh(3, 3, 3);
    kdtree->build(*mesh);

    Eigen::Vector3d origin(0.5, 0.5, -1.0);
    Eigen::Vector3d direction(0.0, 0.0, 1.0);  // Ray along Z axis
    auto results = kdtree->rayIntersect(origin, direction);

    EXPECT_GT(results.size(), 0);
}

TEST_F(KdTreeTest, RayIntersect_MissingMesh) {
    createGridMesh(2, 2, 2);
    kdtree->build(*mesh);

    Eigen::Vector3d origin(10.0, 10.0, 10.0);
    Eigen::Vector3d direction(1.0, 0.0, 0.0);
    auto results = kdtree->rayIntersect(origin, direction);

    EXPECT_EQ(0, results.size());
}

TEST_F(KdTreeTest, Clear_ResetsState) {
    createGridMesh(2, 2, 2);
    kdtree->build(*mesh);

    EXPECT_TRUE(kdtree->isBuilt());
    EXPECT_GT(kdtree->elementCount(), 0);

    kdtree->clear();

    EXPECT_FALSE(kdtree->isBuilt());
    EXPECT_EQ(0, kdtree->elementCount());
}

TEST_F(KdTreeTest, Rebuild_AfterClear) {
    createCubeMesh();
    kdtree->build(*mesh);
    kdtree->clear();

    createGridMesh(2, 2, 2);
    kdtree->build(*mesh);

    EXPECT_TRUE(kdtree->isBuilt());
    EXPECT_EQ(40, kdtree->elementCount());
}

TEST_F(KdTreeTest, MemoryUsage_Increases) {
    createCubeMesh();
    kdtree->build(*mesh);
    size_t smallUsage = kdtree->memoryUsage();

    kdtree->clear();
    createGridMesh(5, 5, 5);
    kdtree->build(*mesh);
    size_t largeUsage = kdtree->memoryUsage();

    EXPECT_GT(largeUsage, smallUsage);
}

TEST_F(KdTreeTest, Statistics_ValidOutput) {
    createGridMesh(3, 3, 3);
    kdtree->build(*mesh);

    std::string stats = kdtree->getStatistics();

    EXPECT_TRUE(stats.find("KdTree Statistics") != std::string::npos);
    EXPECT_TRUE(stats.find("Elements:") != std::string::npos);
    EXPECT_TRUE(stats.find("Nodes:") != std::string::npos);
}

TEST_F(KdTreeTest, BoundingBox_MatchesMesh) {
    createGridMesh(4, 4, 4);
    kdtree->build(*mesh);

    BoundingBox bounds = kdtree->boundingBox();

    // Should encompass [0,4] in each dimension
    EXPECT_LE(bounds.min().x(), 0.0);
    EXPECT_GE(bounds.max().x(), 4.0);
    EXPECT_LE(bounds.min().y(), 0.0);
    EXPECT_GE(bounds.max().y(), 4.0);
    EXPECT_LE(bounds.min().z(), 0.0);
    EXPECT_GE(bounds.max().z(), 4.0);
}

TEST_F(KdTreeTest, MaxDepth_AffectsTreeStructure) {
    createGridMesh(4, 4, 4);

    // Build with shallow tree
    kdtree->setMaxDepth(3);
    kdtree->build(*mesh);
    std::string shallowStats = kdtree->getStatistics();

    kdtree->clear();

    // Build with deep tree
    kdtree->setMaxDepth(20);
    kdtree->build(*mesh);
    std::string deepStats = kdtree->getStatistics();

    // Stats should be different
    EXPECT_NE(shallowStats, deepStats);
}

TEST_F(KdTreeTest, MaxElementsPerNode_AffectsSubdivision) {
    createGridMesh(3, 3, 3);

    // Build with many elements per node
    kdtree->setMaxElementsPerNode(50);
    kdtree->build(*mesh);
    size_t largeNodeMemory = kdtree->memoryUsage();

    kdtree->clear();

    // Build with few elements per node
    kdtree->setMaxElementsPerNode(1);
    kdtree->build(*mesh);
    size_t smallNodeMemory = kdtree->memoryUsage();

    // More subdivision = more memory (usually)
    EXPECT_NE(largeNodeMemory, smallNodeMemory);
}

TEST_F(KdTreeTest, NearestNeighbor_CorrectSide) {
    // Create a simple 2-element mesh
    mesh->addNode(Node(1, 0.0, 0.0, 0.0));
    mesh->addNode(Node(2, 1.0, 0.0, 0.0));
    mesh->addNode(Node(3, 1.0, 1.0, 0.0));
    mesh->addNode(Node(4, 0.0, 1.0, 0.0));

    mesh->addNode(Node(5, 10.0, 10.0, 10.0));
    mesh->addNode(Node(6, 11.0, 10.0, 10.0));
    mesh->addNode(Node(7, 11.0, 11.0, 10.0));
    mesh->addNode(Node(8, 10.0, 11.0, 10.0));

    mesh->addElement(std::make_unique<QuadrilateralElement>(1, 1, std::vector<NodeId>{1, 2, 3, 4}));
    mesh->addElement(std::make_unique<QuadrilateralElement>(2, 1, std::vector<NodeId>{5, 6, 7, 8}));

    kdtree->build(*mesh);

    // Point close to element 1
    Eigen::Vector3d point1(0.5, 0.5, 0.0);
    ElementId nearest1 = kdtree->findNearest(point1);
    EXPECT_EQ(1, nearest1);

    // Point close to element 2
    Eigen::Vector3d point2(10.5, 10.5, 10.0);
    ElementId nearest2 = kdtree->findNearest(point2);
    EXPECT_EQ(2, nearest2);
}
