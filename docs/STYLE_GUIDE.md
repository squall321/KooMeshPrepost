# KooMeshPrepost 스타일 가이드

이 문서는 코드 포맷팅과 스타일에 대한 빠른 참조 가이드입니다.
자세한 내용은 [CODING_GUIDELINES.md](CODING_GUIDELINES.md)를 참조하세요.

## 자동 포맷팅 도구

### clang-format
```bash
# 단일 파일 포맷
clang-format -i src/core/Mesh.cpp

# 모든 파일 포맷
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# 변경사항 확인 (실제 변경 없이)
clang-format --dry-run src/core/Mesh.cpp
```

### clang-tidy
```bash
# 단일 파일 검사
clang-tidy src/core/Mesh.cpp -- -I include

# 빌드 디렉토리에서 모든 파일 검사
cd build
run-clang-tidy

# 자동 수정 (주의!)
clang-tidy -fix src/core/Mesh.cpp -- -I include
```

## 빠른 참조

### 네이밍

| 항목 | 스타일 | 예제 |
|------|--------|------|
| 클래스/구조체 | PascalCase | `Mesh`, `ElementFactory` |
| 함수/메서드 | camelCase | `computeVolume()`, `isValid()` |
| 변수 | camelCase | `nodeCount`, `maxVolume` |
| 멤버 변수 | m_ + camelCase | `m_nodes`, `m_boundingBox` |
| 상수 | UPPER_CASE | `PI`, `MAX_NODES` |
| 열거자 | UPPER_CASE | `ElementType::TETRAHEDRON` |
| 네임스페이스 | lower_case | `koomesh::core` |

### 코드 예제

#### 클래스 정의
```cpp
namespace koomesh::core {

/**
 * @brief Mesh 클래스 설명
 */
class Mesh {
public:
    // 생성자
    Mesh();
    ~Mesh();

    // Public 메서드
    void addNode(const Node& node);
    size_t nodeCount() const noexcept;

private:
    // Private 멤버
    std::unordered_map<NodeId, Node> m_nodes;
    std::unique_ptr<ISpatialIndex> m_spatialIndex;

    // Private 메서드
    void buildIndex();
};

} // namespace koomesh::core
```

#### 함수 정의
```cpp
/**
 * @brief 경계 상자 계산
 * @param mesh 입력 mesh
 * @return 계산된 경계 상자
 * @throws std::invalid_argument mesh가 비어있는 경우
 */
BoundingBox computeBoundingBox(const Mesh& mesh) {
    if (mesh.nodeCount() == 0) {
        throw std::invalid_argument("Mesh is empty");
    }

    BoundingBox bbox;
    for (const auto& [id, node] : mesh.nodes()) {
        bbox.expand(node.coordinates());
    }

    return bbox;
}
```

#### 조건문
```cpp
// 짧은 조건문
if (count > 0) {
    process();
}

// 여러 조건
if (mesh.nodeCount() > 0 &&
    mesh.elementCount() > 0 &&
    mesh.isValid()) {
    buildIndex();
}

// else-if
if (type == ElementType::TETRAHEDRON) {
    processTet();
} else if (type == ElementType::HEXAHEDRON) {
    processHex();
} else {
    processOther();
}
```

#### 반복문
```cpp
// Range-based for (선호)
for (const auto& element : elements) {
    process(element);
}

// 구조화된 바인딩 (C++17)
for (const auto& [id, element] : mesh.elements()) {
    std::cout << "Element " << id << "\n";
}

// 인덱스 필요 시
for (size_t i = 0; i < elements.size(); ++i) {
    processWithIndex(elements[i], i);
}
```

#### Switch문
```cpp
switch (elementType) {
    case ElementType::TETRAHEDRON:
        return 4;

    case ElementType::HEXAHEDRON:
        return 8;

    case ElementType::TRIANGLE:
        return 3;

    default:
        throw std::runtime_error("Unknown element type");
}
```

## 코드 리뷰 체크리스트

작업 전:
- [ ] `.editorconfig` 적용 확인
- [ ] `.clang-format` 설정 확인

코드 작성 후:
- [ ] `clang-format -i` 실행
- [ ] `clang-tidy` 경고 확인 및 수정
- [ ] 컴파일 경고 0개
- [ ] Doxygen 주석 추가
- [ ] 단위 테스트 작성

커밋 전:
- [ ] `git diff` 로 변경사항 검토
- [ ] 불필요한 공백/빈 줄 제거
- [ ] 디버그 코드 제거 (cout, printf 등)

## IDE 설정

### Visual Studio Code
`.vscode/settings.json`:
```json
{
    "C_Cpp.clang_format_path": "/usr/bin/clang-format",
    "C_Cpp.clang_format_style": "file",
    "C_Cpp.codeAnalysis.clangTidy.enabled": true,
    "C_Cpp.codeAnalysis.clangTidy.path": "/usr/bin/clang-tidy",
    "editor.formatOnSave": true,
    "editor.rulers": [100],
    "files.trimTrailingWhitespace": true,
    "files.insertFinalNewline": true
}
```

### CLion
`Settings > Editor > Code Style > C/C++`:
- Scheme: Project
- Enable ClangFormat: Yes
- Use .clang-format config file

### Vim
`.vimrc`:
```vim
" ClangFormat
autocmd FileType cpp ClangFormatAutoEnable
let g:clang_format#style_options = {
    \ "BasedOnStyle": "file",
    \ }

" 저장 시 자동 포맷
autocmd BufWritePre *.cpp,*.h :ClangFormat
```

## 커밋 메시지 스타일

```
<type>: <subject>

<body>

<footer>
```

Types:
- `feat`: 새 기능
- `fix`: 버그 수정
- `docs`: 문서 변경
- `style`: 코드 포맷 (기능 변경 없음)
- `refactor`: 리팩토링
- `test`: 테스트 추가/수정
- `chore`: 빌드 시스템, 의존성 등

예제:
```
feat: Add Octree spatial indexing

Implement Octree class for fast spatial queries.
Performance: O(log n) query time for 1M+ elements.

Closes #42
```

## 참고 자료

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [clang-format documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-tidy checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
- [Doxygen manual](https://www.doxygen.nl/manual/)
