/**
 * @file AreaSelector.h
 * @brief 2D area selection for mesh elements
 */

#pragma once

#include "core/Mesh.h"
#include "core/ISpatialIndex.h"
#include <vector>
#include <functional>
#include <sstream>
#include <algorithm>

#ifdef KOOMESH_HAS_VTK
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkAreaPicker.h>
#include <vtkProp3DCollection.h>
#include <vtkActor.h>
#endif

namespace koomesh {
namespace visualization {

/**
 * @brief Selection mode for area selection
 */
enum class SelectionMode {
    REPLACE,    // Replace current selection
    ADD,        // Add to current selection
    SUBTRACT,   // Remove from current selection
    INTERSECT   // Intersect with current selection
};

/**
 * @brief Selection filter options
 */
struct SelectionFilter {
    bool selectNodes = false;           // Select nodes instead of elements
    bool selectVisible = true;          // Only select visible elements
    bool selectPartiallyEnclosed = true;// Select elements partially in area

    // Element type filter
    bool filterByType = false;
    std::vector<core::ElementType> allowedTypes;

    // Part filter
    bool filterByPart = false;
    std::vector<core::PartId> allowedParts;
};

/**
 * @brief Selection result information
 */
struct SelectionResult {
    std::vector<core::ElementId> elements;
    std::vector<core::NodeId> nodes;

    // Selection area in screen coordinates
    int startX, startY;
    int endX, endY;

    // Selection statistics
    size_t totalCandidates = 0;
    size_t filtered = 0;
    double selectionTime = 0.0;  // in milliseconds
};

#ifdef KOOMESH_HAS_VTK

/**
 * @brief Area-based selection system for mesh elements
 *
 * Provides 2D rectangular area selection functionality:
 * - Mouse drag rectangle selection
 * - VTK-based picking with hardware acceleration
 * - Spatial index integration for performance
 * - Multiple selection modes (replace, add, subtract, intersect)
 * - Element type and part filtering
 * - Partial vs full enclosure options
 */
class AreaSelector {
public:
    /**
     * @brief Constructor
     */
    AreaSelector();

    /**
     * @brief Destructor
     */
    ~AreaSelector();

    // ========================================================================
    // Selection Operations
    // ========================================================================

    /**
     * @brief Select elements in rectangular area
     * @param renderer VTK renderer
     * @param mesh Source mesh
     * @param startX Start X coordinate (screen)
     * @param startY Start Y coordinate (screen)
     * @param endX End X coordinate (screen)
     * @param endY End Y coordinate (screen)
     * @param mode Selection mode
     * @return Selection result
     */
    SelectionResult selectInArea(
        vtkRenderer* renderer,
        const core::Mesh& mesh,
        int startX, int startY,
        int endX, int endY,
        SelectionMode mode = SelectionMode::REPLACE
    );

    /**
     * @brief Select elements with spatial index optimization
     * @param renderer VTK renderer
     * @param mesh Source mesh
     * @param spatialIndex Spatial index for fast queries
     * @param startX Start X coordinate (screen)
     * @param startY Start Y coordinate (screen)
     * @param endX End X coordinate (screen)
     * @param endY End Y coordinate (screen)
     * @param mode Selection mode
     * @return Selection result
     */
    SelectionResult selectInAreaWithIndex(
        vtkRenderer* renderer,
        const core::Mesh& mesh,
        const core::ISpatialIndex& spatialIndex,
        int startX, int startY,
        int endX, int endY,
        SelectionMode mode = SelectionMode::REPLACE
    );

    /**
     * @brief Select using frustum (3D selection volume)
     * @param renderer VTK renderer
     * @param mesh Source mesh
     * @param spatialIndex Spatial index
     * @param startX Start X coordinate (screen)
     * @param startY Start Y coordinate (screen)
     * @param endX End X coordinate (screen)
     * @param endY End Y coordinate (screen)
     * @return Selected element IDs
     */
    std::vector<core::ElementId> selectByFrustum(
        vtkRenderer* renderer,
        const core::Mesh& mesh,
        const core::ISpatialIndex& spatialIndex,
        int startX, int startY,
        int endX, int endY
    );

    // ========================================================================
    // Filter Configuration
    // ========================================================================

    /**
     * @brief Set selection filter
     * @param filter Filter settings
     */
    void setFilter(const SelectionFilter& filter);

    /**
     * @brief Get current filter
     * @return Current filter settings
     */
    const SelectionFilter& getFilter() const;

    /**
     * @brief Enable/disable element type filtering
     * @param enable Enable flag
     * @param types Allowed element types
     */
    void setElementTypeFilter(bool enable, const std::vector<core::ElementType>& types = {});

    /**
     * @brief Enable/disable part filtering
     * @param enable Enable flag
     * @param parts Allowed part IDs
     */
    void setPartFilter(bool enable, const std::vector<core::PartId>& parts = {});

    // ========================================================================
    // Selection Behavior
    // ========================================================================

    /**
     * @brief Set whether to select partially enclosed elements
     * @param enable True to select partial, false for full enclosure only
     */
    void setPartialEnclosure(bool enable);

    /**
     * @brief Get partial enclosure setting
     * @return True if partial enclosure is enabled
     */
    bool getPartialEnclosure() const;

    /**
     * @brief Set selection tolerance (in pixels)
     * @param tolerance Tolerance value
     */
    void setTolerance(double tolerance);

    /**
     * @brief Get selection tolerance
     * @return Tolerance value in pixels
     */
    double getTolerance() const;

    // ========================================================================
    // Callback System
    // ========================================================================

    /**
     * @brief Set callback for selection events
     * @param callback Callback function
     */
    void setSelectionCallback(std::function<void(const SelectionResult&)> callback);

    /**
     * @brief Set callback for progress updates (for large selections)
     * @param callback Progress callback (0.0 to 1.0)
     */
    void setProgressCallback(std::function<void(double)> callback);

    // ========================================================================
    // Utility
    // ========================================================================

    /**
     * @brief Get last selection result
     * @return Last selection result
     */
    const SelectionResult& getLastResult() const;

    /**
     * @brief Clear last result
     */
    void clearLastResult();

    /**
     * @brief Get selection statistics
     * @return Statistics string
     */
    std::string getStatistics() const;

private:
    /**
     * @brief Pick elements using VTK area picker
     */
    std::vector<core::ElementId> pickWithVTK(
        vtkRenderer* renderer,
        int startX, int startY,
        int endX, int endY
    );

    /**
     * @brief Build frustum from screen coordinates
     */
    void buildFrustum(
        vtkRenderer* renderer,
        int startX, int startY,
        int endX, int endY
    );

    /**
     * @brief Test if element is in frustum
     */
    bool isElementInFrustum(
        const core::Element* element,
        const core::Mesh& mesh
    ) const;

    /**
     * @brief Apply filters to element list
     */
    std::vector<core::ElementId> applyFilters(
        const std::vector<core::ElementId>& elements,
        const core::Mesh& mesh
    ) const;

    /**
     * @brief Apply selection mode
     */
    std::vector<core::ElementId> applySelectionMode(
        const std::vector<core::ElementId>& newSelection,
        SelectionMode mode
    ) const;

    /**
     * @brief Check if element passes filter
     */
    bool passesFilter(const core::Element* element) const;

    // VTK picker
    vtkSmartPointer<vtkAreaPicker> m_areaPicker;

    // Frustum planes (6 planes: left, right, bottom, top, near, far)
    std::array<Eigen::Vector4d, 6> m_frustumPlanes;

    // Filter settings
    SelectionFilter m_filter;

    // Selection settings
    double m_tolerance;

    // Previous selection for mode operations
    std::vector<core::ElementId> m_previousSelection;

    // Last selection result
    SelectionResult m_lastResult;

    // Callbacks
    std::function<void(const SelectionResult&)> m_selectionCallback;
    std::function<void(double)> m_progressCallback;
};

#else // !KOOMESH_HAS_VTK

// Stub implementation when VTK is not available
class AreaSelector {
public:
    AreaSelector() : m_tolerance(2.0) {}
    ~AreaSelector() {}

    SelectionResult selectInArea(void*, const core::Mesh&, int startX, int startY, int endX, int endY, SelectionMode = SelectionMode::REPLACE) {
        SelectionResult result;
        result.startX = startX;
        result.startY = startY;
        result.endX = endX;
        result.endY = endY;
        m_lastResult = result;

        if (m_selectionCallback) {
            m_selectionCallback(result);
        }

        return result;
    }

    SelectionResult selectInAreaWithIndex(void*, const core::Mesh&, const core::ISpatialIndex&, int startX, int startY, int endX, int endY, SelectionMode = SelectionMode::REPLACE) {
        return selectInArea(nullptr, core::Mesh(), startX, startY, endX, endY);
    }

    std::vector<core::ElementId> selectByFrustum(void*, const core::Mesh&, const core::ISpatialIndex&, int, int, int, int) {
        return {};
    }

    void setFilter(const SelectionFilter& filter) { m_filter = filter; }
    const SelectionFilter& getFilter() const { return m_filter; }

    void setElementTypeFilter(bool enable, const std::vector<core::ElementType>& types = {}) {
        m_filter.filterByType = enable;
        m_filter.allowedTypes = types;
    }

    void setPartFilter(bool enable, const std::vector<core::PartId>& parts = {}) {
        m_filter.filterByPart = enable;
        m_filter.allowedParts = parts;
    }

    void setPartialEnclosure(bool enable) { m_filter.selectPartiallyEnclosed = enable; }
    bool getPartialEnclosure() const { return m_filter.selectPartiallyEnclosed; }

    void setTolerance(double tolerance) {
        m_tolerance = std::max(0.0, tolerance);
    }

    double getTolerance() const { return m_tolerance; }

    void setSelectionCallback(std::function<void(const SelectionResult&)> callback) {
        m_selectionCallback = callback;
    }

    void setProgressCallback(std::function<void(double)> callback) {
        m_progressCallback = callback;
    }

    const SelectionResult& getLastResult() const { return m_lastResult; }

    void clearLastResult() { m_lastResult = SelectionResult(); }

    std::string getStatistics() const {
        std::ostringstream oss;
        oss << "Last Selection Statistics:\n";
        oss << "  Area: (" << m_lastResult.startX << "," << m_lastResult.startY
            << ") to (" << m_lastResult.endX << "," << m_lastResult.endY << ")\n";
        oss << "  Selected: " << m_lastResult.elements.size() << "\n";
        return oss.str();
    }

private:
    SelectionFilter m_filter;
    double m_tolerance;
    SelectionResult m_lastResult;
    std::function<void(const SelectionResult&)> m_selectionCallback;
    std::function<void(double)> m_progressCallback;
};

#endif // KOOMESH_HAS_VTK

} // namespace visualization
} // namespace koomesh
