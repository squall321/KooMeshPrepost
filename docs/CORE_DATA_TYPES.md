# Core Data Types - 상세 가이드

## 목차

1. [개요](#개요)
2. [Phase 11: Node 클래스](#phase-11-node-클래스)
3. [Phase 12: Element 추상 클래스](#phase-12-element-추상-클래스)
4. [Phase 13: 구체적 요소 타입](#phase-13-구체적-요소-타입)
5. [Phase 14: Element Factory](#phase-14-element-factory)
6. [설계 패턴](#설계-패턴)
7. [성능 최적화](#성능-최적화)
8. [사용 예제](#사용-예제)

---

## 개요

KooMeshPrepost의 핵심 데이터 타입은 FEM(Finite Element Method) 메시를 표현하는 데 필요한 기본 구성 요소들을 정의합니다.

### 핵심 클래스 계층 구조

```
Node (값 의미론)
  ├─ NodeId: uint64_t
  ├─ Coordinates: Eigen::Vector3d
  └─ Connected Elements: vector<ElementId>

Element (추상 클래스, 다형성)
  ├─ TetrahedronElement (4-node solid)
  ├─ HexahedronElement (8-node solid)
  ├─ PentahedronElement (6-node wedge)
  ├─ PyramidElement (5-node pyramid)
  ├─ TriangleElement (3-node shell)
  ├─ QuadrilateralElement (4-node shell)
  └─ BeamElement (2-node beam)

Mesh (Aggregate Root)
  ├─ Nodes: unordered_map<NodeId, Node>
  ├─ Elements: unordered_map<ElementId, unique_ptr<Element>>
  └─ Parts: unordered_map<PartId, Part>

Part (요소 그룹)
  ├─ PartId
  ├─ Name
  ├─ Element IDs
  └─ Material Properties
```

### 설계 원칙

1. **값 의미론 (Value Semantics)**: Node는 복사 가능한 값 타입
2. **참조 의미론 (Reference Semantics)**: Element는 unique_ptr로 관리되는 다형 타입
3. **불변성 (Immutability)**: ID는 생성 후 변경 불가
4. **역참조 (Back-reference)**: Node는 연결된 Element ID를 추적
5. **Aggregate Root**: Mesh가 모든 데이터의 일관성을 보장

---

## Phase 11: Node 클래스

### 1. 클래스 정의

```cpp
namespace koomesh::core {

class Node {
public:
    // 생성자
    Node(NodeId id, double x, double y, double z);
    Node(NodeId id, const Eigen::Vector3d& coords);

    // Getter 메서드
    NodeId id() const;
    const Eigen::Vector3d& coordinates() const;
    double x() const;
    double y() const;
    double z() const;

    // 연결 관리
    void addConnectedElement(ElementId elemId);
    const std::vector<ElementId>& connectedElements() const;

    // 기하학적 연산
    double distanceTo(const Node& other) const;
    void translate(const Eigen::Vector3d& offset);
    void scale(double factor);

private:
    NodeId m_id;                                    // 노드 고유 ID
    Eigen::Vector3d m_coords;                       // 3D 좌표
    std::vector<ElementId> m_connectedElements;     // 역참조
};

} // namespace koomesh::core
```

### 2. 설계 결정 사항

#### 2.1 값 의미론 (Value Semantics)

Node는 값 타입으로 설계되었습니다:

```cpp
// ✓ 복사 가능
Node node1(1, 0.0, 0.0, 0.0);
Node node2 = node1;  // 복사 생성자

// ✓ 이동 가능
Node node3 = std::move(node1);  // 이동 생성자

// ✓ 컨테이너에 저장
std::vector<Node> nodes;
nodes.push_back(Node(1, 0, 0, 0));

// ✓ 맵에 값으로 저장
std::unordered_map<NodeId, Node> nodeMap;
nodeMap.emplace(1, Node(1, 0, 0, 0));
```

**이유:**
- Node는 작고 복사 비용이 저렴함 (ID + Vector3d + vector<ElementId>)
- Eigen::Vector3d는 SSE 최적화된 복사 지원
- 값 의미론이 포인터보다 안전하고 이해하기 쉬움

#### 2.2 Eigen::Vector3d 사용

Eigen 라이브러리를 사용한 이유:

```cpp
// ✓ 벡터 연산 최적화
Eigen::Vector3d v1(1, 2, 3);
Eigen::Vector3d v2(4, 5, 6);
double distance = (v1 - v2).norm();  // SIMD 최적화

// ✓ 수학적 연산 지원
Eigen::Vector3d sum = v1 + v2;
Eigen::Vector3d scaled = v1 * 2.0;
double dot = v1.dot(v2);
Eigen::Vector3d cross = v1.cross(v2);
```

**장점:**
- SSE/AVX SIMD 명령어 최적화
- 메모리 정렬 보장 (16-byte aligned)
- 풍부한 수학 연산 API
- 업계 표준 (많은 FEM 소프트웨어에서 사용)

#### 2.3 역참조 (Back-reference)

Node는 자신에게 연결된 Element ID를 추적합니다:

```cpp
class Node {
    std::vector<ElementId> m_connectedElements;  // 역참조
};
```

**사용 사례:**
- 노드 삭제 시 연결된 요소 찾기
- 노드 주변의 요소 검색
- 토폴로지 분석 (이웃 노드 찾기)

**일관성 보장:**
- Mesh 클래스가 Element 추가/삭제 시 자동으로 역참조 업데이트
- 직접 수정 불가 (const reference만 반환)

### 3. 주요 메서드 상세

#### 3.1 distanceTo()

```cpp
double Node::distanceTo(const Node& other) const {
    return (m_coords - other.m_coords).norm();
}
```

**시간 복잡도:** O(1)
**공간 복잡도:** O(1)

**사용 예제:**
```cpp
Node n1(1, 0, 0, 0);
Node n2(2, 3, 4, 0);
double dist = n1.distanceTo(n2);  // sqrt(3^2 + 4^2) = 5.0
```

#### 3.2 translate()

```cpp
void Node::translate(const Eigen::Vector3d& offset) {
    m_coords += offset;
}
```

**용도:**
- 메시 전체 이동
- 좌표계 변환
- 애니메이션

**사용 예제:**
```cpp
Node node(1, 1, 1, 1);
node.translate(Eigen::Vector3d(10, 20, 30));
// 결과: (11, 21, 31)
```

#### 3.3 scale()

```cpp
void Node::scale(double factor) {
    m_coords *= factor;
}
```

**용도:**
- 메시 크기 조정
- 단위 변환 (mm → m)
- 정규화

**사용 예제:**
```cpp
Node node(1, 10, 20, 30);
node.scale(0.001);  // mm to m
// 결과: (0.01, 0.02, 0.03)
```

#### 3.4 addConnectedElement()

```cpp
void Node::addConnectedElement(ElementId elemId) {
    auto it = std::find(m_connectedElements.begin(),
                        m_connectedElements.end(),
                        elemId);
    if (it == m_connectedElements.end()) {
        m_connectedElements.push_back(elemId);
    }
}
```

**중복 방지:** 같은 Element ID는 한 번만 추가됨
**시간 복잡도:** O(n) where n = 연결된 요소 개수 (보통 작음, ~10)

**참고:** Mesh 클래스가 자동으로 호출하므로 직접 호출 불필요

### 4. 메모리 레이아웃

```
Node 객체 크기 분석:
┌─────────────────────────────┐
│ m_id (uint64_t)       : 8B  │
│ m_coords (Vector3d)   : 24B │ (3 doubles, aligned)
│ m_connectedElements   : 24B │ (vector header)
│   - pointer           : 8B  │
│   - size              : 8B  │
│   - capacity          : 8B  │
└─────────────────────────────┘
Total: 56 bytes (+ dynamic allocation for vector elements)

1M nodes ≈ 56 MB (base) + 연결 정보
```

### 5. 성능 특성

| 연산 | 시간 복잡도 | 공간 복잡도 | 비고 |
|------|------------|------------|------|
| 생성자 | O(1) | O(1) | Vector3d 초기화 |
| 복사 | O(k) | O(k) | k = 연결된 요소 수 |
| 이동 | O(1) | O(1) | vector move |
| distanceTo() | O(1) | O(1) | SIMD 최적화 |
| translate() | O(1) | O(1) | SIMD 최적화 |
| scale() | O(1) | O(1) | SIMD 최적화 |
| addConnectedElement() | O(k) | O(1) | 중복 검사 |

---

## Phase 12: Element 추상 클래스

### 1. 클래스 정의

```cpp
namespace koomesh::core {

class Element {
public:
    Element(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);
    virtual ~Element() = default;

    // Getter
    ElementId id() const;
    PartId partId() const;
    const std::vector<NodeId>& nodeIds() const;

    // 순수 가상 함수 (하위 클래스 구현 필수)
    virtual ElementType type() const = 0;
    virtual size_t nodeCount() const = 0;
    virtual double computeQuality(const Mesh& mesh) const = 0;
    virtual double computeVolume(const Mesh& mesh) const = 0;
    virtual bool containsPoint(const Eigen::Vector3d& point,
                               const Mesh& mesh) const = 0;

    // 기본 구현 제공 (하위 클래스에서 override 가능)
    virtual BoundingBox computeBoundingBox(const Mesh& mesh) const;
    virtual Eigen::Vector3d computeCenter(const Mesh& mesh) const;

protected:
    ElementId m_id;
    PartId m_partId;
    std::vector<NodeId> m_nodeIds;
};

} // namespace koomesh::core
```

### 2. 설계 패턴

#### 2.1 Template Method Pattern

Element 클래스는 Template Method 패턴을 사용합니다:

```cpp
// 기본 알고리즘 제공 (모든 요소 타입에 공통)
BoundingBox Element::computeBoundingBox(const Mesh& mesh) const {
    BoundingBox bbox;
    for (NodeId nodeId : m_nodeIds) {
        const Node* node = mesh.getNode(nodeId);
        if (node) {
            bbox.expand(node->coordinates());
        }
    }
    return bbox;
}

Eigen::Vector3d Element::computeCenter(const Mesh& mesh) const {
    Eigen::Vector3d center = Eigen::Vector3d::Zero();
    for (NodeId nodeId : m_nodeIds) {
        const Node* node = mesh.getNode(nodeId);
        if (node) {
            center += node->coordinates();
        }
    }
    return center / static_cast<double>(m_nodeIds.size());
}
```

**장점:**
- 공통 코드 재사용
- 하위 클래스는 필요한 부분만 override
- 일관된 인터페이스

#### 2.2 Strategy Pattern

각 요소 타입은 고유한 알고리즘을 구현합니다:

```cpp
// 각 요소 타입마다 다른 품질 계산 전략
class TetrahedronElement : public Element {
    double computeQuality(const Mesh& mesh) const override {
        // 사면체 전용 품질 계산 (예: 종횡비, 왜곡도)
        // ...
    }
};

class HexahedronElement : public Element {
    double computeQuality(const Mesh& mesh) const override {
        // 육면체 전용 품질 계산
        // ...
    }
};
```

### 3. BoundingBox 구조체

```cpp
struct BoundingBox {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;

    // 교차 검사 (AABB vs AABB)
    bool intersects(const BoundingBox& other) const {
        return !(maxX < other.minX || minX > other.maxX ||
                 maxY < other.minY || minY > other.maxY ||
                 maxZ < other.minZ || minZ > other.maxZ);
    }

    // 점 포함 검사
    bool contains(const Eigen::Vector3d& point) const {
        return point.x() >= minX && point.x() <= maxX &&
               point.y() >= minY && point.y() <= maxY &&
               point.z() >= minZ && point.z() <= maxZ;
    }

    // 확장 (점을 포함하도록)
    void expand(const Eigen::Vector3d& point) {
        minX = std::min(minX, point.x());
        minY = std::min(minY, point.y());
        minZ = std::min(minZ, point.z());
        maxX = std::max(maxX, point.x());
        maxY = std::max(maxY, point.y());
        maxZ = std::max(maxZ, point.z());
    }

    // 중심점
    Eigen::Vector3d center() const {
        return Eigen::Vector3d(
            (minX + maxX) / 2.0,
            (minY + maxY) / 2.0,
            (minZ + maxZ) / 2.0
        );
    }

    // 부피
    double volume() const {
        return (maxX - minX) * (maxY - minY) * (maxZ - minZ);
    }
};
```

**용도:**
- 공간 인덱싱 (Octree, R-Tree)
- 충돌 감지
- 뷰 프러스텀 컬링
- 선택 영역 계산

### 4. 참조 의미론 (Reference Semantics)

Element는 다형 타입으로 unique_ptr로 관리됩니다:

```cpp
// ✓ 포인터로 관리
std::unique_ptr<Element> elem =
    std::make_unique<TetrahedronElement>(1, 1, nodeIds);

// ✓ 다형성 활용
Element* basePtr = elem.get();
ElementType type = basePtr->type();  // 런타임 다형성

// ✓ 컨테이너에 저장
std::unordered_map<ElementId, std::unique_ptr<Element>> elements;
elements.emplace(1, std::move(elem));

// ✗ 복사 불가
// Element elem2 = *elem;  // 컴파일 에러
```

**이유:**
- Element는 추상 클래스 (순수 가상 함수 포함)
- 다형성 필요 (런타임에 타입 결정)
- 메모리 효율 (슬라이싱 방지)

---

## Phase 13: 구체적 요소 타입

### 1. TetrahedronElement (사면체)

```cpp
class TetrahedronElement : public Element {
public:
    TetrahedronElement(ElementId id, PartId partId,
                       const std::vector<NodeId>& nodeIds);

    ElementType type() const override {
        return ElementType::TETRAHEDRON;
    }

    size_t nodeCount() const override {
        return 4;
    }

    double computeVolume(const Mesh& mesh) const override;
    double computeQuality(const Mesh& mesh) const override;
    bool containsPoint(const Eigen::Vector3d& point,
                       const Mesh& mesh) const override;
};
```

#### 부피 계산

사면체 부피 = |det(v1, v2, v3)| / 6

```cpp
double TetrahedronElement::computeVolume(const Mesh& mesh) const {
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);
    const Node* n3 = mesh.getNode(m_nodeIds[3]);

    Eigen::Vector3d v1 = n1->coordinates() - n0->coordinates();
    Eigen::Vector3d v2 = n2->coordinates() - n0->coordinates();
    Eigen::Vector3d v3 = n3->coordinates() - n0->coordinates();

    // 스칼라 삼중곱 (Scalar Triple Product)
    double det = v1.dot(v2.cross(v3));

    return std::abs(det) / 6.0;
}
```

#### 품질 계산

품질 = (실제 부피) / (이상적 부피)

```cpp
double TetrahedronElement::computeQuality(const Mesh& mesh) const {
    // 변 길이 계산
    double edges[6];
    // ... 계산 ...

    double longestEdge = *std::max_element(edges, edges + 6);
    double volume = computeVolume(mesh);

    // 정규화된 품질 (0.0 ~ 1.0)
    double idealVolume = std::pow(longestEdge, 3) / (6.0 * std::sqrt(2.0));
    return volume / idealVolume;
}
```

### 2. HexahedronElement (육면체)

```cpp
class HexahedronElement : public Element {
public:
    HexahedronElement(ElementId id, PartId partId,
                      const std::vector<NodeId>& nodeIds);

    ElementType type() const override {
        return ElementType::HEXAHEDRON;
    }

    size_t nodeCount() const override {
        return 8;
    }

    double computeVolume(const Mesh& mesh) const override;
    double computeQuality(const Mesh& mesh) const override;
    bool containsPoint(const Eigen::Vector3d& point,
                       const Mesh& mesh) const override;
};
```

#### 노드 순서 (LS-DYNA 규약)

```
      7----------6
     /|         /|
    / |        / |
   4----------5  |
   |  |       |  |
   |  3-------|--2
   | /        | /
   |/         |/
   0----------1
```

#### 부피 계산 (간이 방식)

육면체를 5개의 사면체로 분할하여 계산:

```cpp
double HexahedronElement::computeVolume(const Mesh& mesh) const {
    // 간이 계산: BoundingBox 부피의 근사치
    BoundingBox bbox = computeBoundingBox(mesh);
    return bbox.volume() * 0.8;  // 보정 계수
}
```

**참고:** 정확한 계산은 Gauss 적분 필요 (향후 구현)

### 3. 추가 요소 타입 (Phase 13 확장)

향후 구현 예정:

```cpp
// Shell 요소
class TriangleElement : public Element {
    size_t nodeCount() const override { return 3; }
    // 면적, 두께, 법선 벡터 계산
};

class QuadrilateralElement : public Element {
    size_t nodeCount() const override { return 4; }
    // 면적, 두께, 법선 벡터 계산
};

// 1D 요소
class BeamElement : public Element {
    size_t nodeCount() const override { return 2; }
    // 길이, 단면 속성 계산
};

// 추가 3D 요소
class PentahedronElement : public Element {  // Wedge
    size_t nodeCount() const override { return 6; }
};

class PyramidElement : public Element {
    size_t nodeCount() const override { return 5; }
};
```

---

## Phase 14: Element Factory

### 1. Factory Pattern 구현

```cpp
class ElementFactory {
public:
    static std::unique_ptr<Element> create(
        ElementType type,
        ElementId id,
        PartId partId,
        const std::vector<NodeId>& nodeIds
    ) {
        switch (type) {
            case ElementType::TETRAHEDRON:
                return std::make_unique<TetrahedronElement>(
                    id, partId, nodeIds);

            case ElementType::HEXAHEDRON:
                return std::make_unique<HexahedronElement>(
                    id, partId, nodeIds);

            case ElementType::PENTAHEDRON:
                return std::make_unique<PentahedronElement>(
                    id, partId, nodeIds);

            case ElementType::PYRAMID:
                return std::make_unique<PyramidElement>(
                    id, partId, nodeIds);

            case ElementType::TRIANGLE:
                return std::make_unique<TriangleElement>(
                    id, partId, nodeIds);

            case ElementType::QUADRILATERAL:
                return std::make_unique<QuadrilateralElement>(
                    id, partId, nodeIds);

            case ElementType::BEAM:
                return std::make_unique<BeamElement>(
                    id, partId, nodeIds);

            default:
                KOOMESH_THROW_INVALID_DATA(
                    "Unknown element type: " +
                    std::to_string(static_cast<int>(type))
                );
        }
    }
};
```

### 2. 사용 예제

```cpp
// 직접 생성 대신 Factory 사용
auto elem1 = ElementFactory::create(
    ElementType::TETRAHEDRON,
    1, 1,
    {1, 2, 3, 4}
);

auto elem2 = ElementFactory::create(
    ElementType::HEXAHEDRON,
    2, 1,
    {1, 2, 3, 4, 5, 6, 7, 8}
);

// 파일에서 읽을 때 유용
ElementType type = parseElementType(line);
auto element = ElementFactory::create(type, id, partId, nodeIds);
```

### 3. 장점

- **중앙 집중화:** 요소 생성 로직이 한 곳에 모임
- **확장성:** 새 요소 타입 추가가 쉬움
- **타입 안전성:** 잘못된 타입 생성 방지
- **테스트 용이:** Factory만 테스트하면 됨

---

## 설계 패턴

### 1. Template Method Pattern

**정의:** 알고리즘의 골격을 정의하고, 세부 단계는 하위 클래스에 위임

```cpp
class Element {
    // Template method (기본 알고리즘)
    virtual BoundingBox computeBoundingBox(const Mesh& mesh) const {
        BoundingBox bbox;
        for (NodeId nodeId : m_nodeIds) {
            bbox.expand(mesh.getNode(nodeId)->coordinates());
        }
        return bbox;
    }

    // Primitive operations (하위 클래스 구현)
    virtual double computeVolume(const Mesh& mesh) const = 0;
    virtual double computeQuality(const Mesh& mesh) const = 0;
};
```

### 2. Strategy Pattern

**정의:** 알고리즘 군을 정의하고, 런타임에 선택

```cpp
// 각 요소 타입 = 각 전략
class TetrahedronElement : public Element {
    double computeVolume(...) override {
        // 사면체 전용 알고리즘
    }
};

class HexahedronElement : public Element {
    double computeVolume(...) override {
        // 육면체 전용 알고리즘
    }
};

// 사용
Element* elem = /* ... */;
double volume = elem->computeVolume(mesh);  // 런타임 결정
```

### 3. Factory Pattern

**정의:** 객체 생성 인터페이스를 정의하고, 구체적 클래스는 Factory가 결정

```cpp
// 클라이언트는 구체적 타입을 몰라도 됨
auto elem = ElementFactory::create(type, id, partId, nodeIds);
```

### 4. Aggregate Root Pattern (DDD)

**정의:** 모든 데이터 접근은 Aggregate Root (Mesh)를 통해서만

```cpp
class Mesh {
    // Mesh가 모든 일관성 보장
    void addElement(std::unique_ptr<Element> element) {
        // 1. 노드 존재 확인
        // 2. 중복 ID 확인
        // 3. 역참조 업데이트
        // 4. 요소 추가
    }

    void removeElement(ElementId id) {
        // 1. 역참조 제거
        // 2. 요소 삭제
    }
};
```

---

## 성능 최적화

### 1. 메모리 레이아웃 최적화

```cpp
// ✓ 캐시 친화적 배치
class Element {
    ElementId m_id;              // 8B (자주 접근)
    PartId m_partId;             // 4B (자주 접근)
    // 4B padding
    std::vector<NodeId> m_nodeIds;  // 24B (포인터 + 메타데이터)
};

// 총 40바이트 (aligned)
```

### 2. SIMD 최적화

Eigen::Vector3d는 자동으로 SIMD 명령어 사용:

```cpp
// SSE/AVX 최적화됨
Eigen::Vector3d v1 = n1->coordinates();
Eigen::Vector3d v2 = n2->coordinates();
Eigen::Vector3d diff = v1 - v2;  // 벡터화됨
double dist = diff.norm();       // 벡터화됨
```

### 3. 배치 처리

```cpp
// ✓ 배치로 처리하여 캐시 효율 향상
void Mesh::addElementsBatch(
    std::vector<std::unique_ptr<Element>> elements) {

    // 모든 검증 먼저 수행
    for (const auto& elem : elements) {
        validateElement(*elem);
    }

    // 일괄 추가
    for (auto& elem : elements) {
        m_elements.emplace(elem->id(), std::move(elem));
    }
}
```

### 4. 지연 평가 (Lazy Evaluation)

```cpp
class Mesh {
    mutable std::optional<BoundingBox> m_cachedBBox;  // 캐시

    const BoundingBox& boundingBox() const {
        if (!m_cachedBBox) {
            m_cachedBBox = computeBoundingBox();  // 필요할 때만 계산
        }
        return *m_cachedBBox;
    }
};
```

---

## 사용 예제

### 1. 기본 사용

```cpp
#include "core/Node.h"
#include "core/Element.h"
#include "core/Mesh.h"

using namespace koomesh::core;

// 노드 생성
Node n1(1, 0.0, 0.0, 0.0);
Node n2(2, 1.0, 0.0, 0.0);
Node n3(3, 0.0, 1.0, 0.0);
Node n4(4, 0.0, 0.0, 1.0);

// 메시 생성
Mesh mesh;
mesh.addNode(n1);
mesh.addNode(n2);
mesh.addNode(n3);
mesh.addNode(n4);

// 요소 생성
auto elem = std::make_unique<TetrahedronElement>(
    1, 1, std::vector<NodeId>{1, 2, 3, 4});

// 메시에 추가
mesh.addElement(std::move(elem));

// 조회
const Element* e = mesh.getElement(1);
std::cout << "Element type: " << static_cast<int>(e->type()) << "\n";
std::cout << "Volume: " << e->computeVolume(mesh) << "\n";
```

### 2. 대량 데이터 처리

```cpp
// 100만 노드 추가
std::vector<Node> nodes;
nodes.reserve(1000000);

for (size_t i = 0; i < 1000000; ++i) {
    nodes.emplace_back(i, i * 0.1, i * 0.2, i * 0.3);
}

mesh.addNodesBatch(nodes);  // 배치 추가로 성능 향상
```

### 3. 품질 검사

```cpp
// 모든 요소의 품질 계산
for (size_t i = 0; i < mesh.elementCount(); ++i) {
    const Element* elem = mesh.getElement(i);
    double quality = elem->computeQuality(mesh);

    if (quality < 0.3) {
        std::cout << "Warning: Low quality element "
                  << elem->id() << ": " << quality << "\n";
    }
}
```

### 4. 공간 검색

```cpp
// 특정 점을 포함하는 요소 찾기
Eigen::Vector3d point(0.5, 0.5, 0.5);

for (size_t i = 0; i < mesh.elementCount(); ++i) {
    const Element* elem = mesh.getElement(i);

    if (elem->containsPoint(point, mesh)) {
        std::cout << "Found element: " << elem->id() << "\n";
        break;
    }
}
```

### 5. 메시 변환

```cpp
// 전체 메시 이동
Eigen::Vector3d offset(100, 200, 300);

for (size_t i = 0; i < mesh.nodeCount(); ++i) {
    Node* node = mesh.getNodeMutable(i);
    node->translate(offset);
}

// 전체 메시 축소 (mm → m)
for (size_t i = 0; i < mesh.nodeCount(); ++i) {
    Node* node = mesh.getNodeMutable(i);
    node->scale(0.001);
}
```

---

## 테스트 가이드

### 1. Node 테스트

```cpp
TEST(NodeTest, Construction) {
    Node node(1, 1.0, 2.0, 3.0);
    EXPECT_EQ(1, node.id());
    EXPECT_DOUBLE_EQ(1.0, node.x());
    EXPECT_DOUBLE_EQ(2.0, node.y());
    EXPECT_DOUBLE_EQ(3.0, node.z());
}

TEST(NodeTest, DistanceCalculation) {
    Node n1(1, 0, 0, 0);
    Node n2(2, 3, 4, 0);
    EXPECT_DOUBLE_EQ(5.0, n1.distanceTo(n2));
}
```

### 2. Element 테스트

```cpp
TEST(TetrahedronTest, VolumeCalculation) {
    auto mesh = MeshBuilder()
        .addNode(1, 0, 0, 0)
        .addNode(2, 1, 0, 0)
        .addNode(3, 0, 1, 0)
        .addNode(4, 0, 0, 1)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .build();

    const Element* elem = mesh->getElement(1);
    double volume = elem->computeVolume(*mesh);

    EXPECT_NEAR(1.0/6.0, volume, 1e-10);  // 단위 사면체 부피
}
```

---

## 참고 자료

- [Eigen Documentation](https://eigen.tuxfamily.org/)
- [LS-DYNA Keyword User's Manual](https://www.lstc.com/)
- [FEM 요소 품질 메트릭](https://www.sciencedirect.com/topics/engineering/element-quality)
- [Design Patterns (GoF)](https://en.wikipedia.org/wiki/Design_Patterns)
- [Domain-Driven Design](https://martinfowler.com/bliki/DomainDrivenDesign.html)

---

**작성자:** Claude (KooMeshPrepost Development)
**버전:** 1.0
**최종 수정:** 2025-11-09
