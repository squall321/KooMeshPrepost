# Selection Module

요소 선택 및 공간 인덱싱을 담당하는 모듈입니다. 수천만 요소 중에서 빠른 검색과 선택을 지원합니다.

## 구조

```
selection/
├── SelectionManager.cpp       # 선택 관리 (메인)
├── ISpatialIndex.h            # 공간 인덱스 인터페이스
├── Octree.cpp                 # Octree 구현
├── RTree.cpp                  # R-Tree 구현
├── KdTree.cpp                 # K-d Tree 구현
├── UniformGrid.cpp            # Uniform Grid 구현
├── SpatialIndexFactory.cpp    # 인덱스 팩토리
└── SpatialIndexSerializer.cpp # 인덱스 직렬화/캐싱
```

## 공간 인덱싱 전략

### 1. Octree (기본 선택)

**장점**:
- 균형잡힌 성능
- 동적 구조
- 메모리 효율적

**단점**:
- 불균등 분포 시 비효율

**사용 시나리오**: 일반적인 mesh, 중간 크기 (10K - 10M 요소)

```cpp
Octree octree;
octree.setMaxDepth(10);
octree.setMaxElementsPerNode(100);
octree.build(mesh);

// 범위 검색
BoundingBox searchBox(0, 0, 0, 10, 10, 10);
auto elements = octree.query(searchBox);
```

**시간 복잡도**:
- 구축: O(n log n)
- 쿼리: O(log n + k), k = 결과 수
- 메모리: O(n)

### 2. R-Tree

**장점**:
- 동적 삽입/삭제 효율적
- 범위 쿼리 최적화

**단점**:
- 구현 복잡도 높음
- 구축 시간 김

**사용 시나리오**: 동적 mesh, 빈번한 수정

```cpp
RTree rtree;
rtree.build(mesh);

// 동적 삽입
rtree.insertElement(newElementId, boundingBox);
```

### 3. K-d Tree

**장점**:
- 점 검색 최적화
- 최근접 이웃 빠름

**단점**:
- 범위 쿼리 느림
- 동적 업데이트 어려움

**사용 시나리오**: 노드 검색, 최근접 쿼리

```cpp
KdTree kdtree;
kdtree.build(mesh.getAllNodes());

// 최근접 노드 찾기
Point3D point(1.5, 2.3, 0.8);
NodeId nearest = kdtree.findNearest(point);

// 반경 검색
auto nodesInRadius = kdtree.findWithinRadius(point, radius: 5.0);
```

### 4. Uniform Grid

**장점**:
- 매우 빠른 쿼리 (O(1))
- 구현 단순

**단점**:
- 메모리 사용량 많음
- 불균등 분포 비효율

**사용 시나리오**: 대용량 mesh (10M+ 요소), 메모리 충분

```cpp
UniformGrid grid;
grid.setGridSize(100, 100, 100); // 100x100x100 cells
grid.build(mesh);

// 매우 빠른 쿼리
auto elements = grid.query(searchBox); // O(1) per cell
```

## SelectionManager

중앙 선택 관리 시스템:

```cpp
class SelectionManager {
public:
    enum SelectionMode {
        REPLACE,   // 기존 선택 교체
        ADD,       // 선택에 추가
        SUBTRACT,  // 선택에서 제거
        INTERSECT  // 교집합
    };

    void setMode(SelectionMode mode);

    // 다양한 선택 방법
    void selectElements(const std::vector<ElementId>& elements);
    void selectElementsInBox(const BoundingBox& box);
    void selectElementsInSphere(const Point3D& center, double radius);
    void selectByPart(PartId partId);
    void selectByQuality(double minQuality, double maxQuality);

    // 선택 조작
    void invertSelection();
    void clearSelection();
    void selectAll();

    // 조회
    const std::vector<ElementId>& getSelectedElements() const;
    size_t selectionCount() const;

    // 콜백
    std::function<void(const std::vector<ElementId>&)> onSelectionChanged;
};
```

## 사용 예제

### 기본 선택

```cpp
SelectionManager selMgr;

// REPLACE 모드 (기본)
selMgr.setMode(SelectionMode::REPLACE);
selMgr.selectElementsInBox(BoundingBox(0, 0, 0, 10, 10, 10));

// ADD 모드
selMgr.setMode(SelectionMode::ADD);
selMgr.selectByPart(partId: 5);

std::cout << "Selected: " << selMgr.selectionCount() << " elements\n";
```

### 품질 기반 선택

```cpp
// 품질이 낮은 요소 선택
selMgr.selectByQuality(minQuality: 0.0, maxQuality: 0.3);

// 선택 반전 (품질 높은 요소만)
selMgr.invertSelection();
```

### 공간 인덱스 자동 선택

```cpp
SpatialIndexFactory factory;

// 요소 수에 따라 최적 인덱스 자동 선택
auto indexType = factory.selectOptimal(mesh.elementCount());

auto spatialIndex = factory.create(indexType);
spatialIndex->build(mesh);
```

**자동 선택 규칙**:
- < 10K 요소: Linear (인덱스 없음)
- 10K - 100K: K-d Tree
- 100K - 10M: Octree
- 10M+: Uniform Grid (메모리 충분 시) 또는 Octree

## Octree 상세 구조

```
                    Root [0,100] x [0,100] x [0,100]
                             │
         ┌───────────────────┴───────────────────┐
         │                                       │
    [0,50]x[0,50]x[0,50]              [50,100]x[50,100]x[50,100]
         │                                       │
    ┌────┴────┐                            ┌────┴────┐
    │         │                            │         │
[0,25]  [25,50]                      [50,75]  [75,100]
   │         │                            │         │
Elements  Elements                   Elements  Elements
(리프)    (리프)                       (리프)    (리프)
```

### Octree 파라미터 튜닝

```cpp
Octree octree;

// 최대 깊이 (메모리 vs 정확도)
octree.setMaxDepth(8);  // 얕음: 빠르지만 부정확
octree.setMaxDepth(12); // 깊음: 느리지만 정확
// 권장: 10

// 노드당 최대 요소 수
octree.setMaxElementsPerNode(50);  // 작음: 깊은 트리
octree.setMaxElementsPerNode(200); // 큼: 얕은 트리
// 권장: 100

octree.build(mesh);
```

## 인덱스 캐싱 (성능 최적화)

대형 mesh의 경우 인덱스 구축에 시간이 걸리므로 캐싱:

```cpp
SpatialIndexSerializer serializer;

// 인덱스 저장
serializer.save(octree, "mesh_index.cache");

// 다음 실행 시 로드 (구축 시간 절약)
auto loadedIndex = serializer.load("mesh_index.cache");
```

## 선택 알고리즘 비교

### 1. 단순 선형 검색
```cpp
std::vector<ElementId> selectInBox(const BoundingBox& box) {
    std::vector<ElementId> result;
    for (const auto& [id, element] : mesh.elements()) {
        if (box.intersects(element->boundingBox(mesh))) {
            result.push_back(id);
        }
    }
    return result;
}
```
**시간 복잡도**: O(n)

### 2. Octree 검색
```cpp
std::vector<ElementId> selectInBox(const BoundingBox& box) {
    return octree.query(box);
}
```
**시간 복잡도**: O(log n + k)

### 성능 비교

| 요소 수 | 선형 검색 | Octree | 속도 향상 |
|---------|----------|--------|----------|
| 1K | 0.5ms | 0.3ms | 1.7x |
| 10K | 5ms | 0.4ms | 12.5x |
| 100K | 52ms | 0.8ms | 65x |
| 1M | 530ms | 2ms | 265x |
| 10M | 5500ms | 15ms | 367x |

## 병렬 쿼리

대량 쿼리 시 병렬 처리:

```cpp
#include <tbb/parallel_for.h>

std::vector<BoundingBox> queries = { /* ... */ };
std::vector<std::vector<ElementId>> results(queries.size());

tbb::parallel_for(
    tbb::blocked_range<size_t>(0, queries.size()),
    [&](const tbb::blocked_range<size_t>& range) {
        for (size_t i = range.begin(); i < range.end(); ++i) {
            results[i] = octree.query(queries[i]);
        }
    }
);
```

## 테스트
- `tests/unit/test_octree.cpp`
- `tests/unit/test_selection_manager.cpp`
- `tests/integration/test_spatial_index_performance.cpp`
