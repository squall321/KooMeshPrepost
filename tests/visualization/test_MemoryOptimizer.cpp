/**
 * @file test_MemoryOptimizer.cpp
 * @brief Unit tests for MemoryOptimizer
 */

#include <gtest/gtest.h>
#include "visualization/MemoryOptimizer.h"

using namespace koomesh::visualization;

class MemoryOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        optimizer = std::make_unique<MemoryOptimizer>();
    }

    void TearDown() override {
        optimizer.reset();
    }

    std::unique_ptr<MemoryOptimizer> optimizer;
};

// ============================================================================
// LOD Configuration Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, DefaultLODConfig) {
    const auto& config = optimizer->getLODConfig();

    EXPECT_TRUE(config.enabled);
    EXPECT_EQ(config.numLevels, 3);
    EXPECT_DOUBLE_EQ(config.highResThreshold, 0.1);
    EXPECT_DOUBLE_EQ(config.mediumResThreshold, 0.5);
    EXPECT_DOUBLE_EQ(config.reductionTarget, 0.5);
    EXPECT_EQ(config.maxCells, 1000000);
}

TEST_F(MemoryOptimizerTest, SetLODConfig) {
    LODConfig config;
    config.enabled = false;
    config.numLevels = 5;
    config.highResThreshold = 0.2;
    config.mediumResThreshold = 0.6;
    config.reductionTarget = 0.7;
    config.maxCells = 500000;

    optimizer->setLODConfig(config);

    const auto& retrieved = optimizer->getLODConfig();
    EXPECT_FALSE(retrieved.enabled);
    EXPECT_EQ(retrieved.numLevels, 5);
    EXPECT_DOUBLE_EQ(retrieved.highResThreshold, 0.2);
    EXPECT_DOUBLE_EQ(retrieved.mediumResThreshold, 0.6);
    EXPECT_DOUBLE_EQ(retrieved.reductionTarget, 0.7);
    EXPECT_EQ(retrieved.maxCells, 500000);
}

// ============================================================================
// Streaming Configuration Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, DefaultStreamingConfig) {
    const auto& config = optimizer->getStreamingConfig();

    EXPECT_FALSE(config.enabled);
    EXPECT_EQ(config.numberOfPieces, 4);
    EXPECT_EQ(config.updateRate, 100);
    EXPECT_TRUE(config.prioritizeViewport);
}

TEST_F(MemoryOptimizerTest, SetStreamingConfig) {
    StreamingConfig config;
    config.enabled = true;
    config.numberOfPieces = 8;
    config.updateRate = 50;
    config.prioritizeViewport = false;

    optimizer->setStreamingConfig(config);

    const auto& retrieved = optimizer->getStreamingConfig();
    EXPECT_TRUE(retrieved.enabled);
    EXPECT_EQ(retrieved.numberOfPieces, 8);
    EXPECT_EQ(retrieved.updateRate, 50);
    EXPECT_FALSE(retrieved.prioritizeViewport);
}

TEST_F(MemoryOptimizerTest, EnableStreaming) {
    EXPECT_FALSE(optimizer->isStreamingEnabled());

    StreamingConfig config;
    config.numberOfPieces = 16;
    optimizer->enableStreaming(config);

    EXPECT_TRUE(optimizer->isStreamingEnabled());
    EXPECT_EQ(optimizer->getStreamingConfig().numberOfPieces, 16);
}

TEST_F(MemoryOptimizerTest, DisableStreaming) {
    StreamingConfig config;
    optimizer->enableStreaming(config);
    EXPECT_TRUE(optimizer->isStreamingEnabled());

    optimizer->disableStreaming();
    EXPECT_FALSE(optimizer->isStreamingEnabled());
}

// ============================================================================
// GPU Memory Configuration Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, DefaultGPUConfig) {
    const auto& config = optimizer->getGPUMemoryConfig();

    EXPECT_TRUE(config.trackUsage);
    EXPECT_EQ(config.maxTextureMemory, 512 * 1024 * 1024);
    EXPECT_EQ(config.maxVBOMemory, 1024 * 1024 * 1024);
    EXPECT_FALSE(config.useDisplayLists);
    EXPECT_TRUE(config.useVBOs);
}

TEST_F(MemoryOptimizerTest, SetGPUConfig) {
    GPUMemoryConfig config;
    config.trackUsage = false;
    config.maxTextureMemory = 256 * 1024 * 1024;
    config.maxVBOMemory = 2048 * 1024 * 1024;
    config.useDisplayLists = true;
    config.useVBOs = false;

    optimizer->setGPUMemoryConfig(config);

    const auto& retrieved = optimizer->getGPUMemoryConfig();
    EXPECT_FALSE(retrieved.trackUsage);
    EXPECT_EQ(retrieved.maxTextureMemory, 256 * 1024 * 1024);
    EXPECT_EQ(retrieved.maxVBOMemory, 2048 * 1024 * 1024);
    EXPECT_TRUE(retrieved.useDisplayLists);
    EXPECT_FALSE(retrieved.useVBOs);
}

// ============================================================================
// LOD Creation Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, CreateLODActor) {
    // Stub always returns nullptr
    auto actor = optimizer->createLODActor(nullptr);
    EXPECT_EQ(actor, nullptr);
}

TEST_F(MemoryOptimizerTest, DecimatePolyData) {
    // Stub always returns nullptr
    auto decimated = optimizer->decimatePolyData(nullptr, 0.5);
    EXPECT_EQ(decimated, nullptr);
}

TEST_F(MemoryOptimizerTest, CreateLODLevels) {
    auto levels = optimizer->createLODLevels(nullptr, 3);

    // Stub returns vector of 3 nullptrs
    EXPECT_EQ(levels.size(), 3);
    for (auto& level : levels) {
        EXPECT_EQ(level, nullptr);
    }
}

TEST_F(MemoryOptimizerTest, CreateLODLevelsZero) {
    auto levels = optimizer->createLODLevels(nullptr, 0);
    EXPECT_EQ(levels.size(), 0);
}

TEST_F(MemoryOptimizerTest, CreateLODLevelsNegative) {
    auto levels = optimizer->createLODLevels(nullptr, -1);
    EXPECT_EQ(levels.size(), 0);
}

// ============================================================================
// Memory Estimation Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, EstimateMemoryUsage) {
    // Stub always returns 0
    size_t memory = optimizer->estimateMemoryUsage(nullptr);
    EXPECT_EQ(memory, 0);
}

// ============================================================================
// GPU Optimization Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, OptimizeActorForGPU) {
    // Should not crash with nullptr
    optimizer->optimizeActorForGPU(nullptr);
    SUCCEED();
}

// ============================================================================
// Memory Statistics Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, GetMemoryStats) {
    auto stats = optimizer->getMemoryStats();

    EXPECT_EQ(stats.totalMemoryUsed, 0);
    EXPECT_EQ(stats.vtkObjectMemory, 0);
    EXPECT_EQ(stats.textureMemory, 0);
    EXPECT_EQ(stats.vboMemory, 0);
    EXPECT_EQ(stats.numActors, 0);
    EXPECT_EQ(stats.numPolyData, 0);
    EXPECT_EQ(stats.totalCells, 0);
    EXPECT_EQ(stats.totalPoints, 0);
}

TEST_F(MemoryOptimizerTest, UpdateMemoryStats) {
    optimizer->updateMemoryStats();

    // Should not crash
    SUCCEED();
}

TEST_F(MemoryOptimizerTest, ClearCache) {
    optimizer->clearCache();

    // Verify stats are cleared
    auto stats = optimizer->getMemoryStats();
    EXPECT_EQ(stats.totalMemoryUsed, 0);
}

// ============================================================================
// Utility Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, IsLargeDataset) {
    // Stub always returns false
    bool large = optimizer->isLargeDataset(nullptr);
    EXPECT_FALSE(large);

    large = optimizer->isLargeDataset(nullptr, 10000);
    EXPECT_FALSE(large);
}

TEST_F(MemoryOptimizerTest, GetRecommendedLODLevels) {
    EXPECT_EQ(optimizer->getRecommendedLODLevels(1000), 1);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(50000), 2);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(500000), 3);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(5000000), 4);
}

TEST_F(MemoryOptimizerTest, GetRecommendedLODLevelsZero) {
    EXPECT_EQ(optimizer->getRecommendedLODLevels(0), 1);
}

TEST_F(MemoryOptimizerTest, GetRecommendedLODLevelsEdgeCases) {
    EXPECT_EQ(optimizer->getRecommendedLODLevels(9999), 1);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(10000), 2);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(99999), 2);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(100000), 3);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(999999), 3);
    EXPECT_EQ(optimizer->getRecommendedLODLevels(1000000), 4);
}

// ============================================================================
// Statistics String Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, GetStatistics) {
    std::string stats = optimizer->getStatistics();

    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("Memory Optimizer"), std::string::npos);
}

TEST_F(MemoryOptimizerTest, StatisticsContainsLODInfo) {
    std::string stats = optimizer->getStatistics();

    EXPECT_NE(stats.find("LOD"), std::string::npos);
    EXPECT_NE(stats.find("Enabled"), std::string::npos);
}

TEST_F(MemoryOptimizerTest, StatisticsContainsStreamingInfo) {
    std::string stats = optimizer->getStatistics();

    EXPECT_NE(stats.find("Streaming"), std::string::npos);
}

TEST_F(MemoryOptimizerTest, StatisticsContainsGPUInfo) {
    std::string stats = optimizer->getStatistics();

    EXPECT_NE(stats.find("GPU"), std::string::npos);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(MemoryOptimizerTest, ConfigureAndOptimize) {
    // Set up configuration
    LODConfig lodConfig;
    lodConfig.enabled = true;
    lodConfig.numLevels = 4;
    optimizer->setLODConfig(lodConfig);

    StreamingConfig streamConfig;
    streamConfig.numberOfPieces = 8;
    optimizer->enableStreaming(streamConfig);

    GPUMemoryConfig gpuConfig;
    gpuConfig.maxTextureMemory = 1024 * 1024 * 1024;
    optimizer->setGPUMemoryConfig(gpuConfig);

    // Verify configurations are set
    EXPECT_EQ(optimizer->getLODConfig().numLevels, 4);
    EXPECT_TRUE(optimizer->isStreamingEnabled());
    EXPECT_EQ(optimizer->getGPUMemoryConfig().maxTextureMemory, 1024 * 1024 * 1024);
}

TEST_F(MemoryOptimizerTest, MultipleStreamingToggles) {
    EXPECT_FALSE(optimizer->isStreamingEnabled());

    optimizer->enableStreaming(StreamingConfig());
    EXPECT_TRUE(optimizer->isStreamingEnabled());

    optimizer->disableStreaming();
    EXPECT_FALSE(optimizer->isStreamingEnabled());

    optimizer->enableStreaming(StreamingConfig());
    EXPECT_TRUE(optimizer->isStreamingEnabled());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(MemoryOptimizerTest, NegativeValues) {
    LODConfig config;
    config.numLevels = -1;
    config.maxCells = -100;

    optimizer->setLODConfig(config);

    const auto& retrieved = optimizer->getLODConfig();
    EXPECT_EQ(retrieved.numLevels, -1);
    EXPECT_EQ(retrieved.maxCells, -100);
}

TEST_F(MemoryOptimizerTest, ZeroValues) {
    LODConfig config;
    config.numLevels = 0;
    config.maxCells = 0;
    config.reductionTarget = 0.0;

    optimizer->setLODConfig(config);

    const auto& retrieved = optimizer->getLODConfig();
    EXPECT_EQ(retrieved.numLevels, 0);
    EXPECT_EQ(retrieved.maxCells, 0);
    EXPECT_DOUBLE_EQ(retrieved.reductionTarget, 0.0);
}

TEST_F(MemoryOptimizerTest, ExtremeValues) {
    GPUMemoryConfig config;
    config.maxTextureMemory = 1024ULL * 1024ULL * 1024ULL * 1024ULL;  // 1 TB
    config.maxVBOMemory = 1024ULL * 1024ULL * 1024ULL * 1024ULL;

    optimizer->setGPUMemoryConfig(config);

    const auto& retrieved = optimizer->getGPUMemoryConfig();
    EXPECT_EQ(retrieved.maxTextureMemory, 1024ULL * 1024ULL * 1024ULL * 1024ULL);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
