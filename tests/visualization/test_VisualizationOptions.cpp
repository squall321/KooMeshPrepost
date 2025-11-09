/**
 * @file test_VisualizationOptions.cpp
 * @brief Unit tests for VisualizationOptions
 */

#include <gtest/gtest.h>
#include "visualization/VisualizationOptions.h"

using namespace koomesh::visualization;

class VisualizationOptionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        options = VisualizationOptions();
    }

    VisualizationOptions options;
};

// ============================================================================
// Default Values Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, DefaultValues) {
    EXPECT_EQ(options.displayMode, DisplayMode::SOLID);
    EXPECT_TRUE(options.showEdges);
    EXPECT_DOUBLE_EQ(options.edgeWidth, 1.0);
    EXPECT_FALSE(options.showNodes);
    EXPECT_DOUBLE_EQ(options.nodeSize, 3.0);
    EXPECT_FALSE(options.useScalarColoring);
    EXPECT_TRUE(options.useLighting);
    EXPECT_FALSE(options.useAmbientOcclusion);
}

TEST_F(VisualizationOptionsTest, DefaultColors) {
    // Edge color (black)
    EXPECT_DOUBLE_EQ(options.edgeColor[0], 0.0);
    EXPECT_DOUBLE_EQ(options.edgeColor[1], 0.0);
    EXPECT_DOUBLE_EQ(options.edgeColor[2], 0.0);

    // Node color (red)
    EXPECT_DOUBLE_EQ(options.nodeColor[0], 1.0);
    EXPECT_DOUBLE_EQ(options.nodeColor[1], 0.0);
    EXPECT_DOUBLE_EQ(options.nodeColor[2], 0.0);

    // Surface color (light gray)
    EXPECT_DOUBLE_EQ(options.surfaceColor[0], 0.8);
    EXPECT_DOUBLE_EQ(options.surfaceColor[1], 0.8);
    EXPECT_DOUBLE_EQ(options.surfaceColor[2], 0.8);
}

TEST_F(VisualizationOptionsTest, DefaultLighting) {
    EXPECT_EQ(options.lightingModel, LightingModel::GOURAUD);
    EXPECT_DOUBLE_EQ(options.ambient, 0.2);
    EXPECT_DOUBLE_EQ(options.diffuse, 0.7);
    EXPECT_DOUBLE_EQ(options.specular, 0.3);
    EXPECT_DOUBLE_EQ(options.specularPower, 20.0);
}

// ============================================================================
// Display Mode Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, SetDisplayMode) {
    options.displayMode = DisplayMode::WIREFRAME;
    EXPECT_EQ(options.displayMode, DisplayMode::WIREFRAME);

    options.displayMode = DisplayMode::SURFACE_WITH_EDGES;
    EXPECT_EQ(options.displayMode, DisplayMode::SURFACE_WITH_EDGES);

    options.displayMode = DisplayMode::POINTS;
    EXPECT_EQ(options.displayMode, DisplayMode::POINTS);
}

TEST_F(VisualizationOptionsTest, GetDisplayModeString) {
    options.displayMode = DisplayMode::SOLID;
    EXPECT_STREQ(options.getDisplayModeString(), "Solid");

    options.displayMode = DisplayMode::WIREFRAME;
    EXPECT_STREQ(options.getDisplayModeString(), "Wireframe");

    options.displayMode = DisplayMode::SURFACE_WITH_EDGES;
    EXPECT_STREQ(options.getDisplayModeString(), "Surface with Edges");

    options.displayMode = DisplayMode::POINTS;
    EXPECT_STREQ(options.getDisplayModeString(), "Points");
}

// ============================================================================
// Edge Rendering Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, EdgeSettings) {
    options.showEdges = false;
    EXPECT_FALSE(options.showEdges);

    options.edgeWidth = 2.5;
    EXPECT_DOUBLE_EQ(options.edgeWidth, 2.5);

    options.edgeColor = {1.0, 0.0, 0.0};
    EXPECT_DOUBLE_EQ(options.edgeColor[0], 1.0);
    EXPECT_DOUBLE_EQ(options.edgeColor[1], 0.0);
    EXPECT_DOUBLE_EQ(options.edgeColor[2], 0.0);

    options.edgeOpacity = 0.5;
    EXPECT_DOUBLE_EQ(options.edgeOpacity, 0.5);
}

TEST_F(VisualizationOptionsTest, EdgeStyle) {
    options.edgeStyle = EdgeStyle::DASHED;
    EXPECT_EQ(options.edgeStyle, EdgeStyle::DASHED);

    options.edgeStyle = EdgeStyle::DOTTED;
    EXPECT_EQ(options.edgeStyle, EdgeStyle::DOTTED);
}

// ============================================================================
// Node Rendering Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, NodeSettings) {
    options.showNodes = true;
    EXPECT_TRUE(options.showNodes);

    options.nodeSize = 5.0;
    EXPECT_DOUBLE_EQ(options.nodeSize, 5.0);

    options.nodeColor = {0.0, 1.0, 0.0};
    EXPECT_DOUBLE_EQ(options.nodeColor[0], 0.0);
    EXPECT_DOUBLE_EQ(options.nodeColor[1], 1.0);
    EXPECT_DOUBLE_EQ(options.nodeColor[2], 0.0);

    options.nodeOpacity = 0.8;
    EXPECT_DOUBLE_EQ(options.nodeOpacity, 0.8);
}

// ============================================================================
// Scalar Coloring Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, ScalarColoring) {
    options.useScalarColoring = true;
    options.scalarFieldName = "stress";
    options.colorScheme = ColorScheme::HEAT;

    EXPECT_TRUE(options.useScalarColoring);
    EXPECT_EQ(options.scalarFieldName, "stress");
    EXPECT_EQ(options.colorScheme, ColorScheme::HEAT);
}

TEST_F(VisualizationOptionsTest, ScalarRange) {
    options.scalarRange = {-100.0, 100.0};
    EXPECT_DOUBLE_EQ(options.scalarRange[0], -100.0);
    EXPECT_DOUBLE_EQ(options.scalarRange[1], 100.0);

    options.autoScalarRange = false;
    EXPECT_FALSE(options.autoScalarRange);
}

TEST_F(VisualizationOptionsTest, GetColorSchemeString) {
    options.colorScheme = ColorScheme::RAINBOW;
    EXPECT_STREQ(options.getColorSchemeString(), "Rainbow");

    options.colorScheme = ColorScheme::VIRIDIS;
    EXPECT_STREQ(options.getColorSchemeString(), "Viridis");

    options.colorScheme = ColorScheme::HEAT;
    EXPECT_STREQ(options.getColorSchemeString(), "Heat");
}

// ============================================================================
// Lighting Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, LightingSettings) {
    options.useLighting = false;
    EXPECT_FALSE(options.useLighting);

    options.lightingModel = LightingModel::PHONG;
    EXPECT_EQ(options.lightingModel, LightingModel::PHONG);

    options.ambient = 0.3;
    options.diffuse = 0.6;
    options.specular = 0.5;
    options.specularPower = 50.0;

    EXPECT_DOUBLE_EQ(options.ambient, 0.3);
    EXPECT_DOUBLE_EQ(options.diffuse, 0.6);
    EXPECT_DOUBLE_EQ(options.specular, 0.5);
    EXPECT_DOUBLE_EQ(options.specularPower, 50.0);
}

TEST_F(VisualizationOptionsTest, GetLightingModelString) {
    options.lightingModel = LightingModel::FLAT;
    EXPECT_STREQ(options.getLightingModelString(), "Flat");

    options.lightingModel = LightingModel::GOURAUD;
    EXPECT_STREQ(options.getLightingModelString(), "Gouraud");

    options.lightingModel = LightingModel::PHONG;
    EXPECT_STREQ(options.getLightingModelString(), "Phong");
}

TEST_F(VisualizationOptionsTest, AmbientOcclusion) {
    options.useAmbientOcclusion = true;
    options.ambientOcclusionRadius = 15.0;

    EXPECT_TRUE(options.useAmbientOcclusion);
    EXPECT_DOUBLE_EQ(options.ambientOcclusionRadius, 15.0);
}

// ============================================================================
// Transparency Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, HasTransparency) {
    // Default - no transparency
    EXPECT_FALSE(options.hasTransparency());

    // Surface transparency
    options.surfaceOpacity = 0.5;
    EXPECT_TRUE(options.hasTransparency());

    // Reset and test edge transparency
    options.reset();
    options.edgeOpacity = 0.7;
    EXPECT_TRUE(options.hasTransparency());

    // Reset and test node transparency
    options.reset();
    options.nodeOpacity = 0.9;
    EXPECT_TRUE(options.hasTransparency());
}

TEST_F(VisualizationOptionsTest, DepthPeeling) {
    options.useDepthPeeling = true;
    options.maxDepthPeelingPasses = 8;

    EXPECT_TRUE(options.useDepthPeeling);
    EXPECT_EQ(options.maxDepthPeelingPasses, 8);
}

// ============================================================================
// Clipping Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, ClippingPlane) {
    options.useClipping = true;
    options.clippingOrigin = {1.0, 2.0, 3.0};
    options.clippingNormal = {0.0, 1.0, 0.0};

    EXPECT_TRUE(options.useClipping);
    EXPECT_DOUBLE_EQ(options.clippingOrigin[0], 1.0);
    EXPECT_DOUBLE_EQ(options.clippingOrigin[1], 2.0);
    EXPECT_DOUBLE_EQ(options.clippingOrigin[2], 3.0);
    EXPECT_DOUBLE_EQ(options.clippingNormal[0], 0.0);
    EXPECT_DOUBLE_EQ(options.clippingNormal[1], 1.0);
    EXPECT_DOUBLE_EQ(options.clippingNormal[2], 0.0);
}

// ============================================================================
// Selection Highlighting Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, SelectionHighlight) {
    options.highlightSelection = false;
    EXPECT_FALSE(options.highlightSelection);

    options.selectionColor = {1.0, 0.5, 0.0};
    EXPECT_DOUBLE_EQ(options.selectionColor[0], 1.0);
    EXPECT_DOUBLE_EQ(options.selectionColor[1], 0.5);
    EXPECT_DOUBLE_EQ(options.selectionColor[2], 0.0);

    options.selectionWidthMultiplier = 3.0;
    EXPECT_DOUBLE_EQ(options.selectionWidthMultiplier, 3.0);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, PerformanceSettings) {
    options.useLOD = true;
    options.targetFrameRate = 60.0;
    options.useFrustumCulling = false;

    EXPECT_TRUE(options.useLOD);
    EXPECT_DOUBLE_EQ(options.targetFrameRate, 60.0);
    EXPECT_FALSE(options.useFrustumCulling);
}

// ============================================================================
// Reset Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, Reset) {
    // Modify all settings
    options.displayMode = DisplayMode::WIREFRAME;
    options.showEdges = false;
    options.useLighting = false;
    options.scalarFieldName = "test";

    // Reset
    options.reset();

    // Verify back to defaults
    EXPECT_EQ(options.displayMode, DisplayMode::SOLID);
    EXPECT_TRUE(options.showEdges);
    EXPECT_TRUE(options.useLighting);
    EXPECT_TRUE(options.scalarFieldName.empty());
}

// ============================================================================
// Preset Tests
// ============================================================================

TEST_F(VisualizationOptionsTest, PresetDefaultSolid) {
    auto preset = Presets::defaultSolid();
    EXPECT_EQ(preset.displayMode, DisplayMode::SOLID);
    EXPECT_FALSE(preset.showEdges);
    EXPECT_TRUE(preset.useLighting);
}

TEST_F(VisualizationOptionsTest, PresetWireframe) {
    auto preset = Presets::wireframe();
    EXPECT_EQ(preset.displayMode, DisplayMode::WIREFRAME);
    EXPECT_DOUBLE_EQ(preset.edgeWidth, 1.0);
    EXPECT_FALSE(preset.useLighting);
}

TEST_F(VisualizationOptionsTest, PresetSurfaceWithEdges) {
    auto preset = Presets::surfaceWithEdges();
    EXPECT_EQ(preset.displayMode, DisplayMode::SURFACE_WITH_EDGES);
    EXPECT_TRUE(preset.showEdges);
    EXPECT_DOUBLE_EQ(preset.edgeWidth, 1.0);
    EXPECT_TRUE(preset.useLighting);
}

TEST_F(VisualizationOptionsTest, PresetPointCloud) {
    auto preset = Presets::pointCloud();
    EXPECT_EQ(preset.displayMode, DisplayMode::POINTS);
    EXPECT_DOUBLE_EQ(preset.nodeSize, 3.0);
    EXPECT_FALSE(preset.useLighting);
}

TEST_F(VisualizationOptionsTest, PresetHighQuality) {
    auto preset = Presets::highQuality();
    EXPECT_EQ(preset.displayMode, DisplayMode::SOLID);
    EXPECT_TRUE(preset.useLighting);
    EXPECT_EQ(preset.lightingModel, LightingModel::PHONG);
    EXPECT_TRUE(preset.useAmbientOcclusion);
    EXPECT_TRUE(preset.useDepthPeeling);
}

TEST_F(VisualizationOptionsTest, PresetPerformance) {
    auto preset = Presets::performance();
    EXPECT_EQ(preset.displayMode, DisplayMode::SOLID);
    EXPECT_TRUE(preset.useLighting);
    EXPECT_EQ(preset.lightingModel, LightingModel::FLAT);
    EXPECT_FALSE(preset.useAmbientOcclusion);
    EXPECT_TRUE(preset.useLOD);
    EXPECT_TRUE(preset.useFrustumCulling);
}

TEST_F(VisualizationOptionsTest, PresetScalarField) {
    auto preset = Presets::scalarField("temperature");
    EXPECT_EQ(preset.displayMode, DisplayMode::SOLID);
    EXPECT_TRUE(preset.useScalarColoring);
    EXPECT_EQ(preset.scalarFieldName, "temperature");
    EXPECT_EQ(preset.colorScheme, ColorScheme::VIRIDIS);
    EXPECT_TRUE(preset.autoScalarRange);
    EXPECT_TRUE(preset.useLighting);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
