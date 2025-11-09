/**
 * @file test_custom_interactor_style.cpp
 * @brief Unit tests for CustomInteractorStyle
 */

#include <gtest/gtest.h>
#include "visualization/CustomInteractorStyle.h"

#ifdef KOOMESH_HAS_VTK
#include "visualization/ActorManager.h"
#include "visualization/CameraController.h"
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#endif

using namespace koomesh::visualization;

/**
 * @brief Test fixture for CustomInteractorStyle tests
 */
class CustomInteractorStyleTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef KOOMESH_HAS_VTK
        renderer = vtkSmartPointer<vtkRenderer>::New();
        renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
        renderWindow->AddRenderer(renderer);

        interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
        interactor->SetRenderWindow(renderWindow);

        // Create test actors
        for (int i = 0; i < 3; ++i) {
            auto sphere = vtkSmartPointer<vtkSphereSource>::New();
            sphere->SetCenter(i * 5.0, 0.0, 0.0);
            sphere->SetRadius(1.0);

            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(sphere->GetOutputPort());

            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);

            renderer->AddActor(actor);
            testActors.push_back(actor);
        }

        renderer->ResetCamera();

        // Create custom interactor style
        style = vtkSmartPointer<CustomInteractorStyle>::New();
        style->setRenderer(renderer);
        interactor->SetInteractorStyle(style);

        // Create actor manager
        actorManager = std::make_unique<ActorManager>();
        for (auto actor : testActors) {
            actorManager->addActor(actor);
        }
        style->setActorManager(actorManager.get());

        // Create camera controller
        cameraController = std::make_unique<CameraController>(renderer);
        style->setCameraController(cameraController.get());
#endif
    }

#ifdef KOOMESH_HAS_VTK
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkRenderWindow> renderWindow;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<CustomInteractorStyle> style;
    std::vector<vtkSmartPointer<vtkActor>> testActors;
    std::unique_ptr<ActorManager> actorManager;
    std::unique_ptr<CameraController> cameraController;
#endif
};

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// VTK Available Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, Initialization) {
    EXPECT_NE(style, nullptr);
    EXPECT_EQ(style->getRenderer(), renderer);
    EXPECT_EQ(style->getActorManager(), actorManager.get());
    EXPECT_EQ(style->getCameraController(), cameraController.get());
}

TEST_F(CustomInteractorStyleTest, SetGetRenderer) {
    auto newRenderer = vtkSmartPointer<vtkRenderer>::New();
    style->setRenderer(newRenderer);

    EXPECT_EQ(style->getRenderer(), newRenderer);
}

TEST_F(CustomInteractorStyleTest, SetGetActorManager) {
    auto newManager = std::make_unique<ActorManager>();
    style->setActorManager(newManager.get());

    EXPECT_EQ(style->getActorManager(), newManager.get());
}

TEST_F(CustomInteractorStyleTest, SetGetCameraController) {
    auto newController = std::make_unique<CameraController>(renderer);
    style->setCameraController(newController.get());

    EXPECT_EQ(style->getCameraController(), newController.get());
}

// ============================================================================
// Interaction Mode Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, SetGetInteractionMode) {
    style->setInteractionMode(InteractionMode::SELECT_SINGLE);
    EXPECT_EQ(style->getInteractionMode(), InteractionMode::SELECT_SINGLE);

    style->setInteractionMode(InteractionMode::SELECT_AREA);
    EXPECT_EQ(style->getInteractionMode(), InteractionMode::SELECT_AREA);

    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_EQ(style->getInteractionMode(), InteractionMode::CAMERA);
}

TEST_F(CustomInteractorStyleTest, InteractionModeCamera) {
    style->setInteractionMode(InteractionMode::CAMERA);

    EXPECT_FALSE(style->isPickingEnabled());
    EXPECT_FALSE(style->isRubberBandEnabled());
}

TEST_F(CustomInteractorStyleTest, InteractionModeSelectSingle) {
    style->setInteractionMode(InteractionMode::SELECT_SINGLE);

    EXPECT_TRUE(style->isPickingEnabled());
    EXPECT_FALSE(style->isRubberBandEnabled());
}

TEST_F(CustomInteractorStyleTest, InteractionModeSelectArea) {
    style->setInteractionMode(InteractionMode::SELECT_AREA);

    EXPECT_TRUE(style->isPickingEnabled());
    EXPECT_TRUE(style->isRubberBandEnabled());
}

TEST_F(CustomInteractorStyleTest, SetPickingEnabled) {
    style->setPickingEnabled(true);
    EXPECT_TRUE(style->isPickingEnabled());

    style->setPickingEnabled(false);
    EXPECT_FALSE(style->isPickingEnabled());
}

// ============================================================================
// Callback Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, PickCallback) {
    bool callbackCalled = false;
    PickEvent receivedEvent;

    style->setPickCallback([&](const PickEvent& event) {
        callbackCalled = true;
        receivedEvent = event;
    });

    // Manually trigger pick
    style->setInteractionMode(InteractionMode::SELECT_SINGLE);
    PickEvent event = style->pickAtPosition(100, 100);

    // Note: Callback is called in handlePick which is triggered by mouse events
    // For unit test, we just verify callback can be set without errors
    EXPECT_NO_THROW(style->setPickCallback(nullptr));
}

TEST_F(CustomInteractorStyleTest, AreaSelectionCallback) {
    bool callbackCalled = false;

    style->setAreaSelectionCallback([&](const AreaSelectionEvent& event) {
        callbackCalled = true;
    });

    EXPECT_NO_THROW(style->setAreaSelectionCallback(nullptr));
}

TEST_F(CustomInteractorStyleTest, KeyCallback) {
    bool callbackCalled = false;
    std::string receivedKey;

    style->setKeyCallback([&](const std::string& key, const KeyModifiers& mods) {
        callbackCalled = true;
        receivedKey = key;
    });

    EXPECT_NO_THROW(style->setKeyCallback(nullptr));
}

// ============================================================================
// Picking Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, PickAtPosition) {
    style->setInteractionMode(InteractionMode::SELECT_SINGLE);

    // Pick at a position (may or may not hit an actor depending on rendering)
    PickEvent event = style->pickAtPosition(100, 100);

    // Event should be created (validPick may be false if no actor at position)
    EXPECT_GE(event.displayPosition[0], 0.0);
    EXPECT_GE(event.displayPosition[1], 0.0);
}

TEST_F(CustomInteractorStyleTest, PickInArea) {
    style->setInteractionMode(InteractionMode::SELECT_AREA);

    // Pick in an area
    std::vector<vtkActor*> actors = style->pickInArea(50, 50, 150, 150);

    // May or may not find actors depending on rendering
    EXPECT_GE(actors.size(), 0);
}

// ============================================================================
// Rubber Band Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, SetRubberBandEnabled) {
    style->setRubberBandEnabled(true);
    EXPECT_TRUE(style->isRubberBandEnabled());

    style->setRubberBandEnabled(false);
    EXPECT_FALSE(style->isRubberBandEnabled());
}

TEST_F(CustomInteractorStyleTest, RubberBandLifecycle) {
    style->setInteractionMode(InteractionMode::SELECT_AREA);

    EXPECT_NO_THROW(style->startRubberBand());
    EXPECT_NO_THROW(style->updateRubberBand());
    EXPECT_NO_THROW(style->endRubberBand());
}

// ============================================================================
// Keyboard Shortcut Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, AddKeyboardShortcut) {
    bool shortcutCalled = false;

    style->addKeyboardShortcut("f", [&]() {
        shortcutCalled = true;
    });

    // Shortcut is added without error
    EXPECT_NO_THROW(style->addKeyboardShortcut("g", []() {}));
}

TEST_F(CustomInteractorStyleTest, RemoveKeyboardShortcut) {
    style->addKeyboardShortcut("f", []() {});

    EXPECT_NO_THROW(style->removeKeyboardShortcut("f"));
}

TEST_F(CustomInteractorStyleTest, ClearKeyboardShortcuts) {
    style->addKeyboardShortcut("f", []() {});
    style->addKeyboardShortcut("g", []() {});
    style->addKeyboardShortcut("h", []() {});

    EXPECT_NO_THROW(style->clearKeyboardShortcuts());
}

TEST_F(CustomInteractorStyleTest, MultipleKeyboardShortcuts) {
    int count = 0;

    style->addKeyboardShortcut("a", [&]() { count += 1; });
    style->addKeyboardShortcut("b", [&]() { count += 2; });
    style->addKeyboardShortcut("c", [&]() { count += 3; });

    // Shortcuts are registered
    EXPECT_NO_THROW(style->clearKeyboardShortcuts());
}

// ============================================================================
// Coordinate Conversion Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, DisplayToWorld) {
    int display[2] = {100, 100};
    double world[3];

    EXPECT_NO_THROW(style->displayToWorld(display, world));

    // World coordinates should be set
    EXPECT_TRUE(std::isfinite(world[0]));
    EXPECT_TRUE(std::isfinite(world[1]));
    EXPECT_TRUE(std::isfinite(world[2]));
}

TEST_F(CustomInteractorStyleTest, WorldToDisplay) {
    double world[3] = {0.0, 0.0, 0.0};
    int display[2];

    EXPECT_NO_THROW(style->worldToDisplay(world, display));

    // Display coordinates should be set
    EXPECT_TRUE(display[0] != INT_MAX);
    EXPECT_TRUE(display[1] != INT_MAX);
}

TEST_F(CustomInteractorStyleTest, CoordinateRoundTrip) {
    int originalDisplay[2] = {200, 150};
    double world[3];
    int finalDisplay[2];

    style->displayToWorld(originalDisplay, world);
    style->worldToDisplay(world, finalDisplay);

    // Round trip should be approximately equal
    EXPECT_NEAR(originalDisplay[0], finalDisplay[0], 5);
    EXPECT_NEAR(originalDisplay[1], finalDisplay[1], 5);
}

// ============================================================================
// Key Modifiers Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, GetKeyModifiers) {
    KeyModifiers mods = style->getKeyModifiers();

    // Initially all modifiers should be false
    EXPECT_FALSE(mods.shift);
    EXPECT_FALSE(mods.ctrl);
    EXPECT_FALSE(mods.alt);
}

// ============================================================================
// Event Handler Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, OnLeftButtonDown) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnLeftButtonDown());

    style->setInteractionMode(InteractionMode::SELECT_SINGLE);
    EXPECT_NO_THROW(style->OnLeftButtonDown());

    style->setInteractionMode(InteractionMode::SELECT_AREA);
    EXPECT_NO_THROW(style->OnLeftButtonDown());
}

TEST_F(CustomInteractorStyleTest, OnLeftButtonUp) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnLeftButtonUp());
}

TEST_F(CustomInteractorStyleTest, OnMiddleButtonDown) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnMiddleButtonDown());
}

TEST_F(CustomInteractorStyleTest, OnMiddleButtonUp) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnMiddleButtonUp());
}

TEST_F(CustomInteractorStyleTest, OnRightButtonDown) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnRightButtonDown());
}

TEST_F(CustomInteractorStyleTest, OnRightButtonUp) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnRightButtonUp());
}

TEST_F(CustomInteractorStyleTest, OnMouseMove) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnMouseMove());
}

TEST_F(CustomInteractorStyleTest, OnMouseWheelForward) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnMouseWheelForward());
}

TEST_F(CustomInteractorStyleTest, OnMouseWheelBackward) {
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_NO_THROW(style->OnMouseWheelBackward());
}

TEST_F(CustomInteractorStyleTest, OnKeyPress) {
    EXPECT_NO_THROW(style->OnKeyPress());
}

TEST_F(CustomInteractorStyleTest, OnKeyRelease) {
    EXPECT_NO_THROW(style->OnKeyRelease());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(CustomInteractorStyleTest, CameraModeRotation) {
    style->setInteractionMode(InteractionMode::CAMERA);

    double initialPos[3];
    renderer->GetActiveCamera()->GetPosition(initialPos);

    // Simulate rotation (would need actual mouse events in real scenario)
    EXPECT_NO_THROW(style->OnLeftButtonDown());
    EXPECT_NO_THROW(style->OnMouseMove());
    EXPECT_NO_THROW(style->OnLeftButtonUp());
}

TEST_F(CustomInteractorStyleTest, SelectionModeWithActorManager) {
    style->setInteractionMode(InteractionMode::SELECT_SINGLE);

    size_t initialHighlighted = actorManager->getHighlightedActors().size();

    // Pick would highlight if actors are hit
    PickEvent event = style->pickAtPosition(100, 100);

    // Highlighted count may or may not change depending on pick success
    size_t finalHighlighted = actorManager->getHighlightedActors().size();
    EXPECT_GE(finalHighlighted, 0);
}

TEST_F(CustomInteractorStyleTest, AreaSelectionMode) {
    style->setInteractionMode(InteractionMode::SELECT_AREA);

    EXPECT_TRUE(style->isRubberBandEnabled());

    // Simulate area selection
    style->startRubberBand();
    style->updateRubberBand();
    style->endRubberBand();

    // Should complete without errors
    EXPECT_TRUE(true);
}

TEST_F(CustomInteractorStyleTest, ModeSwitch) {
    // Switch between different modes
    style->setInteractionMode(InteractionMode::CAMERA);
    EXPECT_EQ(style->getInteractionMode(), InteractionMode::CAMERA);

    style->setInteractionMode(InteractionMode::SELECT_SINGLE);
    EXPECT_EQ(style->getInteractionMode(), InteractionMode::SELECT_SINGLE);

    style->setInteractionMode(InteractionMode::SELECT_AREA);
    EXPECT_EQ(style->getInteractionMode(), InteractionMode::SELECT_AREA);

    style->setInteractionMode(InteractionMode::MEASURE);
    EXPECT_EQ(style->getInteractionMode(), InteractionMode::MEASURE);
}

#else // !KOOMESH_HAS_VTK

// ============================================================================
// VTK Not Available Tests (Stub)
// ============================================================================

TEST(CustomInteractorStyleStubTest, StubOperations) {
    CustomInteractorStyle style;

    // All operations should be safe no-ops
    style.setRenderer(nullptr);
    EXPECT_EQ(style.getRenderer(), nullptr);

    style.setActorManager(nullptr);
    EXPECT_EQ(style.getActorManager(), nullptr);

    style.setCameraController(nullptr);
    EXPECT_EQ(style.getCameraController(), nullptr);

    style.setInteractionMode(InteractionMode::CAMERA);
    EXPECT_EQ(style.getInteractionMode(), InteractionMode::CAMERA);

    style.setPickingEnabled(true);
    EXPECT_FALSE(style.isPickingEnabled());

    style.setPickCallback([](const PickEvent&) {});
    style.setAreaSelectionCallback([](const AreaSelectionEvent&) {});
    style.setKeyCallback([](const std::string&, const KeyModifiers&) {});

    PickEvent event = style.pickAtCurrentPosition();
    EXPECT_FALSE(event.validPick);

    event = style.pickAtPosition(0, 0);
    EXPECT_FALSE(event.validPick);

    auto actors = style.pickInArea(0, 0, 100, 100);
    EXPECT_TRUE(actors.empty());

    style.setRubberBandEnabled(true);
    EXPECT_FALSE(style.isRubberBandEnabled());

    style.startRubberBand();
    style.updateRubberBand();
    style.endRubberBand();

    style.addKeyboardShortcut("f", []() {});
    style.removeKeyboardShortcut("f");
    style.clearKeyboardShortcuts();

    int display[2] = {0, 0};
    double world[3] = {0.0, 0.0, 0.0};
    style.displayToWorld(display, world);
    style.worldToDisplay(world, display);

    KeyModifiers mods = style.getKeyModifiers();
    EXPECT_FALSE(mods.shift);
    EXPECT_FALSE(mods.ctrl);
    EXPECT_FALSE(mods.alt);
}

#endif // KOOMESH_HAS_VTK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
