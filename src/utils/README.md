# Utils Module

유틸리티 및 헬퍼 함수/클래스 모음입니다.

## 구조

```
utils/
├── Logger.cpp               # 로깅 시스템
├── BoundingBox.cpp          # 경계 상자 유틸리티
├── Timer.cpp                # 성능 측정 타이머
├── FileUtils.cpp            # 파일 유틸리티
├── StringUtils.cpp          # 문자열 유틸리티
├── MathUtils.cpp            # 수학 유틸리티
└── MemoryProfiler.cpp       # 메모리 프로파일러
```

## Logger

다중 레벨 로깅 시스템:

```cpp
class Logger {
public:
    enum Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    // 싱글톤
    static Logger& getInstance();

    // 로깅
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void critical(const std::string& message);

    // 설정
    void setLevel(Level level);
    void setOutputFile(const std::string& filepath);
    void setConsoleOutput(bool enabled);

    // 포맷된 로깅
    template<typename... Args>
    static void debugf(const std::string& format, Args... args);

private:
    Logger() = default;
    void log(Level level, const std::string& message);

    Level m_level = Level::INFO;
    std::ofstream m_file;
    bool m_consoleOutput = true;
    std::mutex m_mutex; // 스레드 안전성
};
```

### 사용 예제

```cpp
#include "utils/Logger.h"

// 기본 로깅
Logger::info("Application started");
Logger::debug("Loading file: mesh.k");
Logger::warning("Large file detected, this may take a while");
Logger::error("Failed to parse line 1234");
Logger::critical("Out of memory!");

// 포맷된 로깅
Logger::infof("Loaded %d nodes and %d elements", nodeCount, elementCount);
Logger::debugf("Octree built in %.2f seconds", buildTime);

// 설정
auto& logger = Logger::getInstance();
logger.setLevel(Logger::DEBUG);
logger.setOutputFile("app.log");
```

**출력 형식**:
```
[2024-01-15 14:32:10] [INFO] Application started
[2024-01-15 14:32:11] [DEBUG] Loading file: mesh.k
[2024-01-15 14:32:15] [WARNING] Large file detected, this may take a while
[2024-01-15 14:32:20] [ERROR] Failed to parse line 1234
[2024-01-15 14:32:21] [CRITICAL] Out of memory!
```

## Timer

성능 측정 타이머:

```cpp
class Timer {
public:
    Timer(const std::string& name = "Timer");
    ~Timer(); // 자동으로 시간 출력

    void start();
    void stop();
    double elapsed() const; // 밀리초
    void reset();

private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
    bool m_running;
};
```

### 사용 예제

```cpp
// RAII 방식 (자동 측정)
{
    Timer timer("File Loading");
    loadFile("mesh.k", mesh);
    // 스코프 종료 시 자동으로 시간 출력
}

// 수동 측정
Timer timer;
timer.start();
processData();
timer.stop();
std::cout << "Processing took " << timer.elapsed() << " ms\n";
```

## BoundingBox

3D 경계 상자 유틸리티:

```cpp
class BoundingBox {
public:
    BoundingBox();
    BoundingBox(double minX, double minY, double minZ,
                double maxX, double maxY, double maxZ);

    // 기본 연산
    void expand(const Eigen::Vector3d& point);
    void expand(const BoundingBox& other);
    bool contains(const Eigen::Vector3d& point) const;
    bool intersects(const BoundingBox& other) const;

    // 조회
    Eigen::Vector3d center() const;
    Eigen::Vector3d size() const;
    double volume() const;
    double diagonal() const;

    // 분할 (Octree용)
    std::array<BoundingBox, 8> subdivide() const;

    // Getters
    double minX() const { return m_min.x(); }
    // ... 기타

private:
    Eigen::Vector3d m_min;
    Eigen::Vector3d m_max;
};
```

## FileUtils

파일 유틸리티:

```cpp
class FileUtils {
public:
    // 파일 존재 확인
    static bool exists(const std::string& filepath);

    // 파일 크기
    static size_t fileSize(const std::string& filepath);

    // 확장자 추출
    static std::string getExtension(const std::string& filepath);

    // 디렉토리 추출
    static std::string getDirectory(const std::string& filepath);

    // 파일명 추출
    static std::string getFilename(const std::string& filepath);

    // 경로 결합
    static std::string joinPath(const std::string& dir, const std::string& filename);

    // 디렉토리 생성
    static bool createDirectory(const std::string& dirpath);

    // 파일 읽기
    static std::string readTextFile(const std::string& filepath);

    // 파일 쓰기
    static bool writeTextFile(const std::string& filepath, const std::string& content);
};
```

## StringUtils

문자열 유틸리티:

```cpp
class StringUtils {
public:
    // Trim (공백 제거)
    static std::string trim(const std::string& str);
    static std::string ltrim(const std::string& str);
    static std::string rtrim(const std::string& str);

    // Split
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter);

    // Join
    static std::string join(const std::vector<std::string>& parts, const std::string& separator);

    // 대소문자 변환
    static std::string toLower(const std::string& str);
    static std::string toUpper(const std::string& str);

    // 시작/끝 확인
    static bool startsWith(const std::string& str, const std::string& prefix);
    static bool endsWith(const std::string& str, const std::string& suffix);

    // 숫자 변환
    static int toInt(const std::string& str);
    static double toDouble(const std::string& str);
    static std::string toString(int value);
    static std::string toString(double value, int precision = 6);

    // 포맷
    template<typename... Args>
    static std::string format(const std::string& fmt, Args... args);
};
```

### 사용 예제

```cpp
// Trim
std::string s = "  hello  ";
StringUtils::trim(s); // "hello"

// Split
std::string line = "1,2,3,4,5";
auto parts = StringUtils::split(line, ','); // {"1", "2", "3", "4", "5"}

// Join
std::vector<std::string> words = {"Hello", "World"};
StringUtils::join(words, " "); // "Hello World"

// 포맷
StringUtils::format("Node %d at (%.2f, %.2f, %.2f)", id, x, y, z);
```

## MathUtils

수학 유틸리티:

```cpp
class MathUtils {
public:
    // 상수
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double EPSILON = 1e-10;

    // 비교
    static bool almostEqual(double a, double b, double epsilon = EPSILON);
    static bool isZero(double value, double epsilon = EPSILON);

    // 각도 변환
    static double degToRad(double degrees);
    static double radToDeg(double radians);

    // 클램핑
    static double clamp(double value, double min, double max);
    static int clamp(int value, int min, int max);

    // 선형 보간
    static double lerp(double a, double b, double t);
    static Eigen::Vector3d lerp(const Eigen::Vector3d& a, const Eigen::Vector3d& b, double t);

    // 벡터 연산
    static double distance(const Eigen::Vector3d& a, const Eigen::Vector3d& b);
    static double angle(const Eigen::Vector3d& a, const Eigen::Vector3d& b); // 라디안
};
```

## MemoryProfiler

메모리 프로파일러:

```cpp
class MemoryProfiler {
public:
    // 현재 메모리 사용량
    static size_t currentUsage();

    // 피크 메모리 사용량
    static size_t peakUsage();

    // 메모리 할당 추적
    static void startTracking();
    static void stopTracking();

    // 리포트
    static void printReport();

    // 스냅샷
    struct Snapshot {
        size_t totalAllocated;
        size_t totalFreed;
        size_t currentUsage;
        std::chrono::system_clock::time_point timestamp;
    };

    static Snapshot takeSnapshot();
    static void compareSnapshots(const Snapshot& before, const Snapshot& after);
};
```

### 사용 예제

```cpp
// 메모리 추적
MemoryProfiler::startTracking();

auto snapshot1 = MemoryProfiler::takeSnapshot();

// 메모리 사용하는 작업
Mesh mesh;
loadLargeFile("mesh.k", mesh);

auto snapshot2 = MemoryProfiler::takeSnapshot();

MemoryProfiler::compareSnapshots(snapshot1, snapshot2);
// Output: Allocated: 1.2 GB, Current usage: 1.1 GB

MemoryProfiler::stopTracking();
MemoryProfiler::printReport();
```

## 매크로 유틸리티

```cpp
// utils/Macros.h

// 디버그 빌드에서만 실행
#ifdef DEBUG
#define DEBUG_ONLY(x) x
#else
#define DEBUG_ONLY(x)
#endif

// Assert
#define ASSERT(condition, message) \
    if (!(condition)) { \
        Logger::critical(std::string("Assertion failed: ") + message); \
        std::abort(); \
    }

// 성능 측정
#define MEASURE_TIME(name, code) \
    { \
        Timer timer(name); \
        code; \
    }

// 미구현 기능
#define NOT_IMPLEMENTED() \
    throw std::runtime_error(std::string(__func__) + " not implemented")
```

### 사용 예제

```cpp
ASSERT(mesh.nodeCount() > 0, "Mesh has no nodes");

MEASURE_TIME("Octree Build", {
    octree.build(mesh);
});

DEBUG_ONLY(
    Logger::debug("Debug information...");
)
```

## 테스트
- `tests/unit/test_logger.cpp`
- `tests/unit/test_bounding_box.cpp`
- `tests/unit/test_string_utils.cpp`
