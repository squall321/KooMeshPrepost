/**
 * @file PropertiesPanel.cpp
 * @brief Complete implementation of properties panel - Phase 74
 *
 * Full implementation of properties inspector panel with:
 * - Element properties display (ID, type, quality, volume, center)
 * - Node properties display (ID, coordinates, connectivity)
 * - Group properties display (name, counts, element/node lists)
 * - Selection statistics (type breakdown, quality stats, total volume)
 * - Part statistics (material properties, element breakdown)
 */

#include "ui/PropertiesPanel.h"

#ifdef KOOMESH_HAS_QT

#include "core/Element.h"
#include "core/Node.h"
#include "core/Group.h"
#include "core/Mesh.h"
#include "core/Part.h"

#include <QVBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QLabel>
#include <QHeaderView>
#include <QBrush>
#include <QFont>
#include <Eigen/Core>

#include <unordered_map>

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
    clearTable();
    setTitle("Element Properties");

    if (!element || !mesh) {
        return;
    }

    // Basic element info
    addSection("Element Information");
    addProperty("Element ID", QString::number(elementId));
    addProperty("Type", getElementTypeName(element));
    addProperty("Node Count", QString::number(element->nodeCount()));

    // Part info
    core::PartId partId = element->partId();
    const core::Part* part = mesh->getPart(partId);
    if (part) {
        addProperty("Part ID", QString::number(partId));
        addProperty("Part Name", QString::fromStdString(part->name()));
    }

    // Geometry properties
    addSection("Geometry");

    Eigen::Vector3d center = element->computeCenter(*mesh);
    addProperty("Center", formatVector(center.x(), center.y(), center.z()));

    double volume = element->computeVolume(*mesh);
    QString volumeLabel = "Volume";

    // Use appropriate label for 2D/1D elements
    switch (element->type()) {
        case core::ElementType::TRIANGLE:
        case core::ElementType::QUADRILATERAL:
            volumeLabel = "Area";
            break;
        case core::ElementType::BEAM:
            volumeLabel = "Length";
            break;
        default:
            break;
    }
    addProperty(volumeLabel, formatDouble(volume, 6));

    // Quality metric
    addSection("Quality");
    double quality = element->computeQuality(*mesh);
    addProperty("Quality Metric", formatDouble(quality, 4));

    // Quality rating
    QString rating;
    if (quality >= 0.8) {
        rating = "Excellent";
    } else if (quality >= 0.6) {
        rating = "Good";
    } else if (quality >= 0.4) {
        rating = "Fair";
    } else if (quality >= 0.2) {
        rating = "Poor";
    } else {
        rating = "Very Poor";
    }
    addProperty("Quality Rating", rating);

    // Node list
    addSection("Nodes");
    const auto& nodeIds = element->nodeIds();
    QString nodeList;
    for (size_t i = 0; i < nodeIds.size(); ++i) {
        if (i > 0) nodeList += ", ";
        nodeList += QString::number(nodeIds[i]);
    }
    addProperty("Node IDs", nodeList);
}

void PropertiesPanel::displayNodeProperties(core::NodeId nodeId, const core::Node* node) {
    clearTable();
    setTitle("Node Properties");

    if (!node) {
        return;
    }

    // Basic node info
    addSection("Node Information");
    addProperty("Node ID", QString::number(nodeId));

    // Coordinates
    addSection("Coordinates");
    addProperty("X", formatDouble(node->x(), 6));
    addProperty("Y", formatDouble(node->y(), 6));
    addProperty("Z", formatDouble(node->z(), 6));
    addProperty("Position", formatVector(node->x(), node->y(), node->z(), 6));

    // Connectivity
    addSection("Connectivity");
    const auto& connectedElements = node->connectedElements();
    addProperty("Connected Elements", QString::number(connectedElements.size()));

    if (!connectedElements.empty() && connectedElements.size() <= 20) {
        // Show element IDs if not too many
        QString elemList;
        for (size_t i = 0; i < connectedElements.size(); ++i) {
            if (i > 0) elemList += ", ";
            elemList += QString::number(connectedElements[i]);
        }
        addProperty("Element IDs", elemList);
    } else if (connectedElements.size() > 20) {
        // Too many to display, just show first few
        QString elemList;
        for (size_t i = 0; i < 10; ++i) {
            if (i > 0) elemList += ", ";
            elemList += QString::number(connectedElements[i]);
        }
        elemList += QString(" ... (and %1 more)").arg(connectedElements.size() - 10);
        addProperty("Element IDs", elemList);
    }
}

void PropertiesPanel::displayGroupProperties(const core::Group* group) {
    clearTable();
    setTitle("Group Properties");

    if (!group) {
        return;
    }

    // Basic group info
    addSection("Group Information");
    addProperty("Name", QString::fromStdString(group->name()));

    // Element statistics
    addSection("Elements");
    const auto& elements = group->elements();
    addProperty("Element Count", QString::number(elements.size()));

    if (!elements.empty() && elements.size() <= 30) {
        // Show element IDs if not too many
        QString elemList;
        size_t count = 0;
        for (core::ElementId elemId : elements) {
            if (count > 0) elemList += ", ";
            elemList += QString::number(elemId);
            ++count;
        }
        addProperty("Element IDs", elemList);
    } else if (elements.size() > 30) {
        // Too many to display, show summary
        QString elemList;
        size_t count = 0;
        for (core::ElementId elemId : elements) {
            if (count >= 15) break;
            if (count > 0) elemList += ", ";
            elemList += QString::number(elemId);
            ++count;
        }
        elemList += QString(" ... (and %1 more)").arg(elements.size() - 15);
        addProperty("Element IDs", elemList);
    }

    // Node statistics
    addSection("Nodes");
    const auto& nodes = group->nodes();
    addProperty("Node Count", QString::number(nodes.size()));

    if (!nodes.empty() && nodes.size() <= 30) {
        // Show node IDs if not too many
        QString nodeList;
        size_t count = 0;
        for (core::NodeId nodeId : nodes) {
            if (count > 0) nodeList += ", ";
            nodeList += QString::number(nodeId);
            ++count;
        }
        addProperty("Node IDs", nodeList);
    } else if (nodes.size() > 30) {
        // Too many to display, show summary
        QString nodeList;
        size_t count = 0;
        for (core::NodeId nodeId : nodes) {
            if (count >= 15) break;
            if (count > 0) nodeList += ", ";
            nodeList += QString::number(nodeId);
            ++count;
        }
        nodeList += QString(" ... (and %1 more)").arg(nodes.size() - 15);
        addProperty("Node IDs", nodeList);
    }

    // Visibility
    addSection("Display");
    addProperty("Visible", group->isVisible() ? "Yes" : "No");
}

void PropertiesPanel::displaySelectionStatistics(const std::vector<core::ElementId>& elementIds,
                                                 const core::Mesh* mesh) {
    clearTable();
    setTitle("Selection Statistics");

    if (!mesh || elementIds.empty()) {
        return;
    }

    // Basic selection info
    addSection("Selection");
    addProperty("Selected Elements", QString::number(elementIds.size()));

    // Element type breakdown
    addSection("Element Types");
    std::unordered_map<core::ElementType, size_t> typeCounts;
    double totalVolume = 0.0;
    double minQuality = 1.0;
    double maxQuality = 0.0;
    double sumQuality = 0.0;
    size_t qualityCount = 0;

    for (core::ElementId elemId : elementIds) {
        const core::Element* elem = mesh->getElement(elemId);
        if (!elem) continue;

        // Count by type
        core::ElementType type = elem->type();
        typeCounts[type]++;

        // Accumulate volume
        totalVolume += elem->computeVolume(*mesh);

        // Track quality statistics
        double quality = elem->computeQuality(*mesh);
        if (quality < minQuality) minQuality = quality;
        if (quality > maxQuality) maxQuality = quality;
        sumQuality += quality;
        qualityCount++;
    }

    // Display type counts
    for (const auto& pair : typeCounts) {
        QString typeName = getElementTypeNameFromEnum(pair.first);
        addProperty(typeName, QString::number(pair.second));
    }

    // Volume/Area statistics
    addSection("Geometry");
    addProperty("Total Volume/Area", formatDouble(totalVolume, 6));

    // Quality statistics
    if (qualityCount > 0) {
        addSection("Quality Statistics");
        addProperty("Minimum Quality", formatDouble(minQuality, 4));
        addProperty("Maximum Quality", formatDouble(maxQuality, 4));
        addProperty("Average Quality", formatDouble(sumQuality / qualityCount, 4));

        // Quality rating
        double avgQuality = sumQuality / qualityCount;
        QString rating;
        if (avgQuality >= 0.8) {
            rating = "Excellent";
        } else if (avgQuality >= 0.6) {
            rating = "Good";
        } else if (avgQuality >= 0.4) {
            rating = "Fair";
        } else if (avgQuality >= 0.2) {
            rating = "Poor";
        } else {
            rating = "Very Poor";
        }
        addProperty("Average Rating", rating);
    }
}

void PropertiesPanel::displayPartStatistics(core::PartId partId, const core::Mesh* mesh) {
    clearTable();
    setTitle("Part Statistics");

    if (!mesh) {
        return;
    }

    const core::Part* part = mesh->getPart(partId);
    if (!part) {
        return;
    }

    // Basic part info
    addSection("Part Information");
    addProperty("Part ID", QString::number(partId));
    addProperty("Part Name", QString::fromStdString(part->name()));
    addProperty("Element Count", QString::number(part->elementCount()));

    // Material properties
    const auto& matProps = part->materialProperties();
    if (matProps.isValid()) {
        addSection("Material Properties");

        if (!matProps.materialName.empty()) {
            addProperty("Material Name", QString::fromStdString(matProps.materialName));
        }

        addProperty("Density", formatDouble(matProps.density, 2) + " kg/m³");
        addProperty("Young's Modulus", formatDouble(matProps.youngModulus / 1e9, 2) + " GPa");
        addProperty("Poisson's Ratio", formatDouble(matProps.poissonRatio, 3));

        if (matProps.yieldStrength > 0.0) {
            addProperty("Yield Strength", formatDouble(matProps.yieldStrength / 1e6, 2) + " MPa");
        }

        if (matProps.ultimateStrength > 0.0) {
            addProperty("Ultimate Strength", formatDouble(matProps.ultimateStrength / 1e6, 2) + " MPa");
        }

        if (matProps.thermalConductivity > 0.0) {
            addProperty("Thermal Conductivity", formatDouble(matProps.thermalConductivity, 2) + " W/m·K");
        }
    }

    // Element type statistics
    addSection("Element Type Breakdown");
    const auto& elementIds = part->elements();

    std::unordered_map<core::ElementType, size_t> typeCounts;
    double totalVolume = 0.0;

    for (core::ElementId elemId : elementIds) {
        const core::Element* elem = mesh->getElement(elemId);
        if (!elem) continue;

        core::ElementType type = elem->type();
        typeCounts[type]++;
        totalVolume += elem->computeVolume(*mesh);
    }

    // Display type counts
    for (const auto& pair : typeCounts) {
        QString typeName = getElementTypeNameFromEnum(pair.first);
        addProperty(typeName, QString::number(pair.second));
    }

    // Geometry statistics
    addSection("Geometry");
    addProperty("Total Volume/Area", formatDouble(totalVolume, 6));

    // Display properties
    addSection("Display");
    addProperty("Enabled", part->isEnabled() ? "Yes" : "No");

    const auto& color = part->color();
    QString colorStr = QString("RGB(%1, %2, %3)")
        .arg(static_cast<int>(color.r * 255))
        .arg(static_cast<int>(color.g * 255))
        .arg(static_cast<int>(color.b * 255));
    addProperty("Color", colorStr);
}

void PropertiesPanel::clear() {
    clearTable();
    setTitle("Properties");
}

void PropertiesPanel::clearTable() {
    m_table->setRowCount(0);
}

void PropertiesPanel::addSection(const QString& title) {
    if (!m_table) return;

    int row = m_table->rowCount();
    m_table->insertRow(row);

    // Create section header item
    QTableWidgetItem* headerItem = new QTableWidgetItem(title);
    QFont font = headerItem->font();
    font.setBold(true);
    headerItem->setFont(font);
    headerItem->setBackground(QBrush(QColor(230, 230, 230)));

    m_table->setItem(row, 0, headerItem);

    // Span across both columns
    m_table->setSpan(row, 0, 1, 2);
}

void PropertiesPanel::addProperty(const QString& name, const QString& value) {
    if (!m_table) return;

    int row = m_table->rowCount();
    m_table->insertRow(row);

    // Property name (left column)
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);  // Read-only
    m_table->setItem(row, 0, nameItem);

    // Property value (right column)
    QTableWidgetItem* valueItem = new QTableWidgetItem(value);
    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);  // Read-only
    m_table->setItem(row, 1, valueItem);
}

void PropertiesPanel::addPropertyWidget(const QString& name, QWidget* widget) {
    if (!m_table || !widget) return;

    int row = m_table->rowCount();
    m_table->insertRow(row);

    // Property name (left column)
    QTableWidgetItem* nameItem = new QTableWidgetItem(name);
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);  // Read-only
    m_table->setItem(row, 0, nameItem);

    // Custom widget (right column)
    m_table->setCellWidget(row, 1, widget);
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
    if (!element) {
        return "Unknown";
    }
    return getElementTypeNameFromEnum(element->type());
}

QString PropertiesPanel::getElementTypeNameFromEnum(core::ElementType type) const {
    switch (type) {
        case core::ElementType::TETRAHEDRON:
            return "Tetrahedron (4-node)";
        case core::ElementType::HEXAHEDRON:
            return "Hexahedron (8-node)";
        case core::ElementType::PENTAHEDRON:
            return "Pentahedron (6-node)";
        case core::ElementType::PYRAMID:
            return "Pyramid (5-node)";
        case core::ElementType::TRIANGLE:
            return "Triangle Shell (3-node)";
        case core::ElementType::QUADRILATERAL:
            return "Quadrilateral Shell (4-node)";
        case core::ElementType::BEAM:
            return "Beam (2-node)";
        case core::ElementType::UNKNOWN:
        default:
            return "Unknown";
    }
}

void PropertiesPanel::setTitle(const QString& title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title);
    }
}

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
