/**
 * @file main.cpp
 * @brief Logger 사용 예제
 *
 * 다양한 로깅 기능을 시연합니다.
 */

#include "utils/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace koomesh::utils;

/**
 * @brief 기본 로깅 예제
 */
void basicLoggingExample() {
    std::cout << "\n=== 기본 로깅 예제 ===" << std::endl;

    Logger::debug("디버그 메시지입니다");
    Logger::info("정보 메시지입니다");
    Logger::warning("경고 메시지입니다");
    Logger::error("오류 메시지입니다");
    Logger::critical("심각한 오류 메시지입니다");
}

/**
 * @brief 포맷된 로깅 예제
 */
void formattedLoggingExample() {
    std::cout << "\n=== 포맷된 로깅 예제 ===" << std::endl;

    std::string filename = "mesh.k";
    int nodeCount = 10542;
    int elementCount = 8231;
    double loadTime = 1.234;

    Logger::infof("Loading file: {}", filename);
    Logger::infof("Loaded {} nodes and {} elements in {:.3f} seconds",
                  nodeCount, elementCount, loadTime);

    Logger::debugf("Memory usage: {} MB", 256);
    Logger::warningf("Large file detected: {} elements", elementCount);
}

/**
 * @brief 로그 레벨 필터링 예제
 */
void levelFilteringExample() {
    std::cout << "\n=== 로그 레벨 필터링 예제 ===" << std::endl;

    Logger::getInstance().setLevel(LogLevel::WARNING);
    std::cout << "로그 레벨을 WARNING으로 설정 (DEBUG, INFO는 출력 안 됨)" << std::endl;

    Logger::debug("이 메시지는 보이지 않습니다");
    Logger::info("이 메시지도 보이지 않습니다");
    Logger::warning("이 메시지는 보입니다");
    Logger::error("이 메시지도 보입니다");

    // 레벨 복원
    Logger::getInstance().setLevel(LogLevel::DEBUG);
}

/**
 * @brief 파일 로깅 예제
 */
void fileLoggingExample() {
    std::cout << "\n=== 파일 로깅 예제 ===" << std::endl;

    // 파일 출력 설정
    if (Logger::getInstance().setOutputFile("application.log", false)) {
        std::cout << "로그 파일 'application.log'에 기록합니다" << std::endl;

        Logger::info("애플리케이션 시작");
        Logger::info("설정 파일 로딩 중...");
        Logger::info("데이터베이스 연결 중...");
        Logger::warning("일부 설정이 누락되었습니다");
        Logger::info("애플리케이션 준비 완료");

        Logger::getInstance().flush();
        std::cout << "로그가 'application.log' 파일에 저장되었습니다" << std::endl;
    } else {
        std::cerr << "로그 파일을 열 수 없습니다" << std::endl;
    }
}

/**
 * @brief 커스텀 포맷 예제
 */
void customFormatExample() {
    std::cout << "\n=== 커스텀 포맷 예제 ===" << std::endl;

    // 기본 패턴 저장
    auto& logger = Logger::getInstance();

    // 간단한 포맷
    logger.setPattern("{level}: {message}");
    Logger::info("간단한 포맷입니다");

    // JSON 스타일 포맷
    logger.setPattern("{{\"timestamp\":\"{timestamp}\",\"level\":\"{level}\",\"message\":\"{message}\"}}");
    Logger::info("JSON 스타일 로그입니다");

    // 기본 포맷으로 복원
    logger.setPattern("[{timestamp}] [{level}] {message}");
}

/**
 * @brief 실제 애플리케이션 시뮬레이션
 */
void applicationSimulation() {
    std::cout << "\n=== 실제 애플리케이션 시뮬레이션 ===" << std::endl;

    Logger::info("========================================");
    Logger::info("KooMeshPrepost v1.0.0 Starting...");
    Logger::info("========================================");

    // 초기화 단계
    Logger::debug("Initializing configuration manager");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Logger::info("Loading configuration from 'config.json'");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Logger::debug("Configuration loaded successfully");

    // 파일 로딩 시뮬레이션
    Logger::info("Opening mesh file: 'car_chassis.k'");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    Logger::infof("Parsing mesh file... ({} KB)", 15234);

    // 진행률 시뮬레이션
    for (int progress = 25; progress <= 100; progress += 25) {
        Logger::debugf("Loading progress: {}%", progress);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Logger::infof("Mesh loaded successfully: {} nodes, {} elements",
                  123456, 98765);

    // 공간 인덱스 구축
    Logger::info("Building spatial index (Octree)...");
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    Logger::info("Spatial index built in 0.145 seconds");

    // 경고 시뮬레이션
    Logger::warning("Some elements have poor quality (< 0.3)");

    // UI 초기화
    Logger::debug("Initializing Qt application");
    Logger::debug("Initializing VTK renderer");
    Logger::info("User interface ready");

    Logger::info("========================================");
    Logger::info("Application started successfully");
    Logger::info("========================================");
}

/**
 * @brief 멀티스레드 로깅 예제
 */
void multithreadedLoggingExample() {
    std::cout << "\n=== 멀티스레드 로깅 예제 ===" << std::endl;

    const int numThreads = 5;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < 3; ++j) {
                Logger::infof("Thread {} - Message {}", i, j);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "모든 스레드에서 안전하게 로그가 기록되었습니다" << std::endl;
}

/**
 * @brief 오류 처리 예제
 */
void errorHandlingExample() {
    std::cout << "\n=== 오류 처리 예제 ===" << std::endl;

    try {
        Logger::info("파일 열기 시도: 'nonexistent.k'");

        // 파일이 없다고 가정
        throw std::runtime_error("File not found: nonexistent.k");

    } catch (const std::exception& e) {
        Logger::errorf("오류 발생: {}", e.what());
        Logger::warning("기본 파일을 로드합니다");
    }

    try {
        Logger::info("메모리 할당 중...");

        // 메모리 부족 시뮬레이션
        throw std::bad_alloc();

    } catch (const std::bad_alloc&) {
        Logger::critical("메모리 부족! 애플리케이션을 종료합니다");
    }
}

int main() {
    // 콘솔 출력 활성화
    Logger::getInstance().setConsoleOutput(true);
    Logger::getInstance().setLevel(LogLevel::DEBUG);

    std::cout << "╔═══════════════════════════════════════╗" << std::endl;
    std::cout << "║   KooMeshPrepost Logger 예제         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════╝" << std::endl;

    basicLoggingExample();
    formattedLoggingExample();
    levelFilteringExample();
    fileLoggingExample();
    customFormatExample();
    applicationSimulation();
    multithreadedLoggingExample();
    errorHandlingExample();

    std::cout << "\n모든 예제가 완료되었습니다!" << std::endl;
    std::cout << "로그 파일 'application.log'를 확인해보세요." << std::endl;

    return 0;
}
