#include "core/RTree.h"
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

RTree::RTree()
    : m_maxChildren(16)
    , m_minChildren(8)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

RTree::RTree(size_t maxChildren)
    : m_maxChildren(maxChildren)
    , m_minChildren(maxChildren / 2)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

void RTree::setMaxChildren(size_t maxChildren) {
    m_maxChildren = maxChildren;
    m_minChildren = maxChildren / 2;
}

void RTree::build(const Mesh& mesh) {
    clear();

    if (mesh.elementCount() == 0) {
        m_isBuilt = true;
        return;
    }

    // Collect element bounds
    std::vector<Entry> entries;
    entries.reserve(mesh.elementCount());

    bool first = true;
    Eigen::Vector3d minPt, maxPt;

    for (const auto& [elemId, elemPtr] : mesh.elements()) {
        BoundingBox elemBox = elemPtr->computeBoundingBox(mesh);
        m_elementBounds[elemId] = elemBox;

        entries.push_back({elemId, elemBox});

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

    // Build using bulk loading
    bulkLoad(entries);

    m_isBuilt = true;
}

void RTree::bulkLoad(std::vector<Entry>& entries) {
    if (entries.empty()) {
        return;
    }

    // Use STR (Sort-Tile-Recursive) algorithm
    // 1. Sort by X coordinate
    std::sort(entries.begin(), entries.end(),
        [](const Entry& a, const Entry& b) {
            return a.bounds.center().x() < b.bounds.center().x();
        });

    m_root = buildLevel(entries, 0, entries.size(), true);
}

std::unique_ptr<RTree::RTreeNode> RTree::buildLevel(
    std::vector<Entry>& entries,
    size_t start,
    size_t end,
    bool createLeaf) {

    size_t count = end - start;
    if (count == 0) {
        return nullptr;
    }

    auto node = std::make_unique<RTreeNode>();

    if (createLeaf) {
        // Create leaf node
        node->isLeaf = true;

        for (size_t i = start; i < end; ++i) {
            node->elements.push_back(entries[i].elementId);

            if (i == start) {
                node->bounds = entries[i].bounds;
            } else {
                node->bounds.expand(entries[i].bounds);
            }
        }
    } else {
        // Create internal node
        node->isLeaf = false;

        // Calculate number of slices
        size_t sliceSize = m_maxChildren;
        size_t numSlices = (count + sliceSize - 1) / sliceSize;

        // Sort by Y coordinate for vertical slices
        std::sort(entries.begin() + start, entries.begin() + end,
            [](const Entry& a, const Entry& b) {
                return a.bounds.center().y() < b.bounds.center().y();
            });

        // Create child nodes
        for (size_t i = 0; i < numSlices; ++i) {
            size_t sliceStart = start + i * sliceSize;
            size_t sliceEnd = std::min(sliceStart + sliceSize, end);

            // Determine if children should be leaves
            bool childrenAreLeaves = (sliceEnd - sliceStart <= m_maxChildren);

            auto child = buildLevel(entries, sliceStart, sliceEnd, childrenAreLeaves);
            if (child) {
                if (node->children.empty()) {
                    node->bounds = child->bounds;
                } else {
                    node->bounds.expand(child->bounds);
                }
                node->children.push_back(std::move(child));
            }
        }
    }

    return node;
}

std::vector<ElementId> RTree::query(const BoundingBox& box) const {
    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    queryRecursive(m_root.get(), box, results);
    return results;
}

void RTree::queryRecursive(
    const RTreeNode* node,
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
        for (const auto& child : node->children) {
            queryRecursive(child.get(), queryBox, results);
        }
    }
}

std::vector<ElementId> RTree::queryPoint(const Eigen::Vector3d& point) const {
    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    queryPointRecursive(m_root.get(), point, results);
    return results;
}

void RTree::queryPointRecursive(
    const RTreeNode* node,
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
        for (const auto& child : node->children) {
            queryPointRecursive(child.get(), point, results);
        }
    }
}

ElementId RTree::findNearest(const Eigen::Vector3d& point) const {
    if (!m_isBuilt || !m_root || m_elementCount == 0) {
        return 0;
    }

    ElementId nearestId = 0;
    double nearestDistSq = std::numeric_limits<double>::max();

    findNearestRecursive(m_root.get(), point, nearestId, nearestDistSq);

    return nearestId;
}

void RTree::findNearestRecursive(
    const RTreeNode* node,
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
        std::vector<std::pair<double, const RTreeNode*>> childDists;
        for (const auto& child : node->children) {
            double dist = child->bounds.distanceSquared(point);
            childDists.push_back({dist, child.get()});
        }

        std::sort(childDists.begin(), childDists.end());

        for (const auto& [dist, childNode] : childDists) {
            findNearestRecursive(childNode, point, nearestId, nearestDistSq);
        }
    }
}

std::vector<ElementId> RTree::findKNearest(
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

void RTree::findKNearestRecursive(
    const RTreeNode* node,
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
        std::vector<std::pair<double, const RTreeNode*>> childDists;
        for (const auto& child : node->children) {
            double dist = std::sqrt(child->bounds.distanceSquared(point));
            childDists.push_back({dist, child.get()});
        }

        std::sort(childDists.begin(), childDists.end());

        for (const auto& [dist, childNode] : childDists) {
            findKNearestRecursive(childNode, point, results, k);
        }
    }
}

std::vector<ElementId> RTree::findWithinRadius(
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

std::vector<ElementId> RTree::rayIntersect(
    const Eigen::Vector3d& origin,
    const Eigen::Vector3d& direction) const {

    std::vector<ElementId> results;
    if (!m_isBuilt || !m_root) {
        return results;
    }

    rayIntersectRecursive(m_root.get(), origin, direction, results);
    return results;
}

void RTree::rayIntersectRecursive(
    const RTreeNode* node,
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
        for (const auto& child : node->children) {
            rayIntersectRecursive(child.get(), origin, direction, results);
        }
    }
}

bool RTree::rayBoxIntersect(
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

void RTree::clear() {
    m_root.reset();
    m_elementBounds.clear();
    m_elementCount = 0;
    m_isBuilt = false;
}

bool RTree::isBuilt() const {
    return m_isBuilt;
}

size_t RTree::elementCount() const {
    return m_elementCount;
}

size_t RTree::memoryUsage() const {
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

size_t RTree::memoryUsageRecursive(const RTreeNode* node) const {
    if (!node) {
        return 0;
    }

    size_t total = sizeof(RTreeNode);

    if (node->isLeaf) {
        total += node->elements.capacity() * sizeof(ElementId);
    } else {
        total += node->children.capacity() * sizeof(std::unique_ptr<RTreeNode>);
        for (const auto& child : node->children) {
            total += memoryUsageRecursive(child.get());
        }
    }

    return total;
}

BoundingBox RTree::boundingBox() const {
    return m_bounds;
}

std::string RTree::getStatistics() const {
    if (!m_root) {
        return "RTree: empty";
    }

    size_t nodeCount = 0;
    size_t leafCount = 0;
    size_t maxDepth = 0;
    double totalOverlap = 0.0;

    collectStatistics(m_root.get(), 0, nodeCount, leafCount, maxDepth, totalOverlap);

    std::ostringstream oss;
    oss << "RTree Statistics:\n"
        << "  Elements: " << m_elementCount << "\n"
        << "  Nodes: " << nodeCount << "\n"
        << "  Leaf nodes: " << leafCount << "\n"
        << "  Max depth: " << maxDepth << "\n"
        << "  Max children: " << m_maxChildren << "\n"
        << "  Min children: " << m_minChildren << "\n"
        << "  Avg elements per leaf: " << (leafCount > 0 ? m_elementCount / leafCount : 0) << "\n"
        << "  Avg overlap: " << (nodeCount > 0 ? totalOverlap / nodeCount : 0.0) << "\n"
        << "  Memory usage: " << (memoryUsage() / 1024) << " KB\n"
        << "  Bounds: " << m_bounds.min().transpose() << " to " << m_bounds.max().transpose();

    return oss.str();
}

void RTree::collectStatistics(
    const RTreeNode* node,
    size_t depth,
    size_t& nodeCount,
    size_t& leafCount,
    size_t& maxDepth,
    double& totalOverlap) const {

    if (!node) {
        return;
    }

    ++nodeCount;
    maxDepth = std::max(maxDepth, depth);

    if (node->isLeaf) {
        ++leafCount;
    } else {
        totalOverlap += calculateOverlap(node);

        for (const auto& child : node->children) {
            collectStatistics(child.get(), depth + 1, nodeCount, leafCount, maxDepth, totalOverlap);
        }
    }
}

double RTree::calculateOverlap(const RTreeNode* node) const {
    if (!node || node->children.size() <= 1) {
        return 0.0;
    }

    double totalOverlap = 0.0;

    // Check all pairs of children
    for (size_t i = 0; i < node->children.size(); ++i) {
        for (size_t j = i + 1; j < node->children.size(); ++j) {
            const BoundingBox& box1 = node->children[i]->bounds;
            const BoundingBox& box2 = node->children[j]->bounds;

            if (box1.intersects(box2)) {
                // Calculate intersection volume
                Eigen::Vector3d minIntersect = box1.min().cwiseMax(box2.min());
                Eigen::Vector3d maxIntersect = box1.max().cwiseMin(box2.max());
                Eigen::Vector3d size = maxIntersect - minIntersect;

                if (size.x() > 0 && size.y() > 0 && size.z() > 0) {
                    totalOverlap += size.x() * size.y() * size.z();
                }
            }
        }
    }

    return totalOverlap;
}

} // namespace core
} // namespace koomesh
