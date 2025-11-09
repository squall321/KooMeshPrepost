#pragma once

#include <exception>
#include <string>
#include <sstream>

namespace koomesh {
namespace utils {

/**
 * @brief 에러 코드 열거형
 *
 * 애플리케이션에서 발생할 수 있는 모든 에러를 카테고리별로 분류합니다.
 */
enum class ErrorCode {
    // 일반 에러 (1000-1999)
    UNKNOWN = 1000,
    NOT_IMPLEMENTED = 1001,
    INVALID_ARGUMENT = 1002,
    NULL_POINTER = 1003,
    OUT_OF_RANGE = 1004,

    // 파일 I/O 에러 (2000-2999)
    FILE_NOT_FOUND = 2000,
    FILE_OPEN_FAILED = 2001,
    FILE_READ_FAILED = 2002,
    FILE_WRITE_FAILED = 2003,
    FILE_CLOSE_FAILED = 2004,
    DIRECTORY_NOT_FOUND = 2005,
    PERMISSION_DENIED = 2006,

    // 파싱 에러 (3000-3999)
    PARSE_ERROR = 3000,
    INVALID_FORMAT = 3001,
    UNEXPECTED_TOKEN = 3002,
    MISSING_KEYWORD = 3003,
    INVALID_KEYWORD = 3004,
    SYNTAX_ERROR = 3005,

    // 데이터 유효성 에러 (4000-4999)
    INVALID_DATA = 4000,
    INVALID_NODE_ID = 4001,
    INVALID_ELEMENT_ID = 4002,
    INVALID_PART_ID = 4003,
    DUPLICATE_ID = 4004,
    MISSING_DATA = 4005,
    INCONSISTENT_DATA = 4006,

    // 메모리 에러 (5000-5999)
    OUT_OF_MEMORY = 5000,
    ALLOCATION_FAILED = 5001,
    BUFFER_OVERFLOW = 5002,
    BUFFER_UNDERFLOW = 5003,

    // 렌더링 에러 (6000-6999)
    RENDER_ERROR = 6000,
    VTK_INITIALIZATION_FAILED = 6001,
    SHADER_COMPILATION_FAILED = 6002,
    TEXTURE_LOAD_FAILED = 6003,

    // 선택 에러 (7000-7999)
    SELECTION_ERROR = 7000,
    INVALID_SELECTION = 7001,
    EMPTY_SELECTION = 7002,

    // 설정 에러 (8000-8999)
    CONFIG_ERROR = 8000,
    CONFIG_LOAD_FAILED = 8001,
    CONFIG_SAVE_FAILED = 8002,
    INVALID_CONFIG_VALUE = 8003
};

/**
 * @brief 에러 코드를 문자열로 변환
 *
 * @param code 에러 코드
 * @return 에러 코드 문자열 표현
 */
inline std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        // 일반 에러
        case ErrorCode::UNKNOWN: return "UNKNOWN";
        case ErrorCode::NOT_IMPLEMENTED: return "NOT_IMPLEMENTED";
        case ErrorCode::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
        case ErrorCode::NULL_POINTER: return "NULL_POINTER";
        case ErrorCode::OUT_OF_RANGE: return "OUT_OF_RANGE";

        // 파일 I/O 에러
        case ErrorCode::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case ErrorCode::FILE_OPEN_FAILED: return "FILE_OPEN_FAILED";
        case ErrorCode::FILE_READ_FAILED: return "FILE_READ_FAILED";
        case ErrorCode::FILE_WRITE_FAILED: return "FILE_WRITE_FAILED";
        case ErrorCode::FILE_CLOSE_FAILED: return "FILE_CLOSE_FAILED";
        case ErrorCode::DIRECTORY_NOT_FOUND: return "DIRECTORY_NOT_FOUND";
        case ErrorCode::PERMISSION_DENIED: return "PERMISSION_DENIED";

        // 파싱 에러
        case ErrorCode::PARSE_ERROR: return "PARSE_ERROR";
        case ErrorCode::INVALID_FORMAT: return "INVALID_FORMAT";
        case ErrorCode::UNEXPECTED_TOKEN: return "UNEXPECTED_TOKEN";
        case ErrorCode::MISSING_KEYWORD: return "MISSING_KEYWORD";
        case ErrorCode::INVALID_KEYWORD: return "INVALID_KEYWORD";
        case ErrorCode::SYNTAX_ERROR: return "SYNTAX_ERROR";

        // 데이터 유효성 에러
        case ErrorCode::INVALID_DATA: return "INVALID_DATA";
        case ErrorCode::INVALID_NODE_ID: return "INVALID_NODE_ID";
        case ErrorCode::INVALID_ELEMENT_ID: return "INVALID_ELEMENT_ID";
        case ErrorCode::INVALID_PART_ID: return "INVALID_PART_ID";
        case ErrorCode::DUPLICATE_ID: return "DUPLICATE_ID";
        case ErrorCode::MISSING_DATA: return "MISSING_DATA";
        case ErrorCode::INCONSISTENT_DATA: return "INCONSISTENT_DATA";

        // 메모리 에러
        case ErrorCode::OUT_OF_MEMORY: return "OUT_OF_MEMORY";
        case ErrorCode::ALLOCATION_FAILED: return "ALLOCATION_FAILED";
        case ErrorCode::BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
        case ErrorCode::BUFFER_UNDERFLOW: return "BUFFER_UNDERFLOW";

        // 렌더링 에러
        case ErrorCode::RENDER_ERROR: return "RENDER_ERROR";
        case ErrorCode::VTK_INITIALIZATION_FAILED: return "VTK_INITIALIZATION_FAILED";
        case ErrorCode::SHADER_COMPILATION_FAILED: return "SHADER_COMPILATION_FAILED";
        case ErrorCode::TEXTURE_LOAD_FAILED: return "TEXTURE_LOAD_FAILED";

        // 선택 에러
        case ErrorCode::SELECTION_ERROR: return "SELECTION_ERROR";
        case ErrorCode::INVALID_SELECTION: return "INVALID_SELECTION";
        case ErrorCode::EMPTY_SELECTION: return "EMPTY_SELECTION";

        // 설정 에러
        case ErrorCode::CONFIG_ERROR: return "CONFIG_ERROR";
        case ErrorCode::CONFIG_LOAD_FAILED: return "CONFIG_LOAD_FAILED";
        case ErrorCode::CONFIG_SAVE_FAILED: return "CONFIG_SAVE_FAILED";
        case ErrorCode::INVALID_CONFIG_VALUE: return "INVALID_CONFIG_VALUE";

        default: return "UNKNOWN_ERROR_CODE";
    }
}

/**
 * @brief 에러 카테고리 반환
 *
 * @param code 에러 코드
 * @return 에러 카테고리 문자열
 */
inline std::string getErrorCategory(ErrorCode code) {
    int codeValue = static_cast<int>(code);

    if (codeValue >= 1000 && codeValue < 2000) return "General";
    if (codeValue >= 2000 && codeValue < 3000) return "FileIO";
    if (codeValue >= 3000 && codeValue < 4000) return "Parsing";
    if (codeValue >= 4000 && codeValue < 5000) return "DataValidation";
    if (codeValue >= 5000 && codeValue < 6000) return "Memory";
    if (codeValue >= 6000 && codeValue < 7000) return "Rendering";
    if (codeValue >= 7000 && codeValue < 8000) return "Selection";
    if (codeValue >= 8000 && codeValue < 9000) return "Configuration";

    return "Unknown";
}

/**
 * @brief 기본 예외 클래스
 *
 * 모든 KooMesh 예외의 기본 클래스입니다.
 * std::exception을 상속하며, 에러 코드, 파일명, 라인 번호 등의
 * 추가 정보를 제공합니다.
 *
 * Example usage:
 * @code
 * throw KooMeshException(ErrorCode::INVALID_ARGUMENT,
 *                        "Invalid mesh size",
 *                        __FILE__, __LINE__);
 * @endcode
 */
class KooMeshException : public std::exception {
public:
    /**
     * @brief 생성자
     *
     * @param code 에러 코드
     * @param message 에러 메시지
     * @param file 파일명 (기본값: 빈 문자열)
     * @param line 라인 번호 (기본값: 0)
     */
    KooMeshException(ErrorCode code,
                     const std::string& message,
                     const std::string& file = "",
                     int line = 0)
        : m_code(code)
        , m_message(message)
        , m_file(file)
        , m_line(line) {
        buildFullMessage();
    }

    /**
     * @brief 소멸자
     */
    virtual ~KooMeshException() noexcept = default;

    /**
     * @brief 전체 에러 메시지 반환
     *
     * @return 에러 메시지 (C 문자열)
     */
    virtual const char* what() const noexcept override {
        return m_fullMessage.c_str();
    }

    /**
     * @brief 에러 코드 반환
     *
     * @return 에러 코드
     */
    ErrorCode getErrorCode() const noexcept {
        return m_code;
    }

    /**
     * @brief 에러 메시지 반환
     *
     * @return 에러 메시지
     */
    const std::string& getMessage() const noexcept {
        return m_message;
    }

    /**
     * @brief 파일명 반환
     *
     * @return 파일명
     */
    const std::string& getFile() const noexcept {
        return m_file;
    }

    /**
     * @brief 라인 번호 반환
     *
     * @return 라인 번호
     */
    int getLine() const noexcept {
        return m_line;
    }

    /**
     * @brief 예외 타입 이름 반환
     *
     * @return 예외 타입 이름
     */
    virtual const char* getTypeName() const noexcept {
        return "KooMeshException";
    }

protected:
    /**
     * @brief 전체 메시지 생성
     */
    void buildFullMessage() {
        std::ostringstream oss;
        oss << "[" << getTypeName() << "] ";
        oss << "[" << getErrorCategory(m_code) << "] ";
        oss << "[" << errorCodeToString(m_code) << "] ";
        oss << m_message;

        if (!m_file.empty() && m_line > 0) {
            oss << " (at " << m_file << ":" << m_line << ")";
        }

        m_fullMessage = oss.str();
    }

protected:
    ErrorCode m_code;         ///< 에러 코드
    std::string m_message;    ///< 에러 메시지
    std::string m_file;       ///< 파일명
    int m_line;               ///< 라인 번호
    std::string m_fullMessage;///< 전체 에러 메시지
};

/**
 * @brief 파일 I/O 예외
 *
 * 파일 읽기/쓰기 작업 중 발생하는 예외입니다.
 */
class FileIOException : public KooMeshException {
public:
    FileIOException(ErrorCode code,
                   const std::string& message,
                   const std::string& filepath = "",
                   const std::string& file = "",
                   int line = 0)
        : KooMeshException(code, message, file, line)
        , m_filepath(filepath) {
        buildFullMessage();
    }

    const char* getTypeName() const noexcept override {
        return "FileIOException";
    }

    const std::string& getFilePath() const noexcept {
        return m_filepath;
    }

protected:
    void buildFullMessage() {
        std::ostringstream oss;
        oss << "[" << getTypeName() << "] ";
        oss << "[" << getErrorCategory(m_code) << "] ";
        oss << "[" << errorCodeToString(m_code) << "] ";
        oss << m_message;

        if (!m_filepath.empty()) {
            oss << " (file: " << m_filepath << ")";
        }

        if (!m_file.empty() && m_line > 0) {
            oss << " (at " << m_file << ":" << m_line << ")";
        }

        m_fullMessage = oss.str();
    }

private:
    std::string m_filepath;  ///< 관련 파일 경로
};

/**
 * @brief 파싱 예외
 *
 * 파일 파싱 중 발생하는 예외입니다.
 */
class ParseException : public KooMeshException {
public:
    ParseException(ErrorCode code,
                  const std::string& message,
                  int lineNumber = -1,
                  int columnNumber = -1,
                  const std::string& file = "",
                  int line = 0)
        : KooMeshException(code, message, file, line)
        , m_lineNumber(lineNumber)
        , m_columnNumber(columnNumber) {
        buildFullMessage();
    }

    const char* getTypeName() const noexcept override {
        return "ParseException";
    }

    int getLineNumber() const noexcept {
        return m_lineNumber;
    }

    int getColumnNumber() const noexcept {
        return m_columnNumber;
    }

protected:
    void buildFullMessage() {
        std::ostringstream oss;
        oss << "[" << getTypeName() << "] ";
        oss << "[" << getErrorCategory(m_code) << "] ";
        oss << "[" << errorCodeToString(m_code) << "] ";
        oss << m_message;

        if (m_lineNumber >= 0) {
            oss << " (line: " << m_lineNumber;
            if (m_columnNumber >= 0) {
                oss << ", column: " << m_columnNumber;
            }
            oss << ")";
        }

        if (!m_file.empty() && m_line > 0) {
            oss << " (at " << m_file << ":" << m_line << ")";
        }

        m_fullMessage = oss.str();
    }

private:
    int m_lineNumber;    ///< 파싱 중 에러가 발생한 라인 번호
    int m_columnNumber;  ///< 파싱 중 에러가 발생한 컬럼 번호
};

/**
 * @brief 데이터 유효성 예외
 *
 * 데이터 유효성 검증 실패 시 발생하는 예외입니다.
 */
class InvalidDataException : public KooMeshException {
public:
    InvalidDataException(ErrorCode code,
                        const std::string& message,
                        const std::string& dataName = "",
                        const std::string& file = "",
                        int line = 0)
        : KooMeshException(code, message, file, line)
        , m_dataName(dataName) {
        buildFullMessage();
    }

    const char* getTypeName() const noexcept override {
        return "InvalidDataException";
    }

    const std::string& getDataName() const noexcept {
        return m_dataName;
    }

protected:
    void buildFullMessage() {
        std::ostringstream oss;
        oss << "[" << getTypeName() << "] ";
        oss << "[" << getErrorCategory(m_code) << "] ";
        oss << "[" << errorCodeToString(m_code) << "] ";
        oss << m_message;

        if (!m_dataName.empty()) {
            oss << " (data: " << m_dataName << ")";
        }

        if (!m_file.empty() && m_line > 0) {
            oss << " (at " << m_file << ":" << m_line << ")";
        }

        m_fullMessage = oss.str();
    }

private:
    std::string m_dataName;  ///< 유효성 검증 실패한 데이터 이름
};

/**
 * @brief 메모리 예외
 *
 * 메모리 할당/해제 중 발생하는 예외입니다.
 */
class OutOfMemoryException : public KooMeshException {
public:
    OutOfMemoryException(const std::string& message,
                        size_t requestedSize = 0,
                        const std::string& file = "",
                        int line = 0)
        : KooMeshException(ErrorCode::OUT_OF_MEMORY, message, file, line)
        , m_requestedSize(requestedSize) {
        buildFullMessage();
    }

    const char* getTypeName() const noexcept override {
        return "OutOfMemoryException";
    }

    size_t getRequestedSize() const noexcept {
        return m_requestedSize;
    }

protected:
    void buildFullMessage() {
        std::ostringstream oss;
        oss << "[" << getTypeName() << "] ";
        oss << "[" << getErrorCategory(m_code) << "] ";
        oss << "[" << errorCodeToString(m_code) << "] ";
        oss << m_message;

        if (m_requestedSize > 0) {
            oss << " (requested: " << m_requestedSize << " bytes)";
        }

        if (!m_file.empty() && m_line > 0) {
            oss << " (at " << m_file << ":" << m_line << ")";
        }

        m_fullMessage = oss.str();
    }

private:
    size_t m_requestedSize;  ///< 요청한 메모리 크기
};

/**
 * @brief 런타임 예외
 *
 * 런타임 중 발생하는 일반적인 예외입니다.
 */
class RuntimeException : public KooMeshException {
public:
    RuntimeException(ErrorCode code,
                    const std::string& message,
                    const std::string& file = "",
                    int line = 0)
        : KooMeshException(code, message, file, line) {
    }

    const char* getTypeName() const noexcept override {
        return "RuntimeException";
    }
};

// ======================================================================
// 예외 생성 매크로 (파일명과 라인 번호 자동 삽입)
// ======================================================================

#define KOOMESH_THROW(exceptionType, code, message) \
    throw exceptionType(code, message, __FILE__, __LINE__)

#define KOOMESH_THROW_FILE_IO(code, message, filepath) \
    throw FileIOException(code, message, filepath, __FILE__, __LINE__)

#define KOOMESH_THROW_PARSE(code, message, lineNum, colNum) \
    throw ParseException(code, message, lineNum, colNum, __FILE__, __LINE__)

#define KOOMESH_THROW_INVALID_DATA(code, message, dataName) \
    throw InvalidDataException(code, message, dataName, __FILE__, __LINE__)

#define KOOMESH_THROW_OUT_OF_MEMORY(message, size) \
    throw OutOfMemoryException(message, size, __FILE__, __LINE__)

#define KOOMESH_THROW_RUNTIME(code, message) \
    throw RuntimeException(code, message, __FILE__, __LINE__)

} // namespace utils
} // namespace koomesh
