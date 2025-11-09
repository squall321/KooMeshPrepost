/**
 * @file test_LightingManager.cpp
 * @brief Unit tests for LightingManager
 */

#include <gtest/gtest.h>
#include "visualization/LightingManager.h"

using namespace koomesh::visualization;

class LightingManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<LightingManager>(nullptr);
    }

    void TearDown() override {
        manager.reset();
    }

    std::unique_ptr<LightingManager> manager;
};

// ============================================================================
// Construction Tests
// ============================================================================

TEST_F(LightingManagerTest, DefaultConstruction) {
    EXPECT_EQ(manager->getLightCount(), 0);
    EXPECT_DOUBLE_EQ(manager->getAmbientLight(), 0.2);
    EXPECT_FALSE(manager->areShadowsEnabled());
    EXPECT_FALSE(manager->isSSAOEnabled());
}

// ============================================================================
// Light Management Tests
// ============================================================================

TEST_F(LightingManagerTest, AddHeadlight) {
    manager->addHeadlight();
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, AddKeyLight) {
    Eigen::Vector3d position(1.0, 2.0, 3.0);
    manager->addKeyLight(position);
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, AddFillLight) {
    Eigen::Vector3d position(-1.0, 1.0, 2.0);
    manager->addFillLight(position, 0.5);
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, AddBackLight) {
    Eigen::Vector3d position(0.0, -1.0, 1.0);
    manager->addBackLight(position, 0.3);
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, AddCustomLight) {
    LightConfig config;
    config.position = {2.0, 2.0, 2.0};
    config.intensity = 0.8;

    int index = manager->addLight(config);
    EXPECT_GE(index, 0);
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, AddMultipleLights) {
    manager->addHeadlight();
    manager->addKeyLight({1.0, 1.0, 1.0});
    manager->addFillLight({-1.0, 1.0, 0.0});

    EXPECT_EQ(manager->getLightCount(), 3);
}

TEST_F(LightingManagerTest, RemoveLight) {
    manager->addHeadlight();
    manager->addKeyLight({1.0, 1.0, 1.0});

    EXPECT_EQ(manager->getLightCount(), 2);

    manager->removeLight(0);
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, RemoveInvalidLight) {
    manager->addHeadlight();

    // Try to remove non-existent light
    manager->removeLight(10);

    // Should still have 1 light
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, RemoveAllLights) {
    manager->addHeadlight();
    manager->addKeyLight({1.0, 1.0, 1.0});
    manager->addFillLight({-1.0, 1.0, 0.0});

    EXPECT_EQ(manager->getLightCount(), 3);

    manager->removeAllLights();
    EXPECT_EQ(manager->getLightCount(), 0);
}

// ============================================================================
// Light Configuration Tests
// ============================================================================

TEST_F(LightingManagerTest, SetLightPosition) {
    manager->addKeyLight({0.0, 0.0, 0.0});

    Eigen::Vector3d newPosition(5.0, 5.0, 5.0);
    manager->setLightPosition(0, newPosition);

    // Should not crash
    SUCCEED();
}

TEST_F(LightingManagerTest, SetLightColor) {
    manager->addKeyLight({1.0, 1.0, 1.0});

    Eigen::Vector3d color(1.0, 0.0, 0.0);
    manager->setLightColor(0, color);

    // Should not crash
    SUCCEED();
}

TEST_F(LightingManagerTest, SetLightIntensity) {
    manager->addKeyLight({1.0, 1.0, 1.0});

    manager->setLightIntensity(0, 0.7);

    // Should not crash
    SUCCEED();
}

TEST_F(LightingManagerTest, SetLightEnabled) {
    manager->addKeyLight({1.0, 1.0, 1.0});

    manager->setLightEnabled(0, false);
    manager->setLightEnabled(0, true);

    // Should not crash
    SUCCEED();
}

TEST_F(LightingManagerTest, GetLight) {
    manager->addKeyLight({1.0, 1.0, 1.0});

    auto light = manager->getLight(0);
#ifdef KOOMESH_HAS_VTK
    EXPECT_NE(light, nullptr);
#else
    EXPECT_EQ(light, nullptr);
#endif
}

TEST_F(LightingManagerTest, GetInvalidLight) {
    manager->addKeyLight({1.0, 1.0, 1.0});

    auto light = manager->getLight(10);
    EXPECT_EQ(light, nullptr);
}

// ============================================================================
// Ambient Lighting Tests
// ============================================================================

TEST_F(LightingManagerTest, SetAmbientLight) {
    manager->setAmbientLight(0.5);
    EXPECT_DOUBLE_EQ(manager->getAmbientLight(), 0.5);
}

TEST_F(LightingManagerTest, SetAmbientLightClamp) {
    // Test clamping to [0, 1]
    manager->setAmbientLight(-0.5);
    EXPECT_GE(manager->getAmbientLight(), 0.0);

    manager->setAmbientLight(2.0);
    EXPECT_LE(manager->getAmbientLight(), 1.0);
}

TEST_F(LightingManagerTest, SetAmbientColor) {
    Eigen::Vector3d color(0.8, 0.9, 1.0);
    manager->setAmbientColor(color);

    // Should not crash
    SUCCEED();
}

// ============================================================================
// Advanced Features Tests
// ============================================================================

TEST_F(LightingManagerTest, EnableShadows) {
    manager->enableShadows(true);
    EXPECT_TRUE(manager->areShadowsEnabled());

    manager->enableShadows(false);
    EXPECT_FALSE(manager->areShadowsEnabled());
}

TEST_F(LightingManagerTest, EnableSSAO) {
    manager->enableSSAO(true);
    EXPECT_TRUE(manager->isSSAOEnabled());

    manager->enableSSAO(false);
    EXPECT_FALSE(manager->isSSAOEnabled());
}

TEST_F(LightingManagerTest, SetSSAOParams) {
    manager->setSSAOParams(15.0, 0.02, 1.5);

    // Should not crash
    SUCCEED();
}

// ============================================================================
// Lighting Preset Tests
// ============================================================================

TEST_F(LightingManagerTest, SetupThreePointLighting) {
    manager->setupThreePointLighting();

    // Should have 3 lights (key, fill, back)
    EXPECT_EQ(manager->getLightCount(), 3);
}

TEST_F(LightingManagerTest, SetupStudioLighting) {
    manager->setupStudioLighting();

    // Should have multiple lights (key + fills + back)
    EXPECT_GE(manager->getLightCount(), 3);
}

TEST_F(LightingManagerTest, SetupOutdoorLighting) {
    manager->setupOutdoorLighting();

    // Should have at least 1 light (sun)
    EXPECT_GE(manager->getLightCount(), 1);

    // Should have higher ambient
    EXPECT_GE(manager->getAmbientLight(), 0.3);
}

TEST_F(LightingManagerTest, SetupDefaultLighting) {
    manager->setupDefaultLighting();

    // Should have 1 headlight
    EXPECT_EQ(manager->getLightCount(), 1);
}

TEST_F(LightingManagerTest, PresetsClearPreviousLights) {
    // Add some lights
    manager->addHeadlight();
    manager->addKeyLight({1.0, 1.0, 1.0});

    EXPECT_EQ(manager->getLightCount(), 2);

    // Setup preset
    manager->setupThreePointLighting();

    // Should only have preset lights
    EXPECT_EQ(manager->getLightCount(), 3);
}

// ============================================================================
// Utility Tests
// ============================================================================

TEST_F(LightingManagerTest, Update) {
    manager->addHeadlight();
    manager->update();

    // Should not crash
    SUCCEED();
}

TEST_F(LightingManagerTest, GetStatistics) {
    manager->addHeadlight();
    manager->addKeyLight({1.0, 1.0, 1.0});
    manager->setAmbientLight(0.3);
    manager->enableShadows(true);

    std::string stats = manager->getStatistics();

    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Lighting Manager"), std::string::npos);
    EXPECT_NE(stats.find("Lights"), std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(LightingManagerTest, CompleteScenario) {
    // Setup three-point lighting
    manager->setupThreePointLighting();
    EXPECT_EQ(manager->getLightCount(), 3);

    // Adjust ambient
    manager->setAmbientLight(0.3);
    EXPECT_DOUBLE_EQ(manager->getAmbientLight(), 0.3);

    // Enable advanced features
    manager->enableShadows(true);
    manager->enableSSAO(true);
    EXPECT_TRUE(manager->areShadowsEnabled());
    EXPECT_TRUE(manager->isSSAOEnabled());

    // Update
    manager->update();

    // Get statistics
    std::string stats = manager->getStatistics();
    EXPECT_FALSE(stats.empty());
}

TEST_F(LightingManagerTest, DynamicLightManagement) {
    // Start with default
    manager->setupDefaultLighting();
    EXPECT_EQ(manager->getLightCount(), 1);

    // Add more lights
    manager->addKeyLight({5.0, 5.0, 5.0});
    manager->addFillLight({-3.0, 2.0, 3.0});
    EXPECT_EQ(manager->getLightCount(), 3);

    // Remove one
    manager->removeLight(1);
    EXPECT_EQ(manager->getLightCount(), 2);

    // Clear all
    manager->removeAllLights();
    EXPECT_EQ(manager->getLightCount(), 0);
}

TEST_F(LightingManagerTest, LightConfiguration) {
    manager->addKeyLight({0.0, 0.0, 0.0});

    // Configure light
    manager->setLightPosition(0, {10.0, 10.0, 10.0});
    manager->setLightColor(0, {1.0, 0.9, 0.8});
    manager->setLightIntensity(0, 1.5);

    // Should not crash
    SUCCEED();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
