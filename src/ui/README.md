# UI Module

Qt6 기반 사용자 인터페이스 모듈입니다. 현대적이고 직관적인 UI/UX를 제공합니다.

## 구조

```
ui/
├── MainWindow.cpp            # 메인 윈도우
├── PartsPanel.cpp            # Parts 트리 뷰
├── GroupsPanel.cpp           # Groups 리스트
├── PropertiesPanel.cpp       # Properties 테이블
├── SelectionToolBar.cpp      # 선택 도구 툴바
├── ProgressDialog.cpp        # 진행률 대화상자
├── SettingsDialog.cpp        # 설정 대화상자
├── MenuBarBuilder.cpp        # 메뉴 바 구축
├── ContextMenuManager.cpp    # 컨텍스트 메뉴
└── KeyboardShortcuts.cpp     # 단축키 관리
```

## 레이아웃

```
┌─────────────────────────────────────────────────────────────┐
│ File  Edit  View  Tools  Help                   [_] [□] [X] │
├─────────────────────────────────────────────────────────────┤
│ [📁] [💾] [↶] [↷] │ [▶] [☐] [◯] │ [🔍+] [🔍-] [🏠]        │
├──────┬──────────────────────────────────────────────┬───────┤
│Parts │                                              │ Props │
│------│            3D Viewport                       │-------│
│☑ P1  │         (QVTKOpenGLNativeWidget)             │ Elem  │
│☑ P2  │                                              │ ID: ..│
│☐ P3  │                                              │ Type: │
│      │                                              │ Part: │
├──────┤                                              ├───────┤
│Groups│                                              │       │
│------│                                              │       │
│● Gr1 │                                              │       │
│● Gr2 │                                              │       │
│[New] │                                              │       │
└──────┴──────────────────────────────────────────────┴───────┘
│ Ready │ Elements: 1,234,567 │ Selected: 42         │ 60 FPS│
└─────────────────────────────────────────────────────────────┘
```

## 주요 컴포넌트

### 1. MainWindow

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

    void loadMesh(const QString& filepath);
    void updateViewport();

signals:
    void fileOpened(const QString& filepath);
    void selectionChanged(const std::vector<ElementId>& elements);

private slots:
    void onFileOpen();
    void onFileSave();
    void onFileExport();
    void onViewReset();
    void onUndo();
    void onRedo();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBars();
    void setupDockWidgets();
    void setupStatusBar();

    // 위젯
    QVTKOpenGLNativeWidget* m_vtkWidget;
    PartsPanel* m_partsPanel;
    GroupsPanel* m_groupsPanel;
    PropertiesPanel* m_propertiesPanel;

    // 도구 모음
    QToolBar* m_mainToolBar;
    SelectionToolBar* m_selectionToolBar;

    // 상태
    QString m_currentFile;
    int m_fps;
};
```

### 2. PartsPanel (트리 뷰)

```cpp
class PartsPanel : public QWidget {
    Q_OBJECT

public:
    void setMesh(const Mesh* mesh);
    void updatePartsList();

signals:
    void partSelected(PartId id);
    void partVisibilityChanged(PartId id, bool visible);
    void partColorChanged(PartId id, const QColor& color);

private slots:
    void onItemClicked(const QModelIndex& index);
    void onItemDoubleClicked(const QModelIndex& index);
    void onContextMenu(const QPoint& pos);
    void onVisibilityToggled(PartId id);

private:
    QTreeView* m_treeView;
    QStandardItemModel* m_model;
    const Mesh* m_mesh;

    void buildTree();
    QStandardItem* createPartItem(const Part& part);
};
```

**트리 구조**:
```
Parts
├── Part 1 (Material: Steel)
│   ├── Elements: 10,542
│   └── Nodes: 12,345
├── Part 2 (Material: Aluminum)
│   ├── Elements: 8,231
│   └── Nodes: 9,876
└── Part 3 (Material: Rubber)
    ├── Elements: 5,432
    └── Nodes: 6,543
```

### 3. GroupsPanel

```cpp
class GroupsPanel : public QWidget {
    Q_OBJECT

public:
    void setGroupManager(GroupManager* manager);
    void refreshGroupsList();

signals:
    void groupSelected(const std::string& name);
    void createGroupRequested(const std::string& name);
    void deleteGroupRequested(const std::string& name);
    void groupColorChanged(const std::string& name, const QColor& color);

private slots:
    void onCreateGroup();
    void onDeleteGroup();
    void onRenameGroup();
    void onGroupDoubleClicked(QListWidgetItem* item);
    void onColorPickerClicked();

private:
    QListWidget* m_groupsList;
    QPushButton* m_createBtn;
    QPushButton* m_deleteBtn;
    QPushButton* m_renameBtn;

    GroupManager* m_groupManager;
};
```

### 4. PropertiesPanel

```cpp
class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    void displayElementProperties(ElementId id, const Element* element);
    void displayNodeProperties(NodeId id, const Node* node);
    void displayGroupProperties(const Group* group);
    void displaySelectionStatistics(const std::vector<ElementId>& elements);
    void clear();

private:
    QTableWidget* m_table;
    QLabel* m_titleLabel;

    void addProperty(const QString& name, const QString& value);
    void addSection(const QString& title);
};
```

**표시 예시**:
```
┌─────────────────────────────┐
│ Element Properties          │
├──────────────┬──────────────┤
│ ID           │ 12345        │
│ Type         │ Hexahedron   │
│ Part         │ Part 1       │
│ Nodes        │ 8            │
│ Volume       │ 1.234 mm³    │
│ Quality      │ 0.85         │
│ Center       │ (1,2,3)      │
└──────────────┴──────────────┘
```

### 5. SelectionToolBar

```cpp
class SelectionToolBar : public QToolBar {
    Q_OBJECT

public:
    SelectionToolBar(QWidget* parent = nullptr);

signals:
    void selectionToolChanged(SelectionTool tool);
    void selectionModeChanged(SelectionMode mode);

private slots:
    void onToolButtonClicked(int id);
    void onModeComboChanged(int index);

private:
    QButtonGroup* m_toolButtonGroup;
    QComboBox* m_modeCombo;

    QAction* m_singleSelectAction;
    QAction* m_areaSelectAction;
    QAction* m_lassoSelectAction;
    QAction* m_selectByPartAction;
};
```

**도구 모음**:
```
[▶ Single] [☐ Area] [◯ Lasso] [📦 Part] │ Mode: [Replace ▼]
```

## 다이얼로그

### ProgressDialog

```cpp
class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    ProgressDialog(QWidget* parent = nullptr);

    void setProgress(float progress); // 0.0 ~ 1.0
    void setStatus(const QString& status);
    bool isCancelled() const { return m_cancelled; }

signals:
    void cancelled();

private:
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_cancelButton;
    bool m_cancelled;
};
```

### SettingsDialog

```cpp
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(QWidget* parent = nullptr);

    void loadSettings();
    void saveSettings();

private:
    QTabWidget* m_tabs;

    // 탭들
    QWidget* createVisualizationTab();
    QWidget* createPerformanceTab();
    QWidget* createFileIOTab();
    QWidget* createAppearanceTab();
};
```

## 키보드 단축키

```cpp
class KeyboardShortcuts {
public:
    static void setup(MainWindow* mainWindow, Presenter* presenter) {
        // 파일
        mainWindow->addShortcut("Ctrl+O", &Presenter::openFile);
        mainWindow->addShortcut("Ctrl+S", &Presenter::saveFile);
        mainWindow->addShortcut("Ctrl+W", &Presenter::closeFile);

        // 편집
        mainWindow->addShortcut("Ctrl+Z", &Presenter::undo);
        mainWindow->addShortcut("Ctrl+Y", &Presenter::redo);
        mainWindow->addShortcut("Delete", &Presenter::deleteSelected);

        // 선택
        mainWindow->addShortcut("Ctrl+A", &Presenter::selectAll);
        mainWindow->addShortcut("Ctrl+D", &Presenter::deselectAll);
        mainWindow->addShortcut("Ctrl+I", &Presenter::invertSelection);

        // 그룹
        mainWindow->addShortcut("Ctrl+G", &Presenter::createGroup);

        // 뷰
        mainWindow->addShortcut("F5", &Presenter::resetView);
        mainWindow->addShortcut("1", &Presenter::viewFront);
        mainWindow->addShortcut("2", &Presenter::viewTop);
        mainWindow->addShortcut("3", &Presenter::viewRight);

        // 기타
        mainWindow->addShortcut("Escape", &Presenter::clearSelection);
        mainWindow->addShortcut("F11", &Presenter::toggleFullscreen);
    }
};
```

## 스타일링

### Qt Style Sheet (QSS)

```css
/* resources/styles/dark_theme.qss */

QMainWindow {
    background-color: #2b2b2b;
    color: #d4d4d4;
}

QToolBar {
    background-color: #3c3c3c;
    border: none;
    spacing: 3px;
    padding: 5px;
}

QDockWidget {
    background-color: #2b2b2b;
    titlebar-close-icon: url(:/icons/close.png);
    titlebar-normal-icon: url(:/icons/undock.png);
}

QPushButton {
    background-color: #0e639c;
    border: none;
    border-radius: 3px;
    padding: 5px 15px;
    color: white;
}

QPushButton:hover {
    background-color: #1177bb;
}

QPushButton:pressed {
    background-color: #0d5a8f;
}

QTreeView, QListWidget, QTableWidget {
    background-color: #252526;
    border: 1px solid #3c3c3c;
    color: #d4d4d4;
}

QTreeView::item:selected,
QListWidget::item:selected,
QTableWidget::item:selected {
    background-color: #0e639c;
}
```

### 적용

```cpp
// MainWindow.cpp
QFile styleFile(":/styles/dark_theme.qss");
styleFile.open(QFile::ReadOnly);
QString styleSheet = QLatin1String(styleFile.readAll());
qApp->setStyleSheet(styleSheet);
```

## 사용 예제

### 메인 윈도우 생성

```cpp
#include "ui/MainWindow.h"
#include "core/Presenter.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 모델 생성
    Mesh mesh;

    // 뷰 생성
    MainWindow mainWindow;

    // Presenter (MVP 패턴)
    Presenter presenter(&mesh, &mainWindow);

    mainWindow.show();

    return app.exec();
}
```

### 동적 업데이트

```cpp
// 파일 로딩 시
void Presenter::onFileOpen(const QString& filepath) {
    // 진행률 대화상자 표시
    ProgressDialog progress(m_view);
    progress.show();

    // 비동기 로딩
    AsyncFileLoader loader;
    auto future = loader.loadAsync(filepath.toStdString(), *m_model,
        [&progress](float p) {
            progress.setProgress(p);
            progress.setStatus(QString("Loading... %1%").arg(p * 100, 0, 'f', 1));
        }
    );

    // 완료 대기
    future.wait();

    // UI 업데이트
    m_view->updateViewport();
    m_view->partsPanel()->setMesh(m_model);
    m_view->statusBar()->showMessage(
        QString("Loaded %1 elements").arg(m_model->elementCount())
    );
}
```

## 반응형 UI

### 윈도우 크기 변경

```cpp
void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    // VTK 뷰포트 크기 조정
    m_vtkWidget->update();

    // 패널 크기 조정
    if (width() < 800) {
        // 좁은 화면: 패널 숨김
        m_partsPanel->hide();
        m_groupsPanel->hide();
    } else {
        m_partsPanel->show();
        m_groupsPanel->show();
    }
}
```

## 테스트
- `tests/unit/test_main_window.cpp`
- `tests/integration/test_ui_workflow.cpp`
