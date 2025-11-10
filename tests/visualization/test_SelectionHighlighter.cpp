/**
 * @file test_SelectionHighlighter.cpp
 * @brief Unit tests for SelectionHighlighter
 */

#include <gtest/gtest.h>
#include "visualization/SelectionHighlighter.h"
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"

using namespace koomesh::visualization;
using namespace koomesh::core;

class SelectionHighlighterTest : public ::testing::Test {
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

        // Add a hexahedron element
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

        highlighter = std::make_unique<SelectionHighlighter>();
    }

    void TearDown() override {
        highlighter.reset();
        mesh.reset();
    }

    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<SelectionHighlighter> highlighter;
};

// ============================================================================
// Selection Management Tests
// ============================================================================

TEST_F(SelectionHighlighterTest, InitialState) {
    EXPECT_EQ(highlighter->getSelectionCount(), 0);
    EXPECT_TRUE(highlighter->getSelectedElements().empty());
    EXPECT_FALSE(highlighter->isSelected(100));
}

TEST_F(SelectionHighlighterTest, SetSelectedElements) {
    std::vector<ElementId> elements = {100, 101};
    highlighter->setSelectedElements(*mesh, elements);

    EXPECT_EQ(highlighter->getSelectionCount(), 2);
    EXPECT_TRUE(highlighter->isSelected(100));
    EXPECT_TRUE(highlighter->isSelected(101));
}

TEST_F(SelectionHighlighterTest, AddToSelection) {
    std::vector<ElementId> elements1 = {100};
    highlighter->setSelectedElements(*mesh, elements1);

    EXPECT_EQ(highlighter->getSelectionCount(), 1);

    std::vector<ElementId> elements2 = {101};
    highlighter->addToSelection(*mesh, elements2);

    EXPECT_EQ(highlighter->getSelectionCount(), 2);
    EXPECT_TRUE(highlighter->isSelected(100));
    EXPECT_TRUE(highlighter->isSelected(101));
}

TEST_F(SelectionHighlighterTest, RemoveFromSelection) {
    std::vector<ElementId> elements = {100, 101};
    highlighter->setSelectedElements(*mesh, elements);

    EXPECT_EQ(highlighter->getSelectionCount(), 2);

    std::vector<ElementId> toRemove = {100};
    highlighter->removeFromSelection(toRemove);

    EXPECT_EQ(highlighter->getSelectionCount(), 1);
    EXPECT_FALSE(highlighter->isSelected(100));
    EXPECT_TRUE(highlighter->isSelected(101));
}

TEST_F(SelectionHighlighterTest, ClearSelection) {
    std::vector<ElementId> elements = {100, 101};
    highlighter->setSelectedElements(*mesh, elements);

    EXPECT_EQ(highlighter->getSelectionCount(), 2);

    highlighter->clearSelection();

    EXPECT_EQ(highlighter->getSelectionCount(), 0);
    EXPECT_TRUE(highlighter->getSelectedElements().empty());
}

TEST_F(SelectionHighlighterTest, DuplicateSelection) {
    std::vector<ElementId> elements = {100, 100, 101};
    highlighter->setSelectedElements(*mesh, elements);

    // Should only have 2 unique elements
    EXPECT_EQ(highlighter->getSelectionCount(), 2);
}

// ============================================================================
// Style Management Tests
// ============================================================================

TEST_F(SelectionHighlighterTest, DefaultStyle) {
    auto style = highlighter->getHighlightStyle();

    EXPECT_DOUBLE_EQ(style.color[0], 1.0);  // Yellow
    EXPECT_DOUBLE_EQ(style.color[1], 1.0);
    EXPECT_DOUBLE_EQ(style.color[2], 0.0);
    EXPECT_DOUBLE_EQ(style.opacity, 1.0);
    EXPECT_DOUBLE_EQ(style.lineWidth, 3.0);
    EXPECT_EQ(style.mode, HighlightMode::SURFACE_WITH_EDGES);
}

TEST_F(SelectionHighlighterTest, SetHighlightColor) {
    highlighter->setHighlightColor(1.0, 0.0, 0.0);  // Red

    auto style = highlighter->getHighlightStyle();
    EXPECT_DOUBLE_EQ(style.color[0], 1.0);
    EXPECT_DOUBLE_EQ(style.color[1], 0.0);
    EXPECT_DOUBLE_EQ(style.color[2], 0.0);
}

TEST_F(SelectionHighlighterTest, SetEdgeColor) {
    highlighter->setEdgeColor(0.0, 1.0, 0.0);  // Green

    auto style = highlighter->getHighlightStyle();
    EXPECT_DOUBLE_EQ(style.edgeColor[0], 0.0);
    EXPECT_DOUBLE_EQ(style.edgeColor[1], 1.0);
    EXPECT_DOUBLE_EQ(style.edgeColor[2], 0.0);
}

TEST_F(SelectionHighlighterTest, SetOpacity) {
    highlighter->setOpacity(0.5);

    auto style = highlighter->getHighlightStyle();
    EXPECT_DOUBLE_EQ(style.opacity, 0.5);
}

TEST_F(SelectionHighlighterTest, SetOpacityClamp) {
    highlighter->setOpacity(1.5);  // Should clamp to 1.0
    EXPECT_DOUBLE_EQ(highlighter->getHighlightStyle().opacity, 1.0);

    highlighter->setOpacity(-0.5);  // Should clamp to 0.0
    EXPECT_DOUBLE_EQ(highlighter->getHighlightStyle().opacity, 0.0);
}

TEST_F(SelectionHighlighterTest, SetLineWidth) {
    highlighter->setLineWidth(5.0);

    auto style = highlighter->getHighlightStyle();
    EXPECT_DOUBLE_EQ(style.lineWidth, 5.0);
}

TEST_F(SelectionHighlighterTest, SetHighlightMode) {
    highlighter->setHighlightMode(HighlightMode::WIREFRAME);

    auto style = highlighter->getHighlightStyle();
    EXPECT_EQ(style.mode, HighlightMode::WIREFRAME);
}

TEST_F(SelectionHighlighterTest, SetCustomStyle) {
    SelectionHighlightStyle customStyle;
    customStyle.color[0] = 0.0;
    customStyle.color[1] = 0.5;
    customStyle.color[2] = 1.0;
    customStyle.opacity = 0.7;
    customStyle.lineWidth = 4.0;
    customStyle.mode = HighlightMode::SOLID;

    highlighter->setHighlightStyle(customStyle);

    auto style = highlighter->getHighlightStyle();
    EXPECT_DOUBLE_EQ(style.color[0], 0.0);
    EXPECT_DOUBLE_EQ(style.color[1], 0.5);
    EXPECT_DOUBLE_EQ(style.color[2], 1.0);
    EXPECT_DOUBLE_EQ(style.opacity, 0.7);
    EXPECT_DOUBLE_EQ(style.lineWidth, 4.0);
    EXPECT_EQ(style.mode, HighlightMode::SOLID);
}

// ============================================================================
// VTK Integration Tests
// ============================================================================

#ifdef KOOMESH_HAS_VTK

TEST_F(SelectionHighlighterTest, GetActors) {
    auto highlightActor = highlighter->getHighlightActor();
    auto edgeActor = highlighter->getEdgeActor();

    EXPECT_NE(highlightActor, nullptr);
    EXPECT_NE(edgeActor, nullptr);
}

TEST_F(SelectionHighlighterTest, VisibilityControl) {
    EXPECT_TRUE(highlighter->isVisible());

    highlighter->setVisible(false);
    EXPECT_FALSE(highlighter->isVisible());

    highlighter->setVisible(true);
    EXPECT_TRUE(highlighter->isVisible());
}

TEST_F(SelectionHighlighterTest, ActorHasMapperAfterSelection) {
    std::vector<ElementId> elements = {100};
    highlighter->setSelectedElements(*mesh, elements);

    auto actor = highlighter->getHighlightActor();
    EXPECT_NE(actor, nullptr);
    EXPECT_NE(actor->GetMapper(), nullptr);
}

TEST_F(SelectionHighlighterTest, ActorClearedAfterClearSelection) {
    std::vector<ElementId> elements = {100};
    highlighter->setSelectedElements(*mesh, elements);

    auto actor = highlighter->getHighlightActor();
    EXPECT_NE(actor->GetMapper(), nullptr);

    highlighter->clearSelection();
    EXPECT_EQ(actor->GetMapper(), nullptr);
}

#endif // KOOMESH_HAS_VTK

// ============================================================================
// Animation Tests
// ============================================================================

TEST_F(SelectionHighlighterTest, PulseAnimation) {
    highlighter->setPulseAnimation(true);
    auto style = highlighter->getHighlightStyle();
    EXPECT_TRUE(style.enablePulse);

    highlighter->setPulseAnimation(false);
    style = highlighter->getHighlightStyle();
    EXPECT_FALSE(style.enablePulse);
}

TEST_F(SelectionHighlighterTest, UpdateAnimation) {
    // Should not crash with no selection
    highlighter->updateAnimation(0.016);  // ~60 FPS

    // Should work with selection
    std::vector<ElementId> elements = {100};
    highlighter->setSelectedElements(*mesh, elements);
    highlighter->setPulseAnimation(true);
    highlighter->updateAnimation(0.016);

    // No easy way to test the actual animation effect without visual inspection
    // But we can verify it doesn't crash
    SUCCEED();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(SelectionHighlighterTest, EmptyMeshSelection) {
    Mesh emptyMesh;
    std::vector<ElementId> elements = {999};  // Non-existent element

    // Should not crash
    highlighter->setSelectedElements(emptyMesh, elements);
    EXPECT_EQ(highlighter->getSelectionCount(), 1);
}

TEST_F(SelectionHighlighterTest, InvalidElementIds) {
    std::vector<ElementId> elements = {999, 1000};  // Non-existent elements

    // Should not crash
    highlighter->setSelectedElements(*mesh, elements);
    EXPECT_EQ(highlighter->getSelectionCount(), 2);
}

TEST_F(SelectionHighlighterTest, MultipleGeometryUpdates) {
    std::vector<ElementId> elements1 = {100};
    highlighter->setSelectedElements(*mesh, elements1);

    std::vector<ElementId> elements2 = {101};
    highlighter->setSelectedElements(*mesh, elements2);

    std::vector<ElementId> elements3 = {100, 101};
    highlighter->setSelectedElements(*mesh, elements3);

    EXPECT_EQ(highlighter->getSelectionCount(), 2);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
