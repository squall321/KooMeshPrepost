/**
 * @file test_camera_controller.cpp
 * @brief Unit tests for CameraController
 */

#include <gtest/gtest.h>
#include "visualization/CameraController.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <cmath>
#endif

using namespace koomesh::visualization;

/**
 * @brief Test fixture for CameraController tests
 */
class CameraControllerTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef KOOMESH_HAS_VTK
        renderer = vtkSmartPointer<vtkRenderer>::New();
        renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
        renderWindow->AddRenderer(renderer);

        // Add a test actor for camera to focus on
        auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
        sphereSource->SetCenter(0.0, 0.0, 0.0);
        sphereSource->SetRadius(1.0);

        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(sphereSource->GetOutputPort());

        auto actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);

        renderer->AddActor(actor);
        renderer->ResetCamera();

        controller = std::make_unique<CameraController>(renderer);
#endif
    }

#ifdef KOOMESH_HAS_VTK
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkRenderWindow> renderWindow;
#endif
    std::unique_ptr<CameraController> controller;
};

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// VTK Available Tests
// ============================================================================

TEST_F(CameraControllerTest, Initialization) {
    vtkCamera* camera = controller->getCamera();
    EXPECT_NE(camera, nullptr);
}

TEST_F(CameraControllerTest, Reset) {
    // Modify camera
    controller->setPosition(100.0, 100.0, 100.0);

    // Reset
    controller->reset();

    // Camera should be repositioned
    double position[3];
    controller->getPosition(position);

    // Position should be non-zero and at a reasonable distance
    double distance = std::sqrt(
        position[0] * position[0] +
        position[1] * position[1] +
        position[2] * position[2]
    );
    EXPECT_GT(distance, 0.0);
}

TEST_F(CameraControllerTest, ResetWithBounds) {
    double bounds[6] = {-10.0, 10.0, -10.0, 10.0, -10.0, 10.0};

    controller->reset(bounds);

    double focalPoint[3];
    controller->getFocalPoint(focalPoint);

    // Focal point should be at bounds center
    EXPECT_NEAR(focalPoint[0], 0.0, 0.1);
    EXPECT_NEAR(focalPoint[1], 0.0, 0.1);
    EXPECT_NEAR(focalPoint[2], 0.0, 0.1);
}

// ============================================================================
// View Preset Tests
// ============================================================================

TEST_F(CameraControllerTest, FrontView) {
    controller->setFrontView(false);

    double position[3];
    controller->getPosition(position);

    // Front view should have positive Z
    EXPECT_GT(position[2], 0.0);

    // X and Y should be near center
    EXPECT_NEAR(position[0], 0.0, 0.1);
    EXPECT_NEAR(position[1], 0.0, 0.1);
}

TEST_F(CameraControllerTest, BackView) {
    controller->setBackView(false);

    double position[3];
    controller->getPosition(position);

    // Back view should have negative Z
    EXPECT_LT(position[2], 0.0);
}

TEST_F(CameraControllerTest, TopView) {
    controller->setTopView(false);

    double position[3];
    controller->getPosition(position);

    // Top view should have positive Y
    EXPECT_GT(position[1], 0.0);

    // View up should be in -Z direction
    double viewUp[3];
    controller->getViewUp(viewUp);
    EXPECT_NEAR(viewUp[2], -1.0, 0.1);
}

TEST_F(CameraControllerTest, BottomView) {
    controller->setBottomView(false);

    double position[3];
    controller->getPosition(position);

    // Bottom view should have negative Y
    EXPECT_LT(position[1], 0.0);

    // View up should be in +Z direction
    double viewUp[3];
    controller->getViewUp(viewUp);
    EXPECT_NEAR(viewUp[2], 1.0, 0.1);
}

TEST_F(CameraControllerTest, LeftView) {
    controller->setLeftView(false);

    double position[3];
    controller->getPosition(position);

    // Left view should have negative X
    EXPECT_LT(position[0], 0.0);
}

TEST_F(CameraControllerTest, RightView) {
    controller->setRightView(false);

    double position[3];
    controller->getPosition(position);

    // Right view should have positive X
    EXPECT_GT(position[0], 0.0);
}

TEST_F(CameraControllerTest, IsometricView) {
    controller->setIsometricView(false);

    double position[3];
    controller->getPosition(position);

    // Isometric view should have all positive coordinates
    EXPECT_GT(position[0], 0.0);
    EXPECT_GT(position[1], 0.0);
    EXPECT_GT(position[2], 0.0);

    // All coordinates should be roughly equal
    EXPECT_NEAR(position[0], position[1], 0.1);
    EXPECT_NEAR(position[1], position[2], 0.1);
}

// ============================================================================
// Camera Positioning Tests
// ============================================================================

TEST_F(CameraControllerTest, SetGetPosition) {
    controller->setPosition(10.0, 20.0, 30.0);

    double position[3];
    controller->getPosition(position);

    EXPECT_DOUBLE_EQ(position[0], 10.0);
    EXPECT_DOUBLE_EQ(position[1], 20.0);
    EXPECT_DOUBLE_EQ(position[2], 30.0);
}

TEST_F(CameraControllerTest, SetGetFocalPoint) {
    controller->setFocalPoint(5.0, 10.0, 15.0);

    double focalPoint[3];
    controller->getFocalPoint(focalPoint);

    EXPECT_DOUBLE_EQ(focalPoint[0], 5.0);
    EXPECT_DOUBLE_EQ(focalPoint[1], 10.0);
    EXPECT_DOUBLE_EQ(focalPoint[2], 15.0);
}

TEST_F(CameraControllerTest, SetGetViewUp) {
    controller->setViewUp(0.0, 0.0, 1.0);

    double viewUp[3];
    controller->getViewUp(viewUp);

    // VTK may normalize the view up vector
    double length = std::sqrt(
        viewUp[0] * viewUp[0] +
        viewUp[1] * viewUp[1] +
        viewUp[2] * viewUp[2]
    );
    EXPECT_NEAR(length, 1.0, 0.01);

    // Z component should dominate
    EXPECT_GT(std::abs(viewUp[2]), 0.9);
}

// ============================================================================
// Zoom and Distance Tests
// ============================================================================

TEST_F(CameraControllerTest, Zoom) {
    double initialAngle = controller->getViewAngle();

    controller->zoom(2.0);  // Zoom in

    double newAngle = controller->getViewAngle();

    // Zooming in should reduce view angle
    EXPECT_LT(newAngle, initialAngle);
}

TEST_F(CameraControllerTest, SetGetDistance) {
    controller->setDistance(50.0);

    double distance = controller->getDistance();
    EXPECT_DOUBLE_EQ(distance, 50.0);
}

TEST_F(CameraControllerTest, SetGetViewAngle) {
    controller->setViewAngle(45.0);

    double angle = controller->getViewAngle();
    EXPECT_DOUBLE_EQ(angle, 45.0);
}

// ============================================================================
// Focus Tests
// ============================================================================

TEST_F(CameraControllerTest, FocusOnPoint) {
    controller->focusOnPoint(10.0, 20.0, 30.0, false);

    double focalPoint[3];
    controller->getFocalPoint(focalPoint);

    EXPECT_DOUBLE_EQ(focalPoint[0], 10.0);
    EXPECT_DOUBLE_EQ(focalPoint[1], 20.0);
    EXPECT_DOUBLE_EQ(focalPoint[2], 30.0);
}

TEST_F(CameraControllerTest, FocusOnBounds) {
    double bounds[6] = {0.0, 10.0, 0.0, 20.0, 0.0, 30.0};

    controller->focusOnBounds(bounds, false);

    double focalPoint[3];
    controller->getFocalPoint(focalPoint);

    // Focal point should be at bounds center
    EXPECT_NEAR(focalPoint[0], 5.0, 0.1);
    EXPECT_NEAR(focalPoint[1], 10.0, 0.1);
    EXPECT_NEAR(focalPoint[2], 15.0, 0.1);
}

// ============================================================================
// Rotation Tests
// ============================================================================

TEST_F(CameraControllerTest, Rotate) {
    controller->setFrontView(false);

    double initialPos[3];
    controller->getPosition(initialPos);

    controller->rotate(90.0, 0.0);  // Rotate 90 degrees azimuth

    double newPos[3];
    controller->getPosition(newPos);

    // Position should have changed
    bool positionChanged = (
        std::abs(newPos[0] - initialPos[0]) > 0.1 ||
        std::abs(newPos[1] - initialPos[1]) > 0.1 ||
        std::abs(newPos[2] - initialPos[2]) > 0.1
    );
    EXPECT_TRUE(positionChanged);
}

TEST_F(CameraControllerTest, Roll) {
    double initialViewUp[3];
    controller->getViewUp(initialViewUp);

    controller->roll(45.0);

    double newViewUp[3];
    controller->getViewUp(newViewUp);

    // View up should have changed
    bool viewUpChanged = (
        std::abs(newViewUp[0] - initialViewUp[0]) > 0.1 ||
        std::abs(newViewUp[1] - initialViewUp[1]) > 0.1 ||
        std::abs(newViewUp[2] - initialViewUp[2]) > 0.1
    );
    EXPECT_TRUE(viewUpChanged);
}

// ============================================================================
// State Management Tests
// ============================================================================

TEST_F(CameraControllerTest, SaveRestoreState) {
    controller->setPosition(10.0, 20.0, 30.0);
    controller->setFocalPoint(5.0, 10.0, 15.0);
    controller->setViewAngle(45.0);

    CameraState state = controller->saveState();

    EXPECT_DOUBLE_EQ(state.position[0], 10.0);
    EXPECT_DOUBLE_EQ(state.position[1], 20.0);
    EXPECT_DOUBLE_EQ(state.position[2], 30.0);
    EXPECT_DOUBLE_EQ(state.focalPoint[0], 5.0);
    EXPECT_DOUBLE_EQ(state.focalPoint[1], 10.0);
    EXPECT_DOUBLE_EQ(state.focalPoint[2], 15.0);
    EXPECT_DOUBLE_EQ(state.viewAngle, 45.0);

    // Modify camera
    controller->setPosition(100.0, 200.0, 300.0);

    // Restore
    controller->restoreState(state, false);

    double position[3];
    controller->getPosition(position);
    EXPECT_DOUBLE_EQ(position[0], 10.0);
    EXPECT_DOUBLE_EQ(position[1], 20.0);
    EXPECT_DOUBLE_EQ(position[2], 30.0);
}

TEST_F(CameraControllerTest, PushPopState) {
    controller->setPosition(10.0, 20.0, 30.0);
    controller->pushState();

    controller->setPosition(100.0, 200.0, 300.0);

    bool success = controller->popState();
    EXPECT_TRUE(success);

    double position[3];
    controller->getPosition(position);
    EXPECT_DOUBLE_EQ(position[0], 10.0);
    EXPECT_DOUBLE_EQ(position[1], 20.0);
    EXPECT_DOUBLE_EQ(position[2], 30.0);
}

TEST_F(CameraControllerTest, PopEmptyHistory) {
    bool success = controller->popState();
    EXPECT_FALSE(success);
}

TEST_F(CameraControllerTest, HistorySize) {
    EXPECT_EQ(controller->getHistorySize(), 0);

    controller->pushState();
    EXPECT_EQ(controller->getHistorySize(), 1);

    controller->pushState();
    EXPECT_EQ(controller->getHistorySize(), 2);

    controller->popState();
    EXPECT_EQ(controller->getHistorySize(), 1);

    controller->clearHistory();
    EXPECT_EQ(controller->getHistorySize(), 0);
}

TEST_F(CameraControllerTest, HistorySizeLimit) {
    // Push more states than the maximum history size (20)
    for (int i = 0; i < 25; ++i) {
        controller->pushState();
    }

    // History should be limited to 20
    EXPECT_LE(controller->getHistorySize(), 20);
}

// ============================================================================
// Animation Tests
// ============================================================================

TEST_F(CameraControllerTest, SetGetAnimationParams) {
    CameraAnimation params;
    params.duration = 2.0;
    params.steps = 60;
    params.smooth = false;

    controller->setAnimationParams(params);

    const CameraAnimation& retrieved = controller->getAnimationParams();
    EXPECT_DOUBLE_EQ(retrieved.duration, 2.0);
    EXPECT_EQ(retrieved.steps, 60);
    EXPECT_FALSE(retrieved.smooth);
}

TEST_F(CameraControllerTest, AnimateToState) {
    CameraState targetState;
    targetState.position[0] = 100.0;
    targetState.position[1] = 100.0;
    targetState.position[2] = 100.0;
    targetState.focalPoint[0] = 0.0;
    targetState.focalPoint[1] = 0.0;
    targetState.focalPoint[2] = 0.0;
    targetState.viewUp[0] = 0.0;
    targetState.viewUp[1] = 1.0;
    targetState.viewUp[2] = 0.0;
    targetState.viewAngle = 45.0;

    bool success = controller->animateToState(targetState);
    EXPECT_TRUE(success);

    // Camera should be at target state
    double position[3];
    controller->getPosition(position);
    EXPECT_DOUBLE_EQ(position[0], 100.0);
    EXPECT_DOUBLE_EQ(position[1], 100.0);
    EXPECT_DOUBLE_EQ(position[2], 100.0);
}

// ============================================================================
// Clipping Range Tests
// ============================================================================

TEST_F(CameraControllerTest, SetGetClippingRange) {
    controller->setClippingRange(0.5, 500.0);

    double range[2];
    controller->getClippingRange(range);

    EXPECT_DOUBLE_EQ(range[0], 0.5);
    EXPECT_DOUBLE_EQ(range[1], 500.0);
}

TEST_F(CameraControllerTest, ResetClippingRange) {
    controller->setClippingRange(0.1, 100.0);
    controller->resetClippingRange();

    // Clipping range should be automatically computed
    double range[2];
    controller->getClippingRange(range);

    EXPECT_GT(range[0], 0.0);
    EXPECT_GT(range[1], range[0]);
}

// ============================================================================
// Parallel Projection Tests
// ============================================================================

TEST_F(CameraControllerTest, SetParallelProjection) {
    controller->setParallelProjection(true);
    EXPECT_TRUE(controller->isParallelProjection());

    controller->setParallelProjection(false);
    EXPECT_FALSE(controller->isParallelProjection());
}

TEST_F(CameraControllerTest, SetGetParallelScale) {
    controller->setParallelProjection(true);
    controller->setParallelScale(5.0);

    double scale = controller->getParallelScale();
    EXPECT_DOUBLE_EQ(scale, 5.0);
}

#else // !KOOMESH_HAS_VTK

// ============================================================================
// VTK Not Available Tests (Stub)
// ============================================================================

TEST_F(CameraControllerTest, StubOperations) {
    // All operations should be safe no-ops
    controller = std::make_unique<CameraController>(nullptr);

    EXPECT_EQ(controller->getCamera(), nullptr);

    controller->reset();
    double bounds[6] = {0};
    controller->reset(bounds);

    controller->setViewPreset(ViewPreset::FRONT);
    controller->setFrontView();
    controller->setBackView();
    controller->setTopView();
    controller->setBottomView();
    controller->setLeftView();
    controller->setRightView();
    controller->setIsometricView();

    double arr[3];
    controller->setPosition(1.0, 2.0, 3.0);
    controller->getPosition(arr);
    controller->setFocalPoint(1.0, 2.0, 3.0);
    controller->getFocalPoint(arr);
    controller->setViewUp(0.0, 1.0, 0.0);
    controller->getViewUp(arr);

    controller->zoom(2.0);
    controller->setDistance(10.0);
    EXPECT_EQ(controller->getDistance(), 0.0);
    controller->setViewAngle(45.0);
    EXPECT_EQ(controller->getViewAngle(), 30.0);

    controller->focusOnPoint(0.0, 0.0, 0.0);
    controller->focusOnBounds(bounds);

    controller->rotate(45.0, 45.0);
    controller->orbit(10.0, 10.0);
    controller->roll(30.0);

    CameraState state = controller->saveState();
    controller->restoreState(state);
    controller->pushState();
    EXPECT_FALSE(controller->popState());
    controller->clearHistory();
    EXPECT_EQ(controller->getHistorySize(), 0);

    CameraAnimation anim;
    controller->setAnimationParams(anim);
    controller->animateToState(state);

    double range[2];
    controller->setClippingRange(0.1, 100.0);
    controller->getClippingRange(range);
    controller->resetClippingRange();

    controller->setParallelProjection(true);
    EXPECT_FALSE(controller->isParallelProjection());
    controller->setParallelScale(5.0);
    EXPECT_EQ(controller->getParallelScale(), 1.0);
}

#endif // KOOMESH_HAS_VTK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
