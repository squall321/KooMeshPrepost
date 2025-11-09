/**
 * @file VisualizationOptions.h
 * @brief Visualization configuration and display options
 */

#pragma once

#include <string>
#include <array>

namespace koomesh {
namespace visualization {

/**
 * @brief Display mode for mesh rendering
 */
enum class DisplayMode {
    SOLID,                  // Solid surface rendering
    WIREFRAME,              // Wireframe only
    SURFACE_WITH_EDGES,     // Surface with edge overlay
    POINTS                  // Point cloud rendering
};

/**
 * @brief Color scheme for scalar field visualization
 */
enum class ColorScheme {
    RAINBOW,                // Classic rainbow spectrum
    GRAYSCALE,              // Black to white
    HEAT,                   // Blue to red heat map
    COOL_WARM,              // Cool blue to warm red
    JET,                    // Jet colormap
    VIRIDIS,                // Perceptually uniform
    PLASMA,                 // Perceptually uniform
    INFERNO,                // Perceptually uniform
    TURBO                   // Google's Turbo colormap
};

/**
 * @brief Edge display style
 */
enum class EdgeStyle {
    SOLID,                  // Solid lines
    DASHED,                 // Dashed lines
    DOTTED                  // Dotted lines
};

/**
 * @brief Lighting model
 */
enum class LightingModel {
    FLAT,                   // Flat shading
    GOURAUD,                // Gouraud shading (vertex normals)
    PHONG                   // Phong shading (smooth)
};

/**
 * @brief Comprehensive visualization options
 *
 * This struct contains all settings for controlling mesh visualization:
 * - Display modes (solid, wireframe, etc.)
 * - Edge and node rendering
 * - Scalar field coloring
 * - Lighting and shading
 * - Material properties
 * - Camera and rendering settings
 */
struct VisualizationOptions {
    // ========================================================================
    // Display Mode
    // ========================================================================

    /// Primary display mode
    DisplayMode displayMode = DisplayMode::SOLID;

    // ========================================================================
    // Edge Rendering
    // ========================================================================

    /// Show edges on surface
    bool showEdges = true;

    /// Edge line width (pixels)
    double edgeWidth = 1.0;

    /// Edge color (RGB, 0-1 range)
    std::array<double, 3> edgeColor = {0.0, 0.0, 0.0};

    /// Edge rendering style
    EdgeStyle edgeStyle = EdgeStyle::SOLID;

    /// Edge opacity (0-1, where 1 is opaque)
    double edgeOpacity = 1.0;

    // ========================================================================
    // Node/Point Rendering
    // ========================================================================

    /// Show nodes/vertices
    bool showNodes = false;

    /// Node point size (pixels)
    double nodeSize = 3.0;

    /// Node color (RGB, 0-1 range)
    std::array<double, 3> nodeColor = {1.0, 0.0, 0.0};

    /// Node opacity (0-1)
    double nodeOpacity = 1.0;

    // ========================================================================
    // Surface/Element Rendering
    // ========================================================================

    /// Default surface color (RGB, 0-1 range)
    std::array<double, 3> surfaceColor = {0.8, 0.8, 0.8};

    /// Surface opacity (0-1)
    double surfaceOpacity = 1.0;

    /// Enable backface culling (hide back faces)
    bool cullBackfaces = false;

    // ========================================================================
    // Scalar Field Coloring
    // ========================================================================

    /// Use scalar field for coloring
    bool useScalarColoring = false;

    /// Name of scalar field to visualize
    std::string scalarFieldName;

    /// Color scheme for scalar mapping
    ColorScheme colorScheme = ColorScheme::RAINBOW;

    /// Scalar value range (min, max)
    std::array<double, 2> scalarRange = {0.0, 1.0};

    /// Auto-compute scalar range from data
    bool autoScalarRange = true;

    /// Number of discrete colors (0 = continuous)
    int discreteColorLevels = 0;

    // ========================================================================
    // Lighting
    // ========================================================================

    /// Enable lighting
    bool useLighting = true;

    /// Lighting model
    LightingModel lightingModel = LightingModel::GOURAUD;

    /// Ambient coefficient (0-1)
    double ambient = 0.2;

    /// Diffuse coefficient (0-1)
    double diffuse = 0.7;

    /// Specular coefficient (0-1)
    double specular = 0.3;

    /// Specular power (shininess)
    double specularPower = 20.0;

    /// Enable ambient occlusion
    bool useAmbientOcclusion = false;

    /// Ambient occlusion radius
    double ambientOcclusionRadius = 10.0;

    // ========================================================================
    // Transparency and Depth
    // ========================================================================

    /// Enable depth peeling for transparent rendering
    bool useDepthPeeling = false;

    /// Maximum depth peeling passes
    int maxDepthPeelingPasses = 4;

    // ========================================================================
    // Clipping Planes
    // ========================================================================

    /// Enable clipping plane
    bool useClipping = false;

    /// Clipping plane origin
    std::array<double, 3> clippingOrigin = {0.0, 0.0, 0.0};

    /// Clipping plane normal
    std::array<double, 3> clippingNormal = {0.0, 0.0, 1.0};

    // ========================================================================
    // Selection Highlighting
    // ========================================================================

    /// Highlight selected elements
    bool highlightSelection = true;

    /// Selection highlight color
    std::array<double, 3> selectionColor = {1.0, 1.0, 0.0};

    /// Selection highlight width/size multiplier
    double selectionWidthMultiplier = 2.0;

    // ========================================================================
    // Performance
    // ========================================================================

    /// Use level-of-detail (LOD) for large meshes
    bool useLOD = false;

    /// Target frame rate for LOD
    double targetFrameRate = 30.0;

    /// Enable frustum culling
    bool useFrustumCulling = true;

    // ========================================================================
    // Utility Methods
    // ========================================================================

    /**
     * @brief Reset to default values
     */
    void reset() {
        *this = VisualizationOptions();
    }

    /**
     * @brief Check if using transparency
     * @return True if any opacity is less than 1.0
     */
    bool hasTransparency() const {
        return surfaceOpacity < 1.0 || edgeOpacity < 1.0 || nodeOpacity < 1.0;
    }

    /**
     * @brief Get display mode as string
     * @return Display mode name
     */
    const char* getDisplayModeString() const {
        switch (displayMode) {
            case DisplayMode::SOLID: return "Solid";
            case DisplayMode::WIREFRAME: return "Wireframe";
            case DisplayMode::SURFACE_WITH_EDGES: return "Surface with Edges";
            case DisplayMode::POINTS: return "Points";
            default: return "Unknown";
        }
    }

    /**
     * @brief Get color scheme as string
     * @return Color scheme name
     */
    const char* getColorSchemeString() const {
        switch (colorScheme) {
            case ColorScheme::RAINBOW: return "Rainbow";
            case ColorScheme::GRAYSCALE: return "Grayscale";
            case ColorScheme::HEAT: return "Heat";
            case ColorScheme::COOL_WARM: return "Cool-Warm";
            case ColorScheme::JET: return "Jet";
            case ColorScheme::VIRIDIS: return "Viridis";
            case ColorScheme::PLASMA: return "Plasma";
            case ColorScheme::INFERNO: return "Inferno";
            case ColorScheme::TURBO: return "Turbo";
            default: return "Unknown";
        }
    }

    /**
     * @brief Get lighting model as string
     * @return Lighting model name
     */
    const char* getLightingModelString() const {
        switch (lightingModel) {
            case LightingModel::FLAT: return "Flat";
            case LightingModel::GOURAUD: return "Gouraud";
            case LightingModel::PHONG: return "Phong";
            default: return "Unknown";
        }
    }
};

/**
 * @brief Preset visualization configurations
 */
namespace Presets {

    /**
     * @brief Default solid rendering
     */
    inline VisualizationOptions defaultSolid() {
        VisualizationOptions opts;
        opts.displayMode = DisplayMode::SOLID;
        opts.showEdges = false;
        opts.useLighting = true;
        return opts;
    }

    /**
     * @brief Wireframe with edges
     */
    inline VisualizationOptions wireframe() {
        VisualizationOptions opts;
        opts.displayMode = DisplayMode::WIREFRAME;
        opts.edgeWidth = 1.0;
        opts.useLighting = false;
        return opts;
    }

    /**
     * @brief Surface with edge overlay
     */
    inline VisualizationOptions surfaceWithEdges() {
        VisualizationOptions opts;
        opts.displayMode = DisplayMode::SURFACE_WITH_EDGES;
        opts.showEdges = true;
        opts.edgeWidth = 1.0;
        opts.useLighting = true;
        return opts;
    }

    /**
     * @brief Point cloud rendering
     */
    inline VisualizationOptions pointCloud() {
        VisualizationOptions opts;
        opts.displayMode = DisplayMode::POINTS;
        opts.nodeSize = 3.0;
        opts.useLighting = false;
        return opts;
    }

    /**
     * @brief High-quality rendering
     */
    inline VisualizationOptions highQuality() {
        VisualizationOptions opts;
        opts.displayMode = DisplayMode::SOLID;
        opts.useLighting = true;
        opts.lightingModel = LightingModel::PHONG;
        opts.useAmbientOcclusion = true;
        opts.useDepthPeeling = true;
        opts.ambient = 0.1;
        opts.diffuse = 0.7;
        opts.specular = 0.5;
        opts.specularPower = 50.0;
        return opts;
    }

    /**
     * @brief Performance-optimized rendering
     */
    inline VisualizationOptions performance() {
        VisualizationOptions opts;
        opts.displayMode = DisplayMode::SOLID;
        opts.useLighting = true;
        opts.lightingModel = LightingModel::FLAT;
        opts.useAmbientOcclusion = false;
        opts.useLOD = true;
        opts.useFrustumCulling = true;
        return opts;
    }

    /**
     * @brief Scalar field visualization
     */
    inline VisualizationOptions scalarField(const std::string& fieldName) {
        VisualizationOptions opts;
        opts.displayMode = DisplayMode::SOLID;
        opts.useScalarColoring = true;
        opts.scalarFieldName = fieldName;
        opts.colorScheme = ColorScheme::VIRIDIS;
        opts.autoScalarRange = true;
        opts.useLighting = true;
        return opts;
    }

} // namespace Presets

} // namespace visualization
} // namespace koomesh
