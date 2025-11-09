#include "selection/SelectionManager.h"
#include <sstream>
#include <algorithm>

namespace koomesh {
namespace selection {

SelectionManager::SelectionManager()
    : m_mesh(nullptr)
    , m_useSpatialIndex(false)
    , m_hasFilter(false)
    , m_historyEnabled(false)
    , m_maxHistory(50)
    , m_historyIndex(0)
{
}

void SelectionManager::setMesh(const core::Mesh& mesh) {
    m_mesh = &mesh;
    clearSelection();

    // Rebuild spatial index if enabled
    if (m_useSpatialIndex && m_spatialIndex) {
        m_spatialIndex->build(mesh);
    }
}

void SelectionManager::useSpatialIndex(core::SpatialIndexType type) {
    if (!m_mesh) {
        return;
    }

    m_spatialIndex = core::SpatialIndexFactory::create(type);
    m_spatialIndex->build(*m_mesh);
    m_useSpatialIndex = true;
}

void SelectionManager::disableSpatialIndex() {
    m_useSpatialIndex = false;
    m_spatialIndex.reset();
}

size_t SelectionManager::selectByBox(const Eigen::Vector3d& minPt,
                                     const Eigen::Vector3d& maxPt,
                                     SelectionMode mode) {
    if (!m_mesh) {
        return 0;
    }

    core::BoundingBox box(minPt, maxPt);
    std::set<core::ElementId> newSelection;

    if (m_useSpatialIndex && m_spatialIndex) {
        // Use spatial index
        auto results = m_spatialIndex->query(box);
        newSelection.insert(results.begin(), results.end());
    } else {
        // Linear search
        newSelection = findInBoxLinear(box);
    }

    // Apply filter
    if (m_hasFilter) {
        newSelection = applyFilter(newSelection);
    }

    // Apply selection mode
    applySelectionMode(newSelection, mode);

    return newSelection.size();
}

size_t SelectionManager::selectBySphere(const Eigen::Vector3d& center,
                                        double radius,
                                        SelectionMode mode) {
    if (!m_mesh) {
        return 0;
    }

    std::set<core::ElementId> newSelection;

    if (m_useSpatialIndex && m_spatialIndex) {
        // Use spatial index
        auto results = m_spatialIndex->findWithinRadius(center, radius);
        newSelection.insert(results.begin(), results.end());
    } else {
        // Linear search
        newSelection = findInSphereLinear(center, radius);
    }

    // Apply filter
    if (m_hasFilter) {
        newSelection = applyFilter(newSelection);
    }

    // Apply selection mode
    applySelectionMode(newSelection, mode);

    return newSelection.size();
}

bool SelectionManager::selectAtPoint(const Eigen::Vector3d& point,
                                     SelectionMode mode) {
    if (!m_mesh) {
        return false;
    }

    std::set<core::ElementId> newSelection;

    if (m_useSpatialIndex && m_spatialIndex) {
        // Use spatial index
        auto results = m_spatialIndex->queryPoint(point);
        if (!results.empty()) {
            newSelection.insert(results.begin(), results.end());
        }
    } else {
        // Linear search - check all elements
        for (const auto& pair : m_mesh->elements()) {
            const auto& element = pair.second;
            if (element->containsPoint(point, *m_mesh)) {
                newSelection.insert(pair.first);
                break;  // Usually want first match
            }
        }
    }

    // Apply filter
    if (m_hasFilter) {
        newSelection = applyFilter(newSelection);
    }

    if (newSelection.empty()) {
        return false;
    }

    // Apply selection mode
    applySelectionMode(newSelection, mode);

    return true;
}

size_t SelectionManager::selectNearest(const Eigen::Vector3d& point,
                                       size_t count,
                                       SelectionMode mode) {
    if (!m_mesh) {
        return 0;
    }

    std::set<core::ElementId> newSelection;

    if (m_useSpatialIndex && m_spatialIndex) {
        // Use spatial index
        auto results = m_spatialIndex->findKNearest(point, count);
        newSelection.insert(results.begin(), results.end());
    } else {
        // Linear search - find nearest by distance to centroid
        std::vector<std::pair<double, core::ElementId>> distances;

        for (const auto& pair : m_mesh->elements()) {
            const auto& element = pair.second;
            Eigen::Vector3d centroid = element->computeCenter(*m_mesh);
            double dist = (centroid - point).norm();
            distances.push_back({dist, pair.first});
        }

        // Sort by distance
        std::sort(distances.begin(), distances.end());

        // Take first 'count' elements
        size_t numToSelect = std::min(count, distances.size());
        for (size_t i = 0; i < numToSelect; ++i) {
            newSelection.insert(distances[i].second);
        }
    }

    // Apply filter
    if (m_hasFilter) {
        newSelection = applyFilter(newSelection);
    }

    // Apply selection mode
    applySelectionMode(newSelection, mode);

    return newSelection.size();
}

void SelectionManager::selectAll() {
    if (!m_mesh) {
        return;
    }

    saveToHistory();

    m_selectedElements.clear();
    for (const auto& pair : m_mesh->elements()) {
        if (!m_hasFilter || m_filter(pair.first, *m_mesh)) {
            m_selectedElements.insert(pair.first);
        }
    }
}

void SelectionManager::clearSelection() {
    if (m_selectedElements.empty()) {
        return;
    }

    saveToHistory();
    m_selectedElements.clear();
}

void SelectionManager::invertSelection() {
    if (!m_mesh) {
        return;
    }

    saveToHistory();

    std::set<core::ElementId> newSelection;
    for (const auto& pair : m_mesh->elements()) {
        if (m_selectedElements.find(pair.first) == m_selectedElements.end()) {
            if (!m_hasFilter || m_filter(pair.first, *m_mesh)) {
                newSelection.insert(pair.first);
            }
        }
    }

    m_selectedElements = newSelection;
}

void SelectionManager::selectByIds(const std::vector<core::ElementId>& elementIds,
                                  SelectionMode mode) {
    std::set<core::ElementId> newSelection(elementIds.begin(), elementIds.end());

    // Apply filter
    if (m_hasFilter) {
        newSelection = applyFilter(newSelection);
    }

    // Apply selection mode
    applySelectionMode(newSelection, mode);
}

void SelectionManager::setFilter(SelectionFilter filter) {
    m_filter = filter;
    m_hasFilter = true;
}

void SelectionManager::clearFilter() {
    m_hasFilter = false;
    m_filter = nullptr;
}

bool SelectionManager::isSelected(core::ElementId id) const {
    return m_selectedElements.find(id) != m_selectedElements.end();
}

void SelectionManager::enableHistory(size_t maxHistory) {
    m_historyEnabled = true;
    m_maxHistory = maxHistory;
    m_history.clear();
    m_historyIndex = 0;
}

void SelectionManager::disableHistory() {
    m_historyEnabled = false;
    m_history.clear();
    m_historyIndex = 0;
}

bool SelectionManager::undo() {
    if (!canUndo()) {
        return false;
    }

    m_historyIndex--;
    m_selectedElements = m_history[m_historyIndex];
    return true;
}

bool SelectionManager::redo() {
    if (!canRedo()) {
        return false;
    }

    m_historyIndex++;
    m_selectedElements = m_history[m_historyIndex];
    return true;
}

std::string SelectionManager::getStatistics() const {
    std::ostringstream oss;

    oss << "Selection Statistics:\n";
    oss << "  Selected Elements: " << m_selectedElements.size();
    if (m_mesh) {
        oss << " / " << m_mesh->elementCount()
            << " (" << (100.0 * m_selectedElements.size() / m_mesh->elementCount()) << "%)\n";
    } else {
        oss << "\n";
    }
    oss << "  Spatial Index: " << (m_useSpatialIndex ? "Enabled" : "Disabled") << "\n";
    oss << "  Filter: " << (m_hasFilter ? "Active" : "None") << "\n";
    oss << "  History: " << (m_historyEnabled ? "Enabled" : "Disabled");
    if (m_historyEnabled) {
        oss << " (" << m_historyIndex << "/" << m_history.size() << ")";
    }
    oss << "\n";

    return oss.str();
}

void SelectionManager::applySelectionMode(const std::set<core::ElementId>& newSelection,
                                         SelectionMode mode) {
    saveToHistory();

    switch (mode) {
        case SelectionMode::REPLACE:
            m_selectedElements = newSelection;
            break;

        case SelectionMode::ADD:
            m_selectedElements.insert(newSelection.begin(), newSelection.end());
            break;

        case SelectionMode::SUBTRACT:
            for (auto id : newSelection) {
                m_selectedElements.erase(id);
            }
            break;

        case SelectionMode::INTERSECT: {
            std::set<core::ElementId> intersection;
            std::set_intersection(
                m_selectedElements.begin(), m_selectedElements.end(),
                newSelection.begin(), newSelection.end(),
                std::inserter(intersection, intersection.begin())
            );
            m_selectedElements = intersection;
            break;
        }
    }
}

std::set<core::ElementId> SelectionManager::applyFilter(
    const std::set<core::ElementId>& elements) const {

    if (!m_hasFilter || !m_mesh) {
        return elements;
    }

    std::set<core::ElementId> filtered;
    for (auto id : elements) {
        if (m_filter(id, *m_mesh)) {
            filtered.insert(id);
        }
    }

    return filtered;
}

void SelectionManager::saveToHistory() {
    if (!m_historyEnabled) {
        return;
    }

    // Remove any history after current index (for redo)
    if (m_historyIndex < m_history.size()) {
        m_history.erase(m_history.begin() + m_historyIndex, m_history.end());
    }

    // Add current selection to history
    m_history.push_back(m_selectedElements);
    m_historyIndex = m_history.size();

    // Limit history size
    if (m_history.size() > m_maxHistory) {
        m_history.erase(m_history.begin());
        m_historyIndex = m_history.size();
    }
}

std::set<core::ElementId> SelectionManager::findInBoxLinear(
    const core::BoundingBox& box) const {

    std::set<core::ElementId> result;

    if (!m_mesh) {
        return result;
    }

    Eigen::Vector3d minPt = box.min();
    Eigen::Vector3d maxPt = box.max();

    for (const auto& pair : m_mesh->elements()) {
        const auto& element = pair.second;

        // Check if element's bounding box intersects query box
        core::BoundingBox elemBox = element->computeBoundingBox(*m_mesh);

        if (elemBox.minX <= maxPt.x() && elemBox.maxX >= minPt.x() &&
            elemBox.minY <= maxPt.y() && elemBox.maxY >= minPt.y() &&
            elemBox.minZ <= maxPt.z() && elemBox.maxZ >= minPt.z()) {
            result.insert(pair.first);
        }
    }

    return result;
}

std::set<core::ElementId> SelectionManager::findInSphereLinear(
    const Eigen::Vector3d& center,
    double radius) const {

    std::set<core::ElementId> result;

    if (!m_mesh) {
        return result;
    }

    double radiusSq = radius * radius;

    for (const auto& pair : m_mesh->elements()) {
        const auto& element = pair.second;

        // Check distance from sphere center to element centroid
        Eigen::Vector3d centroid = element->computeCenter(*m_mesh);
        double distSq = (centroid - center).squaredNorm();

        if (distSq <= radiusSq) {
            result.insert(pair.first);
        }
    }

    return result;
}

} // namespace selection
} // namespace koomesh
