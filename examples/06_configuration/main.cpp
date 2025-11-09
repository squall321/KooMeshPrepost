/**
 * @file main.cpp
 * @brief ConfigManager 사용 예제
 *
 * ConfigManager의 다양한 기능을 시연합니다:
 * 1. 기본 설정 읽기/쓰기
 * 2. 파일 저장 및 로드
 * 3. 기본값 처리
 * 4. 배열 사용
 * 5. 자동 저장 기능
 * 6. 키 검색 및 관리
 */

#include "utils/ConfigManager.h"
#include "utils/Logger.h"
#include <iostream>
#include <iomanip>

using namespace koomesh::utils;

// ======================================================================
// 예제 헬퍼 함수
// ======================================================================

void printSeparator(const std::string& title) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
}

// ======================================================================
// 예제 1: 기본 설정 읽기/쓰기
// ======================================================================

void example1_BasicUsage() {
    printSeparator("Example 1: 기본 설정 읽기/쓰기");

    ConfigManager& config = ConfigManager::getInstance();

    // 다양한 타입의 설정값 설정
    config.setString("app.name", "KooMeshPrepost");
    config.setString("app.version", "1.0.0");
    config.setInt("ui.window.width", 1280);
    config.setInt("ui.window.height", 720);
    config.setDouble("renderer.fov", 45.0);
    config.setBool("debug.enabled", false);
    config.setBool("ui.show_toolbar", true);

    // 설정값 읽기
    std::cout << "Application Name: " << config.getString("app.name") << "\n";
    std::cout << "Version: " << config.getString("app.version") << "\n";
    std::cout << "Window Size: " << config.getInt("ui.window.width")
              << "x" << config.getInt("ui.window.height") << "\n";
    std::cout << "Field of View: " << config.getDouble("renderer.fov") << "°\n";
    std::cout << "Debug Mode: " << (config.getBool("debug.enabled") ? "ON" : "OFF") << "\n";
    std::cout << "Show Toolbar: " << (config.getBool("ui.show_toolbar") ? "YES" : "NO") << "\n";
}

// ======================================================================
// 예제 2: 파일 저장 및 로드
// ======================================================================

void example2_FileIO() {
    printSeparator("Example 2: 파일 저장 및 로드");

    ConfigManager& config = ConfigManager::getInstance();
    config.clear();

    // 설정값 설정
    config.setString("app.name", "KooMeshPrepost");
    config.setInt("ui.window.width", 1024);
    config.setInt("ui.window.height", 768);
    config.setDouble("renderer.background.r", 0.2);
    config.setDouble("renderer.background.g", 0.3);
    config.setDouble("renderer.background.b", 0.4);
    config.setBool("ui.maximized", false);

    // 파일로 저장
    std::string configFile = "app_config.json";
    if (config.saveToFile(configFile)) {
        std::cout << "✓ 설정을 " << configFile << " 파일로 저장했습니다.\n";
    } else {
        std::cout << "✗ 설정 저장에 실패했습니다.\n";
        return;
    }

    // 설정 초기화
    config.clear();
    std::cout << "설정이 초기화되었습니다.\n";

    // 파일에서 로드
    if (config.loadFromFile(configFile)) {
        std::cout << "✓ " << configFile << " 파일에서 설정을 로드했습니다.\n";
    } else {
        std::cout << "✗ 설정 로드에 실패했습니다.\n";
        return;
    }

    // 로드된 설정 확인
    std::cout << "\n로드된 설정:\n";
    std::cout << "  Application: " << config.getString("app.name") << "\n";
    std::cout << "  Window Size: " << config.getInt("ui.window.width")
              << "x" << config.getInt("ui.window.height") << "\n";
    std::cout << "  Background Color: RGB("
              << config.getDouble("renderer.background.r") << ", "
              << config.getDouble("renderer.background.g") << ", "
              << config.getDouble("renderer.background.b") << ")\n";
    std::cout << "  Maximized: " << (config.getBool("ui.maximized") ? "YES" : "NO") << "\n";

    // 정리
    std::remove(configFile.c_str());
}

// ======================================================================
// 예제 3: 기본값 처리
// ======================================================================

void example3_DefaultValues() {
    printSeparator("Example 3: 기본값 처리");

    ConfigManager& config = ConfigManager::getInstance();
    config.clear();

    // 존재하지 않는 키에 대해 기본값 사용
    std::string theme = config.getString("ui.theme", "dark");
    int fontSize = config.getInt("ui.font_size", 12);
    double lineWidth = config.getDouble("renderer.line_width", 1.5);
    bool antialiasing = config.getBool("renderer.antialiasing", true);

    std::cout << "기본값으로 로드된 설정:\n";
    std::cout << "  Theme: " << theme << " (default)\n";
    std::cout << "  Font Size: " << fontSize << "pt (default)\n";
    std::cout << "  Line Width: " << lineWidth << "px (default)\n";
    std::cout << "  Antialiasing: " << (antialiasing ? "ON" : "OFF") << " (default)\n";

    // 실제 설정값 설정
    config.setString("ui.theme", "light");
    config.setInt("ui.font_size", 14);

    // 다시 읽기 (이제 설정된 값 사용)
    theme = config.getString("ui.theme", "dark");
    fontSize = config.getInt("ui.font_size", 12);
    lineWidth = config.getDouble("renderer.line_width", 1.5);

    std::cout << "\n설정값으로 로드된 설정:\n";
    std::cout << "  Theme: " << theme << " (configured)\n";
    std::cout << "  Font Size: " << fontSize << "pt (configured)\n";
    std::cout << "  Line Width: " << lineWidth << "px (still default)\n";
}

// ======================================================================
// 예제 4: 배열 사용
// ======================================================================

void example4_Arrays() {
    printSeparator("Example 4: 배열 사용");

    ConfigManager& config = ConfigManager::getInstance();
    config.clear();

    // 최근 파일 목록
    std::vector<std::string> recentFiles = {
        "/path/to/mesh1.k",
        "/path/to/mesh2.k",
        "/path/to/mesh3.k",
        "/path/to/mesh4.k"
    };

    config.setArray("ui.recent_files", recentFiles);

    // 플러그인 목록
    std::vector<std::string> plugins = {
        "ExportToVTK",
        "MeshQualityChecker",
        "AutoMesher"
    };

    config.setArray("app.plugins", plugins);

    // 배열 읽기
    std::cout << "최근 파일:\n";
    auto loadedRecentFiles = config.getArray("ui.recent_files");
    for (size_t i = 0; i < loadedRecentFiles.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << loadedRecentFiles[i] << "\n";
    }

    std::cout << "\n로드된 플러그인:\n";
    auto loadedPlugins = config.getArray("app.plugins");
    for (const auto& plugin : loadedPlugins) {
        std::cout << "  - " << plugin << "\n";
    }

    // 빈 배열 기본값
    auto emptyArray = config.getArray("nonexistent.array", {"default1", "default2"});
    std::cout << "\n존재하지 않는 배열 (기본값):\n";
    for (const auto& item : emptyArray) {
        std::cout << "  - " << item << "\n";
    }
}

// ======================================================================
// 예제 5: 자동 저장 기능
// ======================================================================

void example5_AutoSave() {
    printSeparator("Example 5: 자동 저장 기능");

    ConfigManager& config = ConfigManager::getInstance();
    config.clear();

    std::string configFile = "autosave_config.json";

    // 자동 저장 활성화
    config.setConfigFilePath(configFile);
    config.setAutoSave(true);

    std::cout << "자동 저장이 활성화되었습니다.\n";
    std::cout << "설정 파일 경로: " << config.getConfigFilePath() << "\n\n";

    // 설정값 변경 (각 변경마다 자동으로 파일에 저장됨)
    std::cout << "설정값 변경 중...\n";
    config.setString("app.name", "KooMeshPrepost");
    std::cout << "  - app.name 설정 완료 (자동 저장됨)\n";

    config.setInt("ui.window.width", 1920);
    std::cout << "  - ui.window.width 설정 완료 (자동 저장됨)\n";

    config.setBool("debug.enabled", true);
    std::cout << "  - debug.enabled 설정 완료 (자동 저장됨)\n";

    // 자동 저장 비활성화
    config.setAutoSave(false);
    std::cout << "\n자동 저장이 비활성화되었습니다.\n";

    config.setInt("ui.window.height", 1080);
    std::cout << "  - ui.window.height 설정 완료 (자동 저장되지 않음)\n";

    // 수동 저장
    config.saveToFile(configFile);
    std::cout << "\n수동으로 설정을 저장했습니다.\n";

    // 정리
    std::remove(configFile.c_str());
}

// ======================================================================
// 예제 6: 키 검색 및 관리
// ======================================================================

void example6_KeyManagement() {
    printSeparator("Example 6: 키 검색 및 관리");

    ConfigManager& config = ConfigManager::getInstance();
    config.clear();

    // 여러 설정값 추가
    config.setString("ui.window.title", "KooMesh");
    config.setInt("ui.window.width", 1024);
    config.setInt("ui.window.height", 768);
    config.setInt("ui.toolbar.height", 32);
    config.setBool("ui.toolbar.visible", true);
    config.setDouble("renderer.fov", 45.0);
    config.setDouble("renderer.near_plane", 0.1);
    config.setDouble("renderer.far_plane", 1000.0);
    config.setBool("debug.enabled", false);
    config.setBool("debug.show_fps", true);

    // 모든 키 목록
    std::cout << "모든 설정 키:\n";
    auto allKeys = config.getAllKeys();
    for (const auto& key : allKeys) {
        std::cout << "  - " << key << "\n";
    }

    // UI 관련 키만 검색
    std::cout << "\nUI 관련 설정 (ui.):\n";
    auto uiKeys = config.getKeysWithPrefix("ui.");
    for (const auto& key : uiKeys) {
        std::cout << "  - " << key << "\n";
    }

    // Window 관련 키만 검색
    std::cout << "\nWindow 관련 설정 (ui.window.):\n";
    auto windowKeys = config.getKeysWithPrefix("ui.window.");
    for (const auto& key : windowKeys) {
        std::cout << "  - " << key << "\n";
    }

    // 키 존재 여부 확인
    std::cout << "\n키 존재 여부 확인:\n";
    std::cout << "  'ui.window.title' 존재? "
              << (config.hasKey("ui.window.title") ? "YES" : "NO") << "\n";
    std::cout << "  'nonexistent.key' 존재? "
              << (config.hasKey("nonexistent.key") ? "YES" : "NO") << "\n";

    // 키 삭제
    std::cout << "\n'debug.show_fps' 키 삭제...\n";
    if (config.removeKey("debug.show_fps")) {
        std::cout << "  ✓ 키가 삭제되었습니다.\n";
    }

    std::cout << "  'debug.show_fps' 존재? "
              << (config.hasKey("debug.show_fps") ? "YES" : "NO") << "\n";
}

// ======================================================================
// 예제 7: 실제 애플리케이션 설정 시뮬레이션
// ======================================================================

void example7_RealWorldScenario() {
    printSeparator("Example 7: 실제 애플리케이션 설정 시뮬레이션");

    ConfigManager& config = ConfigManager::getInstance();
    config.clear();

    std::string configFile = "koomesh_config.json";

    // 애플리케이션 시작 시 설정 로드
    std::cout << "애플리케이션 시작...\n";

    if (config.loadFromFile(configFile)) {
        std::cout << "  ✓ 기존 설정을 로드했습니다.\n";
    } else {
        std::cout << "  ! 설정 파일이 없습니다. 기본 설정을 사용합니다.\n";

        // 기본 설정 적용
        config.setString("app.name", "KooMeshPrepost");
        config.setString("app.version", "1.0.0");
        config.setInt("ui.window.width", 1280);
        config.setInt("ui.window.height", 720);
        config.setBool("ui.maximized", false);
        config.setString("ui.theme", "dark");
        config.setDouble("renderer.fov", 45.0);
        config.setDouble("renderer.line_width", 1.0);
        config.setBool("renderer.antialiasing", true);
        config.setBool("debug.enabled", false);

        std::vector<std::string> emptyRecentFiles;
        config.setArray("ui.recent_files", emptyRecentFiles);
    }

    // 현재 설정 출력
    std::cout << "\n현재 설정:\n";
    std::cout << "  Application: " << config.getString("app.name", "Unknown")
              << " v" << config.getString("app.version", "0.0.0") << "\n";
    std::cout << "  Window: " << config.getInt("ui.window.width", 800)
              << "x" << config.getInt("ui.window.height", 600) << "\n";
    std::cout << "  Theme: " << config.getString("ui.theme", "light") << "\n";
    std::cout << "  FOV: " << config.getDouble("renderer.fov", 45.0) << "°\n";

    // 사용자가 설정 변경
    std::cout << "\n사용자가 설정을 변경합니다...\n";
    config.setInt("ui.window.width", 1920);
    config.setInt("ui.window.height", 1080);
    config.setString("ui.theme", "light");

    // 최근 파일 추가
    std::vector<std::string> recentFiles = {
        "/home/user/project/mesh1.k",
        "/home/user/project/mesh2.k"
    };
    config.setArray("ui.recent_files", recentFiles);

    std::cout << "  - 창 크기 변경: 1920x1080\n";
    std::cout << "  - 테마 변경: light\n";
    std::cout << "  - 최근 파일 업데이트: 2개\n";

    // 애플리케이션 종료 시 설정 저장
    std::cout << "\n애플리케이션 종료...\n";
    if (config.saveToFile(configFile)) {
        std::cout << "  ✓ 설정을 저장했습니다.\n";
    }

    // 정리
    std::remove(configFile.c_str());
}

// ======================================================================
// 예제 8: Logger와 통합
// ======================================================================

void example8_WithLogger() {
    printSeparator("Example 8: Logger와 통합");

    // Logger 초기화
    Logger::getInstance().setLevel(LogLevel::INFO);
    Logger::getInstance().setConsoleOutput(true);

    ConfigManager& config = ConfigManager::getInstance();
    config.clear();

    std::string configFile = "app_with_log.json";

    // 설정 로드 로깅
    Logger::info("애플리케이션 설정 로드 중...");

    if (config.loadFromFile(configFile)) {
        Logger::info("설정 파일을 로드했습니다: " + configFile);
    } else {
        Logger::warning("설정 파일을 찾을 수 없습니다. 기본 설정을 사용합니다.");

        // 기본 설정
        config.setString("app.name", "KooMeshPrepost");
        config.setInt("ui.window.width", 1024);
        config.setInt("ui.window.height", 768);

        Logger::info("기본 설정이 적용되었습니다.");
    }

    // 설정값 로깅
    Logger::infof("Window size: {}x{}",
                  config.getInt("ui.window.width", 800),
                  config.getInt("ui.window.height", 600));

    // 설정 변경 로깅
    Logger::info("사용자가 창 크기를 변경했습니다.");
    config.setInt("ui.window.width", 1920);
    config.setInt("ui.window.height", 1080);

    Logger::infof("새로운 창 크기: {}x{}",
                  config.getInt("ui.window.width"),
                  config.getInt("ui.window.height"));

    // 설정 저장 로깅
    Logger::info("설정을 저장하는 중...");
    if (config.saveToFile(configFile)) {
        Logger::info("설정이 성공적으로 저장되었습니다: " + configFile);
    } else {
        Logger::error("설정 저장에 실패했습니다!");
    }

    // 정리
    std::remove(configFile.c_str());
}

// ======================================================================
// 메인 함수
// ======================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  ConfigManager Usage Examples\n";
    std::cout << "========================================\n";

    try {
        example1_BasicUsage();
        example2_FileIO();
        example3_DefaultValues();
        example4_Arrays();
        example5_AutoSave();
        example6_KeyManagement();
        example7_RealWorldScenario();
        example8_WithLogger();

        printSeparator("모든 예제 완료");
        std::cout << "✓ 모든 예제가 성공적으로 실행되었습니다.\n";

    } catch (const std::exception& e) {
        std::cerr << "✗ 오류 발생: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
