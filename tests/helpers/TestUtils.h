#pragma once

#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <cmath>
#include <string>
#include <vector>

namespace koomesh {
namespace test {

/**
 * @brief 벡터 비교 유틸리티
 */
class VectorAssert {
public:
    /**
     * @brief 두 Eigen 벡터가 거의 같은지 확인
     */
    static testing::AssertionResult Near(const Eigen::Vector3d& v1,
                                         const Eigen::Vector3d& v2,
                                         double tolerance = 1e-10) {
        double distance = (v1 - v2).norm();
        if (distance <= tolerance) {
            return testing::AssertionSuccess();
        }

        return testing::AssertionFailure()
            << "Vectors are not near:\n"
            << "  v1 = (" << v1.x() << ", " << v1.y() << ", " << v1.z() << ")\n"
            << "  v2 = (" << v2.x() << ", " << v2.y() << ", " << v2.z() << ")\n"
            << "  distance = " << distance << " (tolerance = " << tolerance << ")";
    }

    /**
     * @brief Eigen 벡터가 특정 값인지 확인
     */
    static testing::AssertionResult Equal(const Eigen::Vector3d& v,
                                          double x, double y, double z,
                                          double tolerance = 1e-10) {
        return Near(v, Eigen::Vector3d(x, y, z), tolerance);
    }
};

/**
 * @brief 부동소수점 비교 유틸리티
 */
class FloatAssert {
public:
    /**
     * @brief 두 부동소수점이 거의 같은지 확인
     */
    static testing::AssertionResult Near(double a, double b, double tolerance = 1e-10) {
        double diff = std::abs(a - b);
        if (diff <= tolerance) {
            return testing::AssertionSuccess();
        }

        return testing::AssertionFailure()
            << "Values are not near:\n"
            << "  a = " << a << "\n"
            << "  b = " << b << "\n"
            << "  diff = " << diff << " (tolerance = " << tolerance << ")";
    }
};

/**
 * @brief 문자열 유틸리티
 */
class StringUtils {
public:
    /**
     * @brief 문자열에 부분 문자열이 포함되어 있는지 확인
     */
    static testing::AssertionResult Contains(const std::string& str,
                                             const std::string& substr) {
        if (str.find(substr) != std::string::npos) {
            return testing::AssertionSuccess();
        }

        return testing::AssertionFailure()
            << "String does not contain substring:\n"
            << "  String: \"" << str << "\"\n"
            << "  Substring: \"" << substr << "\"";
    }

    /**
     * @brief 문자열이 특정 패턴으로 시작하는지 확인
     */
    static testing::AssertionResult StartsWith(const std::string& str,
                                               const std::string& prefix) {
        if (str.find(prefix) == 0) {
            return testing::AssertionSuccess();
        }

        return testing::AssertionFailure()
            << "String does not start with prefix:\n"
            << "  String: \"" << str << "\"\n"
            << "  Prefix: \"" << prefix << "\"";
    }
};

/**
 * @brief 파일 시스템 유틸리티
 */
class FileUtils {
public:
    /**
     * @brief 임시 파일 경로 생성
     */
    static std::string tempFilePath(const std::string& prefix = "test_") {
        static int counter = 0;
        return prefix + std::to_string(++counter) + ".tmp";
    }

    /**
     * @brief 파일 존재 여부 확인
     */
    static bool exists(const std::string& path) {
        std::ifstream file(path);
        return file.good();
    }

    /**
     * @brief 파일 읽기
     */
    static std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return "";
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        return content;
    }

    /**
     * @brief 파일 쓰기
     */
    static bool writeFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) {
            return false;
        }

        file << content;
        return true;
    }

    /**
     * @brief 파일 삭제
     */
    static void remove(const std::string& path) {
        std::remove(path.c_str());
    }
};

/**
 * @brief 컬렉션 유틸리티
 */
class CollectionUtils {
public:
    /**
     * @brief 벡터에 특정 값이 포함되어 있는지 확인
     */
    template<typename T>
    static testing::AssertionResult Contains(const std::vector<T>& vec, const T& value) {
        auto it = std::find(vec.begin(), vec.end(), value);
        if (it != vec.end()) {
            return testing::AssertionSuccess();
        }

        return testing::AssertionFailure()
            << "Vector does not contain value";
    }

    /**
     * @brief 두 벡터가 같은 요소를 가지는지 확인 (순서 무관)
     */
    template<typename T>
    static testing::AssertionResult UnorderedEqual(const std::vector<T>& v1,
                                                    const std::vector<T>& v2) {
        if (v1.size() != v2.size()) {
            return testing::AssertionFailure()
                << "Vectors have different sizes: " << v1.size() << " vs " << v2.size();
        }

        std::vector<T> sorted1 = v1;
        std::vector<T> sorted2 = v2;

        std::sort(sorted1.begin(), sorted1.end());
        std::sort(sorted2.begin(), sorted2.end());

        if (sorted1 == sorted2) {
            return testing::AssertionSuccess();
        }

        return testing::AssertionFailure()
            << "Vectors do not contain the same elements";
    }
};

/**
 * @brief 성능 측정 유틸리티
 */
class PerformanceTimer {
public:
    PerformanceTimer() : m_start(std::chrono::high_resolution_clock::now()) {}

    /**
     * @brief 경과 시간 (밀리초)
     */
    long long elapsedMilliseconds() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
    }

    /**
     * @brief 경과 시간 (마이크로초)
     */
    long long elapsedMicroseconds() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
    }

    /**
     * @brief 타이머 리셋
     */
    void reset() {
        m_start = std::chrono::high_resolution_clock::now();
    }

private:
    std::chrono::high_resolution_clock::time_point m_start;
};

/**
 * @brief RAII 스타일 성능 측정
 */
class ScopedTimer {
public:
    ScopedTimer(const std::string& name) : m_name(name) {}

    ~ScopedTimer() {
        std::cout << m_name << ": " << m_timer.elapsedMilliseconds() << " ms\n";
    }

private:
    std::string m_name;
    PerformanceTimer m_timer;
};

} // namespace test
} // namespace koomesh

// 편의 매크로
#define EXPECT_VECTOR_NEAR(v1, v2, tolerance) \
    EXPECT_TRUE(koomesh::test::VectorAssert::Near(v1, v2, tolerance))

#define EXPECT_VECTOR_EQ(v, x, y, z) \
    EXPECT_TRUE(koomesh::test::VectorAssert::Equal(v, x, y, z))

#define EXPECT_FLOAT_NEAR(a, b, tolerance) \
    EXPECT_TRUE(koomesh::test::FloatAssert::Near(a, b, tolerance))

#define EXPECT_STRING_CONTAINS(str, substr) \
    EXPECT_TRUE(koomesh::test::StringUtils::Contains(str, substr))

#define EXPECT_COLLECTION_CONTAINS(vec, value) \
    EXPECT_TRUE(koomesh::test::CollectionUtils::Contains(vec, value))

#define SCOPED_TIMER(name) \
    koomesh::test::ScopedTimer _timer_##__LINE__(name)
