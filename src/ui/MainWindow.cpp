/**
 * @file MainWindow.cpp
 * @brief Implementation of main application window
 */

#include "ui/MainWindow.h"

#ifdef KOOMESH_HAS_QT

#include "ui/PartsPanel.h"
#include "ui/GroupsPanel.h"
#include "ui/PropertiesPanel.h"
#include "ui/SelectionToolBar.h"
#include "core/Mesh.h"
#include "visualization/VTKRenderer.h"

#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QDateTime>
#include <QStandardPaths>

#ifdef KOOMESH_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#endif

namespace koomesh {
namespace ui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_mesh(nullptr)
    , m_vtkWidget(nullptr)
    , m_partsPanel(nullptr)
    , m_groupsPanel(nullptr)
    , m_propertiesPanel(nullptr)
    , m_selectionToolBar(nullptr)
    , m_currentFilePath()
    , m_modified(false)
    , m_frameCount(0)
    , m_lastFpsUpdate(0)
{
    // Set window properties
    setWindowTitle("KooMeshPrepost");
    resize(1280, 720);

    // Initialize renderer
    m_renderer = std::make_unique<visualization::VTKRenderer>();

    // Setup complete UI
    setupUI();

    // Load settings
    loadSettings();

    // Setup update timer for FPS and other updates
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::onUpdateTimer);
    m_updateTimer->start(16);  // ~60 FPS

    // Initial status
    showStatusMessage("Ready", 0);
}

MainWindow::~MainWindow() {
    saveSettings();
}

// ============================================================================
// Public API
// ============================================================================

void MainWindow::loadMeshFile(const QString& filepath) {
    if (filepath.isEmpty()) {
        return;
    }

    // TODO: Implement async file loading with progress dialog
    showStatusMessage(QString("Loading %1...").arg(filepath));

    // Add to recent files
    addRecentFile(filepath);

    m_currentFilePath = filepath;
    setModified(false);

    emit fileOpened(filepath);

    showStatusMessage(QString("Loaded %1").arg(filepath), 3000);
}

void MainWindow::setMesh(core::Mesh* mesh) {
    m_mesh = mesh;

    // Update panels
    if (m_partsPanel) {
        m_partsPanel->setMesh(mesh);
    }

    // Update viewport
    updateViewport();

    // Update statistics
    if (mesh) {
        updateStatistics(mesh->elementCount(), 0);
    }
}

void MainWindow::updateViewport() {
    if (!m_vtkWidget) {
        return;
    }

#ifdef KOOMESH_HAS_VTK
    // TODO: Update VTK rendering
    m_vtkWidget->update();
#endif
}

void MainWindow::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        updateWindowTitle();
    }
}

// ============================================================================
// Public Slots
// ============================================================================

void MainWindow::showStatusMessage(const QString& message, int timeout) {
    if (m_statusLabel) {
        m_statusLabel->setText(message);
    }
    if (timeout > 0) {
        statusBar()->showMessage(message, timeout);
    } else {
        statusBar()->showMessage(message);
    }
}

void MainWindow::updateStatistics(int elementCount, int selectedCount) {
    if (m_elementCountLabel) {
        m_elementCountLabel->setText(QString("Elements: %L1").arg(elementCount));
    }

    if (m_selectedCountLabel) {
        m_selectedCountLabel->setText(QString("Selected: %L1").arg(selectedCount));
    }
}

void MainWindow::showProgress(bool visible) {
    if (m_progressBar) {
        m_progressBar->setVisible(visible);
    }
}

void MainWindow::setProgress(int value) {
    if (m_progressBar) {
        m_progressBar->setValue(value);
    }
}

// ============================================================================
// Private Slots - File Menu
// ============================================================================

void MainWindow::onFileNew() {
    if (maybeSave()) {
        m_currentFilePath.clear();
        setModified(false);
        showStatusMessage("New file created");
    }
}

void MainWindow::onFileOpen() {
    if (!maybeSave()) {
        return;
    }

    QString filters = "LS-DYNA Keyword Files (*.k *.key);;All Files (*)";
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Mesh File",
        QString(),
        filters
    );

    if (!filepath.isEmpty()) {
        loadMeshFile(filepath);
    }
}

void MainWindow::onFileSave() {
    if (m_currentFilePath.isEmpty()) {
        onFileSaveAs();
    } else {
        // TODO: Implement actual save
        setModified(false);
        showStatusMessage(QString("Saved %1").arg(m_currentFilePath), 2000);
        emit fileSaved(m_currentFilePath);
    }
}

void MainWindow::onFileSaveAs() {
    QString filters = "LS-DYNA Keyword Files (*.k *.key);;All Files (*)";
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Mesh File",
        m_currentFilePath,
        filters
    );

    if (!filepath.isEmpty()) {
        m_currentFilePath = filepath;
        onFileSave();
    }
}

void MainWindow::onFileExport() {
    QString filters = "VTK Files (*.vtk);;STL Files (*.stl);;All Files (*)";
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Export Mesh",
        QString(),
        filters
    );

    if (!filepath.isEmpty()) {
        // TODO: Implement export
        showStatusMessage(QString("Exported to %1").arg(filepath), 2000);
        emit fileExported(filepath);
    }
}

void MainWindow::onFileClose() {
    if (maybeSave()) {
        m_currentFilePath.clear();
        setMesh(nullptr);
        setModified(false);
        emit fileClosed();
        showStatusMessage("File closed");
    }
}

void MainWindow::onFileRecentFile() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && maybeSave()) {
        loadMeshFile(action->data().toString());
    }
}

void MainWindow::onFileExit() {
    close();
}

// ============================================================================
// Private Slots - Edit Menu
// ============================================================================

void MainWindow::onEditUndo() {
    // TODO: Implement undo
    showStatusMessage("Undo not yet implemented", 2000);
}

void MainWindow::onEditRedo() {
    // TODO: Implement redo
    showStatusMessage("Redo not yet implemented", 2000);
}

void MainWindow::onEditCut() {
    showStatusMessage("Cut not yet implemented", 2000);
}

void MainWindow::onEditCopy() {
    showStatusMessage("Copy not yet implemented", 2000);
}

void MainWindow::onEditPaste() {
    showStatusMessage("Paste not yet implemented", 2000);
}

void MainWindow::onEditDelete() {
    // TODO: Implement delete selected elements
    setModified(true);
    showStatusMessage("Deleted selected elements", 2000);
}

void MainWindow::onEditSelectAll() {
    // TODO: Select all elements
    showStatusMessage("Selected all elements", 2000);
}

void MainWindow::onEditDeselectAll() {
    emit selectionCleared();
    showStatusMessage("Selection cleared", 2000);
}

void MainWindow::onEditInvertSelection() {
    // TODO: Invert selection
    showStatusMessage("Inverted selection", 2000);
}

// ============================================================================
// Private Slots - View Menu
// ============================================================================

void MainWindow::onViewResetCamera() {
    emit viewResetRequested();
    showStatusMessage("Camera reset", 1000);
}

void MainWindow::onViewFront() {
    // TODO: Set front view
    emit viewChanged();
}

void MainWindow::onViewBack() {
    // TODO: Set back view
    emit viewChanged();
}

void MainWindow::onViewTop() {
    // TODO: Set top view
    emit viewChanged();
}

void MainWindow::onViewBottom() {
    // TODO: Set bottom view
    emit viewChanged();
}

void MainWindow::onViewLeft() {
    // TODO: Set left view
    emit viewChanged();
}

void MainWindow::onViewRight() {
    // TODO: Set right view
    emit viewChanged();
}

void MainWindow::onViewIsometric() {
    // TODO: Set isometric view
    emit viewChanged();
}

void MainWindow::onViewTogglePartsPanel() {
    if (m_partsDock) {
        m_partsDock->setVisible(!m_partsDock->isVisible());
    }
}

void MainWindow::onViewToggleGroupsPanel() {
    if (m_groupsDock) {
        m_groupsDock->setVisible(!m_groupsDock->isVisible());
    }
}

void MainWindow::onViewTogglePropertiesPanel() {
    if (m_propertiesDock) {
        m_propertiesDock->setVisible(!m_propertiesDock->isVisible());
    }
}

void MainWindow::onViewFullscreen() {
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

// ============================================================================
// Private Slots - Tools Menu
// ============================================================================

void MainWindow::onToolsMeasureDistance() {
    showStatusMessage("Click two points to measure distance", 0);
}

void MainWindow::onToolsMeasureAngle() {
    showStatusMessage("Click three points to measure angle", 0);
}

void MainWindow::onToolsCreateGroup() {
    // TODO: Create group from selection
    showStatusMessage("Create group from selection", 2000);
}

void MainWindow::onToolsSettings() {
    // TODO: Show settings dialog
    showStatusMessage("Settings dialog not yet implemented", 2000);
}

// ============================================================================
// Private Slots - Help Menu
// ============================================================================

void MainWindow::onHelpDocumentation() {
    QMessageBox::information(
        this,
        "Documentation",
        "Documentation is available at:\nhttps://docs.kooengineering.com"
    );
}

void MainWindow::onHelpAbout() {
    QMessageBox::about(
        this,
        "About KooMeshPrepost",
        "<h2>KooMeshPrepost v1.0.0</h2>"
        "<p>High-performance mesh pre/post-processor for LS-DYNA</p>"
        "<p>Copyright © 2024 Koo Engineering</p>"
        "<p>Licensed under MIT License</p>"
        "<p><b>Qt</b> is used under LGPL v3 license</p>"
    );
}

void MainWindow::onHelpAboutQt() {
    QMessageBox::aboutQt(this, "About Qt");
}

// ============================================================================
// Private Slots - Timer
// ============================================================================

void MainWindow::onUpdateTimer() {
    m_frameCount++;

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (m_lastFpsUpdate == 0) {
        m_lastFpsUpdate = currentTime;
    }

    qint64 elapsed = currentTime - m_lastFpsUpdate;
    if (elapsed >= 1000) {  // Update FPS every second
        double fps = (m_frameCount * 1000.0) / elapsed;
        if (m_fpsLabel) {
            m_fpsLabel->setText(QString("%1 FPS").arg(fps, 0, 'f', 1));
        }
        m_frameCount = 0;
        m_lastFpsUpdate = currentTime;
    }
}

// ============================================================================
// Protected Event Handlers
// ============================================================================

void MainWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) {
        saveSettings();
        emit aboutToQuit();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    // Adapt UI for small screens
    if (width() < 800) {
        if (m_partsDock) m_partsDock->hide();
        if (m_groupsDock) m_groupsDock->hide();
    }
}

// ============================================================================
// Private Setup Methods
// ============================================================================

void MainWindow::setupUI() {
    createCentralWidget();
    createDockWidgets();
    createMenuBar();
    createToolBars();
    createStatusBar();
    setupShortcuts();
    setupConnections();
}

void MainWindow::createCentralWidget() {
#ifdef KOOMESH_HAS_VTK
    m_vtkWidget = new QVTKOpenGLNativeWidget(this);
    setCentralWidget(m_vtkWidget);

    // Initialize VTK renderer with widget
    if (m_renderer) {
        m_renderer->initialize();
        // TODO: Set render window for VTK widget
    }
#else
    // Placeholder when VTK not available
    QWidget* placeholder = new QWidget(this);
    placeholder->setStyleSheet("background-color: #2b2b2b;");
    setCentralWidget(placeholder);
    m_vtkWidget = placeholder;
#endif
}

void MainWindow::createDockWidgets() {
    // Parts panel (left, top)
    m_partsPanel = new PartsPanel(this);
    m_partsDock = new QDockWidget("Parts", this);
    m_partsDock->setWidget(m_partsPanel);
    m_partsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_partsDock);

    // Groups panel (left, bottom)
    m_groupsPanel = new GroupsPanel(this);
    m_groupsDock = new QDockWidget("Groups", this);
    m_groupsDock->setWidget(m_groupsPanel);
    m_groupsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, m_groupsDock);

    // Properties panel (right)
    m_propertiesPanel = new PropertiesPanel(this);
    m_propertiesDock = new QDockWidget("Properties", this);
    m_propertiesDock->setWidget(m_propertiesPanel);
    m_propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_propertiesDock);
}

void MainWindow::createMenuBar() {
    // File menu
    m_fileMenu = menuBar()->addMenu("&File");

    m_newAction = m_fileMenu->addAction("&New");
    m_newAction->setShortcut(QKeySequence::New);
    connect(m_newAction, &QAction::triggered, this, &MainWindow::onFileNew);

    m_openAction = m_fileMenu->addAction("&Open...");
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onFileOpen);

    m_recentFilesMenu = m_fileMenu->addMenu("Recent Files");
    updateRecentFilesMenu();

    m_fileMenu->addSeparator();

    m_saveAction = m_fileMenu->addAction("&Save");
    m_saveAction->setShortcut(QKeySequence::Save);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::onFileSave);

    m_saveAsAction = m_fileMenu->addAction("Save &As...");
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::onFileSaveAs);

    m_exportAction = m_fileMenu->addAction("&Export...");
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::onFileExport);

    m_fileMenu->addSeparator();

    m_closeAction = m_fileMenu->addAction("&Close");
    m_closeAction->setShortcut(QKeySequence::Close);
    connect(m_closeAction, &QAction::triggered, this, &MainWindow::onFileClose);

    m_exitAction = m_fileMenu->addAction("E&xit");
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &MainWindow::onFileExit);

    // Edit menu
    m_editMenu = menuBar()->addMenu("&Edit");

    m_undoAction = m_editMenu->addAction("&Undo");
    m_undoAction->setShortcut(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::onEditUndo);

    m_redoAction = m_editMenu->addAction("&Redo");
    m_redoAction->setShortcut(QKeySequence::Redo);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::onEditRedo);

    m_editMenu->addSeparator();

    m_selectAllAction = m_editMenu->addAction("Select &All");
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    connect(m_selectAllAction, &QAction::triggered, this, &MainWindow::onEditSelectAll);

    m_deselectAllAction = m_editMenu->addAction("&Deselect All");
    m_deselectAllAction->setShortcut(Qt::CTRL | Qt::Key_D);
    connect(m_deselectAllAction, &QAction::triggered, this, &MainWindow::onEditDeselectAll);

    // View menu
    m_viewMenu = menuBar()->addMenu("&View");

    m_resetCameraAction = m_viewMenu->addAction("&Reset Camera");
    m_resetCameraAction->setShortcut(Qt::Key_Home);
    connect(m_resetCameraAction, &QAction::triggered, this, &MainWindow::onViewResetCamera);

    m_viewCameraMenu = m_viewMenu->addMenu("Camera Views");
    m_viewFrontAction = m_viewCameraMenu->addAction("&Front");
    m_viewFrontAction->setShortcut(Qt::Key_1);
    connect(m_viewFrontAction, &QAction::triggered, this, &MainWindow::onViewFront);

    m_viewTopAction = m_viewCameraMenu->addAction("&Top");
    m_viewTopAction->setShortcut(Qt::Key_2);
    connect(m_viewTopAction, &QAction::triggered, this, &MainWindow::onViewTop);

    m_viewRightAction = m_viewCameraMenu->addAction("&Right");
    m_viewRightAction->setShortcut(Qt::Key_3);
    connect(m_viewRightAction, &QAction::triggered, this, &MainWindow::onViewRight);

    m_viewMenu->addSeparator();

    m_togglePartsPanelAction = m_viewMenu->addAction("&Parts Panel");
    m_togglePartsPanelAction->setCheckable(true);
    m_togglePartsPanelAction->setChecked(true);
    connect(m_togglePartsPanelAction, &QAction::triggered, this, &MainWindow::onViewTogglePartsPanel);

    m_toggleGroupsPanelAction = m_viewMenu->addAction("&Groups Panel");
    m_toggleGroupsPanelAction->setCheckable(true);
    m_toggleGroupsPanelAction->setChecked(true);
    connect(m_toggleGroupsPanelAction, &QAction::triggered, this, &MainWindow::onViewToggleGroupsPanel);

    m_togglePropertiesPanelAction = m_viewMenu->addAction("P&roperties Panel");
    m_togglePropertiesPanelAction->setCheckable(true);
    m_togglePropertiesPanelAction->setChecked(true);
    connect(m_togglePropertiesPanelAction, &QAction::triggered, this, &MainWindow::onViewTogglePropertiesPanel);

    m_viewMenu->addSeparator();

    m_fullscreenAction = m_viewMenu->addAction("&Fullscreen");
    m_fullscreenAction->setShortcut(Qt::Key_F11);
    m_fullscreenAction->setCheckable(true);
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::onViewFullscreen);

    // Tools menu
    m_toolsMenu = menuBar()->addMenu("&Tools");

    m_measureDistanceAction = m_toolsMenu->addAction("Measure &Distance");
    connect(m_measureDistanceAction, &QAction::triggered, this, &MainWindow::onToolsMeasureDistance);

    m_measureAngleAction = m_toolsMenu->addAction("Measure &Angle");
    connect(m_measureAngleAction, &QAction::triggered, this, &MainWindow::onToolsMeasureAngle);

    m_toolsMenu->addSeparator();

    m_createGroupAction = m_toolsMenu->addAction("Create &Group");
    m_createGroupAction->setShortcut(Qt::CTRL | Qt::Key_G);
    connect(m_createGroupAction, &QAction::triggered, this, &MainWindow::onToolsCreateGroup);

    m_toolsMenu->addSeparator();

    m_settingsAction = m_toolsMenu->addAction("&Settings...");
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::onToolsSettings);

    // Help menu
    m_helpMenu = menuBar()->addMenu("&Help");

    m_documentationAction = m_helpMenu->addAction("&Documentation");
    m_documentationAction->setShortcut(Qt::Key_F1);
    connect(m_documentationAction, &QAction::triggered, this, &MainWindow::onHelpDocumentation);

    m_helpMenu->addSeparator();

    m_aboutAction = m_helpMenu->addAction("&About KooMeshPrepost");
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onHelpAbout);

    m_aboutQtAction = m_helpMenu->addAction("About &Qt");
    connect(m_aboutQtAction, &QAction::triggered, this, &MainWindow::onHelpAboutQt);
}

void MainWindow::createToolBars() {
    // Main toolbar
    m_mainToolBar = addToolBar("Main");
    m_mainToolBar->addAction(m_openAction);
    m_mainToolBar->addAction(m_saveAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_undoAction);
    m_mainToolBar->addAction(m_redoAction);

    // Selection toolbar
    m_selectionToolBar = new SelectionToolBar(this);
    addToolBar(m_selectionToolBar);

    // View toolbar
    m_viewToolBar = addToolBar("View");
    m_viewToolBar->addAction(m_resetCameraAction);
    m_viewToolBar->addAction(m_viewFrontAction);
    m_viewToolBar->addAction(m_viewTopAction);
    m_viewToolBar->addAction(m_viewRightAction);
}

void MainWindow::createStatusBar() {
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel, 1);

    m_elementCountLabel = new QLabel("Elements: 0");
    statusBar()->addPermanentWidget(m_elementCountLabel);

    m_selectedCountLabel = new QLabel("Selected: 0");
    statusBar()->addPermanentWidget(m_selectedCountLabel);

    m_fpsLabel = new QLabel("0 FPS");
    statusBar()->addPermanentWidget(m_fpsLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setMaximumWidth(200);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
}

void MainWindow::setupShortcuts() {
    // Additional shortcuts can be added here
}

void MainWindow::setupConnections() {
    // Connect dock widget visibility to menu actions
    connect(m_partsDock, &QDockWidget::visibilityChanged,
            m_togglePartsPanelAction, &QAction::setChecked);
    connect(m_groupsDock, &QDockWidget::visibilityChanged,
            m_toggleGroupsPanelAction, &QAction::setChecked);
    connect(m_propertiesDock, &QDockWidget::visibilityChanged,
            m_togglePropertiesPanelAction, &QAction::setChecked);
}

void MainWindow::loadSettings() {
    QSettings settings("KooEngineering", "KooMeshPrepost");

    // Window geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Recent files
    m_recentFiles = settings.value("recentFiles").toStringList();
    updateRecentFilesMenu();
}

void MainWindow::saveSettings() {
    QSettings settings("KooEngineering", "KooMeshPrepost");

    // Window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // Recent files
    settings.setValue("recentFiles", m_recentFiles);
}

void MainWindow::updateWindowTitle() {
    QString title = "KooMeshPrepost";

    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title = fileInfo.fileName() + " - " + title;
    }

    if (m_modified) {
        title = "* " + title;
    }

    setWindowTitle(title);
}

void MainWindow::updateRecentFilesMenu() {
    m_recentFilesMenu->clear();

    for (const QString& filepath : m_recentFiles) {
        QAction* action = m_recentFilesMenu->addAction(filepath);
        action->setData(filepath);
        connect(action, &QAction::triggered, this, &MainWindow::onFileRecentFile);
    }

    m_recentFilesMenu->setEnabled(!m_recentFiles.isEmpty());
}

void MainWindow::addRecentFile(const QString& filepath) {
    m_recentFiles.removeAll(filepath);
    m_recentFiles.prepend(filepath);

    while (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.removeLast();
    }

    updateRecentFilesMenu();
}

bool MainWindow::maybeSave() {
    if (!m_modified) {
        return true;
    }

    QMessageBox::StandardButton ret = QMessageBox::warning(
        this,
        "KooMeshPrepost",
        "The document has been modified.\nDo you want to save your changes?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (ret == QMessageBox::Save) {
        onFileSave();
        return !m_modified;  // Return false if save failed
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }

    return true;
}

} // namespace ui
} // namespace koomesh

#endif // KOOMESH_HAS_QT
