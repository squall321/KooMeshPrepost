/**
 * @file PropertiesPanel.cpp
 * @brief Properties panel stub - Phase 74 (To be fully implemented)
 */

#include "ui/PropertiesPanel.h"

#ifdef KOOMESH_HAS_QT

#include "core/Element.h"
#include "core/Node.h"
#include "core/Group.h"
#include "core/Mesh.h"

#include <QVBoxLayout>
#include <QTableWidget>
#include <QLabel>
#include <QHeaderView>

namespace koomesh {
namespace ui {

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

PropertiesPanel::~PropertiesPanel() = default;

void PropertiesPanel::setupUI() {
    m_layout = new QVBoxLayout(this);

    m_titleLabel = new QLabel("Properties", this);
    m_layout->addWidget(m_titleLabel);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({"Property", "Value"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_layout->addWidget(m_table);
}

void PropertiesPanel::displayElementProperties(core::ElementId elementId,
                                               const core::Element* element,
                                               const core::Mesh* mesh) {
    // TODO: Phase 74
    clearTable();
    setTitle("Element Properties");
}

void PropertiesPanel::displayNodeProperties(core::NodeId nodeId, const core::Node* node) {
    // TODO: Phase 74
    clearTable();
    setTitle("Node Properties");
}

void PropertiesPanel::displayGroupProperties(const core::Group* group) {
    // TODO: Phase 74
    clearTable();
    setTitle("Group Properties");
}

void PropertiesPanel::displaySelectionStatistics(const std::vector<core::ElementId>& elementIds,
                                                 const core::Mesh* mesh) {
    // TODO: Phase 74
    clearTable();
    setTitle("Selection Statistics");
}

void PropertiesPanel::displayPartStatistics(core::PartId partId, const core::Mesh* mesh) {
    // TODO: Phase 74
    clearTable();
    setTitle("Part Statistics");
}

void PropertiesPanel::clear() {
    clearTable();
    setTitle("Properties");
}

void PropertiesPanel::clearTable() {
    m_table->setRowCount(0);
}

void PropertiesPanel::addSection(const QString& title) {
    // TODO: Phase 74
}

void PropertiesPanel::addProperty(const QString& name, const QString& value) {
    // TODO: Phase 74
}

void PropertiesPanel::addPropertyWidget(const QString& name, QWidget* widget) {
    // TODO: Phase 74
}

QString PropertiesPanel::formatDouble(double value, int precision) const {
    return QString::number(value, 'f', precision);
}

QString PropertiesPanel::formatVector(double x, double y, double z, int precision) const {
    return QString("(%1, %2, %3)")
        .arg(x, 0, 'f', precision)
        .arg(y, 0, 'f', precision)
        .arg(z, 0, 'f', precision);
}

QString PropertiesPanel::getElementTypeName(const core::Element* element) const {
    // TODO: Phase 74
    return "Unknown";
}

void PropertiesPanel::setTitle(const QString& title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
