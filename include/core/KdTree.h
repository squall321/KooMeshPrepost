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
 * @brief K-d Tree spatial index implementation
 *
 * K-d Tree is a binary search tree for organizing points in k-dimensional space.
 * Each level splits along a different axis (cycling through x, y, z).
 *
 * Properties:
 * - O(log n) query time for well-balanced trees
 * - O(n log n) build time
 * - Excellent for point queries and nearest neighbor searches
 * - Uses median-based partitioning for balanced trees
 *
 * Example:
 * @code
 * KdTree kdtree;
 * kdtree.setMaxDepth(20);
 * kdtree.setMaxElementsPerNode(5);
 * kdtree.build(mesh);
 *
 * Eigen::Vector3d point(1.0, 2.0, 3.0);
 * ElementId nearest = kdtree.findNearest(point);
 * @endcode
 */
class KdTree : public ISpatialIndex {
public:
    /**
     * @brief Constructor with default parameters
     */
    KdTree();

    /**
     * @brief Constructor with custom parameters
     *
     * @param maxDepth Maximum tree depth
     * @param maxElementsPerNode Maximum elements per leaf node
     */
    KdTree(size_t maxDepth, size_t maxElementsPerNode);

    /**
     * @brief Destructor
     */
    ~KdTree() override = default;

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

    // K-d Tree specific configuration
    void setMaxDepth(size_t depth) { m_maxDepth = depth; }
    void setMaxElementsPerNode(size_t count) { m_maxElementsPerNode = count; }
    size_t getMaxDepth() const { return m_maxDepth; }
    size_t getMaxElementsPerNode() const { return m_maxElementsPerNode; }

private:
    /**
     * @brief K-d Tree node structure
     */
    struct KdNode {
        BoundingBox bounds;
        std::vector<ElementId> elements;  // Only for leaf nodes
        std::unique_ptr<KdNode> left;     // Only for internal nodes
        std::unique_ptr<KdNode> right;    // Only for internal nodes
        int splitAxis;  // 0=x, 1=y, 2=z
        double splitValue;
        bool isLeaf;

        KdNode() : splitAxis(0), splitValue(0.0), isLeaf(true) {}
    };

    /**
     * @brief Element entry for building
     */
    struct ElementEntry {
        ElementId id;
        BoundingBox bounds;
        Eigen::Vector3d center;
    };

    /**
     * @brief Build K-d Tree recursively
     */
    std::unique_ptr<KdNode> buildRecursive(
        std::vector<ElementEntry>& entries,
        size_t start,
        size_t end,
        size_t depth);

    /**
     * @brief Find split axis and value using median
     */
    void findSplit(
        std::vector<ElementEntry>& entries,
        size_t start,
        size_t end,
        int& axis,
        double& value);

    /**
     * @brief Partition elements by split
     */
    size_t partition(
        std::vector<ElementEntry>& entries,
        size_t start,
        size_t end,
        int axis,
        double value);

    /**
     * @brief Query recursively
     */
    void queryRecursive(
        const KdNode* node,
        const BoundingBox& queryBox,
        std::vector<ElementId>& results) const;

    /**
     * @brief Query point recursively
     */
    void queryPointRecursive(
        const KdNode* node,
        const Eigen::Vector3d& point,
        std::vector<ElementId>& results) const;

    /**
     * @brief Find nearest element recursively
     */
    void findNearestRecursive(
        const KdNode* node,
        const Eigen::Vector3d& point,
        ElementId& nearestId,
        double& nearestDistSq) const;

    /**
     * @brief Find k-nearest elements recursively
     */
    void findKNearestRecursive(
        const KdNode* node,
        const Eigen::Vector3d& point,
        std::vector<SpatialQueryResult>& results,
        size_t k) const;

    /**
     * @brief Distance from point to split plane
     */
    double distanceToSplitPlane(
        const Eigen::Vector3d& point,
        int axis,
        double value) const;

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
        const KdNode* node,
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction,
        std::vector<ElementId>& results) const;

    /**
     * @brief Calculate memory usage recursively
     */
    size_t memoryUsageRecursive(const KdNode* node) const;

    /**
     * @brief Collect tree statistics recursively
     */
    void collectStatistics(
        const KdNode* node,
        size_t depth,
        size_t& nodeCount,
        size_t& leafCount,
        size_t& maxDepthFound,
        size_t& totalElements) const;

    // Tree structure
    std::unique_ptr<KdNode> m_root;
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
