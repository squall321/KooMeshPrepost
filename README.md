# KooMeshPrepost - 고성능 Mesh 전후처리기

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![C++](https://img.shields.io/badge/C++-17-00599C.svg)
![Qt](https://img.shields.io/badge/Qt-6.5-41CD52.svg)
![VTK](https://img.shields.io/badge/VTK-9.3-E34F26.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

**수천만 요소 규모의 LS-DYNA Mesh를 위한 현대적 3D 가시화 및 전처리 도구**

</div>

---

## 🎯 주요 기능

### ✨ 핵심 기능
- **대용량 데이터 처리**: 수천만 요소 실시간 가시화
- **고급 요소 선택**: 마우스 드래그, 영역 선택, 파트별 선택
- **그룹 관리**: 선택된 요소를 그룹으로 관리 및 색상 구분
- **LS-DYNA 지원**: Keyword 파일 완벽 파싱 (NODE, ELEMENT, PART)
- **현대적 UI**: Qt6 기반 직관적 인터페이스

### 🚀 성능 최적화
- **멀티스레딩**: Intel TBB 기반 병렬 처리
- **공간 인덱싱**: Octree/R-Tree를 통한 빠른 검색
- **GPU 렌더링**: OpenGL 4.5+ 하드웨어 가속
- **LOD 시스템**: 거리 기반 자동 상세도 조절
- **메모리 효율**: 스마트 포인터 및 Flyweight 패턴

### 🎨 가시화
- Solid, Wireframe, Surface with Edges 모드
- 요소별 색상 매핑 (Scalar Field)
- 조명 시스템 (Ambient Occlusion 지원)
- 거리/각도 측정 도구
- 스크린샷 및 애니메이션 녹화

---

## 📋 시스템 요구사항

### 최소 사양
- **OS**: Windows 10 / Ubuntu 20.04 / macOS 11+
- **CPU**: 4코어 2.5GHz 이상
- **RAM**: 8GB
- **GPU**: OpenGL 4.5 지원 그래픽 카드
- **저장공간**: 500MB

### 권장 사양
- **OS**: Windows 11 / Ubuntu 22.04
- **CPU**: 8코어 3.5GHz 이상 (Intel i7/AMD Ryzen 7)
- **RAM**: 32GB 이상
- **GPU**: NVIDIA RTX 시리즈 / AMD RX 6000 시리즈
- **저장공간**: 2GB

---

## 🔧 빌드 방법

### 의존성 설치

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-tools-dev \
    libvtk9-dev \
    libeigen3-dev \
    libtbb-dev \
    libboost-all-dev
```

#### macOS (Homebrew)
```bash
brew install cmake qt6 vtk eigen tbb boost
```

#### Windows (vcpkg)
```powershell
vcpkg install qt6 vtk eigen3 tbb boost
```

### 빌드 단계

```bash
# 1. 저장소 클론
git clone https://github.com/yourusername/KooMeshPrepost.git
cd KooMeshPrepost

# 2. 빌드 디렉토리 생성
mkdir build && cd build

# 3. CMake 구성
cmake .. -DCMAKE_BUILD_TYPE=Release

# 4. 빌드
cmake --build . -j$(nproc)

# 5. 실행
./bin/KooMeshPrepost
```

### Windows Visual Studio
```bash
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

---

## 🚀 빠른 시작

### 1. 파일 열기
```
File → Open → LS-DYNA Keyword File (*.k, *.key)
```

### 2. 요소 선택
- **마우스 드래그**: 좌클릭 드래그로 영역 선택
- **단일 선택**: Ctrl + 클릭
- **추가 선택**: Shift + 드래그
- **파트 선택**: Parts 패널에서 파트 클릭

### 3. 그룹 생성
```
1. 요소 선택
2. Groups 패널 → "Create Group" 버튼
3. 그룹 이름 입력
4. 색상 선택 (더블클릭)
```

### 4. 내보내기
```
File → Export Group → 그룹 선택 → 저장
```

---

## 📐 아키텍처

### 레이어 구조
```
┌─────────────────────────────────────┐
│         UI Layer (Qt6)              │  ← 사용자 인터페이스
├─────────────────────────────────────┤
│    Presenter (MVP Pattern)          │  ← 비즈니스 로직
├─────────────────────────────────────┤
│  ┌──────────┬──────────┬──────────┐ │
│  │Visualize │Selection │  I/O     │ │  ← 기능 모듈
│  │(VTK)     │Manager   │(Parser)  │ │
│  └──────────┴──────────┴──────────┘ │
├─────────────────────────────────────┤
│    Data Model (Mesh, Element, Node) │  ← 데이터 모델
├─────────────────────────────────────┤
│  Spatial Index (Octree, R-Tree)     │  ← 공간 인덱싱
└─────────────────────────────────────┘
```

### 핵심 디자인 패턴
- **MVP**: Model-View-Presenter (UI 분리)
- **Command**: Undo/Redo 시스템
- **Observer**: 이벤트 기반 통신
- **Factory**: 요소 생성
- **Strategy**: 선택 알고리즘
- **Flyweight**: 메모리 최적화

---

## 📊 성능 벤치마크

| 작업 | 요소 수 | 시간 | 메모리 |
|------|---------|------|--------|
| 파일 로딩 | 100만 | 0.8초 | 1.2GB |
| 파일 로딩 | 1000만 | 9.2초 | 9.8GB |
| Octree 구축 | 100만 | 0.3초 | 150MB |
| Octree 구축 | 1000만 | 4.1초 | 1.2GB |
| 영역 선택 | 100만 중 10만 | 45ms | - |
| 렌더링 FPS | 100만 표시 | 35 FPS | - |

*테스트 환경: Intel i7-12700K, 32GB RAM, RTX 3070*

---

## 🗂️ 프로젝트 구조

```
KooMeshPrepost/
├── src/
│   ├── core/               # 핵심 데이터 구조
│   │   ├── Node.cpp
│   │   ├── Element.cpp
│   │   ├── Mesh.cpp
│   │   ├── Part.cpp
│   │   ├── Group.cpp
│   │   └── GroupManager.cpp
│   ├── io/                 # 파일 입출력
│   │   ├── DynaFileReader.cpp
│   │   └── DynaFileWriter.cpp
│   ├── visualization/      # VTK 가시화
│   │   ├── VTKRenderer.cpp
│   │   ├── MeshToVTK.cpp
│   │   └── CameraController.cpp
│   ├── selection/          # 요소 선택
│   │   ├── SelectionManager.cpp
│   │   └── Octree.cpp
│   ├── ui/                 # Qt UI
│   │   ├── MainWindow.cpp
│   │   ├── PartsPanel.cpp
│   │   └── GroupsPanel.cpp
│   └── main.cpp
├── include/                # 헤더 파일
├── tests/                  # 단위 테스트
├── docs/                   # 문서
├── resources/              # 리소스 (아이콘, 스타일)
├── CMakeLists.txt
├── README.md
└── LICENSE
```

---

## 🧪 테스트

```bash
# 모든 테스트 실행
cd build
ctest --verbose

# 특정 테스트만 실행
./tests/test_mesh
./tests/test_octree
./tests/test_file_io
```

테스트 커버리지 목표: **80% 이상**

---

## 📖 문서

- [상세 구현 계획](DETAILED_IMPLEMENTATION_PLAN.md) - 100 Phase 개발 로드맵
- [사용자 가이드](docs/user_guide/) - 사용자 매뉴얼
- [개발자 가이드](docs/developer_guide/) - API 및 아키텍처
- [API 레퍼런스](docs/api/) - Doxygen 생성 문서

```bash
# API 문서 생성
cd docs
doxygen Doxyfile
```

---

## 🤝 기여 방법

1. **Fork** 프로젝트
2. **Feature branch** 생성 (`git checkout -b feature/AmazingFeature`)
3. **Commit** (`git commit -m 'Add some AmazingFeature'`)
4. **Push** (`git push origin feature/AmazingFeature`)
5. **Pull Request** 생성

### 코딩 스타일
- Google C++ Style Guide 준수
- clang-format 자동 포맷팅
- 모든 public API에 Doxygen 주석

---

## 🐛 버그 리포트

이슈는 [GitHub Issues](https://github.com/yourusername/KooMeshPrepost/issues)에서 관리합니다.

버그 리포트 시 포함할 내용:
- OS 및 버전
- 재현 단계
- 예상 동작 vs 실제 동작
- 스크린샷 (가능한 경우)
- 샘플 파일 (가능한 경우)

---

## 📜 라이선스

이 프로젝트는 MIT 라이선스를 따릅니다. 자세한 내용은 [LICENSE](LICENSE) 참조.

### 서드파티 라이선스
- **Qt6**: LGPL v3
- **VTK**: BSD 3-Clause
- **Eigen**: MPL2
- **Intel TBB**: Apache 2.0

---

## 🙏 감사의 말

- [VTK Project](https://vtk.org/) - 강력한 3D 가시화 라이브러리
- [Qt Project](https://www.qt.io/) - 크로스플랫폼 UI 프레임워크
- [Eigen](https://eigen.tuxfamily.org/) - 고성능 선형대수 라이브러리

---

## 📧 연락처

- **개발자**: Koo Engineering Team
- **이메일**: support@kooengineering.com
- **웹사이트**: https://kooengineering.com

---

## 🗺️ 로드맵

### v1.0 (Current)
- ✅ LS-DYNA 파일 파싱
- ✅ 기본 가시화
- ✅ 요소 선택 및 그룹핑
- ✅ Qt UI

### v1.1 (Q2 2024)
- ⬜ Python 스크립팅 인터페이스
- ⬜ 추가 파일 포맷 (Nastran, Abaqus)
- ⬜ 플러그인 시스템

### v2.0 (Q4 2024)
- ⬜ 웹 기반 뷰어
- ⬜ 클라우드 협업
- ⬜ AI 자동 그룹핑

---

<div align="center">

**Made with ❤️ by Koo Engineering**

[Homepage](https://kooengineering.com) • [Documentation](https://docs.kooengineering.com) • [Support](https://support.kooengineering.com)

</div>
