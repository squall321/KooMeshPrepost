/**
 * @file PartsPanel.cpp
 * @brief Implementation of parts panel - Phase 72 (Complete)
 *
 * Complete implementation of parts hierarchy panel with:
 * - Tree view of all parts in mesh
 * - Visibility toggles per part
 * - Part selection and focus
 * - Search/filter functionality
 * - Context menu operations
 * - Color customization
 */

#include "ui/PartsPanel.h"

#ifdef KOOMESH_HAS_QT

#include "core/Mesh.h"
#include "core/Part.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QColorDialog>
#include <QHeaderView>
#include <QMessageBox>

namespace koomesh {
namespace ui {

// Custom role for storing part ID in items
static const int PartIdRole = Qt::UserRole + 1;
static const int PartColorRole = Qt::UserRole + 2;

// ============================================================================
// Constructor / Destructor
// ============================================================================

PartsPanel::PartsPanel(QWidget* parent)
    : QWidget(parent)
    , m_mesh(nullptr)
    , m_ignoreChanges(false)
{
    setupUI();
}

PartsPanel::~PartsPanel() = default;

// ============================================================================
// Public API
// ============================================================================

void PartsPanel::setMesh(const core::Mesh* mesh) {
    m_mesh = mesh;
    refresh();
}

void PartsPanel::refresh() {
    clear();
    if (m_mesh) {
        buildTree();
    }
}

void PartsPanel::clear() {
    if (m_model) {
        m_ignoreChanges = true;
        m_model->clear();
        m_model->setHorizontalHeaderLabels({"Parts"});
        m_ignoreChanges = false;
    }
}

void PartsPanel::setPartVisibility(core::PartId partId, bool visible) {
    QStandardItem* item = findItemByPartId(partId);
    if (item) {
        m_ignoreChanges = true;
        item->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
        m_ignoreChanges = false;
    }
}

void PartsPanel::setPartColor(core::PartId partId, const QColor& color) {
    QStandardItem* item = findItemByPartId(partId);
    if (item) {
        // Store color in item data
        item->setData(color, PartColorRole);

        // Update icon to show color
        QPixmap pixmap(16, 16);
        pixmap.fill(color);
        item->setIcon(QIcon(pixmap));
    }
}

std::vector<core::PartId> PartsPanel::getSelectedParts() const {
    std::vector<core::PartId> partIds;

    QModelIndexList selection = m_treeView->selectionModel()->selectedIndexes();
    for (const QModelIndex& index : selection) {
        if (index.column() == 0) {  // Only process first column
            core::PartId partId = getPartIdFromIndex(index);
            if (partId >= 0) {
                partIds.push_back(partId);
            }
        }
    }

    return partIds;
}

void PartsPanel::selectPart(core::PartId partId, bool clearOthers) {
    QStandardItem* item = findItemByPartId(partId);
    if (item) {
        QModelIndex index = item->index();

        if (clearOthers) {
            m_treeView->selectionModel()->clearSelection();
        }

        m_treeView->selectionModel()->select(
            index,
            QItemSelectionModel::Select | QItemSelectionModel::Rows
        );

        m_treeView->scrollTo(index);
    }
}

// ============================================================================
// Private Setup
// ============================================================================

void PartsPanel::setupUI() {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(5);

    // Title
    QLabel* titleLabel = new QLabel("<b>Parts</b>", this);
    m_layout->addWidget(titleLabel);

    // Search box
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Search parts...");
    m_searchBox->setClearButtonEnabled(true);
    m_layout->addWidget(m_searchBox);

    // Tree view
    m_treeView = new QTreeView(this);
    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(false);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Set column headers
    m_model->setHorizontalHeaderLabels({"Parts"});

    m_layout->addWidget(m_treeView, 1);  // Stretch factor 1

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(5);

    m_showAllButton = new QPushButton("Show All", this);
    m_showAllButton->setToolTip("Show all parts");

    m_hideAllButton = new QPushButton("Hide All", this);
    m_hideAllButton->setToolTip("Hide all parts");

    buttonLayout->addWidget(m_showAllButton);
    buttonLayout->addWidget(m_hideAllButton);
    buttonLayout->addStretch();

    m_layout->addLayout(buttonLayout);

    // Setup connections
    connect(m_treeView, &QTreeView::clicked,
            this, &PartsPanel::onItemClicked);
    connect(m_treeView, &QTreeView::doubleClicked,
            this, &PartsPanel::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &PartsPanel::onContextMenuRequested);

    connect(m_model, &QStandardItemModel::itemChanged,
            this, &PartsPanel::onItemChanged);

    connect(m_searchBox, &QLineEdit::textChanged,
            this, &PartsPanel::onSearchTextChanged);

    connect(m_showAllButton, &QPushButton::clicked,
            this, &PartsPanel::onShowAllClicked);
    connect(m_hideAllButton, &QPushButton::clicked,
            this, &PartsPanel::onHideAllClicked);
}

// ============================================================================
// Private Tree Building
// ============================================================================

void PartsPanel::buildTree() {
    if (!m_mesh) {
        return;
    }

    m_ignoreChanges = true;

    // Get all parts from mesh
    const auto& parts = m_mesh->getParts();

    if (parts.empty()) {
        // No parts defined - create single default item showing all elements
        QStandardItem* defaultItem = new QStandardItem("All Elements");
        defaultItem->setCheckable(true);
        defaultItem->setCheckState(Qt::Checked);
        defaultItem->setData(-1, PartIdRole);  // -1 for "all"

        // Add element count
        QString text = QString("All Elements (%1)")
            .arg(m_mesh->elementCount());
        defaultItem->setText(text);

        m_model->appendRow(defaultItem);
    } else {
        // Build tree from parts
        for (const auto& partPair : parts) {
            const core::Part& part = partPair.second;
            QStandardItem* item = createPartItem(part);
            if (item) {
                m_model->appendRow(item);
            }
        }
    }

    // Expand all items by default
    m_treeView->expandAll();

    // Resize columns to content
    m_treeView->resizeColumnToContents(0);

    m_ignoreChanges = false;
}

QStandardItem* PartsPanel::createPartItem(const core::Part& part) {
    // Create main item
    QStandardItem* item = new QStandardItem();
    item->setCheckable(true);
    item->setCheckState(Qt::Checked);  // Visible by default
    item->setData(part.getId(), PartIdRole);

    // Set default color (light gray)
    QColor defaultColor(200, 200, 200);
    item->setData(defaultColor, PartColorRole);

    // Create color icon
    QPixmap pixmap(16, 16);
    pixmap.fill(defaultColor);
    item->setIcon(QIcon(pixmap));

    // Update item with part data
    updatePartItem(item, part);

    // Add child items for details
    addPartDetails(item, part);

    return item;
}

void PartsPanel::updatePartItem(QStandardItem* item, const core::Part& part) {
    if (!item) {
        return;
    }

    // Build display text
    QString text = QString("Part %1").arg(part.getId());

    // Add material name if available
    const std::string& matName = part.getMaterialName();
    if (!matName.empty()) {
        text += QString(" (%1)").arg(QString::fromStdString(matName));
    }

    // Add element count
    size_t elemCount = part.getElementIds().size();
    text += QString(" [%1 elem]").arg(elemCount);

    item->setText(text);

    // Set tooltip with more info
    QString tooltip = QString(
        "Part ID: %1\n"
        "Material: %2\n"
        "Elements: %3"
    ).arg(part.getId())
     .arg(matName.empty() ? "None" : QString::fromStdString(matName))
     .arg(elemCount);

    item->setToolTip(tooltip);
}

void PartsPanel::addPartDetails(QStandardItem* parentItem, const core::Part& part) {
    // Element count detail
    QStandardItem* elemItem = new QStandardItem();
    size_t elemCount = part.getElementIds().size();
    elemItem->setText(QString("Elements: %L1").arg(elemCount));
    elemItem->setEditable(false);
    parentItem->appendRow(elemItem);

    // Material detail
    const std::string& matName = part.getMaterialName();
    if (!matName.empty()) {
        QStandardItem* matItem = new QStandardItem();
        matItem->setText(QString("Material: %1")
            .arg(QString::fromStdString(matName)));
        matItem->setEditable(false);
        parentItem->appendRow(matItem);
    }
}

// ============================================================================
// Private Slots
// ============================================================================

void PartsPanel::onItemClicked(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    core::PartId partId = getPartIdFromIndex(index);
    if (partId >= 0) {
        emit partSelected(partId);

        // Emit multi-selection if multiple parts selected
        auto selectedParts = getSelectedParts();
        if (selectedParts.size() > 1) {
            emit partsSelected(selectedParts);
        }
    }
}

void PartsPanel::onItemDoubleClicked(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }

    core::PartId partId = getPartIdFromIndex(index);
    if (partId >= 0) {
        // Request to focus camera on this part
        emit focusOnPartRequested(partId);
    }
}

void PartsPanel::onContextMenuRequested(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    core::PartId partId = getPartIdFromIndex(index);
    if (partId < 0) {
        return;  // Not a part item
    }

    // Create context menu
    QMenu menu(this);

    QAction* focusAction = menu.addAction("Focus on Part");
    QAction* isolateAction = menu.addAction("Isolate Part");
    menu.addSeparator();
    QAction* colorAction = menu.addAction("Change Color...");
    menu.addSeparator();
    QAction* showAction = menu.addAction("Show");
    QAction* hideAction = menu.addAction("Hide");

    // Execute menu
    QAction* selected = menu.exec(m_treeView->mapToGlobal(pos));

    if (selected == focusAction) {
        emit focusOnPartRequested(partId);
    } else if (selected == isolateAction) {
        emit isolatePartRequested(partId);
    } else if (selected == colorAction) {
        showColorPicker(partId);
    } else if (selected == showAction) {
        setPartVisibility(partId, true);
        emit partVisibilityChanged(partId, true);
    } else if (selected == hideAction) {
        setPartVisibility(partId, false);
        emit partVisibilityChanged(partId, false);
    }
}

void PartsPanel::onItemChanged(QStandardItem* item) {
    if (!item || m_ignoreChanges) {
        return;
    }

    // Check if this is a checkable item (part visibility)
    if (item->isCheckable()) {
        core::PartId partId = item->data(PartIdRole).toInt();
        if (partId >= 0) {
            bool visible = (item->checkState() == Qt::Checked);
            emit partVisibilityChanged(partId, visible);
        }
    }
}

void PartsPanel::onSearchTextChanged(const QString& text) {
    if (!m_model) {
        return;
    }

    // Simple search: hide items that don't match
    QString searchLower = text.toLower();

    for (int row = 0; row < m_model->rowCount(); ++row) {
        QStandardItem* item = m_model->item(row, 0);
        if (!item) {
            continue;
        }

        bool matches = false;
        if (searchLower.isEmpty()) {
            matches = true;  // Show all if search is empty
        } else {
            QString itemText = item->text().toLower();
            matches = itemText.contains(searchLower);

            // Also check tooltip
            if (!matches) {
                QString tooltip = item->toolTip().toLower();
                matches = tooltip.contains(searchLower);
            }
        }

        // Show/hide row
        m_treeView->setRowHidden(row, QModelIndex(), !matches);
    }
}

void PartsPanel::onShowAllClicked() {
    if (!m_model) {
        return;
    }

    m_ignoreChanges = true;

    // Check all items
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QStandardItem* item = m_model->item(row, 0);
        if (item && item->isCheckable()) {
            item->setCheckState(Qt::Checked);

            core::PartId partId = item->data(PartIdRole).toInt();
            if (partId >= 0) {
                emit partVisibilityChanged(partId, true);
            }
        }
    }

    m_ignoreChanges = false;
}

void PartsPanel::onHideAllClicked() {
    if (!m_model) {
        return;
    }

    m_ignoreChanges = true;

    // Uncheck all items
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QStandardItem* item = m_model->item(row, 0);
        if (item && item->isCheckable()) {
            item->setCheckState(Qt::Unchecked);

            core::PartId partId = item->data(PartIdRole).toInt();
            if (partId >= 0) {
                emit partVisibilityChanged(partId, false);
            }
        }
    }

    m_ignoreChanges = false;
}

// ============================================================================
// Private Helpers
// ============================================================================

core::PartId PartsPanel::getPartIdFromIndex(const QModelIndex& index) const {
    if (!index.isValid()) {
        return -1;
    }

    QStandardItem* item = m_model->itemFromIndex(index);
    if (!item) {
        return -1;
    }

    QVariant data = item->data(PartIdRole);
    if (!data.isValid()) {
        return -1;
    }

    return data.toInt();
}

QStandardItem* PartsPanel::findItemByPartId(core::PartId partId) const {
    if (!m_model) {
        return nullptr;
    }

    // Search all top-level items
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QStandardItem* item = m_model->item(row, 0);
        if (item) {
            QVariant data = item->data(PartIdRole);
            if (data.isValid() && data.toInt() == partId) {
                return item;
            }
        }
    }

    return nullptr;
}

void PartsPanel::showColorPicker(core::PartId partId) {
    QStandardItem* item = findItemByPartId(partId);
    if (!item) {
        return;
    }

    // Get current color
    QColor currentColor = item->data(PartColorRole).value<QColor>();
    if (!currentColor.isValid()) {
        currentColor = QColor(200, 200, 200);  // Default gray
    }

    // Show color picker dialog
    QColor newColor = QColorDialog::getColor(
        currentColor,
        this,
        QString("Choose Color for Part %1").arg(partId)
    );

    if (newColor.isValid()) {
        setPartColor(partId, newColor);
        emit partColorChanged(partId, newColor);
    }
}

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
