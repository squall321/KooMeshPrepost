# LS-DYNA Mesh 전후처리기 상세 구현 계획

## 🎯 프로젝트 개요

### 목표
- LS-DYNA 파일 기반 대용량 mesh 가시화 시스템 (수천만 요소)
- 고성능 3D 가시화 및 요소 선택/그룹핑 기능
- 현대적이고 직관적인 사용자 인터페이스

### 기술 스택

#### Core Technologies
- **Language**: C++17/20 (현대적 기능 활용)
- **Build System**: CMake 3.20+
- **3D Visualization**: VTK 9.3+ (대용량 mesh 처리에 최적화)
- **UI Framework**: Qt 6.5+ (현대적 UI/UX)
- **Graphics**: OpenGL 4.5+ (VTK 백엔드)

#### Libraries & Tools
- **VTK Modules**:
  - vtkRenderingCore (렌더링)
  - vtkInteractionStyle (사용자 입력)
  - vtkFiltersGeometry (geometry 처리)
  - vtkRenderingAnnotation (annotation)
  - vtkIOLegacy (파일 I/O)

- **Performance**:
  - Intel TBB (병렬 처리)
  - Eigen3 (수학 연산)
  - std::thread, std::async (멀티스레딩)

- **Spatial Indexing**:
  - VTK의 vtkKdTree, vtkOctreePointLocator
  - 커스텀 R-Tree (요소 공간 인덱싱)

- **File Parsing**:
  - Boost.Spirit (고성능 파싱)
  - 메모리 맵 파일 I/O (대용량 파일)

#### Design Patterns
1. **MVP (Model-View-Presenter)**: UI와 비즈니스 로직 분리
2. **Command Pattern**: Undo/Redo 시스템
3. **Observer Pattern**: 이벤트 기반 아키텍처
4. **Factory Pattern**: 요소 및 객체 생성
5. **Strategy Pattern**: 선택 알고리즘
6. **Singleton Pattern**: 리소스 관리자
7. **Flyweight Pattern**: 메모리 최적화
8. **Chain of Responsibility**: 이벤트 처리

---

## 📋 100-Phase 구현 계획

### **PHASE 1-10: 프로젝트 초기화 및 기반 구조**

#### Phase 1: 프로젝트 구조 생성
```
KooMeshPrepost/
├── src/
│   ├── core/           # 핵심 데이터 구조
│   ├── io/             # 파일 입출력
│   ├── visualization/  # VTK 래핑
│   ├── selection/      # 요소 선택
│   ├── ui/             # Qt UI
│   └── utils/          # 유틸리티
├── include/
├── tests/
├── resources/
├── docs/
└── CMakeLists.txt
```

#### Phase 2: CMake 빌드 시스템 설정
- 모던 CMake 사용 (target-based)
- VTK, Qt6, TBB, Eigen 자동 검색
- 컴파일러 옵션 최적화 (-O3, -march=native)

#### Phase 3: 코딩 표준 및 스타일 가이드 수립
- .clang-format 설정 (Google 또는 LLVM 스타일)
- .clang-tidy 설정 (정적 분석)
- 네이밍 컨벤션 문서화

#### Phase 4: 버전 관리 및 CI/CD 설정
- Git workflow 정의 (GitFlow)
- GitHub Actions / GitLab CI 설정
- 자동 테스트 및 빌드

#### Phase 5: 로깅 시스템 구현
```cpp
// utils/Logger.h
class Logger {
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };
    static void log(Level level, const std::string& message);
};
```

#### Phase 6: 설정 관리 시스템
```cpp
// core/ConfigManager.h - Singleton pattern
class ConfigManager {
public:
    static ConfigManager& getInstance();
    template<typename T> T get(const std::string& key);
    void load(const std::string& configFile);
};
```

#### Phase 7: 예외 처리 프레임워크
```cpp
// core/Exceptions.h
class MeshException : public std::runtime_error {};
class FileIOException : public MeshException {};
class VisualizationException : public MeshException {};
```

#### Phase 8: 기본 데이터 구조 정의
```cpp
// core/Types.h
using NodeId = uint64_t;
using ElementId = uint64_t;
using PartId = uint32_t;

struct Point3D {
    double x, y, z;
};

struct Node {
    NodeId id;
    Point3D coordinates;
};
```

#### Phase 9: 메모리 관리 전략 수립
- 커스텀 allocator 설계 (pool allocator)
- 스마트 포인터 사용 규칙
- 메모리 프로파일링 도구 통합

#### Phase 10: 단위 테스트 프레임워크 설정
- Google Test 통합
- 테스트 디렉토리 구조
- CI에서 자동 실행

---

### **PHASE 11-25: 데이터 모델 계층**

#### Phase 11: Node 클래스 구현
```cpp
// core/Node.h
class Node {
private:
    NodeId m_id;
    Eigen::Vector3d m_coords;
    std::vector<ElementId> m_connectedElements; // 역참조
public:
    // 생성자, getter, setter
    const Eigen::Vector3d& coordinates() const;
};
```

#### Phase 12: Element 추상 클래스 설계
```cpp
// core/Element.h - Strategy pattern
class Element {
protected:
    ElementId m_id;
    PartId m_partId;
    std::vector<NodeId> m_nodeIds;
public:
    virtual ~Element() = default;
    virtual ElementType type() const = 0;
    virtual size_t nodeCount() const = 0;
    virtual std::array<double, 6> boundingBox() const = 0;
};
```

#### Phase 13: 구체적 요소 타입 구현
```cpp
// core/Elements/SolidElement.h
class TetrahedronElement : public Element {
    size_t nodeCount() const override { return 4; }
};

class HexahedronElement : public Element {
    size_t nodeCount() const override { return 8; }
};

// Shell, Beam 등...
```

#### Phase 14: Element Factory 구현
```cpp
// core/ElementFactory.h - Factory pattern
class ElementFactory {
public:
    static std::unique_ptr<Element> create(
        ElementType type,
        ElementId id,
        const std::vector<NodeId>& nodes
    );
};
```

#### Phase 15: Part 클래스 구현
```cpp
// core/Part.h
class Part {
private:
    PartId m_id;
    std::string m_name;
    std::vector<ElementId> m_elements;
    MaterialProperties m_material;
public:
    void addElement(ElementId id);
    const std::vector<ElementId>& elements() const;
};
```

#### Phase 16: Mesh 컨테이너 클래스 (핵심)
```cpp
// core/Mesh.h
class Mesh {
private:
    // 효율적인 저장을 위한 컨테이너
    std::unordered_map<NodeId, Node> m_nodes;
    std::unordered_map<ElementId, std::unique_ptr<Element>> m_elements;
    std::unordered_map<PartId, Part> m_parts;

    // 공간 인덱싱
    std::unique_ptr<SpatialIndex> m_spatialIndex;

public:
    void addNode(const Node& node);
    void addElement(std::unique_ptr<Element> element);
    void buildSpatialIndex(); // Phase 40에서 구현

    // 조회
    const Node* getNode(NodeId id) const;
    const Element* getElement(ElementId id) const;

    // 통계
    size_t nodeCount() const;
    size_t elementCount() const;
    std::array<double, 6> boundingBox() const;
};
```

#### Phase 17: Group 클래스 (요소 그룹핑)
```cpp
// core/Group.h
class Group {
private:
    std::string m_name;
    std::unordered_set<ElementId> m_elements;
    std::unordered_set<NodeId> m_nodes;
    QColor m_color; // 시각화용
public:
    void addElement(ElementId id);
    void removeElement(ElementId id);
    bool contains(ElementId id) const;
    size_t size() const;
};
```

#### Phase 18: GroupManager 구현
```cpp
// core/GroupManager.h - Observer pattern
class GroupManager {
private:
    std::unordered_map<std::string, Group> m_groups;
    std::vector<IGroupObserver*> m_observers;
public:
    void createGroup(const std::string& name);
    void deleteGroup(const std::string& name);
    Group* getGroup(const std::string& name);
    void notifyObservers(const std::string& groupName);
};
```

#### Phase 19: 메모리 효율적인 컨테이너 최적화
- Node/Element 데이터를 연속 메모리에 저장 (cache-friendly)
- SOA (Structure of Arrays) vs AOS (Array of Structures) 벤치마크
- Flyweight pattern으로 중복 데이터 제거

#### Phase 20: BoundingBox 유틸리티
```cpp
// utils/BoundingBox.h
class BoundingBox {
    Eigen::Vector3d m_min, m_max;
public:
    void expand(const Point3D& point);
    bool contains(const Point3D& point) const;
    bool intersects(const BoundingBox& other) const;
    Eigen::Vector3d center() const;
};
```

#### Phase 21: Transform 시스템
```cpp
// core/Transform.h
class Transform {
    Eigen::Matrix4d m_matrix;
public:
    void translate(const Eigen::Vector3d& offset);
    void rotate(const Eigen::Vector3d& axis, double angle);
    void scale(double factor);
    Point3D apply(const Point3D& point) const;
};
```

#### Phase 22: 데이터 유효성 검증
```cpp
// core/MeshValidator.h
class MeshValidator {
public:
    struct ValidationResult {
        bool isValid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    ValidationResult validate(const Mesh& mesh);
    bool checkConnectivity();
    bool checkNodeReferences();
};
```

#### Phase 23: 메시 통계 및 품질 분석
```cpp
// core/MeshStatistics.h
struct MeshStatistics {
    size_t totalNodes;
    size_t totalElements;
    std::map<ElementType, size_t> elementTypeCounts;
    double minElementQuality;
    double avgElementQuality;
    BoundingBox globalBounds;
};
```

#### Phase 24: 데이터 모델 단위 테스트
- Node, Element, Part 생성 테스트
- Mesh 추가/삭제 테스트
- Group 관리 테스트

#### Phase 25: 데이터 모델 성능 벤치마크
- 백만 노드 생성 시간 측정
- 조회 성능 테스트
- 메모리 사용량 프로파일링

---

### **PHASE 26-40: 파일 I/O 계층**

#### Phase 26: LS-DYNA Keyword 파일 포맷 분석
- *NODE, *ELEMENT_SOLID, *ELEMENT_SHELL 등 키워드 정의
- *PART 섹션 파싱 규칙
- 파일 포맷 문서화

#### Phase 27: 파일 리더 인터페이스 설계
```cpp
// io/IFileReader.h - Strategy pattern
class IFileReader {
public:
    virtual ~IFileReader() = default;
    virtual bool read(const std::string& filepath, Mesh& mesh) = 0;
    virtual float progress() const = 0;
    virtual void cancel() = 0;
};
```

#### Phase 28: 메모리 맵 파일 I/O 구현
```cpp
// io/MemoryMappedFile.h
class MemoryMappedFile {
private:
    void* m_data;
    size_t m_size;
public:
    bool open(const std::string& filepath);
    const char* data() const;
    size_t size() const;
};
```

#### Phase 29: LS-DYNA 키워드 파서 (Boost.Spirit)
```cpp
// io/DynaKeywordParser.h
class DynaKeywordParser {
public:
    struct NodeData { NodeId id; double x, y, z; };
    struct ElementData { ElementId id; PartId pid; std::vector<NodeId> nodes; };

    std::vector<NodeData> parseNodes(const std::string& section);
    std::vector<ElementData> parseElements(const std::string& section);
    std::map<PartId, std::string> parseParts(const std::string& section);
};
```

#### Phase 30: DynaFileReader 구현 (메인 파서)
```cpp
// io/DynaFileReader.h
class DynaFileReader : public IFileReader {
private:
    std::atomic<float> m_progress;
    std::atomic<bool> m_cancelled;

    void parseNodesSection(const std::string& section, Mesh& mesh);
    void parseElementsSection(const std::string& section, Mesh& mesh);
    void parsePartsSection(const std::string& section, Mesh& mesh);

public:
    bool read(const std::string& filepath, Mesh& mesh) override;
};
```

#### Phase 31: 대용량 파일 청크 단위 파싱
- 파일을 청크로 나누어 처리 (예: 100MB 단위)
- 멀티스레드 파싱 (각 청크를 별도 스레드에서)
- 진행률 리포팅

#### Phase 32: 파일 파싱 오류 처리
- 손상된 데이터 감지
- 부분 로딩 지원 (오류 섹션 스킵)
- 상세 오류 메시지

#### Phase 33: 파일 라이터 인터페이스
```cpp
// io/IFileWriter.h
class IFileWriter {
public:
    virtual bool write(const std::string& filepath, const Mesh& mesh) = 0;
    virtual bool writeGroup(const std::string& filepath, const Group& group) = 0;
};
```

#### Phase 34: DynaFileWriter 구현
- Mesh를 LS-DYNA 포맷으로 출력
- 선택된 Group만 내보내기
- 포맷 옵션 (고정 소수점, 과학적 표기법)

#### Phase 35: 추가 포맷 지원 (확장성)
```cpp
// io/VTKFileReader.h - VTK 네이티브 포맷
// io/STLFileReader.h - STL 포맷
// io/NastranFileReader.h - Nastran BDF
```

#### Phase 36: 파일 포맷 자동 감지
```cpp
// io/FileFormatDetector.h
enum class FileFormat { DYNA, VTK, STL, NASTRAN, UNKNOWN };

class FileFormatDetector {
public:
    static FileFormat detect(const std::string& filepath);
};
```

#### Phase 37: FileReaderFactory
```cpp
// io/FileReaderFactory.h
class FileReaderFactory {
public:
    static std::unique_ptr<IFileReader> create(FileFormat format);
};
```

#### Phase 38: 비동기 파일 로딩
```cpp
// io/AsyncFileLoader.h
class AsyncFileLoader {
public:
    std::future<bool> loadAsync(
        const std::string& filepath,
        Mesh& mesh,
        std::function<void(float)> progressCallback
    );
};
```

#### Phase 39: 파일 I/O 단위 테스트
- 샘플 LS-DYNA 파일 파싱 테스트
- 대용량 파일 (100만+ 요소) 테스트
- 손상된 파일 처리 테스트

#### Phase 40: 파일 I/O 성능 최적화
- 파싱 속도 벤치마크
- 메모리 사용량 최적화
- 병렬 처리 효과 측정

---

### **PHASE 41-55: 공간 인덱싱 및 검색**

#### Phase 41: 공간 인덱스 인터페이스
```cpp
// core/ISpatialIndex.h
class ISpatialIndex {
public:
    virtual void build(const Mesh& mesh) = 0;
    virtual std::vector<ElementId> query(const BoundingBox& box) const = 0;
    virtual std::vector<ElementId> queryPoint(const Point3D& point) const = 0;
    virtual ElementId findNearest(const Point3D& point) const = 0;
};
```

#### Phase 42: Octree 구현
```cpp
// core/Octree.h
class Octree : public ISpatialIndex {
private:
    struct OctreeNode {
        BoundingBox bounds;
        std::vector<ElementId> elements;
        std::array<std::unique_ptr<OctreeNode>, 8> children;
        bool isLeaf;
    };

    std::unique_ptr<OctreeNode> m_root;
    size_t m_maxDepth = 10;
    size_t m_maxElementsPerNode = 100;

    void subdivide(OctreeNode* node, const Mesh& mesh, size_t depth);
    void queryRecursive(const OctreeNode* node, const BoundingBox& box,
                        std::vector<ElementId>& results) const;
public:
    void build(const Mesh& mesh) override;
    std::vector<ElementId> query(const BoundingBox& box) const override;
};
```

#### Phase 43: R-Tree 구현 (대안)
```cpp
// core/RTree.h
// Boost.Geometry의 R-Tree 활용 또는 커스텀 구현
class RTree : public ISpatialIndex {
    // R-Tree는 동적 삽입/삭제에 유리
};
```

#### Phase 44: K-d Tree (점 검색 최적화)
```cpp
// core/KdTree.h
class KdTree {
public:
    void build(const std::vector<Node>& nodes);
    NodeId findNearest(const Point3D& point) const;
    std::vector<NodeId> findWithinRadius(const Point3D& point, double radius) const;
};
```

#### Phase 45: Uniform Grid (대용량 데이터용)
```cpp
// core/UniformGrid.h
class UniformGrid : public ISpatialIndex {
private:
    std::vector<std::vector<ElementId>> m_cells;
    Eigen::Vector3i m_gridSize;
    BoundingBox m_bounds;

    Eigen::Vector3i pointToCell(const Point3D& point) const;
public:
    void build(const Mesh& mesh) override;
    // 매우 빠른 조회, 메모리는 많이 사용
};
```

#### Phase 46: Spatial Index Factory
```cpp
// core/SpatialIndexFactory.h
enum class SpatialIndexType { OCTREE, RTREE, KDTREE, UNIFORM_GRID };

class SpatialIndexFactory {
public:
    static std::unique_ptr<ISpatialIndex> create(SpatialIndexType type);
    static SpatialIndexType selectOptimal(size_t elementCount);
};
```

#### Phase 47: Level of Detail (LOD) 시스템
```cpp
// core/LODManager.h
class LODManager {
public:
    enum LODLevel { VERY_LOW, LOW, MEDIUM, HIGH, VERY_HIGH };

    struct LODMesh {
        Mesh fullMesh;
        std::map<LODLevel, Mesh> simplified;
    };

    void buildLODHierarchy(const Mesh& fullMesh);
    const Mesh& getMeshForDistance(double cameraDistance) const;
};
```

#### Phase 48: Mesh 간소화 알고리즘
```cpp
// core/MeshSimplifier.h
class MeshSimplifier {
public:
    // Quadric Error Metrics 기반 간소화
    Mesh simplify(const Mesh& input, double targetReduction);

    // Edge collapse
    // Vertex clustering
};
```

#### Phase 49: Frustum Culling
```cpp
// visualization/FrustumCuller.h
class FrustumCuller {
private:
    std::array<Eigen::Vector4d, 6> m_planes; // 6개 평면
public:
    void setFromCamera(const Camera& camera);
    bool isVisible(const BoundingBox& box) const;
    std::vector<ElementId> cullElements(const Mesh& mesh,
                                        const ISpatialIndex& index) const;
};
```

#### Phase 50: 공간 쿼리 최적화
- 범위 검색 (Range query)
- k-최근접 이웃 (k-NN)
- 반경 검색 (Radius search)

#### Phase 51: 증분 인덱스 업데이트
```cpp
// 요소 추가/삭제 시 전체 재구축 없이 증분 업데이트
void Octree::insertElement(ElementId id, const BoundingBox& bounds);
void Octree::removeElement(ElementId id);
```

#### Phase 52: 병렬 인덱스 구축
- TBB parallel_for를 사용한 병렬 Octree 구축
- 멀티스레드 안전성 보장

#### Phase 53: 인덱스 직렬화 (캐싱)
```cpp
// core/SpatialIndexSerializer.h
class SpatialIndexSerializer {
public:
    void save(const ISpatialIndex& index, const std::string& filepath);
    std::unique_ptr<ISpatialIndex> load(const std::string& filepath);
};
```

#### Phase 54: 공간 인덱스 단위 테스트
- Octree 구축 및 쿼리 정확성 테스트
- 경계 케이스 (빈 mesh, 단일 요소 등)

#### Phase 55: 공간 인덱스 성능 벤치마크
- 백만 요소 Octree 구축 시간
- 쿼리 성능 (10만 쿼리/초 목표)
- 메모리 오버헤드 측정

---

### **PHASE 56-70: VTK 가시화 계층**

#### Phase 56: VTK 초기화 및 렌더링 파이프라인
```cpp
// visualization/VTKRenderer.h
class VTKRenderer {
private:
    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkRenderWindow> m_renderWindow;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;

public:
    void initialize();
    void setBackgroundColor(double r, double g, double b);
    void render();
};
```

#### Phase 57: Mesh를 VTK 데이터로 변환
```cpp
// visualization/MeshToVTK.h
class MeshToVTK {
public:
    static vtkSmartPointer<vtkUnstructuredGrid> convert(const Mesh& mesh);
    static vtkSmartPointer<vtkPolyData> convertToPolyData(const Mesh& mesh);

private:
    static void addNodes(vtkUnstructuredGrid* grid, const Mesh& mesh);
    static void addElements(vtkUnstructuredGrid* grid, const Mesh& mesh);
};
```

#### Phase 58: VTK Actor 관리
```cpp
// visualization/ActorManager.h
class ActorManager {
private:
    std::unordered_map<std::string, vtkSmartPointer<vtkActor>> m_actors;

public:
    void addActor(const std::string& name, vtkActor* actor);
    void removeActor(const std::string& name);
    vtkActor* getActor(const std::string& name);
    void setVisibility(const std::string& name, bool visible);
    void setColor(const std::string& name, double r, double g, double b);
    void setOpacity(const std::string& name, double opacity);
};
```

#### Phase 59: 카메라 컨트롤러
```cpp
// visualization/CameraController.h
class CameraController {
private:
    vtkSmartPointer<vtkCamera> m_camera;

public:
    void setPosition(const Eigen::Vector3d& pos);
    void setFocalPoint(const Eigen::Vector3d& point);
    void setViewAngle(double angle);

    void resetToFitScene(const BoundingBox& sceneBounds);
    void orbit(double azimuth, double elevation);
    void pan(double dx, double dy);
    void zoom(double factor);

    // 표준 뷰
    void viewFront();
    void viewTop();
    void viewRight();
    void viewIsometric();
};
```

#### Phase 60: 인터랙션 스타일 (커스텀)
```cpp
// visualization/CustomInteractorStyle.h
class CustomInteractorStyle : public vtkInteractorStyleTrackballCamera {
private:
    bool m_selectionMode = false;

public:
    static CustomInteractorStyle* New();

    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMouseMove() override;
    void OnRightButtonDown() override;

    void setSelectionMode(bool enabled);

    // 시그널/콜백
    std::function<void(const std::vector<ElementId>&)> onSelectionChanged;
};
```

#### Phase 61: 선택 하이라이트 시스템
```cpp
// visualization/SelectionHighlighter.h
class SelectionHighlighter {
private:
    vtkSmartPointer<vtkActor> m_highlightActor;
    std::vector<ElementId> m_selectedElements;

public:
    void setSelectedElements(const std::vector<ElementId>& elements,
                             const Mesh& mesh);
    void clearSelection();
    void setHighlightColor(double r, double g, double b);
    vtkActor* getHighlightActor();
};
```

#### Phase 62: 2D 사각형 선택 (Area Picker)
```cpp
// visualization/AreaSelector.h
class AreaSelector {
private:
    vtkSmartPointer<vtkAreaPicker> m_areaPicker;
    vtkSmartPointer<vtkHardwareSelector> m_hardwareSelector;

public:
    std::vector<ElementId> selectInArea(
        int startX, int startY, int endX, int endY,
        vtkRenderer* renderer,
        const Mesh& mesh,
        const ISpatialIndex& spatialIndex
    );

    void setSelectionMode(SelectionMode mode); // ADD, SUBTRACT, INTERSECT
};
```

#### Phase 63: 하드웨어 가속 선택 (GPU)
```cpp
// VTK의 vtkHardwareSelector 활용
// GPU에서 요소 ID를 색상으로 렌더링하여 선택
```

#### Phase 64: 가시화 옵션
```cpp
// visualization/VisualizationOptions.h
struct VisualizationOptions {
    enum DisplayMode { SOLID, WIREFRAME, SURFACE_WITH_EDGES, POINTS };

    DisplayMode displayMode = SOLID;
    bool showEdges = true;
    double edgeWidth = 1.0;
    bool showNodes = false;
    double nodeSize = 3.0;

    bool useScalarColoring = false;
    std::string scalarFieldName;

    bool useLighting = true;
    bool useAmbientOcclusion = false;
};
```

#### Phase 65: 색상 매핑 (Scalar Field)
```cpp
// visualization/ColorMapper.h
class ColorMapper {
private:
    vtkSmartPointer<vtkLookupTable> m_lookupTable;

public:
    void setColorScheme(ColorScheme scheme); // RAINBOW, GRAYSCALE, HEAT, etc.
    void setRange(double min, double max);
    vtkLookupTable* getLookupTable();

    void applyToActor(vtkActor* actor, const std::vector<double>& scalarValues);
};
```

#### Phase 66: 조명 시스템
```cpp
// visualization/LightingManager.h
class LightingManager {
private:
    std::vector<vtkSmartPointer<vtkLight>> m_lights;

public:
    void addHeadlight();
    void addKeyLight(const Eigen::Vector3d& position);
    void addFillLight(const Eigen::Vector3d& position);
    void setAmbientLight(double intensity);

    void enableShadows(bool enabled);
    void enableSSAO(bool enabled); // Screen-Space Ambient Occlusion
};
```

#### Phase 67: 애너테이션 (측정, 라벨)
```cpp
// visualization/AnnotationManager.h
class AnnotationManager {
public:
    void addDistanceMeasurement(const Point3D& p1, const Point3D& p2);
    void addAngleMeasurement(const Point3D& p1, const Point3D& p2, const Point3D& p3);
    void addTextLabel(const std::string& text, const Point3D& position);
    void addCoordinateAxes();
    void addScalarBar(const std::string& title, vtkLookupTable* lut);

    void clear();
};
```

#### Phase 68: 스크린샷 및 애니메이션
```cpp
// visualization/ScreenCapture.h
class ScreenCapture {
public:
    void saveScreenshot(const std::string& filepath, int width, int height);
    void startRecording(const std::string& filepath, int fps);
    void recordFrame();
    void stopRecording();
};
```

#### Phase 69: VTK 메모리 최적화
- LOD Actor 사용
- 대용량 데이터를 위한 스트리밍
- GPU 메모리 관리

#### Phase 70: 가시화 단위 테스트
- VTK 파이프라인 테스트
- 선택 정확성 테스트
- 렌더링 성능 테스트

---

### **PHASE 71-80: Qt UI 계층**

#### Phase 71: Qt 메인 윈도우 구조
```cpp
// ui/MainWindow.h
class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    QVTKOpenGLNativeWidget* m_vtkWidget;
    QDockWidget* m_partsDock;
    QDockWidget* m_groupsDock;
    QDockWidget* m_propertiesDock;

    QMenuBar* m_menuBar;
    QToolBar* m_mainToolBar;
    QStatusBar* m_statusBar;

public:
    MainWindow(QWidget* parent = nullptr);
    void setupUI();

private slots:
    void onFileOpen();
    void onFileSave();
    void onViewReset();
};
```

#### Phase 72: QVTKOpenGLNativeWidget 통합
```cpp
// Qt와 VTK 연결
m_vtkWidget = new QVTKOpenGLNativeWidget(this);
m_vtkWidget->setRenderWindow(renderer->getRenderWindow());
setCentralWidget(m_vtkWidget);
```

#### Phase 73: 메뉴 바 및 툴바
```cpp
// ui/MenuBarBuilder.h
class MenuBarBuilder {
public:
    static QMenuBar* build(MainWindow* parent);

private:
    static QMenu* createFileMenu(MainWindow* parent);
    static QMenu* createEditMenu(MainWindow* parent);
    static QMenu* createViewMenu(MainWindow* parent);
    static QMenu* createToolsMenu(MainWindow* parent);
    static QMenu* createHelpMenu(MainWindow* parent);
};
```

#### Phase 74: Parts 패널 (트리 뷰)
```cpp
// ui/PartsPanel.h
class PartsPanel : public QWidget {
    Q_OBJECT

private:
    QTreeView* m_treeView;
    QStandardItemModel* m_model;

public:
    void setMesh(const Mesh* mesh);
    void updatePartsList(const std::vector<Part>& parts);

signals:
    void partSelected(PartId id);
    void partVisibilityChanged(PartId id, bool visible);

private slots:
    void onItemClicked(const QModelIndex& index);
    void onContextMenu(const QPoint& pos);
};
```

#### Phase 75: Groups 패널
```cpp
// ui/GroupsPanel.h
class GroupsPanel : public QWidget {
    Q_OBJECT

private:
    QListWidget* m_groupsList;
    QPushButton* m_createGroupBtn;
    QPushButton* m_deleteGroupBtn;
    QColorDialog* m_colorPicker;

public:
    void setGroupManager(GroupManager* manager);
    void refreshGroupsList();

signals:
    void groupSelected(const std::string& name);
    void createGroupRequested(const std::string& name);
    void deleteGroupRequested(const std::string& name);
    void groupColorChanged(const std::string& name, const QColor& color);

private slots:
    void onCreateGroup();
    void onDeleteGroup();
    void onGroupDoubleClicked(QListWidgetItem* item);
};
```

#### Phase 76: Properties 패널
```cpp
// ui/PropertiesPanel.h
class PropertiesPanel : public QWidget {
    Q_OBJECT

private:
    QTableWidget* m_propertiesTable;

public:
    void displayElementProperties(ElementId id, const Element* element);
    void displayNodeProperties(NodeId id, const Node* node);
    void displayGroupProperties(const Group* group);
    void clear();

private:
    void addProperty(const QString& name, const QString& value);
};
```

#### Phase 77: 선택 툴바
```cpp
// ui/SelectionToolBar.h
class SelectionToolBar : public QToolBar {
    Q_OBJECT

private:
    QAction* m_singleSelectAction;
    QAction* m_areaSelectAction;
    QAction* m_lassoSelectAction;
    QAction* m_selectByPartAction;

    QComboBox* m_selectionModeCombo; // ADD, SUBTRACT, INTERSECT

signals:
    void selectionToolChanged(SelectionTool tool);
    void selectionModeChanged(SelectionMode mode);

public:
    SelectionToolBar(QWidget* parent = nullptr);
};
```

#### Phase 78: 진행률 대화상자
```cpp
// ui/ProgressDialog.h
class ProgressDialog : public QDialog {
    Q_OBJECT

private:
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_cancelButton;

public:
    void setProgress(float progress); // 0.0 ~ 1.0
    void setStatus(const QString& status);
    bool isCancelled() const;

signals:
    void cancelled();
};
```

#### Phase 79: 설정 대화상자
```cpp
// ui/SettingsDialog.h
class SettingsDialog : public QDialog {
    Q_OBJECT

private:
    QTabWidget* m_tabs;

    // Visualization 탭
    QWidget* createVisualizationTab();

    // Performance 탭
    QWidget* createPerformanceTab();

    // File I/O 탭
    QWidget* createFileIOTab();

public:
    SettingsDialog(QWidget* parent = nullptr);
    void loadSettings();
    void saveSettings();
};
```

#### Phase 80: Qt UI 스타일링
```cpp
// resources/styles.qss (Qt Style Sheet)
// 현대적인 다크/라이트 테마
// Material Design 또는 Fluent Design 스타일
```

---

### **PHASE 81-90: 통합 및 상호작용**

#### Phase 81: MVP (Model-View-Presenter) 패턴 구현
```cpp
// core/Presenter.h
class Presenter {
private:
    Mesh* m_model;
    MainWindow* m_view;
    VTKRenderer* m_renderer;
    GroupManager* m_groupManager;

public:
    Presenter(Mesh* model, MainWindow* view, VTKRenderer* renderer);

    void onFileOpen(const QString& filepath);
    void onSelectionChanged(const std::vector<ElementId>& elements);
    void onCreateGroup(const QString& name);
    void onPartVisibilityChanged(PartId id, bool visible);

private:
    void updateView();
};
```

#### Phase 82: Command Pattern (Undo/Redo)
```cpp
// core/Command.h
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual QString description() const = 0;
};

// core/CommandManager.h
class CommandManager {
private:
    std::vector<std::unique_ptr<ICommand>> m_undoStack;
    std::vector<std::unique_ptr<ICommand>> m_redoStack;

public:
    void executeCommand(std::unique_ptr<ICommand> command);
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    QString undoDescription() const;
    QString redoDescription() const;
};
```

#### Phase 83: 구체적인 Command 구현
```cpp
// core/commands/CreateGroupCommand.h
class CreateGroupCommand : public ICommand {
private:
    GroupManager* m_groupManager;
    std::string m_groupName;
    std::vector<ElementId> m_elements;

public:
    void execute() override;
    void undo() override;
};

// core/commands/DeleteGroupCommand.h
// core/commands/AddToGroupCommand.h
// core/commands/RemoveFromGroupCommand.h
```

#### Phase 84: 이벤트 시스템 (Observer Pattern)
```cpp
// core/EventSystem.h
enum class EventType {
    MESH_LOADED,
    SELECTION_CHANGED,
    GROUP_CREATED,
    GROUP_DELETED,
    GROUP_MODIFIED,
    VISUALIZATION_CHANGED
};

class IEventObserver {
public:
    virtual void onEvent(EventType type, void* data) = 0;
};

class EventSystem {
private:
    std::unordered_map<EventType, std::vector<IEventObserver*>> m_observers;

public:
    void subscribe(EventType type, IEventObserver* observer);
    void unsubscribe(EventType type, IEventObserver* observer);
    void notify(EventType type, void* data = nullptr);
};
```

#### Phase 85: 비동기 작업 관리
```cpp
// core/TaskManager.h
class TaskManager {
private:
    std::vector<std::future<void>> m_activeTasks;

public:
    template<typename Func>
    void runAsync(Func&& func, std::function<void()> onComplete = nullptr);

    void cancelAll();
    void waitForAll();
    size_t activeTaskCount() const;
};
```

#### Phase 86: 선택 로직 통합
```cpp
// selection/SelectionManager.h
class SelectionManager {
private:
    std::vector<ElementId> m_selectedElements;
    std::vector<NodeId> m_selectedNodes;
    SelectionMode m_mode = SelectionMode::REPLACE;

public:
    void setMode(SelectionMode mode);

    void selectElements(const std::vector<ElementId>& elements);
    void selectElementsInArea(const BoundingBox& area, const Mesh& mesh,
                              const ISpatialIndex& index);
    void selectByPart(PartId partId, const Mesh& mesh);

    void clearSelection();
    const std::vector<ElementId>& getSelectedElements() const;

    // Observer pattern
    std::function<void(const std::vector<ElementId>&)> onSelectionChanged;
};
```

#### Phase 87: 드래그 선택 구현 (마우스 상호작용)
```cpp
// visualization/DragSelector.h
class DragSelector {
private:
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_isDragging = false;

    QRubberBand* m_rubberBand; // 시각적 피드백

public:
    void startDrag(const QPoint& point);
    void updateDrag(const QPoint& point);
    void endDrag(const QPoint& point);

    QRect getSelectionRect() const;
    bool isDragging() const;
};
```

#### Phase 88: 키보드 단축키
```cpp
// ui/KeyboardShortcuts.h
class KeyboardShortcuts {
public:
    static void setup(MainWindow* mainWindow, Presenter* presenter);

    // Ctrl+O: Open
    // Ctrl+S: Save
    // Ctrl+Z: Undo
    // Ctrl+Y: Redo
    // Delete: Delete selected
    // Ctrl+A: Select all
    // Ctrl+G: Create group
    // F5: Refresh view
    // Escape: Clear selection
};
```

#### Phase 89: 컨텍스트 메뉴 (우클릭)
```cpp
// ui/ContextMenuManager.h
class ContextMenuManager {
public:
    QMenu* createElementContextMenu(ElementId id, const Mesh& mesh);
    QMenu* createPartContextMenu(PartId id);
    QMenu* createGroupContextMenu(const std::string& groupName);
    QMenu* createViewportContextMenu();

private:
    QAction* createAction(const QString& text, std::function<void()> callback);
};
```

#### Phase 90: 통합 테스트
- 전체 워크플로우 테스트 (파일 열기 → 선택 → 그룹 생성 → 저장)
- UI와 백엔드 상호작용 테스트
- Undo/Redo 테스트

---

### **PHASE 91-100: 최적화, 배포 및 고급 기능**

#### Phase 91: 멀티스레딩 최적화
```cpp
// 파일 로딩: 별도 스레드
// 공간 인덱스 구축: TBB 병렬화
// VTK 렌더링: 메인 스레드 (OpenGL은 싱글 스레드)
// 선택 연산: 백그라운드 스레드

// core/ThreadPool.h
class ThreadPool {
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop = false;

public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<typename Func>
    auto enqueue(Func&& func) -> std::future<decltype(func())>;
};
```

#### Phase 92: 메모리 사용량 프로파일링
- Valgrind Massif로 메모리 프로파일
- 불필요한 복사 제거 (move semantics)
- 스마트 포인터 최적화

#### Phase 93: 렌더링 성능 최적화
- VBO (Vertex Buffer Objects) 최적화
- Frustum culling 활성화
- LOD 자동 전환
- 백페이스 컬링

#### Phase 94: 대용량 데이터 스트리밍
```cpp
// io/StreamingFileReader.h
// 파일 전체를 메모리에 로드하지 않고 필요한 부분만 스트리밍
class StreamingFileReader {
public:
    void openStream(const std::string& filepath);
    void loadChunk(size_t offset, size_t size);
    void unloadChunk(size_t offset);
};
```

#### Phase 95: GPU 가속 연산
```cpp
// 가능한 경우 CUDA/OpenCL로 연산 가속
// - 요소 품질 계산
// - 거리 계산
// - 공간 쿼리
```

#### Phase 96: 플러그인 시스템
```cpp
// core/PluginInterface.h
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
    virtual void initialize(Mesh* mesh, MainWindow* ui) = 0;
    virtual void shutdown() = 0;
};

// core/PluginManager.h
class PluginManager {
public:
    void loadPlugin(const std::string& filepath);
    void unloadPlugin(const std::string& name);
    std::vector<IPlugin*> loadedPlugins() const;
};
```

#### Phase 97: 패키징 및 배포
```cmake
# CPack 설정
# Windows: NSIS 인스톨러
# Linux: DEB/RPM 패키지
# macOS: DMG 이미지

set(CPACK_PACKAGE_NAME "KooMeshPrepost")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Your Company")
# ...
```

#### Phase 98: 문서화
```
docs/
├── user_guide/
│   ├── installation.md
│   ├── quickstart.md
│   ├── file_formats.md
│   ├── selection.md
│   └── grouping.md
├── developer_guide/
│   ├── architecture.md
│   ├── api_reference.md
│   ├── plugin_development.md
│   └── contributing.md
└── Doxygen 설정
```

#### Phase 99: 사용자 피드백 및 버그 수정
- 베타 테스트
- 이슈 트래킹 (GitHub Issues)
- 성능 벤치마크 공개

#### Phase 100: 최종 릴리스 준비
- 최종 코드 리뷰
- 라이선스 확인 (VTK, Qt 라이선스 호환성)
- 배포 파일 생성
- 릴리스 노트 작성

---

## 🔧 핵심 클래스 다이어그램

```
┌─────────────────┐
│  MainWindow     │ ◄──────── Qt UI Layer
│  (Qt)           │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Presenter      │ ◄──────── MVP Pattern
└────────┬────────┘
         │
         ├──────► ┌─────────────┐
         │        │  Mesh       │ ◄──── Data Model
         │        └──────┬──────┘
         │               │
         │               ├──► Node
         │               ├──► Element (abstract)
         │               │    ├── TetrahedronElement
         │               │    ├── HexahedronElement
         │               │    └── ...
         │               └──► Part
         │
         ├──────► ┌──────────────────┐
         │        │  VTKRenderer     │ ◄─── Visualization
         │        └──────────────────┘
         │
         ├──────► ┌──────────────────┐
         │        │  GroupManager    │ ◄─── Grouping
         │        └──────────────────┘
         │
         ├──────► ┌──────────────────┐
         │        │  SelectionMgr    │ ◄─── Selection
         │        └──────────────────┘
         │
         └──────► ┌──────────────────┐
                  │  CommandManager  │ ◄─── Undo/Redo
                  └──────────────────┘
```

---

## 📊 성능 목표

| 항목 | 목표 | 비고 |
|-----|------|-----|
| 파일 로딩 | 100만 요소/초 | 멀티스레드 파싱 |
| 메모리 사용량 | < 10GB | 1000만 요소 기준 |
| 렌더링 FPS | > 30 FPS | 100만 폴리곤 화면에 표시 시 |
| 선택 응답 시간 | < 100ms | 100만 요소 공간 쿼리 |
| Octree 구축 | < 5초 | 1000만 요소 기준 |

---

## 🎨 UI 목업 (텍스트 기반)

```
┌─────────────────────────────────────────────────────────────┐
│ File  Edit  View  Tools  Help                   [_] [□] [X] │
├─────────────────────────────────────────────────────────────┤
│ [📁] [💾] [↶] [↷] │ [▶] [☐] [◯] │ [🔍+] [🔍-] [🏠]        │
├──────┬──────────────────────────────────────────────┬───────┤
│Parts │                                              │ Props │
│------│            3D Viewport                       │-------│
│☑ P1  │                                              │ Elem  │
│☑ P2  │                                              │ ID: ..│
│☐ P3  │                                              │ Type: │
│      │                                              │ Part: │
├──────┤                                              ├───────┤
│Groups│                                              │       │
│------│                                              │       │
│● Gr1 │                                              │       │
│● Gr2 │                                              │       │
│[New] │                                              │       │
└──────┴──────────────────────────────────────────────┴───────┘
│ Ready │ Elements: 1,234,567 │ Selected: 42         │ 60 FPS│
└─────────────────────────────────────────────────────────────┘
```

---

## 🚀 개발 우선순위

### Phase 1 (MVP - Minimum Viable Product)
1-25: 데이터 모델
26-40: 파일 I/O
56-65: 기본 가시화
71-75: 기본 UI
86-87: 기본 선택

### Phase 2 (Core Features)
41-55: 공간 인덱싱
66-70: 고급 가시화
76-80: 고급 UI
81-90: 통합 및 상호작용

### Phase 3 (Optimization & Release)
91-100: 최적화 및 배포

---

## 📚 참고 자료

- **VTK Documentation**: https://vtk.org/documentation/
- **Qt Documentation**: https://doc.qt.io/
- **LS-DYNA Keyword Manual**: https://www.dynasupport.com/
- **Boost.Spirit**: https://www.boost.org/doc/libs/release/libs/spirit/
- **Intel TBB**: https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html
- **Design Patterns**: "Design Patterns" by Gang of Four

---

## 💡 추가 제안

### 고급 기능 (Phase 100+)
- Python 스크립팅 인터페이스 (pybind11)
- 웹 기반 뷰어 (WebAssembly + VTK.js)
- 클라우드 협업 기능
- AI 기반 자동 그룹핑
- 실시간 시뮬레이션 결과 가시화

### 품질 관리
- 코드 커버리지 > 80%
- 정적 분석 (clang-tidy, cppcheck)
- 동적 분석 (Valgrind, AddressSanitizer)
- 지속적 통합 (CI/CD)

---

**이 계획은 대규모 mesh 전후처리기를 단계별로 구축하기 위한 로드맵입니다. 각 phase는 독립적으로 개발 및 테스트 가능하며, 점진적으로 기능을 추가할 수 있습니다.**
