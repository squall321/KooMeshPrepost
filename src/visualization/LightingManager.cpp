/**
 * @file LightingManager.cpp
 * @brief Lighting management implementation
 */

#include "visualization/LightingManager.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkProperty.h>
#include <sstream>
#include <algorithm>

namespace koomesh {
namespace visualization {

LightingManager::LightingManager(vtkRenderer* renderer)
    : m_renderer(renderer)
    , m_ambientIntensity(0.2)
    , m_ambientColor(1.0, 1.0, 1.0)
    , m_shadowsEnabled(false)
    , m_ssaoEnabled(false)
    , m_ssaoRadius(10.0)
    , m_ssaoBias(0.01)
    , m_ssaoIntensity(1.0)
{
    if (m_renderer) {
        applyAmbientSettings();
    }
}

LightingManager::~LightingManager() {
    removeAllLights();
}

// ============================================================================
// Light Management
// ============================================================================

void LightingManager::addHeadlight() {
    LightConfig config;
    config.type = LightType::HEADLIGHT;
    config.intensity = 1.0;
    config.color = {1.0, 1.0, 1.0};
    addLight(config);
}

void LightingManager::addKeyLight(const Eigen::Vector3d& position, double intensity) {
    LightConfig config;
    config.type = LightType::KEY_LIGHT;
    config.position = position;
    config.intensity = intensity;
    config.color = {1.0, 1.0, 1.0};
    addLight(config);
}

void LightingManager::addFillLight(const Eigen::Vector3d& position, double intensity) {
    LightConfig config;
    config.type = LightType::FILL_LIGHT;
    config.position = position;
    config.intensity = intensity;
    config.color = {0.9, 0.9, 1.0};  // Slightly cooler
    addLight(config);
}

void LightingManager::addBackLight(const Eigen::Vector3d& position, double intensity) {
    LightConfig config;
    config.type = LightType::BACK_LIGHT;
    config.position = position;
    config.intensity = intensity;
    config.color = {1.0, 0.95, 0.9};  // Slightly warmer
    addLight(config);
}

int LightingManager::addLight(const LightConfig& config) {
    if (!m_renderer) {
        return -1;
    }

    auto light = createLight(config);
    m_renderer->AddLight(light);
    m_lights.push_back(light);
    m_lightTypes.push_back(config.type);

    return static_cast<int>(m_lights.size()) - 1;
}

void LightingManager::removeLight(int index) {
    if (!m_renderer || index < 0 || index >= static_cast<int>(m_lights.size())) {
        return;
    }

    m_renderer->RemoveLight(m_lights[index]);
    m_lights.erase(m_lights.begin() + index);
    m_lightTypes.erase(m_lightTypes.begin() + index);
}

void LightingManager::removeAllLights() {
    if (!m_renderer) {
        return;
    }

    for (auto& light : m_lights) {
        m_renderer->RemoveLight(light);
    }

    m_lights.clear();
    m_lightTypes.clear();
}

int LightingManager::getLightCount() const {
    return static_cast<int>(m_lights.size());
}

// ============================================================================
// Light Configuration
// ============================================================================

void LightingManager::setLightPosition(int index, const Eigen::Vector3d& position) {
    if (index < 0 || index >= static_cast<int>(m_lights.size())) {
        return;
    }

    m_lights[index]->SetPosition(position.x(), position.y(), position.z());
}

void LightingManager::setLightColor(int index, const Eigen::Vector3d& color) {
    if (index < 0 || index >= static_cast<int>(m_lights.size())) {
        return;
    }

    m_lights[index]->SetColor(color.x(), color.y(), color.z());
}

void LightingManager::setLightIntensity(int index, double intensity) {
    if (index < 0 || index >= static_cast<int>(m_lights.size())) {
        return;
    }

    m_lights[index]->SetIntensity(intensity);
}

void LightingManager::setLightEnabled(int index, bool enabled) {
    if (index < 0 || index >= static_cast<int>(m_lights.size())) {
        return;
    }

    m_lights[index]->SetSwitch(enabled ? 1 : 0);
}

vtkLight* LightingManager::getLight(int index) {
    if (index < 0 || index >= static_cast<int>(m_lights.size())) {
        return nullptr;
    }

    return m_lights[index];
}

// ============================================================================
// Ambient Lighting
// ============================================================================

void LightingManager::setAmbientLight(double intensity) {
    m_ambientIntensity = std::max(0.0, std::min(1.0, intensity));
    applyAmbientSettings();
}

double LightingManager::getAmbientLight() const {
    return m_ambientIntensity;
}

void LightingManager::setAmbientColor(const Eigen::Vector3d& color) {
    m_ambientColor = color;
    applyAmbientSettings();
}

// ============================================================================
// Advanced Features
// ============================================================================

void LightingManager::enableShadows(bool enabled) {
    m_shadowsEnabled = enabled;

    // VTK shadow rendering requires additional configuration
    // This is a placeholder for shadow setup
    if (m_renderer) {
        // Shadow pass configuration would go here
        // Requires vtkShadowMapPass and related VTK classes
    }
}

bool LightingManager::areShadowsEnabled() const {
    return m_shadowsEnabled;
}

void LightingManager::enableSSAO(bool enabled) {
    m_ssaoEnabled = enabled;

    // SSAO requires post-processing pass
    // This is a placeholder for SSAO setup
    if (m_renderer) {
        // SSAO pass configuration would go here
        // Requires vtkSSAOPass (VTK 9.0+)
    }
}

bool LightingManager::isSSAOEnabled() const {
    return m_ssaoEnabled;
}

void LightingManager::setSSAOParams(double radius, double bias, double intensity) {
    m_ssaoRadius = radius;
    m_ssaoBias = bias;
    m_ssaoIntensity = intensity;

    // Apply SSAO parameters if enabled
    if (m_ssaoEnabled && m_renderer) {
        // Update SSAO pass parameters
    }
}

// ============================================================================
// Lighting Presets
// ============================================================================

void LightingManager::setupThreePointLighting() {
    removeAllLights();

    // Key light - 45 degrees to the right and above
    addKeyLight({5.0, 5.0, 5.0}, 1.0);

    // Fill light - opposite side, softer
    addFillLight({-3.0, 2.0, 3.0}, 0.5);

    // Back light - behind and above for rim lighting
    addBackLight({0.0, -3.0, 5.0}, 0.3);
}

void LightingManager::setupStudioLighting() {
    removeAllLights();

    // Main key light
    addKeyLight({10.0, 10.0, 10.0}, 1.0);

    // Fill lights on sides
    addFillLight({-8.0, 5.0, 5.0}, 0.6);
    addFillLight({8.0, -5.0, 5.0}, 0.4);

    // Back light
    addBackLight({0.0, -10.0, 8.0}, 0.4);

    // Soft ambient
    setAmbientLight(0.3);
}

void LightingManager::setupOutdoorLighting() {
    removeAllLights();

    // Sun (directional key light)
    LightConfig sunConfig;
    sunConfig.type = LightType::KEY_LIGHT;
    sunConfig.position = {20.0, 20.0, 20.0};
    sunConfig.intensity = 1.2;
    sunConfig.color = {1.0, 0.98, 0.9};  // Warm sunlight
    sunConfig.positional = false;  // Directional
    addLight(sunConfig);

    // Sky ambient
    setAmbientLight(0.4);
    setAmbientColor({0.7, 0.8, 1.0});  // Blue sky tint
}

void LightingManager::setupDefaultLighting() {
    removeAllLights();
    addHeadlight();
    setAmbientLight(0.2);
}

// ============================================================================
// Utility
// ============================================================================

void LightingManager::update() {
    if (!m_renderer) {
        return;
    }

    // Update headlights to follow camera
    auto camera = m_renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    for (size_t i = 0; i < m_lights.size(); ++i) {
        if (m_lightTypes[i] == LightType::HEADLIGHT) {
            double* camPos = camera->GetPosition();
            double* focalPoint = camera->GetFocalPoint();

            m_lights[i]->SetPosition(camPos);
            m_lights[i]->SetFocalPoint(focalPoint);
        }
    }
}

std::string LightingManager::getStatistics() const {
    std::ostringstream oss;
    oss << "Lighting Manager Statistics:\n";
    oss << "  Total Lights: " << m_lights.size() << "\n";

    int headlights = 0, keyLights = 0, fillLights = 0, backLights = 0;
    for (auto type : m_lightTypes) {
        switch (type) {
            case LightType::HEADLIGHT: headlights++; break;
            case LightType::KEY_LIGHT: keyLights++; break;
            case LightType::FILL_LIGHT: fillLights++; break;
            case LightType::BACK_LIGHT: backLights++; break;
            default: break;
        }
    }

    oss << "    Headlights: " << headlights << "\n";
    oss << "    Key Lights: " << keyLights << "\n";
    oss << "    Fill Lights: " << fillLights << "\n";
    oss << "    Back Lights: " << backLights << "\n";
    oss << "  Ambient Intensity: " << m_ambientIntensity << "\n";
    oss << "  Shadows: " << (m_shadowsEnabled ? "Enabled" : "Disabled") << "\n";
    oss << "  SSAO: " << (m_ssaoEnabled ? "Enabled" : "Disabled") << "\n";

    if (m_ssaoEnabled) {
        oss << "    Radius: " << m_ssaoRadius << "\n";
        oss << "    Bias: " << m_ssaoBias << "\n";
        oss << "    Intensity: " << m_ssaoIntensity << "\n";
    }

    return oss.str();
}

// ============================================================================
// Private Methods
// ============================================================================

vtkSmartPointer<vtkLight> LightingManager::createLight(const LightConfig& config) {
    auto light = vtkSmartPointer<vtkLight>::New();

    // Position and focal point
    light->SetPosition(config.position.x(), config.position.y(), config.position.z());
    light->SetFocalPoint(config.focalPoint.x(), config.focalPoint.y(), config.focalPoint.z());

    // Color and intensity
    light->SetColor(config.color.x(), config.color.y(), config.color.z());
    light->SetIntensity(config.intensity);

    // Positional vs directional
    light->SetPositional(config.positional ? 1 : 0);

    // Cone angle (for spotlights)
    light->SetConeAngle(config.coneAngle);

    // Special configuration for headlights
    if (config.type == LightType::HEADLIGHT) {
        light->SetLightTypeToHeadlight();
    }

    // Enable by default
    light->SetSwitch(1);

    return light;
}

void LightingManager::applyAmbientSettings() {
    if (!m_renderer) {
        return;
    }

    m_renderer->SetAmbient(
        m_ambientColor.x() * m_ambientIntensity,
        m_ambientColor.y() * m_ambientIntensity,
        m_ambientColor.z() * m_ambientIntensity
    );
}

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
