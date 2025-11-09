#include "core/UniformGrid.h"
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

UniformGrid::UniformGrid()
    : m_cellSize(1.0)
    , m_cellsX(0)
    , m_cellsY(0)
    , m_cellsZ(0)
    , m_useCellCount(false)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

UniformGrid::UniformGrid(double cellSize)
    : m_cellSize(cellSize)
    , m_cellsX(0)
    , m_cellsY(0)
    , m_cellsZ(0)
    , m_useCellCount(false)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

UniformGrid::UniformGrid(size_t cellsX, size_t cellsY, size_t cellsZ)
    : m_cellSize(1.0)
    , m_cellsX(cellsX)
    , m_cellsY(cellsY)
    , m_cellsZ(cellsZ)
    , m_useCellCount(true)
    , m_elementCount(0)
    , m_isBuilt(false) {
}

void UniformGrid::setCellSize(double size) {
    m_cellSize = size;
    m_useCellCount = false;
}

void UniformGrid::setCellCount(size_t cellsX, size_t cellsY, size_t cellsZ) {
    m_cellsX = cellsX;
    m_cellsY = cellsY;
    m_cellsZ = cellsZ;
    m_useCellCount = true;
}

void UniformGrid::build(const Mesh& mesh) {
    clear();

    if (mesh.elementCount() == 0) {
        m_isBuilt = true;
        return;
    }

    // Compute mesh bounding box and element bounds
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

    // Calculate cell size if using cell count
    if (m_useCellCount) {
        Eigen::Vector3d size = maxPt - minPt;
        double cellSizeX = size.x() / m_cellsX;
        double cellSizeY = size.y() / m_cellsY;
        double cellSizeZ = size.z() / m_cellsZ;
        m_cellSize = std::max({cellSizeX, cellSizeY, cellSizeZ});
    }

    m_elementCount = mesh.elementCount();

    // Insert elements into grid cells
    for (const auto& [elemId, elemPtr] : mesh.elements()) {
        const BoundingBox& elemBox = m_elementBounds[elemId];

        // Get all cells that this element intersects
        auto cells = getCellsInBox(elemBox);

        for (const auto& cellIdx : cells) {
            addElementToCell(cellIdx, elemId);
        }
    }

    m_isBuilt = true;
}

UniformGrid::CellIndex UniformGrid::getCellIndex(const Eigen::Vector3d& point) const {
    CellIndex idx;

    Eigen::Vector3d relPos = point - m_bounds.min();

    idx.x = static_cast<int>(std::floor(relPos.x() / m_cellSize));
    idx.y = static_cast<int>(std::floor(relPos.y() / m_cellSize));
    idx.z = static_cast<int>(std::floor(relPos.z() / m_cellSize));

    return idx;
}

std::vector<UniformGrid::CellIndex> UniformGrid::getCellsInBox(const BoundingBox& box) const {
    std::vector<CellIndex> cells;

    CellIndex minIdx = getCellIndex(box.min());
    CellIndex maxIdx = getCellIndex(box.max());

    for (int x = minIdx.x; x <= maxIdx.x; ++x) {
        for (int y = minIdx.y; y <= maxIdx.y; ++y) {
            for (int z = minIdx.z; z <= maxIdx.z; ++z) {
                cells.push_back({x, y, z});
            }
        }
    }

    return cells;
}

bool UniformGrid::isValidCellIndex(const CellIndex& idx) const {
    if (m_useCellCount) {
        return idx.x >= 0 && idx.x < static_cast<int>(m_cellsX) &&
               idx.y >= 0 && idx.y < static_cast<int>(m_cellsY) &&
               idx.z >= 0 && idx.z < static_cast<int>(m_cellsZ);
    }
    return true;  // No bounds checking for dynamic cell size
}

const UniformGrid::GridCell* UniformGrid::getCell(const CellIndex& idx) const {
    auto it = m_cells.find(idx);
    if (it != m_cells.end()) {
        return &it->second;
    }
    return nullptr;
}

void UniformGrid::addElementToCell(const CellIndex& idx, ElementId elemId) {
    m_cells[idx].elements.push_back(elemId);
}

std::vector<ElementId> UniformGrid::query(const BoundingBox& box) const {
    std::vector<ElementId> results;
    if (!m_isBuilt) {
        return results;
    }

    auto cells = getCellsInBox(box);

    // Use set to avoid duplicates
    std::unordered_set<ElementId> uniqueResults;

    for (const auto& cellIdx : cells) {
        const GridCell* cell = getCell(cellIdx);
        if (cell) {
            for (ElementId elemId : cell->elements) {
                const BoundingBox& elemBox = m_elementBounds.at(elemId);
                if (box.intersects(elemBox)) {
                    uniqueResults.insert(elemId);
                }
            }
        }
    }

    results.assign(uniqueResults.begin(), uniqueResults.end());
    return results;
}

std::vector<ElementId> UniformGrid::queryPoint(const Eigen::Vector3d& point) const {
    std::vector<ElementId> results;
    if (!m_isBuilt) {
        return results;
    }

    CellIndex idx = getCellIndex(point);
    const GridCell* cell = getCell(idx);

    if (cell) {
        results = cell->elements;
    }

    return results;
}

ElementId UniformGrid::findNearest(const Eigen::Vector3d& point) const {
    if (!m_isBuilt || m_elementCount == 0) {
        return 0;
    }

    ElementId nearestId = 0;
    double nearestDistSq = std::numeric_limits<double>::max();

    // Start with spiral search
    // Maximum radius in cells to search
    int maxRadius = std::max({
        static_cast<int>(std::ceil((m_bounds.max().x() - m_bounds.min().x()) / m_cellSize)),
        static_cast<int>(std::ceil((m_bounds.max().y() - m_bounds.min().y()) / m_cellSize)),
        static_cast<int>(std::ceil((m_bounds.max().z() - m_bounds.min().z()) / m_cellSize))
    });

    spiralSearch(point, nearestId, nearestDistSq, maxRadius);

    return nearestId;
}

void UniformGrid::spiralSearch(
    const Eigen::Vector3d& point,
    ElementId& nearestId,
    double& nearestDistSq,
    int maxRadius) const {

    CellIndex centerIdx = getCellIndex(point);

    // Search in expanding radius
    for (int radius = 0; radius <= maxRadius; ++radius) {
        auto neighbors = getNeighborCells(centerIdx, radius);

        bool foundInThisRadius = false;

        for (const auto& cellIdx : neighbors) {
            const GridCell* cell = getCell(cellIdx);
            if (!cell) continue;

            for (ElementId elemId : cell->elements) {
                const BoundingBox& elemBox = m_elementBounds.at(elemId);
                double distSq = elemBox.distanceSquared(point);

                if (distSq < nearestDistSq) {
                    nearestDistSq = distSq;
                    nearestId = elemId;
                    foundInThisRadius = true;
                }
            }
        }

        // Early termination: if we found something in this radius,
        // and the next radius is farther than current best, stop
        if (foundInThisRadius && radius > 0) {
            double nextRadiusDistSq = (radius + 1) * (radius + 1) * m_cellSize * m_cellSize;
            if (nextRadiusDistSq > nearestDistSq) {
                break;
            }
        }
    }
}

std::vector<UniformGrid::CellIndex> UniformGrid::getNeighborCells(
    const CellIndex& center,
    int radius) const {

    std::vector<CellIndex> neighbors;

    if (radius == 0) {
        neighbors.push_back(center);
        return neighbors;
    }

    // Get all cells at exactly distance 'radius' from center
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dz = -radius; dz <= radius; ++dz) {
                // Check if at boundary of radius
                if (std::abs(dx) == radius || std::abs(dy) == radius || std::abs(dz) == radius) {
                    CellIndex idx = {center.x + dx, center.y + dy, center.z + dz};
                    neighbors.push_back(idx);
                }
            }
        }
    }

    return neighbors;
}

std::vector<ElementId> UniformGrid::findKNearest(
    const Eigen::Vector3d& point,
    size_t k) const {

    if (!m_isBuilt || k == 0) {
        return {};
    }

    std::vector<SpatialQueryResult> results;
    CellIndex centerIdx = getCellIndex(point);

    int maxRadius = std::max({
        static_cast<int>(std::ceil((m_bounds.max().x() - m_bounds.min().x()) / m_cellSize)),
        static_cast<int>(std::ceil((m_bounds.max().y() - m_bounds.min().y()) / m_cellSize)),
        static_cast<int>(std::ceil((m_bounds.max().z() - m_bounds.min().z()) / m_cellSize))
    });

    // Search in expanding radius
    for (int radius = 0; radius <= maxRadius; ++radius) {
        auto neighbors = getNeighborCells(centerIdx, radius);

        for (const auto& cellIdx : neighbors) {
            const GridCell* cell = getCell(cellIdx);
            if (!cell) continue;

            for (ElementId elemId : cell->elements) {
                const BoundingBox& elemBox = m_elementBounds.at(elemId);
                double dist = std::sqrt(elemBox.distanceSquared(point));

                results.push_back({elemId, dist});
            }
        }

        // Sort and keep only k nearest
        std::sort(results.begin(), results.end());
        if (results.size() > k) {
            results.resize(k);
        }

        // Early termination
        if (results.size() >= k) {
            double maxDist = results.back().distance;
            double nextRadiusDist = (radius + 1) * m_cellSize;
            if (nextRadiusDist > maxDist) {
                break;
            }
        }
    }

    // Extract element IDs
    std::vector<ElementId> elementIds;
    elementIds.reserve(results.size());
    for (const auto& result : results) {
        elementIds.push_back(result.elementId);
    }

    return elementIds;
}

std::vector<ElementId> UniformGrid::findWithinRadius(
    const Eigen::Vector3d& point,
    double radius) const {

    if (!m_isBuilt) {
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

std::vector<ElementId> UniformGrid::rayIntersect(
    const Eigen::Vector3d& origin,
    const Eigen::Vector3d& direction) const {

    std::vector<ElementId> results;
    if (!m_isBuilt) {
        return results;
    }

    traverseRay(origin, direction, results);
    return results;
}

void UniformGrid::traverseRay(
    const Eigen::Vector3d& origin,
    const Eigen::Vector3d& direction,
    std::vector<ElementId>& results) const {

    // DDA-based grid traversal
    if (!m_bounds.contains(origin) && !rayBoxIntersect(origin, direction, m_bounds)) {
        return;
    }

    // Find entry point
    Eigen::Vector3d rayStart = origin;
    if (!m_bounds.contains(origin)) {
        // Calculate intersection with bounding box
        // Simplified: just use origin for now
        rayStart = origin;
    }

    CellIndex currentCell = getCellIndex(rayStart);

    // Step direction
    int stepX = direction.x() > 0 ? 1 : (direction.x() < 0 ? -1 : 0);
    int stepY = direction.y() > 0 ? 1 : (direction.y() < 0 ? -1 : 0);
    int stepZ = direction.z() > 0 ? 1 : (direction.z() < 0 ? -1 : 0);

    // Calculate tMax and tDelta
    Eigen::Vector3d cellMin(
        m_bounds.min().x() + currentCell.x * m_cellSize,
        m_bounds.min().y() + currentCell.y * m_cellSize,
        m_bounds.min().z() + currentCell.z * m_cellSize
    );

    Eigen::Vector3d cellMax = cellMin + Eigen::Vector3d(m_cellSize, m_cellSize, m_cellSize);

    // Use set to avoid duplicates
    std::unordered_set<ElementId> uniqueResults;

    // Limit traversal steps
    int maxSteps = 1000;
    int steps = 0;

    while (steps++ < maxSteps) {
        // Check current cell
        const GridCell* cell = getCell(currentCell);
        if (cell) {
            for (ElementId elemId : cell->elements) {
                const BoundingBox& elemBox = m_elementBounds.at(elemId);
                if (rayBoxIntersect(origin, direction, elemBox)) {
                    uniqueResults.insert(elemId);
                }
            }
        }

        // Find next cell
        double tMaxX = std::numeric_limits<double>::max();
        double tMaxY = std::numeric_limits<double>::max();
        double tMaxZ = std::numeric_limits<double>::max();

        if (stepX != 0) {
            double nextX = stepX > 0 ? cellMax.x() : cellMin.x();
            tMaxX = (nextX - origin.x()) / direction.x();
        }
        if (stepY != 0) {
            double nextY = stepY > 0 ? cellMax.y() : cellMin.y();
            tMaxY = (nextY - origin.y()) / direction.y();
        }
        if (stepZ != 0) {
            double nextZ = stepZ > 0 ? cellMax.z() : cellMin.z();
            tMaxZ = (nextZ - origin.z()) / direction.z();
        }

        // Step to next cell
        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            currentCell.x += stepX;
            cellMin.x() += stepX * m_cellSize;
            cellMax.x() += stepX * m_cellSize;
        } else if (tMaxY < tMaxZ) {
            currentCell.y += stepY;
            cellMin.y() += stepY * m_cellSize;
            cellMax.y() += stepY * m_cellSize;
        } else {
            currentCell.z += stepZ;
            cellMin.z() += stepZ * m_cellSize;
            cellMax.z() += stepZ * m_cellSize;
        }

        // Check if still in bounds
        Eigen::Vector3d cellCenter = (cellMin + cellMax) * 0.5;
        if (!m_bounds.contains(cellCenter)) {
            break;
        }
    }

    results.assign(uniqueResults.begin(), uniqueResults.end());
}

bool UniformGrid::rayBoxIntersect(
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

void UniformGrid::clear() {
    m_cells.clear();
    m_elementBounds.clear();
    m_elementCount = 0;
    m_isBuilt = false;
}

bool UniformGrid::isBuilt() const {
    return m_isBuilt;
}

size_t UniformGrid::elementCount() const {
    return m_elementCount;
}

size_t UniformGrid::memoryUsage() const {
    size_t total = 0;

    // Element bounds map
    total += m_elementBounds.size() * (sizeof(ElementId) + sizeof(BoundingBox));

    // Grid cells
    total += m_cells.size() * sizeof(std::pair<CellIndex, GridCell>);

    for (const auto& [idx, cell] : m_cells) {
        total += cell.elements.capacity() * sizeof(ElementId);
    }

    return total;
}

BoundingBox UniformGrid::boundingBox() const {
    return m_bounds;
}

std::string UniformGrid::getStatistics() const {
    if (m_cells.empty()) {
        return "UniformGrid: empty";
    }

    size_t nonEmptyCells = m_cells.size();
    size_t totalElements = 0;
    size_t maxElementsPerCell = 0;
    size_t minElementsPerCell = std::numeric_limits<size_t>::max();

    for (const auto& [idx, cell] : m_cells) {
        size_t count = cell.elements.size();
        totalElements += count;
        maxElementsPerCell = std::max(maxElementsPerCell, count);
        minElementsPerCell = std::min(minElementsPerCell, count);
    }

    double avgElementsPerCell = nonEmptyCells > 0 ? static_cast<double>(totalElements) / nonEmptyCells : 0.0;

    std::ostringstream oss;
    oss << "UniformGrid Statistics:\n"
        << "  Elements: " << m_elementCount << "\n"
        << "  Cell size: " << m_cellSize << "\n"
        << "  Non-empty cells: " << nonEmptyCells << "\n"
        << "  Avg elements per cell: " << avgElementsPerCell << "\n"
        << "  Max elements per cell: " << maxElementsPerCell << "\n"
        << "  Min elements per cell: " << minElementsPerCell << "\n"
        << "  Memory usage: " << (memoryUsage() / 1024) << " KB\n"
        << "  Bounds: " << m_bounds.min().transpose() << " to " << m_bounds.max().transpose();

    return oss.str();
}

} // namespace core
} // namespace koomesh
