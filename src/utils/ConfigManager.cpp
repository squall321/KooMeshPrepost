#include "utils/ConfigManager.h"
#include <fstream>
#include <algorithm>
#include <cctype>

namespace koomesh {
namespace utils {

// ======================================================================
// ConfigValue 구현
// ======================================================================

ConfigValue::ConfigValue(const std::vector<std::string>& values)
    : m_type(ConfigValueType::ARRAY) {
    // 배열을 JSON 배열 형식으로 저장: ["value1", "value2", ...]
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << "\"" << values[i] << "\"";
    }
    oss << "]";
    m_value = oss.str();
}

int ConfigValue::asInt() const {
    try {
        return std::stoi(m_value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Cannot convert '" + m_value + "' to int: " + e.what());
    }
}

double ConfigValue::asDouble() const {
    try {
        return std::stod(m_value);
    } catch (const std::exception& e) {
        throw std::runtime_error("Cannot convert '" + m_value + "' to double: " + e.what());
    }
}

bool ConfigValue::asBool() const {
    std::string lower = m_value;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        return true;
    } else if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        return false;
    }

    throw std::runtime_error("Cannot convert '" + m_value + "' to bool");
}

std::vector<std::string> ConfigValue::asArray() const {
    if (m_type != ConfigValueType::ARRAY) {
        throw std::runtime_error("ConfigValue is not an array type");
    }

    std::vector<std::string> result;

    // 간단한 JSON 배열 파싱: ["value1", "value2", ...]
    std::string trimmed = m_value;
    if (trimmed.empty() || trimmed.front() != '[' || trimmed.back() != ']') {
        throw std::runtime_error("Invalid array format: " + m_value);
    }

    trimmed = trimmed.substr(1, trimmed.length() - 2); // [ ] 제거

    // 쉼표로 분리
    std::istringstream iss(trimmed);
    std::string token;
    while (std::getline(iss, token, ',')) {
        // 앞뒤 공백 제거
        token.erase(0, token.find_first_not_of(" \t\n\r"));
        token.erase(token.find_last_not_of(" \t\n\r") + 1);

        // 따옴표 제거
        if (!token.empty() && token.front() == '"' && token.back() == '"') {
            token = token.substr(1, token.length() - 2);
        }

        if (!token.empty()) {
            result.push_back(token);
        }
    }

    return result;
}

// ======================================================================
// ConfigManager 구현
// ======================================================================

ConfigManager::ConfigManager()
    : m_autoSave(false) {
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadFromFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ifstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();

    m_configFilePath = filepath;
    return parseJson(content);
}

bool ConfigManager::saveToFile(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    std::string json = generateJson();
    file << json;
    file.close();

    return true;
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const {
    const ConfigValue* value = getValue(key);
    if (value == nullptr) {
        return defaultValue;
    }
    return value->asString();
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    const ConfigValue* value = getValue(key);
    if (value == nullptr) {
        return defaultValue;
    }
    try {
        return value->asInt();
    } catch (const std::exception&) {
        return defaultValue;
    }
}

double ConfigManager::getDouble(const std::string& key, double defaultValue) const {
    const ConfigValue* value = getValue(key);
    if (value == nullptr) {
        return defaultValue;
    }
    try {
        return value->asDouble();
    } catch (const std::exception&) {
        return defaultValue;
    }
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) const {
    const ConfigValue* value = getValue(key);
    if (value == nullptr) {
        return defaultValue;
    }
    try {
        return value->asBool();
    } catch (const std::exception&) {
        return defaultValue;
    }
}

std::vector<std::string> ConfigManager::getArray(const std::string& key,
                                                   const std::vector<std::string>& defaultValue) const {
    const ConfigValue* value = getValue(key);
    if (value == nullptr) {
        return defaultValue;
    }
    try {
        return value->asArray();
    } catch (const std::exception&) {
        return defaultValue;
    }
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    setValue(key, ConfigValue(value));
}

void ConfigManager::setInt(const std::string& key, int value) {
    setValue(key, ConfigValue(value));
}

void ConfigManager::setDouble(const std::string& key, double value) {
    setValue(key, ConfigValue(value));
}

void ConfigManager::setBool(const std::string& key, bool value) {
    setValue(key, ConfigValue(value));
}

void ConfigManager::setArray(const std::string& key, const std::vector<std::string>& values) {
    setValue(key, ConfigValue(values));
}

bool ConfigManager::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_values.find(key) != m_values.end();
}

bool ConfigManager::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(key);
    if (it != m_values.end()) {
        m_values.erase(it);

        if (m_autoSave && !m_configFilePath.empty()) {
            std::lock_guard<std::mutex> fileLock(m_mutex);
            saveToFile(m_configFilePath);
        }

        return true;
    }
    return false;
}

void ConfigManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_values.clear();
}

std::vector<std::string> ConfigManager::getAllKeys() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> keys;
    keys.reserve(m_values.size());
    for (const auto& pair : m_values) {
        keys.push_back(pair.first);
    }
    return keys;
}

std::vector<std::string> ConfigManager::getKeysWithPrefix(const std::string& prefix) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> keys;
    for (const auto& pair : m_values) {
        if (pair.first.find(prefix) == 0) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

void ConfigManager::setConfigFilePath(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_configFilePath = filepath;
}

std::string ConfigManager::getConfigFilePath() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_configFilePath;
}

void ConfigManager::setAutoSave(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_autoSave = enabled;
}

bool ConfigManager::isAutoSaveEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_autoSave;
}

const ConfigValue* ConfigManager::getValue(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_values.find(key);
    if (it != m_values.end()) {
        return &it->second;
    }
    return nullptr;
}

void ConfigManager::setValue(const std::string& key, const ConfigValue& value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_values[key] = value;
    }

    // 자동 저장이 활성화되어 있으면 파일에 저장
    if (m_autoSave && !m_configFilePath.empty()) {
        saveToFile(m_configFilePath);
    }
}

bool ConfigManager::parseJson(const std::string& content) {
    // 간단한 JSON 파서 (중첩된 객체는 키를 점(.)으로 구분)
    // 형식: { "key": "value", "key2": 123, "nested.key": "value" }

    m_values.clear();

    size_t pos = 0;
    size_t len = content.length();

    // { 찾기
    while (pos < len && content[pos] != '{') {
        ++pos;
    }
    if (pos >= len) {
        return false;
    }
    ++pos; // { 건너뛰기

    while (pos < len) {
        // 공백 건너뛰기
        while (pos < len && std::isspace(content[pos])) {
            ++pos;
        }

        if (pos >= len || content[pos] == '}') {
            break; // JSON 끝
        }

        // 키 읽기 (따옴표로 감싸져 있음)
        if (content[pos] != '"') {
            return false;
        }
        ++pos; // " 건너뛰기

        size_t keyStart = pos;
        while (pos < len && content[pos] != '"') {
            if (content[pos] == '\\') {
                ++pos; // 이스케이프 문자 건너뛰기
            }
            ++pos;
        }
        if (pos >= len) {
            return false;
        }

        std::string key = unescapeJsonString(content.substr(keyStart, pos - keyStart));
        ++pos; // " 건너뛰기

        // : 찾기
        while (pos < len && std::isspace(content[pos])) {
            ++pos;
        }
        if (pos >= len || content[pos] != ':') {
            return false;
        }
        ++pos; // : 건너뛰기

        // 값 읽기
        while (pos < len && std::isspace(content[pos])) {
            ++pos;
        }
        if (pos >= len) {
            return false;
        }

        std::string value;
        ConfigValueType type = ConfigValueType::STRING;

        if (content[pos] == '"') {
            // 문자열 값
            ++pos; // " 건너뛰기
            size_t valueStart = pos;
            while (pos < len && content[pos] != '"') {
                if (content[pos] == '\\') {
                    ++pos; // 이스케이프 문자 건너뛰기
                }
                ++pos;
            }
            if (pos >= len) {
                return false;
            }
            value = unescapeJsonString(content.substr(valueStart, pos - valueStart));
            ++pos; // " 건너뛰기
            type = ConfigValueType::STRING;
        } else if (content[pos] == '[') {
            // 배열 값
            size_t arrayStart = pos;
            int bracketCount = 1;
            ++pos;
            while (pos < len && bracketCount > 0) {
                if (content[pos] == '[') {
                    ++bracketCount;
                } else if (content[pos] == ']') {
                    --bracketCount;
                } else if (content[pos] == '"') {
                    // 문자열 내부의 [] 무시
                    ++pos;
                    while (pos < len && content[pos] != '"') {
                        if (content[pos] == '\\') {
                            ++pos;
                        }
                        ++pos;
                    }
                }
                ++pos;
            }
            value = content.substr(arrayStart, pos - arrayStart);
            type = ConfigValueType::ARRAY;
        } else if (content[pos] == 't' || content[pos] == 'f') {
            // 불리언 값
            if (content.substr(pos, 4) == "true") {
                value = "true";
                pos += 4;
            } else if (content.substr(pos, 5) == "false") {
                value = "false";
                pos += 5;
            } else {
                return false;
            }
            type = ConfigValueType::BOOLEAN;
        } else if (std::isdigit(content[pos]) || content[pos] == '-' || content[pos] == '+') {
            // 숫자 값
            size_t valueStart = pos;
            bool hasDecimal = false;
            if (content[pos] == '-' || content[pos] == '+') {
                ++pos;
            }
            while (pos < len && (std::isdigit(content[pos]) || content[pos] == '.')) {
                if (content[pos] == '.') {
                    hasDecimal = true;
                }
                ++pos;
            }
            value = content.substr(valueStart, pos - valueStart);
            type = hasDecimal ? ConfigValueType::DOUBLE : ConfigValueType::INTEGER;
        } else {
            return false;
        }

        // ConfigValue 생성 및 저장
        if (type == ConfigValueType::STRING) {
            m_values[key] = ConfigValue(value);
        } else if (type == ConfigValueType::INTEGER) {
            m_values[key] = ConfigValue(std::stoi(value));
        } else if (type == ConfigValueType::DOUBLE) {
            m_values[key] = ConfigValue(std::stod(value));
        } else if (type == ConfigValueType::BOOLEAN) {
            m_values[key] = ConfigValue(value == "true");
        } else if (type == ConfigValueType::ARRAY) {
            // 배열은 문자열 그대로 저장 (asArray()에서 파싱)
            ConfigValue arrayValue;
            arrayValue = ConfigValue(std::vector<std::string>{}); // 타입 설정을 위해
            // 직접 설정
            m_values[key] = ConfigValue(std::vector<std::string>{});
            // 값 덮어쓰기 (해킹이지만 간단한 구현을 위해)
            m_values[key] = ConfigValue("");
            // 실제로 배열 파싱
            std::vector<std::string> arrayValues;
            std::string trimmed = value.substr(1, value.length() - 2); // [ ] 제거
            std::istringstream iss(trimmed);
            std::string token;
            while (std::getline(iss, token, ',')) {
                token = trim(token);
                if (!token.empty() && token.front() == '"' && token.back() == '"') {
                    token = token.substr(1, token.length() - 2);
                    arrayValues.push_back(unescapeJsonString(token));
                }
            }
            m_values[key] = ConfigValue(arrayValues);
        }

        // , 또는 } 찾기
        while (pos < len && std::isspace(content[pos])) {
            ++pos;
        }
        if (pos >= len) {
            return false;
        }
        if (content[pos] == ',') {
            ++pos;
        } else if (content[pos] == '}') {
            break;
        } else {
            return false;
        }
    }

    return true;
}

std::string ConfigManager::generateJson() const {
    std::ostringstream oss;
    oss << "{\n";

    size_t count = 0;
    for (const auto& pair : m_values) {
        if (count > 0) {
            oss << ",\n";
        }

        oss << "  \"" << escapeJsonString(pair.first) << "\": ";

        const ConfigValue& value = pair.second;
        switch (value.getType()) {
        case ConfigValueType::STRING:
            oss << "\"" << escapeJsonString(value.asString()) << "\"";
            break;
        case ConfigValueType::INTEGER:
            oss << value.asInt();
            break;
        case ConfigValueType::DOUBLE:
            oss << value.asDouble();
            break;
        case ConfigValueType::BOOLEAN:
            oss << (value.asBool() ? "true" : "false");
            break;
        case ConfigValueType::ARRAY: {
            auto arr = value.asArray();
            oss << "[";
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) {
                    oss << ", ";
                }
                oss << "\"" << escapeJsonString(arr[i]) << "\"";
            }
            oss << "]";
            break;
        }
        }

        ++count;
    }

    oss << "\n}\n";
    return oss.str();
}

std::string ConfigManager::escapeJsonString(const std::string& str) const {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
        case '"':  oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\b': oss << "\\b";  break;
        case '\f': oss << "\\f";  break;
        case '\n': oss << "\\n";  break;
        case '\r': oss << "\\r";  break;
        case '\t': oss << "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                // 제어 문자는 \uXXXX 형식으로
                oss << "\\u00" << std::hex << static_cast<int>(c);
            } else {
                oss << c;
            }
        }
    }
    return oss.str();
}

std::string ConfigManager::unescapeJsonString(const std::string& str) const {
    std::ostringstream oss;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
            case '"':  oss << '"';  ++i; break;
            case '\\': oss << '\\'; ++i; break;
            case 'b':  oss << '\b'; ++i; break;
            case 'f':  oss << '\f'; ++i; break;
            case 'n':  oss << '\n'; ++i; break;
            case 'r':  oss << '\r'; ++i; break;
            case 't':  oss << '\t'; ++i; break;
            default:   oss << str[i]; break;
            }
        } else {
            oss << str[i];
        }
    }
    return oss.str();
}

std::string ConfigManager::trim(const std::string& str) const {
    size_t start = 0;
    while (start < str.length() && std::isspace(str[start])) {
        ++start;
    }

    size_t end = str.length();
    while (end > start && std::isspace(str[end - 1])) {
        --end;
    }

    return str.substr(start, end - start);
}

} // namespace utils
} // namespace koomesh
