# Examples

KooMeshPrepost 사용 예제 모음입니다.

## 디렉토리 구조

```
examples/
├── 01_basic_loading/        # 기본 파일 로딩
├── 02_element_selection/    # 요소 선택
├── 03_group_management/     # 그룹 관리
├── 04_visualization/        # 가시화 설정
├── 05_scripting/            # Python 스크립팅 (향후)
└── sample_data/             # 샘플 mesh 파일
```

## 예제 1: 기본 파일 로딩

**파일**: `01_basic_loading/main.cpp`

```cpp
#include <iostream>
#include "core/Mesh.h"
#include "io/DynaFileReader.h"

using namespace koomesh;

int main() {
    // Mesh 생성
    core::Mesh mesh;

    // 파일 리더 생성
    io::DynaFileReader reader;

    // 파일 로딩
    std::string filepath = "../sample_data/cube.k";

    if (reader.read(filepath, mesh)) {
        std::cout << "Successfully loaded mesh!\n";
        std::cout << "Nodes: " << mesh.nodeCount() << "\n";
        std::cout << "Elements: " << mesh.elementCount() << "\n";
        std::cout << "Parts: " << mesh.partCount() << "\n";

        // 경계 상자
        auto bbox = mesh.boundingBox();
        std::cout << "Bounding box:\n";
        std::cout << "  Min: (" << bbox.minX() << ", " << bbox.minY() << ", " << bbox.minZ() << ")\n";
        std::cout << "  Max: (" << bbox.maxX() << ", " << bbox.maxY() << ", " << bbox.maxZ() << ")\n";

    } else {
        std::cerr << "Failed to load mesh!\n";
        return 1;
    }

    return 0;
}
```

**실행**:
```bash
cd examples/01_basic_loading
mkdir build && cd build
cmake ..
make
./basic_loading
```

## 예제 2: 요소 선택

**파일**: `02_element_selection/main.cpp`

```cpp
#include "core/Mesh.h"
#include "selection/SelectionManager.h"
#include "selection/Octree.h"
#include "io/DynaFileReader.h"

int main() {
    // Mesh 로딩
    core::Mesh mesh;
    io::DynaFileReader reader;
    reader.read("../sample_data/sphere.k", mesh);

    // 공간 인덱스 구축
    mesh.buildSpatialIndex();

    // SelectionManager 생성
    selection::SelectionManager selMgr;

    // 영역 선택
    core::BoundingBox searchBox(-5, -5, -5, 5, 5, 5);
    selMgr.selectElementsInBox(searchBox);

    std::cout << "Selected " << selMgr.selectionCount() << " elements\n";

    // 선택된 요소 정보 출력
    for (auto elemId : selMgr.getSelectedElements()) {
        auto* element = mesh.getElement(elemId);
        std::cout << "Element " << elemId << " (Type: "
                  << static_cast<int>(element->type()) << ")\n";
    }

    return 0;
}
```

## 예제 3: 그룹 관리

**파일**: `03_group_management/main.cpp`

```cpp
#include "core/Mesh.h"
#include "core/GroupManager.h"
#include "io/DynaFileReader.h"

int main() {
    // Mesh 로딩
    core::Mesh mesh;
    io::DynaFileReader reader;
    reader.read("../sample_data/car_chassis.k", mesh);

    // GroupManager 생성
    core::GroupManager groupMgr;

    // 그룹 1: 품질이 낮은 요소
    std::vector<core::ElementId> lowQualityElements;
    for (const auto& [id, element] : mesh.elements()) {
        double quality = element->computeQuality(mesh);
        if (quality < 0.3) {
            lowQualityElements.push_back(id);
        }
    }

    groupMgr.createGroup("Low Quality Elements");
    auto* group1 = groupMgr.getGroup("Low Quality Elements");
    for (auto elemId : lowQualityElements) {
        group1->addElement(elemId);
    }

    std::cout << "Low Quality Elements: " << group1->size() << "\n";

    // 그룹 2: Part 1의 모든 요소
    groupMgr.createGroup("Part 1 Elements");
    auto* group2 = groupMgr.getGroup("Part 1 Elements");

    auto* part1 = mesh.getPart(1);
    if (part1) {
        for (auto elemId : part1->elements()) {
            group2->addElement(elemId);
        }
        std::cout << "Part 1 Elements: " << group2->size() << "\n";
    }

    // 그룹 저장 (LS-DYNA 파일로)
    io::DynaFileWriter writer;
    writer.writeGroup("low_quality.k", *group1);

    return 0;
}
```

## 예제 4: 가시화 설정

**파일**: `04_visualization/main.cpp`

```cpp
#include <QApplication>
#include "core/Mesh.h"
#include "visualization/VTKRenderer.h"
#include "visualization/MeshToVTK.h"
#include "io/DynaFileReader.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Mesh 로딩
    core::Mesh mesh;
    io::DynaFileReader reader;
    reader.read("../sample_data/beam.k", mesh);

    // VTK 렌더러 초기화
    visualization::VTKRenderer renderer;
    renderer.initialize();
    renderer.setBackgroundColor(0.2, 0.2, 0.2);

    // Mesh를 VTK로 변환
    auto vtkData = visualization::MeshToVTK::convert(mesh);

    // 가시화 옵션 설정
    visualization::VisualizationOptions opts;
    opts.displayMode = visualization::VisualizationOptions::SURFACE_WITH_EDGES;
    opts.showEdges = true;
    opts.edgeWidth = 2.0;
    renderer.setOptions(opts);

    // 렌더링
    renderer.render();

    // Qt 이벤트 루프
    return app.exec();
}
```

## 샘플 데이터

### cube.k
간단한 육면체 mesh (8 nodes, 1 hexahedron element)

```
*KEYWORD
*NODE
       1       0.0       0.0       0.0
       2       1.0       0.0       0.0
       3       1.0       1.0       0.0
       4       0.0       1.0       0.0
       5       0.0       0.0       1.0
       6       1.0       0.0       1.0
       7       1.0       1.0       1.0
       8       0.0       1.0       1.0
*ELEMENT_SOLID
       1       1       1       2       3       4       5       6       7       8
*END
```

### sphere.k
구 형태 mesh (~1,000 elements)

### car_chassis.k
자동차 섀시 mesh (~100,000 elements)

### beam.k
보 구조 mesh (~10,000 elements)

## 빌드 및 실행

```bash
# 전체 예제 빌드
cd examples
mkdir build && cd build
cmake ..
make

# 개별 예제 실행
./01_basic_loading/basic_loading
./02_element_selection/element_selection
./03_group_management/group_management
./04_visualization/visualization
```

## 추가 예제 (향후)

- **05_scripting**: Python 스크립팅
- **06_plugin_development**: 플러그인 개발
- **07_performance_optimization**: 성능 최적화 기법
- **08_custom_analysis**: 커스텀 분석 도구
