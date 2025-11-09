/**
 * @file ScreenshotManager.cpp
 * @brief Screenshot and animation recording implementation
 */

#include "visualization/ScreenshotManager.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTransform.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace koomesh {
namespace visualization {

ScreenshotManager::ScreenshotManager(vtkRenderWindow* renderWindow)
    : m_renderWindow(renderWindow)
    , m_windowToImage(vtkSmartPointer<vtkWindowToImageFilter>::New())
    , m_isRecording(false)
    , m_currentFrame(0)
{
    if (m_renderWindow) {
        m_windowToImage->SetInput(m_renderWindow);
        m_windowToImage->SetInputBufferTypeToRGB();
    }

    // Set default screenshot config
    m_screenshotConfig.format = ImageFormat::PNG;
    m_screenshotConfig.width = 1920;
    m_screenshotConfig.height = 1080;
    m_screenshotConfig.quality = 95;
    m_screenshotConfig.transparent = false;
    m_screenshotConfig.magnification = 1;

    // Set default animation config
    m_animConfig.mode = AnimationMode::MANUAL;
    m_animConfig.fps = 30;
    m_animConfig.numFrames = 60;
    m_animConfig.rotationDegrees = 360.0;
    m_animConfig.loop = false;
    m_animConfig.outputPrefix = "frame_";
}

ScreenshotManager::~ScreenshotManager() {
    stopAnimation();
}

// ============================================================================
// Screenshot Capture
// ============================================================================

bool ScreenshotManager::captureScreenshot(const std::string& filename) {
    if (!m_renderWindow) {
        return false;
    }

    // Detect format from filename if not explicitly set
    ImageFormat format = detectFormat(filename);

    return saveImage(filename, format, m_screenshotConfig.quality);
}

bool ScreenshotManager::captureScreenshot(const std::string& filename, const ScreenshotConfig& config) {
    if (!m_renderWindow) {
        return false;
    }

    // Temporarily save current config
    ScreenshotConfig oldConfig = m_screenshotConfig;
    m_screenshotConfig = config;

    // Set window size
    if (config.width > 0 && config.height > 0) {
        m_renderWindow->SetSize(config.width, config.height);
    }

    // Set magnification
    if (config.magnification > 1) {
        m_windowToImage->SetScale(config.magnification);
    }

    // Handle transparency
    if (config.transparent && config.format == ImageFormat::PNG) {
        m_windowToImage->SetInputBufferTypeToRGBA();
    } else {
        m_windowToImage->SetInputBufferTypeToRGB();
    }

    // Render and capture
    m_renderWindow->Render();
    bool success = saveImage(filename, config.format, config.quality);

    // Restore settings
    m_windowToImage->SetScale(1);
    m_windowToImage->SetInputBufferTypeToRGB();
    m_screenshotConfig = oldConfig;

    return success;
}

bool ScreenshotManager::captureHighResScreenshot(const std::string& filename, int magnification) {
    if (!m_renderWindow || magnification < 1) {
        return false;
    }

    ScreenshotConfig config = m_screenshotConfig;
    config.magnification = magnification;

    return captureScreenshot(filename, config);
}

void ScreenshotManager::setScreenshotConfig(const ScreenshotConfig& config) {
    m_screenshotConfig = config;
}

const ScreenshotConfig& ScreenshotManager::getScreenshotConfig() const {
    return m_screenshotConfig;
}

// ============================================================================
// Animation Recording
// ============================================================================

bool ScreenshotManager::startAnimation(const std::string& outputDir, const AnimationConfig& config) {
    if (!m_renderWindow || m_isRecording) {
        return false;
    }

    m_outputDir = outputDir;
    m_animConfig = config;
    m_currentFrame = 0;
    m_isRecording = true;

    // Store initial camera state
    storeInitialCamera();

    return true;
}

void ScreenshotManager::stopAnimation() {
    if (!m_isRecording) {
        return;
    }

    m_isRecording = false;
    m_currentFrame = 0;

    // Restore initial camera
    restoreInitialCamera();
}

bool ScreenshotManager::isRecording() const {
    return m_isRecording;
}

bool ScreenshotManager::recordFrame() {
    if (!m_isRecording || !m_renderWindow) {
        return false;
    }

    // Generate filename
    std::string filename = generateFrameFilename(m_currentFrame);

    // Render and capture
    m_renderWindow->Render();
    bool success = saveImage(filename, m_screenshotConfig.format, m_screenshotConfig.quality);

    if (success) {
        m_currentFrame++;

        // Check if animation complete
        if (m_currentFrame >= m_animConfig.numFrames) {
            if (m_animConfig.loop) {
                m_currentFrame = 0;
            } else {
                stopAnimation();
            }
        }
    }

    return success;
}

bool ScreenshotManager::recordAllFrames() {
    if (!m_isRecording || !m_renderWindow) {
        return false;
    }

    for (int i = m_currentFrame; i < m_animConfig.numFrames; ++i) {
        // Update camera/scene based on animation mode
        switch (m_animConfig.mode) {
            case AnimationMode::AUTO_ROTATE:
                applyRotationAnimation(i);
                break;
            case AnimationMode::AUTO_PATH:
                applyPathAnimation(i);
                break;
            case AnimationMode::CUSTOM_CALLBACK:
                if (m_animCallback) {
                    m_animCallback(i);
                }
                break;
            case AnimationMode::MANUAL:
            default:
                // Manual mode: user controls updates
                break;
        }

        // Generate filename
        std::string filename = generateFrameFilename(i);

        // Render and capture
        m_renderWindow->Render();
        if (!saveImage(filename, m_screenshotConfig.format, m_screenshotConfig.quality)) {
            return false;
        }

        m_currentFrame = i + 1;
    }

    stopAnimation();
    return true;
}

void ScreenshotManager::setAnimationConfig(const AnimationConfig& config) {
    m_animConfig = config;
}

const AnimationConfig& ScreenshotManager::getAnimationConfig() const {
    return m_animConfig;
}

// ============================================================================
// Camera Animation
// ============================================================================

void ScreenshotManager::addCameraKeyframe(const CameraKeyframe& keyframe) {
    m_keyframes.push_back(keyframe);

    // Sort keyframes by time
    std::sort(m_keyframes.begin(), m_keyframes.end(),
              [](const CameraKeyframe& a, const CameraKeyframe& b) {
                  return a.time < b.time;
              });
}

void ScreenshotManager::clearCameraKeyframes() {
    m_keyframes.clear();
}

int ScreenshotManager::getCameraKeyframeCount() const {
    return static_cast<int>(m_keyframes.size());
}

void ScreenshotManager::setCameraForFrame(int frameIndex) {
    if (!m_renderWindow || m_animConfig.numFrames == 0) {
        return;
    }

    double t = static_cast<double>(frameIndex) / static_cast<double>(m_animConfig.numFrames - 1);

    switch (m_animConfig.mode) {
        case AnimationMode::AUTO_ROTATE:
            applyRotationAnimation(frameIndex);
            break;
        case AnimationMode::AUTO_PATH:
            applyPathAnimation(frameIndex);
            break;
        default:
            break;
    }
}

void ScreenshotManager::setAnimationCallback(std::function<void(int)> callback) {
    m_animCallback = callback;
}

// ============================================================================
// Utility
// ============================================================================

int ScreenshotManager::getCurrentFrame() const {
    return m_currentFrame;
}

int ScreenshotManager::getTotalFrames() const {
    return m_animConfig.numFrames;
}

double ScreenshotManager::getProgress() const {
    if (m_animConfig.numFrames == 0) {
        return 0.0;
    }

    return static_cast<double>(m_currentFrame) / static_cast<double>(m_animConfig.numFrames);
}

std::string ScreenshotManager::getStatistics() const {
    std::ostringstream oss;
    oss << "Screenshot Manager Statistics:\n";
    oss << "  Recording: " << (m_isRecording ? "Yes" : "No") << "\n";
    oss << "  Current Frame: " << m_currentFrame << "\n";
    oss << "  Total Frames: " << m_animConfig.numFrames << "\n";
    oss << "  FPS: " << m_animConfig.fps << "\n";
    oss << "  Animation Mode: ";

    switch (m_animConfig.mode) {
        case AnimationMode::MANUAL:
            oss << "Manual";
            break;
        case AnimationMode::AUTO_ROTATE:
            oss << "Auto Rotate";
            break;
        case AnimationMode::AUTO_PATH:
            oss << "Path (" << m_keyframes.size() << " keyframes)";
            break;
        case AnimationMode::CUSTOM_CALLBACK:
            oss << "Custom Callback";
            break;
    }

    oss << "\n";
    oss << "  Screenshot Format: ";

    switch (m_screenshotConfig.format) {
        case ImageFormat::PNG:
            oss << "PNG";
            break;
        case ImageFormat::JPEG:
            oss << "JPEG";
            break;
        case ImageFormat::BMP:
            oss << "BMP";
            break;
        case ImageFormat::TIFF:
            oss << "TIFF";
            break;
    }

    oss << "\n";
    oss << "  Resolution: " << m_screenshotConfig.width << "x" << m_screenshotConfig.height << "\n";

    return oss.str();
}

// ============================================================================
// Private Methods
// ============================================================================

bool ScreenshotManager::saveImage(const std::string& filename, ImageFormat format, int quality) {
    if (!m_renderWindow) {
        return false;
    }

    m_windowToImage->Modified();
    m_windowToImage->Update();

    bool success = false;

    switch (format) {
        case ImageFormat::PNG: {
            auto writer = vtkSmartPointer<vtkPNGWriter>::New();
            writer->SetFileName(filename.c_str());
            writer->SetInputConnection(m_windowToImage->GetOutputPort());
            writer->Write();
            success = true;
            break;
        }
        case ImageFormat::JPEG: {
            auto writer = vtkSmartPointer<vtkJPEGWriter>::New();
            writer->SetFileName(filename.c_str());
            writer->SetQuality(quality);
            writer->SetInputConnection(m_windowToImage->GetOutputPort());
            writer->Write();
            success = true;
            break;
        }
        case ImageFormat::BMP: {
            auto writer = vtkSmartPointer<vtkBMPWriter>::New();
            writer->SetFileName(filename.c_str());
            writer->SetInputConnection(m_windowToImage->GetOutputPort());
            writer->Write();
            success = true;
            break;
        }
        case ImageFormat::TIFF: {
            auto writer = vtkSmartPointer<vtkTIFFWriter>::New();
            writer->SetFileName(filename.c_str());
            writer->SetInputConnection(m_windowToImage->GetOutputPort());
            writer->Write();
            success = true;
            break;
        }
        default:
            success = false;
            break;
    }

    return success;
}

CameraKeyframe ScreenshotManager::interpolateCamera(double t) const {
    if (m_keyframes.empty()) {
        CameraKeyframe kf;
        kf.time = t;
        return kf;
    }

    if (m_keyframes.size() == 1 || t <= m_keyframes.front().time) {
        return m_keyframes.front();
    }

    if (t >= m_keyframes.back().time) {
        return m_keyframes.back();
    }

    // Find surrounding keyframes
    size_t i = 0;
    for (; i < m_keyframes.size() - 1; ++i) {
        if (t >= m_keyframes[i].time && t <= m_keyframes[i + 1].time) {
            break;
        }
    }

    const CameraKeyframe& k1 = m_keyframes[i];
    const CameraKeyframe& k2 = m_keyframes[i + 1];

    // Linear interpolation factor
    double alpha = (t - k1.time) / (k2.time - k1.time);

    // Interpolate
    CameraKeyframe result;
    result.time = t;
    result.position = k1.position + alpha * (k2.position - k1.position);
    result.focalPoint = k1.focalPoint + alpha * (k2.focalPoint - k1.focalPoint);
    result.viewUp = k1.viewUp + alpha * (k2.viewUp - k1.viewUp);

    return result;
}

void ScreenshotManager::applyRotationAnimation(int frameIndex) {
    if (!m_renderWindow) {
        return;
    }

    auto renderer = m_renderWindow->GetRenderers()->GetFirstRenderer();
    if (!renderer) {
        return;
    }

    auto camera = renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    // Calculate rotation angle for this frame
    double progress = static_cast<double>(frameIndex) / static_cast<double>(m_animConfig.numFrames - 1);
    double angle = progress * m_animConfig.rotationDegrees;

    // Rotate camera around the focal point
    camera->SetPosition(m_initialCameraPos.data());
    camera->SetFocalPoint(m_initialFocalPoint.data());
    camera->SetViewUp(m_initialViewUp.data());

    camera->Azimuth(angle);
}

void ScreenshotManager::applyPathAnimation(int frameIndex) {
    if (!m_renderWindow || m_keyframes.empty()) {
        return;
    }

    auto renderer = m_renderWindow->GetRenderers()->GetFirstRenderer();
    if (!renderer) {
        return;
    }

    auto camera = renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    // Calculate normalized time
    double t = static_cast<double>(frameIndex) / static_cast<double>(m_animConfig.numFrames - 1);

    // Interpolate camera position
    CameraKeyframe kf = interpolateCamera(t);

    // Apply to camera
    camera->SetPosition(kf.position.data());
    camera->SetFocalPoint(kf.focalPoint.data());
    camera->SetViewUp(kf.viewUp.data());
}

void ScreenshotManager::storeInitialCamera() {
    if (!m_renderWindow) {
        return;
    }

    auto renderer = m_renderWindow->GetRenderers()->GetFirstRenderer();
    if (!renderer) {
        return;
    }

    auto camera = renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    double* pos = camera->GetPosition();
    double* focal = camera->GetFocalPoint();
    double* up = camera->GetViewUp();

    m_initialCameraPos = Eigen::Vector3d(pos[0], pos[1], pos[2]);
    m_initialFocalPoint = Eigen::Vector3d(focal[0], focal[1], focal[2]);
    m_initialViewUp = Eigen::Vector3d(up[0], up[1], up[2]);
}

void ScreenshotManager::restoreInitialCamera() {
    if (!m_renderWindow) {
        return;
    }

    auto renderer = m_renderWindow->GetRenderers()->GetFirstRenderer();
    if (!renderer) {
        return;
    }

    auto camera = renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    camera->SetPosition(m_initialCameraPos.data());
    camera->SetFocalPoint(m_initialFocalPoint.data());
    camera->SetViewUp(m_initialViewUp.data());
}

std::string ScreenshotManager::generateFrameFilename(int frameIndex) const {
    std::ostringstream oss;
    oss << m_outputDir << "/";
    oss << m_animConfig.outputPrefix;
    oss << std::setw(6) << std::setfill('0') << frameIndex;

    // Add extension based on format
    switch (m_screenshotConfig.format) {
        case ImageFormat::PNG:
            oss << ".png";
            break;
        case ImageFormat::JPEG:
            oss << ".jpg";
            break;
        case ImageFormat::BMP:
            oss << ".bmp";
            break;
        case ImageFormat::TIFF:
            oss << ".tiff";
            break;
    }

    return oss.str();
}

ImageFormat ScreenshotManager::detectFormat(const std::string& filename) const {
    // Find last dot
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) {
        return m_screenshotConfig.format;  // Default
    }

    std::string ext = filename.substr(dotPos + 1);

    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "png") {
        return ImageFormat::PNG;
    } else if (ext == "jpg" || ext == "jpeg") {
        return ImageFormat::JPEG;
    } else if (ext == "bmp") {
        return ImageFormat::BMP;
    } else if (ext == "tif" || ext == "tiff") {
        return ImageFormat::TIFF;
    }

    return m_screenshotConfig.format;  // Default
}

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
