# Core Module

이 디렉토리는 KooMeshPrepost의 핵심 데이터 모델과 비즈니스 로직을 포함합니다.

## 구조

```
core/
├── Node.cpp              # 노드 구현
├── Element.cpp           # 요소 구현 (추상 클래스)
├── Mesh.cpp              # Mesh 컨테이너 (Aggregate Root)
├── Part.cpp              # Part 그룹
├── Group.cpp             # 사용자 정의 그룹
├── GroupManager.cpp      # 그룹 관리자
├── ConfigManager.cpp     # 설정 관리 (Singleton)
├── MeshValidator.cpp     # Mesh 유효성 검사
├── Transform.cpp         # 좌표 변환
└── BoundingBox.cpp       # 경계 상자 유틸리티
```

## 주요 클래스

### Mesh (Aggregate Root)
- DDD (Domain-Driven Design)의 Aggregate Root 패턴
- 모든 노드, 요소, 파트를 관리
- 공간 인덱스를 보유

### Element Hierarchy
```
Element (abstract)
├── SolidElement
│   ├── TetrahedronElement
│   ├── HexahedronElement
│   ├── PentahedronElement
│   └── PyramidElement
└── ShellElement
    ├── TriangleElement
    └── QuadrilateralElement
```

## 의존성
- Eigen3: 벡터/행렬 연산
- STL: 컨테이너 및 알고리즘

## 사용 예제

```cpp
#include "core/Mesh.h"
#include "core/Node.h"
#include "core/Element.h"

using namespace koomesh::core;

// Mesh 생성
Mesh mesh;

// 노드 추가
mesh.addNode(Node(1, 0.0, 0.0, 0.0));
mesh.addNode(Node(2, 1.0, 0.0, 0.0));
mesh.addNode(Node(3, 0.0, 1.0, 0.0));
mesh.addNode(Node(4, 0.0, 0.0, 1.0));

// 요소 추가
std::vector<NodeId> nodeIds = {1, 2, 3, 4};
auto element = ElementFactory::create(
    ElementType::TETRAHEDRON,
    1, // element id
    1, // part id
    nodeIds
);
mesh.addElement(std::move(element));

// 공간 인덱스 구축
mesh.buildSpatialIndex();

// 조회
auto* node = mesh.getNode(1);
auto bbox = mesh.boundingBox();
```

## 테스트
- `tests/unit/test_mesh.cpp`
- `tests/unit/test_node.cpp`
- `tests/unit/test_element.cpp`
