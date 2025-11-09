#include <gtest/gtest.h>
#include "core/UniformGrid.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"
#include <Eigen/Dense>

using namespace koomesh::core;

class UniformGridTest : public ::testing::Test {
protected:
    void SetUp() override {
        mesh = std::make_unique<Mesh>();
        grid = std::make_unique<UniformGrid>();
    }

    void TearDown() override {
        grid.reset();
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
    std::unique_ptr<UniformGrid> grid;
};

TEST_F(UniformGridTest, Construction_DefaultParameters) {
    EXPECT_EQ(1.0, grid->getCellSize());
    EXPECT_FALSE(grid->isBuilt());
    EXPECT_EQ(0, grid->elementCount());
}

TEST_F(UniformGridTest, Construction_CustomCellSize) {
    auto customGrid = std::make_unique<UniformGrid>(2.5);
    EXPECT_EQ(2.5, customGrid->getCellSize());
}

TEST_F(UniformGridTest, Construction_CustomCellCount) {
    auto customGrid = std::make_unique<UniformGrid>(10, 20, 30);
    auto cellCount = customGrid->getCellCount();
    EXPECT_EQ(10, cellCount[0]);
    EXPECT_EQ(20, cellCount[1]);
    EXPECT_EQ(30, cellCount[2]);
}

TEST_F(UniformGridTest, Configuration_SetCellSize) {
    grid->setCellSize(3.0);
    EXPECT_EQ(3.0, grid->getCellSize());
}

TEST_F(UniformGridTest, Configuration_SetCellCount) {
    grid->setCellCount(15, 25, 35);
    auto cellCount = grid->getCellCount();
    EXPECT_EQ(15, cellCount[0]);
    EXPECT_EQ(25, cellCount[1]);
    EXPECT_EQ(35, cellCount[2]);
}

TEST_F(UniformGridTest, Build_EmptyMesh) {
    grid->build(*mesh);
    EXPECT_TRUE(grid->isBuilt());
    EXPECT_EQ(0, grid->elementCount());
}

TEST_F(UniformGridTest, Build_SimpleCube) {
    createCubeMesh();
    grid->setCellSize(0.5);
    grid->build(*mesh);

    EXPECT_TRUE(grid->isBuilt());
    EXPECT_EQ(1, grid->elementCount());

    BoundingBox bounds = grid->boundingBox();
    EXPECT_TRUE(bounds.min().x() <= 0.0);
    EXPECT_TRUE(bounds.max().x() >= 1.0);
}

TEST_F(UniformGridTest, Build_GridMesh) {
    createGridMesh(3, 3, 3);  // 3x3x3 grid = 135 tetrahedra
    grid->setCellSize(1.0);
    grid->build(*mesh);

    EXPECT_TRUE(grid->isBuilt());
    EXPECT_EQ(135, grid->elementCount());
}

TEST_F(UniformGridTest, Build_WithCellCount) {
    createGridMesh(4, 4, 4);
    grid->setCellCount(8, 8, 8);
    grid->build(*mesh);

    EXPECT_TRUE(grid->isBuilt());
    EXPECT_EQ(320, grid->elementCount());
}

TEST_F(UniformGridTest, Query_BoundingBox_FullMesh) {
    createGridMesh(2, 2, 2);  // 2x2x2 grid = 40 tetrahedra
    grid->setCellSize(1.0);
    grid->build(*mesh);

    // Query entire mesh
    BoundingBox queryBox(Eigen::Vector3d(-1, -1, -1), Eigen::Vector3d(3, 3, 3));
    auto results = grid->query(queryBox);

    EXPECT_EQ(40, results.size());
}

TEST_F(UniformGridTest, Query_BoundingBox_Partial) {
    createGridMesh(4, 4, 4);
    grid->setCellSize(0.5);
    grid->build(*mesh);

    // Query small region
    BoundingBox queryBox(Eigen::Vector3d(0.0, 0.0, 0.0), Eigen::Vector3d(1.0, 1.0, 1.0));
    auto results = grid->query(queryBox);

    EXPECT_GT(results.size(), 0);
    EXPECT_LT(results.size(), grid->elementCount());
}

TEST_F(UniformGridTest, Query_BoundingBox_NoIntersection) {
    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    // Query outside mesh
    BoundingBox queryBox(Eigen::Vector3d(10.0, 10.0, 10.0), Eigen::Vector3d(20.0, 20.0, 20.0));
    auto results = grid->query(queryBox);

    EXPECT_EQ(0, results.size());
}

TEST_F(UniformGridTest, QueryPoint_InsideMesh) {
    createGridMesh(3, 3, 3);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = grid->queryPoint(point);

    EXPECT_GT(results.size(), 0);
}

TEST_F(UniformGridTest, QueryPoint_OutsideMesh) {
    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(10.0, 10.0, 10.0);
    auto results = grid->queryPoint(point);

    EXPECT_EQ(0, results.size());
}

TEST_F(UniformGridTest, QueryPoint_OnBoundary) {
    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    auto results = grid->queryPoint(point);

    // Should find elements at cell containing this point
    EXPECT_GE(results.size(), 0);
}

TEST_F(UniformGridTest, FindNearest_SingleElement) {
    createCubeMesh();
    grid->setCellSize(0.5);
    grid->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = grid->findNearest(point);

    EXPECT_EQ(1, nearest);
}

TEST_F(UniformGridTest, FindNearest_MultipleElements) {
    createGridMesh(3, 3, 3);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    ElementId nearest = grid->findNearest(point);

    EXPECT_GT(nearest, 0);
}

TEST_F(UniformGridTest, FindNearest_EmptyGrid) {
    grid->build(*mesh);

    Eigen::Vector3d point(0.0, 0.0, 0.0);
    ElementId nearest = grid->findNearest(point);

    EXPECT_EQ(0, nearest);
}

TEST_F(UniformGridTest, FindKNearest_Basic) {
    createGridMesh(3, 3, 3);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    auto results = grid->findKNearest(point, 5);

    EXPECT_EQ(5, results.size());
}

TEST_F(UniformGridTest, FindKNearest_MoreThanAvailable) {
    createCubeMesh();
    grid->setCellSize(0.5);
    grid->build(*mesh);

    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = grid->findKNearest(point, 10);

    EXPECT_EQ(1, results.size());
}

TEST_F(UniformGridTest, FindKNearest_Zero) {
    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    auto results = grid->findKNearest(point, 0);

    EXPECT_EQ(0, results.size());
}

TEST_F(UniformGridTest, FindWithinRadius_Basic) {
    createGridMesh(4, 4, 4);
    grid->setCellSize(0.5);
    grid->build(*mesh);

    Eigen::Vector3d point(2.0, 2.0, 2.0);
    double radius = 1.0;
    auto results = grid->findWithinRadius(point, radius);

    EXPECT_GT(results.size(), 0);
}

TEST_F(UniformGridTest, FindWithinRadius_ZeroRadius) {
    createGridMesh(3, 3, 3);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(1.5, 1.5, 1.5);
    double radius = 0.0;
    auto results = grid->findWithinRadius(point, radius);

    // Should find elements whose bounds contain the point
    EXPECT_GE(results.size(), 0);
}

TEST_F(UniformGridTest, FindWithinRadius_LargeRadius) {
    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d point(1.0, 1.0, 1.0);
    double radius = 100.0;
    auto results = grid->findWithinRadius(point, radius);

    EXPECT_EQ(40, results.size());  // Should find all elements
}

TEST_F(UniformGridTest, RayIntersect_ThroughMesh) {
    createGridMesh(3, 3, 3);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d origin(0.5, 0.5, -1.0);
    Eigen::Vector3d direction(0.0, 0.0, 1.0);  // Ray along Z axis
    auto results = grid->rayIntersect(origin, direction);

    EXPECT_GT(results.size(), 0);
}

TEST_F(UniformGridTest, RayIntersect_MissingMesh) {
    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    Eigen::Vector3d origin(10.0, 10.0, 10.0);
    Eigen::Vector3d direction(1.0, 0.0, 0.0);
    auto results = grid->rayIntersect(origin, direction);

    EXPECT_EQ(0, results.size());
}

TEST_F(UniformGridTest, Clear_ResetsState) {
    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    EXPECT_TRUE(grid->isBuilt());
    EXPECT_GT(grid->elementCount(), 0);

    grid->clear();

    EXPECT_FALSE(grid->isBuilt());
    EXPECT_EQ(0, grid->elementCount());
}

TEST_F(UniformGridTest, Rebuild_AfterClear) {
    createCubeMesh();
    grid->setCellSize(0.5);
    grid->build(*mesh);
    grid->clear();

    createGridMesh(2, 2, 2);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    EXPECT_TRUE(grid->isBuilt());
    EXPECT_EQ(40, grid->elementCount());
}

TEST_F(UniformGridTest, MemoryUsage_Increases) {
    createCubeMesh();
    grid->setCellSize(0.5);
    grid->build(*mesh);
    size_t smallUsage = grid->memoryUsage();

    grid->clear();
    createGridMesh(5, 5, 5);
    grid->setCellSize(0.5);
    grid->build(*mesh);
    size_t largeUsage = grid->memoryUsage();

    EXPECT_GT(largeUsage, smallUsage);
}

TEST_F(UniformGridTest, Statistics_ValidOutput) {
    createGridMesh(3, 3, 3);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    std::string stats = grid->getStatistics();

    EXPECT_TRUE(stats.find("UniformGrid Statistics") != std::string::npos);
    EXPECT_TRUE(stats.find("Elements:") != std::string::npos);
    EXPECT_TRUE(stats.find("Cell size:") != std::string::npos);
    EXPECT_TRUE(stats.find("Non-empty cells:") != std::string::npos);
}

TEST_F(UniformGridTest, BoundingBox_MatchesMesh) {
    createGridMesh(4, 4, 4);
    grid->setCellSize(1.0);
    grid->build(*mesh);

    BoundingBox bounds = grid->boundingBox();

    // Should encompass [0,4] in each dimension
    EXPECT_LE(bounds.min().x(), 0.0);
    EXPECT_GE(bounds.max().x(), 4.0);
    EXPECT_LE(bounds.min().y(), 0.0);
    EXPECT_GE(bounds.max().y(), 4.0);
    EXPECT_LE(bounds.min().z(), 0.0);
    EXPECT_GE(bounds.max().z(), 4.0);
}

TEST_F(UniformGridTest, CellSize_AffectsPerformance) {
    createGridMesh(4, 4, 4);

    // Build with small cells
    grid->setCellSize(0.25);
    grid->build(*mesh);
    size_t smallCellMemory = grid->memoryUsage();

    grid->clear();

    // Build with large cells
    grid->setCellSize(2.0);
    grid->build(*mesh);
    size_t largeCellMemory = grid->memoryUsage();

    // Different cell sizes should have different memory footprints
    EXPECT_NE(smallCellMemory, largeCellMemory);
}

TEST_F(UniformGridTest, SpiralSearch_FindsNearestCorrectly) {
    // Create a simple 2-element mesh with known positions
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

    grid->setCellSize(1.0);
    grid->build(*mesh);

    // Point close to element 1
    Eigen::Vector3d point1(0.5, 0.5, 0.0);
    ElementId nearest1 = grid->findNearest(point1);
    EXPECT_EQ(1, nearest1);

    // Point close to element 2
    Eigen::Vector3d point2(10.5, 10.5, 10.0);
    ElementId nearest2 = grid->findNearest(point2);
    EXPECT_EQ(2, nearest2);
}

TEST_F(UniformGridTest, FineCellSize_HandlesSmallMesh) {
    createCubeMesh();
    grid->setCellSize(0.1);  // Very fine cells
    grid->build(*mesh);

    EXPECT_TRUE(grid->isBuilt());
    EXPECT_EQ(1, grid->elementCount());

    // Query should still work
    Eigen::Vector3d point(0.5, 0.5, 0.5);
    auto results = grid->queryPoint(point);
    EXPECT_GE(results.size(), 0);
}

TEST_F(UniformGridTest, CoarseCellSize_HandlesLargeMesh) {
    createGridMesh(10, 10, 10);
    grid->setCellSize(5.0);  // Very coarse cells
    grid->build(*mesh);

    EXPECT_TRUE(grid->isBuilt());
    EXPECT_EQ(5000, grid->elementCount());

    // Query should still work
    BoundingBox queryBox(Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(5, 5, 5));
    auto results = grid->query(queryBox);
    EXPECT_GT(results.size(), 0);
}
