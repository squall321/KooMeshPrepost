#include "core/SpatialIndexFactory.h"
#include "core/Octree.h"
#include "core/RTree.h"
#include "core/KdTree.h"
#include "core/UniformGrid.h"
#include "core/Mesh.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace koomesh {
namespace core {

std::unique_ptr<ISpatialIndex> SpatialIndexFactory::create(SpatialIndexType type) {
    SpatialIndexConfig config;
    config.type = type;
    return create(config);
}

std::unique_ptr<ISpatialIndex> SpatialIndexFactory::create(const SpatialIndexConfig& config) {
    switch (config.type) {
        case SpatialIndexType::OCTREE: {
            auto octree = std::make_unique<Octree>();
            octree->setMaxDepth(config.octreeMaxDepth);
            octree->setMaxElementsPerNode(config.octreeMaxElementsPerNode);
            return octree;
        }

        case SpatialIndexType::RTREE: {
            auto rtree = std::make_unique<RTree>();
            rtree->setMaxChildren(config.rtreeMaxChildren);
            return rtree;
        }

        case SpatialIndexType::KDTREE: {
            auto kdtree = std::make_unique<KdTree>();
            kdtree->setMaxDepth(config.kdtreeMaxDepth);
            kdtree->setMaxElementsPerNode(config.kdtreeMaxElementsPerLeaf);
            return kdtree;
        }

        case SpatialIndexType::UNIFORM_GRID: {
            auto grid = std::make_unique<UniformGrid>();
            if (config.uniformGridCellsX > 0 && config.uniformGridCellsY > 0 && config.uniformGridCellsZ > 0) {
                // Use fixed cell count mode
                grid->setCellCount(config.uniformGridCellsX, config.uniformGridCellsY, config.uniformGridCellsZ);
            } else if (config.uniformGridCellSize > 0.0) {
                // Use fixed cell size mode
                grid->setCellSize(config.uniformGridCellSize);
            }
            // If neither is set, grid will auto-calculate on build()
            return grid;
        }

        default:
            throw std::invalid_argument("Unknown spatial index type");
    }
}

std::unique_ptr<ISpatialIndex> SpatialIndexFactory::createFromName(const std::string& typeName) {
    SpatialIndexType type = stringToType(typeName);
    return create(type);
}

std::unique_ptr<ISpatialIndex> SpatialIndexFactory::createOptimal(
    const Mesh& mesh,
    SpatialIndexConfig config
) {
    // Override type with optimal selection
    config.type = analyzeAndSelectType(mesh);

    // For uniform grid, calculate optimal cell size if not specified
    if (config.type == SpatialIndexType::UNIFORM_GRID) {
        if (config.uniformGridCellSize <= 0.0 && config.uniformGridCellsX == 0) {
            config.uniformGridCellSize = calculateOptimalCellSize(mesh);
        }
    }

    return create(config);
}

std::string SpatialIndexFactory::typeToString(SpatialIndexType type) {
    switch (type) {
        case SpatialIndexType::OCTREE:
            return "octree";
        case SpatialIndexType::RTREE:
            return "rtree";
        case SpatialIndexType::KDTREE:
            return "kdtree";
        case SpatialIndexType::UNIFORM_GRID:
            return "uniformgrid";
        default:
            return "unknown";
    }
}

SpatialIndexType SpatialIndexFactory::stringToType(const std::string& typeName) {
    // Convert to lowercase for case-insensitive comparison
    std::string lower = typeName;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    if (lower == "octree") {
        return SpatialIndexType::OCTREE;
    } else if (lower == "rtree" || lower == "r-tree") {
        return SpatialIndexType::RTREE;
    } else if (lower == "kdtree" || lower == "kd-tree" || lower == "k-d-tree") {
        return SpatialIndexType::KDTREE;
    } else if (lower == "uniformgrid" || lower == "uniform-grid" || lower == "grid") {
        return SpatialIndexType::UNIFORM_GRID;
    } else {
        throw std::invalid_argument("Unknown spatial index type name: " + typeName);
    }
}

SpatialIndexType SpatialIndexFactory::analyzeAndSelectType(const Mesh& mesh) {
    const size_t elementCount = mesh.elementCount();
    const size_t nodeCount = mesh.nodeCount();

    // For very small meshes, use simple octree
    if (elementCount < 100) {
        return SpatialIndexType::OCTREE;
    }

    // Calculate mesh bounding box and distribution metrics
    if (nodeCount == 0) {
        return SpatialIndexType::OCTREE; // Default fallback
    }

    // Get first node to initialize bounds
    Eigen::Vector3d minPt(std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max());
    Eigen::Vector3d maxPt(std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest());

    // Calculate bounding box
    for (const auto& pair : mesh.nodes()) {
        const Node& node = pair.second;
        const Eigen::Vector3d& pos = node.coordinates();
        minPt.x() = std::min(minPt.x(), pos.x());
        minPt.y() = std::min(minPt.y(), pos.y());
        minPt.z() = std::min(minPt.z(), pos.z());
        maxPt.x() = std::max(maxPt.x(), pos.x());
        maxPt.y() = std::max(maxPt.y(), pos.y());
        maxPt.z() = std::max(maxPt.z(), pos.z());
    }

    Eigen::Vector3d size = maxPt - minPt;
    double volume = size.x() * size.y() * size.z();

    // Calculate aspect ratio (how box-like vs. elongated the mesh is)
    double maxDim = std::max({size.x(), size.y(), size.z()});
    double minDim = std::min({size.x(), size.y(), size.z()});
    double aspectRatio = maxDim / (minDim > 0.0 ? minDim : 1.0);

    // Selection heuristics:

    // 1. For large, uniformly distributed meshes, use Uniform Grid
    //    - Good for large element counts with relatively uniform density
    //    - Best for point and range queries on regular meshes
    if (elementCount > 10000 && aspectRatio < 10.0) {
        // Check if distribution is relatively uniform by sampling
        // For now, use uniform grid for large regular meshes
        return SpatialIndexType::UNIFORM_GRID;
    }

    // 2. For very elongated meshes (pipes, beams), use K-d Tree
    //    - Better handles anisotropic distributions
    //    - Splits along longest axis
    if (aspectRatio > 20.0) {
        return SpatialIndexType::KDTREE;
    }

    // 3. For medium-sized meshes with good aspect ratio, use R-Tree
    //    - Excellent for range queries
    //    - Good balance between construction and query time
    if (elementCount > 1000 && elementCount < 10000 && aspectRatio < 5.0) {
        return SpatialIndexType::RTREE;
    }

    // 4. For hierarchical operations and medium meshes, use K-d Tree
    //    - Very good for nearest neighbor queries
    //    - Fast construction
    if (elementCount > 500) {
        return SpatialIndexType::KDTREE;
    }

    // 5. Default to Octree for general purpose use
    //    - Good all-around performance
    //    - Intuitive spatial subdivision
    return SpatialIndexType::OCTREE;
}

double SpatialIndexFactory::calculateOptimalCellSize(const Mesh& mesh) {
    const size_t elementCount = mesh.elementCount();
    const size_t nodeCount = mesh.nodeCount();

    if (nodeCount == 0) {
        return 1.0; // Default fallback
    }

    // Calculate bounding box
    Eigen::Vector3d minPt(std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::max());
    Eigen::Vector3d maxPt(std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::lowest());

    for (const auto& pair : mesh.nodes()) {
        const Node& node = pair.second;
        const Eigen::Vector3d& pos = node.coordinates();
        minPt.x() = std::min(minPt.x(), pos.x());
        minPt.y() = std::min(minPt.y(), pos.y());
        minPt.z() = std::min(minPt.z(), pos.z());
        maxPt.x() = std::max(maxPt.x(), pos.x());
        maxPt.y() = std::max(maxPt.y(), pos.y());
        maxPt.z() = std::max(maxPt.z(), pos.z());
    }

    Eigen::Vector3d size = maxPt - minPt;
    double volume = size.x() * size.y() * size.z();

    if (volume <= 0.0) {
        return 1.0;
    }

    // Target: approximately 10-20 elements per cell on average
    const double targetElementsPerCell = 15.0;

    // Calculate number of cells needed
    double cellCount = static_cast<double>(elementCount) / targetElementsPerCell;

    // Calculate cell size (assuming cubic cells)
    double cellVolume = volume / cellCount;
    double cellSize = std::cbrt(cellVolume);

    // Clamp to reasonable range (1/100 to 1/5 of smallest dimension)
    double minDim = std::min({size.x(), size.y(), size.z()});
    double minCellSize = minDim / 100.0;
    double maxCellSize = minDim / 5.0;

    cellSize = std::max(minCellSize, std::min(maxCellSize, cellSize));

    return cellSize;
}

} // namespace core
} // namespace koomesh
