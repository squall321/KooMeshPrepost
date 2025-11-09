/**
 * @file test_HardwareSelector.cpp
 * @brief Unit tests for HardwareSelector
 */

#include <gtest/gtest.h>
#include "visualization/HardwareSelector.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"

using namespace koomesh::visualization;
using namespace koomesh::core;

class HardwareSelectorTest : public ::testing::Test {
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

        selector = std::make_unique<HardwareSelector>();
    }

    void TearDown() override {
        selector.reset();
        mesh.reset();
    }

    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<HardwareSelector> selector;
};

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(HardwareSelectorTest, DefaultConfiguration) {
    EXPECT_EQ(selector->getFieldAssociation(), SelectionField::CELL);
    EXPECT_FALSE(selector->getSelectOccluded());
    EXPECT_EQ(selector->getMaxSelectionCount(), 0);
}

TEST_F(HardwareSelectorTest, SetFieldAssociation) {
    selector->setFieldAssociation(SelectionField::POINT);
    EXPECT_EQ(selector->getFieldAssociation(), SelectionField::POINT);

    selector->setFieldAssociation(SelectionField::FIELD);
    EXPECT_EQ(selector->getFieldAssociation(), SelectionField::FIELD);

    selector->setFieldAssociation(SelectionField::VERTEX);
    EXPECT_EQ(selector->getFieldAssociation(), SelectionField::VERTEX);
}

TEST_F(HardwareSelectorTest, SetSelectOccluded) {
    selector->setSelectOccluded(true);
    EXPECT_TRUE(selector->getSelectOccluded());

    selector->setSelectOccluded(false);
    EXPECT_FALSE(selector->getSelectOccluded());
}

TEST_F(HardwareSelectorTest, SetMaxSelectionCount) {
    selector->setMaxSelectionCount(100);
    EXPECT_EQ(selector->getMaxSelectionCount(), 100);

    selector->setMaxSelectionCount(0);  // Unlimited
    EXPECT_EQ(selector->getMaxSelectionCount(), 0);
}

// ============================================================================
// Point Selection Tests (Without VTK Renderer)
// ============================================================================

TEST_F(HardwareSelectorTest, SelectAtPoint) {
    // Without VTK renderer, should return empty result but not crash
    auto result = selector->selectAtPoint(nullptr, 100, 100);

    EXPECT_EQ(result.screenX, 100);
    EXPECT_EQ(result.screenY, 100);
    EXPECT_EQ(result.width, 0);
    EXPECT_EQ(result.height, 0);
}

TEST_F(HardwareSelectorTest, SelectAtPointWithTolerance) {
    auto result = selector->selectAtPointWithTolerance(nullptr, 100, 100, 5);

    EXPECT_EQ(result.screenX, 100);
    EXPECT_EQ(result.screenY, 100);
    EXPECT_EQ(result.width, 10);  // 2 * tolerance
    EXPECT_EQ(result.height, 10);
}

TEST_F(HardwareSelectorTest, SelectAtPointDifferentFields) {
    auto result1 = selector->selectAtPoint(nullptr, 50, 50, SelectionField::CELL);
    EXPECT_EQ(result1.screenX, 50);

    auto result2 = selector->selectAtPoint(nullptr, 75, 75, SelectionField::POINT);
    EXPECT_EQ(result2.screenX, 75);
}

// ============================================================================
// Area Selection Tests
// ============================================================================

TEST_F(HardwareSelectorTest, SelectInArea) {
    auto result = selector->selectInArea(nullptr, 10, 10, 100, 100);

    EXPECT_EQ(result.screenX, 10);
    EXPECT_EQ(result.screenY, 10);
    EXPECT_EQ(result.width, 90);
    EXPECT_EQ(result.height, 90);
}

TEST_F(HardwareSelectorTest, SelectInAreaInverted) {
    // End coordinates before start - should be normalized
    auto result = selector->selectInArea(nullptr, 100, 100, 10, 10);

    EXPECT_EQ(result.screenX, 10);
    EXPECT_EQ(result.screenY, 10);
    EXPECT_EQ(result.width, 90);
    EXPECT_EQ(result.height, 90);
}

TEST_F(HardwareSelectorTest, SelectVisibleInArea) {
    auto result = selector->selectVisibleInArea(nullptr, 0, 0, 50, 50);

    EXPECT_EQ(result.screenX, 0);
    EXPECT_EQ(result.screenY, 0);
    EXPECT_EQ(result.width, 50);
    EXPECT_EQ(result.height, 50);
}

// ============================================================================
// Polygon Selection Tests
// ============================================================================

TEST_F(HardwareSelectorTest, SelectInPolygon) {
    std::vector<std::array<int, 2>> polygon = {
        {10, 10},
        {50, 10},
        {50, 50},
        {10, 50}
    };

    auto result = selector->selectInPolygon(nullptr, polygon);

    // Should use bounding box
    EXPECT_EQ(result.screenX, 10);
    EXPECT_EQ(result.screenY, 10);
}

TEST_F(HardwareSelectorTest, SelectInEmptyPolygon) {
    std::vector<std::array<int, 2>> emptyPolygon;

    auto result = selector->selectInPolygon(nullptr, emptyPolygon);

    // Should return empty result
    EXPECT_EQ(result.elements.size(), 0);
}

// ============================================================================
// ID Mapping Tests
// ============================================================================

TEST_F(HardwareSelectorTest, MapCellIdsToElements) {
    std::set<int64_t> vtkCellIds = {0, 1};

    auto elements = selector->mapCellIdsToElements(vtkCellIds, *mesh);

    // Without proper VTK context, mapping is simplified
    // Just verify it doesn't crash
    SUCCEED();
}

TEST_F(HardwareSelectorTest, MapPointIdsToNodes) {
    std::set<int64_t> vtkPointIds = {0, 1, 2};

    auto nodes = selector->mapPointIdsToNodes(vtkPointIds, *mesh);

    SUCCEED();
}

TEST_F(HardwareSelectorTest, RegisterActorMapping) {
    std::vector<ElementId> elementIds = {100, 101};

    // Register mapping (with nullptr prop for testing)
    selector->registerActorMapping(nullptr, elementIds);

    // Should not crash
    SUCCEED();
}

TEST_F(HardwareSelectorTest, ClearMappings) {
    std::vector<ElementId> elementIds = {100, 101};
    selector->registerActorMapping(nullptr, elementIds);

    selector->clearMappings();

    // Should clear all mappings
    SUCCEED();
}

// ============================================================================
// Result Tracking Tests
// ============================================================================

TEST_F(HardwareSelectorTest, LastResult) {
    auto result = selector->selectAtPoint(nullptr, 123, 456);

    auto lastResult = selector->getLastResult();
    EXPECT_EQ(lastResult.screenX, 123);
    EXPECT_EQ(lastResult.screenY, 456);
}

TEST_F(HardwareSelectorTest, ClearLastResult) {
    selector->selectAtPoint(nullptr, 100, 100);

    selector->clearLastResult();

    auto lastResult = selector->getLastResult();
    EXPECT_EQ(lastResult.screenX, 0);
    EXPECT_EQ(lastResult.screenY, 0);
}

TEST_F(HardwareSelectorTest, Statistics) {
    selector->selectInArea(nullptr, 0, 0, 100, 100);

    std::string stats = selector->getStatistics();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Hardware Selection"), std::string::npos);
    EXPECT_NE(stats.find("Screen Position"), std::string::npos);
}

// ============================================================================
// Support Check Tests
// ============================================================================

TEST_F(HardwareSelectorTest, IsSupportedWithNullRenderer) {
    EXPECT_FALSE(HardwareSelector::isSupported(nullptr));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(HardwareSelectorTest, NegativeCoordinates) {
    auto result = selector->selectAtPoint(nullptr, -10, -10);

    EXPECT_EQ(result.screenX, -10);
    EXPECT_EQ(result.screenY, -10);
}

TEST_F(HardwareSelectorTest, ZeroSizeArea) {
    auto result = selector->selectInArea(nullptr, 50, 50, 50, 50);

    EXPECT_EQ(result.screenX, 50);
    EXPECT_EQ(result.screenY, 50);
    EXPECT_EQ(result.width, 0);
    EXPECT_EQ(result.height, 0);
}

TEST_F(HardwareSelectorTest, LargeCoordinates) {
    auto result = selector->selectAtPoint(nullptr, 10000, 10000);

    EXPECT_EQ(result.screenX, 10000);
    EXPECT_EQ(result.screenY, 10000);
}

TEST_F(HardwareSelectorTest, ToleranceZero) {
    auto result = selector->selectAtPointWithTolerance(nullptr, 100, 100, 0);

    EXPECT_EQ(result.width, 0);
    EXPECT_EQ(result.height, 0);
}

TEST_F(HardwareSelectorTest, LargeTolerance) {
    auto result = selector->selectAtPointWithTolerance(nullptr, 100, 100, 100);

    EXPECT_EQ(result.width, 200);
    EXPECT_EQ(result.height, 200);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
