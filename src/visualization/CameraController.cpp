/**
 * @file CameraController.cpp
 * @brief Implementation of CameraController
 */

#include "visualization/CameraController.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkMath.h>
#include <cmath>
#include <algorithm>
#endif

namespace koomesh {
namespace visualization {

#ifdef KOOMESH_HAS_VTK

// ============================================================================
// Constructor / Destructor
// ============================================================================

CameraController::CameraController(vtkRenderer* renderer)
    : m_renderer(renderer)
    , m_maxHistorySize(20)
{
    if (!m_renderer) {
        throw std::invalid_argument("Renderer cannot be null");
    }
}

CameraController::~CameraController() = default;

// ============================================================================
// Basic Camera Access
// ============================================================================

vtkCamera* CameraController::getCamera() const {
    return m_renderer ? m_renderer->GetActiveCamera() : nullptr;
}

void CameraController::reset() {
    if (m_renderer) {
        m_renderer->ResetCamera();
    }
}

void CameraController::reset(const double bounds[6]) {
    if (m_renderer) {
        m_renderer->ResetCamera(bounds);
    }
}

// ============================================================================
// View Presets
// ============================================================================

void CameraController::setViewPreset(ViewPreset preset, bool animate) {
    double bounds[6];
    m_renderer->ComputeVisiblePropBounds(bounds);

    double center[3] = {
        (bounds[0] + bounds[1]) / 2.0,
        (bounds[2] + bounds[3]) / 2.0,
        (bounds[4] + bounds[5]) / 2.0
    };

    double distance = vtkMath::Distance2BetweenPoints(
        &bounds[0], &bounds[1]
    );
    distance = std::sqrt(distance) * 1.5;  // Add margin

    double position[3];
    double viewUp[3];
    calculatePresetPosition(preset, center, distance, position, viewUp);

    if (animate) {
        CameraState targetState;
        std::copy(position, position + 3, targetState.position);
        std::copy(center, center + 3, targetState.focalPoint);
        std::copy(viewUp, viewUp + 3, targetState.viewUp);
        targetState.viewAngle = getCamera()->GetViewAngle();

        animateToState(targetState);
    } else {
        vtkCamera* camera = getCamera();
        camera->SetPosition(position);
        camera->SetFocalPoint(center);
        camera->SetViewUp(viewUp);
        m_renderer->ResetCameraClippingRange();
    }
}

void CameraController::setFrontView(bool animate) {
    setViewPreset(ViewPreset::FRONT, animate);
}

void CameraController::setBackView(bool animate) {
    setViewPreset(ViewPreset::BACK, animate);
}

void CameraController::setTopView(bool animate) {
    setViewPreset(ViewPreset::TOP, animate);
}

void CameraController::setBottomView(bool animate) {
    setViewPreset(ViewPreset::BOTTOM, animate);
}

void CameraController::setLeftView(bool animate) {
    setViewPreset(ViewPreset::LEFT, animate);
}

void CameraController::setRightView(bool animate) {
    setViewPreset(ViewPreset::RIGHT, animate);
}

void CameraController::setIsometricView(bool animate) {
    setViewPreset(ViewPreset::ISOMETRIC, animate);
}

// ========================================================================
// Camera Positioning
// ========================================================================

void CameraController::setPosition(double x, double y, double z) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetPosition(x, y, z);
    }
}

void CameraController::getPosition(double position[3]) const {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->GetPosition(position);
    }
}

void CameraController::setFocalPoint(double x, double y, double z) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetFocalPoint(x, y, z);
    }
}

void CameraController::getFocalPoint(double focalPoint[3]) const {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->GetFocalPoint(focalPoint);
    }
}

void CameraController::setViewUp(double x, double y, double z) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetViewUp(x, y, z);
    }
}

void CameraController::getViewUp(double viewUp[3]) const {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->GetViewUp(viewUp);
    }
}

// ========================================================================
// Zoom and Distance
// ========================================================================

void CameraController::zoom(double factor) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->Zoom(factor);
    }
}

void CameraController::setDistance(double distance) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetDistance(distance);
    }
}

double CameraController::getDistance() const {
    vtkCamera* camera = getCamera();
    return camera ? camera->GetDistance() : 0.0;
}

void CameraController::setViewAngle(double angle) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetViewAngle(angle);
    }
}

double CameraController::getViewAngle() const {
    vtkCamera* camera = getCamera();
    return camera ? camera->GetViewAngle() : 30.0;
}

// ========================================================================
// Focus Operations
// ========================================================================

void CameraController::focusOnPoint(double x, double y, double z, bool animate) {
    if (animate) {
        CameraState targetState = saveState();
        targetState.focalPoint[0] = x;
        targetState.focalPoint[1] = y;
        targetState.focalPoint[2] = z;

        // Adjust position to maintain distance
        double currentDistance = getDistance();
        vtkCamera* camera = getCamera();
        double viewDirection[3];
        camera->GetViewPlaneNormal(viewDirection);

        targetState.position[0] = x + viewDirection[0] * currentDistance;
        targetState.position[1] = y + viewDirection[1] * currentDistance;
        targetState.position[2] = z + viewDirection[2] * currentDistance;

        animateToState(targetState);
    } else {
        setFocalPoint(x, y, z);
    }
}

void CameraController::focusOnBounds(const double bounds[6], bool animate) {
    double center[3] = {
        (bounds[0] + bounds[1]) / 2.0,
        (bounds[2] + bounds[3]) / 2.0,
        (bounds[4] + bounds[5]) / 2.0
    };

    if (animate) {
        CameraState targetState = saveState();
        targetState.focalPoint[0] = center[0];
        targetState.focalPoint[1] = center[1];
        targetState.focalPoint[2] = center[2];

        animateToState(targetState);
    } else {
        reset(bounds);
    }
}

// ========================================================================
// Camera Rotation
// ========================================================================

void CameraController::rotate(double azimuth, double elevation) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->Azimuth(azimuth);
        camera->Elevation(elevation);
        camera->OrthogonalizeViewUp();
        m_renderer->ResetCameraClippingRange();
    }
}

void CameraController::orbit(double dx, double dy) {
    rotate(dx, dy);
}

void CameraController::roll(double angle) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->Roll(angle);
    }
}

// ========================================================================
// Camera State Management
// ========================================================================

CameraState CameraController::saveState() const {
    CameraState state;

    vtkCamera* camera = getCamera();
    if (camera) {
        camera->GetPosition(state.position);
        camera->GetFocalPoint(state.focalPoint);
        camera->GetViewUp(state.viewUp);
        state.viewAngle = camera->GetViewAngle();
        camera->GetClippingRange(state.clippingRange);
        state.distance = camera->GetDistance();
    }

    return state;
}

void CameraController::restoreState(const CameraState& state, bool animate) {
    if (animate) {
        animateToState(state);
    } else {
        vtkCamera* camera = getCamera();
        if (camera) {
            camera->SetPosition(state.position);
            camera->SetFocalPoint(state.focalPoint);
            camera->SetViewUp(state.viewUp);
            camera->SetViewAngle(state.viewAngle);
            camera->SetClippingRange(state.clippingRange);
        }
    }
}

void CameraController::pushState() {
    CameraState state = saveState();
    m_stateHistory.push_back(state);

    // Limit history size
    if (m_stateHistory.size() > m_maxHistorySize) {
        m_stateHistory.erase(m_stateHistory.begin());
    }
}

bool CameraController::popState() {
    if (m_stateHistory.empty()) {
        return false;
    }

    CameraState state = m_stateHistory.back();
    m_stateHistory.pop_back();

    restoreState(state, false);
    return true;
}

void CameraController::clearHistory() {
    m_stateHistory.clear();
}

size_t CameraController::getHistorySize() const {
    return m_stateHistory.size();
}

// ========================================================================
// Animation
// ========================================================================

void CameraController::setAnimationParams(const CameraAnimation& params) {
    m_animationParams = params;
}

const CameraAnimation& CameraController::getAnimationParams() const {
    return m_animationParams;
}

bool CameraController::animateToState(const CameraState& target) {
    CameraState start = saveState();

    int steps = m_animationParams.steps;
    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(steps);

        if (m_animationParams.smooth) {
            t = easeInOut(t, m_animationParams.easingPower);
        }

        CameraState interpolated = interpolateStates(start, target, t);
        restoreState(interpolated, false);

        // In a real application, would render here and wait
        // For now, just apply the final state
        if (i == steps) {
            break;
        }
    }

    return true;
}

// ========================================================================
// Clipping Range
// ========================================================================

void CameraController::setClippingRange(double near, double far) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetClippingRange(near, far);
    }
}

void CameraController::getClippingRange(double range[2]) const {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->GetClippingRange(range);
    }
}

void CameraController::resetClippingRange() {
    if (m_renderer) {
        m_renderer->ResetCameraClippingRange();
    }
}

// ========================================================================
// Utility
// ========================================================================

void CameraController::setParallelProjection(bool enable) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetParallelProjection(enable ? 1 : 0);
    }
}

bool CameraController::isParallelProjection() const {
    vtkCamera* camera = getCamera();
    return camera ? (camera->GetParallelProjection() != 0) : false;
}

void CameraController::setParallelScale(double scale) {
    vtkCamera* camera = getCamera();
    if (camera) {
        camera->SetParallelScale(scale);
    }
}

double CameraController::getParallelScale() const {
    vtkCamera* camera = getCamera();
    return camera ? camera->GetParallelScale() : 1.0;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void CameraController::calculatePresetPosition(
    ViewPreset preset,
    const double center[3],
    double distance,
    double position[3],
    double viewUp[3]
) {
    // Default view up
    viewUp[0] = 0.0;
    viewUp[1] = 1.0;
    viewUp[2] = 0.0;

    switch (preset) {
        case ViewPreset::FRONT:
            position[0] = center[0];
            position[1] = center[1];
            position[2] = center[2] + distance;
            break;

        case ViewPreset::BACK:
            position[0] = center[0];
            position[1] = center[1];
            position[2] = center[2] - distance;
            break;

        case ViewPreset::TOP:
            position[0] = center[0];
            position[1] = center[1] + distance;
            position[2] = center[2];
            viewUp[0] = 0.0;
            viewUp[1] = 0.0;
            viewUp[2] = -1.0;  // -Z is up when looking from top
            break;

        case ViewPreset::BOTTOM:
            position[0] = center[0];
            position[1] = center[1] - distance;
            position[2] = center[2];
            viewUp[0] = 0.0;
            viewUp[1] = 0.0;
            viewUp[2] = 1.0;  // +Z is up when looking from bottom
            break;

        case ViewPreset::LEFT:
            position[0] = center[0] - distance;
            position[1] = center[1];
            position[2] = center[2];
            break;

        case ViewPreset::RIGHT:
            position[0] = center[0] + distance;
            position[1] = center[1];
            position[2] = center[2];
            break;

        case ViewPreset::ISOMETRIC:
            {
                double factor = distance / std::sqrt(3.0);
                position[0] = center[0] + factor;
                position[1] = center[1] + factor;
                position[2] = center[2] + factor;
            }
            break;

        case ViewPreset::CUSTOM:
            // Custom view - don't change position
            position[0] = center[0];
            position[1] = center[1];
            position[2] = center[2] + distance;
            break;
    }
}

CameraState CameraController::interpolateStates(
    const CameraState& from,
    const CameraState& to,
    double t
) const {
    CameraState result;

    // Linear interpolation for position, focal point, view up
    for (int i = 0; i < 3; ++i) {
        result.position[i] = from.position[i] + t * (to.position[i] - from.position[i]);
        result.focalPoint[i] = from.focalPoint[i] + t * (to.focalPoint[i] - from.focalPoint[i]);
        result.viewUp[i] = from.viewUp[i] + t * (to.viewUp[i] - from.viewUp[i]);
    }

    // Interpolate scalars
    result.viewAngle = from.viewAngle + t * (to.viewAngle - from.viewAngle);
    result.clippingRange[0] = from.clippingRange[0] + t * (to.clippingRange[0] - from.clippingRange[0]);
    result.clippingRange[1] = from.clippingRange[1] + t * (to.clippingRange[1] - from.clippingRange[1]);
    result.distance = from.distance + t * (to.distance - from.distance);

    return result;
}

double CameraController::easeInOut(double t, double power) const {
    if (t < 0.5) {
        return std::pow(2.0 * t, power) / 2.0;
    } else {
        return 1.0 - std::pow(2.0 * (1.0 - t), power) / 2.0;
    }
}

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
