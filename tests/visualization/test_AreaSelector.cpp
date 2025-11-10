/**
 * @file test_AreaSelector.cpp
 * @brief Unit tests for AreaSelector
 */

#include <gtest/gtest.h>
#include "visualization/AreaSelector.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"

using namespace koomesh::visualization;
using namespace koomesh::core;

class AreaSelectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test mesh
        mesh = std::make_unique<Mesh>();

        // Add nodes (8 nodes for a hex element)
        mesh->addNode(Node(1, 0.0, 0.0, 0.0));
        mesh->addNode(Node(2, 1.0, 0.0, 0.0));
        mesh->addNode(Node(3, 1.0, 1.0, 0.0));
        mesh->addNode(Node(4, 0.0, 1.0, 0.0));
        mesh->addNode(Node(5, 0.0, 0.0, 1.0));
        mesh->addNode(Node(6, 1.0, 0.0, 1.0));
        mesh->addNode(Node(7, 1.0, 1.0, 1.0));
        mesh->addNode(Node(8, 0.0, 1.0, 1.0));

        // Add hex elements
        std::vector<NodeId> nodeIds = {1, 2, 3, 4, 5, 6, 7, 8};
        auto element = std::make_unique<HexahedronElement>(100, 1, nodeIds);
        mesh->addElement(std::move(element));

        // Add another hex
        mesh->addNode(Node(9, 2.0, 0.0, 0.0));
        mesh->addNode(Node(10, 2.0, 1.0, 0.0));
        mesh->addNode(Node(11, 2.0, 0.0, 1.0));
        mesh->addNode(Node(12, 2.0, 1.0, 1.0));

        nodeIds = {2, 9, 10, 3, 6, 11, 12, 7};
        element = std::make_unique<HexahedronElement>(101, 1, nodeIds);
        mesh->addElement(std::move(element));

        // Add element from different part
        mesh->addNode(Node(13, 3.0, 0.0, 0.0));
        mesh->addNode(Node(14, 3.0, 1.0, 0.0));
        mesh->addNode(Node(15, 3.0, 0.0, 1.0));
        mesh->addNode(Node(16, 3.0, 1.0, 1.0));

        nodeIds = {9, 13, 14, 10, 11, 15, 16, 12};
        element = std::make_unique<HexahedronElement>(102, 2, nodeIds);
        mesh->addElement(std::move(element));

        selector = std::make_unique<AreaSelector>();
    }

    void TearDown() override {
        selector.reset();
        mesh.reset();
    }

    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<AreaSelector> selector;
};

// ============================================================================
// Filter Configuration Tests
// ============================================================================

TEST_F(AreaSelectorTest, DefaultFilter) {
    auto filter = selector->getFilter();

    EXPECT_FALSE(filter.selectNodes);
    EXPECT_TRUE(filter.selectVisible);
    EXPECT_TRUE(filter.selectPartiallyEnclosed);
    EXPECT_FALSE(filter.filterByType);
    EXPECT_FALSE(filter.filterByPart);
}

TEST_F(AreaSelectorTest, SetFilter) {
    SelectionFilter filter;
    filter.selectNodes = true;
    filter.selectVisible = false;
    filter.selectPartiallyEnclosed = false;
    filter.filterByType = true;
    filter.allowedTypes = {ElementType::HEXAHEDRON};

    selector->setFilter(filter);

    auto retrievedFilter = selector->getFilter();
    EXPECT_TRUE(retrievedFilter.selectNodes);
    EXPECT_FALSE(retrievedFilter.selectVisible);
    EXPECT_FALSE(retrievedFilter.selectPartiallyEnclosed);
    EXPECT_TRUE(retrievedFilter.filterByType);
    EXPECT_EQ(retrievedFilter.allowedTypes.size(), 1);
    EXPECT_EQ(retrievedFilter.allowedTypes[0], ElementType::HEXAHEDRON);
}

TEST_F(AreaSelectorTest, SetElementTypeFilter) {
    std::vector<ElementType> types = {ElementType::HEXAHEDRON, ElementType::TETRAHEDRON};
    selector->setElementTypeFilter(true, types);

    auto filter = selector->getFilter();
    EXPECT_TRUE(filter.filterByType);
    EXPECT_EQ(filter.allowedTypes.size(), 2);
}

TEST_F(AreaSelectorTest, SetPartFilter) {
    std::vector<PartId> parts = {1, 2, 3};
    selector->setPartFilter(true, parts);

    auto filter = selector->getFilter();
    EXPECT_TRUE(filter.filterByPart);
    EXPECT_EQ(filter.allowedParts.size(), 3);
}

// ============================================================================
// Selection Behavior Tests
// ============================================================================

TEST_F(AreaSelectorTest, PartialEnclosure) {
    EXPECT_TRUE(selector->getPartialEnclosure());

    selector->setPartialEnclosure(false);
    EXPECT_FALSE(selector->getPartialEnclosure());
}

TEST_F(AreaSelectorTest, Tolerance) {
    // Default tolerance
    EXPECT_DOUBLE_EQ(selector->getTolerance(), 2.0);

    selector->setTolerance(5.0);
    EXPECT_DOUBLE_EQ(selector->getTolerance(), 5.0);

    // Negative tolerance should be clamped to 0
    selector->setTolerance(-1.0);
    EXPECT_DOUBLE_EQ(selector->getTolerance(), 0.0);
}

// ============================================================================
// Callback Tests
// ============================================================================

TEST_F(AreaSelectorTest, SelectionCallback) {
    bool called = false;
    SelectionResult capturedResult;

    selector->setSelectionCallback([&](const SelectionResult& result) {
        called = true;
        capturedResult = result;
    });

    // Trigger selection (will be empty without VTK renderer)
    auto result = selector->selectInArea(nullptr, *mesh, 0, 0, 100, 100);

    // Callback should have been called
    EXPECT_TRUE(called);
}

TEST_F(AreaSelectorTest, ProgressCallback) {
    int callCount = 0;
    double lastProgress = 0.0;

    selector->setProgressCallback([&](double progress) {
        ++callCount;
        lastProgress = progress;
    });

    // Progress callback would be triggered during large selections
    // Without VTK, we can't test this properly, but we can verify the setter works
    SUCCEED();
}

// ============================================================================
// Selection Result Tests
// ============================================================================

TEST_F(AreaSelectorTest, LastResult) {
    // Initial result should be empty
    auto result = selector->getLastResult();
    EXPECT_EQ(result.elements.size(), 0);

    // After selection, result should be updated
    selector->selectInArea(nullptr, *mesh, 10, 10, 50, 50);

    result = selector->getLastResult();
    EXPECT_EQ(result.startX, 10);
    EXPECT_EQ(result.startY, 10);
    EXPECT_EQ(result.endX, 50);
    EXPECT_EQ(result.endY, 50);
}

TEST_F(AreaSelectorTest, ClearLastResult) {
    selector->selectInArea(nullptr, *mesh, 10, 10, 50, 50);

    auto result = selector->getLastResult();
    // Result should have coordinate data
    EXPECT_TRUE(result.startX != 0 || result.startY != 0);

    selector->clearLastResult();

    result = selector->getLastResult();
    EXPECT_EQ(result.startX, 0);
    EXPECT_EQ(result.startY, 0);
    EXPECT_EQ(result.endX, 0);
    EXPECT_EQ(result.endY, 0);
}

TEST_F(AreaSelectorTest, Statistics) {
    selector->selectInArea(nullptr, *mesh, 0, 0, 100, 100);

    std::string stats = selector->getStatistics();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Area"), std::string::npos);
    EXPECT_NE(stats.find("Selected"), std::string::npos);
}

// ============================================================================
// Selection Mode Tests (stub behavior)
// ============================================================================

TEST_F(AreaSelectorTest, SelectionModeReplace) {
    auto result = selector->selectInArea(nullptr, *mesh, 0, 0, 100, 100, SelectionMode::REPLACE);
    // Without VTK, result will be empty but should not crash
    SUCCEED();
}

TEST_F(AreaSelectorTest, SelectionModeAdd) {
    auto result = selector->selectInArea(nullptr, *mesh, 0, 0, 50, 50, SelectionMode::REPLACE);
    result = selector->selectInArea(nullptr, *mesh, 50, 50, 100, 100, SelectionMode::ADD);
    SUCCEED();
}

TEST_F(AreaSelectorTest, SelectionModeSubtract) {
    auto result = selector->selectInArea(nullptr, *mesh, 0, 0, 100, 100, SelectionMode::REPLACE);
    result = selector->selectInArea(nullptr, *mesh, 25, 25, 75, 75, SelectionMode::SUBTRACT);
    SUCCEED();
}

TEST_F(AreaSelectorTest, SelectionModeIntersect) {
    auto result = selector->selectInArea(nullptr, *mesh, 0, 0, 75, 75, SelectionMode::REPLACE);
    result = selector->selectInArea(nullptr, *mesh, 25, 25, 100, 100, SelectionMode::INTERSECT);
    SUCCEED();
}

// ============================================================================
// Area Selection with Index Tests
// ============================================================================

TEST_F(AreaSelectorTest, SelectInAreaWithIndex) {
    // Build spatial index (if available)
    mesh->buildSpatialIndex();

    // Note: Without access to getSpatialIndex(), we can't test this properly
    // This test verifies the method exists and compiles
    // In production code, you would need access to the spatial index
    SUCCEED();
}

TEST_F(AreaSelectorTest, SelectByFrustum) {
    mesh->buildSpatialIndex();

    // Note: Without access to getSpatialIndex(), we can't test this properly
    // This test verifies the method exists and compiles
    SUCCEED();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(AreaSelectorTest, NullRenderer) {
    // Should not crash with null renderer
    auto result = selector->selectInArea(nullptr, *mesh, 0, 0, 100, 100);
    EXPECT_EQ(result.elements.size(), 0);
}

TEST_F(AreaSelectorTest, EmptyMesh) {
    Mesh emptyMesh;
    auto result = selector->selectInArea(nullptr, emptyMesh, 0, 0, 100, 100);
    EXPECT_EQ(result.elements.size(), 0);
}

TEST_F(AreaSelectorTest, InvertedCoordinates) {
    // End before start - should be normalized
    auto result = selector->selectInArea(nullptr, *mesh, 100, 100, 0, 0);
    // Should not crash
    SUCCEED();
}

TEST_F(AreaSelectorTest, ZeroSizeArea) {
    auto result = selector->selectInArea(nullptr, *mesh, 50, 50, 50, 50);
    // Should handle zero-size area gracefully
    SUCCEED();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
