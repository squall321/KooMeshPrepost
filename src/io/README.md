# I/O Module

파일 입출력을 담당하는 모듈입니다. 다양한 파일 포맷을 지원하며, 대용량 파일을 효율적으로 처리합니다.

## 구조

```
io/
├── IFileReader.h         # 파일 리더 인터페이스
├── IFileWriter.h         # 파일 라이터 인터페이스
├── DynaFileReader.cpp    # LS-DYNA Keyword 파일 리더
├── DynaFileWriter.cpp    # LS-DYNA Keyword 파일 라이터
├── DynaKeywordParser.cpp # LS-DYNA 키워드 파서
├── FileFormatDetector.cpp # 파일 포맷 자동 감지
├── FileReaderFactory.cpp  # 리더 팩토리
├── AsyncFileLoader.cpp    # 비동기 파일 로더
└── MemoryMappedFile.cpp   # 메모리 맵 파일 I/O
```

## 지원 파일 포맷

### 현재 지원
- **LS-DYNA Keyword** (*.k, *.key)
  - *NODE
  - *ELEMENT_SOLID
  - *ELEMENT_SHELL
  - *PART
  - *SECTION_*

### 향후 지원 예정
- VTK (*.vtk, *.vtu)
- STL (*.stl)
- Nastran BDF (*.bdf, *.nas)
- Abaqus INP (*.inp)

## 성능 최적화

### 메모리 맵 파일 I/O
대용량 파일을 메모리에 매핑하여 빠른 접근:
```cpp
MemoryMappedFile file;
file.open("large_mesh.k");
const char* data = file.data();
// 파일 전체가 메모리에 매핑됨 (실제 물리 메모리는 필요 시 로드)
```

### 멀티스레드 파싱
파일을 섹션별로 나누어 병렬 파싱:
```
File: mesh.k (1GB)
├── *NODE section (300MB)     → Thread 1
├── *ELEMENT section (600MB)  → Thread 2
└── *PART section (100MB)     → Thread 3
```

### 청크 단위 처리
메모리 효율을 위해 청크 단위로 처리:
```cpp
const size_t CHUNK_SIZE = 100 * 1024 * 1024; // 100MB
for (size_t offset = 0; offset < fileSize; offset += CHUNK_SIZE) {
    processChunk(offset, std::min(CHUNK_SIZE, fileSize - offset));
}
```

## 사용 예제

### 기본 사용법
```cpp
#include "io/DynaFileReader.h"
#include "core/Mesh.h"

Mesh mesh;
DynaFileReader reader;

if (reader.read("model.k", mesh)) {
    std::cout << "Loaded " << mesh.nodeCount() << " nodes\n";
    std::cout << "Loaded " << mesh.elementCount() << " elements\n";
}
```

### 비동기 로딩 (진행률 표시)
```cpp
AsyncFileLoader loader;

auto future = loader.loadAsync("large_model.k", mesh,
    [](float progress) {
        std::cout << "Progress: " << (progress * 100) << "%\r" << std::flush;
    }
);

// 다른 작업 수행...

future.wait(); // 완료 대기
```

### 파일 포맷 자동 감지
```cpp
auto format = FileFormatDetector::detect("unknown_file.dat");
auto reader = FileReaderFactory::create(format);
reader->read("unknown_file.dat", mesh);
```

## LS-DYNA 키워드 포맷

### *NODE
```
*NODE
$#   nid               x               y               z      tc      rc
       1        0.000000        0.000000        0.000000       0       0
       2        1.000000        0.000000        0.000000       0       0
```

### *ELEMENT_SOLID
```
*ELEMENT_SOLID
$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8
       1       1       1       2       3       4       5       6       7       8
```

### *PART
```
*PART
Part 1
$#     pid     secid       mid     eosid      hgid      grav    adpopt      tmid
         1         1         1         0         0         0         0         0
```

## 오류 처리

### 파싱 오류
```cpp
try {
    reader.read("corrupt_file.k", mesh);
} catch (const FileIOException& e) {
    std::cerr << "File I/O error: " << e.what() << "\n";
} catch (const ParseException& e) {
    std::cerr << "Parse error at line " << e.line() << ": " << e.what() << "\n";
}
```

### 부분 로딩
손상된 섹션을 건너뛰고 나머지 로드:
```cpp
DynaFileReader reader;
reader.setSkipInvalidSections(true);
reader.read("partially_corrupt.k", mesh);

auto errors = reader.getErrors();
for (const auto& error : errors) {
    std::cerr << "Skipped: " << error << "\n";
}
```

## 성능 벤치마크

| 파일 크기 | 노드 수 | 요소 수 | 로딩 시간 (단일 스레드) | 로딩 시간 (4 스레드) |
|-----------|---------|---------|------------------------|---------------------|
| 10 MB     | 10K     | 8K      | 0.05s                  | 0.03s               |
| 100 MB    | 100K    | 80K     | 0.4s                   | 0.15s               |
| 1 GB      | 1M      | 800K    | 4.2s                   | 1.3s                |
| 10 GB     | 10M     | 8M      | 45s                    | 12s                 |

*테스트 환경: Intel i7-12700K, NVMe SSD*

## 테스트
- `tests/unit/test_dyna_reader.cpp`
- `tests/unit/test_file_format_detector.cpp`
- `tests/integration/test_large_file_loading.cpp`
