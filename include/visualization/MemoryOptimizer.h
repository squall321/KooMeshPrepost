/**
 * @file MemoryOptimizer.h
 * @brief Memory optimization for VTK visualization
 */

#pragma once

#include <memory>
#include <string>
#include <sstream>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkMapper.h>
#include <vtkLODActor.h>
#include <vtkQuadricClustering.h>
#include <vtkDecimatePro.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Level of Detail (LOD) configuration
 */
struct LODConfig {
    bool enabled = true;
    int numLevels = 3;              // Number of LOD levels
    double highResThreshold = 0.1;   // Distance threshold for high res
    double mediumResThreshold = 0.5; // Distance threshold for medium res
    double reductionTarget = 0.5;    // Target reduction for decimation (0-1)
    int maxCells = 1000000;          // Maximum cells per level
};

/**
 * @brief Streaming configuration for large datasets
 */
struct StreamingConfig {
    bool enabled = false;
    int numberOfPieces = 4;          // Number of streaming pieces
    int updateRate = 100;            // Update rate in milliseconds
    bool prioritizeViewport = true;  // Stream visible data first
};

/**
 * @brief GPU memory management configuration
 */
struct GPUMemoryConfig {
    bool trackUsage = true;
    size_t maxTextureMemory = 512 * 1024 * 1024;  // 512 MB default
    size_t maxVBOMemory = 1024 * 1024 * 1024;     // 1 GB default
    bool useDisplayLists = false;                  // Deprecated in modern OpenGL
    bool useVBOs = true;                           // Vertex Buffer Objects
};

/**
 * @brief Memory usage statistics
 */
struct MemoryStats {
    size_t totalMemoryUsed = 0;
    size_t vtkObjectMemory = 0;
    size_t textureMemory = 0;
    size_t vboMemory = 0;
    int numActors = 0;
    int numPolyData = 0;
    int totalCells = 0;
    int totalPoints = 0;
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Memory optimization manager for VTK visualization
 *
 * Provides functionality for:
 * - Level of Detail (LOD) management for large meshes
 * - Streaming support for huge datasets
 * - GPU memory tracking and management
 * - Automatic decimation and simplification
 * - Memory usage statistics
 */
class MemoryOptimizer {
public:
    /**
     * @brief Constructor
     */
    MemoryOptimizer();

    /**
     * @brief Destructor
     */
    ~MemoryOptimizer();

    // ========================================================================
    // LOD Management
    // ========================================================================

    /**
     * @brief Create LOD actor from poly data
     * @param polyData Input poly data
     * @param config LOD configuration
     * @return LOD actor with multiple resolution levels
     */
    vtkSmartPointer<vtkLODActor> createLODActor(
        vtkPolyData* polyData,
        const LODConfig& config = LODConfig()
    );

    /**
     * @brief Decimate poly data to target reduction
     * @param polyData Input poly data
     * @param targetReduction Target reduction factor (0-1)
     * @return Decimated poly data
     */
    vtkSmartPointer<vtkPolyData> decimatePolyData(
        vtkPolyData* polyData,
        double targetReduction
    );

    /**
     * @brief Create multi-resolution version of poly data
     * @param polyData Input poly data
     * @param numLevels Number of LOD levels
     * @return Vector of poly data at different resolutions
     */
    std::vector<vtkSmartPointer<vtkPolyData>> createLODLevels(
        vtkPolyData* polyData,
        int numLevels
    );

    /**
     * @brief Set LOD configuration
     * @param config LOD configuration
     */
    void setLODConfig(const LODConfig& config);

    /**
     * @brief Get LOD configuration
     * @return Current LOD configuration
     */
    const LODConfig& getLODConfig() const;

    // ========================================================================
    // Streaming Support
    // ========================================================================

    /**
     * @brief Enable streaming for large dataset
     * @param config Streaming configuration
     */
    void enableStreaming(const StreamingConfig& config);

    /**
     * @brief Disable streaming
     */
    void disableStreaming();

    /**
     * @brief Check if streaming is enabled
     * @return True if streaming enabled
     */
    bool isStreamingEnabled() const;

    /**
     * @brief Set streaming configuration
     * @param config Streaming configuration
     */
    void setStreamingConfig(const StreamingConfig& config);

    /**
     * @brief Get streaming configuration
     * @return Current streaming configuration
     */
    const StreamingConfig& getStreamingConfig() const;

    // ========================================================================
    // GPU Memory Management
    // ========================================================================

    /**
     * @brief Set GPU memory limits
     * @param config GPU memory configuration
     */
    void setGPUMemoryConfig(const GPUMemoryConfig& config);

    /**
     * @brief Get GPU memory configuration
     * @return Current GPU memory configuration
     */
    const GPUMemoryConfig& getGPUMemoryConfig() const;

    /**
     * @brief Estimate memory usage for poly data
     * @param polyData Poly data to estimate
     * @return Estimated memory in bytes
     */
    size_t estimateMemoryUsage(vtkPolyData* polyData) const;

    /**
     * @brief Optimize actor for GPU rendering
     * @param actor Actor to optimize
     */
    void optimizeActorForGPU(vtkActor* actor);

    // ========================================================================
    // Memory Statistics
    // ========================================================================

    /**
     * @brief Get current memory statistics
     * @return Memory statistics
     */
    MemoryStats getMemoryStats() const;

    /**
     * @brief Update memory statistics
     */
    void updateMemoryStats();

    /**
     * @brief Clear all cached data
     */
    void clearCache();

    /**
     * @brief Get statistics string
     * @return Statistics string
     */
    std::string getStatistics() const;

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Check if poly data is large
     * @param polyData Poly data to check
     * @param threshold Cell count threshold
     * @return True if large
     */
    bool isLargeDataset(vtkPolyData* polyData, int threshold = 1000000) const;

    /**
     * @brief Get recommended number of LOD levels
     * @param cellCount Number of cells
     * @return Recommended LOD levels
     */
    int getRecommendedLODLevels(int cellCount) const;

private:
    LODConfig m_lodConfig;
    StreamingConfig m_streamingConfig;
    GPUMemoryConfig m_gpuConfig;
    MemoryStats m_memoryStats;

    /**
     * @brief Create decimated version using quadric clustering
     * @param polyData Input data
     * @param divisions Number of divisions
     * @return Decimated poly data
     */
    vtkSmartPointer<vtkPolyData> quadricDecimate(vtkPolyData* polyData, int divisions);
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class MemoryOptimizer {
public:
    MemoryOptimizer() {}
    ~MemoryOptimizer() {}

    void* createLODActor(void*, const LODConfig& = LODConfig()) {
        return nullptr;
    }

    void* decimatePolyData(void*, double) {
        return nullptr;
    }

    std::vector<void*> createLODLevels(void*, int numLevels) {
        if (numLevels <= 0) {
            return std::vector<void*>();
        }
        return std::vector<void*>(static_cast<size_t>(numLevels), nullptr);
    }

    void setLODConfig(const LODConfig& config) {
        m_lodConfig = config;
    }

    const LODConfig& getLODConfig() const {
        return m_lodConfig;
    }

    void enableStreaming(const StreamingConfig& config) {
        m_streamingConfig = config;
        m_streamingConfig.enabled = true;
    }

    void disableStreaming() {
        m_streamingConfig.enabled = false;
    }

    bool isStreamingEnabled() const {
        return m_streamingConfig.enabled;
    }

    void setStreamingConfig(const StreamingConfig& config) {
        m_streamingConfig = config;
    }

    const StreamingConfig& getStreamingConfig() const {
        return m_streamingConfig;
    }

    void setGPUMemoryConfig(const GPUMemoryConfig& config) {
        m_gpuConfig = config;
    }

    const GPUMemoryConfig& getGPUMemoryConfig() const {
        return m_gpuConfig;
    }

    size_t estimateMemoryUsage(void*) const {
        return 0;
    }

    void optimizeActorForGPU(void*) {}

    MemoryStats getMemoryStats() const {
        return m_memoryStats;
    }

    void updateMemoryStats() {}

    void clearCache() {}

    std::string getStatistics() const {
        std::ostringstream oss;
        oss << "Memory Optimizer Statistics (VTK not available):\n";
        oss << "  LOD Enabled: " << (m_lodConfig.enabled ? "Yes" : "No") << "\n";
        oss << "  Streaming Enabled: " << (m_streamingConfig.enabled ? "Yes" : "No") << "\n";
        oss << "  GPU Tracking: " << (m_gpuConfig.trackUsage ? "Yes" : "No") << "\n";
        return oss.str();
    }

    bool isLargeDataset(void*, int = 1000000) const {
        return false;
    }

    int getRecommendedLODLevels(int cellCount) const {
        if (cellCount < 10000) return 1;
        if (cellCount < 100000) return 2;
        if (cellCount < 1000000) return 3;
        return 4;
    }

private:
    LODConfig m_lodConfig;
    StreamingConfig m_streamingConfig;
    GPUMemoryConfig m_gpuConfig;
    MemoryStats m_memoryStats;
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
