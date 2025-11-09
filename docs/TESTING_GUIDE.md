# Testing Guide

## 개요

KooMeshPrepost는 Google Test 프레임워크를 사용하여 포괄적인 단위 테스트 및 통합 테스트를 수행합니다. 이 가이드는 효과적인 테스트 작성 방법과 모범 사례를 제공합니다.

## 목차

1. [테스트 구조](#테스트-구조)
2. [테스트 작성 가이드](#테스트-작성-가이드)
3. [테스트 픽스처](#테스트-픽스처)
4. [Assertion 가이드](#assertion-가이드)
5. [테스트 커버리지](#테스트-커버리지)
6. [모킹과 페이크](#모킹과-페이크)
7. [성능 테스트](#성능-테스트)
8. [테스트 실행](#테스트-실행)
9. [CI/CD 통합](#cicd-통합)

---

## 테스트 구조

### 디렉토리 구조

```
tests/
├── unit/                 # 단위 테스트
│   ├── test_logger.cpp
│   ├── test_config.cpp
│   ├── test_exception.cpp
│   ├── test_core.cpp
│   └── test_memory_pool.cpp
├── integration/          # 통합 테스트 (향후)
│   ├── test_file_io.cpp
│   └── test_mesh_loading.cpp
└── helpers/              # 테스트 헬퍼 (향후)
    ├── MeshBuilder.h
    └── TestFixtures.h
```

### 테스트 파일 명명 규칙

- 단위 테스트: `test_<module_name>.cpp`
- 통합 테스트: `test_<feature>_integration.cpp`
- 각 테스트 파일은 하나의 모듈 또는 기능을 테스트

---

## 테스트 작성 가이드

### 1. 기본 테스트 구조

```cpp
#include <gtest/gtest.h>
#include "core/Node.h"

using namespace koomesh::core;

TEST(NodeTest, Constructor_SetsPositionCorrectly) {
    // Arrange (준비)
    double x = 1.0, y = 2.0, z = 3.0;

    // Act (실행)
    Node node(1, x, y, z);

    // Assert (검증)
    EXPECT_EQ(1, node.id());
    EXPECT_DOUBLE_EQ(x, node.x());
    EXPECT_DOUBLE_EQ(y, node.y());
    EXPECT_DOUBLE_EQ(z, node.z());
}
```

### 2. AAA 패턴 (Arrange-Act-Assert)

모든 테스트는 AAA 패턴을 따릅니다:

```cpp
TEST(MeshTest, AddNode_IncreasesNodeCount) {
    // Arrange: 테스트 준비
    Mesh mesh;
    Node node(1, 0.0, 0.0, 0.0);

    // Act: 테스트 실행
    mesh.addNode(node);

    // Assert: 결과 검증
    EXPECT_EQ(1, mesh.nodeCount());
}
```

### 3. 테스트 이름 규칙

**형식**: `MethodName_Scenario_ExpectedBehavior`

```cpp
// Good examples
TEST(NodeTest, DistanceTo_TwoNodes_ReturnsCorrectDistance)
TEST(MeshTest, AddNode_DuplicateID_ThrowsException)
TEST(ConfigManager, GetInt_NonexistentKey_ReturnsDefault)

// Bad examples
TEST(NodeTest, Test1)  // 너무 모호함
TEST(NodeTest, ItWorks)  // 무엇이 작동하는지 불명확
```

### 4. 한 테스트는 한 가지만 검증

```cpp
// Bad: 여러 것을 동시에 테스트
TEST(NodeTest, AllNodeOperations) {
    Node node(1, 0, 0, 0);
    node.translate(Eigen::Vector3d(1, 0, 0));
    EXPECT_EQ(1.0, node.x());
    node.scale(2.0);
    EXPECT_EQ(2.0, node.x());
    // ... 더 많은 테스트
}

// Good: 각 기능별로 분리
TEST(NodeTest, Translate_ByOffset_UpdatesPosition) {
    Node node(1, 0, 0, 0);
    node.translate(Eigen::Vector3d(1, 0, 0));
    EXPECT_EQ(1.0, node.x());
}

TEST(NodeTest, Scale_ByFactor_ScalesPosition) {
    Node node(1, 1, 0, 0);
    node.scale(2.0);
    EXPECT_EQ(2.0, node.x());
}
```

---

## 테스트 픽스처

### 1. 기본 픽스처

반복되는 설정 코드를 픽스처로 추출:

```cpp
class MeshTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 각 테스트 전에 실행
        mesh = std::make_unique<Mesh>();
        mesh->addNode(Node(1, 0.0, 0.0, 0.0));
        mesh->addNode(Node(2, 1.0, 0.0, 0.0));
    }

    void TearDown() override {
        // 각 테스트 후에 실행
        mesh.reset();
    }

    std::unique_ptr<Mesh> mesh;
};

TEST_F(MeshTest, NodeCount_AfterSetup_IsTwo) {
    EXPECT_EQ(2, mesh->nodeCount());
}

TEST_F(MeshTest, GetNode_ValidID_ReturnsNode) {
    const Node* node = mesh->getNode(1);
    ASSERT_NE(nullptr, node);
    EXPECT_EQ(1, node->id());
}
```

### 2. 매개변수화된 테스트

여러 입력값으로 동일한 테스트:

```cpp
class NodeDistanceTest : public ::testing::TestWithParam<std::tuple<double, double, double, double>> {
};

TEST_P(NodeDistanceTest, DistanceTo_VariousPositions_ReturnsCorrectDistance) {
    auto [x, y, z, expectedDistance] = GetParam();

    Node node1(1, 0.0, 0.0, 0.0);
    Node node2(2, x, y, z);

    double distance = node1.distanceTo(node2);

    EXPECT_NEAR(expectedDistance, distance, 1e-10);
}

INSTANTIATE_TEST_SUITE_P(
    DistanceTests,
    NodeDistanceTest,
    ::testing::Values(
        std::make_tuple(3.0, 4.0, 0.0, 5.0),     // 3-4-5 삼각형
        std::make_tuple(1.0, 0.0, 0.0, 1.0),     // X축 방향
        std::make_tuple(0.0, 1.0, 0.0, 1.0),     // Y축 방향
        std::make_tuple(0.0, 0.0, 1.0, 1.0)      // Z축 방향
    )
);
```

---

## Assertion 가이드

### 1. EXPECT vs ASSERT

- `EXPECT_*`: 실패해도 테스트 계속 실행
- `ASSERT_*`: 실패하면 즉시 중단 (이후 코드가 실행되면 위험한 경우)

```cpp
TEST(NodeTest, GetNode_ValidID_ReturnsNode) {
    Mesh mesh;
    mesh.addNode(Node(1, 0, 0, 0));

    const Node* node = mesh.getNode(1);

    // ASSERT: nullptr이면 다음 코드가 크래시
    ASSERT_NE(nullptr, node);

    // EXPECT: 여러 속성 검증
    EXPECT_EQ(1, node->id());
    EXPECT_DOUBLE_EQ(0.0, node->x());
}
```

### 2. 일반적인 Assertion

```cpp
// 동등 비교
EXPECT_EQ(expected, actual);
EXPECT_NE(val1, val2);

// 부동소수점 비교
EXPECT_DOUBLE_EQ(3.14, pi);          // 정확한 비교
EXPECT_NEAR(3.14, pi, 0.01);         // 오차 범위 내

// 불리언
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// 포인터
EXPECT_EQ(nullptr, ptr);
EXPECT_NE(nullptr, ptr);

// 문자열
EXPECT_STREQ("hello", str);
EXPECT_STRNE("hello", str);

// 예외
EXPECT_THROW(statement, ExceptionType);
EXPECT_NO_THROW(statement);
EXPECT_ANY_THROW(statement);

// 조건
EXPECT_LT(val1, val2);  // Less Than
EXPECT_LE(val1, val2);  // Less or Equal
EXPECT_GT(val1, val2);  // Greater Than
EXPECT_GE(val1, val2);  // Greater or Equal
```

### 3. 커스텀 Assertion

```cpp
// 커스텀 매처
testing::AssertionResult IsNodeAt(const Node& node, double x, double y, double z) {
    if (std::abs(node.x() - x) < 1e-10 &&
        std::abs(node.y() - y) < 1e-10 &&
        std::abs(node.z() - z) < 1e-10) {
        return testing::AssertionSuccess();
    }

    return testing::AssertionFailure()
        << "Node position (" << node.x() << ", " << node.y() << ", " << node.z() << ")"
        << " is not at expected (" << x << ", " << y << ", " << z << ")";
}

TEST(NodeTest, Translate_ByOffset_UpdatesPosition) {
    Node node(1, 0, 0, 0);
    node.translate(Eigen::Vector3d(1, 2, 3));

    EXPECT_TRUE(IsNodeAt(node, 1.0, 2.0, 3.0));
}
```

---

## 테스트 커버리지

### 1. 커버리지 측정

```bash
# 커버리지 활성화 빌드
cmake -DENABLE_COVERAGE=ON ..
make

# 테스트 실행
ctest

# 커버리지 리포트 생성
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# 리포트 보기
open coverage_report/index.html
```

### 2. 커버리지 목표

- **최소**: 80% 라인 커버리지
- **권장**: 90% 라인 커버리지
- **중요 모듈**: 95%+ 커버리지

### 3. 테스트해야 할 것들

#### 필수 테스트:
- ✅ 정상 경로 (Happy Path)
- ✅ 경계 조건 (Boundary Conditions)
- ✅ 오류 조건 (Error Conditions)
- ✅ 예외 처리 (Exception Handling)

#### 권장 테스트:
- ✅ 엣지 케이스 (Edge Cases)
- ✅ 성능 벤치마크
- ✅ 스레드 안전성 (해당되는 경우)

```cpp
// 예시: 포괄적인 테스트
TEST(NodeTest, DistanceTo_SamePosition_ReturnsZero) {
    // 정상 경로
    Node node1(1, 0, 0, 0);
    Node node2(2, 0, 0, 0);
    EXPECT_DOUBLE_EQ(0.0, node1.distanceTo(node2));
}

TEST(NodeTest, DistanceTo_LargeDistance_ReturnsCorrectValue) {
    // 경계 조건
    Node node1(1, 0, 0, 0);
    Node node2(2, 1e6, 0, 0);
    EXPECT_NEAR(1e6, node1.distanceTo(node2), 1e-5);
}

TEST(MeshTest, AddNode_DuplicateID_ThrowsException) {
    // 오류 조건
    Mesh mesh;
    mesh.addNode(Node(1, 0, 0, 0));

    EXPECT_THROW(
        mesh.addNode(Node(1, 1, 1, 1)),
        InvalidDataException
    );
}
```

---

## 모킹과 페이크

### 1. 인터페이스 정의

```cpp
// 인터페이스
class IFileReader {
public:
    virtual ~IFileReader() = default;
    virtual std::string readLine() = 0;
    virtual bool hasMore() const = 0;
};

// 실제 구현
class DynaFileReader : public IFileReader {
public:
    std::string readLine() override { /* ... */ }
    bool hasMore() const override { /* ... */ }
};
```

### 2. 페이크 구현 (테스트용)

```cpp
class FakeFileReader : public IFileReader {
public:
    FakeFileReader(const std::vector<std::string>& lines)
        : m_lines(lines), m_currentLine(0) {}

    std::string readLine() override {
        return m_lines[m_currentLine++];
    }

    bool hasMore() const override {
        return m_currentLine < m_lines.size();
    }

private:
    std::vector<std::string> m_lines;
    size_t m_currentLine;
};

TEST(ParserTest, ParseNodes_FromFakeFile_CreatesNodes) {
    std::vector<std::string> lines = {
        "*NODE",
        "1, 0.0, 0.0, 0.0",
        "2, 1.0, 0.0, 0.0"
    };

    FakeFileReader reader(lines);
    Parser parser(reader);

    auto nodes = parser.parseNodes();

    EXPECT_EQ(2, nodes.size());
}
```

---

## 성능 테스트

### 1. 벤치마크 테스트

```cpp
TEST(MemoryPoolTest, DISABLED_PerformanceBenchmark) {
    const size_t iterations = 100000;

    auto start = std::chrono::high_resolution_clock::now();

    MemoryPool<Node> pool;
    for (size_t i = 0; i < iterations; ++i) {
        Node* node = pool.construct(i, 0, 0, 0);
        pool.destroy(node);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "MemoryPool: " << duration.count() << " ms\n";

    // 성능 기준 확인
    EXPECT_LT(duration.count(), 1000);  // 1초 이내
}
```

### 2. 스케일 테스트

```cpp
TEST(MeshTest, AddNodes_LargeScale_CompletesInTime) {
    Mesh mesh;

    auto start = std::chrono::high_resolution_clock::now();

    // 100만 개 노드 추가
    std::vector<Node> nodes;
    nodes.reserve(1000000);
    for (int i = 0; i < 1000000; ++i) {
        nodes.emplace_back(i, 0, 0, 0);
    }
    mesh.addNodesBatch(nodes);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    std::cout << "Added 1M nodes in " << duration.count() << " seconds\n";

    EXPECT_EQ(1000000, mesh.nodeCount());
    EXPECT_LT(duration.count(), 5);  // 5초 이내
}
```

---

## 테스트 실행

### 1. 모든 테스트 실행

```bash
# 빌드 디렉토리에서
ctest

# 또는 make를 통해
make test

# Verbose 모드
ctest -V
ctest --output-on-failure
```

### 2. 특정 테스트만 실행

```bash
# 이름으로 필터링
ctest -R Logger

# 제외
ctest -E Integration

# 병렬 실행
ctest -j8
```

### 3. 개별 테스트 실행

```bash
# 직접 실행
./test_logger

# Google Test 필터 사용
./test_logger --gtest_filter=LoggerTest.BasicLogging
./test_logger --gtest_filter=Logger*  # 와일드카드

# 반복 실행
./test_logger --gtest_repeat=100

# 실패 시 중단
./test_logger --gtest_break_on_failure
```

---

## CI/CD 통합

### 1. GitHub Actions 설정

```yaml
# .github/workflows/ci.yml (이미 구현됨)
name: CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build
        run: |
          mkdir build && cd build
          cmake -DBUILD_TESTS=ON ..
          make -j4
      - name: Run Tests
        run: |
          cd build
          ctest --output-on-failure
```

### 2. 코드 커버리지 리포트

```yaml
- name: Generate Coverage
  run: |
    cd build
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' --output-file coverage.info

- name: Upload Coverage
  uses: codecov/codecov-action@v3
  with:
    files: ./build/coverage.info
```

---

## 베스트 프랙티스

### ✅ DO

1. **명확한 테스트 이름 사용**
   ```cpp
   TEST(MeshTest, AddNode_DuplicateID_ThrowsException)
   ```

2. **AAA 패턴 준수**
   ```cpp
   // Arrange
   Mesh mesh;
   // Act
   mesh.addNode(node);
   // Assert
   EXPECT_EQ(1, mesh.nodeCount());
   ```

3. **테스트는 독립적으로 작성**
   - 다른 테스트에 의존하지 않음
   - 실행 순서에 무관하게 동작

4. **예외 테스트 포함**
   ```cpp
   EXPECT_THROW(mesh.addNode(duplicateNode), InvalidDataException);
   ```

5. **엣지 케이스 테스트**
   - 빈 입력, nullptr, 경계값 등

### ❌ DON'T

1. **테스트에 로직 넣지 않기**
   ```cpp
   // Bad
   if (node.id() > 0) {
       EXPECT_GT(node.x(), 0);
   }
   ```

2. **프로덕션 코드에 테스트 코드 섞지 않기**

3. **너무 많은 것을 한 테스트에서 검증**

4. **하드코딩된 경로/값 사용**
   ```cpp
   // Bad
   FileReader reader("/home/user/data.txt");
   ```

---

## 체크리스트

테스트 작성 시:

- [ ] AAA 패턴 준수
- [ ] 명확한 테스트 이름
- [ ] 한 테스트는 한 가지만 검증
- [ ] 정상 경로 테스트
- [ ] 오류 조건 테스트
- [ ] 경계값 테스트
- [ ] 예외 처리 테스트
- [ ] 픽스처 활용 (반복 코드 제거)
- [ ] 테스트는 빠르게 실행 (1초 이내)
- [ ] 테스트는 독립적 (다른 테스트에 무관)

코드 리뷰 시:

- [ ] 테스트 커버리지 80% 이상
- [ ] 중요 기능은 95% 이상
- [ ] 실패하는 테스트 없음
- [ ] 테스트 이름이 명확함
- [ ] DISABLED 테스트가 있다면 이유 명시

---

## 참고 자료

1. **Google Test Documentation**: https://google.github.io/googletest/
2. **Test-Driven Development** by Kent Beck
3. **xUnit Test Patterns** by Gerard Meszaros
4. **The Art of Unit Testing** by Roy Osherove

---

## 결론

효과적인 테스트는 코드 품질과 유지보수성을 크게 향상시킵니다. 이 가이드의 원칙과 패턴을 따라 견고하고 신뢰할 수 있는 테스트를 작성하세요.

테스트 작성에 어려움이 있다면:
1. 기존 테스트 파일 참고
2. AAA 패턴부터 시작
3. 간단한 케이스부터 점진적으로 확장
4. 코드 리뷰 요청
