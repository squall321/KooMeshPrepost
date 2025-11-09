#include "core/KdTree.h"
#include "core/Mesh.h"
#include "core/Element.h"
#include "core/Node.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace koomesh {
namespace core {

KdTree::KdTree()
    : m_maxDepth(20)
    , m_maxElementsPerNode(5)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

KdTree::KdTree(size_t maxDepth, size_t maxElementsPerNode)
    : m_maxDepth(maxDepth)
    , m_maxElementsPerNode(maxElementsPerNode)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

void KdTree::build(const Mesh& mesh) {
    clear();

    if (mesh.elementCount() == 0) {
        m_isBuilt = true;
        return;
    }

    // Collect element bounds and centers
    std::vector<ElementEntry> entries;
    entries.reserve(mesh.elementCount());

    bool first = true;
    Eigen::Vector3d minPt, maxPt;

    for (const auto& [elemId, elemPtr] : mesh.elements()) {
        BoundingBox elemBox = elemPtr->computeBoundingBox(mesh);
        m_elementBounds[elemId] = elemBox;

        ElementEntry entry;
        entry.id = elemId;
        entry.bounds = elemBox;
        entry.center = elemBox.center();
        entries.push_back(entry);

        if (first) {
            minPt = elemBox.min();
            maxPt = elemBox.max();
            first = false;
        } else {
            minPt = minPt.cwiseMin(elemBox.min());
            maxPt = maxPt.cwiseMax(elemBox.max());
        }
    }

    // Expand bounds slightly
    Eigen::Vector3d epsilon = (maxPt - minPt) * 0.001;
    minPt -= epsilon;
    maxPt += epsilon;
    m_bounds = BoundingBox(minPt, maxPt);

    m_elementCount = entries.size();

    // Build tree
    m_root = buildRecursive(entries, 0, entries.size(), 0);

    m_isBuilt = true;
}

std::unique_ptr<KdTree::KdNode> KdTree::buildRecursive(
    std::vector<ElementEntry>& entries,
    size_t start,
    size_t end,
    size_t depth) {

    if (start >= end) {
        return nullptr;
    }

    auto node = std::make_unique<KdNode>();

    // Calculate node bounds
    bool first = true;
    for (size_t i = start; i < end; ++i) {
        if (first) {
            node->bounds = entries[i].bounds;
            first = false;
        } else {
            node->bounds.expand(entries[i].bounds);
        }
    }

    // Stop conditions
    size_t count = end - start;
    if (depth >= m_maxDepth || count <= m_maxElementsPerNode) {
        // Create leaf node
        node->isLeaf = true;
        node->elements.reserve(count);
        for (size_t i = start; i < end; ++i) {
            node->elements.push_back(entries[i].id);
        }
        return node;
    }

    // Find split axis and value
    int axis;
    double splitValue;
    findSplit(entries, start, end, axis, splitValue);

    node->splitAxis = axis;
    node->splitValue = splitValue;
    node->isLeaf = false;

    // Partition elements
    size_t mid = partition(entries, start, end, axis, splitValue);

    // Handle edge case where all elements are on one side
    if (mid == start || mid == end) {
        // Create leaf instead
        node->isLeaf = true;
        node->elements.reserve(count);
        for (size_t i = start; i < end; ++i) {
            node->elements.push_back(entries[i].id);
        }
        return node;
    }

    // Recursively build children
    node->left = buildRecursive(entries, start, mid, depth + 1);
    node->right = buildRecursive(entries, mid, end, depth + 1);

    return node;
}

void KdTree::findSplit(
    std::vector<ElementEntry>& entries,
    size_t start,
    size_t end,
    int& axis,
    double& value) {

    // Choose axis with largest extent
    Eigen::Vector3d minPt = entries[start].center;
    Eigen::Vector3d maxPt = entries[start].center;

    for (size_t i = start + 1; i < end; ++i) {
        minPt = minPt.cwiseMin(entries[i].center);
        maxPt = maxPt.cwiseMax(entries[i].center);
    }

    Eigen::Vector3d extent = maxPt - minPt;
    if (extent.x() >= extent.y() && extent.x() >= extent.z()) {
        axis = 0;
    } else if (extent.y() >= extent.z()) {
        axis = 1;
    } else {
        axis = 2;
    }

    // Use median as split value
    size_t mid = (start + end) / 2;
    std::nth_element(
        entries.begin() + start,
        entries.begin() + mid,
        entries.begin() + end,
        [axis](const ElementEntry& a, const ElementEntry& b) {
            return a.center[axis] < b.center[axis];
        });

    value = entries[mid].center[axis];
}

size_t KdTree::partition(
    std::vector<ElementEntry>& entries,
    size_t start,
    size_t end,
    int axis,
    double value) {

    size_t i = start;
    size_t j = end;

    while (i < j) {
        while (i < j && entries[i].center[axis] <= value) {
            ++i;
        }
        while (i < j && entries[j - 1].center[axis] > value) {
            --j;
        }
        if (i < j) {
            std::swap(entries[i], entries[j - 1]);
        }
    }

    return i;
}

std::vector<ElementId> KdTree::query(const BoundingBox& box) const {
    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    queryRecursive(m_root.get(), box, results);
    return results;
}

void KdTree::queryRecursive(
    const KdNode* node,
    const BoundingBox& queryBox,
    std::vector<ElementId>& results) const {

    if (!node || !node->bounds.intersects(queryBox)) {
        return;
    }

    if (node->isLeaf) {
        // Add all elements in this leaf
        for (ElementId elemId : node->elements) {
            const BoundingBox& elemBox = m_elementBounds.at(elemId);
            if (queryBox.intersects(elemBox)) {
                results.push_back(elemId);
            }
        }
    } else {
        // Recurse to children
        if (node->left) {
            queryRecursive(node->left.get(), queryBox, results);
        }
        if (node->right) {
            queryRecursive(node->right.get(), queryBox, results);
        }
    }
}

std::vector<ElementId> KdTree::queryPoint(const Eigen::Vector3d& point) const {
    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    queryPointRecursive(m_root.get(), point, results);
    return results;
}

void KdTree::queryPointRecursive(
    const KdNode* node,
    const Eigen::Vector3d& point,
    std::vector<ElementId>& results) const {

    if (!node || !node->bounds.contains(point)) {
        return;
    }

    if (node->isLeaf) {
        // Add all elements in this leaf
        results.insert(results.end(), node->elements.begin(), node->elements.end());
    } else {
        // Decide which child to visit
        if (point[node->splitAxis] <= node->splitValue) {
            if (node->left) {
                queryPointRecursive(node->left.get(), point, results);
            }
        } else {
            if (node->right) {
                queryPointRecursive(node->right.get(), point, results);
            }
        }
    }
}

ElementId KdTree::findNearest(const Eigen::Vector3d& point) const {
    if (!m_isBuilt || !m_root || m_elementCount == 0) {
        return 0;
    }

    ElementId nearestId = 0;
    double nearestDistSq = std::numeric_limits<double>::max();

    findNearestRecursive(m_root.get(), point, nearestId, nearestDistSq);

    return nearestId;
}

void KdTree::findNearestRecursive(
    const KdNode* node,
    const Eigen::Vector3d& point,
    ElementId& nearestId,
    double& nearestDistSq) const {

    if (!node) {
        return;
    }

    // Prune if node is too far away
    double nodeDistSq = node->bounds.distanceSquared(point);
    if (nodeDistSq >= nearestDistSq) {
        return;
    }

    if (node->isLeaf) {
        // Check all elements in this leaf
        for (ElementId elemId : node->elements) {
            const BoundingBox& elemBox = m_elementBounds.at(elemId);
            double distSq = elemBox.distanceSquared(point);

            if (distSq < nearestDistSq) {
                nearestDistSq = distSq;
                nearestId = elemId;
            }
        }
    } else {
        // Determine which child to search first
        double distToSplit = point[node->splitAxis] - node->splitValue;
        KdNode* nearChild = (distToSplit <= 0) ? node->left.get() : node->right.get();
        KdNode* farChild = (distToSplit <= 0) ? node->right.get() : node->left.get();

        // Search near child first
        findNearestRecursive(nearChild, point, nearestId, nearestDistSq);

        // Check if we need to search far child
        if (distToSplit * distToSplit < nearestDistSq) {
            findNearestRecursive(farChild, point, nearestId, nearestDistSq);
        }
    }
}

std::vector<ElementId> KdTree::findKNearest(
    const Eigen::Vector3d& point,
    size_t k) const {

    if (!m_isBuilt || !m_root || k == 0) {
        return {};
    }

    std::vector<SpatialQueryResult> results;
    findKNearestRecursive(m_root.get(), point, results, k);

    // Extract element IDs
    std::vector<ElementId> elementIds;
    elementIds.reserve(results.size());
    for (const auto& result : results) {
        elementIds.push_back(result.elementId);
    }

    return elementIds;
}

void KdTree::findKNearestRecursive(
    const KdNode* node,
    const Eigen::Vector3d& point,
    std::vector<SpatialQueryResult>& results,
    size_t k) const {

    if (!node) {
        return;
    }

    // Prune if we have k results and node is too far
    if (results.size() >= k) {
        double maxDist = results.back().distance;
        double nodeDistSq = node->bounds.distanceSquared(point);
        if (nodeDistSq >= maxDist * maxDist) {
            return;
        }
    }

    if (node->isLeaf) {
        // Add all elements from this leaf
        for (ElementId elemId : node->elements) {
            const BoundingBox& elemBox = m_elementBounds.at(elemId);
            double dist = std::sqrt(elemBox.distanceSquared(point));

            results.push_back({elemId, dist});
        }

        // Keep only k nearest
        std::sort(results.begin(), results.end());
        if (results.size() > k) {
            results.resize(k);
        }
    } else {
        // Determine which child to search first
        double distToSplit = point[node->splitAxis] - node->splitValue;
        KdNode* nearChild = (distToSplit <= 0) ? node->left.get() : node->right.get();
        KdNode* farChild = (distToSplit <= 0) ? node->right.get() : node->left.get();

        // Search near child first
        findKNearestRecursive(nearChild, point, results, k);

        // Check if we need to search far child
        if (results.size() < k || distToSplit * distToSplit < results.back().distance * results.back().distance) {
            findKNearestRecursive(farChild, point, results, k);
        }
    }
}

std::vector<ElementId> KdTree::findWithinRadius(
    const Eigen::Vector3d& point,
    double radius) const {

    if (!m_isBuilt || !m_root) {
        return {};
    }

    // Query with sphere bounding box
    Eigen::Vector3d radiusVec(radius, radius, radius);
    BoundingBox queryBox(point - radiusVec, point + radiusVec);

    std::vector<ElementId> candidates = query(queryBox);

    // Filter by actual distance
    std::vector<ElementId> results;
    double radiusSq = radius * radius;

    for (ElementId elemId : candidates) {
        const BoundingBox& elemBox = m_elementBounds.at(elemId);
        if (elemBox.distanceSquared(point) <= radiusSq) {
            results.push_back(elemId);
        }
    }

    return results;
}

std::vector<ElementId> KdTree::rayIntersect(
    const Eigen::Vector3d& origin,
    const Eigen::Vector3d& direction) const {

    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    rayIntersectRecursive(m_root.get(), origin, direction, results);
    return results;
}

void KdTree::rayIntersectRecursive(
    const KdNode* node,
    const Eigen::Vector3d& origin,
    const Eigen::Vector3d& direction,
    std::vector<ElementId>& results) const {

    if (!node || !rayBoxIntersect(origin, direction, node->bounds)) {
        return;
    }

    if (node->isLeaf) {
        // Add all elements in this leaf
        for (ElementId elemId : node->elements) {
            const BoundingBox& elemBox = m_elementBounds.at(elemId);
            if (rayBoxIntersect(origin, direction, elemBox)) {
                results.push_back(elemId);
            }
        }
    } else {
        // Recurse to children
        if (node->left) {
            rayIntersectRecursive(node->left.get(), origin, direction, results);
        }
        if (node->right) {
            rayIntersectRecursive(node->right.get(), origin, direction, results);
        }
    }
}

bool KdTree::rayBoxIntersect(
    const Eigen::Vector3d& origin,
    const Eigen::Vector3d& direction,
    const BoundingBox& box) const {

    // Slab method
    const Eigen::Vector3d& bmin = box.min();
    const Eigen::Vector3d& bmax = box.max();

    double tmin = 0.0;
    double tmax = std::numeric_limits<double>::max();

    for (int i = 0; i < 3; ++i) {
        if (std::abs(direction[i]) < 1e-10) {
            // Ray parallel to slab
            if (origin[i] < bmin[i] || origin[i] > bmax[i]) {
                return false;
            }
        } else {
            double t1 = (bmin[i] - origin[i]) / direction[i];
            double t2 = (bmax[i] - origin[i]) / direction[i];

            if (t1 > t2) std::swap(t1, t2);

            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);

            if (tmin > tmax) {
                return false;
            }
        }
    }

    return true;
}

void KdTree::clear() {
    m_root.reset();
    m_elementBounds.clear();
    m_elementCount = 0;
    m_isBuilt = false;
}

bool KdTree::isBuilt() const {
    return m_isBuilt;
}

size_t KdTree::elementCount() const {
    return m_elementCount;
}

size_t KdTree::memoryUsage() const {
    if (!m_root) {
        return 0;
    }

    size_t total = 0;

    // Element bounds map
    total += m_elementBounds.size() * (sizeof(ElementId) + sizeof(BoundingBox));

    // Tree structure
    total += memoryUsageRecursive(m_root.get());

    return total;
}

size_t KdTree::memoryUsageRecursive(const KdNode* node) const {
    if (!node) {
        return 0;
    }

    size_t total = sizeof(KdNode);

    if (node->isLeaf) {
        total += node->elements.capacity() * sizeof(ElementId);
    } else {
        if (node->left) {
            total += memoryUsageRecursive(node->left.get());
        }
        if (node->right) {
            total += memoryUsageRecursive(node->right.get());
        }
    }

    return total;
}

BoundingBox KdTree::boundingBox() const {
    return m_bounds;
}

std::string KdTree::getStatistics() const {
    if (!m_root) {
        return "KdTree: empty";
    }

    size_t nodeCount = 0;
    size_t leafCount = 0;
    size_t maxDepthFound = 0;
    size_t totalElements = 0;

    collectStatistics(m_root.get(), 0, nodeCount, leafCount, maxDepthFound, totalElements);

    std::ostringstream oss;
    oss << "KdTree Statistics:\n"
        << "  Elements: " << m_elementCount << "\n"
        << "  Nodes: " << nodeCount << "\n"
        << "  Leaf nodes: " << leafCount << "\n"
        << "  Max depth: " << maxDepthFound << " (limit: " << m_maxDepth << ")\n"
        << "  Avg elements per leaf: " << (leafCount > 0 ? totalElements / leafCount : 0) << "\n"
        << "  Memory usage: " << (memoryUsage() / 1024) << " KB\n"
        << "  Bounds: " << m_bounds.min().transpose() << " to " << m_bounds.max().transpose();

    return oss.str();
}

void KdTree::collectStatistics(
    const KdNode* node,
    size_t depth,
    size_t& nodeCount,
    size_t& leafCount,
    size_t& maxDepthFound,
    size_t& totalElements) const {

    if (!node) {
        return;
    }

    ++nodeCount;
    maxDepthFound = std::max(maxDepthFound, depth);

    if (node->isLeaf) {
        ++leafCount;
        totalElements += node->elements.size();
    } else {
        if (node->left) {
            collectStatistics(node->left.get(), depth + 1, nodeCount, leafCount, maxDepthFound, totalElements);
        }
        if (node->right) {
            collectStatistics(node->right.get(), depth + 1, nodeCount, leafCount, maxDepthFound, totalElements);
        }
    }
}

} // namespace core
} // namespace koomesh
