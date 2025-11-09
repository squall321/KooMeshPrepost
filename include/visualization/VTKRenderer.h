/**
 * @file VTKRenderer.h
 * @brief VTK-based 3D renderer for mesh visualization
 */

#pragma once

#ifdef KOOMESH_HAS_VTK

#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkActor.h>
#include <string>
#include <memory>

namespace koomesh {
namespace visualization {

/**
 * @brief Rendering configuration
 */
struct RenderConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    bool enableAntiAliasing = true;
    int multiSamples = 8;
    bool enableShadows = false;
    bool enableSSAO = false;
    double backgroundColor[3] = {0.1, 0.1, 0.2};
    std::string windowTitle = "KooMeshPrepost";
};

/**
 * @brief Rendering statistics
 */
struct RenderStats {
    double fps = 0.0;
    size_t triangleCount = 0;
    size_t actorCount = 0;
    double lastFrameTime = 0.0;
    size_t totalFrames = 0;
};

/**
 * @brief Main VTK renderer class
 *
 * This class encapsulates VTK rendering pipeline providing:
 * - Window and viewport management
 * - Camera control
 * - Actor management
 * - Rendering configuration
 * - Performance monitoring
 */
class VTKRenderer {
public:
    /**
     * @brief Constructor
     */
    VTKRenderer();

    /**
     * @brief Destructor
     */
    ~VTKRenderer();

    /**
     * @brief Initialize renderer with configuration
     * @param config Rendering configuration
     * @return True if initialization successful
     */
    bool initialize(const RenderConfig& config = RenderConfig());

    /**
     * @brief Start rendering loop (blocking)
     */
    void startRenderLoop();

    /**
     * @brief Render single frame
     */
    void render();

    /**
     * @brief Stop rendering and cleanup
     */
    void shutdown();

    /**
     * @brief Check if renderer is initialized
     */
    bool isInitialized() const { return m_initialized; }

    // ========================================================================
    // Window Management
    // ========================================================================

    /**
     * @brief Set window size
     * @param width Window width in pixels
     * @param height Window height in pixels
     */
    void setWindowSize(int width, int height);

    /**
     * @brief Get window size
     */
    void getWindowSize(int& width, int& height) const;

    /**
     * @brief Set window title
     */
    void setWindowTitle(const std::string& title);

    /**
     * @brief Enable/disable full screen
     */
    void setFullScreen(bool enabled);

    // ========================================================================
    // Background and Environment
    // ========================================================================

    /**
     * @brief Set background color
     * @param r Red component (0-1)
     * @param g Green component (0-1)
     * @param b Blue component (0-1)
     */
    void setBackgroundColor(double r, double g, double b);

    /**
     * @brief Set gradient background
     * @param bottom Bottom color (RGB)
     * @param top Top color (RGB)
     */
    void setGradientBackground(
        double bottomR, double bottomG, double bottomB,
        double topR, double topG, double topB
    );

    /**
     * @brief Enable/disable gradient background
     */
    void setGradientBackgroundEnabled(bool enabled);

    // ========================================================================
    // Rendering Quality
    // ========================================================================

    /**
     * @brief Enable/disable anti-aliasing
     * @param enabled Enable anti-aliasing
     * @param samples Number of samples (2, 4, 8, 16)
     */
    void setAntiAliasing(bool enabled, int samples = 8);

    /**
     * @brief Enable/disable shadows
     */
    void setShadowsEnabled(bool enabled);

    /**
     * @brief Enable/disable Screen Space Ambient Occlusion
     */
    void setSSAOEnabled(bool enabled);

    /**
     * @brief Set ambient lighting level
     * @param intensity Ambient intensity (0-1)
     */
    void setAmbientLight(double intensity);

    // ========================================================================
    // Camera Access
    // ========================================================================

    /**
     * @brief Get VTK camera
     */
    vtkCamera* getCamera() const;

    /**
     * @brief Reset camera to view all actors
     */
    void resetCamera();

    /**
     * @brief Reset camera with specific bounds
     */
    void resetCamera(const double bounds[6]);

    // ========================================================================
    // Actor Management
    // ========================================================================

    /**
     * @brief Add actor to scene
     * @param actor VTK actor to add
     */
    void addActor(vtkActor* actor);

    /**
     * @brief Remove actor from scene
     * @param actor VTK actor to remove
     */
    void removeActor(vtkActor* actor);

    /**
     * @brief Remove all actors
     */
    void removeAllActors();

    /**
     * @brief Get number of actors in scene
     */
    int getNumberOfActors() const;

    // ========================================================================
    // Screenshot and Export
    // ========================================================================

    /**
     * @brief Save screenshot to file
     * @param filename Output file path (png, jpg, tiff supported)
     * @param magnification Scale factor for higher resolution
     * @return True if save successful
     */
    bool saveScreenshot(const std::string& filename, int magnification = 1);

    /**
     * @brief Save screenshot with transparency
     */
    bool saveScreenshotWithAlpha(const std::string& filename, int magnification = 1);

    // ========================================================================
    // Performance Monitoring
    // ========================================================================

    /**
     * @brief Get rendering statistics
     */
    RenderStats getStatistics() const;

    /**
     * @brief Enable/disable FPS display in window
     */
    void setFPSDisplayEnabled(bool enabled);

    /**
     * @brief Get average FPS over last N frames
     */
    double getAverageFPS(int numFrames = 60) const;

    // ========================================================================
    // VTK Object Access
    // ========================================================================

    /**
     * @brief Get VTK renderer
     */
    vtkRenderer* getVTKRenderer() const { return m_renderer; }

    /**
     * @brief Get VTK render window
     */
    vtkRenderWindow* getRenderWindow() const { return m_renderWindow; }

    /**
     * @brief Get VTK interactor
     */
    vtkRenderWindowInteractor* getInteractor() const { return m_interactor; }

private:
    /**
     * @brief Setup rendering pipeline
     */
    void setupPipeline();

    /**
     * @brief Update statistics
     */
    void updateStatistics();

    /**
     * @brief Configure quality settings
     */
    void configureQuality(const RenderConfig& config);

    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;

    RenderConfig m_config;
    RenderStats m_stats;
    bool m_initialized = false;

    // Performance tracking
    std::vector<double> m_frameTimes;
    size_t m_maxFrameHistory = 120;
};

} // namespace visualization
} // namespace koomesh

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
namespace koomesh {
namespace visualization {

struct RenderConfig {};
struct RenderStats { double fps = 0.0; };

class VTKRenderer {
public:
    VTKRenderer() {}
    ~VTKRenderer() {}
    bool initialize(const RenderConfig& = RenderConfig()) { return false; }
    void startRenderLoop() {}
    void render() {}
    void shutdown() {}
    bool isInitialized() const { return false; }
    void setWindowSize(int, int) {}
    void getWindowSize(int&, int&) const {}
    void setWindowTitle(const std::string&) {}
    void setFullScreen(bool) {}
    void setBackgroundColor(double, double, double) {}
    void setGradientBackground(double, double, double, double, double, double) {}
    void setGradientBackgroundEnabled(bool) {}
    void setAntiAliasing(bool, int = 8) {}
    void setShadowsEnabled(bool) {}
    void setSSAOEnabled(bool) {}
    void setAmbientLight(double) {}
    void resetCamera() {}
    void resetCamera(const double[6]) {}
    void removeAllActors() {}
    int getNumberOfActors() const { return 0; }
    bool saveScreenshot(const std::string&, int = 1) { return false; }
    bool saveScreenshotWithAlpha(const std::string&, int = 1) { return false; }
    RenderStats getStatistics() const { return RenderStats(); }
    void setFPSDisplayEnabled(bool) {}
    double getAverageFPS(int = 60) const { return 0.0; }
};

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
