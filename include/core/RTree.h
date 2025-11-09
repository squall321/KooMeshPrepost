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
 * @brief R-Tree spatial index implementation
 *
 * R-Tree is a balanced tree structure that groups nearby objects
 * using their minimum bounding rectangles (MBRs).
 *
 * Properties:
 * - O(log n) query time on average
 * - O(n log n) build time
 * - Good for dynamic data with insertions/deletions
 * - Minimizes overlap between nodes
 *
 * This implementation uses:
 * - Quadratic split algorithm
 * - ChooseLeaf with minimum area enlargement
 * - Bottom-up bulk loading
 *
 * Example:
 * @code
 * RTree rtree;
 * rtree.setMaxChildren(16);
 * rtree.build(mesh);
 *
 * BoundingBox queryBox(min, max);
 * auto elements = rtree.query(queryBox);
 * @endcode
 */
class RTree : public ISpatialIndex {
public:
    /**
     * @brief Constructor with default parameters
     */
    RTree();

    /**
     * @brief Constructor with custom parameters
     *
     * @param maxChildren Maximum children per node (M)
     */
    explicit RTree(size_t maxChildren);

    /**
     * @brief Destructor
     */
    ~RTree() override = default;

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

    // R-Tree specific configuration
    void setMaxChildren(size_t maxChildren);
    size_t getMaxChildren() const { return m_maxChildren; }
    size_t getMinChildren() const { return m_minChildren; }

private:
    /**
     * @brief R-Tree node structure
     */
    struct RTreeNode {
        BoundingBox bounds;
        std::vector<std::unique_ptr<RTreeNode>> children;  // For internal nodes
        std::vector<ElementId> elements;  // For leaf nodes
        bool isLeaf;

        RTreeNode() : isLeaf(true) {}
    };

    /**
     * @brief Entry for bulk loading
     */
    struct Entry {
        ElementId elementId;
        BoundingBox bounds;
    };

    /**
     * @brief Build R-Tree using bulk loading (STR algorithm)
     */
    void bulkLoad(std::vector<Entry>& entries);

    /**
     * @brief Build R-Tree level recursively
     */
    std::unique_ptr<RTreeNode> buildLevel(
        std::vector<Entry>& entries,
        size_t start,
        size_t end,
        bool createLeaf);

    /**
     * @brief Query recursively
     */
    void queryRecursive(
        const RTreeNode* node,
        const BoundingBox& queryBox,
        std::vector<ElementId>& results) const;

    /**
     * @brief Query point recursively
     */
    void queryPointRecursive(
        const RTreeNode* node,
        const Eigen::Vector3d& point,
        std::vector<ElementId>& results) const;

    /**
     * @brief Find nearest element recursively
     */
    void findNearestRecursive(
        const RTreeNode* node,
        const Eigen::Vector3d& point,
        ElementId& nearestId,
        double& nearestDistSq) const;

    /**
     * @brief Find k-nearest elements recursively
     */
    void findKNearestRecursive(
        const RTreeNode* node,
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
        const RTreeNode* node,
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction,
        std::vector<ElementId>& results) const;

    /**
     * @brief Calculate memory usage recursively
     */
    size_t memoryUsageRecursive(const RTreeNode* node) const;

    /**
     * @brief Collect tree statistics recursively
     */
    void collectStatistics(
        const RTreeNode* node,
        size_t depth,
        size_t& nodeCount,
        size_t& leafCount,
        size_t& maxDepth,
        double& totalOverlap) const;

    /**
     * @brief Calculate overlap between node children
     */
    double calculateOverlap(const RTreeNode* node) const;

    // Tree structure
    std::unique_ptr<RTreeNode> m_root;
    BoundingBox m_bounds;

    // Element bounds cache
    std::unordered_map<ElementId, BoundingBox> m_elementBounds;

    // Configuration
    size_t m_maxChildren;
    size_t m_minChildren;

    // Statistics
    size_t m_elementCount;
    bool m_isBuilt;
};

} // namespace core
} // namespace koomesh
