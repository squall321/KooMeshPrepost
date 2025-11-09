# LS-DYNA Keyword File Format Specification

## 개요

LS-DYNA Keyword 파일은 유한요소해석 입력 파일로, ASCII 텍스트 기반의 구조화된 형식입니다.

### 파일 특징
- **인코딩**: ASCII 텍스트
- **확장자**: `.k`, `.key`, `.dyn`, `.inc`
- **구조**: 키워드 기반 섹션으로 구성
- **대소문자**: 키워드는 대소문자 구분 없음
- **주석**: `$` 또는 `#`로 시작하는 라인

---

## 기본 구조

```
$# LS-DYNA Keyword File
$# Created: 2025-01-09

*KEYWORD
*TITLE
Sample Mesh Title

*NODE
$#   nid               x               y               z      tc      rc
       1            0.000           0.000           0.000       0       0
       2            1.000           0.000           0.000       0       0
       3            0.000           1.000           0.000       0       0

*ELEMENT_SOLID
$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8
       1       1       1       2       3       4       5       6       7       8

*PART
$# heading
Part 1
$#     pid     secid       mid     eosid      hgid      grav    adpopt      tmid
         1         1         1         0         0         0         0         0

*END
```

---

## 주요 키워드

### 1. *NODE

노드 좌표 정의

**형식:**
```
*NODE
$#   nid               x               y               z      tc      rc
       1            0.000           0.000           0.000       0       0
```

**필드 설명:**
| 필드 | 칼럼 | 타입 | 설명 |
|------|------|------|------|
| nid | 1-8 | int | Node ID (고유 식별자) |
| x | 9-24 | float | X 좌표 |
| y | 25-40 | float | Y 좌표 |
| z | 41-56 | float | Z 좌표 |
| tc | 57-64 | int | Temperature constraint (옵션) |
| rc | 65-72 | int | Rotation constraint (옵션) |

**파싱 규칙:**
- Fixed format (고정 길이)
- 각 필드는 정해진 칼럼 위치
- 부동소수점은 과학적 표기법 가능 (예: 1.23E-5)

---

### 2. *ELEMENT_SOLID

3D Solid 요소 정의 (육면체, 사면체 등)

**형식 (8-node Hexahedron):**
```
*ELEMENT_SOLID
$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8
       1       1       1       2       3       4       5       6       7       8
```

**필드 설명:**
| 필드 | 칼럼 | 타입 | 설명 |
|------|------|------|------|
| eid | 1-8 | int | Element ID |
| pid | 9-16 | int | Part ID |
| n1-n8 | 17-80 | int | Node IDs (요소 구성 노드) |

**노드 순서 (Hexahedron):**
```
      7----------6
     /|         /|
    / |        / |
   4----------5  |
   |  |       |  |
   |  3-------|--2
   | /        | /
   |/         |/
   0----------1
```

**특수 형식:**
- **Tetrahedron (4-node)**: n5=n6=n7=n8 (중복)
- **Pentahedron (6-node)**: n4=n7, n5=n8 (wedge)
- **Pyramid (5-node)**: n5=n6=n7=n8 (apex)

---

### 3. *ELEMENT_SHELL

2D Shell 요소 정의 (삼각형, 사각형)

**형식 (4-node Quadrilateral):**
```
*ELEMENT_SHELL
$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8
       1       1       1       2       3       4       0       0       0       0
```

**필드 설명:**
| 필드 | 칼럼 | 타입 | 설명 |
|------|------|------|------|
| eid | 1-8 | int | Element ID |
| pid | 9-16 | int | Part ID |
| n1-n4 | 17-48 | int | Node IDs (shell 구성 노드) |
| n5-n8 | 49-80 | int | 두께 방향 노드 (옵션, 보통 0) |

**노드 순서 (Quadrilateral):**
```
   3----------2
   |          |
   |          |
   |          |
   0----------1
```

**특수 형식:**
- **Triangle (3-node)**: n3=n4 (중복) 또는 n4=0

---

### 4. *ELEMENT_BEAM

1D Beam 요소 정의

**형식:**
```
*ELEMENT_BEAM
$#   eid     pid      n1      n2      n3     rt1     rr1     rt2     rr2   local
       1       1       1       2       0       0       0       0       0       2
```

**필드 설명:**
| 필드 | 칼럼 | 타입 | 설명 |
|------|------|------|------|
| eid | 1-8 | int | Element ID |
| pid | 9-16 | int | Part ID |
| n1 | 17-24 | int | Node 1 |
| n2 | 25-32 | int | Node 2 |
| n3 | 33-40 | int | Orientation node (옵션) |
| rt1/rr1 | 41-56 | int | Release flags node 1 |
| rt2/rr2 | 57-72 | int | Release flags node 2 |
| local | 73-80 | int | Local coordinate system |

---

### 5. *PART

파트 정의 (요소 그룹)

**형식:**
```
*PART
$# heading
Steel Chassis
$#     pid     secid       mid     eosid      hgid      grav    adpopt      tmid
         1         1         1         0         0         0         0         0
```

**필드 설명:**

**Line 1:** 파트 이름 (최대 80자)

**Line 2:**
| 필드 | 칼럼 | 타입 | 설명 |
|------|------|------|------|
| pid | 1-10 | int | Part ID |
| secid | 11-20 | int | Section ID |
| mid | 21-30 | int | Material ID |
| eosid | 31-40 | int | Equation of state ID |
| hgid | 41-50 | int | Hourglass ID |
| grav | 51-60 | int | Gravity option |
| adpopt | 61-70 | int | Adaptive option |
| tmid | 71-80 | int | Thermal material ID |

---

### 6. *SECTION_SOLID

Solid 요소 섹션 속성

**형식:**
```
*SECTION_SOLID
$#   secid      elform       aet    unused    unused    unused     cohoff      gaskt
         1           1         0                                         0          0
```

**필드 설명:**
| 필드 | 설명 |
|------|------|
| secid | Section ID |
| elform | Element formulation (1=constant stress, 2=selective reduced, etc.) |
| aet | Ambient element type |

---

### 7. *SECTION_SHELL

Shell 요소 섹션 속성

**형식:**
```
*SECTION_SHELL
$#   secid    elform      shrf       nip     propt   qr/irid     icomp     setyp
         1         2     1.000         2       1.0         0         0         1
$#      t1        t2        t3        t4      nloc     marea      idof    edgset
     1.000     1.000     1.000     1.000       0.0       0.0       0.0         0
```

**주요 필드:**
| 필드 | 설명 |
|------|------|
| secid | Section ID |
| elform | Element formulation (2=Belytschko-Tsay, 16=fully integrated, etc.) |
| shrf | Shear correction factor |
| nip | Number of integration points through thickness |
| t1-t4 | Shell thickness at nodes 1-4 |

---

### 8. *MAT_ELASTIC (Material)

탄성 재료 속성

**형식:**
```
*MAT_ELASTIC
$#     mid        ro         e        pr        da        db  not used
         1  7850.000  2.00E+11     0.300       0.0       0.0
```

**필드 설명:**
| 필드 | 설명 |
|------|------|
| mid | Material ID |
| ro | Mass density (kg/m³) |
| e | Young's modulus (Pa) |
| pr | Poisson's ratio |
| da | Damping coefficient (mass) |
| db | Damping coefficient (stiffness) |

---

### 9. *SET_NODE

노드 집합 정의

**형식:**
```
*SET_NODE
$#     sid       da1       da2       da3       da4    solver
         1       0.0       0.0       0.0       0.0MECH
$#    nid1      nid2      nid3      nid4      nid5      nid6      nid7      nid8
         1         2         3         4         5         6         7         8
        10        11        12        13
```

**필드 설명:**
| 필드 | 설명 |
|------|------|
| sid | Set ID |
| nid1-nid8 | Node IDs (한 줄에 최대 8개) |

---

### 10. *SET_PART

파트 집합 정의

**형식:**
```
*SET_PART
$#     sid       da1       da2       da3       da4    solver
         1       0.0       0.0       0.0       0.0MECH
$#    pid1      pid2      pid3      pid4      pid5      pid6      pid7      pid8
         1         2         3
```

---

## 파싱 전략

### 1. 키워드 감지

```cpp
bool isKeyword(const std::string& line) {
    return !line.empty() && line[0] == '*';
}

std::string extractKeyword(const std::string& line) {
    // "*NODE" -> "NODE"
    // "*ELEMENT_SOLID" -> "ELEMENT_SOLID"
    size_t start = 1;  // Skip '*'
    size_t end = line.find_first_of(" \t\r\n", start);
    return line.substr(start, end - start);
}
```

### 2. 주석 제거

```cpp
bool isComment(const std::string& line) {
    if (line.empty()) return true;
    return line[0] == '$' || line[0] == '#';
}
```

### 3. Fixed Format 파싱

```cpp
// *NODE: nid(1-8), x(9-24), y(25-40), z(41-56)
NodeId parseNodeId(const std::string& line) {
    return std::stoi(line.substr(0, 8));
}

double parseX(const std::string& line) {
    return std::stod(line.substr(8, 16));
}
```

### 4. Free Format 파싱 (일부 키워드)

일부 키워드는 콤마 또는 공백으로 구분된 자유 형식 지원:
```
*NODE
1, 0.0, 0.0, 0.0
2, 1.0, 0.0, 0.0
```

---

## 파일 읽기 순서

1. **헤더 확인**: `*KEYWORD` 키워드로 시작
2. **키워드별 파싱**:
   - `*NODE` → 모든 노드 읽기
   - `*ELEMENT_*` → 요소 읽기
   - `*PART` → 파트 정보 읽기
   - `*SECTION_*` → 섹션 속성 읽기
   - `*MAT_*` → 재료 속성 읽기
3. **종료**: `*END` 키워드

---

## 에러 처리

### 일반적인 오류

1. **중복 ID**: 같은 Node ID 또는 Element ID
2. **잘못된 참조**: 존재하지 않는 Node ID 참조
3. **형식 오류**: 필드 개수 불일치
4. **범위 오류**: 칼럼 위치 초과

### 오류 복구 전략

```cpp
try {
    parseNode(line);
} catch (const std::exception& e) {
    // Log error with line number
    logger.error("Line {}: {}", lineNumber, e.what());
    // Continue or abort based on severity
}
```

---

## 예제 파일

### 단순 사면체 메시

```
*KEYWORD
*TITLE
Single Tetrahedron

*NODE
       1            0.000           0.000           0.000
       2            1.000           0.000           0.000
       3            0.000           1.000           0.000
       4            0.000           0.000           1.000

*ELEMENT_SOLID
       1       1       1       2       3       4       4       4       4       4

*PART
Tetrahedron Part
         1         1         1

*SECTION_SOLID
         1           1

*MAT_ELASTIC
         1  2700.000  7.00E+10     0.330

*END
```

---

## 참고 문헌

- LS-DYNA Keyword User's Manual
- LS-DYNA Theory Manual
- [LS-DYNA Support Site](https://www.lstc.com/)

---

**작성일**: 2025-01-09
**버전**: 1.0
**작성자**: KooMeshPrepost Development Team
