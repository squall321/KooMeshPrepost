/**
 * @file test_ScreenshotManager.cpp
 * @brief Unit tests for ScreenshotManager
 */

#include <gtest/gtest.h>
#include "visualization/ScreenshotManager.h"

using namespace koomesh::visualization;

class ScreenshotManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<ScreenshotManager>(nullptr);
    }

    void TearDown() override {
        manager.reset();
    }

    std::unique_ptr<ScreenshotManager> manager;
};

// ============================================================================
// Screenshot Configuration Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, DefaultScreenshotConfig) {
    const auto& config = manager->getScreenshotConfig();

    EXPECT_EQ(config.format, ImageFormat::PNG);
    EXPECT_EQ(config.width, 1920);
    EXPECT_EQ(config.height, 1080);
    EXPECT_EQ(config.quality, 95);
    EXPECT_FALSE(config.transparent);
    EXPECT_EQ(config.magnification, 1);
}

TEST_F(ScreenshotManagerTest, SetScreenshotConfig) {
    ScreenshotConfig config;
    config.format = ImageFormat::JPEG;
    config.width = 1280;
    config.height = 720;
    config.quality = 80;
    config.transparent = true;
    config.magnification = 2;

    manager->setScreenshotConfig(config);

    const auto& retrieved = manager->getScreenshotConfig();
    EXPECT_EQ(retrieved.format, ImageFormat::JPEG);
    EXPECT_EQ(retrieved.width, 1280);
    EXPECT_EQ(retrieved.height, 720);
    EXPECT_EQ(retrieved.quality, 80);
    EXPECT_TRUE(retrieved.transparent);
    EXPECT_EQ(retrieved.magnification, 2);
}

TEST_F(ScreenshotManagerTest, AllImageFormats) {
    ScreenshotConfig config;

    config.format = ImageFormat::PNG;
    manager->setScreenshotConfig(config);
    EXPECT_EQ(manager->getScreenshotConfig().format, ImageFormat::PNG);

    config.format = ImageFormat::JPEG;
    manager->setScreenshotConfig(config);
    EXPECT_EQ(manager->getScreenshotConfig().format, ImageFormat::JPEG);

    config.format = ImageFormat::BMP;
    manager->setScreenshotConfig(config);
    EXPECT_EQ(manager->getScreenshotConfig().format, ImageFormat::BMP);

    config.format = ImageFormat::TIFF;
    manager->setScreenshotConfig(config);
    EXPECT_EQ(manager->getScreenshotConfig().format, ImageFormat::TIFF);
}

// ============================================================================
// Screenshot Capture Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, CaptureScreenshot) {
    // Without VTK, this should return false
    bool result = manager->captureScreenshot("test.png");
    EXPECT_FALSE(result);
}

TEST_F(ScreenshotManagerTest, CaptureScreenshotWithConfig) {
    ScreenshotConfig config;
    config.format = ImageFormat::PNG;
    config.width = 800;
    config.height = 600;

    bool result = manager->captureScreenshot("test.png", config);
    EXPECT_FALSE(result);
}

TEST_F(ScreenshotManagerTest, CaptureHighResScreenshot) {
    bool result = manager->captureHighResScreenshot("test_hires.png", 2);
    EXPECT_FALSE(result);
}

TEST_F(ScreenshotManagerTest, CaptureHighResWithMagnification) {
    for (int mag = 1; mag <= 4; ++mag) {
        bool result = manager->captureHighResScreenshot("test.png", mag);
        EXPECT_FALSE(result);
    }
}

// ============================================================================
// Animation Configuration Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, DefaultAnimationConfig) {
    const auto& config = manager->getAnimationConfig();

    EXPECT_EQ(config.mode, AnimationMode::MANUAL);
    EXPECT_EQ(config.fps, 30);
    EXPECT_EQ(config.numFrames, 60);
    EXPECT_DOUBLE_EQ(config.rotationDegrees, 360.0);
    EXPECT_FALSE(config.loop);
    EXPECT_EQ(config.outputPrefix, "frame_");
}

TEST_F(ScreenshotManagerTest, SetAnimationConfig) {
    AnimationConfig config;
    config.mode = AnimationMode::AUTO_ROTATE;
    config.fps = 60;
    config.numFrames = 120;
    config.rotationDegrees = 180.0;
    config.loop = true;
    config.outputPrefix = "anim_";

    manager->setAnimationConfig(config);

    const auto& retrieved = manager->getAnimationConfig();
    EXPECT_EQ(retrieved.mode, AnimationMode::AUTO_ROTATE);
    EXPECT_EQ(retrieved.fps, 60);
    EXPECT_EQ(retrieved.numFrames, 120);
    EXPECT_DOUBLE_EQ(retrieved.rotationDegrees, 180.0);
    EXPECT_TRUE(retrieved.loop);
    EXPECT_EQ(retrieved.outputPrefix, "anim_");
}

TEST_F(ScreenshotManagerTest, AllAnimationModes) {
    AnimationConfig config;

    config.mode = AnimationMode::MANUAL;
    manager->setAnimationConfig(config);
    EXPECT_EQ(manager->getAnimationConfig().mode, AnimationMode::MANUAL);

    config.mode = AnimationMode::AUTO_ROTATE;
    manager->setAnimationConfig(config);
    EXPECT_EQ(manager->getAnimationConfig().mode, AnimationMode::AUTO_ROTATE);

    config.mode = AnimationMode::AUTO_PATH;
    manager->setAnimationConfig(config);
    EXPECT_EQ(manager->getAnimationConfig().mode, AnimationMode::AUTO_PATH);

    config.mode = AnimationMode::CUSTOM_CALLBACK;
    manager->setAnimationConfig(config);
    EXPECT_EQ(manager->getAnimationConfig().mode, AnimationMode::CUSTOM_CALLBACK);
}

// ============================================================================
// Animation Recording Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, StartAnimation) {
    AnimationConfig config;
    config.numFrames = 30;

    bool result = manager->startAnimation("/tmp", config);
    EXPECT_TRUE(result);
    EXPECT_TRUE(manager->isRecording());
    EXPECT_EQ(manager->getCurrentFrame(), 0);
    EXPECT_EQ(manager->getTotalFrames(), 30);
}

TEST_F(ScreenshotManagerTest, StopAnimation) {
    AnimationConfig config;
    config.numFrames = 30;

    manager->startAnimation("/tmp", config);
    EXPECT_TRUE(manager->isRecording());

    manager->stopAnimation();
    EXPECT_FALSE(manager->isRecording());
}

TEST_F(ScreenshotManagerTest, RecordFrame) {
    AnimationConfig config;
    config.numFrames = 3;

    manager->startAnimation("/tmp", config);
    EXPECT_EQ(manager->getCurrentFrame(), 0);

    // Record frames
    bool result = manager->recordFrame();
    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getCurrentFrame(), 1);

    result = manager->recordFrame();
    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getCurrentFrame(), 2);

    result = manager->recordFrame();
    EXPECT_TRUE(result);
    EXPECT_EQ(manager->getCurrentFrame(), 3);

    // Animation should stop after last frame
    EXPECT_FALSE(manager->isRecording());
}

TEST_F(ScreenshotManagerTest, RecordAllFrames) {
    AnimationConfig config;
    config.numFrames = 10;

    manager->startAnimation("/tmp", config);
    EXPECT_TRUE(manager->isRecording());

    bool result = manager->recordAllFrames();
    EXPECT_TRUE(result);
    EXPECT_FALSE(manager->isRecording());
    EXPECT_EQ(manager->getCurrentFrame(), 10);
}

TEST_F(ScreenshotManagerTest, RecordFrameWithoutStart) {
    bool result = manager->recordFrame();
    EXPECT_FALSE(result);
}

TEST_F(ScreenshotManagerTest, AnimationLoop) {
    AnimationConfig config;
    config.numFrames = 3;
    config.loop = true;

    manager->startAnimation("/tmp", config);

    // Record all frames
    manager->recordFrame();
    manager->recordFrame();
    manager->recordFrame();

    // With loop, animation should still be recording and reset to frame 0
    EXPECT_TRUE(manager->isRecording());
    EXPECT_EQ(manager->getCurrentFrame(), 0);
}

// ============================================================================
// Camera Keyframe Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, AddCameraKeyframe) {
    CameraKeyframe kf;
    kf.position = {1.0, 2.0, 3.0};
    kf.focalPoint = {0.0, 0.0, 0.0};
    kf.viewUp = {0.0, 1.0, 0.0};
    kf.time = 0.5;

    manager->addCameraKeyframe(kf);
    EXPECT_EQ(manager->getCameraKeyframeCount(), 1);
}

TEST_F(ScreenshotManagerTest, MultipleCameraKeyframes) {
    for (int i = 0; i < 5; ++i) {
        CameraKeyframe kf;
        kf.position = {static_cast<double>(i), 0.0, 0.0};
        kf.focalPoint = {0.0, 0.0, 0.0};
        kf.viewUp = {0.0, 1.0, 0.0};
        kf.time = i * 0.2;

        manager->addCameraKeyframe(kf);
    }

    EXPECT_EQ(manager->getCameraKeyframeCount(), 5);
}

TEST_F(ScreenshotManagerTest, ClearCameraKeyframes) {
    CameraKeyframe kf;
    kf.time = 0.0;

    manager->addCameraKeyframe(kf);
    manager->addCameraKeyframe(kf);
    manager->addCameraKeyframe(kf);

    EXPECT_EQ(manager->getCameraKeyframeCount(), 3);

    manager->clearCameraKeyframes();
    EXPECT_EQ(manager->getCameraKeyframeCount(), 0);
}

// ============================================================================
// Animation Callback Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, SetAnimationCallback) {
    int callbackCalled = 0;
    auto callback = [&callbackCalled](int frame) {
        callbackCalled = frame;
    };

    manager->setAnimationCallback(callback);

    // Callback is set (no way to verify directly in stub, just ensure it doesn't crash)
    SUCCEED();
}

// ============================================================================
// Progress Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, GetProgress) {
    AnimationConfig config;
    config.numFrames = 10;

    manager->startAnimation("/tmp", config);

    EXPECT_DOUBLE_EQ(manager->getProgress(), 0.0);

    for (int i = 0; i < 5; ++i) {
        manager->recordFrame();
    }

    EXPECT_DOUBLE_EQ(manager->getProgress(), 0.5);
}

TEST_F(ScreenshotManagerTest, ProgressFullAnimation) {
    AnimationConfig config;
    config.numFrames = 100;

    manager->startAnimation("/tmp", config);

    for (int i = 0; i < 50; ++i) {
        manager->recordFrame();
    }

    EXPECT_DOUBLE_EQ(manager->getProgress(), 0.5);

    for (int i = 0; i < 50; ++i) {
        manager->recordFrame();
    }

    EXPECT_DOUBLE_EQ(manager->getProgress(), 1.0);
}

TEST_F(ScreenshotManagerTest, ProgressWithNoFrames) {
    AnimationConfig config;
    config.numFrames = 0;

    manager->setAnimationConfig(config);
    EXPECT_DOUBLE_EQ(manager->getProgress(), 0.0);
}

// ============================================================================
// Frame Tracking Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, GetCurrentFrame) {
    AnimationConfig config;
    config.numFrames = 10;

    manager->startAnimation("/tmp", config);
    EXPECT_EQ(manager->getCurrentFrame(), 0);

    manager->recordFrame();
    EXPECT_EQ(manager->getCurrentFrame(), 1);

    manager->recordFrame();
    EXPECT_EQ(manager->getCurrentFrame(), 2);
}

TEST_F(ScreenshotManagerTest, GetTotalFrames) {
    AnimationConfig config;
    config.numFrames = 42;

    manager->setAnimationConfig(config);
    EXPECT_EQ(manager->getTotalFrames(), 42);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, GetStatistics) {
    std::string stats = manager->getStatistics();

    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Screenshot Manager"), std::string::npos);
}

TEST_F(ScreenshotManagerTest, StatisticsWhileRecording) {
    AnimationConfig config;
    config.numFrames = 30;
    config.fps = 60;

    manager->startAnimation("/tmp", config);

    std::string stats = manager->getStatistics();

    EXPECT_NE(stats.find("Yes"), std::string::npos);  // Is Recording
    EXPECT_NE(stats.find("30"), std::string::npos);   // Total Frames
}

TEST_F(ScreenshotManagerTest, StatisticsNotRecording) {
    std::string stats = manager->getStatistics();

    EXPECT_NE(stats.find("No"), std::string::npos);  // Not Recording
}

// ============================================================================
// Set Camera for Frame Tests
// ============================================================================

TEST_F(ScreenshotManagerTest, SetCameraForFrame) {
    AnimationConfig config;
    config.numFrames = 10;
    config.mode = AnimationMode::AUTO_ROTATE;

    manager->setAnimationConfig(config);

    // Should not crash with null render window
    manager->setCameraForFrame(0);
    manager->setCameraForFrame(5);
    manager->setCameraForFrame(9);

    SUCCEED();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ScreenshotManagerTest, NegativeFrameCount) {
    AnimationConfig config;
    config.numFrames = -1;

    manager->setAnimationConfig(config);

    // Should handle gracefully
    EXPECT_EQ(manager->getTotalFrames(), -1);
}

TEST_F(ScreenshotManagerTest, ZeroFrameCount) {
    AnimationConfig config;
    config.numFrames = 0;

    manager->setAnimationConfig(config);
    EXPECT_EQ(manager->getTotalFrames(), 0);
}

TEST_F(ScreenshotManagerTest, VeryHighMagnification) {
    bool result = manager->captureHighResScreenshot("test.png", 100);
    EXPECT_FALSE(result);  // Stub always returns false
}

TEST_F(ScreenshotManagerTest, InvalidQuality) {
    ScreenshotConfig config;
    config.quality = -10;

    manager->setScreenshotConfig(config);
    EXPECT_EQ(manager->getScreenshotConfig().quality, -10);

    config.quality = 200;
    manager->setScreenshotConfig(config);
    EXPECT_EQ(manager->getScreenshotConfig().quality, 200);
}

TEST_F(ScreenshotManagerTest, MultipleStartStop) {
    AnimationConfig config;
    config.numFrames = 10;

    manager->startAnimation("/tmp", config);
    EXPECT_TRUE(manager->isRecording());

    manager->stopAnimation();
    EXPECT_FALSE(manager->isRecording());

    manager->startAnimation("/tmp", config);
    EXPECT_TRUE(manager->isRecording());

    manager->stopAnimation();
    EXPECT_FALSE(manager->isRecording());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
