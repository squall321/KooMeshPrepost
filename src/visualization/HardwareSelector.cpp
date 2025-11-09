/**
 * @file HardwareSelector.cpp
 * @brief Implementation of HardwareSelector
 */

#include "visualization/HardwareSelector.h"

#ifdef KOOMESH_HAS_VTK

#include <vtkSelectionNode.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <chrono>
#include <sstream>
#include <algorithm>

namespace koomesh {
namespace visualization {

// ============================================================================
// Constructor / Destructor
// ============================================================================

HardwareSelector::HardwareSelector()
    : m_selector(vtkSmartPointer<vtkHardwareSelector>::New())
    , m_fieldAssociation(SelectionField::CELL)
    , m_selectOccluded(false)
    , m_maxSelectionCount(0)
{
    // Configure hardware selector for best performance
    m_selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

HardwareSelector::~HardwareSelector()
{
}

// ============================================================================
// Point Selection
// ============================================================================

HardwareSelectionResult HardwareSelector::selectAtPoint(
    vtkRenderer* renderer,
    int x, int y,
    SelectionField field)
{
    auto start = std::chrono::high_resolution_clock::now();

    HardwareSelectionResult result;
    result.screenX = x;
    result.screenY = y;
    result.width = 0;
    result.height = 0;

    if (!renderer) {
        return result;
    }

    // Set field association
    setFieldAssociation(field);

    // Perform selection at single point
    auto selection = performSelection(renderer, x, y, x, y);

    if (selection) {
        extractIdsFromSelection(selection, result);
        result.success = true;
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.selectionTime = std::chrono::duration<double, std::milli>(end - start).count();

    m_lastResult = result;
    return result;
}

HardwareSelectionResult HardwareSelector::selectAtPointWithTolerance(
    vtkRenderer* renderer,
    int x, int y,
    int tolerance,
    SelectionField field)
{
    auto start = std::chrono::high_resolution_clock::now();

    HardwareSelectionResult result;
    result.screenX = x;
    result.screenY = y;
    result.width = tolerance * 2;
    result.height = tolerance * 2;

    if (!renderer) {
        return result;
    }

    // Set field association
    setFieldAssociation(field);

    // Create selection area around point
    int x1 = x - tolerance;
    int y1 = y - tolerance;
    int x2 = x + tolerance;
    int y2 = y + tolerance;

    auto selection = performSelection(renderer, x1, y1, x2, y2);

    if (selection) {
        extractIdsFromSelection(selection, result);
        result.success = true;
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.selectionTime = std::chrono::duration<double, std::milli>(end - start).count();

    m_lastResult = result;
    return result;
}

// ============================================================================
// Area Selection
// ============================================================================

HardwareSelectionResult HardwareSelector::selectInArea(
    vtkRenderer* renderer,
    int x1, int y1,
    int x2, int y2,
    SelectionField field)
{
    auto start = std::chrono::high_resolution_clock::now();

    HardwareSelectionResult result;
    result.screenX = std::min(x1, x2);
    result.screenY = std::min(y1, y2);
    result.width = std::abs(x2 - x1);
    result.height = std::abs(y2 - y1);

    if (!renderer) {
        return result;
    }

    // Set field association
    setFieldAssociation(field);

    // Perform area selection
    auto selection = performSelection(renderer, x1, y1, x2, y2);

    if (selection) {
        extractIdsFromSelection(selection, result);
        result.success = true;
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.selectionTime = std::chrono::duration<double, std::milli>(end - start).count();

    m_lastResult = result;
    return result;
}

HardwareSelectionResult HardwareSelector::selectVisibleInArea(
    vtkRenderer* renderer,
    int x1, int y1,
    int x2, int y2)
{
    // Set to only select visible (not occluded) elements
    bool previousOccluded = m_selectOccluded;
    setSelectOccluded(false);

    auto result = selectInArea(renderer, x1, y1, x2, y2, SelectionField::CELL);

    // Restore previous setting
    setSelectOccluded(previousOccluded);

    return result;
}

// ============================================================================
// Polygon Selection
// ============================================================================

HardwareSelectionResult HardwareSelector::selectInPolygon(
    vtkRenderer* renderer,
    const std::vector<std::array<int, 2>>& polygonPoints,
    SelectionField field)
{
    HardwareSelectionResult result;

    if (!renderer || polygonPoints.empty()) {
        return result;
    }

    // For now, use bounding box of polygon
    // Full polygon selection would require custom VTK filter
    int minX = polygonPoints[0][0];
    int maxX = polygonPoints[0][0];
    int minY = polygonPoints[0][1];
    int maxY = polygonPoints[0][1];

    for (const auto& point : polygonPoints) {
        minX = std::min(minX, point[0]);
        maxX = std::max(maxX, point[0]);
        minY = std::min(minY, point[1]);
        maxY = std::max(maxY, point[1]);
    }

    // Use area selection with bounding box
    result = selectInArea(renderer, minX, minY, maxX, maxY, field);

    return result;
}

// ============================================================================
// Configuration
// ============================================================================

void HardwareSelector::setFieldAssociation(SelectionField field)
{
    m_fieldAssociation = field;
    m_selector->SetFieldAssociation(getVTKFieldAssociation(field));
}

SelectionField HardwareSelector::getFieldAssociation() const
{
    return m_fieldAssociation;
}

void HardwareSelector::setSelectOccluded(bool enable)
{
    m_selectOccluded = enable;
    // Note: VTK hardware selector always selects only visible by default
    // To select occluded, would need different approach
}

bool HardwareSelector::getSelectOccluded() const
{
    return m_selectOccluded;
}

void HardwareSelector::setMaxSelectionCount(size_t max)
{
    m_maxSelectionCount = max;
}

size_t HardwareSelector::getMaxSelectionCount() const
{
    return m_maxSelectionCount;
}

// ============================================================================
// ID Mapping
// ============================================================================

std::vector<core::ElementId> HardwareSelector::mapCellIdsToElements(
    const std::set<vtkIdType>& vtkCellIds,
    const core::Mesh& mesh) const
{
    std::vector<core::ElementId> elements;
    elements.reserve(vtkCellIds.size());

    // Direct mapping: VTK cell ID corresponds to mesh element ID
    // In production, would use registered mappings
    for (vtkIdType cellId : vtkCellIds) {
        if (cellId >= 0 && static_cast<size_t>(cellId) < mesh.elementCount()) {
            // This is simplified - actual implementation needs proper mapping
            auto allIds = mesh.getAllElementIds();
            if (static_cast<size_t>(cellId) < allIds.size()) {
                elements.push_back(allIds[static_cast<size_t>(cellId)]);
            }
        }
    }

    return elements;
}

std::vector<core::NodeId> HardwareSelector::mapPointIdsToNodes(
    const std::set<vtkIdType>& vtkPointIds,
    const core::Mesh& mesh) const
{
    std::vector<core::NodeId> nodes;
    nodes.reserve(vtkPointIds.size());

    // Direct mapping: VTK point ID corresponds to mesh node ID
    for (vtkIdType pointId : vtkPointIds) {
        if (pointId >= 0 && static_cast<size_t>(pointId) < mesh.nodeCount()) {
            auto allIds = mesh.getAllNodeIds();
            if (static_cast<size_t>(pointId) < allIds.size()) {
                nodes.push_back(allIds[static_cast<size_t>(pointId)]);
            }
        }
    }

    return nodes;
}

void HardwareSelector::registerActorMapping(
    vtkProp* prop,
    const std::vector<core::ElementId>& elementIds)
{
    if (prop) {
        m_actorToElements[prop] = elementIds;
    }
}

void HardwareSelector::clearMappings()
{
    m_actorToElements.clear();
    m_actorToNodes.clear();
}

// ============================================================================
// Utility
// ============================================================================

const HardwareSelectionResult& HardwareSelector::getLastResult() const
{
    return m_lastResult;
}

void HardwareSelector::clearLastResult()
{
    m_lastResult = HardwareSelectionResult();
}

std::string HardwareSelector::getStatistics() const
{
    std::ostringstream oss;
    oss << "Hardware Selection Statistics:\n";
    oss << "  Screen Position: (" << m_lastResult.screenX << ", " << m_lastResult.screenY << ")\n";
    oss << "  Area Size: " << m_lastResult.width << " x " << m_lastResult.height << "\n";
    oss << "  VTK Cells Selected: " << m_lastResult.vtkCellIds.size() << "\n";
    oss << "  VTK Points Selected: " << m_lastResult.vtkPointIds.size() << "\n";
    oss << "  Mesh Elements: " << m_lastResult.elements.size() << "\n";
    oss << "  Mesh Nodes: " << m_lastResult.nodes.size() << "\n";
    oss << "  Selection Time: " << m_lastResult.selectionTime << " ms\n";
    oss << "  Success: " << (m_lastResult.success ? "Yes" : "No") << "\n";
    return oss.str();
}

bool HardwareSelector::isSupported(vtkRenderer* renderer)
{
    if (!renderer) {
        return false;
    }

    // Check if render window supports hardware selection
    auto renderWindow = renderer->GetRenderWindow();
    if (!renderWindow) {
        return false;
    }

    // Hardware selection requires OpenGL context
    return true;
}

// ============================================================================
// Private Methods
// ============================================================================

vtkSmartPointer<vtkSelection> HardwareSelector::performSelection(
    vtkRenderer* renderer,
    int x1, int y1,
    int x2, int y2)
{
    if (!renderer) {
        return nullptr;
    }

    // Set renderer
    m_selector->SetRenderer(renderer);

    // Set selection area
    int* windowSize = renderer->GetRenderWindow()->GetSize();

    // Ensure coordinates are within window bounds
    x1 = std::max(0, std::min(x1, windowSize[0] - 1));
    x2 = std::max(0, std::min(x2, windowSize[0] - 1));
    y1 = std::max(0, std::min(y1, windowSize[1] - 1));
    y2 = std::max(0, std::min(y2, windowSize[1] - 1));

    // Ensure proper ordering
    int minX = std::min(x1, x2);
    int maxX = std::max(x1, x2);
    int minY = std::min(y1, y2);
    int maxY = std::max(y1, y2);

    m_selector->SetArea(minX, minY, maxX, maxY);

    // Perform selection
    auto selection = m_selector->Select();

    return selection;
}

void HardwareSelector::extractIdsFromSelection(
    vtkSelection* selection,
    HardwareSelectionResult& result)
{
    if (!selection) {
        return;
    }

    // Get number of selection nodes
    unsigned int numNodes = selection->GetNumberOfNodes();

    for (unsigned int i = 0; i < numNodes; ++i) {
        auto node = selection->GetNode(i);
        if (!node) {
            continue;
        }

        // Get selection list (array of IDs)
        auto selectionList = node->GetSelectionList();
        if (!selectionList) {
            continue;
        }

        // Extract IDs based on field type
        int fieldType = node->GetFieldType();

        vtkIdType numIds = selectionList->GetNumberOfTuples();

        // Apply max count limit if set
        if (m_maxSelectionCount > 0 && static_cast<size_t>(numIds) > m_maxSelectionCount) {
            numIds = static_cast<vtkIdType>(m_maxSelectionCount);
        }

        for (vtkIdType j = 0; j < numIds; ++j) {
            vtkIdType id = selectionList->GetValue(j);

            if (fieldType == vtkSelectionNode::CELL) {
                result.vtkCellIds.insert(id);
            } else if (fieldType == vtkSelectionNode::POINT) {
                result.vtkPointIds.insert(id);
            }
        }
    }

    result.selectedCount = result.vtkCellIds.size() + result.vtkPointIds.size();
}

int HardwareSelector::getVTKFieldAssociation(SelectionField field) const
{
    switch (field) {
        case SelectionField::CELL:
            return vtkDataObject::FIELD_ASSOCIATION_CELLS;
        case SelectionField::POINT:
            return vtkDataObject::FIELD_ASSOCIATION_POINTS;
        case SelectionField::FIELD:
            return vtkDataObject::FIELD_ASSOCIATION_NONE;
        case SelectionField::VERTEX:
            return vtkDataObject::FIELD_ASSOCIATION_VERTICES;
        default:
            return vtkDataObject::FIELD_ASSOCIATION_CELLS;
    }
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
