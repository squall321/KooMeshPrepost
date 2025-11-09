#pragma once

#include "core/Mesh.h"
#include "core/ISpatialIndex.h"
#include "core/SpatialIndexFactory.h"
#include <vector>
#include <set>
#include <memory>
#include <functional>

namespace koomesh {
namespace selection {

/**
 * @brief Selection mode for combining selections
 */
enum class SelectionMode {
    REPLACE,  ///< Replace current selection
    ADD,      ///< Add to current selection
    SUBTRACT, ///< Remove from current selection
    INTERSECT ///< Intersect with current selection
};

/**
 * @brief Filter function type for element filtering
 */
using SelectionFilter = std::function<bool(core::ElementId, const core::Mesh&)>;

/**
 * @brief Manager for element selection with spatial index integration
 *
 * This class provides efficient element selection using spatial indexes:
 * - Box selection
 * - Sphere selection
 * - Point-based selection
 * - Nearest neighbor selection
 * - Custom region selection
 * - Selection filtering
 * - Selection history (undo/redo)
 *
 * Example usage:
 * @code
 * SelectionManager manager;
 * manager.setMesh(mesh);
 * manager.useSpatialIndex(core::SpatialIndexType::OCTREE);
 *
 * // Select elements in box
 * manager.selectByBox(minPt, maxPt, SelectionMode::REPLACE);
 *
 * // Add sphere selection
 * manager.selectBySphere(center, radius, SelectionMode::ADD);
 *
 * // Get selected elements
 * auto selected = manager.getSelection();
 * @endcode
 */
class SelectionManager {
public:
    /**
     * @brief Constructor
     */
    SelectionManager();

    /**
     * @brief Destructor
     */
    ~SelectionManager() = default;

    /**
     * @brief Set the mesh to select from
     * @param mesh The mesh
     */
    void setMesh(const core::Mesh& mesh);

    /**
     * @brief Enable spatial index for efficient selection
     * @param type Type of spatial index to use
     */
    void useSpatialIndex(core::SpatialIndexType type);

    /**
     * @brief Disable spatial index (use linear search)
     */
    void disableSpatialIndex();

    /**
     * @brief Select elements by bounding box
     * @param minPt Minimum point of box
     * @param maxPt Maximum point of box
     * @param mode Selection mode
     * @return Number of elements selected
     */
    size_t selectByBox(const Eigen::Vector3d& minPt,
                      const Eigen::Vector3d& maxPt,
                      SelectionMode mode = SelectionMode::REPLACE);

    /**
     * @brief Select elements by sphere
     * @param center Center of sphere
     * @param radius Radius
     * @param mode Selection mode
     * @return Number of elements selected
     */
    size_t selectBySphere(const Eigen::Vector3d& center,
                         double radius,
                         SelectionMode mode = SelectionMode::REPLACE);

    /**
     * @brief Select element at point
     * @param point Query point
     * @param mode Selection mode
     * @return True if element found and selected
     */
    bool selectAtPoint(const Eigen::Vector3d& point,
                      SelectionMode mode = SelectionMode::REPLACE);

    /**
     * @brief Select nearest N elements to point
     * @param point Query point
     * @param count Number of elements to select
     * @param mode Selection mode
     * @return Number of elements selected
     */
    size_t selectNearest(const Eigen::Vector3d& point,
                        size_t count,
                        SelectionMode mode = SelectionMode::REPLACE);

    /**
     * @brief Select all elements
     */
    void selectAll();

    /**
     * @brief Clear selection
     */
    void clearSelection();

    /**
     * @brief Invert selection
     */
    void invertSelection();

    /**
     * @brief Select by element IDs
     * @param elementIds IDs to select
     * @param mode Selection mode
     */
    void selectByIds(const std::vector<core::ElementId>& elementIds,
                    SelectionMode mode = SelectionMode::REPLACE);

    /**
     * @brief Set selection filter
     * @param filter Filter function (returns true for selectable elements)
     */
    void setFilter(SelectionFilter filter);

    /**
     * @brief Clear selection filter
     */
    void clearFilter();

    /**
     * @brief Get current selection
     * @return Set of selected element IDs
     */
    const std::set<core::ElementId>& getSelection() const { return m_selectedElements; }

    /**
     * @brief Get number of selected elements
     */
    size_t getSelectionCount() const { return m_selectedElements.size(); }

    /**
     * @brief Check if element is selected
     */
    bool isSelected(core::ElementId id) const;

    /**
     * @brief Enable selection history for undo/redo
     * @param maxHistory Maximum number of history entries
     */
    void enableHistory(size_t maxHistory = 50);

    /**
     * @brief Disable selection history
     */
    void disableHistory();

    /**
     * @brief Undo last selection operation
     * @return True if undo successful
     */
    bool undo();

    /**
     * @brief Redo last undone operation
     * @return True if redo successful
     */
    bool redo();

    /**
     * @brief Check if undo is available
     */
    bool canUndo() const { return m_historyEnabled && m_historyIndex > 0; }

    /**
     * @brief Check if redo is available
     */
    bool canRedo() const { return m_historyEnabled && m_historyIndex < m_history.size(); }

    /**
     * @brief Get selection statistics
     */
    std::string getStatistics() const;

private:
    /**
     * @brief Apply selection mode to combine selections
     */
    void applySelectionMode(const std::set<core::ElementId>& newSelection,
                           SelectionMode mode);

    /**
     * @brief Filter elements based on current filter
     */
    std::set<core::ElementId> applyFilter(const std::set<core::ElementId>& elements) const;

    /**
     * @brief Save current selection to history
     */
    void saveToHistory();

    /**
     * @brief Find elements in bounding box (without spatial index)
     */
    std::set<core::ElementId> findInBoxLinear(const core::BoundingBox& box) const;

    /**
     * @brief Find elements in sphere (without spatial index)
     */
    std::set<core::ElementId> findInSphereLinear(const Eigen::Vector3d& center,
                                                  double radius) const;

    const core::Mesh* m_mesh;
    std::unique_ptr<core::ISpatialIndex> m_spatialIndex;
    bool m_useSpatialIndex;

    std::set<core::ElementId> m_selectedElements;
    SelectionFilter m_filter;
    bool m_hasFilter;

    // History for undo/redo
    bool m_historyEnabled;
    size_t m_maxHistory;
    std::vector<std::set<core::ElementId>> m_history;
    size_t m_historyIndex;
};

} // namespace selection
} // namespace koomesh
