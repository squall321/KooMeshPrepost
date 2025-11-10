/**
 * @file SelectionToolBar.cpp
 * @brief Selection toolbar stub - Phase 75 (To be fully implemented)
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
    m_currentTool = tool;
    // TODO: Phase 75 - Update UI to reflect tool change
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
    // TODO: Phase 75 - Determine which tool was selected and emit signal
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
    // TODO: Phase 75 - Return icon paths
    return QString();
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
