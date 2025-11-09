# KooMeshPrepost 코딩 가이드라인

## 목차
1. [일반 원칙](#일반-원칙)
2. [네이밍 규칙](#네이밍-규칙)
3. [코드 포맷](#코드-포맷)
4. [C++ 스타일](#c-스타일)
5. [문서화](#문서화)
6. [에러 처리](#에러-처리)
7. [성능 고려사항](#성능-고려사항)
8. [테스트](#테스트)

---

## 일반 원칙

### 코드 품질
- **가독성이 성능보다 우선** (성능이 중요한 경우 프로파일링 후 최적화)
- **명확성이 간결성보다 우선** (clever code보다는 clear code)
- **KISS 원칙** (Keep It Simple, Stupid)
- **DRY 원칙** (Don't Repeat Yourself)
- **YAGNI 원칙** (You Aren't Gonna Need It)

### C++ 표준
- **C++17** 사용 (C++20 기능은 컴파일러 지원 확인 후)
- 모던 C++ 기능 적극 활용
  - `auto`, `nullptr`, range-based for
  - Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
  - Lambda expressions
  - `constexpr`, `noexcept`

### 헤더 파일
```cpp
#pragma once  // Include guard (권장)

// 또는 전통적 방식
#ifndef KOOMESH_CORE_MESH_H
#define KOOMESH_CORE_MESH_H
// ...
#endif
```

---

## 네이밍 규칙

### 파일명
- **헤더 파일**: `ClassName.h`
- **소스 파일**: `ClassName.cpp`
- **소문자 + 언더스코어**: 예외적으로 허용 (예: `mesh_utils.cpp`)

```
✅ Good:
  Mesh.h, Mesh.cpp
  Node.h, Node.cpp

❌ Bad:
  mesh.h, mesh.cpp (C 스타일)
  MeshClass.h (불필요한 접미사)
```

### 네임스페이스
- **소문자**, 단어 구분은 언더스코어
- 중첩 네임스페이스 사용

```cpp
namespace koomesh {
namespace core {

class Mesh {
    // ...
};

} // namespace core
} // namespace koomesh

// C++17 중첩 네임스페이스
namespace koomesh::core {
    // ...
}
```

### 클래스/구조체/열거형
- **PascalCase** (UpperCamelCase)
- 명확하고 설명적인 이름

```cpp
✅ Good:
class MeshValidator;
struct BoundingBox;
enum class ElementType;

❌ Bad:
class meshvalidator;  // 대소문자 없음
class MeshVal;         // 불필요한 축약
class IMesh;           // 인터페이스 접두사 (C# 스타일)
```

### 함수/메서드
- **camelCase** (lowerCamelCase)
- 동사로 시작 (동작을 나타냄)

```cpp
✅ Good:
void computeBoundingBox();
bool isValid() const;
size_t nodeCount() const;
void addElement(ElementId id);

❌ Bad:
void ComputeBoundingBox();  // PascalCase
void compute_bounding_box(); // snake_case (Python 스타일)
void bbox();                 // 불명확
```

### 변수
- **camelCase**
- 멤버 변수는 `m_` 접두사

```cpp
class Node {
public:
    NodeId id() const { return m_id; }

private:
    NodeId m_id;                    // 멤버 변수
    Eigen::Vector3d m_coordinates;  // 멤버 변수
    std::vector<ElementId> m_connectedElements;
};

// 로컬 변수
void function() {
    int elementCount = 0;      // camelCase
    const double maxVolume = computeMaxVolume();
}
```

### 상수
- **UPPER_CASE** with underscores
- `constexpr` 사용 권장

```cpp
✅ Good:
constexpr double PI = 3.14159265358979323846;
constexpr int MAX_NODES_PER_ELEMENT = 20;
const std::string DEFAULT_CONFIG_FILE = "config.json";

❌ Bad:
const double pi = 3.14;  // 소문자
#define PI 3.14          // 매크로 (피하기)
```

### 열거형
- **Enum class** 사용 (타입 안전성)
- 열거자는 UPPER_CASE

```cpp
✅ Good:
enum class ElementType {
    TETRAHEDRON,
    HEXAHEDRON,
    TRIANGLE,
    QUADRILATERAL
};

// 사용
ElementType type = ElementType::TETRAHEDRON;

❌ Bad:
enum ElementType {  // C 스타일 (타입 안전하지 않음)
    TETRA,
    HEXA
};
```

### 템플릿 파라미터
- **PascalCase**

```cpp
template<typename T, typename Allocator>
class Container {
    // ...
};
```

---

## 코드 포맷

### 들여쓰기
- **4 스페이스** (탭 아님)
- `.clang-format` 파일 참조

### 중괄호
- **K&R 스타일** (Kernighan & Ritchie)
- 여는 중괄호는 같은 줄

```cpp
✅ Good:
void function() {
    if (condition) {
        doSomething();
    } else {
        doSomethingElse();
    }
}

class MyClass {
public:
    void method() {
        // ...
    }
};

❌ Bad:
void function()
{  // Allman 스타일 (우리는 사용 안 함)
    // ...
}
```

### 줄 길이
- **최대 100자** (선호)
- 120자까지 허용 (필요 시)

### 공백
```cpp
// 연산자 주변
int a = b + c;     // ✅
int a=b+c;         // ❌

// 쉼표 뒤
function(a, b, c); // ✅
function(a,b,c);   // ❌

// 중괄호
if (condition) {   // ✅
if(condition){     // ❌

// 포인터/레퍼런스 (왼쪽 붙임)
int* ptr;          // ✅
int *ptr;          // ❌ (C 스타일)
int & ref;         // ❌
```

### 빈 줄
- 함수 사이: 1줄
- 논리적 블록 사이: 1줄
- 클래스 섹션 사이: 1줄

```cpp
class Example {
public:
    void method1();

    void method2();  // 관련 없는 메서드 사이 빈 줄

private:
    int m_value;
    std::string m_name;

    void helperMethod();  // 논리적 그룹 분리
};
```

---

## C++ 스타일

### Include 순서
```cpp
// 1. 해당 헤더 (소스 파일인 경우)
#include "Mesh.h"

// 2. C++ 표준 라이브러리
#include <iostream>
#include <vector>
#include <memory>

// 3. 써드파티 라이브러리
#include <Eigen/Core>
#include <boost/filesystem.hpp>

// 4. 프로젝트 헤더
#include "core/Node.h"
#include "core/Element.h"
```

### Forward Declaration
- 헤더에서 포인터/참조만 사용하는 경우 forward declaration
- 컴파일 시간 단축

```cpp
// Node.h
namespace koomesh::core {

class Element;  // Forward declaration

class Node {
    void addConnectedElement(const Element* element);
};

} // namespace koomesh::core
```

### Smart Pointers
- **Raw 포인터 대신 smart pointer 사용**
- 소유권이 명확한 경우: `std::unique_ptr`
- 공유 소유권: `std::shared_ptr`
- 관찰만: `std::weak_ptr` 또는 raw pointer

```cpp
✅ Good:
std::unique_ptr<Element> element = std::make_unique<TetrahedronElement>(...);
mesh.addElement(std::move(element));

❌ Bad:
Element* element = new TetrahedronElement(...);  // 수동 메모리 관리
mesh.addElement(element);
// ... delete element;  // 누가, 언제?
```

### RAII (Resource Acquisition Is Initialization)
- 리소스는 생성자에서 획득, 소멸자에서 해제

```cpp
class FileReader {
public:
    FileReader(const std::string& filepath) {
        m_file.open(filepath);
        if (!m_file.is_open()) {
            throw FileIOException("Cannot open file");
        }
    }

    ~FileReader() {
        if (m_file.is_open()) {
            m_file.close();  // 자동 정리
        }
    }

private:
    std::ifstream m_file;
};
```

### Const Correctness
- **const를 최대한 활용**
- 멤버 함수가 상태를 변경하지 않으면 `const`

```cpp
class BoundingBox {
public:
    // Getter는 const
    double volume() const {
        return (m_max.x() - m_min.x()) *
               (m_max.y() - m_min.y()) *
               (m_max.z() - m_min.z());
    }

    // Setter는 non-const
    void expand(const Eigen::Vector3d& point) {
        m_min = m_min.cwiseMin(point);
        m_max = m_max.cwiseMax(point);
    }

private:
    Eigen::Vector3d m_min;
    Eigen::Vector3d m_max;
};

// 파라미터도 const 참조
void process(const Mesh& mesh) {  // 복사 방지
    // ...
}
```

### Auto 사용
- **타입이 명확하거나 중요하지 않을 때**
- 복잡한 반복자, 람다

```cpp
✅ Good:
auto it = container.begin();  // 반복자
auto lambda = [](int x) { return x * 2; };
auto result = std::make_unique<Mesh>();

// 타입이 명확
for (const auto& [id, element] : mesh.elements()) {
    // ...
}

❌ Bad (명확성 손실):
auto value = computeQuality();  // double? float? int?
// 명시적 타입이 나음: double value = computeQuality();
```

### Range-Based For Loop
```cpp
✅ Good:
for (const auto& element : elements) {
    process(element);
}

❌ Bad (C 스타일):
for (size_t i = 0; i < elements.size(); ++i) {
    process(elements[i]);
}

// 인덱스가 필요한 경우만 전통적 for 사용
for (size_t i = 0; i < elements.size(); ++i) {
    std::cout << "Element " << i << ": " << elements[i] << "\n";
}
```

### Nullptr
```cpp
✅ Good:
int* ptr = nullptr;
if (ptr != nullptr) {
    // ...
}

❌ Bad:
int* ptr = NULL;   // C 스타일
int* ptr = 0;      // 숫자 0
```

---

## 문서화

### Doxygen 주석
- **모든 public API**는 Doxygen 주석 필수
- private 메서드는 선택적

```cpp
/**
 * @brief Mesh 클래스 - FEM mesh의 전체 데이터 관리
 *
 * Aggregate Root 패턴을 따르며, 모든 노드, 요소, 파트를 관리합니다.
 * 공간 인덱싱을 통해 빠른 검색을 지원합니다.
 *
 * @note 스레드 안전하지 않습니다. 외부에서 동기화 필요.
 *
 * Example usage:
 * @code
 * Mesh mesh;
 * mesh.addNode(Node(1, 0.0, 0.0, 0.0));
 * mesh.buildSpatialIndex();
 * auto bbox = mesh.boundingBox();
 * @endcode
 */
class Mesh {
public:
    /**
     * @brief 노드 추가
     *
     * @param node 추가할 노드 (복사됨)
     * @throws std::invalid_argument 동일한 ID의 노드가 이미 존재하는 경우
     *
     * @note 노드 추가 후에는 공간 인덱스를 재구축해야 합니다.
     */
    void addNode(const Node& node);

    /**
     * @brief 전체 노드 수 반환
     * @return 노드 개수
     * @complexity O(1)
     */
    size_t nodeCount() const { return m_nodes.size(); }

private:
    std::unordered_map<NodeId, Node> m_nodes;  ///< 노드 맵 (ID -> Node)
};
```

### 주석 스타일
```cpp
// 한 줄 주석 (간단한 설명)
int count = 0;  // 요소 개수

/*
 * 블록 주석 (여러 줄)
 * 복잡한 알고리즘 설명
 */

/**
 * Doxygen 주석
 * public API 문서화
 */
```

### TODO/FIXME/NOTE
```cpp
// TODO: 성능 최적화 필요
// FIXME: 경계 케이스 처리 버그
// NOTE: 이 함수는 스레드 안전하지 않음
// HACK: 임시 해결책, 나중에 리팩토링
```

---

## 에러 처리

### 예외 사용
- **예외적인 상황**에만 예외 사용
- 정상 흐름은 반환값으로 처리

```cpp
✅ Good:
void loadFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw FileIOException("Cannot open file: " + filepath);
    }
    // ...
}

// 사용
try {
    loadFile("mesh.k");
} catch (const FileIOException& e) {
    std::cerr << "Error: " << e.what() << "\n";
}
```

### 예외 명세
```cpp
// noexcept: 예외를 던지지 않음을 보장
size_t nodeCount() const noexcept {
    return m_nodes.size();
}

// 조건부 noexcept
template<typename T>
void swap(T& a, T& b) noexcept(std::is_nothrow_move_constructible_v<T>) {
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}
```

### Assert
```cpp
#include <cassert>

void process(const Element* element) {
    assert(element != nullptr);  // Debug 빌드에서만
    // ...
}

// 커스텀 assert (Release에서도 체크)
#define KOOMESH_ASSERT(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(message); \
    }
```

---

## 성능 고려사항

### 불필요한 복사 방지
```cpp
✅ Good:
void process(const std::vector<int>& data) {  // 참조
    for (const auto& value : data) {  // 참조
        // ...
    }
}

❌ Bad:
void process(std::vector<int> data) {  // 복사!
    for (auto value : data) {  // 또 복사!
        // ...
    }
}
```

### Move Semantics
```cpp
class Mesh {
public:
    void addElement(std::unique_ptr<Element> element) {
        m_elements[element->id()] = std::move(element);  // Move
    }
};

// 사용
auto element = std::make_unique<TetrahedronElement>(...);
mesh.addElement(std::move(element));  // element는 이제 nullptr
```

### Reserve
```cpp
std::vector<Node> nodes;
nodes.reserve(1000000);  // 메모리 사전 할당

for (int i = 0; i < 1000000; ++i) {
    nodes.emplace_back(i, x, y, z);  // 재할당 없음
}
```

---

## 테스트

### 단위 테스트
- **Google Test** 사용
- 모든 public 메서드 테스트

```cpp
#include <gtest/gtest.h>
#include "core/Mesh.h"

using namespace koomesh::core;

TEST(MeshTest, AddNode) {
    Mesh mesh;
    Node node(1, 0.0, 0.0, 0.0);

    mesh.addNode(node);

    EXPECT_EQ(mesh.nodeCount(), 1);
    EXPECT_NE(mesh.getNode(1), nullptr);
}

TEST(MeshTest, BoundingBox) {
    Mesh mesh;
    mesh.addNode(Node(1, 0.0, 0.0, 0.0));
    mesh.addNode(Node(2, 10.0, 10.0, 10.0));

    auto bbox = mesh.boundingBox();

    EXPECT_DOUBLE_EQ(bbox.minX(), 0.0);
    EXPECT_DOUBLE_EQ(bbox.maxX(), 10.0);
}
```

---

## 체크리스트

코드 리뷰 전 확인:

- [ ] `.clang-format`으로 포맷팅
- [ ] `.clang-tidy` 경고 해결
- [ ] 모든 public API에 Doxygen 주석
- [ ] 단위 테스트 작성 및 통과
- [ ] 메모리 누수 없음 (Valgrind/AddressSanitizer)
- [ ] const correctness 확인
- [ ] 예외 안전성 확인
- [ ] 컴파일 경고 0개

---

## 참고 자료

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Effective Modern C++](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/)
