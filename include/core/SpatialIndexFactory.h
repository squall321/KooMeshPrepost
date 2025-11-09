#pragma once

#include "core/ISpatialIndex.h"
#include <memory>
#include <string>

namespace koomesh {
namespace core {

/**
 * @brief Enumeration of available spatial index types
 */
enum class SpatialIndexType {
    OCTREE,      ///< Octree spatial index
    RTREE,       ///< R-Tree spatial index
    KDTREE,      ///< K-d Tree spatial index
    UNIFORM_GRID ///< Uniform Grid spatial index
};

/**
 * @brief Configuration for spatial index creation
 */
struct SpatialIndexConfig {
    SpatialIndexType type = SpatialIndexType::OCTREE;

    // Octree configuration
    size_t octreeMaxDepth = 8;
    size_t octreeMaxElementsPerNode = 10;

    // R-Tree configuration
    size_t rtreeMaxChildren = 8;
    size_t rtreeMinChildren = 3;

    // K-d Tree configuration
    size_t kdtreeMaxDepth = 20;
    size_t kdtreeMaxElementsPerLeaf = 10;

    // Uniform Grid configuration
    double uniformGridCellSize = 0.0;  // 0 means auto-calculate
    size_t uniformGridCellsX = 0;      // 0 means use cell size
    size_t uniformGridCellsY = 0;
    size_t uniformGridCellsZ = 0;

    SpatialIndexConfig() = default;
};

/**
 * @brief Factory for creating spatial index instances
 *
 * This factory provides methods for creating different types of spatial indexes
 * with appropriate configuration. It supports both explicit type selection and
 * automatic type selection based on mesh characteristics.
 *
 * Example usage:
 * @code
 * // Create with default config
 * auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
 *
 * // Create with custom config
 * SpatialIndexConfig config;
 * config.type = SpatialIndexType::RTREE;
 * config.rtreeMaxChildren = 16;
 * auto rtree = SpatialIndexFactory::create(config);
 *
 * // Auto-select based on mesh
 * auto autoIndex = SpatialIndexFactory::createOptimal(mesh);
 * @endcode
 */
class SpatialIndexFactory {
public:
    /**
     * @brief Create a spatial index of the specified type with default configuration
     * @param type The type of spatial index to create
     * @return Unique pointer to the created spatial index
     */
    static std::unique_ptr<ISpatialIndex> create(SpatialIndexType type);

    /**
     * @brief Create a spatial index using the provided configuration
     * @param config Configuration specifying type and parameters
     * @return Unique pointer to the created spatial index
     */
    static std::unique_ptr<ISpatialIndex> create(const SpatialIndexConfig& config);

    /**
     * @brief Create a spatial index from a type name string
     * @param typeName Name of the spatial index type ("octree", "rtree", "kdtree", "uniformgrid")
     * @return Unique pointer to the created spatial index
     * @throws std::invalid_argument if typeName is not recognized
     */
    static std::unique_ptr<ISpatialIndex> createFromName(const std::string& typeName);

    /**
     * @brief Automatically select and create the optimal spatial index for the given mesh
     *
     * Selection heuristics:
     * - Uniform Grid: For large meshes with uniform distribution
     * - K-d Tree: For point queries and nearest neighbor searches
     * - R-Tree: For range queries on irregular meshes
     * - Octree: General purpose, good for hierarchical operations
     *
     * @param mesh The mesh to analyze
     * @param config Optional configuration (type will be overridden with optimal choice)
     * @return Unique pointer to the created spatial index
     */
    static std::unique_ptr<ISpatialIndex> createOptimal(
        const Mesh& mesh,
        SpatialIndexConfig config = SpatialIndexConfig()
    );

    /**
     * @brief Convert spatial index type to string name
     * @param type The spatial index type
     * @return String name of the type
     */
    static std::string typeToString(SpatialIndexType type);

    /**
     * @brief Convert string name to spatial index type
     * @param typeName The string name
     * @return The spatial index type
     * @throws std::invalid_argument if typeName is not recognized
     */
    static SpatialIndexType stringToType(const std::string& typeName);

private:
    /**
     * @brief Analyze mesh characteristics to determine optimal index type
     * @param mesh The mesh to analyze
     * @return Recommended spatial index type
     */
    static SpatialIndexType analyzeAndSelectType(const Mesh& mesh);

    /**
     * @brief Calculate optimal cell size for uniform grid based on mesh
     * @param mesh The mesh to analyze
     * @return Recommended cell size
     */
    static double calculateOptimalCellSize(const Mesh& mesh);
};

} // namespace core
} // namespace koomesh
