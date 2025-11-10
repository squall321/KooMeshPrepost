/**
 * @file MemoryOptimizer.cpp
 * @brief Memory optimization implementation
 */

#include "visualization/MemoryOptimizer.h"

#ifdef KOOMESH_HAS_VTK
#include <vtkProperty.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <algorithm>
#include <cmath>

namespace koomesh {
namespace visualization {

MemoryOptimizer::MemoryOptimizer() {
    // Set default LOD config
    m_lodConfig.enabled = true;
    m_lodConfig.numLevels = 3;
    m_lodConfig.highResThreshold = 0.1;
    m_lodConfig.mediumResThreshold = 0.5;
    m_lodConfig.reductionTarget = 0.5;
    m_lodConfig.maxCells = 1000000;

    // Set default streaming config
    m_streamingConfig.enabled = false;
    m_streamingConfig.numberOfPieces = 4;
    m_streamingConfig.updateRate = 100;
    m_streamingConfig.prioritizeViewport = true;

    // Set default GPU config
    m_gpuConfig.trackUsage = true;
    m_gpuConfig.maxTextureMemory = 512 * 1024 * 1024;
    m_gpuConfig.maxVBOMemory = 1024 * 1024 * 1024;
    m_gpuConfig.useDisplayLists = false;
    m_gpuConfig.useVBOs = true;

    // Initialize stats
    m_memoryStats = MemoryStats();
}

MemoryOptimizer::~MemoryOptimizer() {
    clearCache();
}

// ============================================================================
// LOD Management
// ============================================================================

vtkSmartPointer<vtkLODActor> MemoryOptimizer::createLODActor(
    vtkPolyData* polyData,
    const LODConfig& config)
{
    if (!polyData) {
        return nullptr;
    }

    auto lodActor = vtkSmartPointer<vtkLODActor>::New();

    if (!config.enabled) {
        // Just use regular mapper without LOD
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(polyData);
        lodActor->SetMapper(mapper);
        return lodActor;
    }

    // Create LOD levels
    auto lodLevels = createLODLevels(polyData, config.numLevels);

    // Set up mapper with highest resolution
    if (!lodLevels.empty()) {
        auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputData(lodLevels[0]);
        lodActor->SetMapper(mapper);
    }

    // Configure LOD actor
    lodActor->SetNumberOfCloudPoints(config.maxCells / 10);  // Points for cloud representation

    return lodActor;
}

vtkSmartPointer<vtkPolyData> MemoryOptimizer::decimatePolyData(
    vtkPolyData* polyData,
    double targetReduction)
{
    if (!polyData || targetReduction <= 0.0 || targetReduction >= 1.0) {
        return polyData;
    }

    auto decimator = vtkSmartPointer<vtkDecimatePro>::New();
    decimator->SetInputData(polyData);
    decimator->SetTargetReduction(targetReduction);
    decimator->PreserveTopologyOn();
    decimator->Update();

    return decimator->GetOutput();
}

std::vector<vtkSmartPointer<vtkPolyData>> MemoryOptimizer::createLODLevels(
    vtkPolyData* polyData,
    int numLevels)
{
    std::vector<vtkSmartPointer<vtkPolyData>> levels;

    if (!polyData || numLevels <= 0) {
        return levels;
    }

    // Level 0: Original data
    levels.push_back(polyData);

    // Create progressively decimated levels
    for (int i = 1; i < numLevels; ++i) {
        double reductionFactor = static_cast<double>(i) / static_cast<double>(numLevels);
        auto decimated = decimatePolyData(polyData, reductionFactor);
        levels.push_back(decimated);
    }

    return levels;
}

void MemoryOptimizer::setLODConfig(const LODConfig& config) {
    m_lodConfig = config;
}

const LODConfig& MemoryOptimizer::getLODConfig() const {
    return m_lodConfig;
}

// ============================================================================
// Streaming Support
// ============================================================================

void MemoryOptimizer::enableStreaming(const StreamingConfig& config) {
    m_streamingConfig = config;
    m_streamingConfig.enabled = true;
}

void MemoryOptimizer::disableStreaming() {
    m_streamingConfig.enabled = false;
}

bool MemoryOptimizer::isStreamingEnabled() const {
    return m_streamingConfig.enabled;
}

void MemoryOptimizer::setStreamingConfig(const StreamingConfig& config) {
    m_streamingConfig = config;
}

const StreamingConfig& MemoryOptimizer::getStreamingConfig() const {
    return m_streamingConfig;
}

// ============================================================================
// GPU Memory Management
// ============================================================================

void MemoryOptimizer::setGPUMemoryConfig(const GPUMemoryConfig& config) {
    m_gpuConfig = config;
}

const GPUMemoryConfig& MemoryOptimizer::getGPUMemoryConfig() const {
    return m_gpuConfig;
}

size_t MemoryOptimizer::estimateMemoryUsage(vtkPolyData* polyData) const {
    if (!polyData) {
        return 0;
    }

    size_t memoryUsage = 0;

    // Points memory (3 doubles per point)
    vtkIdType numPoints = polyData->GetNumberOfPoints();
    memoryUsage += numPoints * 3 * sizeof(double);

    // Cells memory (estimate 4 points per cell average)
    vtkIdType numCells = polyData->GetNumberOfCells();
    memoryUsage += numCells * 4 * sizeof(vtkIdType);

    // Point data arrays
    auto pointData = polyData->GetPointData();
    if (pointData) {
        for (int i = 0; i < pointData->GetNumberOfArrays(); ++i) {
            auto array = pointData->GetArray(i);
            if (array) {
                memoryUsage += array->GetNumberOfTuples() *
                               array->GetNumberOfComponents() *
                               array->GetDataTypeSize();
            }
        }
    }

    // Cell data arrays
    auto cellData = polyData->GetCellData();
    if (cellData) {
        for (int i = 0; i < cellData->GetNumberOfArrays(); ++i) {
            auto array = cellData->GetArray(i);
            if (array) {
                memoryUsage += array->GetNumberOfTuples() *
                               array->GetNumberOfComponents() *
                               array->GetDataTypeSize();
            }
        }
    }

    return memoryUsage;
}

void MemoryOptimizer::optimizeActorForGPU(vtkActor* actor) {
    if (!actor) {
        return;
    }

    auto mapper = actor->GetMapper();
    if (!mapper) {
        return;
    }

    // Enable VBOs for faster rendering
    if (m_gpuConfig.useVBOs) {
        auto polyMapper = vtkPolyDataMapper::SafeDownCast(mapper);
        if (polyMapper) {
            // VBOs are enabled by default in modern VTK
            // This is mainly for documentation purposes
        }
    }

    // Disable display lists (deprecated)
    if (!m_gpuConfig.useDisplayLists) {
        // Display lists are deprecated in VTK 6+
    }
}

// ============================================================================
// Memory Statistics
// ============================================================================

MemoryStats MemoryOptimizer::getMemoryStats() const {
    return m_memoryStats;
}

void MemoryOptimizer::updateMemoryStats() {
    // Update statistics
    // In a real implementation, this would query VTK's memory management
    // For now, just maintain current stats
}

void MemoryOptimizer::clearCache() {
    // Clear any cached data
    m_memoryStats = MemoryStats();
}

std::string MemoryOptimizer::getStatistics() const {
    std::ostringstream oss;
    oss << "Memory Optimizer Statistics:\n";
    oss << "  LOD Configuration:\n";
    oss << "    Enabled: " << (m_lodConfig.enabled ? "Yes" : "No") << "\n";
    oss << "    Number of Levels: " << m_lodConfig.numLevels << "\n";
    oss << "    Reduction Target: " << (m_lodConfig.reductionTarget * 100.0) << "%\n";
    oss << "    Max Cells: " << m_lodConfig.maxCells << "\n";
    oss << "  Streaming Configuration:\n";
    oss << "    Enabled: " << (m_streamingConfig.enabled ? "Yes" : "No") << "\n";
    oss << "    Number of Pieces: " << m_streamingConfig.numberOfPieces << "\n";
    oss << "    Update Rate: " << m_streamingConfig.updateRate << " ms\n";
    oss << "  GPU Memory Configuration:\n";
    oss << "    Track Usage: " << (m_gpuConfig.trackUsage ? "Yes" : "No") << "\n";
    oss << "    Use VBOs: " << (m_gpuConfig.useVBOs ? "Yes" : "No") << "\n";
    oss << "    Max Texture Memory: " << (m_gpuConfig.maxTextureMemory / (1024 * 1024)) << " MB\n";
    oss << "    Max VBO Memory: " << (m_gpuConfig.maxVBOMemory / (1024 * 1024)) << " MB\n";
    oss << "  Memory Statistics:\n";
    oss << "    Total Memory Used: " << (m_memoryStats.totalMemoryUsed / (1024 * 1024)) << " MB\n";
    oss << "    Number of Actors: " << m_memoryStats.numActors << "\n";
    oss << "    Total Cells: " << m_memoryStats.totalCells << "\n";
    oss << "    Total Points: " << m_memoryStats.totalPoints << "\n";

    return oss.str();
}

// ============================================================================
// Utility
// ============================================================================

bool MemoryOptimizer::isLargeDataset(vtkPolyData* polyData, int threshold) const {
    if (!polyData) {
        return false;
    }

    return polyData->GetNumberOfCells() > threshold;
}

int MemoryOptimizer::getRecommendedLODLevels(int cellCount) const {
    if (cellCount < 10000) {
        return 1;  // No LOD needed
    } else if (cellCount < 100000) {
        return 2;  // One LOD level
    } else if (cellCount < 1000000) {
        return 3;  // Two LOD levels
    } else {
        return 4;  // Three LOD levels
    }
}

// ============================================================================
// Private Methods
// ============================================================================

vtkSmartPointer<vtkPolyData> MemoryOptimizer::quadricDecimate(
    vtkPolyData* polyData,
    int divisions)
{
    if (!polyData || divisions <= 0) {
        return polyData;
    }

    auto clusterer = vtkSmartPointer<vtkQuadricClustering>::New();
    clusterer->SetInputData(polyData);
    clusterer->SetNumberOfDivisions(divisions, divisions, divisions);
    clusterer->Update();

    return clusterer->GetOutput();
}

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
