/**
 * @file HardwareSelector.h
 * @brief GPU-accelerated selection using VTK hardware selector
 */

#pragma once

#include "core/Mesh.h"
#include <vector>
#include <set>
#include <array>
#include <cstdint>
#include <map>
#include <algorithm>
#include <sstream>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkHardwareSelector.h>
#include <vtkSelection.h>
#include <vtkProp.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Selection field type for hardware selector
 */
enum class SelectionField {
    CELL,           // Select cells/elements
    POINT,          // Select points/nodes
    FIELD,          // Select by field data
    VERTEX          // Select vertices
};

/**
 * @brief Hardware selection result
 */
struct HardwareSelectionResult {
    std::vector<core::ElementId> elements;
    std::vector<core::NodeId> nodes;

#ifdef KOOMESH_HAS_VTK
    std::set<vtkIdType> vtkCellIds;     // VTK cell IDs
    std::set<vtkIdType> vtkPointIds;    // VTK point IDs
#else
    std::set<int64_t> vtkCellIds;       // VTK cell IDs (stub)
    std::set<int64_t> vtkPointIds;      // VTK point IDs (stub)
#endif

    int screenX = 0;
    int screenY = 0;
    int width = 0;
    int height = 0;

    size_t selectedCount = 0;
    double selectionTime = 0.0;  // milliseconds
    bool success = false;
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Hardware-accelerated selection using GPU
 *
 * Provides GPU-based selection using VTK's vtkHardwareSelector:
 * - Renders elements with unique color IDs
 * - Uses GPU for extremely fast selection
 * - Supports point (single click) and area selection
 * - Much faster than CPU-based picking for large meshes
 * - Handles occlusion automatically
 */
class HardwareSelector {
public:
    /**
     * @brief Constructor
     */
    HardwareSelector();

    /**
     * @brief Destructor
     */
    ~HardwareSelector();

    // ========================================================================
    // Point Selection (Single Click)
    // ========================================================================

    /**
     * @brief Select at screen point
     * @param renderer VTK renderer
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param field Selection field type
     * @return Selection result
     */
    HardwareSelectionResult selectAtPoint(
        vtkRenderer* renderer,
        int x, int y,
        SelectionField field = SelectionField::CELL
    );

    /**
     * @brief Select at point with tolerance
     * @param renderer VTK renderer
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param tolerance Tolerance in pixels
     * @param field Selection field type
     * @return Selection result
     */
    HardwareSelectionResult selectAtPointWithTolerance(
        vtkRenderer* renderer,
        int x, int y,
        int tolerance,
        SelectionField field = SelectionField::CELL
    );

    // ========================================================================
    // Area Selection
    // ========================================================================

    /**
     * @brief Select in rectangular area
     * @param renderer VTK renderer
     * @param x1 Start X coordinate
     * @param y1 Start Y coordinate
     * @param x2 End X coordinate
     * @param y2 End Y coordinate
     * @param field Selection field type
     * @return Selection result
     */
    HardwareSelectionResult selectInArea(
        vtkRenderer* renderer,
        int x1, int y1,
        int x2, int y2,
        SelectionField field = SelectionField::CELL
    );

    /**
     * @brief Select visible elements in area
     * @param renderer VTK renderer
     * @param x1 Start X coordinate
     * @param y1 Start Y coordinate
     * @param x2 End X coordinate
     * @param y2 End Y coordinate
     * @return Selection result
     */
    HardwareSelectionResult selectVisibleInArea(
        vtkRenderer* renderer,
        int x1, int y1,
        int x2, int y2
    );

    // ========================================================================
    // Polygon Selection (Advanced)
    // ========================================================================

    /**
     * @brief Select in polygonal region
     * @param renderer VTK renderer
     * @param polygonPoints Screen coordinates of polygon vertices
     * @param field Selection field type
     * @return Selection result
     */
    HardwareSelectionResult selectInPolygon(
        vtkRenderer* renderer,
        const std::vector<std::array<int, 2>>& polygonPoints,
        SelectionField field = SelectionField::CELL
    );

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set field association for selection
     * @param field Field type
     */
    void setFieldAssociation(SelectionField field);

    /**
     * @brief Get current field association
     * @return Current field type
     */
    SelectionField getFieldAssociation() const;

    /**
     * @brief Enable/disable selection of occluded elements
     * @param enable True to select all in volume (not just visible)
     */
    void setSelectOccluded(bool enable);

    /**
     * @brief Get occluded selection setting
     * @return True if selecting occluded elements
     */
    bool getSelectOccluded() const;

    /**
     * @brief Set maximum number of selected items (0 = unlimited)
     * @param max Maximum count
     */
    void setMaxSelectionCount(size_t max);

    /**
     * @brief Get maximum selection count
     * @return Maximum count
     */
    size_t getMaxSelectionCount() const;

    // ========================================================================
    // ID Mapping
    // ========================================================================

    /**
     * @brief Map VTK cell IDs to mesh element IDs
     * @param vtkCellIds VTK cell IDs
     * @param mesh Source mesh
     * @return Mesh element IDs
     */
    std::vector<core::ElementId> mapCellIdsToElements(
        const std::set<vtkIdType>& vtkCellIds,
        const core::Mesh& mesh
    ) const;

    /**
     * @brief Map VTK point IDs to mesh node IDs
     * @param vtkPointIds VTK point IDs
     * @param mesh Source mesh
     * @return Mesh node IDs
     */
    std::vector<core::NodeId> mapPointIdsToNodes(
        const std::set<vtkIdType>& vtkPointIds,
        const core::Mesh& mesh
    ) const;

    /**
     * @brief Register ID mapping for actor
     * @param prop VTK prop/actor
     * @param elementIds Element IDs for this actor
     */
    void registerActorMapping(
        vtkProp* prop,
        const std::vector<core::ElementId>& elementIds
    );

    /**
     * @brief Clear all ID mappings
     */
    void clearMappings();

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Get last selection result
     * @return Last result
     */
    const HardwareSelectionResult& getLastResult() const;

    /**
     * @brief Clear last result
     */
    void clearLastResult();

    /**
     * @brief Get selection statistics
     * @return Statistics string
     */
    std::string getStatistics() const;

    /**
     * @brief Check if hardware selection is supported
     * @param renderer VTK renderer
     * @return True if supported
     */
    static bool isSupported(vtkRenderer* renderer);

private:
    /**
     * @brief Perform selection using hardware selector
     */
    vtkSmartPointer<vtkSelection> performSelection(
        vtkRenderer* renderer,
        int x1, int y1,
        int x2, int y2
    );

    /**
     * @brief Extract IDs from VTK selection
     */
    void extractIdsFromSelection(
        vtkSelection* selection,
        HardwareSelectionResult& result
    );

    /**
     * @brief Convert selection field to VTK field association
     */
    int getVTKFieldAssociation(SelectionField field) const;

    // Hardware selector
    vtkSmartPointer<vtkHardwareSelector> m_selector;

    // Configuration
    SelectionField m_fieldAssociation;
    bool m_selectOccluded;
    size_t m_maxSelectionCount;

    // ID mapping (VTK ID -> Mesh ID)
    std::map<vtkProp*, std::vector<core::ElementId>> m_actorToElements;
    std::map<vtkProp*, std::vector<core::NodeId>> m_actorToNodes;

    // Last result
    HardwareSelectionResult m_lastResult;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class HardwareSelector {
public:
    HardwareSelector() : m_fieldAssociation(SelectionField::CELL), m_selectOccluded(false), m_maxSelectionCount(0) {}
    ~HardwareSelector() {}

    HardwareSelectionResult selectAtPoint(void*, int x, int y, SelectionField = SelectionField::CELL) {
        HardwareSelectionResult result;
        result.screenX = x;
        result.screenY = y;
        m_lastResult = result;
        return result;
    }

    HardwareSelectionResult selectAtPointWithTolerance(void*, int x, int y, int tolerance, SelectionField = SelectionField::CELL) {
        HardwareSelectionResult result;
        result.screenX = x;
        result.screenY = y;
        result.width = tolerance * 2;
        result.height = tolerance * 2;
        m_lastResult = result;
        return result;
    }

    HardwareSelectionResult selectInArea(void*, int x1, int y1, int x2, int y2, SelectionField = SelectionField::CELL) {
        HardwareSelectionResult result;
        // Normalize coordinates
        int minX = std::min(x1, x2);
        int minY = std::min(y1, y2);
        int maxX = std::max(x1, x2);
        int maxY = std::max(y1, y2);

        result.screenX = minX;
        result.screenY = minY;
        result.width = maxX - minX;
        result.height = maxY - minY;
        m_lastResult = result;
        return result;
    }

    HardwareSelectionResult selectVisibleInArea(void*, int x1, int y1, int x2, int y2) {
        return selectInArea(nullptr, x1, y1, x2, y2);
    }

    HardwareSelectionResult selectInPolygon(void*, const std::vector<std::array<int, 2>>& polygonPoints, SelectionField = SelectionField::CELL) {
        HardwareSelectionResult result;

        if (!polygonPoints.empty()) {
            // Calculate bounding box
            int minX = polygonPoints[0][0];
            int minY = polygonPoints[0][1];
            int maxX = polygonPoints[0][0];
            int maxY = polygonPoints[0][1];

            for (const auto& point : polygonPoints) {
                minX = std::min(minX, point[0]);
                minY = std::min(minY, point[1]);
                maxX = std::max(maxX, point[0]);
                maxY = std::max(maxY, point[1]);
            }

            result.screenX = minX;
            result.screenY = minY;
            result.width = maxX - minX;
            result.height = maxY - minY;
        }

        m_lastResult = result;
        return result;
    }

    void setFieldAssociation(SelectionField field) { m_fieldAssociation = field; }
    SelectionField getFieldAssociation() const { return m_fieldAssociation; }
    void setSelectOccluded(bool enable) { m_selectOccluded = enable; }
    bool getSelectOccluded() const { return m_selectOccluded; }
    void setMaxSelectionCount(size_t max) { m_maxSelectionCount = max; }
    size_t getMaxSelectionCount() const { return m_maxSelectionCount; }

    std::vector<core::ElementId> mapCellIdsToElements(const std::set<int64_t>&, const core::Mesh&) const { return {}; }
    std::vector<core::NodeId> mapPointIdsToNodes(const std::set<int64_t>&, const core::Mesh&) const { return {}; }
    void registerActorMapping(void*, const std::vector<core::ElementId>&) {}
    void clearMappings() {}

    const HardwareSelectionResult& getLastResult() const { return m_lastResult; }
    void clearLastResult() { m_lastResult = HardwareSelectionResult(); }
    std::string getStatistics() const {
        std::ostringstream oss;
        oss << "Hardware Selection Statistics (VTK not available):\n";
        oss << "  Screen Position: (" << m_lastResult.screenX << ", " << m_lastResult.screenY << ")\n";
        oss << "  Area Size: " << m_lastResult.width << " x " << m_lastResult.height << "\n";
        oss << "  Selected Elements: " << m_lastResult.elements.size() << "\n";
        oss << "  Selected Nodes: " << m_lastResult.nodes.size() << "\n";
        return oss.str();
    }
    static bool isSupported(void*) { return false; }

private:
    SelectionField m_fieldAssociation;
    bool m_selectOccluded;
    size_t m_maxSelectionCount;
    HardwareSelectionResult m_lastResult;
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
