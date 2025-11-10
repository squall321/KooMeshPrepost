/**
 * @file GroupsPanel.h
 * @brief Groups management panel with list view
 *
 * Displays and manages element groups with:
 * - Group creation and deletion
 * - Group renaming
 * - Color assignment per group
 * - Group visibility control
 *
 * Uses Qt6 under LGPL v3 license (dynamically linked)
 */

#pragma once

#include <memory>
#include <vector>
#include <string>

#ifdef KOOMESH_HAS_QT

#include <QWidget>
#include <QColor>
#include <QString>

// Forward declarations - Qt
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QVBoxLayout;
class QHBoxLayout;
class QPoint;
class QLabel;

// Forward declarations - KooMesh
namespace koomesh {

namespace core {
class GroupManager;
class Group;
}

namespace ui {

/**
 * @brief Groups panel for managing element groups
 *
 * List widget displaying all groups with operations:
 * ```
 * ┌─────────────────┐
 * │ Groups          │
 * ├─────────────────┤
 * │ ● Group 1 (Red) │
 * │ ● Group 2 (Blu) │
 * │ ● Group 3 (Grn) │
 * ├─────────────────┤
 * │ [New] [Del] [...│
 * └─────────────────┘
 * ```
 *
 * Features:
 * - Create groups from current selection
 * - Delete groups
 * - Rename groups
 * - Assign colors (double-click)
 * - Show/hide groups
 * - Export groups to file
 */
class GroupsPanel : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit GroupsPanel(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~GroupsPanel() override;

    /**
     * @brief Set group manager
     * @param manager Pointer to group manager (not owned)
     */
    void setGroupManager(core::GroupManager* manager);

    /**
     * @brief Refresh groups list
     */
    void refresh();

    /**
     * @brief Clear all groups
     */
    void clear();

    /**
     * @brief Select group
     * @param groupName Group name to select
     */
    void selectGroup(const std::string& groupName);

    /**
     * @brief Get selected group name
     * @return Selected group name or empty string
     */
    std::string getSelectedGroupName() const;

    /**
     * @brief Set group color
     * @param groupName Group name
     * @param color Color to set
     */
    void setGroupColor(const std::string& groupName, const QColor& color);

signals:
    /**
     * @brief Emitted when group is selected
     * @param groupName Selected group name
     */
    void groupSelected(const std::string& groupName);

    /**
     * @brief Emitted when group selection is cleared
     */
    void groupSelectionCleared();

    /**
     * @brief Emitted when user requests to create new group
     * @param groupName Desired group name
     */
    void createGroupRequested(const std::string& groupName);

    /**
     * @brief Emitted when user requests to delete group
     * @param groupName Group name to delete
     */
    void deleteGroupRequested(const std::string& groupName);

    /**
     * @brief Emitted when user requests to rename group
     * @param oldName Current group name
     * @param newName New group name
     */
    void renameGroupRequested(const std::string& oldName, const std::string& newName);

    /**
     * @brief Emitted when group color changes
     * @param groupName Group name
     * @param color New color
     */
    void groupColorChanged(const std::string& groupName, const QColor& color);

    /**
     * @brief Emitted when user requests to export group
     * @param groupName Group name to export
     */
    void exportGroupRequested(const std::string& groupName);

    /**
     * @brief Emitted when user requests to focus camera on group
     * @param groupName Group name
     */
    void focusOnGroupRequested(const std::string& groupName);

    /**
     * @brief Emitted when user requests to isolate group (hide others)
     * @param groupName Group name
     */
    void isolateGroupRequested(const std::string& groupName);

private slots:
    /**
     * @brief Handle create group button clicked
     */
    void onCreateGroupClicked();

    /**
     * @brief Handle delete group button clicked
     */
    void onDeleteGroupClicked();

    /**
     * @brief Handle rename group button clicked
     */
    void onRenameGroupClicked();

    /**
     * @brief Handle group item clicked
     * @param item Clicked item
     */
    void onGroupItemClicked(QListWidgetItem* item);

    /**
     * @brief Handle group item double-clicked (for color picker)
     * @param item Double-clicked item
     */
    void onGroupItemDoubleClicked(QListWidgetItem* item);

    /**
     * @brief Handle context menu request
     * @param pos Position in widget coordinates
     */
    void onContextMenuRequested(const QPoint& pos);

    /**
     * @brief Handle color picker clicked
     */
    void onChangeColorClicked();

    /**
     * @brief Handle export group clicked
     */
    void onExportGroupClicked();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Create group item
     * @param group Group to create item for
     * @return Created item
     */
    QListWidgetItem* createGroupItem(const core::Group& group);

    /**
     * @brief Update group item with current data
     * @param item Item to update
     * @param group Group data
     */
    void updateGroupItem(QListWidgetItem* item, const core::Group& group);

    /**
     * @brief Get group name from list item
     * @param item List item
     * @return Group name or empty string
     */
    std::string getGroupNameFromItem(QListWidgetItem* item) const;

    /**
     * @brief Find item by group name
     * @param groupName Group name
     * @return Pointer to item or nullptr if not found
     */
    QListWidgetItem* findItemByGroupName(const std::string& groupName) const;

    /**
     * @brief Show color picker for group
     * @param groupName Group name
     */
    void showColorPicker(const std::string& groupName);

    /**
     * @brief Prompt for group name
     * @param title Dialog title
     * @param label Dialog label
     * @param defaultValue Default value
     * @return Entered name or empty string if cancelled
     */
    std::string promptForGroupName(const QString& title,
                                    const QString& label,
                                    const QString& defaultValue = "");

    // UI components
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QListWidget* m_groupsList;

    QHBoxLayout* m_buttonLayout;
    QPushButton* m_createButton;
    QPushButton* m_deleteButton;
    QPushButton* m_renameButton;
    QPushButton* m_colorButton;

    // Data
    core::GroupManager* m_groupManager;  // Not owned
};

} // namespace ui
} // namespace koomesh

#else // !KOOMESH_HAS_QT

// Stub when Qt not available
namespace koomesh {
namespace ui {

class GroupsPanel {
public:
    GroupsPanel(void* = nullptr) {}
    void setGroupManager(void*) {}
    void refresh() {}
};

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
