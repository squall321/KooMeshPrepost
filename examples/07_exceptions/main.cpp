/**
 * @file main.cpp
 * @brief Exception 사용 예제
 *
 * KooMesh 예외 처리 프레임워크의 다양한 기능을 시연합니다:
 * 1. 기본 예외 사용
 * 2. 특수화된 예외 타입 (FileIO, Parse, InvalidData, OutOfMemory)
 * 3. 예외 매크로 사용
 * 4. Logger와의 통합
 * 5. 실제 시나리오 시뮬레이션
 */

#include "utils/Exception.h"
#include "utils/Logger.h"
#include <iostream>
#include <fstream>
#include <vector>

using namespace koomesh::utils;

// ======================================================================
// 예제 헬퍼 함수
// ======================================================================

void printSeparator(const std::string& title) {
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
}

// ======================================================================
// 예제 1: 기본 예외 사용
// ======================================================================

void example1_BasicException() {
    printSeparator("Example 1: 기본 예외 사용");

    try {
        // 기본 KooMeshException 던지기
        throw KooMeshException(ErrorCode::INVALID_ARGUMENT,
                              "Mesh size must be positive",
                              __FILE__, __LINE__);
    } catch (const KooMeshException& ex) {
        std::cout << "예외 발생!\n";
        std::cout << "  타입: " << ex.getTypeName() << "\n";
        std::cout << "  에러 코드: " << errorCodeToString(ex.getErrorCode()) << "\n";
        std::cout << "  카테고리: " << getErrorCategory(ex.getErrorCode()) << "\n";
        std::cout << "  메시지: " << ex.getMessage() << "\n";
        std::cout << "  위치: " << ex.getFile() << ":" << ex.getLine() << "\n";
        std::cout << "\n전체 메시지:\n  " << ex.what() << "\n";
    }
}

// ======================================================================
// 예제 2: FileIOException 사용
// ======================================================================

void simulateFileRead(const std::string& filepath) {
    // 파일이 존재하는지 확인
    std::ifstream file(filepath);
    if (!file.good()) {
        KOOMESH_THROW_FILE_IO(ErrorCode::FILE_NOT_FOUND,
                             "Cannot find the specified file",
                             filepath);
    }
    file.close();
}

void example2_FileIOException() {
    printSeparator("Example 2: FileIOException");

    try {
        simulateFileRead("/nonexistent/path/mesh.k");
    } catch (const FileIOException& ex) {
        std::cout << "파일 I/O 예외 발생!\n";
        std::cout << "  에러 코드: " << errorCodeToString(ex.getErrorCode()) << "\n";
        std::cout << "  메시지: " << ex.getMessage() << "\n";
        std::cout << "  파일 경로: " << ex.getFilePath() << "\n";
        std::cout << "\n전체 메시지:\n  " << ex.what() << "\n";
    }

    // 파일 쓰기 실패 시뮬레이션
    try {
        KOOMESH_THROW_FILE_IO(ErrorCode::FILE_WRITE_FAILED,
                             "Failed to write data to file",
                             "/read-only/output.k");
    } catch (const FileIOException& ex) {
        std::cout << "\n파일 쓰기 예외 발생!\n";
        std::cout << "  메시지: " << ex.getMessage() << "\n";
        std::cout << "  파일: " << ex.getFilePath() << "\n";
    }
}

// ======================================================================
// 예제 3: ParseException 사용
// ======================================================================

void simulateParseLine(const std::string& line, int lineNumber) {
    // *KEYWORD 형식이 아니면 에러
    if (line.empty() || line[0] != '*') {
        KOOMESH_THROW_PARSE(ErrorCode::MISSING_KEYWORD,
                           "Expected keyword starting with '*'",
                           lineNumber,
                           0);
    }

    // *NODE 키워드만 허용 (예시)
    if (line.find("*NODE") == std::string::npos &&
        line.find("*ELEMENT") == std::string::npos) {
        KOOMESH_THROW_PARSE(ErrorCode::INVALID_KEYWORD,
                           "Unknown keyword: " + line,
                           lineNumber,
                           0);
    }
}

void example3_ParseException() {
    printSeparator("Example 3: ParseException");

    std::vector<std::string> lines = {
        "*NODE",
        "1, 0.0, 0.0, 0.0",
        "INVALID LINE",  // 에러 발생
        "*ELEMENT_SOLID"
    };

    for (size_t i = 0; i < lines.size(); ++i) {
        try {
            simulateParseLine(lines[i], static_cast<int>(i + 1));
            std::cout << "라인 " << (i + 1) << " 파싱 성공: " << lines[i] << "\n";
        } catch (const ParseException& ex) {
            std::cout << "\n파싱 예외 발생!\n";
            std::cout << "  에러 코드: " << errorCodeToString(ex.getErrorCode()) << "\n";
            std::cout << "  메시지: " << ex.getMessage() << "\n";
            std::cout << "  라인: " << ex.getLineNumber() << "\n";
            std::cout << "\n전체 메시지:\n  " << ex.what() << "\n";
            break;
        }
    }
}

// ======================================================================
// 예제 4: InvalidDataException 사용
// ======================================================================

void validateNodeID(int nodeID) {
    if (nodeID <= 0) {
        KOOMESH_THROW_INVALID_DATA(ErrorCode::INVALID_NODE_ID,
                                   "Node ID must be positive",
                                   "NodeID=" + std::to_string(nodeID));
    }
}

void validateElementConnectivity(const std::vector<int>& nodeIDs) {
    if (nodeIDs.size() != 4 && nodeIDs.size() != 8) {
        KOOMESH_THROW_INVALID_DATA(ErrorCode::INVALID_DATA,
                                   "Element must have 4 or 8 nodes",
                                   "ElementConnectivity");
    }

    for (int nodeID : nodeIDs) {
        if (nodeID <= 0) {
            KOOMESH_THROW_INVALID_DATA(ErrorCode::INVALID_NODE_ID,
                                       "Invalid node ID in element connectivity",
                                       "NodeID=" + std::to_string(nodeID));
        }
    }
}

void example4_InvalidDataException() {
    printSeparator("Example 4: InvalidDataException");

    // 잘못된 노드 ID 검증
    try {
        validateNodeID(-5);
    } catch (const InvalidDataException& ex) {
        std::cout << "데이터 유효성 예외 발생!\n";
        std::cout << "  에러 코드: " << errorCodeToString(ex.getErrorCode()) << "\n";
        std::cout << "  메시지: " << ex.getMessage() << "\n";
        std::cout << "  데이터 이름: " << ex.getDataName() << "\n";
        std::cout << "\n전체 메시지:\n  " << ex.what() << "\n";
    }

    // 잘못된 요소 연결성 검증
    std::cout << "\n";
    try {
        std::vector<int> badConnectivity = {1, 2, 3};  // 3개 노드 (잘못됨)
        validateElementConnectivity(badConnectivity);
    } catch (const InvalidDataException& ex) {
        std::cout << "연결성 검증 예외 발생!\n";
        std::cout << "  메시지: " << ex.getMessage() << "\n";
        std::cout << "  데이터: " << ex.getDataName() << "\n";
    }
}

// ======================================================================
// 예제 5: OutOfMemoryException 사용
// ======================================================================

void allocateLargeBuffer(size_t size) {
    try {
        std::vector<char> buffer(size);
        std::cout << "성공적으로 " << size << " 바이트 할당\n";
    } catch (const std::bad_alloc&) {
        KOOMESH_THROW_OUT_OF_MEMORY("Failed to allocate large buffer", size);
    }
}

void example5_OutOfMemoryException() {
    printSeparator("Example 5: OutOfMemoryException");

    try {
        // 현실적인 크기 할당 (성공)
        allocateLargeBuffer(1024 * 1024);  // 1 MB

        // 매우 큰 크기 할당 시도 (실패 가능)
        size_t hugeSize = static_cast<size_t>(1e15);  // 1 PB (실패할 것임)
        allocateLargeBuffer(hugeSize);
    } catch (const OutOfMemoryException& ex) {
        std::cout << "\n메모리 부족 예외 발생!\n";
        std::cout << "  에러 코드: " << errorCodeToString(ex.getErrorCode()) << "\n";
        std::cout << "  메시지: " << ex.getMessage() << "\n";
        std::cout << "  요청 크기: " << ex.getRequestedSize() << " 바이트\n";
        std::cout << "\n전체 메시지:\n  " << ex.what() << "\n";
    }
}

// ======================================================================
// 예제 6: 예외 계층 구조 및 다형성
// ======================================================================

void throwDifferentExceptions(int type) {
    switch (type) {
        case 0:
            KOOMESH_THROW_FILE_IO(ErrorCode::FILE_NOT_FOUND, "File not found", "/test.k");
            break;
        case 1:
            KOOMESH_THROW_PARSE(ErrorCode::SYNTAX_ERROR, "Syntax error", 10, 5);
            break;
        case 2:
            KOOMESH_THROW_INVALID_DATA(ErrorCode::INVALID_DATA, "Invalid data", "TestData");
            break;
        case 3:
            KOOMESH_THROW_RUNTIME(ErrorCode::NOT_IMPLEMENTED, "Feature not implemented");
            break;
    }
}

void example6_ExceptionHierarchy() {
    printSeparator("Example 6: 예외 계층 구조");

    for (int i = 0; i < 4; ++i) {
        try {
            throwDifferentExceptions(i);
        } catch (const KooMeshException& ex) {
            std::cout << "예외 " << (i + 1) << " 발생:\n";
            std::cout << "  타입: " << ex.getTypeName() << "\n";
            std::cout << "  카테고리: " << getErrorCategory(ex.getErrorCode()) << "\n";
            std::cout << "  메시지: " << ex.getMessage() << "\n";
            std::cout << "\n";
        }
    }

    std::cout << "모든 예외가 KooMeshException으로 캐치되었습니다 (다형성).\n";
}

// ======================================================================
// 예제 7: Logger와 통합
// ======================================================================

void readMeshFile(const std::string& filepath) {
    Logger::infof("Reading mesh file: {}", filepath);

    try {
        std::ifstream file(filepath);
        if (!file.good()) {
            KOOMESH_THROW_FILE_IO(ErrorCode::FILE_NOT_FOUND,
                                 "Cannot open mesh file",
                                 filepath);
        }

        Logger::info("Mesh file opened successfully");

        // 파일 읽기 시뮬레이션
        std::string line;
        int lineNumber = 0;
        while (std::getline(file, line)) {
            ++lineNumber;

            if (line.find("*INVALID") != std::string::npos) {
                KOOMESH_THROW_PARSE(ErrorCode::INVALID_KEYWORD,
                                   "Invalid keyword encountered",
                                   lineNumber,
                                   0);
            }
        }

        Logger::info("Mesh file read successfully");

    } catch (const FileIOException& ex) {
        Logger::errorf("File I/O error: {}", ex.getMessage());
        Logger::errorf("  File: {}", ex.getFilePath());
        throw;  // 재throw
    } catch (const ParseException& ex) {
        Logger::errorf("Parse error: {}", ex.getMessage());
        Logger::errorf("  Line: {}", ex.getLineNumber());
        throw;
    }
}

void example7_WithLogger() {
    printSeparator("Example 7: Logger와 통합");

    Logger::getInstance().setLevel(LogLevel::INFO);
    Logger::getInstance().setConsoleOutput(true);

    try {
        readMeshFile("/nonexistent/mesh.k");
    } catch (const KooMeshException& ex) {
        std::cout << "\n예외가 로깅되었습니다:\n";
        std::cout << "  " << ex.what() << "\n";
    }
}

// ======================================================================
// 예제 8: 실제 시나리오 - 메시 로딩 시뮬레이션
// ======================================================================

struct MeshData {
    std::vector<int> nodeIDs;
    std::vector<int> elementIDs;
};

MeshData loadMeshData(const std::string& filepath) {
    MeshData data;

    // 파일 존재 확인
    std::ifstream file(filepath);
    if (!file.good()) {
        KOOMESH_THROW_FILE_IO(ErrorCode::FILE_NOT_FOUND,
                             "Mesh file does not exist",
                             filepath);
    }

    Logger::infof("Loading mesh from: {}", filepath);

    // 파일 읽기 시뮬레이션
    std::string line;
    int lineNumber = 0;
    bool inNodeSection = false;
    bool inElementSection = false;

    while (std::getline(file, line)) {
        ++lineNumber;

        if (line.empty() || line[0] == '$') {
            continue;  // 빈 줄 또는 주석 건너뛰기
        }

        if (line.find("*NODE") != std::string::npos) {
            inNodeSection = true;
            inElementSection = false;
            Logger::debug("Entering NODE section");
            continue;
        }

        if (line.find("*ELEMENT") != std::string::npos) {
            inElementSection = true;
            inNodeSection = false;
            Logger::debug("Entering ELEMENT section");
            continue;
        }

        if (inNodeSection) {
            // 노드 ID 파싱 (간단한 예시)
            int nodeID = lineNumber * 100;  // 임시
            if (nodeID <= 0) {
                KOOMESH_THROW_INVALID_DATA(ErrorCode::INVALID_NODE_ID,
                                           "Node ID must be positive",
                                           "Line " + std::to_string(lineNumber));
            }
            data.nodeIDs.push_back(nodeID);
        }

        if (inElementSection) {
            // 요소 ID 파싱
            int elementID = lineNumber * 200;  // 임시
            if (elementID <= 0) {
                KOOMESH_THROW_INVALID_DATA(ErrorCode::INVALID_ELEMENT_ID,
                                           "Element ID must be positive",
                                           "Line " + std::to_string(lineNumber));
            }
            data.elementIDs.push_back(elementID);
        }
    }

    file.close();

    Logger::infof("Loaded {} nodes and {} elements",
                  data.nodeIDs.size(),
                  data.elementIDs.size());

    // 데이터 검증
    if (data.nodeIDs.empty()) {
        KOOMESH_THROW_INVALID_DATA(ErrorCode::MISSING_DATA,
                                   "No nodes found in mesh file",
                                   filepath);
    }

    if (data.elementIDs.empty()) {
        KOOMESH_THROW_INVALID_DATA(ErrorCode::MISSING_DATA,
                                   "No elements found in mesh file",
                                   filepath);
    }

    return data;
}

void example8_RealWorldScenario() {
    printSeparator("Example 8: 실제 시나리오 - 메시 로딩");

    Logger::getInstance().setLevel(LogLevel::INFO);

    try {
        // 샘플 메시 파일 생성
        std::string testFile = "sample_mesh.k";
        std::ofstream out(testFile);
        out << "$# Sample mesh file\n";
        out << "*NODE\n";
        out << "1, 0.0, 0.0, 0.0\n";
        out << "2, 1.0, 0.0, 0.0\n";
        out << "*ELEMENT_SOLID\n";
        out << "1, 1, 1, 2, 3, 4, 5, 6, 7, 8\n";
        out.close();

        // 메시 로드
        MeshData mesh = loadMeshData(testFile);

        std::cout << "\n메시 로딩 성공!\n";
        std::cout << "  노드 수: " << mesh.nodeIDs.size() << "\n";
        std::cout << "  요소 수: " << mesh.elementIDs.size() << "\n";

        // 정리
        std::remove(testFile.c_str());

        // 존재하지 않는 파일 로드 시도
        std::cout << "\n존재하지 않는 파일 로드 시도...\n";
        loadMeshData("/nonexistent/file.k");

    } catch (const FileIOException& ex) {
        std::cout << "\n파일 I/O 예외:\n";
        std::cout << "  " << ex.what() << "\n";
    } catch (const InvalidDataException& ex) {
        std::cout << "\n데이터 유효성 예외:\n";
        std::cout << "  " << ex.what() << "\n";
    } catch (const KooMeshException& ex) {
        std::cout << "\n일반 예외:\n";
        std::cout << "  " << ex.what() << "\n";
    }
}

// ======================================================================
// 메인 함수
// ======================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "  Exception Framework Examples\n";
    std::cout << "========================================\n";

    try {
        example1_BasicException();
        example2_FileIOException();
        example3_ParseException();
        example4_InvalidDataException();
        example5_OutOfMemoryException();
        example6_ExceptionHierarchy();
        example7_WithLogger();
        example8_RealWorldScenario();

        printSeparator("모든 예제 완료");
        std::cout << "✓ 모든 예제가 성공적으로 실행되었습니다.\n";

    } catch (const std::exception& e) {
        std::cerr << "✗ 예기치 않은 오류 발생: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
