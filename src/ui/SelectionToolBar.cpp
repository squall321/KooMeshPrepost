/**
 * @file SelectionToolBar.cpp
 * @brief Complete implementation of selection toolbar - Phase 75
 *
 * Full implementation of selection tools toolbar with:
 * - Five selection tool buttons (Single, Area, Lasso, ByPart, ByQuality)
 * - Exclusive tool selection with QActionGroup
 * - Selection mode combo box (Replace, Add, Subtract, Intersect)
 * - Signal emission on tool/mode changes
 * - Tooltips for all tools
 */

#include "ui/SelectionToolBar.h"

#ifdef KOOMESH_HAS_QT

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QLabel>

namespace koomesh {
namespace ui {

SelectionToolBar::SelectionToolBar(QWidget* parent)
    : QToolBar(parent)
    , m_currentTool(SelectionTool::Single)
    , m_currentMode(SelectionMode::Replace)
{
    setWindowTitle("Selection Tools");
    setupUI();
}

SelectionToolBar::~SelectionToolBar() = default;

void SelectionToolBar::setCurrentTool(SelectionTool tool) {
    if (m_currentTool == tool) {
        return;  // Already set
    }

    m_currentTool = tool;

    // Update UI to reflect tool change
    QAction* actionToCheck = nullptr;

    switch (tool) {
        case SelectionTool::Single:
            actionToCheck = m_singleSelectAction;
            break;
        case SelectionTool::Area:
            actionToCheck = m_areaSelectAction;
            break;
        case SelectionTool::Lasso:
            actionToCheck = m_lassoSelectAction;
            break;
        case SelectionTool::ByPart:
            actionToCheck = m_selectByPartAction;
            break;
        case SelectionTool::ByQuality:
            actionToCheck = m_selectByQualityAction;
            break;
    }

    if (actionToCheck && !actionToCheck->isChecked()) {
        actionToCheck->setChecked(true);
    }
}

void SelectionToolBar::setCurrentMode(SelectionMode mode) {
    m_currentMode = mode;
    if (m_modeComboBox) {
        m_modeComboBox->setCurrentIndex(static_cast<int>(mode));
    }
}

void SelectionToolBar::setupUI() {
    createActions();

    // Add tool actions
    addAction(m_singleSelectAction);
    addAction(m_areaSelectAction);
    addAction(m_lassoSelectAction);
    addAction(m_selectByPartAction);
    addAction(m_selectByQualityAction);

    addSeparator();

    // Add mode combo
    m_modeLabel = new QLabel("Mode:", this);
    addWidget(m_modeLabel);

    m_modeComboBox = new QComboBox(this);
    m_modeComboBox->addItem("Replace");
    m_modeComboBox->addItem("Add");
    m_modeComboBox->addItem("Subtract");
    m_modeComboBox->addItem("Intersect");
    addWidget(m_modeComboBox);

    connect(m_modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SelectionToolBar::onModeComboChanged);
}

void SelectionToolBar::createActions() {
    m_toolActionGroup = new QActionGroup(this);
    m_toolActionGroup->setExclusive(true);

    m_singleSelectAction = new QAction("Single", this);
    m_singleSelectAction->setCheckable(true);
    m_singleSelectAction->setChecked(true);
    m_singleSelectAction->setToolTip(getToolTooltip(SelectionTool::Single));
    m_toolActionGroup->addAction(m_singleSelectAction);

    m_areaSelectAction = new QAction("Area", this);
    m_areaSelectAction->setCheckable(true);
    m_areaSelectAction->setToolTip(getToolTooltip(SelectionTool::Area));
    m_toolActionGroup->addAction(m_areaSelectAction);

    m_lassoSelectAction = new QAction("Lasso", this);
    m_lassoSelectAction->setCheckable(true);
    m_lassoSelectAction->setToolTip(getToolTooltip(SelectionTool::Lasso));
    m_toolActionGroup->addAction(m_lassoSelectAction);

    m_selectByPartAction = new QAction("By Part", this);
    m_selectByPartAction->setCheckable(true);
    m_selectByPartAction->setToolTip(getToolTooltip(SelectionTool::ByPart));
    m_toolActionGroup->addAction(m_selectByPartAction);

    m_selectByQualityAction = new QAction("By Quality", this);
    m_selectByQualityAction->setCheckable(true);
    m_selectByQualityAction->setToolTip(getToolTooltip(SelectionTool::ByQuality));
    m_toolActionGroup->addAction(m_selectByQualityAction);

    connect(m_toolActionGroup, &QActionGroup::triggered,
            this, &SelectionToolBar::onToolActionTriggered);
}

void SelectionToolBar::onToolActionTriggered(QAction* action) {
    // Determine which tool was selected and emit signal
    if (action == m_singleSelectAction) {
        m_currentTool = SelectionTool::Single;
    } else if (action == m_areaSelectAction) {
        m_currentTool = SelectionTool::Area;
    } else if (action == m_lassoSelectAction) {
        m_currentTool = SelectionTool::Lasso;
    } else if (action == m_selectByPartAction) {
        m_currentTool = SelectionTool::ByPart;
    } else if (action == m_selectByQualityAction) {
        m_currentTool = SelectionTool::ByQuality;
    }

    emit selectionToolChanged(m_currentTool);
}

void SelectionToolBar::onModeComboChanged(int index) {
    m_currentMode = static_cast<SelectionMode>(index);
    emit selectionModeChanged(m_currentMode);
}

QString SelectionToolBar::getToolIcon(SelectionTool tool) const {
    // Icon paths for future implementation
    // For now, return empty string (Qt will use text labels)
    // Future: Return paths to icon files in resources
    //
    // Example:
    // case SelectionTool::Single:
    //     return ":/icons/select_single.svg";
    //
    // For now, icons are not required - text labels are sufficient

    switch (tool) {
        case SelectionTool::Single:
            return QString();  // Future: ":/icons/select_single.svg"
        case SelectionTool::Area:
            return QString();  // Future: ":/icons/select_area.svg"
        case SelectionTool::Lasso:
            return QString();  // Future: ":/icons/select_lasso.svg"
        case SelectionTool::ByPart:
            return QString();  // Future: ":/icons/select_part.svg"
        case SelectionTool::ByQuality:
            return QString();  // Future: ":/icons/select_quality.svg"
        default:
            return QString();
    }
}

QString SelectionToolBar::getToolTooltip(SelectionTool tool) const {
    switch (tool) {
        case SelectionTool::Single:
            return "Single Element Selection (Click)";
        case SelectionTool::Area:
            return "Rectangular Area Selection (Drag)";
        case SelectionTool::Lasso:
            return "Free-form Lasso Selection";
        case SelectionTool::ByPart:
            return "Select All Elements in Part";
        case SelectionTool::ByQuality:
            return "Select Elements by Quality Threshold";
        default:
            return "";
    }
}

QString SelectionToolBar::getModeText(SelectionMode mode) const {
    switch (mode) {
        case SelectionMode::Replace:
            return "Replace";
        case SelectionMode::Add:
            return "Add";
        case SelectionMode::Subtract:
            return "Subtract";
        case SelectionMode::Intersect:
            return "Intersect";
        default:
            return "Unknown";
    }
}

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
