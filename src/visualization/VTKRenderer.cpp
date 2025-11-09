/**
 * @file VTKRenderer.cpp
 * @brief Implementation of VTK renderer
 */

#include "visualization/VTKRenderer.h"

#ifdef KOOMESH_HAS_VTK

#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkTIFFWriter.h>
#include <vtkProperty.h>
#include <vtkLight.h>
#include <vtkActorCollection.h>
#include <vtkRendererCollection.h>
#include <chrono>
#include <algorithm>
#include <cstring>

namespace koomesh {
namespace visualization {

// ============================================================================
// VTKRenderer Implementation
// ============================================================================

VTKRenderer::VTKRenderer()
    : m_renderer(vtkSmartPointer<vtkRenderer>::New()),
      m_renderWindow(vtkSmartPointer<vtkRenderWindow>::New()),
      m_interactor(vtkSmartPointer<vtkRenderWindowInteractor>::New()) {
}

VTKRenderer::~VTKRenderer() {
    shutdown();
}

bool VTKRenderer::initialize(const RenderConfig& config) {
    if (m_initialized) {
        return true;
    }

    m_config = config;

    try {
        // Setup rendering pipeline
        setupPipeline();

        // Configure quality settings
        configureQuality(config);

        // Set window properties
        m_renderWindow->SetSize(config.windowWidth, config.windowHeight);
        m_renderWindow->SetWindowName(config.windowTitle.c_str());

        // Set background
        m_renderer->SetBackground(
            config.backgroundColor[0],
            config.backgroundColor[1],
            config.backgroundColor[2]
        );

        // Add default lighting
        m_renderer->AutomaticLightCreationOn();

        m_initialized = true;
        return true;

    } catch (...) {
        return false;
    }
}

void VTKRenderer::startRenderLoop() {
    if (!m_initialized) {
        return;
    }

    m_interactor->Start();
}

void VTKRenderer::render() {
    if (!m_initialized) {
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    m_renderWindow->Render();

    auto endTime = std::chrono::high_resolution_clock::now();
    double frameTime = std::chrono::duration<double>(endTime - startTime).count();

    // Update statistics
    m_frameTimes.push_back(frameTime);
    if (m_frameTimes.size() > m_maxFrameHistory) {
        m_frameTimes.erase(m_frameTimes.begin());
    }

    m_stats.lastFrameTime = frameTime;
    m_stats.totalFrames++;

    updateStatistics();
}

void VTKRenderer::shutdown() {
    if (m_initialized) {
        removeAllActors();
        m_interactor->TerminateApp();
        m_initialized = false;
    }
}

// ============================================================================
// Window Management
// ============================================================================

void VTKRenderer::setWindowSize(int width, int height) {
    if (m_renderWindow) {
        m_renderWindow->SetSize(width, height);
        m_config.windowWidth = width;
        m_config.windowHeight = height;
    }
}

void VTKRenderer::getWindowSize(int& width, int& height) const {
    if (m_renderWindow) {
        int* size = m_renderWindow->GetSize();
        width = size[0];
        height = size[1];
    } else {
        width = m_config.windowWidth;
        height = m_config.windowHeight;
    }
}

void VTKRenderer::setWindowTitle(const std::string& title) {
    if (m_renderWindow) {
        m_renderWindow->SetWindowName(title.c_str());
        m_config.windowTitle = title;
    }
}

void VTKRenderer::setFullScreen(bool enabled) {
    if (m_renderWindow) {
        m_renderWindow->SetFullScreen(enabled);
    }
}

// ============================================================================
// Background and Environment
// ============================================================================

void VTKRenderer::setBackgroundColor(double r, double g, double b) {
    if (m_renderer) {
        m_renderer->SetBackground(r, g, b);
        m_config.backgroundColor[0] = r;
        m_config.backgroundColor[1] = g;
        m_config.backgroundColor[2] = b;
    }
}

void VTKRenderer::setGradientBackground(
    double bottomR, double bottomG, double bottomB,
    double topR, double topG, double topB
) {
    if (m_renderer) {
        m_renderer->SetBackground(bottomR, bottomG, bottomB);
        m_renderer->SetBackground2(topR, topG, topB);
        m_renderer->SetGradientBackground(true);
    }
}

void VTKRenderer::setGradientBackgroundEnabled(bool enabled) {
    if (m_renderer) {
        m_renderer->SetGradientBackground(enabled);
    }
}

// ============================================================================
// Rendering Quality
// ============================================================================

void VTKRenderer::setAntiAliasing(bool enabled, int samples) {
    if (m_renderWindow) {
        m_renderWindow->SetMultiSamples(enabled ? samples : 0);
        m_config.enableAntiAliasing = enabled;
        m_config.multiSamples = samples;
    }
}

void VTKRenderer::setShadowsEnabled(bool enabled) {
    if (m_renderer) {
        // VTK shadow implementation would go here
        // This is a simplified version
        m_config.enableShadows = enabled;
    }
}

void VTKRenderer::setSSAOEnabled(bool enabled) {
    if (m_renderer) {
        // SSAO implementation would require additional VTK passes
        m_config.enableSSAO = enabled;
    }
}

void VTKRenderer::setAmbientLight(double intensity) {
    if (m_renderer) {
        m_renderer->SetAmbient(intensity, intensity, intensity);
    }
}

// ============================================================================
// Camera Access
// ============================================================================

vtkCamera* VTKRenderer::getCamera() const {
    return m_renderer ? m_renderer->GetActiveCamera() : nullptr;
}

void VTKRenderer::resetCamera() {
    if (m_renderer) {
        m_renderer->ResetCamera();
    }
}

void VTKRenderer::resetCamera(const double bounds[6]) {
    if (m_renderer) {
        m_renderer->ResetCamera(bounds);
    }
}

// ============================================================================
// Actor Management
// ============================================================================

void VTKRenderer::addActor(vtkActor* actor) {
    if (m_renderer && actor) {
        m_renderer->AddActor(actor);
    }
}

void VTKRenderer::removeActor(vtkActor* actor) {
    if (m_renderer && actor) {
        m_renderer->RemoveActor(actor);
    }
}

void VTKRenderer::removeAllActors() {
    if (m_renderer) {
        m_renderer->RemoveAllViewProps();
    }
}

int VTKRenderer::getNumberOfActors() const {
    if (m_renderer) {
        vtkActorCollection* actors = m_renderer->GetActors();
        return actors ? actors->GetNumberOfItems() : 0;
    }
    return 0;
}

// ============================================================================
// Screenshot and Export
// ============================================================================

bool VTKRenderer::saveScreenshot(const std::string& filename, int magnification) {
    if (!m_renderWindow) {
        return false;
    }

    try {
        // Create window-to-image filter
        vtkSmartPointer<vtkWindowToImageFilter> windowToImage =
            vtkSmartPointer<vtkWindowToImageFilter>::New();
        windowToImage->SetInput(m_renderWindow);
        windowToImage->SetScale(magnification);
        windowToImage->ReadFrontBufferOff();
        windowToImage->Update();

        // Determine writer based on file extension
        std::string ext = filename.substr(filename.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        vtkSmartPointer<vtkImageWriter> writer;

        if (ext == "png") {
            writer = vtkSmartPointer<vtkPNGWriter>::New();
        } else if (ext == "jpg" || ext == "jpeg") {
            writer = vtkSmartPointer<vtkJPEGWriter>::New();
        } else if (ext == "tif" || ext == "tiff") {
            writer = vtkSmartPointer<vtkTIFFWriter>::New();
        } else {
            // Default to PNG
            writer = vtkSmartPointer<vtkPNGWriter>::New();
        }

        writer->SetFileName(filename.c_str());
        writer->SetInputConnection(windowToImage->GetOutputPort());
        writer->Write();

        return true;

    } catch (...) {
        return false;
    }
}

bool VTKRenderer::saveScreenshotWithAlpha(const std::string& filename, int magnification) {
    if (!m_renderWindow) {
        return false;
    }

    try {
        vtkSmartPointer<vtkWindowToImageFilter> windowToImage =
            vtkSmartPointer<vtkWindowToImageFilter>::New();
        windowToImage->SetInput(m_renderWindow);
        windowToImage->SetScale(magnification);
        windowToImage->SetInputBufferTypeToRGBA();
        windowToImage->ReadFrontBufferOff();
        windowToImage->Update();

        vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
        writer->SetFileName(filename.c_str());
        writer->SetInputConnection(windowToImage->GetOutputPort());
        writer->Write();

        return true;

    } catch (...) {
        return false;
    }
}

// ============================================================================
// Performance Monitoring
// ============================================================================

RenderStats VTKRenderer::getStatistics() const {
    return m_stats;
}

void VTKRenderer::setFPSDisplayEnabled(bool enabled) {
    // FPS display implementation would require text overlay
    (void)enabled;
}

double VTKRenderer::getAverageFPS(int numFrames) const {
    if (m_frameTimes.empty()) {
        return 0.0;
    }

    size_t count = std::min(static_cast<size_t>(numFrames), m_frameTimes.size());
    double totalTime = 0.0;

    for (size_t i = m_frameTimes.size() - count; i < m_frameTimes.size(); ++i) {
        totalTime += m_frameTimes[i];
    }

    if (totalTime > 0.0) {
        return static_cast<double>(count) / totalTime;
    }

    return 0.0;
}

// ============================================================================
// Private Methods
// ============================================================================

void VTKRenderer::setupPipeline() {
    // Add renderer to window
    m_renderWindow->AddRenderer(m_renderer);

    // Set up interactor
    m_interactor->SetRenderWindow(m_renderWindow);

    // Enable user interface interactor
    m_interactor->Initialize();
}

void VTKRenderer::updateStatistics() {
    m_stats.actorCount = static_cast<size_t>(getNumberOfActors());

    // Calculate FPS
    if (!m_frameTimes.empty()) {
        m_stats.fps = getAverageFPS(60);
    }

    // Triangle count would require iterating through all actors
    // and summing their polydata sizes - left as TODO
    m_stats.triangleCount = 0;
}

void VTKRenderer::configureQuality(const RenderConfig& config) {
    if (config.enableAntiAliasing) {
        setAntiAliasing(true, config.multiSamples);
    }

    if (config.enableShadows) {
        setShadowsEnabled(true);
    }

    if (config.enableSSAO) {
        setSSAOEnabled(true);
    }
}

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
