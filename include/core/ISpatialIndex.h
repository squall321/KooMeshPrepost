#pragma once

#include "core/Types.h"
#include "core/BoundingBox.h"
#include <vector>
#include <memory>

namespace koomesh {
namespace core {

// Forward declaration
class Mesh;

/**
 * @brief Spatial index interface for fast spatial queries
 *
 * Provides an abstract interface for spatial data structures
 * that accelerate queries like:
 * - Find elements in a region
 * - Find nearest element/node
 * - Ray intersection tests
 *
 * Implementations:
 * - Octree: Good for uniform distributions
 * - R-Tree: Good for dynamic data
 * - K-d Tree: Good for point queries
 * - Uniform Grid: Very fast for uniform data
 *
 * Example usage:
 * @code
 * auto octree = std::make_unique<Octree>();
 * octree->build(mesh);
 *
 * // Find elements in bounding box
 * BoundingBox queryBox(min, max);
 * auto elements = octree->query(queryBox);
 *
 * // Find nearest element
 * Eigen::Vector3d point(1.0, 2.0, 3.0);
 * ElementId nearest = octree->findNearest(point);
 * @endcode
 */
class ISpatialIndex {
public:
    virtual ~ISpatialIndex() = default;

    /**
     * @brief Build spatial index from mesh
     *
     * Constructs the spatial data structure from mesh data.
     * Must be called before any queries.
     *
     * @param mesh Source mesh
     */
    virtual void build(const Mesh& mesh) = 0;

    /**
     * @brief Query elements in bounding box
     *
     * Returns all elements whose bounding boxes intersect
     * the query bounding box.
     *
     * @param box Query bounding box
     * @return Vector of element IDs
     */
    virtual std::vector<ElementId> query(const BoundingBox& box) const = 0;

    /**
     * @brief Query elements containing or near a point
     *
     * Returns elements that contain or are close to the query point.
     *
     * @param point Query point
     * @return Vector of element IDs
     */
    virtual std::vector<ElementId> queryPoint(const Eigen::Vector3d& point) const = 0;

    /**
     * @brief Find nearest element to a point
     *
     * Returns the ID of the element whose center/closest point
     * is nearest to the query point.
     *
     * @param point Query point
     * @return Element ID, or 0 if index is empty
     */
    virtual ElementId findNearest(const Eigen::Vector3d& point) const = 0;

    /**
     * @brief Find k-nearest elements to a point
     *
     * Returns the IDs of the k elements nearest to the query point.
     *
     * @param point Query point
     * @param k Number of nearest elements
     * @return Vector of element IDs (may be less than k if not enough elements)
     */
    virtual std::vector<ElementId> findKNearest(
        const Eigen::Vector3d& point,
        size_t k) const = 0;

    /**
     * @brief Find elements within radius of a point
     *
     * Returns all elements within the specified radius of the query point.
     *
     * @param point Query point (center)
     * @param radius Search radius
     * @return Vector of element IDs
     */
    virtual std::vector<ElementId> findWithinRadius(
        const Eigen::Vector3d& point,
        double radius) const = 0;

    /**
     * @brief Ray intersection query
     *
     * Returns elements that intersect with the ray.
     *
     * @param origin Ray origin point
     * @param direction Ray direction (should be normalized)
     * @return Vector of element IDs
     */
    virtual std::vector<ElementId> rayIntersect(
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction) const = 0;

    /**
     * @brief Clear the spatial index
     *
     * Frees memory and resets the index to empty state.
     */
    virtual void clear() = 0;

    /**
     * @brief Check if index is built
     *
     * @return true if index is ready for queries
     */
    virtual bool isBuilt() const = 0;

    /**
     * @brief Get number of indexed elements
     *
     * @return Element count
     */
    virtual size_t elementCount() const = 0;

    /**
     * @brief Get memory usage in bytes
     *
     * Estimate of memory used by the index structure.
     *
     * @return Memory usage in bytes
     */
    virtual size_t memoryUsage() const = 0;

    /**
     * @brief Get bounding box of entire index
     *
     * @return Bounding box containing all indexed elements
     */
    virtual BoundingBox boundingBox() const = 0;

    /**
     * @brief Get index statistics (for debugging/optimization)
     *
     * Returns implementation-specific statistics.
     *
     * @return Statistics as string (format varies by implementation)
     */
    virtual std::string getStatistics() const = 0;
};

/**
 * @brief Query result with distance information
 */
struct SpatialQueryResult {
    ElementId elementId;
    double distance;  ///< Distance from query point

    bool operator<(const SpatialQueryResult& other) const {
        return distance < other.distance;
    }
};

} // namespace core
} // namespace koomesh
