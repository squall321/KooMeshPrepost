/**
 * @file PropertiesPanel.h
 * @brief Properties inspector panel
 *
 * Displays detailed properties of selected objects:
 * - Element properties (ID, type, quality, volume, etc.)
 * - Node properties (ID, coordinates, etc.)
 * - Group properties (name, count, etc.)
 * - Selection statistics
 *
 * Uses Qt6 under LGPL v3 license (dynamically linked)
 */

#pragma once

#include "core/Types.h"
#include <memory>
#include <vector>
#include <string>

#ifdef KOOMESH_HAS_QT

#include <QWidget>
#include <QString>

// Forward declarations - Qt
class QTableWidget;
class QTableWidgetItem;
class QVBoxLayout;
class QLabel;

// Forward declarations - KooMesh
namespace koomesh {

namespace core {
class Element;
class Node;
class Group;
class Mesh;
}

namespace ui {

/**
 * @brief Properties panel displaying detailed object information
 *
 * Table view showing properties:
 * ```
 * ┌─────────────────────────┐
 * │ Element Properties      │
 * ├──────────────┬──────────┤
 * │ ID           │ 12345    │
 * │ Type         │ Hexahed. │
 * │ Part         │ Part 1   │
 * │ Nodes        │ 8        │
 * │ Volume       │ 1.234 mm³│
 * │ Quality      │ 0.85     │
 * │ Center       │ (1,2,3)  │
 * └──────────────┴──────────┘
 * ```
 *
 * Supports displaying properties for:
 * - Single element
 * - Single node
 * - Group
 * - Multiple selections (statistics)
 */
class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit PropertiesPanel(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~PropertiesPanel() override;

    /**
     * @brief Display element properties
     * @param elementId Element ID
     * @param element Pointer to element
     * @param mesh Pointer to mesh (for volume calculation, etc.)
     */
    void displayElementProperties(core::ElementId elementId,
                                  const core::Element* element,
                                  const core::Mesh* mesh);

    /**
     * @brief Display node properties
     * @param nodeId Node ID
     * @param node Pointer to node
     */
    void displayNodeProperties(core::NodeId nodeId, const core::Node* node);

    /**
     * @brief Display group properties
     * @param group Pointer to group
     */
    void displayGroupProperties(const core::Group* group);

    /**
     * @brief Display selection statistics
     * @param elementIds Vector of selected element IDs
     * @param mesh Pointer to mesh
     */
    void displaySelectionStatistics(const std::vector<core::ElementId>& elementIds,
                                    const core::Mesh* mesh);

    /**
     * @brief Display part statistics
     * @param partId Part ID
     * @param mesh Pointer to mesh
     */
    void displayPartStatistics(core::PartId partId, const core::Mesh* mesh);

    /**
     * @brief Clear all properties
     */
    void clear();

signals:
    /**
     * @brief Emitted when user wants to navigate to element
     * @param elementId Element ID to navigate to
     */
    void navigateToElementRequested(core::ElementId elementId);

    /**
     * @brief Emitted when user wants to navigate to node
     * @param nodeId Node ID to navigate to
     */
    void navigateToNodeRequested(core::NodeId nodeId);

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Clear table
     */
    void clearTable();

    /**
     * @brief Add section header
     * @param title Section title
     */
    void addSection(const QString& title);

    /**
     * @brief Add property row
     * @param name Property name
     * @param value Property value
     */
    void addProperty(const QString& name, const QString& value);

    /**
     * @brief Add property row with custom widget
     * @param name Property name
     * @param widget Custom widget
     */
    void addPropertyWidget(const QString& name, QWidget* widget);

    /**
     * @brief Format double value
     * @param value Value to format
     * @param precision Decimal precision
     * @return Formatted string
     */
    QString formatDouble(double value, int precision = 3) const;

    /**
     * @brief Format vector value
     * @param x X component
     * @param y Y component
     * @param z Z component
     * @param precision Decimal precision
     * @return Formatted string like "(x, y, z)"
     */
    QString formatVector(double x, double y, double z, int precision = 3) const;

    /**
     * @brief Get element type name
     * @param element Element pointer
     * @return Human-readable type name
     */
    QString getElementTypeName(const core::Element* element) const;

    /**
     * @brief Set title text
     * @param title Title to display
     */
    void setTitle(const QString& title);

    // UI components
    QVBoxLayout* m_layout;
    QLabel* m_titleLabel;
    QTableWidget* m_table;

    // Styling
    static constexpr int COLUMN_NAME = 0;
    static constexpr int COLUMN_VALUE = 1;
};

} // namespace ui
} // namespace koomesh

#else // !KOOMESH_HAS_QT

// Stub when Qt not available
namespace koomesh {
namespace ui {

class PropertiesPanel {
public:
    PropertiesPanel(void* = nullptr) {}
    void clear() {}
    void displayElementProperties(int, const void*, const void*) {}
};

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
