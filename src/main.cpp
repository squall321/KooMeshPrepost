/**
 * @file main.cpp
 * @brief KooMeshPrepost 메인 엔트리 포인트
 *
 * 고성능 Mesh 가시화 및 전후처리 도구
 */

#include <iostream>
#include <cstdlib>

#ifdef KOOMESH_HAS_QT
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#endif

// 버전 정보
#define KOOMESH_VERSION_MAJOR 1
#define KOOMESH_VERSION_MINOR 0
#define KOOMESH_VERSION_PATCH 0
#define KOOMESH_VERSION_STRING "1.0.0"

/**
 * @brief 프로그램 정보 출력
 */
void printVersion() {
    std::cout << "KooMeshPrepost v" << KOOMESH_VERSION_STRING << "\n";
    std::cout << "High-Performance Mesh Visualization and Preprocessing Tool\n";
    std::cout << "Copyright (c) 2024 Koo Engineering\n";
    std::cout << "\n";

    std::cout << "Build configuration:\n";
#ifdef KOOMESH_HAS_QT
    std::cout << "  Qt: Enabled\n";
#else
    std::cout << "  Qt: Disabled\n";
#endif

#ifdef KOOMESH_HAS_VTK
    std::cout << "  VTK: Enabled\n";
#else
    std::cout << "  VTK: Disabled\n";
#endif

#ifdef KOOMESH_HAS_TBB
    std::cout << "  TBB: Enabled\n";
#else
    std::cout << "  TBB: Disabled\n";
#endif

#ifdef KOOMESH_HAS_BOOST
    std::cout << "  Boost: Enabled\n";
#else
    std::cout << "  Boost: Disabled\n";
#endif

    std::cout << "\n";
}

/**
 * @brief 사용법 출력
 */
void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options] [file]\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help        Show this help message\n";
    std::cout << "  -v, --version     Show version information\n";
    std::cout << "  -f, --file <path> Input mesh file (LS-DYNA Keyword format)\n";
    std::cout << "  --no-gui          Run without GUI (command-line mode)\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " mesh.k            # Open mesh.k in GUI\n";
    std::cout << "  " << programName << " --no-gui mesh.k   # Process mesh.k in CLI mode\n";
    std::cout << "\n";
}

#ifdef KOOMESH_HAS_QT
/**
 * @brief GUI 모드로 실행
 */
int runGUI(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("KooMeshPrepost");
    QApplication::setApplicationVersion(KOOMESH_VERSION_STRING);
    QApplication::setOrganizationName("Koo Engineering");

    // 커맨드라인 파서
    QCommandLineParser parser;
    parser.setApplicationDescription("High-Performance Mesh Visualization Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption fileOption(QStringList() << "f" << "file",
        "Input mesh file", "file");
    parser.addOption(fileOption);

    parser.process(app);

    std::cout << "Starting KooMeshPrepost GUI...\n";

    // TODO: MainWindow 생성 및 표시
    // MainWindow mainWindow;
    // if (parser.isSet(fileOption)) {
    //     mainWindow.loadFile(parser.value(fileOption));
    // }
    // mainWindow.show();

    std::cout << "GUI mode is not yet implemented.\n";
    std::cout << "Please build the full application to use GUI features.\n";

    return EXIT_SUCCESS;
    // return app.exec();
}
#endif

/**
 * @brief CLI 모드로 실행
 */
int runCLI(const char* inputFile) {
    std::cout << "Running in command-line mode...\n";

    if (inputFile) {
        std::cout << "Input file: " << inputFile << "\n";

        // TODO: 파일 로딩 및 처리
        // core::Mesh mesh;
        // io::DynaFileReader reader;
        // if (reader.read(inputFile, mesh)) {
        //     std::cout << "Successfully loaded mesh\n";
        //     std::cout << "Nodes: " << mesh.nodeCount() << "\n";
        //     std::cout << "Elements: " << mesh.elementCount() << "\n";
        // } else {
        //     std::cerr << "Failed to load mesh\n";
        //     return EXIT_FAILURE;
        // }

        std::cout << "CLI processing is not yet implemented.\n";
        std::cout << "This is a placeholder for future functionality.\n";
    } else {
        std::cerr << "Error: No input file specified\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief 메인 함수
 */
int main(int argc, char* argv[]) {
    // 버전 또는 도움말 요청 확인
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v" || arg == "--version") {
            printVersion();
            return EXIT_SUCCESS;
        }
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        }
    }

    // GUI 모드 확인
    bool useGUI = true;
    const char* inputFile = nullptr;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-gui") {
            useGUI = false;
        } else if (arg == "-f" || arg == "--file") {
            if (i + 1 < argc) {
                inputFile = argv[i + 1];
                ++i;
            }
        } else if (arg[0] != '-') {
            inputFile = argv[i];
        }
    }

#ifdef KOOMESH_HAS_QT
    if (useGUI) {
        return runGUI(argc, argv);
    }
#else
    if (useGUI) {
        std::cerr << "Error: GUI support not available (Qt not found during build)\n";
        std::cerr << "Please rebuild with Qt support or use --no-gui flag\n";
        return EXIT_FAILURE;
    }
#endif

    return runCLI(inputFile);
}
