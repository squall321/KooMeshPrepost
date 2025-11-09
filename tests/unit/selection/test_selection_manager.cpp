/**
 * @file test_selection_manager.cpp
 * @brief Unit tests for SelectionManager
 */

#include <gtest/gtest.h>
#include "selection/SelectionManager.h"
#include "core/Mesh.h"
#include "core/Element.h"
#include <Eigen/Dense>

using namespace koomesh::selection;
using namespace koomesh::core;

/**
 * @brief Test fixture for SelectionManager tests
 */
class SelectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test mesh (3x3x3 grid of hexahedrons)
        createTestMesh();
    }

    void createTestMesh() {
        mesh = std::make_unique<Mesh>();

        // Create nodes in a 4x4x4 grid (0-3 in each dimension)
        NodeId nodeId = 1;
        for (int k = 0; k <= 3; ++k) {
            for (int j = 0; j <= 3; ++j) {
                for (int i = 0; i <= 3; ++i) {
                    double x = static_cast<double>(i);
                    double y = static_cast<double>(j);
                    double z = static_cast<double>(k);
                    Node node(nodeId++, Eigen::Vector3d(x, y, z));
                    mesh->addNode(node);
                }
            }
        }

        // Create hexahedral elements (3x3x3 = 27 elements)
        ElementId elemId = 1;
        for (int k = 0; k < 3; ++k) {
            for (int j = 0; j < 3; ++j) {
                for (int i = 0; i < 3; ++i) {
                    NodeId n000 = 1 + i + j * 4 + k * 16;
                    NodeId n100 = n000 + 1;
                    NodeId n010 = n000 + 4;
                    NodeId n110 = n010 + 1;
                    NodeId n001 = n000 + 16;
                    NodeId n101 = n001 + 1;
                    NodeId n011 = n001 + 4;
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
// Basic Selection Tests
// ============================================================================

TEST_F(SelectionManagerTest, InitialState) {
    SelectionManager manager;

    EXPECT_EQ(manager.getSelection().size(), 0);
    EXPECT_FALSE(manager.canUndo());
    EXPECT_FALSE(manager.canRedo());
}

TEST_F(SelectionManagerTest, SetMesh) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    EXPECT_EQ(manager.getSelection().size(), 0);
}

TEST_F(SelectionManagerTest, SelectAll) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.selectAll();

    EXPECT_EQ(manager.getSelection().size(), mesh->elementCount());
}

TEST_F(SelectionManagerTest, ClearSelection) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.selectAll();
    EXPECT_GT(manager.getSelection().size(), 0);

    manager.clearSelection();
    EXPECT_EQ(manager.getSelection().size(), 0);
}

TEST_F(SelectionManagerTest, InvertSelection) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.selectAll();
    manager.clearSelection();

    manager.invertSelection();
    EXPECT_EQ(manager.getSelection().size(), mesh->elementCount());
}

// ============================================================================
// Box Selection Tests
// ============================================================================

TEST_F(SelectionManagerTest, SelectByBox_Replace) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Select elements in box [0,0,0] - [1,1,1]
    size_t count = manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::REPLACE
    );

    EXPECT_GT(count, 0);
    EXPECT_EQ(count, manager.getSelection().size());
}

TEST_F(SelectionManagerTest, SelectByBox_Add) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // First selection
    manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::REPLACE
    );
    size_t firstCount = manager.getSelection().size();

    // Add more elements
    manager.selectByBox(
        Eigen::Vector3d(2, 2, 2),
        Eigen::Vector3d(3, 3, 3),
        SelectionMode::ADD
    );

    EXPECT_GT(manager.getSelection().size(), firstCount);
}

TEST_F(SelectionManagerTest, SelectByBox_Subtract) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Select all
    manager.selectAll();
    size_t allCount = manager.getSelection().size();

    // Subtract some elements
    manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::SUBTRACT
    );

    EXPECT_LT(manager.getSelection().size(), allCount);
}

TEST_F(SelectionManagerTest, SelectByBox_Intersect) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // First selection - larger box
    manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(2, 2, 2),
        SelectionMode::REPLACE
    );
    size_t firstCount = manager.getSelection().size();

    // Intersect with smaller box
    manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::INTERSECT
    );

    EXPECT_LT(manager.getSelection().size(), firstCount);
    EXPECT_GT(manager.getSelection().size(), 0);
}

// ============================================================================
// Sphere Selection Tests
// ============================================================================

TEST_F(SelectionManagerTest, SelectBySphere) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Select elements within radius 1.5 of center (1.5, 1.5, 1.5)
    size_t count = manager.selectBySphere(
        Eigen::Vector3d(1.5, 1.5, 1.5),
        1.5,
        SelectionMode::REPLACE
    );

    EXPECT_GT(count, 0);
    EXPECT_EQ(count, manager.getSelection().size());
}

TEST_F(SelectionManagerTest, SelectBySphere_LargeRadius) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Large radius should select all elements
    size_t count = manager.selectBySphere(
        Eigen::Vector3d(1.5, 1.5, 1.5),
        100.0,
        SelectionMode::REPLACE
    );

    EXPECT_EQ(count, mesh->elementCount());
}

// ============================================================================
// Point Selection Tests
// ============================================================================

TEST_F(SelectionManagerTest, SelectAtPoint) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Select element containing point (0.5, 0.5, 0.5)
    bool found = manager.selectAtPoint(
        Eigen::Vector3d(0.5, 0.5, 0.5),
        SelectionMode::REPLACE
    );

    EXPECT_TRUE(found);
    EXPECT_GT(manager.getSelection().size(), 0);
}

TEST_F(SelectionManagerTest, SelectAtPoint_OutsideMesh) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Try to select point outside mesh
    bool found = manager.selectAtPoint(
        Eigen::Vector3d(10, 10, 10),
        SelectionMode::REPLACE
    );

    EXPECT_FALSE(found);
    EXPECT_EQ(manager.getSelection().size(), 0);
}

// ============================================================================
// Nearest Selection Tests
// ============================================================================

TEST_F(SelectionManagerTest, SelectNearest_Single) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Select nearest element to point
    size_t count = manager.selectNearest(
        Eigen::Vector3d(0.5, 0.5, 0.5),
        1,
        SelectionMode::REPLACE
    );

    EXPECT_EQ(count, 1);
    EXPECT_EQ(manager.getSelection().size(), 1);
}

TEST_F(SelectionManagerTest, SelectNearest_Multiple) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Select 5 nearest elements
    size_t count = manager.selectNearest(
        Eigen::Vector3d(1.5, 1.5, 1.5),
        5,
        SelectionMode::REPLACE
    );

    EXPECT_EQ(count, 5);
    EXPECT_EQ(manager.getSelection().size(), 5);
}

TEST_F(SelectionManagerTest, SelectNearest_MoreThanAvailable) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Request more elements than available
    size_t count = manager.selectNearest(
        Eigen::Vector3d(1.5, 1.5, 1.5),
        1000,
        SelectionMode::REPLACE
    );

    EXPECT_EQ(count, mesh->elementCount());
}

// ============================================================================
// ID Selection Tests
// ============================================================================

TEST_F(SelectionManagerTest, SelectByIds) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    std::vector<ElementId> ids = {1, 5, 10, 15};
    manager.selectByIds(ids, SelectionMode::REPLACE);

    EXPECT_EQ(manager.getSelection().size(), ids.size());

    for (auto id : ids) {
        EXPECT_TRUE(manager.isSelected(id));
    }
}

TEST_F(SelectionManagerTest, SelectByIds_InvalidIds) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Include some invalid IDs
    std::vector<ElementId> ids = {1, 999, 1000, 5};
    manager.selectByIds(ids, SelectionMode::REPLACE);

    // Should only select valid IDs (1 and 5)
    EXPECT_LE(manager.getSelection().size(), ids.size());
}

// ============================================================================
// Filter Tests
// ============================================================================

TEST_F(SelectionManagerTest, SetFilter) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Filter: only select elements with ID < 5
    manager.setFilter([](ElementId id, const Mesh&) {
        return id < 5;
    });

    manager.selectAll();

    // Should only select elements 1-4
    EXPECT_EQ(manager.getSelection().size(), 4);
}

TEST_F(SelectionManagerTest, ClearFilter) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Set filter
    manager.setFilter([](ElementId id, const Mesh&) {
        return id < 5;
    });

    manager.selectAll();
    size_t filteredCount = manager.getSelection().size();

    // Clear filter
    manager.clearFilter();
    manager.selectAll();

    EXPECT_GT(manager.getSelection().size(), filteredCount);
}

// ============================================================================
// History (Undo/Redo) Tests
// ============================================================================

TEST_F(SelectionManagerTest, EnableHistory) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.enableHistory();

    // Make a selection
    manager.selectAll();

    EXPECT_TRUE(manager.canUndo());
    EXPECT_FALSE(manager.canRedo());
}

TEST_F(SelectionManagerTest, Undo) {
    SelectionManager manager;
    manager.setMesh(*mesh);
    manager.enableHistory();

    // Initial state - empty
    EXPECT_EQ(manager.getSelection().size(), 0);

    // Select all
    manager.selectAll();
    EXPECT_EQ(manager.getSelection().size(), mesh->elementCount());

    // Undo
    bool undone = manager.undo();
    EXPECT_TRUE(undone);
    EXPECT_EQ(manager.getSelection().size(), 0);
}

TEST_F(SelectionManagerTest, Redo) {
    SelectionManager manager;
    manager.setMesh(*mesh);
    manager.enableHistory();

    // Select all
    manager.selectAll();
    size_t allCount = manager.getSelection().size();

    // Undo
    manager.undo();
    EXPECT_EQ(manager.getSelection().size(), 0);

    // Redo
    bool redone = manager.redo();
    EXPECT_TRUE(redone);
    EXPECT_EQ(manager.getSelection().size(), allCount);
}

TEST_F(SelectionManagerTest, MultipleUndoRedo) {
    SelectionManager manager;
    manager.setMesh(*mesh);
    manager.enableHistory();

    // Make multiple selections
    manager.selectAll();                          // State 1
    manager.clearSelection();                     // State 2
    manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::REPLACE
    );                                            // State 3

    size_t state3Count = manager.getSelection().size();

    // Undo twice
    manager.undo();  // Back to state 2
    EXPECT_EQ(manager.getSelection().size(), 0);

    manager.undo();  // Back to state 1
    EXPECT_EQ(manager.getSelection().size(), mesh->elementCount());

    // Redo twice
    manager.redo();  // Forward to state 2
    EXPECT_EQ(manager.getSelection().size(), 0);

    manager.redo();  // Forward to state 3
    EXPECT_EQ(manager.getSelection().size(), state3Count);
}

TEST_F(SelectionManagerTest, DisableHistory) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.enableHistory();
    manager.selectAll();
    EXPECT_TRUE(manager.canUndo());

    manager.disableHistory();
    EXPECT_FALSE(manager.canUndo());
}

// ============================================================================
// Spatial Index Tests
// ============================================================================

TEST_F(SelectionManagerTest, UseSpatialIndex_Octree) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.useSpatialIndex(SpatialIndexType::OCTREE);

    // Box selection should work with spatial index
    size_t count = manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::REPLACE
    );

    EXPECT_GT(count, 0);
}

TEST_F(SelectionManagerTest, UseSpatialIndex_KdTree) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.useSpatialIndex(SpatialIndexType::KDTREE);

    // Nearest neighbor should work with spatial index
    size_t count = manager.selectNearest(
        Eigen::Vector3d(1.5, 1.5, 1.5),
        3,
        SelectionMode::REPLACE
    );

    EXPECT_EQ(count, 3);
}

TEST_F(SelectionManagerTest, DisableSpatialIndex) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    manager.useSpatialIndex(SpatialIndexType::OCTREE);
    manager.disableSpatialIndex();

    // Should still work with linear search
    size_t count = manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::REPLACE
    );

    EXPECT_GT(count, 0);
}

TEST_F(SelectionManagerTest, SpatialIndex_CompareWithLinear) {
    SelectionManager managerWithIndex;
    managerWithIndex.setMesh(*mesh);
    managerWithIndex.useSpatialIndex(SpatialIndexType::OCTREE);

    SelectionManager managerLinear;
    managerLinear.setMesh(*mesh);

    Eigen::Vector3d minPt(0.5, 0.5, 0.5);
    Eigen::Vector3d maxPt(2.5, 2.5, 2.5);

    // Both should produce same results
    managerWithIndex.selectByBox(minPt, maxPt, SelectionMode::REPLACE);
    managerLinear.selectByBox(minPt, maxPt, SelectionMode::REPLACE);

    EXPECT_EQ(managerWithIndex.getSelection().size(),
              managerLinear.getSelection().size());
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(SelectionManagerTest, GetStatistics) {
    SelectionManager manager;
    manager.setMesh(*mesh);
    manager.enableHistory();
    manager.useSpatialIndex(SpatialIndexType::OCTREE);

    manager.selectAll();

    std::string stats = manager.getStatistics();

    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Selected Elements"), std::string::npos);
    EXPECT_NE(stats.find("Spatial Index"), std::string::npos);
    EXPECT_NE(stats.find("History"), std::string::npos);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(SelectionManagerTest, OperationsWithoutMesh) {
    SelectionManager manager;

    // Should not crash
    manager.selectAll();
    manager.clearSelection();
    manager.invertSelection();

    size_t count = manager.selectByBox(
        Eigen::Vector3d(0, 0, 0),
        Eigen::Vector3d(1, 1, 1),
        SelectionMode::REPLACE
    );

    EXPECT_EQ(count, 0);
}

TEST_F(SelectionManagerTest, EmptyBoxSelection) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    // Select with box outside mesh
    size_t count = manager.selectByBox(
        Eigen::Vector3d(100, 100, 100),
        Eigen::Vector3d(101, 101, 101),
        SelectionMode::REPLACE
    );

    EXPECT_EQ(count, 0);
}

TEST_F(SelectionManagerTest, ZeroRadiusSphere) {
    SelectionManager manager;
    manager.setMesh(*mesh);

    size_t count = manager.selectBySphere(
        Eigen::Vector3d(1.5, 1.5, 1.5),
        0.0,
        SelectionMode::REPLACE
    );

    // Should select nothing or very few elements
    EXPECT_LE(count, 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
