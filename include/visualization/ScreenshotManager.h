/**
 * @file ScreenshotManager.h
 * @brief Screenshot and animation recording for VTK renderer
 */

#pragma once

#include <Eigen/Core>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkBMPWriter.h>
#include <vtkTIFFWriter.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Image file format types
 */
enum class ImageFormat {
    PNG,         // Portable Network Graphics (lossless)
    JPEG,        // Joint Photographic Experts Group (lossy)
    BMP,         // Bitmap (uncompressed)
    TIFF         // Tagged Image File Format (lossless)
};

/**
 * @brief Animation recording mode
 */
enum class AnimationMode {
    MANUAL,           // Manually add frames
    AUTO_ROTATE,      // Automatic rotation around object
    AUTO_PATH,        // Follow predefined camera path
    CUSTOM_CALLBACK   // User-provided callback function
};

/**
 * @brief Screenshot configuration
 */
struct ScreenshotConfig {
    ImageFormat format = ImageFormat::PNG;
    int width = 1920;           // Image width in pixels
    int height = 1080;          // Image height in pixels
    int quality = 95;           // JPEG quality (0-100)
    bool transparent = false;   // Transparent background (PNG only)
    int magnification = 1;      // Resolution multiplier
};

/**
 * @brief Animation configuration
 */
struct AnimationConfig {
    AnimationMode mode = AnimationMode::MANUAL;
    int fps = 30;                        // Frames per second
    int numFrames = 60;                  // Total number of frames
    double rotationDegrees = 360.0;      // For AUTO_ROTATE mode
    bool loop = false;                   // Loop animation
    std::string outputPrefix = "frame_"; // Output filename prefix
};

/**
 * @brief Camera keyframe for path animation
 */
struct CameraKeyframe {
    Eigen::Vector3d position;
    Eigen::Vector3d focalPoint;
    Eigen::Vector3d viewUp;
    double time;  // Normalized time (0-1)
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Screenshot and animation manager
 *
 * Provides functionality for:
 * - High-resolution screenshot capture
 * - Frame-by-frame animation recording
 * - Multiple image format support
 * - Automatic camera animation
 * - Custom animation callbacks
 */
class ScreenshotManager {
public:
    /**
     * @brief Constructor
     * @param renderWindow VTK render window to capture
     */
    explicit ScreenshotManager(vtkRenderWindow* renderWindow);

    /**
     * @brief Destructor
     */
    ~ScreenshotManager();

    // ========================================================================
    // Screenshot Capture
    // ========================================================================

    /**
     * @brief Capture screenshot with current settings
     * @param filename Output filename (extension determines format if not set in config)
     * @return True if successful
     */
    bool captureScreenshot(const std::string& filename);

    /**
     * @brief Capture screenshot with specific configuration
     * @param filename Output filename
     * @param config Screenshot configuration
     * @return True if successful
     */
    bool captureScreenshot(const std::string& filename, const ScreenshotConfig& config);

    /**
     * @brief Capture screenshot at higher resolution
     * @param filename Output filename
     * @param magnification Resolution multiplier (2 = 2x resolution)
     * @return True if successful
     */
    bool captureHighResScreenshot(const std::string& filename, int magnification = 2);

    /**
     * @brief Set default screenshot configuration
     * @param config Configuration settings
     */
    void setScreenshotConfig(const ScreenshotConfig& config);

    /**
     * @brief Get current screenshot configuration
     * @return Current configuration
     */
    const ScreenshotConfig& getScreenshotConfig() const;

    // ========================================================================
    // Animation Recording
    // ========================================================================

    /**
     * @brief Start animation recording
     * @param outputDir Output directory for frames
     * @param config Animation configuration
     * @return True if started successfully
     */
    bool startAnimation(const std::string& outputDir, const AnimationConfig& config);

    /**
     * @brief Stop animation recording
     */
    void stopAnimation();

    /**
     * @brief Check if animation is currently recording
     * @return True if recording
     */
    bool isRecording() const;

    /**
     * @brief Record next frame (for MANUAL mode)
     * @return True if frame captured
     */
    bool recordFrame();

    /**
     * @brief Record all frames automatically
     * @return True if successful
     */
    bool recordAllFrames();

    /**
     * @brief Set animation configuration
     * @param config Animation configuration
     */
    void setAnimationConfig(const AnimationConfig& config);

    /**
     * @brief Get current animation configuration
     * @return Current configuration
     */
    const AnimationConfig& getAnimationConfig() const;

    // ========================================================================
    // Camera Animation
    // ========================================================================

    /**
     * @brief Add camera keyframe for path animation
     * @param keyframe Camera keyframe
     */
    void addCameraKeyframe(const CameraKeyframe& keyframe);

    /**
     * @brief Clear all camera keyframes
     */
    void clearCameraKeyframes();

    /**
     * @brief Get number of camera keyframes
     * @return Number of keyframes
     */
    int getCameraKeyframeCount() const;

    /**
     * @brief Set camera position for current frame
     * @param frameIndex Frame index
     */
    void setCameraForFrame(int frameIndex);

    /**
     * @brief Set custom animation callback
     * @param callback Function called for each frame (frame index parameter)
     *
     * The callback should update the scene/camera for the given frame index
     */
    void setAnimationCallback(std::function<void(int)> callback);

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Get current frame index
     * @return Current frame index
     */
    int getCurrentFrame() const;

    /**
     * @brief Get total number of frames
     * @return Total frames
     */
    int getTotalFrames() const;

    /**
     * @brief Get recording progress (0-1)
     * @return Progress value
     */
    double getProgress() const;

    /**
     * @brief Get statistics
     * @return Statistics string
     */
    std::string getStatistics() const;

private:
    vtkRenderWindow* m_renderWindow;
    vtkSmartPointer<vtkWindowToImageFilter> m_windowToImage;

    ScreenshotConfig m_screenshotConfig;
    AnimationConfig m_animConfig;

    bool m_isRecording;
    int m_currentFrame;
    std::string m_outputDir;
    std::vector<CameraKeyframe> m_keyframes;
    std::function<void(int)> m_animCallback;

    Eigen::Vector3d m_initialCameraPos;
    Eigen::Vector3d m_initialFocalPoint;
    Eigen::Vector3d m_initialViewUp;

    /**
     * @brief Save image to file
     * @param filename Output filename
     * @param format Image format
     * @param quality JPEG quality (0-100)
     * @return True if successful
     */
    bool saveImage(const std::string& filename, ImageFormat format, int quality);

    /**
     * @brief Interpolate camera between keyframes
     * @param t Normalized time (0-1)
     * @return Interpolated keyframe
     */
    CameraKeyframe interpolateCamera(double t) const;

    /**
     * @brief Apply rotation animation
     * @param frameIndex Current frame index
     */
    void applyRotationAnimation(int frameIndex);

    /**
     * @brief Apply path animation
     * @param frameIndex Current frame index
     */
    void applyPathAnimation(int frameIndex);

    /**
     * @brief Store initial camera state
     */
    void storeInitialCamera();

    /**
     * @brief Restore initial camera state
     */
    void restoreInitialCamera();

    /**
     * @brief Generate frame filename
     * @param frameIndex Frame index
     * @return Filename
     */
    std::string generateFrameFilename(int frameIndex) const;

    /**
     * @brief Detect format from filename extension
     * @param filename Filename with extension
     * @return Detected format
     */
    ImageFormat detectFormat(const std::string& filename) const;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class ScreenshotManager {
public:
    explicit ScreenshotManager(void* renderWindow = nullptr) : m_renderWindow(renderWindow), m_isRecording(false), m_currentFrame(0) {}
    ~ScreenshotManager() {}

    bool captureScreenshot(const std::string& filename) {
        (void)filename;
        return false;
    }

    bool captureScreenshot(const std::string& filename, const ScreenshotConfig& config) {
        (void)filename;
        (void)config;
        return false;
    }

    bool captureHighResScreenshot(const std::string& filename, int magnification = 2) {
        (void)filename;
        (void)magnification;
        return false;
    }

    void setScreenshotConfig(const ScreenshotConfig& config) {
        m_screenshotConfig = config;
    }

    const ScreenshotConfig& getScreenshotConfig() const {
        return m_screenshotConfig;
    }

    bool startAnimation(const std::string& outputDir, const AnimationConfig& config) {
        (void)outputDir;
        m_animConfig = config;
        m_isRecording = true;
        m_currentFrame = 0;
        return true;
    }

    void stopAnimation() {
        m_isRecording = false;
    }

    bool isRecording() const {
        return m_isRecording;
    }

    bool recordFrame() {
        if (!m_isRecording) return false;
        m_currentFrame++;
        if (m_currentFrame >= m_animConfig.numFrames) {
            if (m_animConfig.loop) {
                m_currentFrame = 0;
            } else {
                m_isRecording = false;
            }
        }
        return true;
    }

    bool recordAllFrames() {
        if (!m_isRecording) return false;
        m_currentFrame = m_animConfig.numFrames;
        m_isRecording = false;
        return true;
    }

    void setAnimationConfig(const AnimationConfig& config) {
        m_animConfig = config;
    }

    const AnimationConfig& getAnimationConfig() const {
        return m_animConfig;
    }

    void addCameraKeyframe(const CameraKeyframe& keyframe) {
        m_keyframes.push_back(keyframe);
    }

    void clearCameraKeyframes() {
        m_keyframes.clear();
    }

    int getCameraKeyframeCount() const {
        return static_cast<int>(m_keyframes.size());
    }

    void setCameraForFrame(int frameIndex) {
        (void)frameIndex;
    }

    void setAnimationCallback(std::function<void(int)> callback) {
        (void)callback;
    }

    int getCurrentFrame() const {
        return m_currentFrame;
    }

    int getTotalFrames() const {
        return m_animConfig.numFrames;
    }

    double getProgress() const {
        if (m_animConfig.numFrames == 0) return 0.0;
        return static_cast<double>(m_currentFrame) / static_cast<double>(m_animConfig.numFrames);
    }

    std::string getStatistics() const {
        std::ostringstream oss;
        oss << "Screenshot Manager Statistics (VTK not available):\n";
        oss << "  Is Recording: " << (m_isRecording ? "Yes" : "No") << "\n";
        oss << "  Current Frame: " << m_currentFrame << "\n";
        oss << "  Total Frames: " << m_animConfig.numFrames << "\n";
        return oss.str();
    }

private:
    void* m_renderWindow;
    ScreenshotConfig m_screenshotConfig;
    AnimationConfig m_animConfig;
    bool m_isRecording;
    int m_currentFrame;
    std::vector<CameraKeyframe> m_keyframes;
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
