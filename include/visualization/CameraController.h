/**
 * @file CameraController.h
 * @brief High-level camera control for VTK visualization
 */

#pragma once

#include <memory>
#include <vector>
#include <string>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Camera view presets
 */
enum class ViewPreset {
    FRONT,      // +Z direction
    BACK,       // -Z direction
    TOP,        // +Y direction
    BOTTOM,     // -Y direction
    LEFT,       // -X direction
    RIGHT,      // +X direction
    ISOMETRIC,  // Diagonal view
    CUSTOM      // User-defined view
};

/**
 * @brief Camera state for saving/restoring
 */
struct CameraState {
    double position[3] = {0.0, 0.0, 0.0};
    double focalPoint[3] = {0.0, 0.0, 0.0};
    double viewUp[3] = {0.0, 1.0, 0.0};
    double viewAngle = 30.0;
    double clippingRange[2] = {0.1, 1000.0};
    double distance = 0.0;
};

/**
 * @brief Camera animation parameters
 */
struct CameraAnimation {
    double duration = 1.0;           // Animation duration in seconds
    int steps = 30;                  // Number of interpolation steps
    bool smooth = true;              // Use smooth interpolation
    double easingPower = 2.0;        // Easing function power (for smooth)
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief High-level camera controller for VTK visualization
 *
 * Provides convenient camera control including:
 * - View presets (front, back, top, etc.)
 * - Zoom and focus operations
 * - Camera state save/restore
 * - Smooth camera transitions
 * - Custom view positioning
 */
class CameraController {
public:
    /**
     * @brief Constructor
     * @param renderer VTK renderer
     */
    explicit CameraController(vtkRenderer* renderer);

    /**
     * @brief Destructor
     */
    ~CameraController();

    // ========================================================================
    // Basic Camera Access
    // ========================================================================

    /**
     * @brief Get VTK camera
     * @return Camera pointer
     */
    vtkCamera* getCamera() const;

    /**
     * @brief Reset camera to view all objects
     */
    void reset();

    /**
     * @brief Reset camera to specific bounds
     * @param bounds Bounding box [xmin, xmax, ymin, ymax, zmin, zmax]
     */
    void reset(const double bounds[6]);

    // ========================================================================
    // View Presets
    // ========================================================================

    /**
     * @brief Set camera to preset view
     * @param preset View preset
     * @param animate Use smooth transition
     */
    void setViewPreset(ViewPreset preset, bool animate = false);

    /**
     * @brief Set front view (+Z looking at -Z)
     * @param animate Use smooth transition
     */
    void setFrontView(bool animate = false);

    /**
     * @brief Set back view (-Z looking at +Z)
     * @param animate Use smooth transition
     */
    void setBackView(bool animate = false);

    /**
     * @brief Set top view (+Y looking at -Y)
     * @param animate Use smooth transition
     */
    void setTopView(bool animate = false);

    /**
     * @brief Set bottom view (-Y looking at +Y)
     * @param animate Use smooth transition
     */
    void setBottomView(bool animate = false);

    /**
     * @brief Set left view (-X looking at +X)
     * @param animate Use smooth transition
     */
    void setLeftView(bool animate = false);

    /**
     * @brief Set right view (+X looking at -X)
     * @param animate Use smooth transition
     */
    void setRightView(bool animate = false);

    /**
     * @brief Set isometric view (diagonal)
     * @param animate Use smooth transition
     */
    void setIsometricView(bool animate = false);

    // ========================================================================
    // Camera Positioning
    // ========================================================================

    /**
     * @brief Set camera position
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    void setPosition(double x, double y, double z);

    /**
     * @brief Get camera position
     * @param position Output position array [x, y, z]
     */
    void getPosition(double position[3]) const;

    /**
     * @brief Set focal point (look-at point)
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     */
    void setFocalPoint(double x, double y, double z);

    /**
     * @brief Get focal point
     * @param focalPoint Output focal point array [x, y, z]
     */
    void getFocalPoint(double focalPoint[3]) const;

    /**
     * @brief Set view up vector
     * @param x X component
     * @param y Y component
     * @param z Z component
     */
    void setViewUp(double x, double y, double z);

    /**
     * @brief Get view up vector
     * @param viewUp Output view up array [x, y, z]
     */
    void getViewUp(double viewUp[3]) const;

    // ========================================================================
    // Zoom and Distance
    // ========================================================================

    /**
     * @brief Zoom camera by factor
     * @param factor Zoom factor (>1 zoom in, <1 zoom out)
     */
    void zoom(double factor);

    /**
     * @brief Set camera distance from focal point
     * @param distance Distance value
     */
    void setDistance(double distance);

    /**
     * @brief Get camera distance from focal point
     * @return Distance value
     */
    double getDistance() const;

    /**
     * @brief Set view angle (field of view)
     * @param angle View angle in degrees
     */
    void setViewAngle(double angle);

    /**
     * @brief Get view angle
     * @return View angle in degrees
     */
    double getViewAngle() const;

    // ========================================================================
    // Focus Operations
    // ========================================================================

    /**
     * @brief Focus camera on point
     * @param x X coordinate
     * @param y Y coordinate
     * @param z Z coordinate
     * @param animate Use smooth transition
     */
    void focusOnPoint(double x, double y, double z, bool animate = false);

    /**
     * @brief Focus camera on bounds
     * @param bounds Bounding box [xmin, xmax, ymin, ymax, zmin, zmax]
     * @param animate Use smooth transition
     */
    void focusOnBounds(const double bounds[6], bool animate = false);

    // ========================================================================
    // Camera Rotation
    // ========================================================================

    /**
     * @brief Rotate camera around focal point
     * @param azimuth Rotation around view up vector (degrees)
     * @param elevation Rotation perpendicular to view up (degrees)
     */
    void rotate(double azimuth, double elevation);

    /**
     * @brief Orbit camera around focal point
     * @param dx Horizontal rotation (degrees)
     * @param dy Vertical rotation (degrees)
     */
    void orbit(double dx, double dy);

    /**
     * @brief Roll camera (rotation around view direction)
     * @param angle Roll angle in degrees
     */
    void roll(double angle);

    // ========================================================================
    // Camera State Management
    // ========================================================================

    /**
     * @brief Save current camera state
     * @return Camera state
     */
    CameraState saveState() const;

    /**
     * @brief Restore camera state
     * @param state Camera state to restore
     * @param animate Use smooth transition
     */
    void restoreState(const CameraState& state, bool animate = false);

    /**
     * @brief Push current state to history
     */
    void pushState();

    /**
     * @brief Pop and restore previous state
     * @return True if state was restored
     */
    bool popState();

    /**
     * @brief Clear state history
     */
    void clearHistory();

    /**
     * @brief Get number of states in history
     * @return History size
     */
    size_t getHistorySize() const;

    // ========================================================================
    // Animation
    // ========================================================================

    /**
     * @brief Set animation parameters
     * @param params Animation parameters
     */
    void setAnimationParams(const CameraAnimation& params);

    /**
     * @brief Get animation parameters
     * @return Current animation parameters
     */
    const CameraAnimation& getAnimationParams() const;

    /**
     * @brief Animate camera to target state
     * @param target Target camera state
     * @return True if animation completed
     */
    bool animateToState(const CameraState& target);

    // ========================================================================
    // Clipping Range
    // ========================================================================

    /**
     * @brief Set clipping range
     * @param near Near clipping distance
     * @param far Far clipping distance
     */
    void setClippingRange(double near, double far);

    /**
     * @brief Get clipping range
     * @param range Output range array [near, far]
     */
    void getClippingRange(double range[2]) const;

    /**
     * @brief Reset clipping range to automatic
     */
    void resetClippingRange();

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Set parallel projection
     * @param enable Enable parallel projection (orthographic)
     */
    void setParallelProjection(bool enable);

    /**
     * @brief Check if using parallel projection
     * @return True if parallel projection enabled
     */
    bool isParallelProjection() const;

    /**
     * @brief Set parallel scale (for orthographic view)
     * @param scale Parallel scale
     */
    void setParallelScale(double scale);

    /**
     * @brief Get parallel scale
     * @return Parallel scale
     */
    double getParallelScale() const;

private:
    /**
     * @brief Calculate camera position for preset view
     */
    void calculatePresetPosition(
        ViewPreset preset,
        const double center[3],
        double distance,
        double position[3],
        double viewUp[3]
    );

    /**
     * @brief Interpolate between camera states
     */
    CameraState interpolateStates(
        const CameraState& from,
        const CameraState& to,
        double t
    ) const;

    /**
     * @brief Apply easing function
     */
    double easeInOut(double t, double power) const;

    vtkRenderer* m_renderer;
    CameraAnimation m_animationParams;
    std::vector<CameraState> m_stateHistory;
    size_t m_maxHistorySize;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class CameraController {
public:
    explicit CameraController(void* camera = nullptr) {
        (void)camera;
    }
    ~CameraController() {}

    void* getCamera() const { return nullptr; }
    void reset() {}
    void reset(const double[6]) {}

    void setViewPreset(ViewPreset, bool = false) {}
    void setFrontView(bool = false) {}
    void setBackView(bool = false) {}
    void setTopView(bool = false) {}
    void setBottomView(bool = false) {}
    void setLeftView(bool = false) {}
    void setRightView(bool = false) {}
    void setIsometricView(bool = false) {}

    void setPosition(double, double, double) {}
    void getPosition(double[3]) const {}
    void setFocalPoint(double, double, double) {}
    void getFocalPoint(double[3]) const {}
    void setViewUp(double, double, double) {}
    void getViewUp(double[3]) const {}

    void zoom(double) {}
    void setDistance(double) {}
    double getDistance() const { return 0.0; }
    void setViewAngle(double) {}
    double getViewAngle() const { return 30.0; }

    void focusOnPoint(double, double, double, bool = false) {}
    void focusOnBounds(const double[6], bool = false) {}

    void rotate(double, double) {}
    void orbit(double, double) {}
    void roll(double) {}

    CameraState saveState() const { return CameraState(); }
    void restoreState(const CameraState&, bool = false) {}
    void pushState() {}
    bool popState() { return false; }
    void clearHistory() {}
    size_t getHistorySize() const { return 0; }

    void setAnimationParams(const CameraAnimation&) {}
    CameraAnimation getAnimationParams() const { return CameraAnimation(); }
    bool animateToState(const CameraState&) { return false; }

    void setClippingRange(double, double) {}
    void getClippingRange(double[2]) const {}
    void resetClippingRange() {}

    void setParallelProjection(bool) {}
    bool isParallelProjection() const { return false; }
    void setParallelScale(double) {}
    double getParallelScale() const { return 1.0; }
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
