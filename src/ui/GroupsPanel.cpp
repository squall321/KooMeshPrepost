/**
 * @file GroupsPanel.cpp
 * @brief Complete implementation of groups panel - Phase 73
 *
 * Full implementation of groups management panel with:
 * - Group list display with colors
 * - CRUD operations (Create, Rename, Delete)
 * - Color customization
 * - Context menu
 * - Export functionality
 */

#include "ui/GroupsPanel.h"

#ifdef KOOMESH_HAS_QT

#include "core/GroupManager.h"
#include "core/Group.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>

namespace koomesh {
namespace ui {

// Custom role for storing group data
static const int GroupNameRole = Qt::UserRole + 1;
static const int GroupColorRole = Qt::UserRole + 2;

// ============================================================================
// Constructor / Destructor
// ============================================================================

GroupsPanel::GroupsPanel(QWidget* parent)
    : QWidget(parent)
    , m_groupManager(nullptr)
{
    setupUI();
}

GroupsPanel::~GroupsPanel() = default;

// ============================================================================
// Public API
// ============================================================================

void GroupsPanel::setGroupManager(core::GroupManager* manager) {
    m_groupManager = manager;
    refresh();
}

void GroupsPanel::refresh() {
    clear();

    if (!m_groupManager) {
        return;
    }

    // Get all groups
    const auto& groups = m_groupManager->getAllGroups();

    for (const auto& groupPair : groups) {
        const core::Group& group = groupPair.second;
        QListWidgetItem* item = createGroupItem(group);
        if (item) {
            m_groupsList->addItem(item);
        }
    }

    // Update button states
    updateButtonStates();
}

void GroupsPanel::clear() {
    if (m_groupsList) {
        m_groupsList->clear();
    }
    updateButtonStates();
}

void GroupsPanel::selectGroup(const std::string& groupName) {
    QListWidgetItem* item = findItemByGroupName(groupName);
    if (item) {
        m_groupsList->setCurrentItem(item);
        m_groupsList->scrollToItem(item);
    }
}

std::string GroupsPanel::getSelectedGroupName() const {
    QListWidgetItem* item = m_groupsList->currentItem();
    return item ? getGroupNameFromItem(item) : "";
}

void GroupsPanel::setGroupColor(const std::string& groupName, const QColor& color) {
    QListWidgetItem* item = findItemByGroupName(groupName);
    if (item) {
        // Store color
        item->setData(GroupColorRole, color);

        // Update icon
        QPixmap pixmap(16, 16);
        pixmap.fill(color);
        item->setIcon(QIcon(pixmap));

        // Update text color for visibility
        item->setForeground(QBrush(color));
    }
}

// ============================================================================
// Private Setup
// ============================================================================

void GroupsPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);

    // Title
    m_titleLabel = new QLabel("<b>Groups</b>", this);
    m_mainLayout->addWidget(m_titleLabel);

    // List widget
    m_groupsList = new QListWidget(this);
    m_groupsList->setAlternatingRowColors(true);
    m_groupsList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_groupsList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_mainLayout->addWidget(m_groupsList, 1);  // Stretch factor 1

    // Buttons
    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(3);

    m_createButton = new QPushButton("New", this);
    m_createButton->setToolTip("Create new group from selection");

    m_deleteButton = new QPushButton("Delete", this);
    m_deleteButton->setToolTip("Delete selected group");
    m_deleteButton->setEnabled(false);

    m_renameButton = new QPushButton("Rename", this);
    m_renameButton->setToolTip("Rename selected group");
    m_renameButton->setEnabled(false);

    m_colorButton = new QPushButton("Color", this);
    m_colorButton->setToolTip("Change group color");
    m_colorButton->setEnabled(false);

    m_buttonLayout->addWidget(m_createButton);
    m_buttonLayout->addWidget(m_deleteButton);
    m_buttonLayout->addWidget(m_renameButton);
    m_buttonLayout->addWidget(m_colorButton);

    m_mainLayout->addLayout(m_buttonLayout);

    // Setup connections
    connect(m_groupsList, &QListWidget::itemClicked,
            this, &GroupsPanel::onGroupItemClicked);
    connect(m_groupsList, &QListWidget::itemDoubleClicked,
            this, &GroupsPanel::onGroupItemDoubleClicked);
    connect(m_groupsList, &QListWidget::customContextMenuRequested,
            this, &GroupsPanel::onContextMenuRequested);

    connect(m_createButton, &QPushButton::clicked,
            this, &GroupsPanel::onCreateGroupClicked);
    connect(m_deleteButton, &QPushButton::clicked,
            this, &GroupsPanel::onDeleteGroupClicked);
    connect(m_renameButton, &QPushButton::clicked,
            this, &GroupsPanel::onRenameGroupClicked);
    connect(m_colorButton, &QPushButton::clicked,
            this, &GroupsPanel::onChangeColorClicked);

    // Selection changed
    connect(m_groupsList, &QListWidget::itemSelectionChanged,
            this, [this]() {
                updateButtonStates();
                std::string groupName = getSelectedGroupName();
                if (!groupName.empty()) {
                    emit groupSelected(groupName);
                } else {
                    emit groupSelectionCleared();
                }
            });
}

void GroupsPanel::updateButtonStates() {
    bool hasSelection = (m_groupsList->currentItem() != nullptr);

    m_deleteButton->setEnabled(hasSelection);
    m_renameButton->setEnabled(hasSelection);
    m_colorButton->setEnabled(hasSelection);
}

// ============================================================================
// Private Slots
// ============================================================================

void GroupsPanel::onCreateGroupClicked() {
    std::string groupName = promptForGroupName(
        "Create Group",
        "Enter group name:",
        "Group"
    );

    if (!groupName.empty()) {
        emit createGroupRequested(groupName);
    }
}

void GroupsPanel::onDeleteGroupClicked() {
    std::string groupName = getSelectedGroupName();
    if (groupName.empty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Delete Group",
        QString("Are you sure you want to delete group '%1'?")
            .arg(QString::fromStdString(groupName)),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        emit deleteGroupRequested(groupName);
    }
}

void GroupsPanel::onRenameGroupClicked() {
    std::string oldName = getSelectedGroupName();
    if (oldName.empty()) {
        return;
    }

    std::string newName = promptForGroupName(
        "Rename Group",
        "Enter new name:",
        QString::fromStdString(oldName)
    );

    if (!newName.empty() && newName != oldName) {
        emit renameGroupRequested(oldName, newName);
    }
}

void GroupsPanel::onGroupItemClicked(QListWidgetItem* item) {
    if (!item) {
        return;
    }

    std::string groupName = getGroupNameFromItem(item);
    if (!groupName.empty()) {
        emit groupSelected(groupName);
    }
}

void GroupsPanel::onGroupItemDoubleClicked(QListWidgetItem* item) {
    if (!item) {
        return;
    }

    // Double-click opens color picker
    std::string groupName = getGroupNameFromItem(item);
    if (!groupName.empty()) {
        showColorPicker(groupName);
    }
}

void GroupsPanel::onContextMenuRequested(const QPoint& pos) {
    QListWidgetItem* item = m_groupsList->itemAt(pos);
    if (!item) {
        return;
    }

    std::string groupName = getGroupNameFromItem(item);
    if (groupName.empty()) {
        return;
    }

    // Create context menu
    QMenu menu(this);

    QAction* renameAction = menu.addAction("Rename...");
    QAction* colorAction = menu.addAction("Change Color...");
    menu.addSeparator();
    QAction* focusAction = menu.addAction("Focus on Group");
    QAction* isolateAction = menu.addAction("Isolate Group");
    menu.addSeparator();
    QAction* exportAction = menu.addAction("Export...");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("Delete");

    // Execute menu
    QAction* selected = menu.exec(m_groupsList->mapToGlobal(pos));

    if (selected == renameAction) {
        onRenameGroupClicked();
    } else if (selected == colorAction) {
        showColorPicker(groupName);
    } else if (selected == focusAction) {
        emit focusOnGroupRequested(groupName);
    } else if (selected == isolateAction) {
        emit isolateGroupRequested(groupName);
    } else if (selected == exportAction) {
        onExportGroupClicked();
    } else if (selected == deleteAction) {
        onDeleteGroupClicked();
    }
}

void GroupsPanel::onChangeColorClicked() {
    std::string groupName = getSelectedGroupName();
    if (!groupName.empty()) {
        showColorPicker(groupName);
    }
}

void GroupsPanel::onExportGroupClicked() {
    std::string groupName = getSelectedGroupName();
    if (groupName.empty()) {
        return;
    }

    QString filename = QFileDialog::getSaveFileName(
        this,
        "Export Group",
        QString::fromStdString(groupName) + ".k",
        "LS-DYNA Keyword Files (*.k *.key);;All Files (*)"
    );

    if (!filename.isEmpty()) {
        emit exportGroupRequested(groupName);
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

QListWidgetItem* GroupsPanel::createGroupItem(const core::Group& group) {
    QListWidgetItem* item = new QListWidgetItem();

    // Store group name
    item->setData(GroupNameRole, QString::fromStdString(group.getName()));

    // Set default color or group color
    QColor color(100, 150, 200);  // Default blue
    item->setData(GroupColorRole, color);

    // Update item display
    updateGroupItem(item, group);

    return item;
}

void GroupsPanel::updateGroupItem(QListWidgetItem* item, const core::Group& group) {
    if (!item) {
        return;
    }

    // Build display text
    size_t elementCount = group.getElementIds().size();
    QString text = QString("%1 (%L2)")
        .arg(QString::fromStdString(group.getName()))
        .arg(elementCount);

    item->setText(text);

    // Set tooltip
    QString tooltip = QString(
        "Group: %1\n"
        "Elements: %2\n"
        "Double-click to change color"
    ).arg(QString::fromStdString(group.getName()))
     .arg(elementCount);

    item->setToolTip(tooltip);

    // Get color and update icon
    QColor color = item->data(GroupColorRole).value<QColor>();
    if (color.isValid()) {
        QPixmap pixmap(16, 16);
        pixmap.fill(color);
        item->setIcon(QIcon(pixmap));

        // Tint text with color
        item->setForeground(QBrush(color));
    }
}

std::string GroupsPanel::getGroupNameFromItem(QListWidgetItem* item) const {
    if (!item) {
        return "";
    }

    QVariant data = item->data(GroupNameRole);
    if (!data.isValid()) {
        return "";
    }

    return data.toString().toStdString();
}

QListWidgetItem* GroupsPanel::findItemByGroupName(const std::string& groupName) const {
    if (!m_groupsList || groupName.empty()) {
        return nullptr;
    }

    QString qGroupName = QString::fromStdString(groupName);

    for (int i = 0; i < m_groupsList->count(); ++i) {
        QListWidgetItem* item = m_groupsList->item(i);
        if (item) {
            QVariant data = item->data(GroupNameRole);
            if (data.isValid() && data.toString() == qGroupName) {
                return item;
            }
        }
    }

    return nullptr;
}

void GroupsPanel::showColorPicker(const std::string& groupName) {
    QListWidgetItem* item = findItemByGroupName(groupName);
    if (!item) {
        return;
    }

    // Get current color
    QColor currentColor = item->data(GroupColorRole).value<QColor>();
    if (!currentColor.isValid()) {
        currentColor = QColor(100, 150, 200);  // Default blue
    }

    // Show color picker dialog
    QColor newColor = QColorDialog::getColor(
        currentColor,
        this,
        QString("Choose Color for Group '%1'")
            .arg(QString::fromStdString(groupName))
    );

    if (newColor.isValid()) {
        setGroupColor(groupName, newColor);
        emit groupColorChanged(groupName, newColor);
    }
}

std::string GroupsPanel::promptForGroupName(const QString& title,
                                            const QString& label,
                                            const QString& defaultValue) {
    bool ok = false;
    QString text = QInputDialog::getText(
        this,
        title,
        label,
        QLineEdit::Normal,
        defaultValue,
        &ok
    );

    if (ok && !text.isEmpty()) {
        return text.trimmed().toStdString();
    }

    return "";
}

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
