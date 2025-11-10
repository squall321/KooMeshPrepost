/**
 * @file PartsPanel.h
 * @brief Parts hierarchy panel with tree view
 *
 * Displays mesh parts in a tree structure with:
 * - Part visibility toggles
 * - Part properties (material, element count, etc.)
 * - Context menu for part operations
 * - Color customization per part
 *
 * Uses Qt6 under LGPL v3 license (dynamically linked)
 */

#pragma once

#include "core/Types.h"
#include <memory>
#include <vector>

#ifdef KOOMESH_HAS_QT

#include <QWidget>
#include <QColor>

// Forward declarations - Qt
class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QModelIndex;
class QPoint;
class QVBoxLayout;
class QPushButton;
class QLineEdit;

// Forward declarations - KooMesh
namespace koomesh {

namespace core {
class Mesh;
class Part;
}

namespace ui {

/**
 * @brief Parts panel showing hierarchical part structure
 *
 * Tree view displaying all parts with their properties:
 * ```
 * Parts
 * ├── ☑ Part 1 (Steel)
 * │   ├── Elements: 10,542
 * │   └── Nodes: 12,345
 * ├── ☑ Part 2 (Aluminum)
 * │   ├── Elements: 8,231
 * │   └── Nodes: 9,876
 * └── ☐ Part 3 (Rubber)
 *     ├── Elements: 5,432
 *     └── Nodes: 6,543
 * ```
 *
 * Features:
 * - Check boxes for visibility control
 * - Double-click to focus camera on part
 * - Right-click context menu
 * - Color indicators per part
 * - Search/filter functionality
 */
class PartsPanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit PartsPanel(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~PartsPanel() override;

    /**
     * @brief Set mesh to display
     * @param mesh Pointer to mesh object (not owned)
     */
    void setMesh(const core::Mesh* mesh);

    /**
     * @brief Refresh parts list from current mesh
     */
    void refresh();

    /**
     * @brief Clear all parts
     */
    void clear();

    /**
     * @brief Set part visibility
     * @param partId Part ID
     * @param visible Visibility state
     */
    void setPartVisibility(core::PartId partId, bool visible);

    /**
     * @brief Set part color
     * @param partId Part ID
     * @param color Color to set
     */
    void setPartColor(core::PartId partId, const QColor& color);

    /**
     * @brief Get selected part IDs
     * @return Vector of selected part IDs
     */
    std::vector<core::PartId> getSelectedParts() const;

    /**
     * @brief Select part
     * @param partId Part ID to select
     * @param clearOthers True to clear other selections
     */
    void selectPart(core::PartId partId, bool clearOthers = true);

signals:
    /**
     * @brief Emitted when part is selected
     * @param partId Part ID
     */
    void partSelected(core::PartId partId);

    /**
     * @brief Emitted when multiple parts are selected
     * @param partIds Vector of part IDs
     */
    void partsSelected(const std::vector<core::PartId>& partIds);

    /**
     * @brief Emitted when part visibility changes
     * @param partId Part ID
     * @param visible New visibility state
     */
    void partVisibilityChanged(core::PartId partId, bool visible);

    /**
     * @brief Emitted when part color changes
     * @param partId Part ID
     * @param color New color
     */
    void partColorChanged(core::PartId partId, const QColor& color);

    /**
     * @brief Emitted when user requests to focus camera on part
     * @param partId Part ID to focus on
     */
    void focusOnPartRequested(core::PartId partId);

    /**
     * @brief Emitted when user requests to hide all other parts
     * @param partId Part ID to keep visible
     */
    void isolatePartRequested(core::PartId partId);

private slots:
    /**
     * @brief Handle item clicked
     * @param index Model index of clicked item
     */
    void onItemClicked(const QModelIndex& index);

    /**
     * @brief Handle item double-clicked
     * @param index Model index of double-clicked item
     */
    void onItemDoubleClicked(const QModelIndex& index);

    /**
     * @brief Handle context menu request
     * @param pos Position in widget coordinates
     */
    void onContextMenuRequested(const QPoint& pos);

    /**
     * @brief Handle item changed (e.g., checkbox toggled)
     * @param item Item that changed
     */
    void onItemChanged(QStandardItem* item);

    /**
     * @brief Handle search text changed
     * @param text Search text
     */
    void onSearchTextChanged(const QString& text);

    /**
     * @brief Handle show all button clicked
     */
    void onShowAllClicked();

    /**
     * @brief Handle hide all button clicked
     */
    void onHideAllClicked();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Build part tree from mesh
     */
    void buildTree();

    /**
     * @brief Create item for part
     * @param part Part to create item for
     * @return Created item
     */
    QStandardItem* createPartItem(const core::Part& part);

    /**
     * @brief Get part ID from model index
     * @param index Model index
     * @return Part ID or -1 if not found
     */
    core::PartId getPartIdFromIndex(const QModelIndex& index) const;

    /**
     * @brief Find item by part ID
     * @param partId Part ID to find
     * @return Pointer to item or nullptr if not found
     */
    QStandardItem* findItemByPartId(core::PartId partId) const;

    /**
     * @brief Update part item with current data
     * @param item Item to update
     * @param part Part data
     */
    void updatePartItem(QStandardItem* item, const core::Part& part);

    // UI components
    QVBoxLayout* m_layout;
    QLineEdit* m_searchBox;
    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    QPushButton* m_showAllButton;
    QPushButton* m_hideAllButton;

    // Data
    const core::Mesh* m_mesh;  // Not owned

    // State
    bool m_ignoreChanges;  // To prevent recursive updates
};

} // namespace ui
} // namespace koomesh

#else // !KOOMESH_HAS_QT

// Stub when Qt not available
namespace koomesh {
namespace ui {

class PartsPanel {
public:
    PartsPanel(void* = nullptr) {}
    void setMesh(const void*) {}
    void refresh() {}
};

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
