/**
 * @file test_vtk_renderer.cpp
 * @brief Unit tests for VTKRenderer
 */

#include <gtest/gtest.h>
#include "visualization/VTKRenderer.h"

using namespace koomesh::visualization;

/**
 * @brief Test fixture for VTKRenderer tests
 */
class VTKRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        renderer = std::make_unique<VTKRenderer>();
    }

    std::unique_ptr<VTKRenderer> renderer;
};

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// VTK Available Tests
// ============================================================================

TEST_F(VTKRendererTest, Initialization) {
    RenderConfig config;
    config.windowWidth = 800;
    config.windowHeight = 600;

    bool success = renderer->initialize(config);
    EXPECT_TRUE(success);
    EXPECT_TRUE(renderer->isInitialized());
}

TEST_F(VTKRendererTest, InitializeTwice) {
    RenderConfig config;

    bool success1 = renderer->initialize(config);
    EXPECT_TRUE(success1);

    // Second initialization should succeed (no-op)
    bool success2 = renderer->initialize(config);
    EXPECT_TRUE(success2);
}

TEST_F(VTKRendererTest, WindowSize) {
    RenderConfig config;
    config.windowWidth = 1024;
    config.windowHeight = 768;

    renderer->initialize(config);

    int width, height;
    renderer->getWindowSize(width, height);

    EXPECT_EQ(width, 1024);
    EXPECT_EQ(height, 768);
}

TEST_F(VTKRendererTest, SetWindowSize) {
    renderer->initialize();

    renderer->setWindowSize(1920, 1080);

    int width, height;
    renderer->getWindowSize(width, height);

    EXPECT_EQ(width, 1920);
    EXPECT_EQ(height, 1080);
}

TEST_F(VTKRendererTest, BackgroundColor) {
    renderer->initialize();

    // Set background color (no crash test)
    renderer->setBackgroundColor(0.2, 0.3, 0.4);

    // Cannot easily verify without rendering, but should not crash
}

TEST_F(VTKRendererTest, GradientBackground) {
    renderer->initialize();

    renderer->setGradientBackground(0.1, 0.1, 0.2, 0.5, 0.6, 0.8);
    renderer->setGradientBackgroundEnabled(true);

    // Should not crash
}

TEST_F(VTKRendererTest, AntiAliasing) {
    renderer->initialize();

    renderer->setAntiAliasing(true, 8);
    // Should not crash
}

TEST_F(VTKRendererTest, CameraAccess) {
    renderer->initialize();

    auto camera = renderer->getCamera();
    EXPECT_NE(camera, nullptr);
}

TEST_F(VTKRendererTest, ResetCamera) {
    renderer->initialize();

    // Should not crash
    renderer->resetCamera();
}

TEST_F(VTKRendererTest, ResetCameraWithBounds) {
    renderer->initialize();

    double bounds[6] = {0.0, 10.0, 0.0, 10.0, 0.0, 10.0};
    renderer->resetCamera(bounds);

    // Should not crash
}

TEST_F(VTKRendererTest, ActorManagement) {
    renderer->initialize();

    // Initially no actors
    EXPECT_EQ(renderer->getNumberOfActors(), 0);

    // Add actor
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    renderer->addActor(actor);

    EXPECT_EQ(renderer->getNumberOfActors(), 1);

    // Remove actor
    renderer->removeActor(actor);
    EXPECT_EQ(renderer->getNumberOfActors(), 0);
}

TEST_F(VTKRendererTest, RemoveAllActors) {
    renderer->initialize();

    // Add multiple actors
    for (int i = 0; i < 5; ++i) {
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        renderer->addActor(actor);
    }

    EXPECT_EQ(renderer->getNumberOfActors(), 5);

    renderer->removeAllActors();
    EXPECT_EQ(renderer->getNumberOfActors(), 0);
}

TEST_F(VTKRendererTest, Render) {
    renderer->initialize();

    // Should not crash
    renderer->render();
}

TEST_F(VTKRendererTest, Statistics) {
    renderer->initialize();

    // Render a few frames
    for (int i = 0; i < 10; ++i) {
        renderer->render();
    }

    auto stats = renderer->getStatistics();

    EXPECT_EQ(stats.totalFrames, 10);
    EXPECT_GT(stats.lastFrameTime, 0.0);
}

TEST_F(VTKRendererTest, AverageFPS) {
    renderer->initialize();

    // Render some frames
    for (int i = 0; i < 5; ++i) {
        renderer->render();
    }

    double fps = renderer->getAverageFPS();
    EXPECT_GE(fps, 0.0);  // Should be non-negative
}

TEST_F(VTKRendererTest, SaveScreenshot) {
    renderer->initialize();

    std::string filename = "/tmp/test_screenshot.png";

    bool success = renderer->saveScreenshot(filename);
    EXPECT_TRUE(success);

    // Clean up
    std::remove(filename.c_str());
}

TEST_F(VTKRendererTest, SaveScreenshotWithMagnification) {
    renderer->initialize();

    std::string filename = "/tmp/test_screenshot_2x.png";

    bool success = renderer->saveScreenshot(filename, 2);
    EXPECT_TRUE(success);

    std::remove(filename.c_str());
}

TEST_F(VTKRendererTest, SaveScreenshotWithAlpha) {
    renderer->initialize();

    std::string filename = "/tmp/test_screenshot_alpha.png";

    bool success = renderer->saveScreenshotWithAlpha(filename);
    EXPECT_TRUE(success);

    std::remove(filename.c_str());
}

TEST_F(VTKRendererTest, VTKObjectAccess) {
    renderer->initialize();

    auto vtkRenderer = renderer->getVTKRenderer();
    EXPECT_NE(vtkRenderer, nullptr);

    auto renderWindow = renderer->getRenderWindow();
    EXPECT_NE(renderWindow, nullptr);

    auto interactor = renderer->getInteractor();
    EXPECT_NE(interactor, nullptr);
}

TEST_F(VTKRendererTest, Shutdown) {
    renderer->initialize();
    EXPECT_TRUE(renderer->isInitialized());

    renderer->shutdown();
    EXPECT_FALSE(renderer->isInitialized());
}

TEST_F(VTKRendererTest, OperationsBeforeInit) {
    // Operations before initialization should not crash
    renderer->render();
    renderer->setWindowSize(800, 600);
    renderer->setBackgroundColor(0.1, 0.2, 0.3);
    renderer->resetCamera();

    int width, height;
    renderer->getWindowSize(width, height);

    EXPECT_FALSE(renderer->isInitialized());
}

#else // !KOOMESH_HAS_VTK

// ============================================================================
// VTK Not Available Tests (Stub)
// ============================================================================

TEST_F(VTKRendererTest, StubInitialization) {
    RenderConfig config;
    bool success = renderer->initialize(config);

    EXPECT_FALSE(success);
    EXPECT_FALSE(renderer->isInitialized());
}

TEST_F(VTKRendererTest, StubOperations) {
    // All operations should be safe no-ops
    renderer->initialize();
    renderer->startRenderLoop();
    renderer->render();
    renderer->setWindowSize(800, 600);
    renderer->setBackgroundColor(0.1, 0.2, 0.3);
    renderer->resetCamera();
    renderer->removeAllActors();

    EXPECT_EQ(renderer->getNumberOfActors(), 0);

    auto stats = renderer->getStatistics();
    EXPECT_EQ(stats.fps, 0.0);

    double fps = renderer->getAverageFPS();
    EXPECT_EQ(fps, 0.0);

    bool success = renderer->saveScreenshot("/tmp/test.png");
    EXPECT_FALSE(success);

    renderer->shutdown();
}

TEST_F(VTKRendererTest, StubNoVTKMessage) {
    // This test documents that VTK is not available
    EXPECT_FALSE(renderer->isInitialized());

    std::cout << "Note: VTK is not available. VTKRenderer runs in stub mode.\n";
}

#endif // KOOMESH_HAS_VTK

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
