/**
 * @file MainWindow.h
 * @brief Main application window with Qt6 UI
 *
 * Main window implementing the complete UI layout with:
 * - Central 3D viewport (VTK widget)
 * - Dockable panels (Parts, Groups, Properties)
 * - Menu bar and toolbars
 * - Status bar with statistics
 *
 * Uses Qt6 under LGPL v3 license (dynamically linked)
 * See docs/QT_LGPL_COMPLIANCE.md for details
 */

#pragma once

#include "core/Types.h"
#include <memory>
#include <vector>
#include <string>

#ifdef KOOMESH_HAS_QT

#include <QMainWindow>
#include <QString>
#include <QTimer>

// Forward declarations - Qt
class QVTKOpenGLNativeWidget;
class QToolBar;
class QDockWidget;
class QLabel;
class QAction;
class QMenu;
class QProgressBar;

// Forward declarations - KooMesh
namespace koomesh {

namespace core {
class Mesh;
class GroupManager;
}

namespace visualization {
class VTKRenderer;
}

namespace io {
class AsyncFileLoader;
class LSDynaFileWriter;
class VTKFileWriter;
class STLFileWriter;
}

namespace selection {
class SelectionManager;
}

namespace ui {

// Forward declarations for UI components
class PartsPanel;
class GroupsPanel;
class PropertiesPanel;
class SelectionToolBar;
class ProgressDialog;

/**
 * @brief Main application window
 *
 * Central window managing the entire application UI including:
 * - 3D visualization viewport
 * - Parts hierarchy panel
 * - Groups management panel
 * - Properties inspector panel
 * - Menu bar with all actions
 * - Toolbars for common operations
 * - Status bar with real-time statistics
 *
 * Layout:
 * ```
 * ┌─────────────────────────────────────┐
 * │ Menu Bar                            │
 * ├─────────────────────────────────────┤
 * │ Toolbars                            │
 * ├──────┬──────────────────────┬───────┤
 * │Parts │   3D Viewport        │ Props │
 * │Panel │   (VTK Widget)       │ Panel │
 * ├──────┤                      ├───────┤
 * │Groups│                      │       │
 * │Panel │                      │       │
 * └──────┴──────────────────────┴───────┘
 * │ Status Bar                          │
 * └─────────────────────────────────────┘
 * ```
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget (nullptr for top-level window)
     */
    explicit MainWindow(QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~MainWindow() override;

    // ========================================================================
    // Public API
    // ========================================================================

    /**
     * @brief Load mesh from file
     * @param filepath Path to mesh file
     */
    void loadMeshFile(const QString& filepath);

    /**
     * @brief Set mesh for visualization
     * @param mesh Pointer to mesh object (not owned)
     */
    void setMesh(core::Mesh* mesh);

    /**
     * @brief Get currently loaded mesh
     * @return Pointer to current mesh (may be nullptr)
     */
    core::Mesh* getMesh() const { return m_mesh; }

    /**
     * @brief Update 3D viewport rendering
     */
    void updateViewport();

    /**
     * @brief Get current file path
     * @return Current file path or empty string
     */
    QString getCurrentFilePath() const { return m_currentFilePath; }

    /**
     * @brief Check if there are unsaved changes
     * @return True if modified
     */
    bool isModified() const { return m_modified; }

    /**
     * @brief Set modified state
     * @param modified True if there are unsaved changes
     */
    void setModified(bool modified);

    // ========================================================================
    // Panel Access
    // ========================================================================

    /**
     * @brief Get parts panel
     * @return Pointer to parts panel (never nullptr after construction)
     */
    PartsPanel* getPartsPanel() const { return m_partsPanel; }

    /**
     * @brief Get groups panel
     * @return Pointer to groups panel (never nullptr after construction)
     */
    GroupsPanel* getGroupsPanel() const { return m_groupsPanel; }

    /**
     * @brief Get properties panel
     * @return Pointer to properties panel (never nullptr after construction)
     */
    PropertiesPanel* getPropertiesPanel() const { return m_propertiesPanel; }

signals:
    // File operations
    void fileOpened(const QString& filepath);
    void fileSaved(const QString& filepath);
    void fileExported(const QString& filepath);
    void fileClosed();

    // Selection
    void selectionChanged(const std::vector<core::ElementId>& elements);
    void selectionCleared();

    // View
    void viewResetRequested();
    void viewChanged();

    // Application
    void aboutToQuit();

public slots:
    /**
     * @brief Show status message
     * @param message Message to display
     * @param timeout Duration in milliseconds (0 for permanent)
     */
    void showStatusMessage(const QString& message, int timeout = 0);

    /**
     * @brief Update statistics in status bar
     * @param elementCount Total number of elements
     * @param selectedCount Number of selected elements
     */
    void updateStatistics(int elementCount, int selectedCount);

    /**
     * @brief Show progress indicator
     * @param visible True to show, false to hide
     */
    void showProgress(bool visible);

    /**
     * @brief Set progress value
     * @param value Progress percentage (0-100)
     */
    void setProgress(int value);

private slots:
    // File menu
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onFileSaveAs();
    void onFileExport();
    void onFileClose();
    void onFileRecentFile();
    void onFileExit();

    // Edit menu
    void onEditUndo();
    void onEditRedo();
    void onEditCut();
    void onEditCopy();
    void onEditPaste();
    void onEditDelete();
    void onEditSelectAll();
    void onEditDeselectAll();
    void onEditInvertSelection();

    // View menu
    void onViewResetCamera();
    void onViewFront();
    void onViewBack();
    void onViewTop();
    void onViewBottom();
    void onViewLeft();
    void onViewRight();
    void onViewIsometric();
    void onViewTogglePartsPanel();
    void onViewToggleGroupsPanel();
    void onViewTogglePropertiesPanel();
    void onViewFullscreen();

    // Tools menu
    void onToolsMeasureDistance();
    void onToolsMeasureAngle();
    void onToolsCreateGroup();
    void onToolsSettings();

    // Help menu
    void onHelpDocumentation();
    void onHelpAbout();
    void onHelpAboutQt();

    // Timer
    void onUpdateTimer();

protected:
    /**
     * @brief Handle close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent* event) override;

    /**
     * @brief Handle resize event
     * @param event Resize event
     */
    void resizeEvent(QResizeEvent* event) override;

private:
    // ========================================================================
    // Setup Methods
    // ========================================================================

    /**
     * @brief Setup complete UI
     */
    void setupUI();

    /**
     * @brief Create menu bar
     */
    void createMenuBar();

    /**
     * @brief Create toolbars
     */
    void createToolBars();

    /**
     * @brief Create dock widgets
     */
    void createDockWidgets();

    /**
     * @brief Create status bar
     */
    void createStatusBar();

    /**
     * @brief Create central widget (VTK viewport)
     */
    void createCentralWidget();

    /**
     * @brief Setup keyboard shortcuts
     */
    void setupShortcuts();

    /**
     * @brief Setup signal connections
     */
    void setupConnections();

    /**
     * @brief Load application settings
     */
    void loadSettings();

    /**
     * @brief Save application settings
     */
    void saveSettings();

    /**
     * @brief Update window title
     */
    void updateWindowTitle();

    /**
     * @brief Update recent files menu
     */
    void updateRecentFilesMenu();

    /**
     * @brief Add to recent files
     * @param filepath File path to add
     */
    void addRecentFile(const QString& filepath);

    /**
     * @brief Check for unsaved changes and prompt user
     * @return True if can proceed, false if user cancelled
     */
    bool maybeSave();

    // ========================================================================
    // Menu Actions
    // ========================================================================

    // File menu
    QMenu* m_fileMenu;
    QMenu* m_recentFilesMenu;
    QAction* m_newAction;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_exportAction;
    QAction* m_closeAction;
    QAction* m_exitAction;

    // Edit menu
    QMenu* m_editMenu;
    QAction* m_undoAction;
    QAction* m_redoAction;
    QAction* m_cutAction;
    QAction* m_copyAction;
    QAction* m_pasteAction;
    QAction* m_deleteAction;
    QAction* m_selectAllAction;
    QAction* m_deselectAllAction;
    QAction* m_invertSelectionAction;

    // View menu
    QMenu* m_viewMenu;
    QMenu* m_viewCameraMenu;
    QAction* m_resetCameraAction;
    QAction* m_viewFrontAction;
    QAction* m_viewBackAction;
    QAction* m_viewTopAction;
    QAction* m_viewBottomAction;
    QAction* m_viewLeftAction;
    QAction* m_viewRightAction;
    QAction* m_viewIsometricAction;
    QAction* m_togglePartsPanelAction;
    QAction* m_toggleGroupsPanelAction;
    QAction* m_togglePropertiesPanelAction;
    QAction* m_fullscreenAction;

    // Tools menu
    QMenu* m_toolsMenu;
    QAction* m_measureDistanceAction;
    QAction* m_measureAngleAction;
    QAction* m_createGroupAction;
    QAction* m_settingsAction;

    // Help menu
    QMenu* m_helpMenu;
    QAction* m_documentationAction;
    QAction* m_aboutAction;
    QAction* m_aboutQtAction;

    // ========================================================================
    // Toolbars
    // ========================================================================

    QToolBar* m_mainToolBar;
    SelectionToolBar* m_selectionToolBar;
    QToolBar* m_viewToolBar;

    // ========================================================================
    // Dock Widgets and Panels
    // ========================================================================

    QDockWidget* m_partsDock;
    QDockWidget* m_groupsDock;
    QDockWidget* m_propertiesDock;

    PartsPanel* m_partsPanel;
    GroupsPanel* m_groupsPanel;
    PropertiesPanel* m_propertiesPanel;

    // ========================================================================
    // Central Widget
    // ========================================================================

#ifdef KOOMESH_HAS_VTK
    QVTKOpenGLNativeWidget* m_vtkWidget;
#else
    QWidget* m_vtkWidget;  // Placeholder when VTK not available
#endif

    // ========================================================================
    // Status Bar Widgets
    // ========================================================================

    QLabel* m_statusLabel;
    QLabel* m_elementCountLabel;
    QLabel* m_selectedCountLabel;
    QLabel* m_fpsLabel;
    QProgressBar* m_progressBar;

    // ========================================================================
    // Data
    // ========================================================================

    core::Mesh* m_mesh;  // Not owned
    std::unique_ptr<visualization::VTKRenderer> m_renderer;

    QString m_currentFilePath;
    bool m_modified;

    QStringList m_recentFiles;
    static constexpr int MAX_RECENT_FILES = 10;

    // ========================================================================
    // Timer
    // ========================================================================

    QTimer* m_updateTimer;
    int m_frameCount;
    qint64 m_lastFpsUpdate;

    // ========================================================================
    // File I/O (Phase 76)
    // ========================================================================

    std::unique_ptr<io::AsyncFileLoader> m_fileLoader;
    std::unique_ptr<io::LSDynaFileWriter> m_lsdynaWriter;
    std::unique_ptr<io::VTKFileWriter> m_vtkWriter;
    std::unique_ptr<io::STLFileWriter> m_stlWriter;
    ProgressDialog* m_progressDialog;

    // ========================================================================
    // Selection and Groups (Phase 77)
    // ========================================================================

    std::unique_ptr<selection::SelectionManager> m_selectionManager;
    std::unique_ptr<core::GroupManager> m_groupManager;
};

} // namespace ui
} // namespace koomesh

#else // !KOOMESH_HAS_QT

// Stub implementation when Qt is not available
namespace koomesh {
namespace ui {

class MainWindow {
public:
    MainWindow() {}
    ~MainWindow() {}

    void show() {}
    void loadMeshFile(const std::string&) {}
};

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
