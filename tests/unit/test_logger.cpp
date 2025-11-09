#include <gtest/gtest.h>
#include "utils/Logger.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

using namespace koomesh::utils;

/**
 * @brief Logger 테스트 픽스처
 */
class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 각 테스트 전에 Logger 초기화
        Logger::getInstance().setLevel(LogLevel::DEBUG);
        Logger::getInstance().setConsoleOutput(false);  // 테스트 중 콘솔 출력 비활성화

        // 테스트용 로그 파일
        testLogFile = "test_log.txt";
    }

    void TearDown() override {
        // 테스트 후 정리
        std::remove(testLogFile.c_str());
    }

    /**
     * @brief 파일에서 로그 내용 읽기
     */
    std::string readLogFile(const std::string& filepath) {
        Logger::getInstance().flush();  // 버퍼 플러시
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    /**
     * @brief 로그 파일의 라인 수 세기
     */
    size_t countLines(const std::string& filepath) {
        Logger::getInstance().flush();
        std::ifstream file(filepath);
        size_t count = 0;
        std::string line;
        while (std::getline(file, line)) {
            ++count;
        }
        return count;
    }

    std::string testLogFile;
};

// ======================================================================
// 기본 기능 테스트
// ======================================================================

TEST_F(LoggerTest, Singleton) {
    // Logger는 싱글톤이어야 함
    Logger& logger1 = Logger::getInstance();
    Logger& logger2 = Logger::getInstance();

    EXPECT_EQ(&logger1, &logger2);
}

TEST_F(LoggerTest, LogLevelFiltering) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::getInstance().setLevel(LogLevel::WARNING);

    // WARNING 이상만 기록되어야 함
    Logger::debug("Debug message");      // 기록 안 됨
    Logger::info("Info message");        // 기록 안 됨
    Logger::warning("Warning message");  // 기록됨
    Logger::error("Error message");      // 기록됨
    Logger::critical("Critical message");// 기록됨

    EXPECT_EQ(countLines(testLogFile), 3);

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("Warning message") != std::string::npos);
    EXPECT_TRUE(content.find("Error message") != std::string::npos);
    EXPECT_TRUE(content.find("Critical message") != std::string::npos);
    EXPECT_TRUE(content.find("Debug message") == std::string::npos);
    EXPECT_TRUE(content.find("Info message") == std::string::npos);
}

TEST_F(LoggerTest, FileOutput) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::getInstance().setLevel(LogLevel::INFO);

    Logger::info("Test message 1");
    Logger::info("Test message 2");

    std::string content = readLogFile(testLogFile);

    EXPECT_TRUE(content.find("Test message 1") != std::string::npos);
    EXPECT_TRUE(content.find("Test message 2") != std::string::npos);
}

TEST_F(LoggerTest, AppendMode) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::info("First message");
    Logger::getInstance().flush();

    // 파일 닫기
    Logger::getInstance().setConsoleOutput(true);

    // 같은 파일에 append
    Logger::getInstance().setOutputFile(testLogFile, true);
    Logger::info("Second message");

    EXPECT_EQ(countLines(testLogFile), 2);
}

TEST_F(LoggerTest, OverwriteMode) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::info("First message");
    Logger::getInstance().flush();

    // 파일 덮어쓰기
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::info("Second message");

    EXPECT_EQ(countLines(testLogFile), 1);

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("Second message") != std::string::npos);
    EXPECT_TRUE(content.find("First message") == std::string::npos);
}

// ======================================================================
// 로그 레벨 테스트
// ======================================================================

TEST_F(LoggerTest, DebugLevel) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::debug("Debug message");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("DEBUG") != std::string::npos);
    EXPECT_TRUE(content.find("Debug message") != std::string::npos);
}

TEST_F(LoggerTest, InfoLevel) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::info("Info message");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("INFO") != std::string::npos);
    EXPECT_TRUE(content.find("Info message") != std::string::npos);
}

TEST_F(LoggerTest, WarningLevel) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::warning("Warning message");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("WARNING") != std::string::npos);
    EXPECT_TRUE(content.find("Warning message") != std::string::npos);
}

TEST_F(LoggerTest, ErrorLevel) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::error("Error message");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("ERROR") != std::string::npos);
    EXPECT_TRUE(content.find("Error message") != std::string::npos);
}

TEST_F(LoggerTest, CriticalLevel) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::critical("Critical message");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("CRITICAL") != std::string::npos);
    EXPECT_TRUE(content.find("Critical message") != std::string::npos);
}

// ======================================================================
// 포맷된 로깅 테스트
// ======================================================================

TEST_F(LoggerTest, FormattedLogging_SingleArg) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    Logger::infof("Loading file: {}", "test.txt");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("Loading file: test.txt") != std::string::npos);
}

TEST_F(LoggerTest, FormattedLogging_MultipleArgs) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    Logger::infof("Loaded {} nodes and {} elements", 1000, 500);

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("Loaded 1000 nodes and 500 elements") != std::string::npos);
}

TEST_F(LoggerTest, FormattedLogging_MixedTypes) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    Logger::infof("File: {}, size: {} bytes, ratio: {}", "mesh.k", 1024, 3.14);

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("File: mesh.k") != std::string::npos);
    EXPECT_TRUE(content.find("size: 1024 bytes") != std::string::npos);
    EXPECT_TRUE(content.find("ratio: 3.14") != std::string::npos);
}

TEST_F(LoggerTest, FormattedLogging_AllLevels) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    Logger::debugf("Debug: {}", 1);
    Logger::infof("Info: {}", 2);
    Logger::warningf("Warning: {}", 3);
    Logger::errorf("Error: {}", 4);
    Logger::criticalf("Critical: {}", 5);

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("Debug: 1") != std::string::npos);
    EXPECT_TRUE(content.find("Info: 2") != std::string::npos);
    EXPECT_TRUE(content.find("Warning: 3") != std::string::npos);
    EXPECT_TRUE(content.find("Error: 4") != std::string::npos);
    EXPECT_TRUE(content.find("Critical: 5") != std::string::npos);
}

// ======================================================================
// 스레드 안전성 테스트
// ======================================================================

TEST_F(LoggerTest, ThreadSafety) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    const int numThreads = 10;
    const int messagesPerThread = 100;

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    // 여러 스레드에서 동시에 로그
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, messagesPerThread]() {
            for (int j = 0; j < messagesPerThread; ++j) {
                Logger::infof("Thread {}, message {}", i, j);
            }
        });
    }

    // 모든 스레드 완료 대기
    for (auto& thread : threads) {
        thread.join();
    }

    // 모든 메시지가 기록되었는지 확인
    size_t expectedLines = numThreads * messagesPerThread;
    EXPECT_EQ(countLines(testLogFile), expectedLines);
}

// ======================================================================
// 설정 테스트
// ======================================================================

TEST_F(LoggerTest, CustomTimestampFormat) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::getInstance().setTimestampFormat("%Y/%m/%d");

    Logger::info("Test message");

    std::string content = readLogFile(testLogFile);
    // YYYY/MM/DD 포맷 확인 (정규식 대신 간단한 검사)
    EXPECT_TRUE(content.find("/") != std::string::npos);
}

TEST_F(LoggerTest, CustomPattern) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::getInstance().setPattern("{level}: {message}");

    Logger::info("Test message");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("INFO") != std::string::npos);
    EXPECT_TRUE(content.find("Test message") != std::string::npos);
    // 타임스탬프가 없어야 함
    EXPECT_TRUE(content.find("[") == std::string::npos);
}

TEST_F(LoggerTest, GetSetLevel) {
    Logger::getInstance().setLevel(LogLevel::WARNING);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::WARNING);

    Logger::getInstance().setLevel(LogLevel::DEBUG);
    EXPECT_EQ(Logger::getInstance().getLevel(), LogLevel::DEBUG);
}

TEST_F(LoggerTest, ConsoleOutputToggle) {
    Logger::getInstance().setConsoleOutput(true);
    EXPECT_TRUE(Logger::getInstance().isConsoleOutputEnabled());

    Logger::getInstance().setConsoleOutput(false);
    EXPECT_FALSE(Logger::getInstance().isConsoleOutputEnabled());
}

// ======================================================================
// 엣지 케이스 테스트
// ======================================================================

TEST_F(LoggerTest, EmptyMessage) {
    Logger::getInstance().setOutputFile(testLogFile, false);
    Logger::info("");

    EXPECT_EQ(countLines(testLogFile), 1);
}

TEST_F(LoggerTest, VeryLongMessage) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    std::string longMessage(10000, 'a');
    Logger::info(longMessage);

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find(longMessage) != std::string::npos);
}

TEST_F(LoggerTest, SpecialCharacters) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    Logger::info("Special: \n\t\"quotes\" 'apostrophe' \\backslash");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("Special:") != std::string::npos);
}

TEST_F(LoggerTest, NonAsciiCharacters) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    Logger::info("한글 테스트 메시지");
    Logger::info("日本語のテスト");

    std::string content = readLogFile(testLogFile);
    EXPECT_TRUE(content.find("한글") != std::string::npos);
    EXPECT_TRUE(content.find("日本語") != std::string::npos);
}

// ======================================================================
// 성능 테스트 (간단한 벤치마크)
// ======================================================================

TEST_F(LoggerTest, PerformanceBenchmark) {
    Logger::getInstance().setOutputFile(testLogFile, false);

    const int numMessages = 10000;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numMessages; ++i) {
        Logger::info("Performance test message");
    }
    Logger::getInstance().flush();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Logged " << numMessages << " messages in "
              << duration.count() << " ms" << std::endl;
    std::cout << "Throughput: " << (numMessages * 1000.0 / duration.count())
              << " messages/sec" << std::endl;

    // 성능이 극도로 나쁘지 않은지 확인 (10초 이내)
    EXPECT_LT(duration.count(), 10000);
}
