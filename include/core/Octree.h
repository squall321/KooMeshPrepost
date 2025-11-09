#pragma once

#include "core/ISpatialIndex.h"
#include <Eigen/Dense>
#include <vector>
#include <memory>
#include <unordered_map>

namespace koomesh {
namespace core {

// Forward declaration
class Mesh;

/**
 * @brief Octree spatial index implementation
 *
 * Octree recursively subdivides 3D space into 8 octants.
 * Good for uniformly distributed data.
 *
 * Properties:
 * - O(log n) query time for well-balanced trees
 * - O(n) build time
 * - Adaptive subdivision based on element density
 *
 * Example:
 * @code
 * Octree octree;
 * octree.setMaxDepth(8);
 * octree.setMaxElementsPerNode(10);
 * octree.build(mesh);
 *
 * BoundingBox queryBox(min, max);
 * auto elements = octree.query(queryBox);
 * @endcode
 */
class Octree : public ISpatialIndex {
public:
    /**
     * @brief Constructor with default parameters
     */
    Octree();

    /**
     * @brief Constructor with custom parameters
     *
     * @param maxDepth Maximum tree depth
     * @param maxElementsPerNode Maximum elements per leaf node
     */
    Octree(size_t maxDepth, size_t maxElementsPerNode);

    /**
     * @brief Destructor
     */
    ~Octree() override = default;

    // ISpatialIndex interface implementation
    void build(const Mesh& mesh) override;
    std::vector<ElementId> query(const BoundingBox& box) const override;
    std::vector<ElementId> queryPoint(const Eigen::Vector3d& point) const override;
    ElementId findNearest(const Eigen::Vector3d& point) const override;
    std::vector<ElementId> findKNearest(const Eigen::Vector3d& point, size_t k) const override;
    std::vector<ElementId> findWithinRadius(const Eigen::Vector3d& point, double radius) const override;
    std::vector<ElementId> rayIntersect(const Eigen::Vector3d& origin, const Eigen::Vector3d& direction) const override;
    void clear() override;
    bool isBuilt() const override;
    size_t elementCount() const override;
    size_t memoryUsage() const override;
    BoundingBox boundingBox() const override;
    std::string getStatistics() const override;

    // Octree-specific configuration
    void setMaxDepth(size_t depth) { m_maxDepth = depth; }
    void setMaxElementsPerNode(size_t count) { m_maxElementsPerNode = count; }
    size_t getMaxDepth() const { return m_maxDepth; }
    size_t getMaxElementsPerNode() const { return m_maxElementsPerNode; }

private:
    /**
     * @brief Octree node structure
     */
    struct OctreeNode {
        BoundingBox bounds;
        std::vector<ElementId> elements;  // Only for leaf nodes
        std::unique_ptr<OctreeNode> children[8];  // Only for internal nodes
        bool isLeaf;

        OctreeNode(const BoundingBox& bbox) : bounds(bbox), isLeaf(true) {}
    };

    /**
     * @brief Build octree recursively
     */
    void buildRecursive(
        OctreeNode* node,
        const std::vector<ElementId>& elementIds,
        size_t depth);

    /**
     * @brief Subdivide node into 8 children
     */
    void subdivide(OctreeNode* node);

    /**
     * @brief Get octant index for element
     */
    int getOctant(const BoundingBox& nodeBounds, const BoundingBox& elementBounds) const;

    /**
     * @brief Query recursively
     */
    void queryRecursive(
        const OctreeNode* node,
        const BoundingBox& queryBox,
        std::vector<ElementId>& results) const;

    /**
     * @brief Query point recursively
     */
    void queryPointRecursive(
        const OctreeNode* node,
        const Eigen::Vector3d& point,
        std::vector<ElementId>& results) const;

    /**
     * @brief Find nearest element recursively
     */
    void findNearestRecursive(
        const OctreeNode* node,
        const Eigen::Vector3d& point,
        ElementId& nearestId,
        double& nearestDistSq) const;

    /**
     * @brief Find k-nearest elements recursively
     */
    void findKNearestRecursive(
        const OctreeNode* node,
        const Eigen::Vector3d& point,
        std::vector<SpatialQueryResult>& results,
        size_t k) const;

    /**
     * @brief Ray-box intersection test
     */
    bool rayBoxIntersect(
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction,
        const BoundingBox& box) const;

    /**
     * @brief Ray intersection recursively
     */
    void rayIntersectRecursive(
        const OctreeNode* node,
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction,
        std::vector<ElementId>& results) const;

    /**
     * @brief Calculate memory usage recursively
     */
    size_t memoryUsageRecursive(const OctreeNode* node) const;

    /**
     * @brief Collect tree statistics recursively
     */
    void collectStatistics(
        const OctreeNode* node,
        size_t depth,
        size_t& nodeCount,
        size_t& leafCount,
        size_t& maxDepthFound,
        size_t& totalElements) const;

    // Tree structure
    std::unique_ptr<OctreeNode> m_root;
    BoundingBox m_bounds;

    // Element bounds cache
    std::unordered_map<ElementId, BoundingBox> m_elementBounds;

    // Configuration
    size_t m_maxDepth;
    size_t m_maxElementsPerNode;

    // Statistics
    size_t m_elementCount;
    bool m_isBuilt;
};

} // namespace core
} // namespace koomesh
