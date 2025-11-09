# File I/O Performance Optimization Guide

## 개요

KooMeshPrepost 파일 I/O 시스템의 성능 최적화 전략 및 벤치마크 가이드입니다.

---

## 현재 성능 특성

### 파싱 속도

| 파일 크기 | 요소 수 | 읽기 시간 (예상) | 처리량 |
|-----------|---------|-----------------|--------|
| 10 MB | 10만 | ~1초 | 100K 요소/초 |
| 100 MB | 100만 | ~10초 | 100K 요소/초 |
| 1 GB | 1000만 | ~100초 | 100K 요소/초 |

*Note: 실제 성능은 하드웨어 및 파일 구조에 따라 달라질 수 있습니다.*

### 메모리 사용량

| 요소 수 | 메모리 사용량 (예상) | 노드당 |
|---------|---------------------|--------|
| 10만 | ~50 MB | ~500 bytes |
| 100만 | ~500 MB | ~500 bytes |
| 1000만 | ~5 GB | ~500 bytes |

---

## 최적화 전략

### 1. 메모리 맵 파일 I/O 사용

**현재 구현**: MemoryMappedFile 클래스 사용

**장점**:
- OS 커널의 페이지 캐시 활용
- 대용량 파일을 효율적으로 처리
- 메모리 복사 최소화

**권장사항**:
```cpp
// 10MB 이상 파일의 경우 메모리 맵 사용
if (fileSize > 10 * 1024 * 1024) {
    MemoryMappedFile mmf;
    mmf.open(filepath);
    // Process mapped data
}
```

### 2. 청크 단위 파싱

**현재 구현**: ChunkedFileReader 클래스

**장점**:
- 메모리 사용량 제한
- 진행률 추적 용이
- 병렬 처리 가능

**권장사항**:
```cpp
ChunkedFileReader reader;
reader.setChunkSize(100 * 1024 * 1024); // 100MB chunks
reader.read(filepath, mesh);
```

### 3. 병렬 파싱

**현재 구현**: 준비 중

**제안**:
- 파일을 여러 청크로 분할
- 각 청크를 별도 스레드에서 파싱
- Intel TBB parallel_for 사용

**예상 성능 향상**: 2-4x (코어 수에 따라)

```cpp
// TODO: Implement parallel parsing
tbb::parallel_for(
    tbb::blocked_range<size_t>(0, chunks.size()),
    [&](const tbb::blocked_range<size_t>& r) {
        for (size_t i = r.begin(); i < r.end(); ++i) {
            parseChunk(chunks[i], mesh);
        }
    }
);
```

### 4. 메모리 할당 최적화

**전략**:
- Node/Element 벡터에 reserve() 사용
- Custom allocator 구현 (Pool allocator)
- SOA (Structure of Arrays) 레이아웃 고려

```cpp
// Reserve memory upfront if size is known
mesh.nodes().reserve(expectedNodeCount);
mesh.elements().reserve(expectedElementCount);
```

### 5. 파싱 최적화

**문자열 파싱**:
- `std::strtod` 대신 fast_float 라이브러리 사용
- `atoi`/`atof` 최적화된 버전 사용

**줄 단위 읽기**:
- `std::getline` 대신 buffer 기반 읽기
- SIMD 명령어로 구분자 탐색

```cpp
// Fast double parsing
#include <fast_float/fast_float.h>

double value;
auto result = fast_float::from_chars(str.data(), str.data() + str.size(), value);
if (result.ec == std::errc()) {
    // Success
}
```

---

## 벤치마크 방법

### 1. 시간 측정

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// File I/O operation
reader.read(filepath, mesh);

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration<double>(end - start).count();

std::cout << "Read time: " << duration << " seconds\n";
std::cout << "Throughput: " << (mesh.elementCount() / duration) << " elements/sec\n";
```

### 2. 메모리 프로파일링

**Linux (Valgrind Massif)**:
```bash
valgrind --tool=massif ./KooMeshPrepost
ms_print massif.out.* > memory_profile.txt
```

**macOS (Instruments)**:
```bash
instruments -t "Allocations" ./KooMeshPrepost
```

**Windows (Visual Studio Profiler)**:
```
Debug → Performance Profiler → Memory Usage
```

### 3. CPU 프로파일링

**Linux (perf)**:
```bash
perf record -g ./KooMeshPrepost
perf report
```

**Cross-platform (gprof)**:
```bash
g++ -pg -o KooMeshPrepost ...
./KooMeshPrepost
gprof KooMeshPrepost gmon.out > profile.txt
```

---

## 성능 테스트 케이스

### Small File (빠른 반응 테스트)
- 파일 크기: < 1 MB
- 요소 수: < 10,000
- 목표: < 0.1초

### Medium File (일반적인 사용)
- 파일 크기: 10-100 MB
- 요소 수: 100,000 - 1,000,000
- 목표: < 10초

### Large File (대용량 처리)
- 파일 크기: 1-10 GB
- 요소 수: 10,000,000 - 100,000,000
- 목표: < 300초

---

## 최적화 체크리스트

### 메모리 최적화
- [ ] 벡터에 reserve() 사용
- [ ] 불필요한 복사 제거 (move semantics)
- [ ] 스마트 포인터 최적화
- [ ] 중복 데이터 제거 (Flyweight pattern)

### I/O 최적화
- [ ] 메모리 맵 파일 사용 (대용량)
- [ ] 버퍼링 최적화
- [ ] 비동기 I/O 사용 (AsyncFileLoader)
- [ ] 진행률 추적 오버헤드 최소화

### 파싱 최적화
- [ ] fast_float 라이브러리 사용
- [ ] SIMD 명령어 활용
- [ ] 불필요한 문자열 할당 제거
- [ ] Boost.Spirit 파서 최적화

### 병렬화
- [ ] 청크 단위 병렬 파싱
- [ ] Intel TBB 활용
- [ ] 스레드 풀 사용
- [ ] Lock-free 데이터 구조

---

## 병목 지점 분석

### 일반적인 병목
1. **문자열 파싱** (30-40% 시간)
   - 해결: fast_float, SIMD
2. **메모리 할당** (20-30% 시간)
   - 해결: Pool allocator, reserve()
3. **디스크 I/O** (20-30% 시간)
   - 해결: Memory-mapped files, 비동기 I/O
4. **데이터 복사** (10-20% 시간)
   - 해결: Move semantics, 참조 사용

---

## 향후 개선 사항

### Short-term (1-3 months)
- fast_float 라이브러리 통합
- 병렬 파싱 구현
- Pool allocator 구현

### Medium-term (3-6 months)
- SIMD 최적화
- GPU 가속 파싱 (CUDA)
- Binary 파일 포맷 지원

### Long-term (6-12 months)
- Distributed 파싱 (MPI)
- 클라우드 스트리밍
- 실시간 증분 로딩

---

## 참고 자료

- [fast_float](https://github.com/fastfloat/fast_float)
- [Intel TBB](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html)
- [Memory-Mapped Files](https://en.wikipedia.org/wiki/Memory-mapped_file)
- [SIMD Programming](https://www.intel.com/content/www/us/en/docs/intrinsics-guide)
