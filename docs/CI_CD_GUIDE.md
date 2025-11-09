# CI/CD 가이드

KooMeshPrepost는 GitHub Actions를 사용하여 자동화된 CI/CD 파이프라인을 운영합니다.

## 워크플로우 개요

### 1. CI (Continuous Integration)
**파일**: `.github/workflows/ci.yml`
**트리거**: Push, Pull Request (main, develop, claude/* 브랜치)

#### 빌드 매트릭스
- **Ubuntu**: 22.04, 24.04
- **macOS**: 13, 14
- **Windows**: 2022
- **컴파일러**: GCC 11/12, Clang 14/15, MSVC 2022

#### 검사 항목
- ✅ 다중 플랫폼 빌드
- ✅ 단위 테스트 실행
- ✅ Sanitizers (Address, Undefined, Thread)
- ✅ 코드 커버리지 (Codecov)

### 2. Code Quality
**파일**: `.github/workflows/code-quality.yml`
**트리거**: Push, Pull Request

#### 검사 항목
- ✅ **clang-format**: 코드 포맷 검사
- ✅ **clang-tidy**: 정적 분석 (C++ Core Guidelines)
- ✅ **cppcheck**: 추가 정적 분석
- ✅ **Doxygen**: 문서 생성 검사
- ✅ **Include guards**: `#pragma once` 검사
- ✅ **License headers**: 라이선스 헤더 검사
- ✅ **TODO tracker**: TODO/FIXME 추적
- ✅ **Complexity**: 코드 복잡도 분석 (Lizard)

### 3. Release
**파일**: `.github/workflows/release.yml`
**트리거**: Git tag (v*.*.*)

#### 빌드 출력물
- **Linux**: DEB, RPM, TGZ
- **macOS**: DMG
- **Windows**: NSIS installer, ZIP

#### 자동화
- GitHub Release 생성
- 패키지 업로드
- Changelog 자동 생성
- 문서 GitHub Pages 배포

### 4. Nightly Build
**파일**: `.github/workflows/nightly.yml`
**트리거**: 매일 UTC 00:00 (한국 시간 09:00)

#### 목적
- 최신 코드 지속적 검증
- 빌드 문제 조기 발견
- 실패 시 자동 이슈 생성

---

## 사용 가이드

### Pull Request 워크플로우

1. **Feature 브랜치 생성**
   ```bash
   git checkout -b feature/my-feature
   ```

2. **코드 작성 및 로컬 검증**
   ```bash
   # 포맷 확인
   clang-format -i src/**/*.cpp include/**/*.h

   # 빌드 및 테스트
   cmake -B build -DBUILD_TESTS=ON
   cmake --build build
   cd build && ctest
   ```

3. **Push 및 PR 생성**
   ```bash
   git push origin feature/my-feature
   ```

   GitHub에서 Pull Request 생성 시 자동으로:
   - CI 빌드 (모든 플랫폼)
   - 코드 품질 검사
   - 테스트 실행

4. **CI 통과 확인**
   - ✅ 모든 빌드 성공
   - ✅ 모든 테스트 통과
   - ✅ 코드 품질 검사 통과

5. **Merge**
   - CI 통과 후 main/develop에 병합

### 릴리스 프로세스

1. **버전 태그 생성**
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. **자동 실행**
   - 모든 플랫폼 릴리스 빌드
   - 패키지 생성 (DEB, RPM, DMG, NSIS, ZIP)
   - GitHub Release 생성
   - 문서 GitHub Pages 배포

3. **릴리스 확인**
   - GitHub Releases 페이지에서 다운로드 링크 확인
   - 문서: `https://yourusername.github.io/KooMeshPrepost/docs/v1.0.0`

---

## CI 상태 배지

README.md에 추가:

```markdown
![CI](https://github.com/yourusername/KooMeshPrepost/workflows/CI/badge.svg)
![Code Quality](https://github.com/yourusername/KooMeshPrepost/workflows/Code%20Quality/badge.svg)
[![codecov](https://codecov.io/gh/yourusername/KooMeshPrepost/branch/main/graph/badge.svg)](https://codecov.io/gh/yourusername/KooMeshPrepost)
```

---

## 트러블슈팅

### CI 빌드 실패

#### 1. 컴파일 오류
```bash
# 로컬에서 동일한 컴파일러로 테스트
cmake -B build -DCMAKE_CXX_COMPILER=g++-12
cmake --build build
```

#### 2. 테스트 실패
```bash
# 테스트 자세히 실행
cd build
ctest --output-on-failure --verbose
```

#### 3. 의존성 문제
- GitHub Actions 로그에서 `Install dependencies` 단계 확인
- 필요 시 `.github/workflows/ci.yml` 수정

### 코드 품질 검사 실패

#### clang-format
```bash
# 자동 수정
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
git add -u
git commit -m "fix: Apply clang-format"
```

#### clang-tidy
```bash
# 로컬 실행
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy src/core/Mesh.cpp -p build
```

#### cppcheck
```bash
# 로컬 실행
cppcheck --enable=all --std=c++17 -I include src/
```

### 릴리스 실패

#### 패키징 오류
```bash
# 로컬에서 패키지 생성 테스트
cmake -B build -DCMAKE_BUILD_TYPE=Release
cd build
cpack -G "DEB;RPM;TGZ"  # Linux
cpack -G "DragNDrop"     # macOS
cpack -G "NSIS;ZIP"      # Windows
```

---

## 고급 설정

### 커스텀 CI 트리거

특정 브랜치만 CI 실행:
```yaml
on:
  push:
    branches: [ main, develop, feature/* ]
```

### 조건부 Job 실행

문서 변경 시 빌드 스킵:
```yaml
jobs:
  build:
    if: "!contains(github.event.head_commit.message, '[skip ci]')"
```

### Secrets 설정

1. GitHub 저장소 → Settings → Secrets
2. New repository secret 클릭
3. 예: `CODECOV_TOKEN`, `DEPLOY_KEY`

사용:
```yaml
- name: Upload to Codecov
  env:
    CODECOV_TOKEN: ${{ secrets.CODECOV_TOKEN }}
  run: bash <(curl -s https://codecov.io/bash)
```

### 캐시 사용

빌드 속도 향상을 위한 의존성 캐싱:
```yaml
- name: Cache vcpkg
  uses: actions/cache@v3
  with:
    path: |
      ${{ env.VCPKG_ROOT }}/installed
      ${{ env.VCPKG_ROOT }}/packages
    key: vcpkg-${{ runner.os }}-${{ hashFiles('**/vcpkg.json') }}
```

---

## 모범 사례

### 1. 커밋 메시지
- 의미 있는 커밋 메시지 작성
- `[skip ci]` 사용으로 불필요한 CI 방지 (문서 변경 등)

### 2. PR 크기
- 작고 집중된 PR 유지
- 리뷰 및 CI 시간 단축

### 3. 테스트 작성
- 새 기능마다 단위 테스트 추가
- CI에서 자동 검증

### 4. 코드 품질
- PR 전에 로컬에서 `clang-format`, `clang-tidy` 실행
- CI 실패 방지

### 5. 의존성 관리
- 필수 의존성 최소화
- 선택적 의존성은 QUIET 플래그 사용

---

## 참고 자료

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [CMake + GitHub Actions](https://github.com/marketplace/actions/run-cmake)
- [Codecov Documentation](https://docs.codecov.com/)
- [clang-tidy checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
