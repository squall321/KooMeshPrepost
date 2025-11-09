# Memory Management Strategy

## 개요

KooMeshPrepost는 수천만 개의 요소를 처리해야 하므로, 효율적인 메모리 관리가 필수적입니다. 이 문서는 프로젝트의 메모리 관리 전략과 모범 사례를 설명합니다.

## 목차

1. [설계 원칙](#설계-원칙)
2. [메모리 관리 도구](#메모리-관리-도구)
3. [스마트 포인터 사용 가이드](#스마트-포인터-사용-가이드)
4. [메모리 풀](#메모리-풀)
5. [대용량 데이터 처리](#대용량-데이터-처리)
6. [메모리 프로파일링](#메모리-프로파일링)
7. [성능 최적화](#성능-최적화)

---

## 설계 원칙

### 1. RAII (Resource Acquisition Is Initialization)

모든 리소스는 객체의 생명주기와 함께 관리됩니다.

```cpp
// Good: RAII를 사용한 파일 핸들링
class FileReader {
private:
    std::ifstream m_file;

public:
    FileReader(const std::string& path) : m_file(path) {
        if (!m_file) {
            throw FileIOException(...);
        }
    }

    ~FileReader() {
        // 파일은 자동으로 닫힘
    }
};

// Bad: 수동 메모리 관리
void badExample() {
    FILE* file = fopen("data.txt", "r");
    // ... 작업 중 예외 발생 시 파일이 닫히지 않음!
    fclose(file);  // 도달하지 못할 수 있음
}
```

### 2. 값 의미론 (Value Semantics) 우선

가능한 경우 값 의미론을 사용하여 소유권을 명확히 합니다.

```cpp
// 값 의미론 - 명확한 소유권
Node createNode(int id) {
    return Node(id, 0.0, 0.0, 0.0);  // RVO/NRVO로 최적화
}

// 참조 의미론 - 소유권이 불명확
Node* createNodePtr(int id) {
    return new Node(id, 0.0, 0.0, 0.0);  // 누가 delete 해야 하는가?
}
```

### 3. 소유권 명확화

모든 동적 할당 객체는 명확한 소유자가 있어야 합니다.

```cpp
// Mesh가 Element를 소유
class Mesh {
private:
    std::unordered_map<ElementId, std::unique_ptr<Element>> m_elements;

public:
    void addElement(std::unique_ptr<Element> element) {
        // 소유권 이전
        m_elements[element->id()] = std::move(element);
    }
};
```

---

## 메모리 관리 도구

### 1. 스택 vs 힙

| 사용 사례 | 권장 위치 | 이유 |
|----------|----------|------|
| 작은 임시 객체 | 스택 | 빠른 할당/해제 |
| 크기를 알 수 있는 지역 변수 | 스택 | 자동 정리 |
| 큰 객체 (>1KB) | 힙 | 스택 오버플로우 방지 |
| 수명이 불명확한 객체 | 힙 (스마트 포인터) | 자동 메모리 관리 |
| 다형성 객체 | 힙 | 포인터를 통한 접근 필요 |

```cpp
// 스택 할당 (권장)
void processSmallData() {
    Node node(1, 0.0, 0.0, 0.0);  // 스택
    std::array<double, 100> buffer;  // 스택
    // 함수 종료 시 자동 정리
}

// 힙 할당 (필요시)
void processLargeData() {
    auto mesh = std::make_unique<Mesh>();  // 힙, 자동 정리
    std::vector<Node> nodes;  // 내부적으로 힙 사용, 자동 관리
}
```

### 2. 컨테이너 선택

| 컨테이너 | 사용 사례 | 시간 복잡도 |
|---------|----------|-----------|
| `std::vector` | 순차 접근, 빈번한 끝 추가 | O(1) 접근, O(1) 끝 추가 |
| `std::unordered_map` | ID 기반 조회 | O(1) 평균 조회 |
| `std::set` | 정렬된 고유 요소 | O(log n) 삽입/조회 |
| `std::deque` | 양쪽 끝 추가/제거 | O(1) 양쪽 끝 연산 |
| `std::list` | 중간 삽입/삭제 빈번 | O(1) 삽입/삭제 |

```cpp
// KooMeshPrepost에서의 사용 예
class Mesh {
private:
    // ID 기반 빠른 조회
    std::unordered_map<NodeId, Node> m_nodes;

    // Element는 다형성이므로 unique_ptr 사용
    std::unordered_map<ElementId, std::unique_ptr<Element>> m_elements;

    // 순차 처리가 주된 작업
    std::vector<NodeId> m_nodeIds;
};
```

---

## 스마트 포인터 사용 가이드

### 1. `std::unique_ptr` (독점 소유권)

**사용 시기**: 단일 소유자가 명확한 경우

```cpp
// Element는 Mesh가 독점 소유
class Mesh {
private:
    std::unordered_map<ElementId, std::unique_ptr<Element>> m_elements;

public:
    void addElement(std::unique_ptr<Element> element) {
        m_elements[element->id()] = std::move(element);
    }

    // 소유권을 외부로 이전할 수도 있음
    std::unique_ptr<Element> takeElement(ElementId id) {
        auto it = m_elements.find(id);
        if (it != m_elements.end()) {
            auto element = std::move(it->second);
            m_elements.erase(it);
            return element;
        }
        return nullptr;
    }
};

// 사용 예
auto element = ElementFactory::create(ElementType::TETRAHEDRON, 1, 1, nodeIds);
mesh.addElement(std::move(element));  // 소유권 이전
```

### 2. `std::shared_ptr` (공유 소유권)

**사용 시기**: 여러 소유자가 필요한 경우 (드물게 사용)

```cpp
// 캐시나 관찰자 패턴에서 사용
class SelectionManager {
private:
    std::shared_ptr<Mesh> m_mesh;  // Mesh를 다른 곳에서도 사용

public:
    SelectionManager(std::shared_ptr<Mesh> mesh) : m_mesh(mesh) {}
};

// 주의: shared_ptr은 성능 오버헤드가 있으므로 필요한 경우만 사용
```

### 3. `std::weak_ptr` (약한 참조)

**사용 시기**: 순환 참조 방지, 캐시 구현

```cpp
// 순환 참조 방지
class Node {
private:
    std::weak_ptr<Mesh> m_mesh;  // Mesh에 대한 약한 참조
};

class Mesh {
private:
    std::vector<std::shared_ptr<Node>> m_nodes;  // Node 소유
};
```

### 4. Raw Pointer (소유권 없음)

**사용 시기**: 관찰만 하고 소유하지 않는 경우

```cpp
// Mesh에서 Node를 관찰만 함
const Node* node = mesh.getNode(nodeId);  // 소유권 없음
if (node) {
    double x = node->x();  // 읽기만 함
}
```

---

## 메모리 풀

대량의 작은 객체를 빈번히 할당/해제하는 경우 메모리 풀을 사용합니다.

### MemoryPool 구현

```cpp
template<typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    MemoryPool() : m_freeList(nullptr) {}

    ~MemoryPool() {
        // 모든 블록 해제
        for (auto block : m_blocks) {
            ::operator delete(block);
        }
    }

    T* allocate() {
        if (!m_freeList) {
            allocateBlock();
        }

        T* result = reinterpret_cast<T*>(m_freeList);
        m_freeList = m_freeList->next;
        return result;
    }

    void deallocate(T* ptr) {
        if (!ptr) return;

        FreeNode* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = m_freeList;
        m_freeList = node;
    }

private:
    struct FreeNode {
        FreeNode* next;
    };

    void allocateBlock() {
        size_t objectSize = sizeof(T);
        size_t nodeSize = sizeof(FreeNode);
        size_t allocationSize = std::max(objectSize, nodeSize);

        size_t count = BlockSize / allocationSize;
        char* block = static_cast<char*>(::operator new(BlockSize));
        m_blocks.push_back(block);

        for (size_t i = 0; i < count; ++i) {
            FreeNode* node = reinterpret_cast<FreeNode*>(block + i * allocationSize);
            node->next = m_freeList;
            m_freeList = node;
        }
    }

    FreeNode* m_freeList;
    std::vector<void*> m_blocks;
};
```

### 사용 예

```cpp
// 작은 객체를 대량으로 할당하는 경우
MemoryPool<Node> nodePool;

Node* node1 = nodePool.allocate();
new (node1) Node(1, 0.0, 0.0, 0.0);  // Placement new

// 사용 후
node1->~Node();  // 소멸자 명시적 호출
nodePool.deallocate(node1);
```

---

## 대용량 데이터 처리

### 1. 배치 처리

대량의 데이터는 배치로 처리하여 할당 오버헤드를 줄입니다.

```cpp
// Good: 배치 추가
std::vector<Node> nodes;
nodes.reserve(1000000);  // 미리 예약
for (int i = 0; i < 1000000; ++i) {
    nodes.emplace_back(i, 0.0, 0.0, 0.0);
}
mesh.addNodesBatch(nodes);

// Bad: 개별 추가
for (int i = 0; i < 1000000; ++i) {
    mesh.addNode(Node(i, 0.0, 0.0, 0.0));  // 매번 재할당 가능
}
```

### 2. Reserve 사용

벡터 크기를 미리 알 경우 `reserve()`를 사용합니다.

```cpp
std::vector<Node> nodes;
nodes.reserve(expectedSize);  // 재할당 방지

for (...) {
    nodes.emplace_back(...);
}
```

### 3. 메모리 매핑 파일

매우 큰 파일은 메모리 매핑을 사용합니다.

```cpp
class MemoryMappedFile {
public:
    MemoryMappedFile(const std::string& path);

    const char* data() const { return m_data; }
    size_t size() const { return m_size; }

private:
    void* m_handle;
    char* m_data;
    size_t m_size;
};
```

### 4. 스트리밍 처리

전체 데이터를 메모리에 로드하지 않고 스트리밍 방식으로 처리합니다.

```cpp
void processLargeFile(const std::string& filepath) {
    std::ifstream file(filepath);
    std::string line;

    while (std::getline(file, line)) {
        // 한 줄씩 처리
        processLine(line);
    }
}
```

---

## 메모리 프로파일링

### 1. Valgrind (Linux)

```bash
# 메모리 누수 검사
valgrind --leak-check=full --show-leak-kinds=all ./koomesh input.k

# 캐시 프로파일링
valgrind --tool=cachegrind ./koomesh input.k
```

### 2. AddressSanitizer (ASan)

```cmake
# CMakeLists.txt
if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
```

```bash
# 빌드 및 실행
cmake -DENABLE_SANITIZERS=ON ..
make
./koomesh input.k
```

### 3. Heaptrack (Linux)

```bash
heaptrack ./koomesh input.k
heaptrack_gui heaptrack.koomesh.<pid>.gz
```

### 4. 커스텀 메모리 추적

```cpp
class MemoryTracker {
public:
    static void* allocate(size_t size) {
        void* ptr = std::malloc(size);
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_allocations[ptr] = size;
            s_totalAllocated += size;
        }
        Logger::debugf("Allocated {} bytes at {}", size, ptr);
        return ptr;
    }

    static void deallocate(void* ptr) {
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            auto it = s_allocations.find(ptr);
            if (it != s_allocations.end()) {
                s_totalAllocated -= it->second;
                s_allocations.erase(it);
            }
        }
        std::free(ptr);
    }

    static size_t getTotalAllocated() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_totalAllocated;
    }

private:
    static std::unordered_map<void*, size_t> s_allocations;
    static size_t s_totalAllocated;
    static std::mutex s_mutex;
};
```

---

## 성능 최적화

### 1. 메모리 정렬

캐시 라인 정렬로 성능을 향상시킵니다.

```cpp
// 캐시 라인 크기에 맞춰 정렬 (일반적으로 64바이트)
struct alignas(64) CacheAlignedNode {
    NodeId id;
    Eigen::Vector3d position;
    // ...
};
```

### 2. 구조체 패킹

메모리 사용량을 줄이기 위해 구조체를 최적화합니다.

```cpp
// Bad: 12바이트 낭비 (패딩)
struct BadStruct {
    char a;      // 1 byte + 7 padding
    double b;    // 8 bytes
    char c;      // 1 byte + 7 padding
    double d;    // 8 bytes
};  // 총 32 bytes

// Good: 2바이트 낭비
struct GoodStruct {
    double b;    // 8 bytes
    double d;    // 8 bytes
    char a;      // 1 byte
    char c;      // 1 byte + 6 padding
};  // 총 24 bytes
```

### 3. Small String Optimization (SSO)

짧은 문자열은 별도 할당 없이 객체 내부에 저장합니다.

```cpp
// std::string은 이미 SSO를 사용
std::string shortName = "Part1";  // 힙 할당 없음 (일반적으로 15자 이하)
std::string longName = "Very long part name...";  // 힙 할당
```

### 4. 복사 회피

불필요한 복사를 피하고 이동 의미론을 사용합니다.

```cpp
// Bad: 복사
Element* createElementBad() {
    TetrahedronElement elem(1, 1, {1, 2, 3, 4});
    return new TetrahedronElement(elem);  // 복사 생성
}

// Good: 이동
std::unique_ptr<Element> createElementGood() {
    return std::make_unique<TetrahedronElement>(1, 1, std::vector<NodeId>{1, 2, 3, 4});
}
```

---

## 체크리스트

메모리 관리 모범 사례 체크리스트:

- [ ] 모든 동적 할당은 스마트 포인터 사용
- [ ] Raw pointer는 관찰 목적으로만 사용
- [ ] RAII 패턴 준수
- [ ] 벡터에 `reserve()` 사용 (크기를 알 경우)
- [ ] 큰 객체는 값이 아닌 참조/포인터로 전달
- [ ] 이동 의미론 활용 (`std::move`)
- [ ] 메모리 누수 검사 도구 사용 (Valgrind, ASan)
- [ ] 예외 안전성 보장
- [ ] 메모리 프로파일링으로 병목 지점 확인
- [ ] 대용량 데이터는 배치 처리 또는 스트리밍

---

## 참고 자료

1. **C++ Core Guidelines**: https://isocpp.github.io/CppCoreGuidelines/
2. **Effective Modern C++** by Scott Meyers
3. **C++ Concurrency in Action** by Anthony Williams
4. **Valgrind Documentation**: https://valgrind.org/docs/
5. **Google Sanitizers**: https://github.com/google/sanitizers

---

## 결론

효율적인 메모리 관리는 KooMeshPrepost의 성능과 안정성에 직접적인 영향을 미칩니다. 이 문서의 원칙과 모범 사례를 따라 메모리 누수를 방지하고, 성능을 최적화하며, 유지보수가 용이한 코드를 작성할 수 있습니다.

메모리 문제가 발생하면:
1. 프로파일링 도구로 원인 파악
2. 이 문서의 원칙 재검토
3. 코드 리뷰 요청
4. 필요시 메모리 풀 또는 캐싱 전략 적용
