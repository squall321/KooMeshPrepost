#include <gtest/gtest.h>
#include "utils/ConfigManager.h"
#include <fstream>
#include <thread>

using namespace koomesh::utils;

/**
 * @brief ConfigManager 테스트 픽스처
 */
class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트용 임시 파일 경로
        testConfigFile = "test_config.json";

        // ConfigManager 초기화
        ConfigManager::getInstance().clear();
        ConfigManager::getInstance().setAutoSave(false);
    }

    void TearDown() override {
        // 테스트 파일 정리
        std::remove(testConfigFile.c_str());

        // ConfigManager 초기화
        ConfigManager::getInstance().clear();
    }

    std::string readFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return "";
        }
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        file.close();
        return content;
    }

    void writeFile(const std::string& filepath, const std::string& content) {
        std::ofstream file(filepath);
        if (file.is_open()) {
            file << content;
            file.close();
        }
    }

    std::string testConfigFile;
};

// ======================================================================
// Singleton 테스트
// ======================================================================

TEST_F(ConfigManagerTest, Singleton_ReturnsSameInstance) {
    ConfigManager& instance1 = ConfigManager::getInstance();
    ConfigManager& instance2 = ConfigManager::getInstance();

    EXPECT_EQ(&instance1, &instance2);
}

// ======================================================================
// 설정값 읽기/쓰기 테스트
// ======================================================================

TEST_F(ConfigManagerTest, SetGetString) {
    ConfigManager::getInstance().setString("app.name", "KooMeshPrepost");

    std::string value = ConfigManager::getInstance().getString("app.name");
    EXPECT_EQ("KooMeshPrepost", value);
}

TEST_F(ConfigManagerTest, SetGetInt) {
    ConfigManager::getInstance().setInt("ui.window.width", 1024);

    int value = ConfigManager::getInstance().getInt("ui.window.width");
    EXPECT_EQ(1024, value);
}

TEST_F(ConfigManagerTest, SetGetDouble) {
    ConfigManager::getInstance().setDouble("renderer.fov", 45.5);

    double value = ConfigManager::getInstance().getDouble("renderer.fov");
    EXPECT_DOUBLE_EQ(45.5, value);
}

TEST_F(ConfigManagerTest, SetGetBool) {
    ConfigManager::getInstance().setBool("debug.enabled", true);

    bool value = ConfigManager::getInstance().getBool("debug.enabled");
    EXPECT_TRUE(value);
}

TEST_F(ConfigManagerTest, SetGetArray) {
    std::vector<std::string> recentFiles = {"file1.k", "file2.k", "file3.k"};
    ConfigManager::getInstance().setArray("ui.recent_files", recentFiles);

    std::vector<std::string> value = ConfigManager::getInstance().getArray("ui.recent_files");
    EXPECT_EQ(recentFiles, value);
}

// ======================================================================
// 기본값 테스트
// ======================================================================

TEST_F(ConfigManagerTest, GetString_WithDefaultValue) {
    std::string value = ConfigManager::getInstance().getString("nonexistent.key", "default");
    EXPECT_EQ("default", value);
}

TEST_F(ConfigManagerTest, GetInt_WithDefaultValue) {
    int value = ConfigManager::getInstance().getInt("nonexistent.key", 42);
    EXPECT_EQ(42, value);
}

TEST_F(ConfigManagerTest, GetDouble_WithDefaultValue) {
    double value = ConfigManager::getInstance().getDouble("nonexistent.key", 3.14);
    EXPECT_DOUBLE_EQ(3.14, value);
}

TEST_F(ConfigManagerTest, GetBool_WithDefaultValue) {
    bool value = ConfigManager::getInstance().getBool("nonexistent.key", true);
    EXPECT_TRUE(value);
}

TEST_F(ConfigManagerTest, GetArray_WithDefaultValue) {
    std::vector<std::string> defaultValue = {"default1", "default2"};
    std::vector<std::string> value = ConfigManager::getInstance().getArray("nonexistent.key", defaultValue);
    EXPECT_EQ(defaultValue, value);
}

// ======================================================================
// 파일 I/O 테스트
// ======================================================================

TEST_F(ConfigManagerTest, SaveToFile_CreatesValidJson) {
    ConfigManager::getInstance().setString("app.name", "KooMeshPrepost");
    ConfigManager::getInstance().setInt("ui.window.width", 1024);
    ConfigManager::getInstance().setBool("debug.enabled", false);

    bool saved = ConfigManager::getInstance().saveToFile(testConfigFile);
    EXPECT_TRUE(saved);

    std::string content = readFile(testConfigFile);
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("\"app.name\""), std::string::npos);
    EXPECT_NE(content.find("\"KooMeshPrepost\""), std::string::npos);
    EXPECT_NE(content.find("\"ui.window.width\""), std::string::npos);
    EXPECT_NE(content.find("1024"), std::string::npos);
}

TEST_F(ConfigManagerTest, LoadFromFile_ReadsValidJson) {
    std::string jsonContent = R"({
  "app.name": "KooMeshPrepost",
  "ui.window.width": 1024,
  "ui.window.height": 768,
  "debug.enabled": true,
  "renderer.fov": 45.5,
  "ui.recent_files": ["file1.k", "file2.k", "file3.k"]
})";

    writeFile(testConfigFile, jsonContent);

    bool loaded = ConfigManager::getInstance().loadFromFile(testConfigFile);
    EXPECT_TRUE(loaded);

    EXPECT_EQ("KooMeshPrepost", ConfigManager::getInstance().getString("app.name"));
    EXPECT_EQ(1024, ConfigManager::getInstance().getInt("ui.window.width"));
    EXPECT_EQ(768, ConfigManager::getInstance().getInt("ui.window.height"));
    EXPECT_TRUE(ConfigManager::getInstance().getBool("debug.enabled"));
    EXPECT_DOUBLE_EQ(45.5, ConfigManager::getInstance().getDouble("renderer.fov"));

    std::vector<std::string> expectedFiles = {"file1.k", "file2.k", "file3.k"};
    EXPECT_EQ(expectedFiles, ConfigManager::getInstance().getArray("ui.recent_files"));
}

TEST_F(ConfigManagerTest, LoadFromFile_NonexistentFile) {
    bool loaded = ConfigManager::getInstance().loadFromFile("nonexistent_file.json");
    EXPECT_FALSE(loaded);
}

TEST_F(ConfigManagerTest, RoundTrip_SaveAndLoad) {
    // 설정값 설정
    ConfigManager::getInstance().setString("app.name", "KooMeshPrepost");
    ConfigManager::getInstance().setInt("ui.window.width", 1280);
    ConfigManager::getInstance().setDouble("renderer.fov", 60.0);
    ConfigManager::getInstance().setBool("debug.enabled", true);
    std::vector<std::string> files = {"file1.k", "file2.k"};
    ConfigManager::getInstance().setArray("ui.recent_files", files);

    // 저장
    bool saved = ConfigManager::getInstance().saveToFile(testConfigFile);
    EXPECT_TRUE(saved);

    // 초기화
    ConfigManager::getInstance().clear();

    // 로드
    bool loaded = ConfigManager::getInstance().loadFromFile(testConfigFile);
    EXPECT_TRUE(loaded);

    // 검증
    EXPECT_EQ("KooMeshPrepost", ConfigManager::getInstance().getString("app.name"));
    EXPECT_EQ(1280, ConfigManager::getInstance().getInt("ui.window.width"));
    EXPECT_DOUBLE_EQ(60.0, ConfigManager::getInstance().getDouble("renderer.fov"));
    EXPECT_TRUE(ConfigManager::getInstance().getBool("debug.enabled"));
    EXPECT_EQ(files, ConfigManager::getInstance().getArray("ui.recent_files"));
}

// ======================================================================
// 유틸리티 메서드 테스트
// ======================================================================

TEST_F(ConfigManagerTest, HasKey_ExistingKey) {
    ConfigManager::getInstance().setString("test.key", "value");
    EXPECT_TRUE(ConfigManager::getInstance().hasKey("test.key"));
}

TEST_F(ConfigManagerTest, HasKey_NonexistentKey) {
    EXPECT_FALSE(ConfigManager::getInstance().hasKey("nonexistent.key"));
}

TEST_F(ConfigManagerTest, RemoveKey_ExistingKey) {
    ConfigManager::getInstance().setString("test.key", "value");
    EXPECT_TRUE(ConfigManager::getInstance().hasKey("test.key"));

    bool removed = ConfigManager::getInstance().removeKey("test.key");
    EXPECT_TRUE(removed);
    EXPECT_FALSE(ConfigManager::getInstance().hasKey("test.key"));
}

TEST_F(ConfigManagerTest, RemoveKey_NonexistentKey) {
    bool removed = ConfigManager::getInstance().removeKey("nonexistent.key");
    EXPECT_FALSE(removed);
}

TEST_F(ConfigManagerTest, Clear_RemovesAllKeys) {
    ConfigManager::getInstance().setString("key1", "value1");
    ConfigManager::getInstance().setString("key2", "value2");
    ConfigManager::getInstance().setInt("key3", 123);

    ConfigManager::getInstance().clear();

    EXPECT_FALSE(ConfigManager::getInstance().hasKey("key1"));
    EXPECT_FALSE(ConfigManager::getInstance().hasKey("key2"));
    EXPECT_FALSE(ConfigManager::getInstance().hasKey("key3"));
}

TEST_F(ConfigManagerTest, GetAllKeys) {
    ConfigManager::getInstance().setString("ui.window.title", "KooMesh");
    ConfigManager::getInstance().setInt("ui.window.width", 1024);
    ConfigManager::getInstance().setBool("debug.enabled", true);

    std::vector<std::string> keys = ConfigManager::getInstance().getAllKeys();
    EXPECT_EQ(3, keys.size());

    EXPECT_NE(std::find(keys.begin(), keys.end(), "ui.window.title"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "ui.window.width"), keys.end());
    EXPECT_NE(std::find(keys.begin(), keys.end(), "debug.enabled"), keys.end());
}

TEST_F(ConfigManagerTest, GetKeysWithPrefix) {
    ConfigManager::getInstance().setString("ui.window.title", "KooMesh");
    ConfigManager::getInstance().setInt("ui.window.width", 1024);
    ConfigManager::getInstance().setInt("ui.toolbar.height", 32);
    ConfigManager::getInstance().setBool("debug.enabled", true);

    std::vector<std::string> uiKeys = ConfigManager::getInstance().getKeysWithPrefix("ui.");
    EXPECT_EQ(3, uiKeys.size());

    std::vector<std::string> windowKeys = ConfigManager::getInstance().getKeysWithPrefix("ui.window.");
    EXPECT_EQ(2, windowKeys.size());

    std::vector<std::string> debugKeys = ConfigManager::getInstance().getKeysWithPrefix("debug.");
    EXPECT_EQ(1, debugKeys.size());
}

// ======================================================================
// 자동 저장 테스트
// ======================================================================

TEST_F(ConfigManagerTest, AutoSave_Disabled) {
    ConfigManager::getInstance().setConfigFilePath(testConfigFile);
    ConfigManager::getInstance().setAutoSave(false);

    ConfigManager::getInstance().setString("test.key", "value");

    // 파일이 생성되지 않아야 함
    std::ifstream file(testConfigFile);
    EXPECT_FALSE(file.good());
}

TEST_F(ConfigManagerTest, AutoSave_Enabled) {
    ConfigManager::getInstance().setConfigFilePath(testConfigFile);
    ConfigManager::getInstance().setAutoSave(true);

    ConfigManager::getInstance().setString("test.key", "value");

    // 파일이 자동으로 생성되어야 함
    std::ifstream file(testConfigFile);
    EXPECT_TRUE(file.good());
    file.close();

    // 내용 확인
    std::string content = readFile(testConfigFile);
    EXPECT_NE(content.find("\"test.key\""), std::string::npos);
    EXPECT_NE(content.find("\"value\""), std::string::npos);
}

// ======================================================================
// 타입 변환 테스트
// ======================================================================

TEST_F(ConfigManagerTest, TypeConversion_IntToDouble) {
    ConfigManager::getInstance().setInt("number", 42);

    // 정수를 실수로 읽기
    double value = ConfigManager::getInstance().getDouble("number");
    EXPECT_DOUBLE_EQ(42.0, value);
}

TEST_F(ConfigManagerTest, TypeConversion_BooleanVariants) {
    ConfigManager::getInstance().setString("bool1", "true");
    ConfigManager::getInstance().setString("bool2", "false");
    ConfigManager::getInstance().setString("bool3", "1");
    ConfigManager::getInstance().setString("bool4", "0");
    ConfigManager::getInstance().setString("bool5", "yes");
    ConfigManager::getInstance().setString("bool6", "no");

    EXPECT_TRUE(ConfigManager::getInstance().getBool("bool1"));
    EXPECT_FALSE(ConfigManager::getInstance().getBool("bool2"));
    EXPECT_TRUE(ConfigManager::getInstance().getBool("bool3"));
    EXPECT_FALSE(ConfigManager::getInstance().getBool("bool4"));
    EXPECT_TRUE(ConfigManager::getInstance().getBool("bool5"));
    EXPECT_FALSE(ConfigManager::getInstance().getBool("bool6"));
}

// ======================================================================
// 스레드 안전성 테스트
// ======================================================================

TEST_F(ConfigManagerTest, ThreadSafety_ConcurrentWrites) {
    const int numThreads = 10;
    const int numOperations = 100;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string key = "thread" + std::to_string(i) + ".key" + std::to_string(j);
                ConfigManager::getInstance().setInt(key, i * numOperations + j);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 모든 값 확인
    for (int i = 0; i < numThreads; ++i) {
        for (int j = 0; j < numOperations; ++j) {
            std::string key = "thread" + std::to_string(i) + ".key" + std::to_string(j);
            EXPECT_TRUE(ConfigManager::getInstance().hasKey(key));
            int value = ConfigManager::getInstance().getInt(key);
            EXPECT_EQ(i * numOperations + j, value);
        }
    }
}

TEST_F(ConfigManagerTest, ThreadSafety_ConcurrentReadsAndWrites) {
    ConfigManager::getInstance().setInt("shared.counter", 0);

    const int numThreads = 5;
    const int numOperations = 100;

    std::vector<std::thread> threads;

    // Writer threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string key = "thread" + std::to_string(i);
                ConfigManager::getInstance().setInt(key, j);
            }
        });
    }

    // Reader threads
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([i, numOperations]() {
            for (int j = 0; j < numOperations; ++j) {
                std::string key = "thread" + std::to_string(i);
                int value = ConfigManager::getInstance().getInt(key, -1);
                // 값이 유효한지만 확인 (경쟁 조건으로 인해 정확한 값 예측 불가)
                EXPECT_GE(value, -1);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }
}

// ======================================================================
// 엣지 케이스 테스트
// ======================================================================

TEST_F(ConfigManagerTest, EdgeCase_EmptyString) {
    ConfigManager::getInstance().setString("empty.key", "");
    std::string value = ConfigManager::getInstance().getString("empty.key");
    EXPECT_EQ("", value);
}

TEST_F(ConfigManagerTest, EdgeCase_EmptyArray) {
    std::vector<std::string> emptyArray;
    ConfigManager::getInstance().setArray("empty.array", emptyArray);

    std::vector<std::string> value = ConfigManager::getInstance().getArray("empty.array");
    EXPECT_TRUE(value.empty());
}

TEST_F(ConfigManagerTest, EdgeCase_SpecialCharactersInString) {
    std::string specialString = "Hello \"World\"\n\t\\Special";
    ConfigManager::getInstance().setString("special.string", specialString);

    // 저장 및 로드
    ConfigManager::getInstance().saveToFile(testConfigFile);
    ConfigManager::getInstance().clear();
    ConfigManager::getInstance().loadFromFile(testConfigFile);

    std::string value = ConfigManager::getInstance().getString("special.string");
    EXPECT_EQ(specialString, value);
}

TEST_F(ConfigManagerTest, EdgeCase_LargeNumber) {
    int largeInt = 2147483647; // INT_MAX
    ConfigManager::getInstance().setInt("large.int", largeInt);

    int value = ConfigManager::getInstance().getInt("large.int");
    EXPECT_EQ(largeInt, value);
}

TEST_F(ConfigManagerTest, EdgeCase_NegativeNumber) {
    ConfigManager::getInstance().setInt("negative.int", -12345);
    ConfigManager::getInstance().setDouble("negative.double", -3.14159);

    EXPECT_EQ(-12345, ConfigManager::getInstance().getInt("negative.int"));
    EXPECT_DOUBLE_EQ(-3.14159, ConfigManager::getInstance().getDouble("negative.double"));
}

TEST_F(ConfigManagerTest, EdgeCase_VeryLongKey) {
    std::string longKey = "very.long.nested.key.with.many.levels.to.test.deep.nesting.capability";
    ConfigManager::getInstance().setString(longKey, "value");

    EXPECT_EQ("value", ConfigManager::getInstance().getString(longKey));
}

// ======================================================================
// 메인 함수
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
