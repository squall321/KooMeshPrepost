/**
 * @file AreaSelector.cpp
 * @brief Implementation of AreaSelector
 */

#include "visualization/AreaSelector.h"

#ifdef KOOMESH_HAS_VTK

#include <vtkCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkPlanes.h>
#include <vtkPoints.h>
#include <vtkDataSet.h>
#include <vtkCellArray.h>
#include <vtkIdTypeArray.h>
#include <algorithm>
#include <chrono>
#include <sstream>

namespace koomesh {
namespace visualization {

// ============================================================================
// Constructor / Destructor
// ============================================================================

AreaSelector::AreaSelector()
    : m_areaPicker(vtkSmartPointer<vtkAreaPicker>::New())
    , m_tolerance(2.0)
{
    // Initialize frustum planes
    for (auto& plane : m_frustumPlanes) {
        plane = Eigen::Vector4d(0, 0, 0, 0);
    }
}

AreaSelector::~AreaSelector()
{
}

// ============================================================================
// Selection Operations
// ============================================================================

SelectionResult AreaSelector::selectInArea(
    vtkRenderer* renderer,
    const core::Mesh& mesh,
    int startX, int startY,
    int endX, int endY,
    SelectionMode mode)
{
    auto start = std::chrono::high_resolution_clock::now();

    SelectionResult result;
    result.startX = startX;
    result.startY = startY;
    result.endX = endX;
    result.endY = endY;

    if (!renderer) {
        return result;
    }

    // Normalize coordinates (ensure min/max order)
    int minX = std::min(startX, endX);
    int maxX = std::max(startX, endX);
    int minY = std::min(startY, endY);
    int maxY = std::max(startY, endY);

    // Use VTK area picker for initial selection
    std::vector<core::ElementId> picked = pickWithVTK(renderer, minX, minY, maxX, maxY);
    result.totalCandidates = picked.size();

    // Apply filters
    std::vector<core::ElementId> filtered = applyFilters(picked, mesh);
    result.filtered = result.totalCandidates - filtered.size();

    // Apply selection mode
    result.elements = applySelectionMode(filtered, mode);

    auto end = std::chrono::high_resolution_clock::now();
    result.selectionTime = std::chrono::duration<double, std::milli>(end - start).count();

    // Store for future mode operations
    m_previousSelection = result.elements;
    m_lastResult = result;

    // Trigger callback
    if (m_selectionCallback) {
        m_selectionCallback(result);
    }

    return result;
}

SelectionResult AreaSelector::selectInAreaWithIndex(
    vtkRenderer* renderer,
    const core::Mesh& mesh,
    const core::ISpatialIndex& spatialIndex,
    int startX, int startY,
    int endX, int endY,
    SelectionMode mode)
{
    auto start = std::chrono::high_resolution_clock::now();

    SelectionResult result;
    result.startX = startX;
    result.startY = startY;
    result.endX = endX;
    result.endY = endY;

    if (!renderer) {
        return result;
    }

    // Use frustum-based selection with spatial index
    std::vector<core::ElementId> selected = selectByFrustum(
        renderer, mesh, spatialIndex,
        startX, startY, endX, endY
    );

    result.totalCandidates = selected.size();

    // Apply filters
    std::vector<core::ElementId> filtered = applyFilters(selected, mesh);
    result.filtered = result.totalCandidates - filtered.size();

    // Apply selection mode
    result.elements = applySelectionMode(filtered, mode);

    auto end = std::chrono::high_resolution_clock::now();
    result.selectionTime = std::chrono::duration<double, std::milli>(end - start).count();

    m_previousSelection = result.elements;
    m_lastResult = result;

    if (m_selectionCallback) {
        m_selectionCallback(result);
    }

    return result;
}

std::vector<core::ElementId> AreaSelector::selectByFrustum(
    vtkRenderer* renderer,
    const core::Mesh& mesh,
    const core::ISpatialIndex& spatialIndex,
    int startX, int startY,
    int endX, int endY)
{
    if (!renderer) {
        return {};
    }

    // Build frustum from screen coordinates
    buildFrustum(renderer, startX, startY, endX, endY);

    // Get bounding box of frustum (approximate)
    // For now, query all elements - in production, compute frustum bounds
    std::vector<core::ElementId> candidates;

    // Get all element IDs
    auto elementIds = mesh.getAllElementIds();

    // Iterate all elements and test against frustum
    size_t count = 0;
    for (const auto& elementId : elementIds) {
        const auto* element = mesh.getElement(elementId);

        if (element && isElementInFrustum(element, mesh)) {
            candidates.push_back(elementId);
        }

        // Progress callback for large meshes
        if (m_progressCallback && count % 10000 == 0) {
            m_progressCallback(static_cast<double>(count) / elementIds.size());
        }
        ++count;
    }

    if (m_progressCallback) {
        m_progressCallback(1.0);
    }

    return candidates;
}

// ============================================================================
// Filter Configuration
// ============================================================================

void AreaSelector::setFilter(const SelectionFilter& filter)
{
    m_filter = filter;
}

const SelectionFilter& AreaSelector::getFilter() const
{
    return m_filter;
}

void AreaSelector::setElementTypeFilter(bool enable, const std::vector<core::ElementType>& types)
{
    m_filter.filterByType = enable;
    m_filter.allowedTypes = types;
}

void AreaSelector::setPartFilter(bool enable, const std::vector<core::PartId>& parts)
{
    m_filter.filterByPart = enable;
    m_filter.allowedParts = parts;
}

// ============================================================================
// Selection Behavior
// ============================================================================

void AreaSelector::setPartialEnclosure(bool enable)
{
    m_filter.selectPartiallyEnclosed = enable;
}

bool AreaSelector::getPartialEnclosure() const
{
    return m_filter.selectPartiallyEnclosed;
}

void AreaSelector::setTolerance(double tolerance)
{
    m_tolerance = std::max(0.0, tolerance);
}

double AreaSelector::getTolerance() const
{
    return m_tolerance;
}

// ============================================================================
// Callback System
// ============================================================================

void AreaSelector::setSelectionCallback(std::function<void(const SelectionResult&)> callback)
{
    m_selectionCallback = callback;
}

void AreaSelector::setProgressCallback(std::function<void(double)> callback)
{
    m_progressCallback = callback;
}

// ============================================================================
// Utility
// ============================================================================

const SelectionResult& AreaSelector::getLastResult() const
{
    return m_lastResult;
}

void AreaSelector::clearLastResult()
{
    m_lastResult = SelectionResult();
}

std::string AreaSelector::getStatistics() const
{
    std::ostringstream oss;
    oss << "Last Selection Statistics:\n";
    oss << "  Area: (" << m_lastResult.startX << "," << m_lastResult.startY
        << ") to (" << m_lastResult.endX << "," << m_lastResult.endY << ")\n";
    oss << "  Candidates: " << m_lastResult.totalCandidates << "\n";
    oss << "  Filtered out: " << m_lastResult.filtered << "\n";
    oss << "  Selected: " << m_lastResult.elements.size() << "\n";
    oss << "  Time: " << m_lastResult.selectionTime << " ms\n";
    return oss.str();
}

// ============================================================================
// Private Methods
// ============================================================================

std::vector<core::ElementId> AreaSelector::pickWithVTK(
    vtkRenderer* renderer,
    int startX, int startY,
    int endX, int endY)
{
    std::vector<core::ElementId> result;

    // Perform area pick
    if (m_areaPicker->AreaPick(startX, startY, endX, endY, renderer) == 0) {
        return result;  // Pick failed
    }

    // Get picked props
    vtkProp3DCollection* props = m_areaPicker->GetProp3Ds();
    if (!props) {
        return result;
    }

    // Extract element IDs from picked actors
    // Note: In a real implementation, you'd need a mapping from VTK actors to element IDs
    // For now, return empty - this should be integrated with ActorManager

    return result;
}

void AreaSelector::buildFrustum(
    vtkRenderer* renderer,
    int startX, int startY,
    int endX, int endY)
{
    if (!renderer) {
        return;
    }

    vtkCamera* camera = renderer->GetActiveCamera();
    if (!camera) {
        return;
    }

    // Get viewport dimensions
    int* viewportSize = renderer->GetSize();
    int width = viewportSize[0];
    int height = viewportSize[1];

    // Normalize coordinates to [-1, 1]
    double ndcMinX = (2.0 * std::min(startX, endX) / width) - 1.0;
    double ndcMaxX = (2.0 * std::max(startX, endX) / width) - 1.0;
    double ndcMinY = (2.0 * std::min(startY, endY) / height) - 1.0;
    double ndcMaxY = (2.0 * std::max(startY, endY) / height) - 1.0;

    // Build frustum planes
    // This is a simplified version - full implementation would use proper
    // projection and view matrices

    // For now, store the screen-space bounds
    // In production, you'd compute the 6 frustum planes in world space
    m_frustumPlanes[0] = Eigen::Vector4d(1, 0, 0, -ndcMinX);   // left
    m_frustumPlanes[1] = Eigen::Vector4d(-1, 0, 0, ndcMaxX);   // right
    m_frustumPlanes[2] = Eigen::Vector4d(0, 1, 0, -ndcMinY);   // bottom
    m_frustumPlanes[3] = Eigen::Vector4d(0, -1, 0, ndcMaxY);   // top
    m_frustumPlanes[4] = Eigen::Vector4d(0, 0, 1, -0.1);       // near
    m_frustumPlanes[5] = Eigen::Vector4d(0, 0, -1, 1000.0);    // far
}

bool AreaSelector::isElementInFrustum(
    const core::Element* element,
    const core::Mesh& mesh) const
{
    if (!element) {
        return false;
    }

    // Get element bounding box
    core::BoundingBox bbox = element->computeBoundingBox(mesh);

    // Simple containment test - check if bbox center is in frustum
    // In production, do proper bbox-frustum intersection test
    Eigen::Vector3d center = bbox.center();

    // For now, always return true (select all) since we don't have
    // a proper world-to-screen transformation here
    // This should be implemented with proper camera matrices

    return true;
}

std::vector<core::ElementId> AreaSelector::applyFilters(
    const std::vector<core::ElementId>& elements,
    const core::Mesh& mesh) const
{
    std::vector<core::ElementId> filtered;
    filtered.reserve(elements.size());

    for (const auto& id : elements) {
        const auto* element = mesh.getElement(id);
        if (element && passesFilter(element)) {
            filtered.push_back(id);
        }
    }

    return filtered;
}

std::vector<core::ElementId> AreaSelector::applySelectionMode(
    const std::vector<core::ElementId>& newSelection,
    SelectionMode mode) const
{
    switch (mode) {
        case SelectionMode::REPLACE:
            return newSelection;

        case SelectionMode::ADD: {
            std::vector<core::ElementId> result = m_previousSelection;
            for (const auto& id : newSelection) {
                if (std::find(result.begin(), result.end(), id) == result.end()) {
                    result.push_back(id);
                }
            }
            return result;
        }

        case SelectionMode::SUBTRACT: {
            std::vector<core::ElementId> result;
            for (const auto& id : m_previousSelection) {
                if (std::find(newSelection.begin(), newSelection.end(), id) == newSelection.end()) {
                    result.push_back(id);
                }
            }
            return result;
        }

        case SelectionMode::INTERSECT: {
            std::vector<core::ElementId> result;
            for (const auto& id : newSelection) {
                if (std::find(m_previousSelection.begin(), m_previousSelection.end(), id) != m_previousSelection.end()) {
                    result.push_back(id);
                }
            }
            return result;
        }

        default:
            return newSelection;
    }
}

bool AreaSelector::passesFilter(const core::Element* element) const
{
    if (!element) {
        return false;
    }

    // Element type filter
    if (m_filter.filterByType) {
        auto type = element->type();
        if (std::find(m_filter.allowedTypes.begin(), m_filter.allowedTypes.end(), type)
            == m_filter.allowedTypes.end()) {
            return false;
        }
    }

    // Part filter
    if (m_filter.filterByPart) {
        auto partId = element->partId();
        if (std::find(m_filter.allowedParts.begin(), m_filter.allowedParts.end(), partId)
            == m_filter.allowedParts.end()) {
            return false;
        }
    }

    return true;
}

} // namespace visualization
} // namespace koomesh

#else // !KOOMESH_HAS_VTK

// Empty implementation when VTK is not available
namespace koomesh {
namespace visualization {

// No implementation needed for stub class

} // namespace visualization
} // namespace koomesh

#endif // KOOMESH_HAS_VTK
