#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace koomesh {
namespace utils {

/**
 * @brief 로그 레벨
 */
enum class LogLevel {
    DEBUG = 0,      ///< 디버그 정보 (상세한 진단 정보)
    INFO = 1,       ///< 일반 정보 메시지
    WARNING = 2,    ///< 경고 (잠재적 문제)
    ERROR = 3,      ///< 오류 (기능 실패)
    CRITICAL = 4    ///< 심각한 오류 (프로그램 종료 가능)
};

/**
 * @brief Logger 클래스 - Singleton 패턴
 *
 * 스레드 안전한 로깅 시스템을 제공합니다.
 * 콘솔 및 파일 출력을 지원하며, 다양한 로그 레벨을 제공합니다.
 *
 * Example usage:
 * @code
 * Logger::info("Application started");
 * Logger::debug("Loading file: {}", filepath);
 * Logger::error("Failed to open file: {}", error);
 * @endcode
 *
 * @note 이 클래스는 스레드 안전합니다.
 */
class Logger {
public:
    /**
     * @brief Singleton 인스턴스 얻기
     * @return Logger 인스턴스 참조
     */
    static Logger& getInstance();

    // Singleton: 복사 및 이동 금지
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief 소멸자 - 파일 스트림 정리
     */
    ~Logger();

    // ======================================================================
    // 설정 메서드
    // ======================================================================

    /**
     * @brief 최소 로그 레벨 설정
     *
     * 이 레벨 이상의 로그만 출력됩니다.
     *
     * @param level 최소 로그 레벨
     */
    void setLevel(LogLevel level);

    /**
     * @brief 현재 로그 레벨 반환
     * @return 현재 로그 레벨
     */
    LogLevel getLevel() const;

    /**
     * @brief 로그 파일 경로 설정
     *
     * @param filepath 로그 파일 경로
     * @param append true이면 기존 파일에 추가, false이면 덮어쓰기
     * @return 성공 여부
     */
    bool setOutputFile(const std::string& filepath, bool append = true);

    /**
     * @brief 콘솔 출력 활성화/비활성화
     * @param enabled true이면 콘솔 출력, false이면 비활성화
     */
    void setConsoleOutput(bool enabled);

    /**
     * @brief 콘솔 출력 여부 반환
     * @return 콘솔 출력 활성화 여부
     */
    bool isConsoleOutputEnabled() const;

    /**
     * @brief 타임스탬프 포맷 설정
     *
     * @param format strftime 포맷 문자열
     *               기본값: "%Y-%m-%d %H:%M:%S"
     */
    void setTimestampFormat(const std::string& format);

    /**
     * @brief 로그 메시지 패턴 설정
     *
     * 사용 가능한 플레이스홀더:
     * - {timestamp}: 타임스탬프
     * - {level}: 로그 레벨
     * - {message}: 로그 메시지
     *
     * @param pattern 로그 패턴 문자열
     *                기본값: "[{timestamp}] [{level}] {message}"
     */
    void setPattern(const std::string& pattern);

    /**
     * @brief 모든 로그 즉시 플러시 (파일에 기록)
     */
    void flush();

    // ======================================================================
    // 로깅 메서드 (정적 - 편의 함수)
    // ======================================================================

    /**
     * @brief DEBUG 레벨 로그
     * @param message 로그 메시지
     */
    static void debug(const std::string& message);

    /**
     * @brief INFO 레벨 로그
     * @param message 로그 메시지
     */
    static void info(const std::string& message);

    /**
     * @brief WARNING 레벨 로그
     * @param message 로그 메시지
     */
    static void warning(const std::string& message);

    /**
     * @brief ERROR 레벨 로그
     * @param message 로그 메시지
     */
    static void error(const std::string& message);

    /**
     * @brief CRITICAL 레벨 로그
     * @param message 로그 메시지
     */
    static void critical(const std::string& message);

    // ======================================================================
    // 포맷된 로깅 (C++20 std::format 스타일)
    // ======================================================================

    /**
     * @brief 포맷된 DEBUG 로그
     *
     * Example:
     * @code
     * Logger::debugf("Loading file: {}, size: {} bytes", filepath, size);
     * @endcode
     *
     * @tparam Args 가변 인자 타입
     * @param format 포맷 문자열 (단순 {} 치환)
     * @param args 포맷 인자들
     */
    template<typename... Args>
    static void debugf(const std::string& format, Args&&... args);

    /**
     * @brief 포맷된 INFO 로그
     * @tparam Args 가변 인자 타입
     * @param format 포맷 문자열
     * @param args 포맷 인자들
     */
    template<typename... Args>
    static void infof(const std::string& format, Args&&... args);

    /**
     * @brief 포맷된 WARNING 로그
     * @tparam Args 가변 인자 타입
     * @param format 포맷 문자열
     * @param args 포맷 인자들
     */
    template<typename... Args>
    static void warningf(const std::string& format, Args&&... args);

    /**
     * @brief 포맷된 ERROR 로그
     * @tparam Args 가변 인자 타입
     * @param format 포맷 문자열
     * @param args 포맷 인자들
     */
    template<typename... Args>
    static void errorf(const std::string& format, Args&&... args);

    /**
     * @brief 포맷된 CRITICAL 로그
     * @tparam Args 가변 인자 타입
     * @param format 포맷 문자열
     * @param args 포맷 인자들
     */
    template<typename... Args>
    static void criticalf(const std::string& format, Args&&... args);

    // ======================================================================
    // 인스턴스 메서드
    // ======================================================================

    /**
     * @brief 로그 메시지 기록 (인스턴스 메서드)
     *
     * @param level 로그 레벨
     * @param message 로그 메시지
     */
    void log(LogLevel level, const std::string& message);

private:
    /**
     * @brief Private 생성자 (Singleton)
     */
    Logger();

    /**
     * @brief 로그 레벨을 문자열로 변환
     * @param level 로그 레벨
     * @return 로그 레벨 문자열
     */
    std::string levelToString(LogLevel level) const;

    /**
     * @brief ANSI 색상 코드 얻기 (콘솔 출력용)
     * @param level 로그 레벨
     * @return ANSI 색상 코드
     */
    std::string getColorCode(LogLevel level) const;

    /**
     * @brief 현재 타임스탬프 생성
     * @return 포맷된 타임스탬프 문자열
     */
    std::string getCurrentTimestamp() const;

    /**
     * @brief 로그 메시지 포맷팅
     * @param level 로그 레벨
     * @param message 로그 메시지
     * @return 포맷된 로그 문자열
     */
    std::string formatMessage(LogLevel level, const std::string& message) const;

    /**
     * @brief 단순 문자열 포맷 ({} 치환)
     * @param format 포맷 문자열
     * @param args 가변 인자
     * @return 포맷된 문자열
     */
    template<typename... Args>
    static std::string formatString(const std::string& format, Args&&... args);

    // 재귀 종료
    static void formatStringImpl(std::ostringstream& oss, const std::string& format, size_t pos);

    // 재귀 구현
    template<typename T, typename... Args>
    static void formatStringImpl(std::ostringstream& oss, const std::string& format,
                                  size_t pos, T&& value, Args&&... args);

private:
    LogLevel m_level;                   ///< 현재 로그 레벨
    bool m_consoleOutput;               ///< 콘솔 출력 활성화 여부
    std::ofstream m_fileStream;         ///< 파일 출력 스트림
    mutable std::mutex m_mutex;         ///< 스레드 안전성을 위한 뮤텍스
    std::string m_timestampFormat;      ///< 타임스탬프 포맷
    std::string m_pattern;              ///< 로그 메시지 패턴
};

// ======================================================================
// 템플릿 구현 (헤더에 포함되어야 함)
// ======================================================================

template<typename... Args>
void Logger::debugf(const std::string& format, Args&&... args) {
    getInstance().log(LogLevel::DEBUG, formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void Logger::infof(const std::string& format, Args&&... args) {
    getInstance().log(LogLevel::INFO, formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void Logger::warningf(const std::string& format, Args&&... args) {
    getInstance().log(LogLevel::WARNING, formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void Logger::errorf(const std::string& format, Args&&... args) {
    getInstance().log(LogLevel::ERROR, formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
void Logger::criticalf(const std::string& format, Args&&... args) {
    getInstance().log(LogLevel::CRITICAL, formatString(format, std::forward<Args>(args)...));
}

template<typename... Args>
std::string Logger::formatString(const std::string& format, Args&&... args) {
    std::ostringstream oss;
    formatStringImpl(oss, format, 0, std::forward<Args>(args)...);
    return oss.str();
}

template<typename T, typename... Args>
void Logger::formatStringImpl(std::ostringstream& oss, const std::string& format,
                              size_t pos, T&& value, Args&&... args) {
    size_t placeholderPos = format.find("{}", pos);
    if (placeholderPos != std::string::npos) {
        oss << format.substr(pos, placeholderPos - pos);
        oss << std::forward<T>(value);
        formatStringImpl(oss, format, placeholderPos + 2, std::forward<Args>(args)...);
    } else {
        oss << format.substr(pos);
    }
}

} // namespace utils
} // namespace koomesh
