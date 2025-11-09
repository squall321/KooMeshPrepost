#include "core/Octree.h"
#include "core/Mesh.h"
#include "core/Element.h"
#include "core/Node.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <queue>

namespace koomesh {
namespace core {

Octree::Octree()
    : m_maxDepth(8)
    , m_maxElementsPerNode(10)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

Octree::Octree(size_t maxDepth, size_t maxElementsPerNode)
    : m_maxDepth(maxDepth)
    , m_maxElementsPerNode(maxElementsPerNode)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

void Octree::build(const Mesh& mesh) {
    clear();

    // Compute mesh bounding box and element bounds
    if (mesh.elementCount() == 0) {
        m_isBuilt = true;
        return;
    }

    // Calculate overall bounds
    bool first = true;
    Eigen::Vector3d minPt, maxPt;

    m_elementBounds.reserve(mesh.elementCount());

    for (const auto& [elemId, elemPtr] : mesh.elements()) {
        BoundingBox elemBox = elemPtr->computeBoundingBox(mesh);
        m_elementBounds[elemId] = elemBox;

        if (first) {
            minPt = elemBox.min();
            maxPt = elemBox.max();
            first = false;
        } else {
            minPt = minPt.cwiseMin(elemBox.min());
            maxPt = maxPt.cwiseMax(elemBox.max());
        }
    }

    // Expand bounds slightly to avoid boundary issues
    Eigen::Vector3d epsilon = (maxPt - minPt) * 0.001;
    minPt -= epsilon;
    maxPt += epsilon;

    m_bounds = BoundingBox(minPt, maxPt);

    // Collect all element IDs
    std::vector<ElementId> allElements;
    allElements.reserve(mesh.elementCount());
    for (const auto& [elemId, _] : mesh.elements()) {
        allElements.push_back(elemId);
    }

    m_elementCount = allElements.size();

    // Build tree
    m_root = std::make_unique<OctreeNode>(m_bounds);
    buildRecursive(m_root.get(), allElements, 0);

    m_isBuilt = true;
}

void Octree::buildRecursive(
    OctreeNode* node,
    const std::vector<ElementId>& elementIds,
    size_t depth) {

    // Stop conditions
    if (elementIds.empty()) {
        return;
    }

    if (depth >= m_maxDepth || elementIds.size() <= m_maxElementsPerNode) {
        // Create leaf node
        node->isLeaf = true;
        node->elements = elementIds;
        return;
    }

    // Subdivide
    subdivide(node);
    node->isLeaf = false;

    // Distribute elements to children
    std::vector<std::vector<ElementId>> childElements(8);

    for (ElementId elemId : elementIds) {
        const BoundingBox& elemBox = m_elementBounds[elemId];

        // Determine which octants this element intersects
        for (int octant = 0; octant < 8; ++octant) {
            if (node->children[octant]->bounds.intersects(elemBox)) {
                childElements[octant].push_back(elemId);
            }
        }
    }

    // Recursively build children
    for (int i = 0; i < 8; ++i) {
        if (!childElements[i].empty()) {
            buildRecursive(node->children[i].get(), childElements[i], depth + 1);
        }
    }
}

void Octree::subdivide(OctreeNode* node) {
    const Eigen::Vector3d& min = node->bounds.min();
    const Eigen::Vector3d& max = node->bounds.max();
    Eigen::Vector3d center = (min + max) * 0.5;

    // Create 8 children
    // Octant ordering: [x][y][z] where 0=min, 1=max
    for (int i = 0; i < 8; ++i) {
        Eigen::Vector3d childMin, childMax;

        childMin.x() = (i & 1) ? center.x() : min.x();
        childMax.x() = (i & 1) ? max.x() : center.x();

        childMin.y() = (i & 2) ? center.y() : min.y();
        childMax.y() = (i & 2) ? max.y() : center.y();

        childMin.z() = (i & 4) ? center.z() : min.z();
        childMax.z() = (i & 4) ? max.z() : center.z();

        node->children[i] = std::make_unique<OctreeNode>(BoundingBox(childMin, childMax));
    }
}

std::vector<ElementId> Octree::query(const BoundingBox& box) const {
    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    queryRecursive(m_root.get(), box, results);
    return results;
}

void Octree::queryRecursive(
    const OctreeNode* node,
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
        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                queryRecursive(node->children[i].get(), queryBox, results);
            }
        }
    }
}

std::vector<ElementId> Octree::queryPoint(const Eigen::Vector3d& point) const {
    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    queryPointRecursive(m_root.get(), point, results);
    return results;
}

void Octree::queryPointRecursive(
    const OctreeNode* node,
    const Eigen::Vector3d& point,
    std::vector<ElementId>& results) const {

    if (!node || !node->bounds.contains(point)) {
        return;
    }

    if (node->isLeaf) {
        // Add all elements in this leaf
        results.insert(results.end(), node->elements.begin(), node->elements.end());
    } else {
        // Recurse to children
        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                queryPointRecursive(node->children[i].get(), point, results);
            }
        }
    }
}

ElementId Octree::findNearest(const Eigen::Vector3d& point) const {
    if (!m_isBuilt || !m_root || m_elementCount == 0) {
        return 0;
    }

    ElementId nearestId = 0;
    double nearestDistSq = std::numeric_limits<double>::max();

    findNearestRecursive(m_root.get(), point, nearestId, nearestDistSq);

    return nearestId;
}

void Octree::findNearestRecursive(
    const OctreeNode* node,
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
        // Sort children by distance and search nearest first
        std::vector<std::pair<double, int>> childDists;
        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                double dist = node->children[i]->bounds.distanceSquared(point);
                childDists.push_back({dist, i});
            }
        }

        std::sort(childDists.begin(), childDists.end());

        for (const auto& [dist, idx] : childDists) {
            findNearestRecursive(node->children[idx].get(), point, nearestId, nearestDistSq);
        }
    }
}

std::vector<ElementId> Octree::findKNearest(
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

void Octree::findKNearestRecursive(
    const OctreeNode* node,
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
        // Sort children by distance
        std::vector<std::pair<double, int>> childDists;
        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                double dist = std::sqrt(node->children[i]->bounds.distanceSquared(point));
                childDists.push_back({dist, i});
            }
        }

        std::sort(childDists.begin(), childDists.end());

        for (const auto& [dist, idx] : childDists) {
            findKNearestRecursive(node->children[idx].get(), point, results, k);
        }
    }
}

std::vector<ElementId> Octree::findWithinRadius(
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

std::vector<ElementId> Octree::rayIntersect(
    const Eigen::Vector3d& origin,
    const Eigen::Vector3d& direction) const {

    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    rayIntersectRecursive(m_root.get(), origin, direction, results);
    return results;
}

void Octree::rayIntersectRecursive(
    const OctreeNode* node,
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
        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                rayIntersectRecursive(node->children[i].get(), origin, direction, results);
            }
        }
    }
}

bool Octree::rayBoxIntersect(
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

void Octree::clear() {
    m_root.reset();
    m_elementBounds.clear();
    m_elementCount = 0;
    m_isBuilt = false;
}

bool Octree::isBuilt() const {
    return m_isBuilt;
}

size_t Octree::elementCount() const {
    return m_elementCount;
}

size_t Octree::memoryUsage() const {
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

size_t Octree::memoryUsageRecursive(const OctreeNode* node) const {
    if (!node) {
        return 0;
    }

    size_t total = sizeof(OctreeNode);

    if (node->isLeaf) {
        total += node->elements.capacity() * sizeof(ElementId);
    } else {
        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                total += memoryUsageRecursive(node->children[i].get());
            }
        }
    }

    return total;
}

BoundingBox Octree::boundingBox() const {
    return m_bounds;
}

std::string Octree::getStatistics() const {
    if (!m_root) {
        return "Octree: empty";
    }

    size_t nodeCount = 0;
    size_t leafCount = 0;
    size_t maxDepthFound = 0;
    size_t totalElements = 0;

    collectStatistics(m_root.get(), 0, nodeCount, leafCount, maxDepthFound, totalElements);

    std::ostringstream oss;
    oss << "Octree Statistics:\n"
        << "  Elements: " << m_elementCount << "\n"
        << "  Nodes: " << nodeCount << "\n"
        << "  Leaf nodes: " << leafCount << "\n"
        << "  Max depth: " << maxDepthFound << " (limit: " << m_maxDepth << ")\n"
        << "  Avg elements per leaf: " << (leafCount > 0 ? totalElements / leafCount : 0) << "\n"
        << "  Memory usage: " << (memoryUsage() / 1024) << " KB\n"
        << "  Bounds: " << m_bounds.min().transpose() << " to " << m_bounds.max().transpose();

    return oss.str();
}

void Octree::collectStatistics(
    const OctreeNode* node,
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
        for (int i = 0; i < 8; ++i) {
            if (node->children[i]) {
                collectStatistics(
                    node->children[i].get(),
                    depth + 1,
                    nodeCount,
                    leafCount,
                    maxDepthFound,
                    totalElements);
            }
        }
    }
}

} // namespace core
} // namespace koomesh
