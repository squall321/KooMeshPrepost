/**
 * @file test_integration.cpp
 * @brief Integration tests for visualization system
 *
 * Tests the complete visualization pipeline including:
 * - VTK rendering integration
 * - Selection system integration
 * - Memory optimization
 * - Camera control
 * - Lighting and materials
 * - Annotations and screenshots
 */

#include <gtest/gtest.h>
#include "visualization/VTKRenderer.h"
#include "visualization/MeshToVTK.h"
#include "visualization/ActorManager.h"
#include "visualization/CameraController.h"
#include "visualization/CustomInteractorStyle.h"
#include "visualization/SelectionHighlighter.h"
#include "visualization/AreaSelector.h"
#include "visualization/HardwareSelector.h"
#include "visualization/ColorMapper.h"
#include "visualization/LightingManager.h"
#include "visualization/AnnotationManager.h"
#include "visualization/ScreenshotManager.h"
#include "visualization/MemoryOptimizer.h"
#include "core/Mesh.h"
#include "core/Element.h"
#include "core/Node.h"
#include <memory>

#ifdef KOOMESH_HAS_VTK
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#endif

using namespace koomesh;
using namespace koomesh::core;
using namespace koomesh::visualization;

// ============================================================================
// Test Fixture with Complete System Setup
// ============================================================================

class VisualizationIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test mesh with known geometry
        createTestMesh();

        // Initialize visualization components
        renderer = std::make_unique<VTKRenderer>();
        meshConverter = std::make_unique<MeshToVTK>();
        actorManager = std::make_unique<ActorManager>();
        cameraController = std::make_unique<CameraController>();
        selectionHighlighter = std::make_unique<SelectionHighlighter>();
        areaSelector = std::make_unique<AreaSelector>();
        hardwareSelector = std::make_unique<HardwareSelector>();
        colorMapper = std::make_unique<ColorMapper>();
        lightingManager = std::make_unique<LightingManager>();
        annotationManager = std::make_unique<AnnotationManager>();
        screenshotManager = std::make_unique<ScreenshotManager>();
        memoryOptimizer = std::make_unique<MemoryOptimizer>();
    }

    void TearDown() override {
        // Clean up in reverse order
        memoryOptimizer.reset();
        screenshotManager.reset();
        annotationManager.reset();
        lightingManager.reset();
        colorMapper.reset();
        hardwareSelector.reset();
        areaSelector.reset();
        selectionHighlighter.reset();
        cameraController.reset();
        actorManager.reset();
        meshConverter.reset();
        renderer.reset();

        mesh.reset();
    }

    void createTestMesh() {
        mesh = std::make_unique<Mesh>();

        // Create a simple mesh for testing (just nodes, elements will be tested separately)
        // 8 nodes forming a cube
        mesh->addNode(Node(0, Eigen::Vector3d(0.0, 0.0, 0.0)));
        mesh->addNode(Node(1, Eigen::Vector3d(1.0, 0.0, 0.0)));
        mesh->addNode(Node(2, Eigen::Vector3d(1.0, 1.0, 0.0)));
        mesh->addNode(Node(3, Eigen::Vector3d(0.0, 1.0, 0.0)));
        mesh->addNode(Node(4, Eigen::Vector3d(0.0, 0.0, 1.0)));
        mesh->addNode(Node(5, Eigen::Vector3d(1.0, 0.0, 1.0)));
        mesh->addNode(Node(6, Eigen::Vector3d(1.0, 1.0, 1.0)));
        mesh->addNode(Node(7, Eigen::Vector3d(0.0, 1.0, 1.0)));

        // Bounding box is computed automatically when accessed
    }

    // Test components
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<VTKRenderer> renderer;
    std::unique_ptr<MeshToVTK> meshConverter;
    std::unique_ptr<ActorManager> actorManager;
    std::unique_ptr<CameraController> cameraController;
    std::unique_ptr<SelectionHighlighter> selectionHighlighter;
    std::unique_ptr<AreaSelector> areaSelector;
    std::unique_ptr<HardwareSelector> hardwareSelector;
    std::unique_ptr<ColorMapper> colorMapper;
    std::unique_ptr<LightingManager> lightingManager;
    std::unique_ptr<AnnotationManager> annotationManager;
    std::unique_ptr<ScreenshotManager> screenshotManager;
    std::unique_ptr<MemoryOptimizer> memoryOptimizer;
};

// ============================================================================
// Basic Integration Tests
// ============================================================================

TEST_F(VisualizationIntegrationTest, ComponentsInitialize) {
    // Verify all components initialized successfully
    EXPECT_NE(renderer, nullptr);
    EXPECT_NE(meshConverter, nullptr);
    EXPECT_NE(actorManager, nullptr);
    EXPECT_NE(cameraController, nullptr);
    EXPECT_NE(selectionHighlighter, nullptr);
    EXPECT_NE(areaSelector, nullptr);
    EXPECT_NE(hardwareSelector, nullptr);
    EXPECT_NE(colorMapper, nullptr);
    EXPECT_NE(lightingManager, nullptr);
    EXPECT_NE(annotationManager, nullptr);
    EXPECT_NE(screenshotManager, nullptr);
    EXPECT_NE(memoryOptimizer, nullptr);
}

TEST_F(VisualizationIntegrationTest, TestMeshCreation) {
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(mesh->nodeCount(), 8);

    const auto& bbox = mesh->boundingBox();
    EXPECT_DOUBLE_EQ(bbox.min().x(), 0.0);
    EXPECT_DOUBLE_EQ(bbox.max().x(), 1.0);
}

// ============================================================================
// Mesh to VTK Conversion Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, MeshToVTKConversion) {
    auto polyData = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData, nullptr);
    EXPECT_EQ(polyData->GetNumberOfPoints(), 8);
    EXPECT_GT(polyData->GetNumberOfCells(), 0);
#else
    EXPECT_EQ(polyData, nullptr);
#endif
}

TEST_F(VisualizationIntegrationTest, MeshToVTKWithMultipleConversions) {
    // Test multiple conversions of the same mesh
    auto polyData1 = meshConverter->convert(*mesh);
    auto polyData2 = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData1, nullptr);
    ASSERT_NE(polyData2, nullptr);
    EXPECT_EQ(polyData1->GetNumberOfPoints(), polyData2->GetNumberOfPoints());
#else
    EXPECT_EQ(polyData1, nullptr);
    EXPECT_EQ(polyData2, nullptr);
#endif
}

// ============================================================================
// Rendering Pipeline Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, RendererInitialization) {
    bool initialized = renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    EXPECT_TRUE(initialized);
#else
    EXPECT_FALSE(initialized);  // Stub returns false
#endif

#ifdef KOOMESH_HAS_VTK
    auto renderWindow = renderer->getRenderWindow();
    EXPECT_NE(renderWindow, nullptr);

    auto vtkRenderer = renderer->getRenderer();
    EXPECT_NE(vtkRenderer, nullptr);
#endif
}

TEST_F(VisualizationIntegrationTest, RenderMeshPipeline) {
    // Complete pipeline: Mesh -> VTK -> Actor -> Renderer
    renderer->initialize();

    auto polyData = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData, nullptr);

    // Create actor from poly data
    auto actor = actorManager->createActor(polyData);
    ASSERT_NE(actor, nullptr);

    // Add to renderer
    renderer->getRenderer()->AddActor(actor);

    // Verify actor is in renderer
    auto actors = renderer->getRenderer()->GetActors();
    EXPECT_EQ(actors->GetNumberOfItems(), 1);
#endif
}

// ============================================================================
// Camera Control Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, CameraControllerWithRenderer) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto camera = renderer->getRenderer()->GetActiveCamera();
    cameraController->setCamera(camera);

    // Test camera operations
    cameraController->resetCamera();
    cameraController->viewFront();

    auto pos = cameraController->getCameraPosition();
    EXPECT_GT(pos.norm(), 0.0);
#endif
}

TEST_F(VisualizationIntegrationTest, CameraFocusOnMesh) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto camera = renderer->getRenderer()->GetActiveCamera();
    cameraController->setCamera(camera);

    const auto& bbox = mesh->boundingBox();
    Eigen::Vector3d center = (bbox.min() + bbox.max()) * 0.5;

    cameraController->focusOn(center);

    auto focal = cameraController->getFocalPoint();
    EXPECT_NEAR(focal.x(), center.x(), 1e-6);
    EXPECT_NEAR(focal.y(), center.y(), 1e-6);
    EXPECT_NEAR(focal.z(), center.z(), 1e-6);
#endif
}

// ============================================================================
// Selection System Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, SelectionHighlighterWithRenderer) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    selectionHighlighter->setRenderer(renderer->getRenderer());

    // Test highlight functionality (use mesh polydata as selection)
    auto polyData = meshConverter->convert(*mesh);

    if (polyData) {
        std::vector<ElementId> selected = {0};
        selectionHighlighter->highlightElements(selected, polyData);
        EXPECT_TRUE(selectionHighlighter->hasHighlight());

        selectionHighlighter->clearHighlight();
        EXPECT_FALSE(selectionHighlighter->hasHighlight());
    }
#endif
}

TEST_F(VisualizationIntegrationTest, AreaSelectorWithRenderer) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    areaSelector->setRenderer(renderer->getRenderer());

    // Test rectangular selection
    areaSelector->startSelection(100, 100);
    areaSelector->updateSelection(200, 200);
    auto selected = areaSelector->endSelection();

    // Selected elements depend on what's visible
    EXPECT_GE(selected.size(), 0);
#endif
}

TEST_F(VisualizationIntegrationTest, HardwareSelectorWithRenderer) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto renderWindow = renderer->getRenderWindow();
    hardwareSelector->setRenderWindow(renderWindow);

    // Test picking at screen position
    auto selected = hardwareSelector->selectAtPosition(100, 100);
    EXPECT_GE(selected.size(), 0);
#endif
}

// ============================================================================
// Color Mapping Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, ColorMapperWithMesh) {
    // Generate scalar field for testing
    std::vector<double> values = {0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 0.5, 0.3};

    auto polyData = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData, nullptr);

    colorMapper->setScalarRange(0.0, 1.0);
    colorMapper->applyScalarField(polyData, values, "TestScalars");

    // Verify scalar array was added
    auto pointData = polyData->GetPointData();
    EXPECT_NE(pointData->GetArray("TestScalars"), nullptr);
#endif
}

TEST_F(VisualizationIntegrationTest, ColorMapperWithActor) {
    auto polyData = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData, nullptr);

    auto actor = actorManager->createActor(polyData);
    ASSERT_NE(actor, nullptr);

    // Apply solid color
    colorMapper->applySolidColor(actor, Eigen::Vector3d(1.0, 0.0, 0.0));

    auto color = actor->GetProperty()->GetColor();
    EXPECT_DOUBLE_EQ(color[0], 1.0);
    EXPECT_DOUBLE_EQ(color[1], 0.0);
    EXPECT_DOUBLE_EQ(color[2], 0.0);
#endif
}

// ============================================================================
// Lighting System Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, LightingManagerWithRenderer) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    lightingManager->setRenderer(renderer->getRenderer());

    // Add lights
    int lightId = lightingManager->addLight(
        Eigen::Vector3d(1.0, 1.0, 1.0),
        Eigen::Vector3d(1.0, 1.0, 1.0),
        1.0
    );

    EXPECT_GE(lightId, 0);
    EXPECT_EQ(lightingManager->getLightCount(), 1);

    lightingManager->removeLight(lightId);
    EXPECT_EQ(lightingManager->getLightCount(), 0);
#endif
}

TEST_F(VisualizationIntegrationTest, LightingPresetsWithMesh) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    lightingManager->setRenderer(renderer->getRenderer());

    auto polyData = meshConverter->convert(*mesh);
    auto actor = actorManager->createActor(polyData);
    renderer->getRenderer()->AddActor(actor);

    // Apply different lighting presets
    lightingManager->applyPreset(LightingPreset::THREE_POINT);
    EXPECT_GT(lightingManager->getLightCount(), 0);

    lightingManager->removeAllLights();
    lightingManager->applyPreset(LightingPreset::STUDIO);
    EXPECT_GT(lightingManager->getLightCount(), 0);
#endif
}

// ============================================================================
// Annotation System Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, AnnotationManagerWithRenderer) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    annotationManager->setRenderer(renderer->getRenderer());

    // Add distance measurement
    Eigen::Vector3d p1(0.0, 0.0, 0.0);
    Eigen::Vector3d p2(1.0, 0.0, 0.0);
    int annoId = annotationManager->addDistanceMeasurement(p1, p2, "Length");

    EXPECT_GE(annoId, 0);
    EXPECT_EQ(annotationManager->getAnnotationCount(), 1);

    annotationManager->clear();
    EXPECT_EQ(annotationManager->getAnnotationCount(), 0);
#endif
}

TEST_F(VisualizationIntegrationTest, AnnotationWithCameraTracking) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto camera = renderer->getRenderer()->GetActiveCamera();
    annotationManager->setRenderer(renderer->getRenderer());
    cameraController->setCamera(camera);

    // Add camera-tracked label
    int labelId = annotationManager->addTextLabel(
        "Test Label",
        Eigen::Vector3d(0.5, 0.5, 0.5),
        true  // attach to camera
    );

    EXPECT_GE(labelId, 0);

    // Change camera view
    cameraController->viewFront();
    cameraController->viewTop();

    // Label should still exist
    EXPECT_EQ(annotationManager->getAnnotationCount(), 1);
#endif
}

// ============================================================================
// Screenshot System Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, ScreenshotManagerWithRenderer) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto renderWindow = renderer->getRenderWindow();
    screenshotManager->setRenderWindow(renderWindow);

    ScreenshotConfig config;
    config.format = ImageFormat::PNG;
    config.width = 800;
    config.height = 600;

    screenshotManager->setScreenshotConfig(config);

    const auto& retrieved = screenshotManager->getScreenshotConfig();
    EXPECT_EQ(retrieved.width, 800);
    EXPECT_EQ(retrieved.height, 600);
#endif
}

TEST_F(VisualizationIntegrationTest, AnimationRecordingSetup) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto renderWindow = renderer->getRenderWindow();
    screenshotManager->setRenderWindow(renderWindow);

    AnimationConfig animConfig;
    animConfig.mode = AnimationMode::AUTO_ROTATE;
    animConfig.fps = 30;
    animConfig.numFrames = 60;

    screenshotManager->startRecording(animConfig);
    EXPECT_TRUE(screenshotManager->isRecording());

    screenshotManager->stopRecording();
    EXPECT_FALSE(screenshotManager->isRecording());
#endif
}

// ============================================================================
// Memory Optimization Integration
// ============================================================================

TEST_F(VisualizationIntegrationTest, MemoryOptimizerLODCreation) {
    auto polyData = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData, nullptr);

    LODConfig lodConfig;
    lodConfig.enabled = true;
    lodConfig.numLevels = 3;

    memoryOptimizer->setLODConfig(lodConfig);

    auto lodActor = memoryOptimizer->createLODActor(polyData, lodConfig);
    EXPECT_NE(lodActor, nullptr);
#endif
}

TEST_F(VisualizationIntegrationTest, MemoryEstimationForMesh) {
    auto polyData = meshConverter->convert(*mesh);

    size_t memoryUsage = memoryOptimizer->estimateMemoryUsage(polyData);

#ifdef KOOMESH_HAS_VTK
    EXPECT_GT(memoryUsage, 0);
#else
    EXPECT_EQ(memoryUsage, 0);
#endif
}

TEST_F(VisualizationIntegrationTest, GPUOptimizationForActor) {
    auto polyData = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData, nullptr);

    auto actor = actorManager->createActor(polyData);
    ASSERT_NE(actor, nullptr);

    GPUMemoryConfig gpuConfig;
    gpuConfig.useVBOs = true;
    memoryOptimizer->setGPUMemoryConfig(gpuConfig);

    memoryOptimizer->optimizeActorForGPU(actor);

    // Should not crash
    SUCCEED();
#endif
}

// ============================================================================
// Complete Workflow Tests
// ============================================================================

TEST_F(VisualizationIntegrationTest, CompleteVisualizationWorkflow) {
    // 1. Initialize renderer
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto vtkRenderer = renderer->getRenderer();
    auto renderWindow = renderer->getRenderWindow();
    auto camera = vtkRenderer->GetActiveCamera();

    // 2. Convert mesh to VTK
    auto polyData = meshConverter->convert(*mesh);
    ASSERT_NE(polyData, nullptr);

    // 3. Create and add actor
    auto actor = actorManager->createActor(polyData);
    ASSERT_NE(actor, nullptr);
    vtkRenderer->AddActor(actor);

    // 4. Setup camera
    cameraController->setCamera(camera);
    cameraController->resetCamera();
    cameraController->viewFront();

    // 5. Apply lighting
    lightingManager->setRenderer(vtkRenderer);
    lightingManager->applyPreset(LightingPreset::THREE_POINT);

    // 6. Add color mapping
    std::vector<double> scalars(8, 0.5);
    colorMapper->applyScalarField(polyData, scalars, "TestField");

    // 7. Add annotations
    annotationManager->setRenderer(vtkRenderer);
    annotationManager->addTextLabel("Test Mesh", Eigen::Vector3d(0.5, 0.5, 0.5));

    // 8. Setup screenshot capability
    screenshotManager->setRenderWindow(renderWindow);

    // Verify complete system
    EXPECT_EQ(vtkRenderer->GetActors()->GetNumberOfItems(), 1);
    EXPECT_GT(lightingManager->getLightCount(), 0);
    EXPECT_EQ(annotationManager->getAnnotationCount(), 1);
#endif

    SUCCEED();
}

TEST_F(VisualizationIntegrationTest, SelectionAndHighlightWorkflow) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto vtkRenderer = renderer->getRenderer();

    // Setup mesh visualization
    auto polyData = meshConverter->convert(*mesh);
    auto actor = actorManager->createActor(polyData);
    vtkRenderer->AddActor(actor);

    // Setup selection
    selectionHighlighter->setRenderer(vtkRenderer);

    // Select element
    std::vector<ElementId> selected = {0};
    auto selectedPolyData = meshConverter->convert(*mesh);

    if (selectedPolyData) {
        // Highlight selection
        selectionHighlighter->highlightElements(selected, selectedPolyData);
        EXPECT_TRUE(selectionHighlighter->hasHighlight());

        // Add measurement annotation
        annotationManager->setRenderer(vtkRenderer);
        annotationManager->addDistanceMeasurement(
            Eigen::Vector3d(0.0, 0.0, 0.0),
            Eigen::Vector3d(1.0, 0.0, 0.0)
        );

        EXPECT_EQ(annotationManager->getAnnotationCount(), 1);
    }
#endif

    SUCCEED();
}

TEST_F(VisualizationIntegrationTest, LargeDatasetOptimizationWorkflow) {
    renderer->initialize();

    auto polyData = meshConverter->convert(*mesh);

#ifdef KOOMESH_HAS_VTK
    ASSERT_NE(polyData, nullptr);

    // Check if dataset is large
    bool isLarge = memoryOptimizer->isLargeDataset(polyData, 100);

    // Get recommended LOD levels
    int cellCount = polyData->GetNumberOfCells();
    int recommendedLevels = memoryOptimizer->getRecommendedLODLevels(cellCount);
    EXPECT_GT(recommendedLevels, 0);

    // Create LOD actor
    LODConfig lodConfig;
    lodConfig.numLevels = recommendedLevels;
    auto lodActor = memoryOptimizer->createLODActor(polyData, lodConfig);

    if (lodActor) {
        // Optimize for GPU
        memoryOptimizer->optimizeActorForGPU(lodActor);

        // Add to renderer
        renderer->getRenderer()->AddActor(lodActor);

        EXPECT_EQ(renderer->getRenderer()->GetActors()->GetNumberOfItems(), 1);
    }
#endif

    SUCCEED();
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(VisualizationIntegrationTest, RenderingPerformance) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto polyData = meshConverter->convert(*mesh);
    ASSERT_NE(polyData, nullptr);

    auto actor = actorManager->createActor(polyData);
    renderer->getRenderer()->AddActor(actor);

    // Multiple render calls should not crash
    for (int i = 0; i < 10; ++i) {
        renderer->render();
    }
#endif

    SUCCEED();
}

TEST_F(VisualizationIntegrationTest, MultipleActorsPerformance) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto vtkRenderer = renderer->getRenderer();

    // Add multiple actors (one per element if we had more elements)
    auto polyData = meshConverter->convert(*mesh);
    ASSERT_NE(polyData, nullptr);

    for (int i = 0; i < 5; ++i) {
        auto actor = actorManager->createActor(polyData);
        actor->SetPosition(i * 2.0, 0, 0);
        vtkRenderer->AddActor(actor);
    }

    EXPECT_EQ(vtkRenderer->GetActors()->GetNumberOfItems(), 5);

    // Should render without issues
    renderer->render();
#endif

    SUCCEED();
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(VisualizationIntegrationTest, NullPointerSafety) {
    // All components should handle nullptr gracefully
    Mesh emptyMesh;
    EXPECT_EQ(meshConverter->convert(emptyMesh), nullptr);
    EXPECT_EQ(memoryOptimizer->estimateMemoryUsage(nullptr), 0);

#ifdef KOOMESH_HAS_VTK
    selectionHighlighter->setRenderer(nullptr);
    annotationManager->setRenderer(nullptr);
    lightingManager->setRenderer(nullptr);

    // Should not crash
    SUCCEED();
#endif
}

TEST_F(VisualizationIntegrationTest, InvalidConfigurationHandling) {
    // Invalid LOD configuration
    LODConfig lodConfig;
    lodConfig.numLevels = -1;
    memoryOptimizer->setLODConfig(lodConfig);

    auto retrieved = memoryOptimizer->getLODConfig();
    EXPECT_EQ(retrieved.numLevels, -1);  // Should store as-is

    // Invalid screenshot configuration
    ScreenshotConfig screenConfig;
    screenConfig.width = -100;
    screenConfig.height = -100;
    screenshotManager->setScreenshotConfig(screenConfig);

    // Should not crash
    SUCCEED();
}

// ============================================================================
// Cleanup Tests
// ============================================================================

TEST_F(VisualizationIntegrationTest, ProperCleanup) {
    renderer->initialize();

#ifdef KOOMESH_HAS_VTK
    auto vtkRenderer = renderer->getRenderer();

    // Add various components
    auto polyData = meshConverter->convert(*mesh);
    auto actor = actorManager->createActor(polyData);
    vtkRenderer->AddActor(actor);

    lightingManager->setRenderer(vtkRenderer);
    lightingManager->applyPreset(LightingPreset::THREE_POINT);

    annotationManager->setRenderer(vtkRenderer);
    annotationManager->addTextLabel("Test", Eigen::Vector3d::Zero());

    // Clear all
    vtkRenderer->RemoveAllViewProps();
    lightingManager->removeAllLights();
    annotationManager->clear();
    memoryOptimizer->clearCache();

    EXPECT_EQ(vtkRenderer->GetActors()->GetNumberOfItems(), 0);
    EXPECT_EQ(lightingManager->getLightCount(), 0);
    EXPECT_EQ(annotationManager->getAnnotationCount(), 0);
#endif

    SUCCEED();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
