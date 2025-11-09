/**
 * @file test_AnnotationManager.cpp
 * @brief Unit tests for AnnotationManager
 */

#include <gtest/gtest.h>
#include "visualization/AnnotationManager.h"
#include <cmath>

using namespace koomesh::visualization;

class AnnotationManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef KOOMESH_HAS_VTK
        renderer = vtkRenderer::New();
        manager = std::make_unique<AnnotationManager>(renderer);
#else
        manager = std::make_unique<AnnotationManager>(nullptr);
#endif
    }

    void TearDown() override {
        manager.reset();
#ifdef KOOMESH_HAS_VTK
        if (renderer) {
            renderer->Delete();
            renderer = nullptr;
        }
#endif
    }

#ifdef KOOMESH_HAS_VTK
    vtkRenderer* renderer = nullptr;
#endif
    std::unique_ptr<AnnotationManager> manager;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(AnnotationManagerTest, DefaultConstruction) {
    EXPECT_EQ(manager->getAnnotationCount(), 0);
}

TEST_F(AnnotationManagerTest, NullRendererHandling) {
    auto nullManager = std::make_unique<AnnotationManager>(nullptr);
    EXPECT_EQ(nullManager->getAnnotationCount(), 0);
}

// ============================================================================
// Distance Measurement Tests
// ============================================================================

TEST_F(AnnotationManagerTest, AddDistanceMeasurement) {
    Eigen::Vector3d p1(0.0, 0.0, 0.0);
    Eigen::Vector3d p2(1.0, 0.0, 0.0);

    int id = manager->addDistanceMeasurement(p1, p2);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, AddDistanceMeasurementWithLabel) {
    Eigen::Vector3d p1(0.0, 0.0, 0.0);
    Eigen::Vector3d p2(3.0, 4.0, 0.0);

    int id = manager->addDistanceMeasurement(p1, p2, "Diagonal");
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, MultipleDistanceMeasurements) {
    int id1 = manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
    int id2 = manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    int id3 = manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {0.0, 0.0, 1.0});

    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_GE(id3, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 3);

    // All IDs should be unique
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id1, id3);
}

TEST_F(AnnotationManagerTest, DistanceMeasurementZeroLength) {
    Eigen::Vector3d p(1.0, 2.0, 3.0);
    int id = manager->addDistanceMeasurement(p, p);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

// ============================================================================
// Angle Measurement Tests
// ============================================================================

TEST_F(AnnotationManagerTest, AddAngleMeasurement) {
    Eigen::Vector3d p1(1.0, 0.0, 0.0);
    Eigen::Vector3d p2(0.0, 0.0, 0.0);
    Eigen::Vector3d p3(0.0, 1.0, 0.0);

    int id = manager->addAngleMeasurement(p1, p2, p3);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, AddAngleMeasurementWithLabel) {
    Eigen::Vector3d p1(1.0, 0.0, 0.0);
    Eigen::Vector3d p2(0.0, 0.0, 0.0);
    Eigen::Vector3d p3(0.0, 1.0, 0.0);

    int id = manager->addAngleMeasurement(p1, p2, p3, "Right Angle");
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, MultipleAngleMeasurements) {
    int id1 = manager->addAngleMeasurement({1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    int id2 = manager->addAngleMeasurement({1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {-1.0, 1.0, 0.0});

    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 2);
    EXPECT_NE(id1, id2);
}

// ============================================================================
// Text Label Tests
// ============================================================================

TEST_F(AnnotationManagerTest, AddTextLabel) {
    Eigen::Vector3d position(1.0, 2.0, 3.0);
    int id = manager->addTextLabel("Test Label", position);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, AddCameraAttachedTextLabel) {
    Eigen::Vector3d position(0.0, 0.0, 0.0);
    int id = manager->addTextLabel("Camera Label", position, true);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, AddWorldTextLabel) {
    Eigen::Vector3d position(5.0, 5.0, 5.0);
    int id = manager->addTextLabel("World Label", position, false);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, MultipleTextLabels) {
    int id1 = manager->addTextLabel("Label 1", {0.0, 0.0, 0.0});
    int id2 = manager->addTextLabel("Label 2", {1.0, 1.0, 1.0});
    int id3 = manager->addTextLabel("Label 3", {2.0, 2.0, 2.0});

    EXPECT_EQ(manager->getAnnotationCount(), 3);
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
}

TEST_F(AnnotationManagerTest, EmptyTextLabel) {
    int id = manager->addTextLabel("", {0.0, 0.0, 0.0});
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

// ============================================================================
// 2D Text Label Tests
// ============================================================================

TEST_F(AnnotationManagerTest, AddScreenLabel) {
    int id = manager->addScreenLabel("Screen Label", 0.5, 0.5);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, AddScreenLabelCorners) {
    int id1 = manager->addScreenLabel("Top Left", 0.1, 0.1);
    int id2 = manager->addScreenLabel("Bottom Right", 0.9, 0.9);

    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 2);
}

TEST_F(AnnotationManagerTest, MultipleScreenLabels) {
    int id1 = manager->addScreenLabel("Label 1", 0.25, 0.25);
    int id2 = manager->addScreenLabel("Label 2", 0.5, 0.5);
    int id3 = manager->addScreenLabel("Label 3", 0.75, 0.75);

    EXPECT_EQ(manager->getAnnotationCount(), 3);
}

// ============================================================================
// Coordinate Axes Tests
// ============================================================================

TEST_F(AnnotationManagerTest, AddCoordinateAxes) {
    int id = manager->addCoordinateAxes();
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, MultipleCoordinateAxes) {
    int id1 = manager->addCoordinateAxes();
    int id2 = manager->addCoordinateAxes();

    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 2);
}

// ============================================================================
// Scalar Bar Tests
// ============================================================================

TEST_F(AnnotationManagerTest, AddScalarBar) {
    int id = manager->addScalarBar("Temperature", nullptr);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, AddScalarBarWithPosition) {
    int id1 = manager->addScalarBar("Pressure", nullptr, 0);  // Right
    int id2 = manager->addScalarBar("Velocity", nullptr, 1);  // Top
    int id3 = manager->addScalarBar("Stress", nullptr, 2);    // Left
    int id4 = manager->addScalarBar("Strain", nullptr, 3);    // Bottom

    EXPECT_EQ(manager->getAnnotationCount(), 4);
}

TEST_F(AnnotationManagerTest, ScalarBarEmptyTitle) {
    int id = manager->addScalarBar("", nullptr);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

// ============================================================================
// Dimension Tests
// ============================================================================

TEST_F(AnnotationManagerTest, AddDimensionLine) {
    Eigen::Vector3d p1(0.0, 0.0, 0.0);
    Eigen::Vector3d p2(5.0, 0.0, 0.0);

    int id = manager->addDimensionLine(p1, p2);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, AddDimensionLineWithOffset) {
    int id = manager->addDimensionLine({0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, 0.2);
    EXPECT_GE(id, 0);
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, MultipleDimensionLines) {
    int id1 = manager->addDimensionLine({0.0, 0.0, 0.0}, {5.0, 0.0, 0.0});
    int id2 = manager->addDimensionLine({0.0, 0.0, 0.0}, {0.0, 3.0, 0.0});

    EXPECT_EQ(manager->getAnnotationCount(), 2);
}

// ============================================================================
// Annotation Management Tests
// ============================================================================

TEST_F(AnnotationManagerTest, RemoveAnnotation) {
    int id1 = manager->addTextLabel("Label 1", {0.0, 0.0, 0.0});
    int id2 = manager->addTextLabel("Label 2", {1.0, 1.0, 1.0});
    int id3 = manager->addTextLabel("Label 3", {2.0, 2.0, 2.0});

    EXPECT_EQ(manager->getAnnotationCount(), 3);

    manager->removeAnnotation(id2);
    EXPECT_EQ(manager->getAnnotationCount(), 2);
}

TEST_F(AnnotationManagerTest, RemoveInvalidAnnotation) {
    int id = manager->addTextLabel("Test", {0.0, 0.0, 0.0});
    EXPECT_EQ(manager->getAnnotationCount(), 1);

    manager->removeAnnotation(999);  // Invalid ID
    EXPECT_EQ(manager->getAnnotationCount(), 1);

    manager->removeAnnotation(-1);  // Negative ID
    EXPECT_EQ(manager->getAnnotationCount(), 1);
}

TEST_F(AnnotationManagerTest, ClearAllAnnotations) {
    manager->addTextLabel("Label 1", {0.0, 0.0, 0.0});
    manager->addTextLabel("Label 2", {1.0, 1.0, 1.0});
    manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
    manager->addAngleMeasurement({1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});

    EXPECT_EQ(manager->getAnnotationCount(), 4);

    manager->clear();
    EXPECT_EQ(manager->getAnnotationCount(), 0);
}

TEST_F(AnnotationManagerTest, ClearAnnotationsEmpty) {
    EXPECT_EQ(manager->getAnnotationCount(), 0);
    manager->clear();
    EXPECT_EQ(manager->getAnnotationCount(), 0);
}

// ============================================================================
// Visibility Tests
// ============================================================================

TEST_F(AnnotationManagerTest, SetVisible) {
    int id = manager->addTextLabel("Test", {0.0, 0.0, 0.0});

    manager->setVisible(id, false);
    // Should not crash

    manager->setVisible(id, true);
    // Should not crash

    SUCCEED();
}

TEST_F(AnnotationManagerTest, SetInvalidVisibility) {
    manager->setVisible(999, false);
    manager->setVisible(-1, true);

    // Should not crash
    SUCCEED();
}

TEST_F(AnnotationManagerTest, SetAllVisible) {
    manager->addTextLabel("Label 1", {0.0, 0.0, 0.0});
    manager->addTextLabel("Label 2", {1.0, 1.0, 1.0});
    manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0});

    manager->setAllVisible(false);
    // Should not crash

    manager->setAllVisible(true);
    // Should not crash

    SUCCEED();
}

TEST_F(AnnotationManagerTest, SetAllVisibleEmpty) {
    EXPECT_EQ(manager->getAnnotationCount(), 0);
    manager->setAllVisible(false);
    manager->setAllVisible(true);

    SUCCEED();
}

// ============================================================================
// Style Tests
// ============================================================================

TEST_F(AnnotationManagerTest, SetDefaultStyle) {
    AnnotationStyle style;
    style.fontSize = 16.0;
    style.textColor = {0.0, 1.0, 1.0};
    style.lineWidth = 3.0;
    style.lineColor = {0.0, 1.0, 0.0};

    manager->setDefaultStyle(style);

    // New annotations should use this style
    int id = manager->addTextLabel("Test", {0.0, 0.0, 0.0});
    EXPECT_GE(id, 0);
}

// ============================================================================
// Update Tests
// ============================================================================

TEST_F(AnnotationManagerTest, Update) {
    manager->addTextLabel("Test", {0.0, 0.0, 0.0});
    manager->addCoordinateAxes();

    manager->update();

    // Should not crash
    SUCCEED();
}

TEST_F(AnnotationManagerTest, UpdateEmpty) {
    EXPECT_EQ(manager->getAnnotationCount(), 0);
    manager->update();

    SUCCEED();
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(AnnotationManagerTest, Statistics) {
    manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
    manager->addAngleMeasurement({1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    manager->addTextLabel("Test", {0.0, 0.0, 0.0});
    manager->addCoordinateAxes();

    std::string stats = manager->getStatistics();

    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Annotation Manager"), std::string::npos);
    EXPECT_NE(stats.find("Total"), std::string::npos);
}

TEST_F(AnnotationManagerTest, StatisticsEmpty) {
    std::string stats = manager->getStatistics();

    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Total"), std::string::npos);
}

// ============================================================================
// Mixed Annotation Tests
// ============================================================================

TEST_F(AnnotationManagerTest, MixedAnnotationTypes) {
    int id1 = manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {1.0, 0.0, 0.0});
    int id2 = manager->addAngleMeasurement({1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
    int id3 = manager->addTextLabel("Label", {0.0, 0.0, 0.0});
    int id4 = manager->addScreenLabel("Screen Label", 0.5, 0.5);
    int id5 = manager->addCoordinateAxes();
    int id6 = manager->addScalarBar("Field", nullptr);
    int id7 = manager->addDimensionLine({0.0, 0.0, 0.0}, {5.0, 0.0, 0.0});
    int id8 = manager->addCaption("Caption", {1.0, 1.0, 1.0});

    EXPECT_EQ(manager->getAnnotationCount(), 8);

    // All IDs should be non-negative
    EXPECT_GE(id1, 0);
    EXPECT_GE(id2, 0);
    EXPECT_GE(id3, 0);
    EXPECT_GE(id4, 0);
    EXPECT_GE(id5, 0);
    EXPECT_GE(id6, 0);
    EXPECT_GE(id7, 0);
    EXPECT_GE(id8, 0);
}

TEST_F(AnnotationManagerTest, MixedOperations) {
    int id1 = manager->addTextLabel("Label 1", {0.0, 0.0, 0.0});
    int id2 = manager->addTextLabel("Label 2", {1.0, 1.0, 1.0});
    int id3 = manager->addDistanceMeasurement({0.0, 0.0, 0.0}, {2.0, 0.0, 0.0});

    EXPECT_EQ(manager->getAnnotationCount(), 3);

    manager->removeAnnotation(id2);
    EXPECT_EQ(manager->getAnnotationCount(), 2);

    manager->setVisible(id1, false);
    manager->setVisible(id3, true);

    AnnotationStyle style;
    style.fontSize = 18.0;
    manager->setDefaultStyle(style);

    manager->update();

    std::string stats = manager->getStatistics();
    EXPECT_FALSE(stats.empty());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
