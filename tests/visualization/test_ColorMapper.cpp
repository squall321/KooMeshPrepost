/**
 * @file test_ColorMapper.cpp
 * @brief Unit tests for ColorMapper
 */

#include <gtest/gtest.h>
#include "visualization/ColorMapper.h"
#include <cmath>

using namespace koomesh::visualization;

class ColorMapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        mapper = std::make_unique<ColorMapper>();
    }

    void TearDown() override {
        mapper.reset();
    }

    std::unique_ptr<ColorMapper> mapper;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(ColorMapperTest, DefaultConstruction) {
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::RAINBOW);
    EXPECT_EQ(mapper->getNumberOfColors(), 256);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], 0.0);
    EXPECT_DOUBLE_EQ(range[1], 1.0);
}

TEST_F(ColorMapperTest, CustomNumberOfColors) {
    auto mapper2 = std::make_unique<ColorMapper>(128);
    EXPECT_EQ(mapper2->getNumberOfColors(), 128);
}

// ============================================================================
// Color Scheme Tests
// ============================================================================

TEST_F(ColorMapperTest, SetColorScheme) {
    mapper->setColorScheme(ColorScheme::HEAT);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::HEAT);

    mapper->setColorScheme(ColorScheme::VIRIDIS);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::VIRIDIS);

    mapper->setColorScheme(ColorScheme::GRAYSCALE);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::GRAYSCALE);
}

TEST_F(ColorMapperTest, AllColorSchemes) {
    // Verify all color schemes work
    mapper->setColorScheme(ColorScheme::RAINBOW);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::RAINBOW);

    mapper->setColorScheme(ColorScheme::GRAYSCALE);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::GRAYSCALE);

    mapper->setColorScheme(ColorScheme::HEAT);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::HEAT);

    mapper->setColorScheme(ColorScheme::COOL_WARM);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::COOL_WARM);

    mapper->setColorScheme(ColorScheme::JET);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::JET);

    mapper->setColorScheme(ColorScheme::VIRIDIS);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::VIRIDIS);

    mapper->setColorScheme(ColorScheme::PLASMA);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::PLASMA);

    mapper->setColorScheme(ColorScheme::INFERNO);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::INFERNO);

    mapper->setColorScheme(ColorScheme::TURBO);
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::TURBO);
}

// ============================================================================
// Number of Colors Tests
// ============================================================================

TEST_F(ColorMapperTest, SetNumberOfColors) {
    mapper->setNumberOfColors(64);
    EXPECT_EQ(mapper->getNumberOfColors(), 64);

    mapper->setNumberOfColors(512);
    EXPECT_EQ(mapper->getNumberOfColors(), 512);
}

// ============================================================================
// Range Tests
// ============================================================================

TEST_F(ColorMapperTest, SetRange) {
    mapper->setRange(-100.0, 100.0);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], -100.0);
    EXPECT_DOUBLE_EQ(range[1], 100.0);
}

TEST_F(ColorMapperTest, SetRangeZeroToOne) {
    mapper->setRange(0.0, 1.0);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], 0.0);
    EXPECT_DOUBLE_EQ(range[1], 1.0);
}

TEST_F(ColorMapperTest, SetRangeNegative) {
    mapper->setRange(-50.0, -10.0);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], -50.0);
    EXPECT_DOUBLE_EQ(range[1], -10.0);
}

TEST_F(ColorMapperTest, ComputeRangeFromData) {
    std::vector<double> data = {1.0, 2.5, 3.7, -1.2, 5.9, 0.3};

    mapper->computeRangeFromData(data);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], -1.2);
    EXPECT_DOUBLE_EQ(range[1], 5.9);
}

TEST_F(ColorMapperTest, ComputeRangeFromEmptyData) {
    std::vector<double> emptyData;

    mapper->computeRangeFromData(emptyData);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], 0.0);
    EXPECT_DOUBLE_EQ(range[1], 1.0);
}

TEST_F(ColorMapperTest, ComputeRangeFromConstantData) {
    std::vector<double> data = {5.0, 5.0, 5.0, 5.0};

    mapper->computeRangeFromData(data);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], 5.0);
    EXPECT_DOUBLE_EQ(range[1], 5.0);
}

// ============================================================================
// Color Mapping Query Tests
// ============================================================================

TEST_F(ColorMapperTest, MapValueToColor) {
    mapper->setRange(0.0, 10.0);

    // Test value at minimum
    auto color = mapper->mapValueToColor(0.0);
    EXPECT_GE(color[0], 0.0);
    EXPECT_LE(color[0], 1.0);
    EXPECT_GE(color[1], 0.0);
    EXPECT_LE(color[1], 1.0);
    EXPECT_GE(color[2], 0.0);
    EXPECT_LE(color[2], 1.0);

    // Test value at maximum
    color = mapper->mapValueToColor(10.0);
    EXPECT_GE(color[0], 0.0);
    EXPECT_LE(color[0], 1.0);
    EXPECT_GE(color[1], 0.0);
    EXPECT_LE(color[1], 1.0);
    EXPECT_GE(color[2], 0.0);
    EXPECT_LE(color[2], 1.0);

    // Test value in middle
    color = mapper->mapValueToColor(5.0);
    EXPECT_GE(color[0], 0.0);
    EXPECT_LE(color[0], 1.0);
    EXPECT_GE(color[1], 0.0);
    EXPECT_LE(color[1], 1.0);
    EXPECT_GE(color[2], 0.0);
    EXPECT_LE(color[2], 1.0);
}

TEST_F(ColorMapperTest, MapValueToColor255) {
    mapper->setRange(0.0, 100.0);

    auto color = mapper->mapValueToColor255(50.0);

    EXPECT_GE(color[0], 0);
    EXPECT_LE(color[0], 255);
    EXPECT_GE(color[1], 0);
    EXPECT_LE(color[1], 255);
    EXPECT_GE(color[2], 0);
    EXPECT_LE(color[2], 255);
}

TEST_F(ColorMapperTest, ColorMappingContinuity) {
    mapper->setRange(0.0, 1.0);

    // Test that colors change smoothly
    auto color1 = mapper->mapValueToColor(0.0);
    auto color2 = mapper->mapValueToColor(0.5);
    auto color3 = mapper->mapValueToColor(1.0);

    // Colors should be different (at least one channel differs by more than 0.01)
    double diff1 = std::abs(color1[0] - color3[0]);
    double diff2 = std::abs(color1[1] - color3[1]);
    double diff3 = std::abs(color1[2] - color3[2]);

    EXPECT_TRUE(diff1 > 0.01 || diff2 > 0.01 || diff3 > 0.01);
}

// ============================================================================
// VTK Integration Tests
// ============================================================================

TEST_F(ColorMapperTest, GetLookupTable) {
    auto lut = mapper->getLookupTable();

#ifdef KOOMESH_HAS_VTK
    EXPECT_NE(lut, nullptr);
#else
    EXPECT_EQ(lut, nullptr);
#endif
}

TEST_F(ColorMapperTest, ApplyToActor) {
    // Without VTK, this should not crash
    std::vector<double> scalarValues = {1.0, 2.0, 3.0, 4.0, 5.0};
    mapper->applyToActor(nullptr, scalarValues);

    // Should succeed without crash
    SUCCEED();
}

TEST_F(ColorMapperTest, ApplyToPolyData) {
    // Without VTK, this should not crash
    std::vector<double> scalarValues = {1.0, 2.0, 3.0};
    mapper->applyToPolyData(nullptr, scalarValues, "TestScalar");

    // Should succeed without crash
    SUCCEED();
}

// ============================================================================
// Utility Tests
// ============================================================================

TEST_F(ColorMapperTest, BuildTable) {
    mapper->setColorScheme(ColorScheme::HEAT);
    mapper->buildTable();

    // Should not crash
    SUCCEED();
}

TEST_F(ColorMapperTest, Reset) {
    // Modify all settings
    mapper->setColorScheme(ColorScheme::HEAT);
    mapper->setNumberOfColors(128);
    mapper->setRange(-50.0, 50.0);

    // Reset
    mapper->reset();

    // Verify back to defaults
    EXPECT_EQ(mapper->getColorScheme(), ColorScheme::RAINBOW);
    EXPECT_EQ(mapper->getNumberOfColors(), 256);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], 0.0);
    EXPECT_DOUBLE_EQ(range[1], 1.0);
}

TEST_F(ColorMapperTest, Statistics) {
    mapper->setColorScheme(ColorScheme::VIRIDIS);
    mapper->setRange(0.0, 100.0);

    std::string stats = mapper->getStatistics();

    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Color Mapper"), std::string::npos);
    EXPECT_NE(stats.find("Viridis"), std::string::npos);
    EXPECT_NE(stats.find("256"), std::string::npos);
}

// ============================================================================
// Different Color Schemes Tests
// ============================================================================

TEST_F(ColorMapperTest, GrayscaleColorScheme) {
    mapper->setColorScheme(ColorScheme::GRAYSCALE);
    mapper->setRange(0.0, 1.0);

#ifdef KOOMESH_HAS_VTK
    // With VTK, grayscale should work properly
    auto color0 = mapper->mapValueToColor(0.0);
    auto color1 = mapper->mapValueToColor(1.0);

    // At minimum, should be dark (low values)
    EXPECT_LE(color0[0], 0.1);
    EXPECT_LE(color0[1], 0.1);
    EXPECT_LE(color0[2], 0.1);

    // At maximum, should be bright (high values)
    EXPECT_GE(color1[0], 0.9);
    EXPECT_GE(color1[1], 0.9);
    EXPECT_GE(color1[2], 0.9);
#else
    // Without VTK, stub implementation uses rainbow - just verify it doesn't crash
    auto color = mapper->mapValueToColor(0.5);
    EXPECT_GE(color[0], 0.0);
    EXPECT_LE(color[0], 1.0);
#endif
}

TEST_F(ColorMapperTest, HeatColorScheme) {
    mapper->setColorScheme(ColorScheme::HEAT);
    mapper->setRange(0.0, 1.0);

    auto color = mapper->mapValueToColor(0.5);

    // Should have some color (not black or white)
    EXPECT_TRUE(color[0] > 0.0 || color[1] > 0.0 || color[2] > 0.0);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ColorMapperTest, ZeroColors) {
    // Even with 0 colors requested, should handle gracefully
    mapper->setNumberOfColors(0);

    // Should not crash
    SUCCEED();
}

TEST_F(ColorMapperTest, SingleColor) {
    mapper->setNumberOfColors(1);

    EXPECT_EQ(mapper->getNumberOfColors(), 1);
}

TEST_F(ColorMapperTest, LargeNumberOfColors) {
    mapper->setNumberOfColors(4096);

    EXPECT_EQ(mapper->getNumberOfColors(), 4096);
}

TEST_F(ColorMapperTest, InvertedRange) {
    // Max < Min (inverted range)
    mapper->setRange(100.0, 0.0);

    auto range = mapper->getRange();
    EXPECT_DOUBLE_EQ(range[0], 100.0);
    EXPECT_DOUBLE_EQ(range[1], 0.0);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
