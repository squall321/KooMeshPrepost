# 빌드 가이드

## 시스템 요구사항

### 필수
- CMake 3.20+
- C++17 지원 컴파일러:
  - GCC 7+
  - Clang 5+
  - MSVC 2019+
- Eigen3 3.3+

### 선택 (기능별)
- Qt 6.2+ (GUI)
- VTK 9.0+ (3D 가시화)
- Intel TBB (병렬 처리)
- Boost 1.70+ (파일 유틸리티)
- Google Test (테스트)

## 빌드 옵션

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_EXAMPLES=ON \
  -DENABLE_LTO=ON
```

### 사용 가능한 옵션

| 옵션 | 설명 | 기본값 |
|------|------|-------|
| BUILD_TESTS | 테스트 빌드 | ON |
| BUILD_EXAMPLES | 예제 빌드 | ON |
| BUILD_DOCS | Doxygen 문서 | OFF |
| ENABLE_SANITIZERS | 메모리 sanitizer | OFF |
| ENABLE_LTO | Link-Time Optimization | OFF |

## 빌드 절차

### Linux/macOS

```bash
# 1. 빌드 디렉토리 생성
mkdir build && cd build

# 2. CMake 설정
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. 빌드
cmake --build . -j$(nproc)

# 4. 테스트 (선택)
ctest --output-on-failure

# 5. 설치 (선택)
sudo cmake --install .
```

### Windows (Visual Studio)

```powershell
# 1. 빌드 디렉토리 생성
mkdir build
cd build

# 2. CMake 설정
cmake .. -G "Visual Studio 17 2022" -A x64

# 3. 빌드
cmake --build . --config Release

# 4. 테스트
ctest -C Release
```

## 의존성 설치

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    libeigen3-dev

# 선택적 의존성
sudo apt install -y \
    qt6-base-dev \
    libvtk9-dev \
    libtbb-dev \
    libboost-all-dev \
    libgtest-dev
```

### macOS (Homebrew)

```bash
brew install cmake eigen

# 선택적 의존성
brew install qt6 vtk tbb boost googletest
```

### Windows (vcpkg)

```powershell
vcpkg install eigen3:x64-windows

# 선택적 의존성
vcpkg install qt6:x64-windows vtk:x64-windows tbb:x64-windows boost:x64-windows gtest:x64-windows
```

## 트러블슈팅

### Qt6를 찾을 수 없음

```bash
cmake .. -DQt6_DIR=/path/to/Qt/6.x/gcc_64/lib/cmake/Qt6
```

### VTK를 찾을 수 없음

```bash
cmake .. -DVTK_DIR=/path/to/VTK/lib/cmake/vtk-9.x
```

### 컴파일러 지정

```bash
cmake .. -DCMAKE_CXX_COMPILER=g++-11
```
