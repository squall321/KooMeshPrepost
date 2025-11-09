#pragma once

#include <string>
#include <map>
#include <mutex>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace koomesh {
namespace utils {

/**
 * @brief 설정값 타입
 */
enum class ConfigValueType {
    STRING,     ///< 문자열
    INTEGER,    ///< 정수
    DOUBLE,     ///< 실수
    BOOLEAN,    ///< 불리언
    ARRAY       ///< 배열 (문자열 배열)
};

/**
 * @brief 설정값을 저장하는 클래스
 *
 * 다양한 타입의 값을 문자열로 저장하고,
 * 타입 안전한 방식으로 변환합니다.
 */
class ConfigValue {
public:
    ConfigValue() : m_type(ConfigValueType::STRING), m_value("") {}

    explicit ConfigValue(const std::string& value)
        : m_type(ConfigValueType::STRING), m_value(value) {}

    explicit ConfigValue(int value)
        : m_type(ConfigValueType::INTEGER), m_value(std::to_string(value)) {}

    explicit ConfigValue(double value)
        : m_type(ConfigValueType::DOUBLE), m_value(std::to_string(value)) {}

    explicit ConfigValue(bool value)
        : m_type(ConfigValueType::BOOLEAN), m_value(value ? "true" : "false") {}

    explicit ConfigValue(const std::vector<std::string>& values);

    /**
     * @brief 타입 반환
     * @return 설정값 타입
     */
    ConfigValueType getType() const { return m_type; }

    /**
     * @brief 문자열로 변환
     * @return 문자열 값
     */
    std::string asString() const { return m_value; }

    /**
     * @brief 정수로 변환
     * @return 정수 값
     * @throws std::runtime_error 변환 실패 시
     */
    int asInt() const;

    /**
     * @brief 실수로 변환
     * @return 실수 값
     * @throws std::runtime_error 변환 실패 시
     */
    double asDouble() const;

    /**
     * @brief 불리언으로 변환
     * @return 불리언 값
     * @throws std::runtime_error 변환 실패 시
     */
    bool asBool() const;

    /**
     * @brief 배열로 변환
     * @return 문자열 벡터
     * @throws std::runtime_error 변환 실패 시
     */
    std::vector<std::string> asArray() const;

private:
    ConfigValueType m_type;  ///< 값 타입
    std::string m_value;     ///< 값 (문자열로 저장)
};

/**
 * @brief ConfigManager 클래스 - Singleton 패턴
 *
 * 애플리케이션 설정을 관리하는 중앙 집중식 설정 관리자입니다.
 * JSON 형식의 설정 파일을 읽고 쓸 수 있으며,
 * 타입 안전한 방식으로 설정값에 접근할 수 있습니다.
 *
 * Example usage:
 * @code
 * ConfigManager::getInstance().loadFromFile("config.json");
 * int windowWidth = ConfigManager::getInstance().getInt("ui.window.width", 1024);
 * ConfigManager::getInstance().setInt("ui.window.width", 1280);
 * ConfigManager::getInstance().saveToFile("config.json");
 * @endcode
 *
 * @note 이 클래스는 스레드 안전합니다.
 */
class ConfigManager {
public:
    /**
     * @brief Singleton 인스턴스 얻기
     * @return ConfigManager 인스턴스 참조
     */
    static ConfigManager& getInstance();

    // Singleton: 복사 및 이동 금지
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    ConfigManager(ConfigManager&&) = delete;
    ConfigManager& operator=(ConfigManager&&) = delete;

    /**
     * @brief 소멸자
     */
    ~ConfigManager() = default;

    // ======================================================================
    // 파일 I/O
    // ======================================================================

    /**
     * @brief JSON 파일에서 설정 로드
     *
     * @param filepath 설정 파일 경로
     * @return 성공 여부
     */
    bool loadFromFile(const std::string& filepath);

    /**
     * @brief 현재 설정을 JSON 파일로 저장
     *
     * @param filepath 설정 파일 경로
     * @return 성공 여부
     */
    bool saveToFile(const std::string& filepath) const;

    // ======================================================================
    // 설정값 읽기 (기본값 지원)
    // ======================================================================

    /**
     * @brief 문자열 설정값 읽기
     *
     * @param key 설정 키 (예: "ui.window.title")
     * @param defaultValue 기본값 (키가 없을 경우)
     * @return 설정값 또는 기본값
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * @brief 정수 설정값 읽기
     *
     * @param key 설정 키
     * @param defaultValue 기본값
     * @return 설정값 또는 기본값
     */
    int getInt(const std::string& key, int defaultValue = 0) const;

    /**
     * @brief 실수 설정값 읽기
     *
     * @param key 설정 키
     * @param defaultValue 기본값
     * @return 설정값 또는 기본값
     */
    double getDouble(const std::string& key, double defaultValue = 0.0) const;

    /**
     * @brief 불리언 설정값 읽기
     *
     * @param key 설정 키
     * @param defaultValue 기본값
     * @return 설정값 또는 기본값
     */
    bool getBool(const std::string& key, bool defaultValue = false) const;

    /**
     * @brief 배열 설정값 읽기
     *
     * @param key 설정 키
     * @param defaultValue 기본값
     * @return 설정값 또는 기본값
     */
    std::vector<std::string> getArray(const std::string& key,
                                       const std::vector<std::string>& defaultValue = {}) const;

    // ======================================================================
    // 설정값 쓰기
    // ======================================================================

    /**
     * @brief 문자열 설정값 쓰기
     *
     * @param key 설정 키
     * @param value 설정값
     */
    void setString(const std::string& key, const std::string& value);

    /**
     * @brief 정수 설정값 쓰기
     *
     * @param key 설정 키
     * @param value 설정값
     */
    void setInt(const std::string& key, int value);

    /**
     * @brief 실수 설정값 쓰기
     *
     * @param key 설정 키
     * @param value 설정값
     */
    void setDouble(const std::string& key, double value);

    /**
     * @brief 불리언 설정값 쓰기
     *
     * @param key 설정 키
     * @param value 설정값
     */
    void setBool(const std::string& key, bool value);

    /**
     * @brief 배열 설정값 쓰기
     *
     * @param key 설정 키
     * @param values 설정값 배열
     */
    void setArray(const std::string& key, const std::vector<std::string>& values);

    // ======================================================================
    // 유틸리티
    // ======================================================================

    /**
     * @brief 키 존재 여부 확인
     *
     * @param key 설정 키
     * @return 존재 여부
     */
    bool hasKey(const std::string& key) const;

    /**
     * @brief 키 삭제
     *
     * @param key 설정 키
     * @return 삭제 성공 여부 (키가 존재했는지)
     */
    bool removeKey(const std::string& key);

    /**
     * @brief 모든 설정 초기화
     */
    void clear();

    /**
     * @brief 모든 키 목록 반환
     *
     * @return 키 목록
     */
    std::vector<std::string> getAllKeys() const;

    /**
     * @brief 특정 접두사로 시작하는 키 목록 반환
     *
     * 예: getKeysWithPrefix("ui.") -> ["ui.window.width", "ui.window.height", ...]
     *
     * @param prefix 접두사
     * @return 키 목록
     */
    std::vector<std::string> getKeysWithPrefix(const std::string& prefix) const;

    /**
     * @brief 설정 파일 경로 설정
     *
     * @param filepath 설정 파일 경로
     */
    void setConfigFilePath(const std::string& filepath);

    /**
     * @brief 현재 설정 파일 경로 반환
     *
     * @return 설정 파일 경로
     */
    std::string getConfigFilePath() const;

    /**
     * @brief 설정 자동 저장 활성화/비활성화
     *
     * true로 설정 시, set* 메서드 호출마다 자동으로 파일에 저장
     *
     * @param enabled 활성화 여부
     */
    void setAutoSave(bool enabled);

    /**
     * @brief 설정 자동 저장 활성화 여부 반환
     *
     * @return 활성화 여부
     */
    bool isAutoSaveEnabled() const;

private:
    /**
     * @brief Private 생성자 (Singleton)
     */
    ConfigManager();

    /**
     * @brief 설정값 읽기 (내부 메서드)
     *
     * @param key 설정 키
     * @return 설정값 (없으면 nullptr)
     */
    const ConfigValue* getValue(const std::string& key) const;

    /**
     * @brief 설정값 쓰기 (내부 메서드)
     *
     * @param key 설정 키
     * @param value 설정값
     */
    void setValue(const std::string& key, const ConfigValue& value);

    /**
     * @brief JSON 파싱 (단순 구현)
     *
     * @param content JSON 문자열
     * @return 파싱 성공 여부
     */
    bool parseJson(const std::string& content);

    /**
     * @brief JSON 생성
     *
     * @return JSON 문자열
     */
    std::string generateJson() const;

    /**
     * @brief 문자열 이스케이프 (JSON용)
     *
     * @param str 원본 문자열
     * @return 이스케이프된 문자열
     */
    std::string escapeJsonString(const std::string& str) const;

    /**
     * @brief 문자열 언이스케이프 (JSON용)
     *
     * @param str 이스케이프된 문자열
     * @return 원본 문자열
     */
    std::string unescapeJsonString(const std::string& str) const;

    /**
     * @brief 문자열 공백 제거 (앞뒤)
     *
     * @param str 원본 문자열
     * @return 공백 제거된 문자열
     */
    std::string trim(const std::string& str) const;

private:
    mutable std::mutex m_mutex;                   ///< 스레드 안전성을 위한 뮤텍스
    std::map<std::string, ConfigValue> m_values;  ///< 설정값 저장소
    std::string m_configFilePath;                 ///< 설정 파일 경로
    bool m_autoSave;                              ///< 자동 저장 활성화 여부
};

} // namespace utils
} // namespace koomesh
