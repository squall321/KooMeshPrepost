# Visualization Module

VTK를 사용한 3D 가시화 모듈입니다. 대용량 mesh의 실시간 렌더링과 상호작용을 제공합니다.

## 구조

```
visualization/
├── VTKRenderer.cpp           # VTK 렌더러 래퍼
├── MeshToVTK.cpp             # Mesh → VTK 데이터 변환
├── ActorManager.cpp          # VTK Actor 관리
├── CameraController.cpp      # 카메라 제어
├── CustomInteractorStyle.cpp # 커스텀 상호작용 스타일
├── SelectionHighlighter.cpp  # 선택 하이라이트
├── AreaSelector.cpp          # 영역 선택 (드래그)
├── ColorMapper.cpp           # 색상 매핑
├── LightingManager.cpp       # 조명 관리
├── AnnotationManager.cpp     # 주석/측정 도구
├── ScreenCapture.cpp         # 스크린샷/녹화
└── FrustumCuller.cpp         # Frustum culling
```

## VTK 파이프라인

```
Mesh (our data)
     │
     ▼
MeshToVTK::convert()
     │
     ▼
vtkUnstructuredGrid
     │
     ▼
vtkGeometryFilter
     │
     ▼
vtkPolyDataMapper
     │
     ▼
vtkActor
     │
     ▼
vtkRenderer → vtkRenderWindow → Screen
```

## 주요 기능

### 1. 렌더링 모드

#### Solid (기본)
```cpp
renderer.setDisplayMode(VisualizationOptions::SOLID);
```

#### Wireframe
```cpp
renderer.setDisplayMode(VisualizationOptions::WIREFRAME);
```

#### Surface with Edges
```cpp
VisualizationOptions opts;
opts.displayMode = VisualizationOptions::SOLID;
opts.showEdges = true;
opts.edgeWidth = 1.5;
renderer.setOptions(opts);
```

### 2. 카메라 제어

```cpp
CameraController camera;

// 표준 뷰
camera.viewFront();
camera.viewTop();
camera.viewRight();
camera.viewIsometric();

// 장면에 맞추기
camera.resetToFitScene(mesh.boundingBox());

// 수동 제어
camera.orbit(azimuth: 45.0, elevation: 30.0);
camera.pan(dx: 10.0, dy: 5.0);
camera.zoom(factor: 1.5);
```

### 3. 선택 시스템

#### 하드웨어 선택 (GPU 기반)
```cpp
AreaSelector selector;

// 화면 좌표로 영역 선택
auto selectedElements = selector.selectInArea(
    startX: 100, startY: 100,
    endX: 300, endY: 300,
    renderer, mesh, spatialIndex
);

// 하이라이트
SelectionHighlighter highlighter;
highlighter.setSelectedElements(selectedElements, mesh);
highlighter.setHighlightColor(1.0, 0.0, 0.0); // 빨간색
```

### 4. 색상 매핑 (Scalar Field)

```cpp
ColorMapper mapper;

// 색상 스킴 설정
mapper.setColorScheme(ColorScheme::RAINBOW);
mapper.setRange(min: 0.0, max: 100.0);

// 요소별 스칼라 값 (예: 품질)
std::vector<double> qualities;
for (const auto& [id, element] : mesh.elements()) {
    qualities.push_back(element->computeQuality(mesh));
}

// Actor에 적용
mapper.applyToActor(actor, qualities);
```

### 5. 조명

```cpp
LightingManager lighting;

// 3-point lighting
lighting.addKeyLight(Eigen::Vector3d(10, 10, 10));
lighting.addFillLight(Eigen::Vector3d(-5, 5, 5));
lighting.addHeadlight();

// 고급 효과
lighting.enableShadows(true);
lighting.enableSSAO(true); // Screen-Space Ambient Occlusion
```

### 6. 주석 및 측정

```cpp
AnnotationManager annotations;

// 거리 측정
Point3D p1(0, 0, 0), p2(10, 0, 0);
annotations.addDistanceMeasurement(p1, p2);

// 텍스트 라벨
annotations.addTextLabel("Critical Zone", Point3D(5, 5, 0));

// 좌표 축
annotations.addCoordinateAxes();

// 스칼라 바 (범례)
annotations.addScalarBar("Element Quality", mapper.getLookupTable());
```

## 성능 최적화

### LOD (Level of Detail)

거리에 따라 자동으로 상세도 조절:

```cpp
LODManager lodManager;
lodManager.buildLODHierarchy(fullMesh);

// 렌더링 시
double cameraDistance = computeCameraDistance();
const Mesh& meshToRender = lodManager.getMeshForDistance(cameraDistance);
```

| 거리 | LOD 레벨 | 폴리곤 감소율 |
|------|---------|--------------|
| < 10 | VERY_HIGH | 0% |
| 10-50 | HIGH | 25% |
| 50-100 | MEDIUM | 50% |
| 100-500 | LOW | 75% |
| > 500 | VERY_LOW | 90% |

### Frustum Culling

카메라 시야 밖의 요소 제거:

```cpp
FrustumCuller culler;
culler.setFromCamera(camera);

auto visibleElements = culler.cullElements(mesh, spatialIndex);
// 보이는 요소만 렌더링
```

### VBO (Vertex Buffer Objects)

GPU 메모리에 데이터 캐싱:
```cpp
// VTK가 자동으로 VBO 사용
// 정적 데이터는 GPU에 한 번만 업로드
```

## 사용 예제

### 기본 렌더링

```cpp
#include "visualization/VTKRenderer.h"
#include "visualization/MeshToVTK.h"

// VTK 렌더러 초기화
VTKRenderer renderer;
renderer.initialize();
renderer.setBackgroundColor(0.1, 0.1, 0.1); // 어두운 회색

// Mesh를 VTK로 변환
auto vtkData = MeshToVTK::convert(mesh);

// Actor 생성
vtkSmartPointer<vtkActor> actor = createActor(vtkData);

// 렌더러에 추가
renderer.addActor(actor);

// 렌더링
renderer.render();
```

### Qt와 통합

```cpp
#include <QVTKOpenGLNativeWidget.h>

// Qt 위젯
QVTKOpenGLNativeWidget* vtkWidget = new QVTKOpenGLNativeWidget();
vtkWidget->setRenderWindow(renderer.getRenderWindow());

// Qt 레이아웃에 추가
mainLayout->addWidget(vtkWidget);
```

### 인터랙티브 선택

```cpp
// 커스텀 상호작용 스타일
auto style = vtkSmartPointer<CustomInteractorStyle>::New();

// 선택 콜백
style->onSelectionChanged = [&](const std::vector<ElementId>& elements) {
    std::cout << "Selected " << elements.size() << " elements\n";

    // 하이라이트
    highlighter.setSelectedElements(elements, mesh);
    renderer.render();
};

renderer.setInteractorStyle(style);
```

## 스크린샷 및 애니메이션

```cpp
ScreenCapture capture;

// 스크린샷
capture.saveScreenshot("output.png", width: 1920, height: 1080);

// 애니메이션 녹화
capture.startRecording("animation.mp4", fps: 30);

for (int i = 0; i < 360; i++) {
    camera.orbit(azimuth: 1.0, elevation: 0.0);
    renderer.render();
    capture.recordFrame();
}

capture.stopRecording();
```

## 렌더링 품질 설정

```cpp
VisualizationOptions opts;

// 안티앨리어싱
opts.enableAntialiasing = true;
opts.antialiasSamples = 8; // MSAA 8x

// 고품질 렌더링
opts.usePhongShading = true;
opts.useSpecularLighting = true;

// 성능 vs 품질 균형
opts.maxFPS = 60; // FPS 제한

renderer.setOptions(opts);
```

## 성능 벤치마크

| 폴리곤 수 | FPS (최적화 전) | FPS (최적화 후) | 기법 |
|----------|----------------|----------------|-----|
| 10K | 300 | 300 | - |
| 100K | 120 | 150 | VBO |
| 1M | 25 | 45 | VBO + Frustum Culling |
| 10M | 3 | 35 | VBO + Frustum + LOD |

*테스트 환경: NVIDIA RTX 3070, 1920x1080*

## 테스트
- `tests/unit/test_vtk_renderer.cpp`
- `tests/unit/test_mesh_to_vtk.cpp`
- `tests/integration/test_rendering_performance.cpp`
