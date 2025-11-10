/**
 * @file SelectionToolBar.h
 * @brief Selection tools toolbar
 *
 * Toolbar providing various selection tools:
 * - Single element selection
 * - Area/rectangle selection
 * - Lasso selection
 * - Selection by part
 * - Selection modes (replace, add, subtract)
 *
 * Uses Qt6 under LGPL v3 license (dynamically linked)
 */

#pragma once

#ifdef KOOMESH_HAS_QT

#include <QToolBar>
#include <QString>

// Forward declarations - Qt
class QAction;
class QActionGroup;
class QComboBox;
class QLabel;

namespace koomesh {
namespace ui {

/**
 * @brief Selection tool types
 */
enum class SelectionTool {
    Single,      ///< Single element selection
    Area,        ///< Rectangular area selection
    Lasso,       ///< Free-form lasso selection
    ByPart,      ///< Select all elements in a part
    ByQuality    ///< Select elements by quality threshold
};

/**
 * @brief Selection modes
 */
enum class SelectionMode {
    Replace,     ///< Replace current selection
    Add,         ///< Add to current selection
    Subtract,    ///< Subtract from current selection
    Intersect    ///< Intersect with current selection
};

/**
 * @brief Selection toolbar with various selection tools
 *
 * Toolbar layout:
 * ```
 * [▶ Single] [☐ Area] [◯ Lasso] [📦 Part] [🔍 Quality] │ Mode: [Replace ▼]
 * ```
 *
 * Features:
 * - Exclusive selection tool buttons
 * - Selection mode combo box
 * - Visual feedback for active tool
 * - Keyboard shortcuts
 */
class SelectionToolBar : public QToolBar {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit SelectionToolBar(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SelectionToolBar() override;

    /**
     * @brief Get current selection tool
     * @return Active selection tool
     */
    SelectionTool getCurrentTool() const { return m_currentTool; }

    /**
     * @brief Set current selection tool
     * @param tool Tool to activate
     */
    void setCurrentTool(SelectionTool tool);

    /**
     * @brief Get current selection mode
     * @return Active selection mode
     */
    SelectionMode getCurrentMode() const { return m_currentMode; }

    /**
     * @brief Set current selection mode
     * @param mode Mode to set
     */
    void setCurrentMode(SelectionMode mode);

signals:
    /**
     * @brief Emitted when selection tool changes
     * @param tool New active tool
     */
    void selectionToolChanged(SelectionTool tool);

    /**
     * @brief Emitted when selection mode changes
     * @param mode New active mode
     */
    void selectionModeChanged(SelectionMode mode);

private slots:
    /**
     * @brief Handle tool action triggered
     * @param action Action that was triggered
     */
    void onToolActionTriggered(QAction* action);

    /**
     * @brief Handle mode combo box changed
     * @param index Selected index
     */
    void onModeComboChanged(int index);

private:
    /**
     * @brief Setup toolbar UI
     */
    void setupUI();

    /**
     * @brief Create tool actions
     */
    void createActions();

    /**
     * @brief Get icon for selection tool
     * @param tool Selection tool
     * @return Icon path or identifier
     */
    QString getToolIcon(SelectionTool tool) const;

    /**
     * @brief Get tooltip for selection tool
     * @param tool Selection tool
     * @return Tooltip text
     */
    QString getToolTooltip(SelectionTool tool) const;

    /**
     * @brief Get text for selection mode
     * @param mode Selection mode
     * @return Mode text
     */
    QString getModeText(SelectionMode mode) const;

    // Actions
    QActionGroup* m_toolActionGroup;
    QAction* m_singleSelectAction;
    QAction* m_areaSelectAction;
    QAction* m_lassoSelectAction;
    QAction* m_selectByPartAction;
    QAction* m_selectByQualityAction;

    // Mode selection
    QLabel* m_modeLabel;
    QComboBox* m_modeComboBox;

    // State
    SelectionTool m_currentTool;
    SelectionMode m_currentMode;
};

} // namespace ui
} // namespace koomesh

#else // !KOOMESH_HAS_QT

// Stub when Qt not available
namespace koomesh {
namespace ui {

enum class SelectionTool { Single, Area, Lasso, ByPart, ByQuality };
enum class SelectionMode { Replace, Add, Subtract, Intersect };

class SelectionToolBar {
public:
    SelectionToolBar(void* = nullptr) {}
    SelectionTool getCurrentTool() const { return SelectionTool::Single; }
    SelectionMode getCurrentMode() const { return SelectionMode::Replace; }
};

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
