/**
 * @file LightingManager.h
 * @brief Lighting management for 3D visualization
 */

#pragma once

#include <Eigen/Core>
#include <vector>
#include <memory>
#include <string>
#include <sstream>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkLight.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Light types
 */
enum class LightType {
    HEADLIGHT,      // Camera-attached light
    KEY_LIGHT,      // Primary scene light
    FILL_LIGHT,     // Secondary fill light
    BACK_LIGHT,     // Backlight for rim lighting
    AMBIENT         // Ambient/environmental light
};

/**
 * @brief Light configuration
 */
struct LightConfig {
    LightType type = LightType::KEY_LIGHT;
    Eigen::Vector3d position = {0.0, 0.0, 1.0};
    Eigen::Vector3d focalPoint = {0.0, 0.0, 0.0};
    Eigen::Vector3d color = {1.0, 1.0, 1.0};
    double intensity = 1.0;
    double coneAngle = 30.0;      // For spotlights
    bool positional = true;        // Positional vs directional
    bool shadowEnabled = false;
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Lighting manager for VTK-based visualization
 *
 * Provides comprehensive lighting control:
 * - Multiple light types (headlight, key, fill, back)
 * - Light positioning and configuration
 * - Ambient lighting control
 * - Shadow support
 * - Screen-space ambient occlusion (SSAO)
 * - Three-point lighting presets
 */
class LightingManager {
public:
    /**
     * @brief Constructor
     * @param renderer VTK renderer to attach lights to
     */
    explicit LightingManager(vtkRenderer* renderer);

    /**
     * @brief Destructor
     */
    ~LightingManager();

    // ========================================================================
    // Light Management
    // ========================================================================

    /**
     * @brief Add headlight (camera-attached)
     *
     * Headlight follows the camera and illuminates from viewpoint
     */
    void addHeadlight();

    /**
     * @brief Add key light (primary light)
     * @param position Light position in world coordinates
     * @param intensity Light intensity (0-1)
     */
    void addKeyLight(const Eigen::Vector3d& position, double intensity = 1.0);

    /**
     * @brief Add fill light (secondary light)
     * @param position Light position in world coordinates
     * @param intensity Light intensity (0-1)
     */
    void addFillLight(const Eigen::Vector3d& position, double intensity = 0.5);

    /**
     * @brief Add back light (rim light)
     * @param position Light position in world coordinates
     * @param intensity Light intensity (0-1)
     */
    void addBackLight(const Eigen::Vector3d& position, double intensity = 0.3);

    /**
     * @brief Add custom light
     * @param config Light configuration
     * @return Index of added light
     */
    int addLight(const LightConfig& config);

    /**
     * @brief Remove light by index
     * @param index Light index
     */
    void removeLight(int index);

    /**
     * @brief Remove all lights
     */
    void removeAllLights();

    /**
     * @brief Get number of lights
     * @return Number of lights
     */
    int getLightCount() const;

    // ========================================================================
    // Light Configuration
    // ========================================================================

    /**
     * @brief Set light position
     * @param index Light index
     * @param position Position in world coordinates
     */
    void setLightPosition(int index, const Eigen::Vector3d& position);

    /**
     * @brief Set light color
     * @param index Light index
     * @param color RGB color (0-1 range)
     */
    void setLightColor(int index, const Eigen::Vector3d& color);

    /**
     * @brief Set light intensity
     * @param index Light index
     * @param intensity Intensity (0-1)
     */
    void setLightIntensity(int index, double intensity);

    /**
     * @brief Enable/disable light
     * @param index Light index
     * @param enabled Enable flag
     */
    void setLightEnabled(int index, bool enabled);

    /**
     * @brief Get light by index
     * @param index Light index
     * @return VTK light pointer (nullptr if invalid)
     */
    vtkLight* getLight(int index);

    // ========================================================================
    // Ambient Lighting
    // ========================================================================

    /**
     * @brief Set ambient light intensity
     * @param intensity Ambient intensity (0-1)
     */
    void setAmbientLight(double intensity);

    /**
     * @brief Get ambient light intensity
     * @return Ambient intensity
     */
    double getAmbientLight() const;

    /**
     * @brief Set ambient light color
     * @param color RGB color (0-1 range)
     */
    void setAmbientColor(const Eigen::Vector3d& color);

    // ========================================================================
    // Advanced Features
    // ========================================================================

    /**
     * @brief Enable/disable shadows
     * @param enabled Shadow flag
     *
     * Note: Shadow rendering requires VTK shadow mapping support
     */
    void enableShadows(bool enabled);

    /**
     * @brief Check if shadows are enabled
     * @return True if shadows enabled
     */
    bool areShadowsEnabled() const;

    /**
     * @brief Enable/disable SSAO (Screen-Space Ambient Occlusion)
     * @param enabled SSAO flag
     */
    void enableSSAO(bool enabled);

    /**
     * @brief Check if SSAO is enabled
     * @return True if SSAO enabled
     */
    bool isSSAOEnabled() const;

    /**
     * @brief Set SSAO parameters
     * @param radius SSAO radius
     * @param bias SSAO bias
     * @param intensity SSAO intensity
     */
    void setSSAOParams(double radius, double bias, double intensity);

    // ========================================================================
    // Lighting Presets
    // ========================================================================

    /**
     * @brief Setup three-point lighting
     *
     * Classic three-point lighting setup:
     * - Key light (main, 45° from camera)
     * - Fill light (softer, opposite side)
     * - Back light (rim lighting from behind)
     */
    void setupThreePointLighting();

    /**
     * @brief Setup studio lighting
     *
     * Professional studio lighting with multiple soft lights
     */
    void setupStudioLighting();

    /**
     * @brief Setup outdoor lighting
     *
     * Simulates natural outdoor lighting with sun and sky
     */
    void setupOutdoorLighting();

    /**
     * @brief Setup default headlight only
     */
    void setupDefaultLighting();

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Update lights (call after camera changes)
     */
    void update();

    /**
     * @brief Get statistics
     * @return Lighting statistics string
     */
    std::string getStatistics() const;

private:
    vtkRenderer* m_renderer;
    std::vector<vtkSmartPointer<vtkLight>> m_lights;
    std::vector<LightType> m_lightTypes;

    double m_ambientIntensity;
    Eigen::Vector3d m_ambientColor;
    bool m_shadowsEnabled;
    bool m_ssaoEnabled;
    double m_ssaoRadius;
    double m_ssaoBias;
    double m_ssaoIntensity;

    /**
     * @brief Create VTK light from config
     */
    vtkSmartPointer<vtkLight> createLight(const LightConfig& config);

    /**
     * @brief Apply ambient settings to renderer
     */
    void applyAmbientSettings();
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class LightingManager {
public:
    explicit LightingManager(void* renderer)
        : m_renderer(renderer)
        , m_ambientIntensity(0.2)
        , m_ambientColor(1.0, 1.0, 1.0)
        , m_shadowsEnabled(false)
        , m_ssaoEnabled(false)
        , m_lightCount(0)
    {}

    ~LightingManager() {}

    void addHeadlight() { m_lightCount++; }
    void addKeyLight(const Eigen::Vector3d&, double = 1.0) { m_lightCount++; }
    void addFillLight(const Eigen::Vector3d&, double = 0.5) { m_lightCount++; }
    void addBackLight(const Eigen::Vector3d&, double = 0.3) { m_lightCount++; }
    int addLight(const LightConfig&) { return m_lightCount++; }
    void removeLight(int index) {
        if (index >= 0 && index < m_lightCount && m_lightCount > 0) {
            m_lightCount--;
        }
    }
    void removeAllLights() { m_lightCount = 0; }
    int getLightCount() const { return m_lightCount; }

    void setLightPosition(int, const Eigen::Vector3d&) {}
    void setLightColor(int, const Eigen::Vector3d&) {}
    void setLightIntensity(int, double) {}
    void setLightEnabled(int, bool) {}
    void* getLight(int) { return nullptr; }

    void setAmbientLight(double intensity) {
        m_ambientIntensity = std::max(0.0, std::min(1.0, intensity));
    }
    double getAmbientLight() const { return m_ambientIntensity; }
    void setAmbientColor(const Eigen::Vector3d& color) { m_ambientColor = color; }

    void enableShadows(bool enabled) { m_shadowsEnabled = enabled; }
    bool areShadowsEnabled() const { return m_shadowsEnabled; }
    void enableSSAO(bool enabled) { m_ssaoEnabled = enabled; }
    bool isSSAOEnabled() const { return m_ssaoEnabled; }
    void setSSAOParams(double, double, double) {}

    void setupThreePointLighting() {
        removeAllLights();
        addKeyLight({1.0, 1.0, 1.0});
        addFillLight({-1.0, 0.5, 0.5});
        addBackLight({0.0, -1.0, 1.0});
    }

    void setupStudioLighting() {
        removeAllLights();
        addKeyLight({2.0, 2.0, 2.0});
        addFillLight({-2.0, 1.0, 1.0});
        addBackLight({0.0, -2.0, 2.0});
        addHeadlight();
    }

    void setupOutdoorLighting() {
        removeAllLights();
        addKeyLight({10.0, 10.0, 10.0}, 1.0);
        setAmbientLight(0.4);
    }

    void setupDefaultLighting() {
        removeAllLights();
        addHeadlight();
    }

    void update() {}

    std::string getStatistics() const {
        std::ostringstream oss;
        oss << "Lighting Manager Statistics (VTK not available):\n";
        oss << "  Lights: " << m_lightCount << "\n";
        oss << "  Ambient Intensity: " << m_ambientIntensity << "\n";
        oss << "  Shadows: " << (m_shadowsEnabled ? "Enabled" : "Disabled") << "\n";
        oss << "  SSAO: " << (m_ssaoEnabled ? "Enabled" : "Disabled") << "\n";
        return oss.str();
    }

private:
    void* m_renderer;
    double m_ambientIntensity;
    Eigen::Vector3d m_ambientColor;
    bool m_shadowsEnabled;
    bool m_ssaoEnabled;
    int m_lightCount;
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
