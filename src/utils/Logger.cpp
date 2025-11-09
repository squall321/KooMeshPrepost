#include "utils/Logger.h"
#include <iostream>
#include <ctime>

namespace koomesh {
namespace utils {

// ======================================================================
// Singleton 인스턴스
// ======================================================================

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

// ======================================================================
// 생성자 및 소멸자
// ======================================================================

Logger::Logger()
    : m_level(LogLevel::INFO)
    , m_consoleOutput(true)
    , m_timestampFormat("%Y-%m-%d %H:%M:%S")
    , m_pattern("[{timestamp}] [{level}] {message}")
{
}

Logger::~Logger() {
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
}

// ======================================================================
// 설정 메서드
// ======================================================================

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

LogLevel Logger::getLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

bool Logger::setOutputFile(const std::string& filepath, bool append) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 기존 파일 스트림 닫기
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }

    // 새 파일 열기
    auto mode = append ? (std::ios::out | std::ios::app) : std::ios::out;
    m_fileStream.open(filepath, mode);

    return m_fileStream.is_open();
}

void Logger::setConsoleOutput(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consoleOutput = enabled;
}

bool Logger::isConsoleOutputEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_consoleOutput;
}

void Logger::setTimestampFormat(const std::string& format) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timestampFormat = format;
}

void Logger::setPattern(const std::string& pattern) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pattern = pattern;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

// ======================================================================
// 정적 로깅 메서드
// ======================================================================

void Logger::debug(const std::string& message) {
    getInstance().log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    getInstance().log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    getInstance().log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    getInstance().log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    getInstance().log(LogLevel::CRITICAL, message);
}

// ======================================================================
// 로그 메시지 처리
// ======================================================================

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 레벨 필터링
    if (level < m_level) {
        return;
    }

    // 메시지 포맷팅
    std::string formattedMessage = formatMessage(level, message);

    // 콘솔 출력
    if (m_consoleOutput) {
        if (level >= LogLevel::ERROR) {
            // 에러는 stderr로
            std::cerr << getColorCode(level) << formattedMessage << "\033[0m" << std::endl;
        } else {
            // 일반 로그는 stdout으로
            std::cout << getColorCode(level) << formattedMessage << "\033[0m" << std::endl;
        }
    }

    // 파일 출력 (색상 코드 없이)
    if (m_fileStream.is_open()) {
        m_fileStream << formattedMessage << std::endl;
    }
}

// ======================================================================
// Private 헬퍼 메서드
// ======================================================================

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

std::string Logger::getColorCode(LogLevel level) const {
    // ANSI 색상 코드
    switch (level) {
        case LogLevel::DEBUG:    return "\033[36m";      // Cyan
        case LogLevel::INFO:     return "\033[32m";      // Green
        case LogLevel::WARNING:  return "\033[33m";      // Yellow
        case LogLevel::ERROR:    return "\033[31m";      // Red
        case LogLevel::CRITICAL: return "\033[1;31m";    // Bold Red
        default:                 return "\033[0m";       // Reset
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &timeT);
#else
    localtime_r(&timeT, &localTime);
#endif

    char buffer[100];
    std::strftime(buffer, sizeof(buffer), m_timestampFormat.c_str(), &localTime);

    // 밀리초 추가
    std::ostringstream oss;
    oss << buffer << "." << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) const {
    std::string result = m_pattern;

    // {timestamp} 치환
    size_t pos = result.find("{timestamp}");
    if (pos != std::string::npos) {
        result.replace(pos, 11, getCurrentTimestamp());
    }

    // {level} 치환
    pos = result.find("{level}");
    if (pos != std::string::npos) {
        std::string levelStr = levelToString(level);
        // 레벨 문자열을 고정 너비로 (정렬을 위해)
        levelStr.resize(8, ' ');
        result.replace(pos, 7, levelStr);
    }

    // {message} 치환
    pos = result.find("{message}");
    if (pos != std::string::npos) {
        result.replace(pos, 9, message);
    }

    return result;
}

// ======================================================================
// 재귀 종료
// ======================================================================

void Logger::formatStringImpl(std::ostringstream& oss, const std::string& format, size_t pos) {
    oss << format.substr(pos);
}

} // namespace utils
} // namespace koomesh
