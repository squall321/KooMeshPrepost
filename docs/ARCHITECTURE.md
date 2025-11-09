# KooMeshPrepost - 아키텍처 문서

## 목차
1. [시스템 개요](#시스템-개요)
2. [아키텍처 패턴](#아키텍처-패턴)
3. [계층 구조](#계층-구조)
4. [핵심 컴포넌트](#핵심-컴포넌트)
5. [데이터 흐름](#데이터-흐름)
6. [성능 최적화 전략](#성능-최적화-전략)
7. [확장성](#확장성)

---

## 시스템 개요

KooMeshPrepost는 수천만 요소 규모의 LS-DYNA mesh를 효율적으로 가시화하고 전처리하기 위한 C++ 기반 데스크톱 애플리케이션입니다.

### 설계 목표
1. **고성능**: 1000만 요소 이상의 대용량 데이터 실시간 처리
2. **확장성**: 새로운 파일 포맷 및 기능 추가 용이
3. **유지보수성**: 명확한 계층 분리 및 디자인 패턴 적용
4. **사용성**: 직관적이고 현대적인 사용자 인터페이스

---

## 아키텍처 패턴

### 1. MVP (Model-View-Presenter) 패턴

```
┌────────────┐         ┌────────────┐         ┌────────────┐
│            │◄────────│            │────────►│            │
│   View     │  Events │  Presenter │  Update │   Model    │
│  (Qt UI)   │         │  (Logic)   │         │  (Data)    │
│            │────────►│            │◄────────│            │
└────────────┘  Notify └────────────┘  Events └────────────┘
```

**장점**:
- UI와 비즈니스 로직 완전 분리
- 단위 테스트 용이
- 여러 뷰에서 동일한 Model 재사용 가능

**구현**:
```cpp
// Presenter가 중재자 역할
class Presenter {
    Mesh* m_model;              // 데이터 모델
    MainWindow* m_view;          // UI 뷰

    void onUserAction() {
        // 1. View로부터 이벤트 수신
        // 2. Model 업데이트
        // 3. View에 변경사항 반영
    }
};
```

### 2. Command Pattern (실행 취소/재실행)

```
┌──────────────┐
│CommandManager│
│  ┌────────┐  │
│  │Undo    │  │
│  │Stack   │  │
│  └────────┘  │
│  ┌────────┐  │
│  │Redo    │  │
│  │Stack   │  │
│  └────────┘  │
└──────────────┘
       │
       │ executes
       ▼
┌────────────────┐
│   ICommand     │
│ ┌──────────┐   │
│ │ execute()│   │
│ │ undo()   │   │
│ └──────────┘   │
└────────────────┘
       △
       │ implements
       │
   ┌───┴───┬───────────┬────────────┐
   │       │           │            │
Create  Delete    Modify     Select
Group   Group     Group      Elements
```

**구현**:
```cpp
class CreateGroupCommand : public ICommand {
    void execute() override {
        groupManager->createGroup(name);
    }
    void undo() override {
        groupManager->deleteGroup(name);
    }
};
```

### 3. Observer Pattern (이벤트 시스템)

```
┌─────────────┐
│EventSystem  │
│             │
│ subscribe() │◄────┐
│ notify()    │     │
└─────────────┘     │
       │            │
       │ notifies   │ subscribes
       ▼            │
┌─────────────┐     │
│ Observers   │─────┘
│ - UI        │
│ - Renderer  │
│ - Logger    │
└─────────────┘
```

### 4. Factory Pattern (객체 생성)

```cpp
// 다양한 Element 타입을 생성
class ElementFactory {
public:
    static unique_ptr<Element> create(ElementType type, ...) {
        switch(type) {
            case TETRA: return make_unique<TetrahedronElement>(...);
            case HEXA:  return make_unique<HexahedronElement>(...);
            // ...
        }
    }
};
```

### 5. Strategy Pattern (알고리즘 교체)

```cpp
// 다양한 공간 인덱싱 전략
class ISpatialIndex {
    virtual void build(const Mesh&) = 0;
    virtual vector<ElementId> query(const BoundingBox&) = 0;
};

class Octree : public ISpatialIndex { /* ... */ };
class RTree : public ISpatialIndex { /* ... */ };
class UniformGrid : public ISpatialIndex { /* ... */ };
```

---

## 계층 구조

### 전체 시스템 레이어

```
┌─────────────────────────────────────────────────────────┐
│                    Presentation Layer                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐  │
│  │MainWindow│  │Parts     │  │Groups    │  │Props   │  │
│  │  (Qt)    │  │Panel     │  │Panel     │  │Panel   │  │
│  └──────────┘  └──────────┘  └──────────┘  └────────┘  │
└─────────────────────────────────────────────────────────┘
                         │
                         │ User Interactions
                         ▼
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                     │
│              ┌──────────────────────┐                   │
│              │      Presenter       │                   │
│              │  (Business Logic)    │                   │
│              └──────────────────────┘                   │
│                         │                               │
│     ┌───────────────────┼───────────────────┐           │
│     ▼                   ▼                   ▼           │
│ ┌────────┐       ┌────────────┐      ┌──────────┐      │
│ │Command │       │Event       │      │Task      │      │
│ │Manager │       │System      │      │Manager   │      │
│ └────────┘       └────────────┘      └──────────┘      │
└─────────────────────────────────────────────────────────┘
                         │
                         │ Delegates to Services
                         ▼
┌─────────────────────────────────────────────────────────┐
│                    Service Layer                        │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐  │
│  │File I/O  │  │Visualize │  │Selection │  │Group   │  │
│  │Service   │  │Service   │  │Service   │  │Manager │  │
│  │(Parser)  │  │(VTK)     │  │(Spatial) │  │        │  │
│  └──────────┘  └──────────┘  └──────────┘  └────────┘  │
└─────────────────────────────────────────────────────────┘
                         │
                         │ Operates on
                         ▼
┌─────────────────────────────────────────────────────────┐
│                     Domain Layer                        │
│              ┌──────────────────────┐                   │
│              │        Mesh          │                   │
│              │  (Aggregate Root)    │                   │
│              └──────────────────────┘                   │
│                         │                               │
│     ┌───────────────────┼───────────────────┐           │
│     ▼                   ▼                   ▼           │
│ ┌────────┐       ┌────────────┐      ┌──────────┐      │
│ │Node    │       │Element     │      │Part      │      │
│ │        │       │(abstract)  │      │          │      │
│ └────────┘       └────────────┘      └──────────┘      │
│                         △                               │
│              ┌──────────┼──────────┐                    │
│              ▼          ▼          ▼                    │
│           Tetra      Hexa       Shell                   │
└─────────────────────────────────────────────────────────┘
                         │
                         │ Indexed by
                         ▼
┌─────────────────────────────────────────────────────────┐
│                Infrastructure Layer                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────┐  │
│  │Spatial   │  │Thread    │  │Memory    │  │Config  │  │
│  │Index     │  │Pool      │  │Manager   │  │Manager │  │
│  │(Octree)  │  │          │  │          │  │        │  │
│  └──────────┘  └──────────┘  └──────────┘  └────────┘  │
└─────────────────────────────────────────────────────────┘
```

---

## 핵심 컴포넌트

### 1. Data Model (Domain Layer)

#### 1.1 Mesh (Aggregate Root)

```cpp
class Mesh {
private:
    // 데이터 저장 (효율적인 조회를 위한 hash map)
    unordered_map<NodeId, Node> m_nodes;
    unordered_map<ElementId, unique_ptr<Element>> m_elements;
    unordered_map<PartId, Part> m_parts;

    // 공간 인덱싱 (빠른 검색)
    unique_ptr<ISpatialIndex> m_spatialIndex;

    // 경계 정보
    BoundingBox m_boundingBox;

public:
    // CQRS 패턴: Command와 Query 분리

    // Commands (상태 변경)
    void addNode(const Node& node);
    void addElement(unique_ptr<Element> element);
    void removeElement(ElementId id);

    // Queries (조회만)
    const Node* getNode(NodeId id) const;
    const Element* getElement(ElementId id) const;
    vector<ElementId> findElementsInBox(const BoundingBox& box) const;

    // Batch operations (성능 최적화)
    void addNodesBatch(const vector<Node>& nodes);
    void buildSpatialIndex();
};
```

**설계 결정**:
- `unordered_map`: O(1) 평균 조회 시간
- `unique_ptr`: 소유권 명확화, 메모리 자동 관리
- 공간 인덱스: 별도 객체로 분리 (Strategy 패턴)

#### 1.2 Element Hierarchy

```
        Element (abstract)
            │
    ┌───────┴───────────────────┐
    │                           │
SolidElement              ShellElement
    │                           │
┌───┴────┐                  ┌───┴────┐
│        │                  │        │
Tetra  Hexa              Tria    Quad
(4)    (8)               (3)     (4)
```

```cpp
class Element {
protected:
    ElementId m_id;
    PartId m_partId;
    vector<NodeId> m_nodeIds;

public:
    virtual ~Element() = default;

    // Template Method Pattern
    virtual ElementType type() const = 0;
    virtual size_t nodeCount() const = 0;
    virtual BoundingBox computeBoundingBox(const Mesh& mesh) const = 0;
    virtual double computeVolume(const Mesh& mesh) const = 0;
    virtual double computeQuality(const Mesh& mesh) const = 0;
};
```

### 2. I/O Layer

#### 2.1 파일 파싱 전략

```
┌────────────────┐
│FileFormatDetect│──┐
└────────────────┘  │
                    │ detects format
                    ▼
          ┌──────────────────┐
          │FileReaderFactory │
          └──────────────────┘
                    │
        ┌───────────┼───────────┐
        ▼           ▼           ▼
   ┌─────────┐ ┌─────────┐ ┌─────────┐
   │Dyna     │ │VTK      │ │Nastran  │
   │Reader   │ │Reader   │ │Reader   │
   └─────────┘ └─────────┘ └─────────┘
        │
        │ parses
        ▼
   ┌─────────────────┐
   │  Mesh (Model)   │
   └─────────────────┘
```

#### 2.2 대용량 파일 파싱 전략

```cpp
class DynaFileReader {
private:
    // 1. 메모리 맵 파일 사용 (빠른 I/O)
    MemoryMappedFile m_file;

    // 2. 멀티스레드 파싱
    ThreadPool m_threadPool;

    // 3. 청크 단위 처리
    struct Chunk {
        size_t offset;
        size_t size;
        SectionType type; // NODE, ELEMENT, etc.
    };

public:
    bool read(const string& filepath, Mesh& mesh) override {
        // Step 1: 파일을 메모리에 매핑
        m_file.open(filepath);

        // Step 2: 섹션 경계 탐지 (단일 스레드, 빠름)
        vector<Chunk> chunks = detectSections(m_file);

        // Step 3: 각 섹션을 병렬로 파싱
        vector<future<ParseResult>> futures;
        for (const auto& chunk : chunks) {
            futures.push_back(
                m_threadPool.enqueue([this, chunk]() {
                    return parseChunk(chunk);
                })
            );
        }

        // Step 4: 결과 취합
        for (auto& future : futures) {
            auto result = future.get();
            mergeIntoMesh(result, mesh);
        }

        // Step 5: 공간 인덱스 구축
        mesh.buildSpatialIndex();

        return true;
    }
};
```

**성능 최적화**:
- 메모리 맵: 파일 I/O 최소화
- 병렬 파싱: CPU 코어 활용
- 사전 할당: 메모리 재할당 방지

### 3. Visualization Layer (VTK)

#### 3.1 VTK 파이프라인

```
Mesh Data                 VTK Pipeline              Display
─────────                 ────────────              ───────

┌────────┐               ┌──────────────┐
│Mesh    │──convert───►  │vtkUnstructured│
│        │               │Grid           │
└────────┘               └──────────────┘
                                │
                                │ filter
                                ▼
                         ┌──────────────┐
                         │vtkGeometry   │
                         │Filter        │
                         └──────────────┘
                                │
                                │ map
                                ▼
                         ┌──────────────┐
                         │vtkPolyData   │
                         │Mapper        │
                         └──────────────┘
                                │
                                │ render
                                ▼
                         ┌──────────────┐
                         │vtkActor      │
                         └──────────────┘
                                │
                                ▼
                         ┌──────────────┐          ┌────────┐
                         │vtkRenderer   │─────────►│Screen  │
                         └──────────────┘          └────────┘
```

#### 3.2 렌더링 최적화

```cpp
class VTKRenderer {
private:
    vtkSmartPointer<vtkRenderer> m_renderer;

    // LOD 시스템
    LODManager m_lodManager;

    // Frustum Culling
    FrustumCuller m_culler;

public:
    void render() {
        // 1. Frustum culling (보이지 않는 요소 제거)
        auto camera = m_renderer->GetActiveCamera();
        m_culler.setFromCamera(camera);
        auto visibleElements = m_culler.cullElements(mesh, spatialIndex);

        // 2. LOD 선택 (거리에 따라 상세도 조절)
        double distance = computeCameraDistance(camera);
        const Mesh& lodMesh = m_lodManager.getMeshForDistance(distance);

        // 3. VTK 데이터로 변환 (보이는 요소만)
        auto vtkData = MeshToVTK::convert(lodMesh, visibleElements);

        // 4. 렌더링
        m_renderer->Render();
    }
};
```

### 4. Selection System

#### 4.1 공간 인덱싱 (Octree)

```
                    Root Node
                 [0,0,0 - 100,100,100]
                         │
         ┌───────────────┴───────────────┐
         │                               │
    [0,0,0-50,50,50]              [50,50,50-100,100,100]
         │                               │
    ┌────┴────┐                     ┌────┴────┐
    │         │                     │         │
[0,0,0-   [25,25,25-           [50,50,50- [75,75,75-
25,25,25] 50,50,50]            75,75,75]  100,100,100]
   │          │                    │          │
Elements   Elements            Elements   Elements
```

```cpp
class Octree {
private:
    struct OctreeNode {
        BoundingBox bounds;
        vector<ElementId> elements;  // 리프 노드만
        array<unique_ptr<OctreeNode>, 8> children;
        bool isLeaf;

        // 공간 분할 (8개 하위 노드)
        enum Octant {
            LBN, RBN, LBF, RBF,  // Lower Back/Front
            LTN, RTN, LTF, RTF   // Upper Back/Front
        };
    };

    unique_ptr<OctreeNode> m_root;
    size_t m_maxDepth = 10;
    size_t m_maxElementsPerNode = 100;

    void subdivide(OctreeNode* node, const Mesh& mesh, size_t depth) {
        if (depth >= m_maxDepth || node->elements.size() <= m_maxElementsPerNode) {
            return; // 리프 노드
        }

        // 8개 하위 공간으로 분할
        auto subBoxes = node->bounds.subdivide();

        for (size_t i = 0; i < 8; ++i) {
            node->children[i] = make_unique<OctreeNode>();
            node->children[i]->bounds = subBoxes[i];

            // 요소를 하위 노드에 분배
            for (auto elemId : node->elements) {
                auto* elem = mesh.getElement(elemId);
                if (subBoxes[i].intersects(elem->boundingBox(mesh))) {
                    node->children[i]->elements.push_back(elemId);
                }
            }

            // 재귀적으로 세분화
            subdivide(node->children[i].get(), mesh, depth + 1);
        }

        node->elements.clear(); // 내부 노드는 비움
        node->isLeaf = false;
    }

public:
    vector<ElementId> query(const BoundingBox& box) const {
        vector<ElementId> results;
        queryRecursive(m_root.get(), box, results);
        return results;
    }

private:
    void queryRecursive(const OctreeNode* node, const BoundingBox& box,
                        vector<ElementId>& results) const {
        if (!node || !node->bounds.intersects(box)) {
            return; // 교차 없음
        }

        if (node->isLeaf) {
            // 리프 노드: 요소 추가
            results.insert(results.end(),
                          node->elements.begin(),
                          node->elements.end());
        } else {
            // 내부 노드: 하위 노드 탐색
            for (const auto& child : node->children) {
                queryRecursive(child.get(), box, results);
            }
        }
    }
};
```

**시간 복잡도**:
- 구축: O(n log n) - n은 요소 수
- 쿼리: O(log n + k) - k는 결과 수

#### 4.2 영역 선택 (Area Selection)

```cpp
class AreaSelector {
public:
    vector<ElementId> selectInArea(
        int startX, int startY, int endX, int endY,
        vtkRenderer* renderer,
        const Mesh& mesh,
        const ISpatialIndex& spatialIndex)
    {
        // Method 1: VTK Hardware Selector (GPU 기반, 매우 빠름)
        // 각 요소를 고유 색상으로 렌더링하여 선택
        auto hwSelector = vtkSmartPointer<vtkHardwareSelector>::New();
        hwSelector->SetRenderer(renderer);
        hwSelector->SetArea(startX, startY, endX, endY);

        auto selection = hwSelector->Select();
        auto elementIds = extractElementIds(selection);

        return elementIds;

        // Method 2: 공간 인덱스 기반 (CPU, 유연함)
        // 화면 좌표 → 3D 공간 변환
        auto frustum = screenToWorldFrustum(startX, startY, endX, endY, renderer);
        auto candidateElements = spatialIndex.query(frustum.boundingBox());

        // 정확한 교차 테스트
        vector<ElementId> selectedElements;
        for (auto elemId : candidateElements) {
            if (frustum.contains(mesh.getElement(elemId), mesh)) {
                selectedElements.push_back(elemId);
            }
        }

        return selectedElements;
    }
};
```

---

## 데이터 흐름

### 파일 로딩 시퀀스

```
User                MainWindow       Presenter      FileReader      Mesh        SpatialIndex
 │                      │               │               │            │               │
 │  Click "Open"        │               │               │            │               │
 ├─────────────────────►│               │               │            │               │
 │                      │ onFileOpen()  │               │            │               │
 │                      ├──────────────►│               │            │               │
 │                      │               │ read()        │            │               │
 │                      │               ├──────────────►│            │               │
 │                      │               │               │ addNode()  │               │
 │                      │               │               ├───────────►│               │
 │                      │               │               │ addElement()               │
 │                      │               │               ├───────────►│               │
 │                      │               │               │            │               │
 │  Show Progress       │               │               │            │               │
 │◄─────────────────────┤               │               │            │               │
 │                      │               │               │            │ build()       │
 │                      │               │               │            ├──────────────►│
 │                      │               │ success       │            │               │
 │                      │               │◄──────────────┤            │               │
 │                      │ updateView()  │               │            │               │
 │                      │◄──────────────┤               │            │               │
 │  Display Mesh        │               │               │            │               │
 │◄─────────────────────┤               │            │               │
```

### 요소 선택 시퀀스

```
User        MainWindow    Presenter    SelectionMgr   SpatialIndex   Renderer
 │              │             │              │              │            │
 │ Drag Mouse   │             │              │              │            │
 ├─────────────►│             │              │              │            │
 │              │ onDrag()    │              │              │            │
 │              ├────────────►│              │              │            │
 │              │             │ selectInArea()              │            │
 │              │             ├─────────────►│              │            │
 │              │             │              │ query(box)   │            │
 │              │             │              ├─────────────►│            │
 │              │             │              │ elementIds   │            │
 │              │             │              │◄─────────────┤            │
 │              │             │ elementIds   │              │            │
 │              │             │◄─────────────┤              │            │
 │              │             │ highlight(ids)              │            │
 │              │             ├─────────────────────────────┼───────────►│
 │              │ updateUI()  │              │              │            │
 │              │◄────────────┤              │              │            │
 │ Show Selected│             │              │              │            │
 │◄─────────────┤             │              │              │            │
```

---

## 성능 최적화 전략

### 1. 메모리 최적화

#### 1.1 Flyweight Pattern (공유 데이터)

```cpp
// 같은 타입의 요소는 타입 정보를 공유
class ElementTypeInfo {
    static map<ElementType, shared_ptr<ElementTypeInfo>> s_typeInfoCache;

    ElementType type;
    size_t nodeCount;
    string description;

public:
    static shared_ptr<ElementTypeInfo> get(ElementType type) {
        if (s_typeInfoCache.find(type) == s_typeInfoCache.end()) {
            s_typeInfoCache[type] = make_shared<ElementTypeInfo>(type);
        }
        return s_typeInfoCache[type];
    }
};
```

#### 1.2 Structure of Arrays (SOA)

```cpp
// 캐시 친화적인 데이터 레이아웃
class NodeStorage {
private:
    vector<NodeId> m_ids;        // 연속 메모리
    vector<double> m_x;          // 연속 메모리
    vector<double> m_y;
    vector<double> m_z;

public:
    // 벡터화 가능 (SIMD)
    void translate(double dx, double dy, double dz) {
        for (size_t i = 0; i < m_x.size(); ++i) {
            m_x[i] += dx;
            m_y[i] += dy;
            m_z[i] += dz;
        }
    }
};
```

### 2. 병렬 처리

```cpp
// Intel TBB를 사용한 병렬화
#include <tbb/parallel_for.h>

void Mesh::computeElementQuality() {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, m_elements.size()),
        [&](const tbb::blocked_range<size_t>& range) {
            for (size_t i = range.begin(); i < range.end(); ++i) {
                auto& elem = m_elements[i];
                elem->computeQuality(*this);
            }
        }
    );
}
```

### 3. 지연 평가 (Lazy Evaluation)

```cpp
class BoundingBox {
private:
    mutable optional<array<double, 6>> m_cachedBounds;

public:
    const array<double, 6>& bounds() const {
        if (!m_cachedBounds) {
            m_cachedBounds = computeBounds(); // 필요할 때만 계산
        }
        return *m_cachedBounds;
    }

    void invalidate() {
        m_cachedBounds.reset(); // 변경 시 무효화
    }
};
```

---

## 확장성

### 1. 플러그인 아키텍처

```
┌──────────────────┐
│   Main App       │
│                  │
│  ┌────────────┐  │
│  │Plugin      │  │
│  │Manager     │  │
│  └────────────┘  │
│         │        │
└─────────┼────────┘
          │ loads
          ▼
┌──────────────────┐      ┌──────────────────┐
│  IPlugin         │◄─────│ Concrete Plugin  │
│                  │      │                  │
│  - initialize()  │      │  - Custom logic  │
│  - shutdown()    │      │  - UI extension  │
└──────────────────┘      └──────────────────┘
```

### 2. 새로운 파일 포맷 추가

```cpp
// 1. IFileReader 인터페이스 구현
class AbaqusFileReader : public IFileReader {
    bool read(const string& filepath, Mesh& mesh) override {
        // Abaqus INP 포맷 파싱
    }
};

// 2. Factory에 등록
void FileReaderFactory::registerReader(FileFormat format,
                                       function<unique_ptr<IFileReader>()> creator) {
    m_creators[format] = creator;
}

// 3. 사용
FileReaderFactory::registerReader(FileFormat::ABAQUS,
    []() { return make_unique<AbaqusFileReader>(); });
```

### 3. Python 스크립팅 (미래 확장)

```cpp
// pybind11을 사용한 Python 바인딩
#include <pybind11/pybind11.h>

PYBIND11_MODULE(koomesh, m) {
    py::class_<Mesh>(m, "Mesh")
        .def("node_count", &Mesh::nodeCount)
        .def("element_count", &Mesh::elementCount)
        .def("get_node", &Mesh::getNode);

    py::class_<Node>(m, "Node")
        .def_readonly("id", &Node::id)
        .def_property_readonly("coordinates", &Node::coordinates);
}
```

Python에서 사용:
```python
import koomesh

mesh = koomesh.Mesh()
# mesh 조작...
```

---

## 요약

이 아키텍처는 다음을 달성합니다:

1. **분리된 관심사**: 각 레이어가 명확한 책임을 가짐
2. **확장 가능**: 새로운 기능 추가 용이
3. **테스트 가능**: 각 컴포넌트 독립적으로 테스트
4. **고성능**: 병렬 처리, 공간 인덱싱, 캐싱 등
5. **유지보수 가능**: 명확한 구조와 디자인 패턴

이 설계를 따르면 수천만 요소 규모의 mesh를 효율적으로 처리할 수 있는 견고한 시스템을 구축할 수 있습니다.
