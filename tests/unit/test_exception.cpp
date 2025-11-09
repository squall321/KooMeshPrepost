#include <gtest/gtest.h>
#include "utils/Exception.h"

using namespace koomesh::utils;

/**
 * @brief Exception 테스트 픽스처
 */
class ExceptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 테스트 설정
    }

    void TearDown() override {
        // 테스트 정리
    }
};

// ======================================================================
// 에러 코드 테스트
// ======================================================================

TEST_F(ExceptionTest, ErrorCodeToString_GeneralErrors) {
    EXPECT_EQ("UNKNOWN", errorCodeToString(ErrorCode::UNKNOWN));
    EXPECT_EQ("NOT_IMPLEMENTED", errorCodeToString(ErrorCode::NOT_IMPLEMENTED));
    EXPECT_EQ("INVALID_ARGUMENT", errorCodeToString(ErrorCode::INVALID_ARGUMENT));
    EXPECT_EQ("NULL_POINTER", errorCodeToString(ErrorCode::NULL_POINTER));
    EXPECT_EQ("OUT_OF_RANGE", errorCodeToString(ErrorCode::OUT_OF_RANGE));
}

TEST_F(ExceptionTest, ErrorCodeToString_FileIOErrors) {
    EXPECT_EQ("FILE_NOT_FOUND", errorCodeToString(ErrorCode::FILE_NOT_FOUND));
    EXPECT_EQ("FILE_OPEN_FAILED", errorCodeToString(ErrorCode::FILE_OPEN_FAILED));
    EXPECT_EQ("FILE_READ_FAILED", errorCodeToString(ErrorCode::FILE_READ_FAILED));
    EXPECT_EQ("FILE_WRITE_FAILED", errorCodeToString(ErrorCode::FILE_WRITE_FAILED));
}

TEST_F(ExceptionTest, ErrorCodeToString_ParseErrors) {
    EXPECT_EQ("PARSE_ERROR", errorCodeToString(ErrorCode::PARSE_ERROR));
    EXPECT_EQ("INVALID_FORMAT", errorCodeToString(ErrorCode::INVALID_FORMAT));
    EXPECT_EQ("SYNTAX_ERROR", errorCodeToString(ErrorCode::SYNTAX_ERROR));
}

TEST_F(ExceptionTest, GetErrorCategory_General) {
    EXPECT_EQ("General", getErrorCategory(ErrorCode::UNKNOWN));
    EXPECT_EQ("General", getErrorCategory(ErrorCode::INVALID_ARGUMENT));
}

TEST_F(ExceptionTest, GetErrorCategory_FileIO) {
    EXPECT_EQ("FileIO", getErrorCategory(ErrorCode::FILE_NOT_FOUND));
    EXPECT_EQ("FileIO", getErrorCategory(ErrorCode::FILE_READ_FAILED));
}

TEST_F(ExceptionTest, GetErrorCategory_Parsing) {
    EXPECT_EQ("Parsing", getErrorCategory(ErrorCode::PARSE_ERROR));
    EXPECT_EQ("Parsing", getErrorCategory(ErrorCode::SYNTAX_ERROR));
}

TEST_F(ExceptionTest, GetErrorCategory_DataValidation) {
    EXPECT_EQ("DataValidation", getErrorCategory(ErrorCode::INVALID_DATA));
    EXPECT_EQ("DataValidation", getErrorCategory(ErrorCode::INVALID_NODE_ID));
}

TEST_F(ExceptionTest, GetErrorCategory_Memory) {
    EXPECT_EQ("Memory", getErrorCategory(ErrorCode::OUT_OF_MEMORY));
    EXPECT_EQ("Memory", getErrorCategory(ErrorCode::ALLOCATION_FAILED));
}

// ======================================================================
// KooMeshException 테스트
// ======================================================================

TEST_F(ExceptionTest, KooMeshException_BasicCreation) {
    KooMeshException ex(ErrorCode::INVALID_ARGUMENT, "Invalid argument provided");

    EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, ex.getErrorCode());
    EXPECT_EQ("Invalid argument provided", ex.getMessage());
    EXPECT_EQ("", ex.getFile());
    EXPECT_EQ(0, ex.getLine());
}

TEST_F(ExceptionTest, KooMeshException_WithFileAndLine) {
    KooMeshException ex(ErrorCode::NULL_POINTER, "Null pointer detected", "test.cpp", 42);

    EXPECT_EQ(ErrorCode::NULL_POINTER, ex.getErrorCode());
    EXPECT_EQ("Null pointer detected", ex.getMessage());
    EXPECT_EQ("test.cpp", ex.getFile());
    EXPECT_EQ(42, ex.getLine());
}

TEST_F(ExceptionTest, KooMeshException_WhatMessage) {
    KooMeshException ex(ErrorCode::INVALID_ARGUMENT, "Test message", "file.cpp", 100);

    std::string whatMsg = ex.what();
    EXPECT_NE(whatMsg.find("KooMeshException"), std::string::npos);
    EXPECT_NE(whatMsg.find("INVALID_ARGUMENT"), std::string::npos);
    EXPECT_NE(whatMsg.find("Test message"), std::string::npos);
    EXPECT_NE(whatMsg.find("file.cpp"), std::string::npos);
    EXPECT_NE(whatMsg.find("100"), std::string::npos);
}

TEST_F(ExceptionTest, KooMeshException_ThrowAndCatch) {
    try {
        throw KooMeshException(ErrorCode::NOT_IMPLEMENTED, "Feature not implemented");
        FAIL() << "Exception should be thrown";
    } catch (const KooMeshException& ex) {
        EXPECT_EQ(ErrorCode::NOT_IMPLEMENTED, ex.getErrorCode());
        EXPECT_EQ("Feature not implemented", ex.getMessage());
    } catch (...) {
        FAIL() << "Wrong exception type caught";
    }
}

TEST_F(ExceptionTest, KooMeshException_CatchAsStdException) {
    try {
        throw KooMeshException(ErrorCode::UNKNOWN, "Unknown error");
        FAIL() << "Exception should be thrown";
    } catch (const std::exception& ex) {
        std::string msg = ex.what();
        EXPECT_NE(msg.find("Unknown error"), std::string::npos);
    }
}

// ======================================================================
// FileIOException 테스트
// ======================================================================

TEST_F(ExceptionTest, FileIOException_BasicCreation) {
    FileIOException ex(ErrorCode::FILE_NOT_FOUND, "File not found", "/path/to/file.k");

    EXPECT_EQ(ErrorCode::FILE_NOT_FOUND, ex.getErrorCode());
    EXPECT_EQ("File not found", ex.getMessage());
    EXPECT_EQ("/path/to/file.k", ex.getFilePath());
}

TEST_F(ExceptionTest, FileIOException_WhatMessage) {
    FileIOException ex(ErrorCode::FILE_READ_FAILED, "Read failed", "/data/mesh.k");

    std::string whatMsg = ex.what();
    EXPECT_NE(whatMsg.find("FileIOException"), std::string::npos);
    EXPECT_NE(whatMsg.find("FILE_READ_FAILED"), std::string::npos);
    EXPECT_NE(whatMsg.find("Read failed"), std::string::npos);
    EXPECT_NE(whatMsg.find("/data/mesh.k"), std::string::npos);
}

TEST_F(ExceptionTest, FileIOException_ThrowAndCatch) {
    try {
        throw FileIOException(ErrorCode::PERMISSION_DENIED, "No permission", "/etc/config");
        FAIL() << "Exception should be thrown";
    } catch (const FileIOException& ex) {
        EXPECT_EQ(ErrorCode::PERMISSION_DENIED, ex.getErrorCode());
        EXPECT_EQ("/etc/config", ex.getFilePath());
    }
}

TEST_F(ExceptionTest, FileIOException_CatchAsBase) {
    try {
        throw FileIOException(ErrorCode::FILE_WRITE_FAILED, "Write failed", "/tmp/output.k");
        FAIL() << "Exception should be thrown";
    } catch (const KooMeshException& ex) {
        EXPECT_EQ(ErrorCode::FILE_WRITE_FAILED, ex.getErrorCode());
        EXPECT_EQ("Write failed", ex.getMessage());
    }
}

// ======================================================================
// ParseException 테스트
// ======================================================================

TEST_F(ExceptionTest, ParseException_BasicCreation) {
    ParseException ex(ErrorCode::SYNTAX_ERROR, "Unexpected token", 42, 15);

    EXPECT_EQ(ErrorCode::SYNTAX_ERROR, ex.getErrorCode());
    EXPECT_EQ("Unexpected token", ex.getMessage());
    EXPECT_EQ(42, ex.getLineNumber());
    EXPECT_EQ(15, ex.getColumnNumber());
}

TEST_F(ExceptionTest, ParseException_WhatMessage) {
    ParseException ex(ErrorCode::MISSING_KEYWORD, "Missing keyword", 100, 5);

    std::string whatMsg = ex.what();
    EXPECT_NE(whatMsg.find("ParseException"), std::string::npos);
    EXPECT_NE(whatMsg.find("MISSING_KEYWORD"), std::string::npos);
    EXPECT_NE(whatMsg.find("Missing keyword"), std::string::npos);
    EXPECT_NE(whatMsg.find("100"), std::string::npos);
    EXPECT_NE(whatMsg.find("5"), std::string::npos);
}

TEST_F(ExceptionTest, ParseException_ThrowAndCatch) {
    try {
        throw ParseException(ErrorCode::INVALID_FORMAT, "Invalid format", 10, 20);
        FAIL() << "Exception should be thrown";
    } catch (const ParseException& ex) {
        EXPECT_EQ(ErrorCode::INVALID_FORMAT, ex.getErrorCode());
        EXPECT_EQ(10, ex.getLineNumber());
        EXPECT_EQ(20, ex.getColumnNumber());
    }
}

// ======================================================================
// InvalidDataException 테스트
// ======================================================================

TEST_F(ExceptionTest, InvalidDataException_BasicCreation) {
    InvalidDataException ex(ErrorCode::INVALID_NODE_ID, "Invalid node ID", "Node123");

    EXPECT_EQ(ErrorCode::INVALID_NODE_ID, ex.getErrorCode());
    EXPECT_EQ("Invalid node ID", ex.getMessage());
    EXPECT_EQ("Node123", ex.getDataName());
}

TEST_F(ExceptionTest, InvalidDataException_WhatMessage) {
    InvalidDataException ex(ErrorCode::DUPLICATE_ID, "Duplicate ID found", "Element456");

    std::string whatMsg = ex.what();
    EXPECT_NE(whatMsg.find("InvalidDataException"), std::string::npos);
    EXPECT_NE(whatMsg.find("DUPLICATE_ID"), std::string::npos);
    EXPECT_NE(whatMsg.find("Duplicate ID found"), std::string::npos);
    EXPECT_NE(whatMsg.find("Element456"), std::string::npos);
}

TEST_F(ExceptionTest, InvalidDataException_ThrowAndCatch) {
    try {
        throw InvalidDataException(ErrorCode::INCONSISTENT_DATA, "Data mismatch", "MeshData");
        FAIL() << "Exception should be thrown";
    } catch (const InvalidDataException& ex) {
        EXPECT_EQ(ErrorCode::INCONSISTENT_DATA, ex.getErrorCode());
        EXPECT_EQ("MeshData", ex.getDataName());
    }
}

// ======================================================================
// OutOfMemoryException 테스트
// ======================================================================

TEST_F(ExceptionTest, OutOfMemoryException_BasicCreation) {
    OutOfMemoryException ex("Failed to allocate memory", 1024000);

    EXPECT_EQ(ErrorCode::OUT_OF_MEMORY, ex.getErrorCode());
    EXPECT_EQ("Failed to allocate memory", ex.getMessage());
    EXPECT_EQ(1024000, ex.getRequestedSize());
}

TEST_F(ExceptionTest, OutOfMemoryException_WhatMessage) {
    OutOfMemoryException ex("Memory allocation failed", 5000000);

    std::string whatMsg = ex.what();
    EXPECT_NE(whatMsg.find("OutOfMemoryException"), std::string::npos);
    EXPECT_NE(whatMsg.find("OUT_OF_MEMORY"), std::string::npos);
    EXPECT_NE(whatMsg.find("Memory allocation failed"), std::string::npos);
    EXPECT_NE(whatMsg.find("5000000"), std::string::npos);
}

TEST_F(ExceptionTest, OutOfMemoryException_ThrowAndCatch) {
    try {
        throw OutOfMemoryException("Cannot allocate buffer", 8192);
        FAIL() << "Exception should be thrown";
    } catch (const OutOfMemoryException& ex) {
        EXPECT_EQ(8192, ex.getRequestedSize());
    }
}

// ======================================================================
// RuntimeException 테스트
// ======================================================================

TEST_F(ExceptionTest, RuntimeException_BasicCreation) {
    RuntimeException ex(ErrorCode::UNKNOWN, "Runtime error occurred");

    EXPECT_EQ(ErrorCode::UNKNOWN, ex.getErrorCode());
    EXPECT_EQ("Runtime error occurred", ex.getMessage());
}

TEST_F(ExceptionTest, RuntimeException_ThrowAndCatch) {
    try {
        throw RuntimeException(ErrorCode::NOT_IMPLEMENTED, "Not implemented yet");
        FAIL() << "Exception should be thrown";
    } catch (const RuntimeException& ex) {
        EXPECT_EQ(ErrorCode::NOT_IMPLEMENTED, ex.getErrorCode());
        EXPECT_EQ("Not implemented yet", ex.getMessage());
    }
}

// ======================================================================
// 매크로 테스트
// ======================================================================

TEST_F(ExceptionTest, Macro_KOOMESH_THROW) {
    try {
        KOOMESH_THROW(KooMeshException, ErrorCode::INVALID_ARGUMENT, "Test message");
        FAIL() << "Exception should be thrown";
    } catch (const KooMeshException& ex) {
        EXPECT_EQ(ErrorCode::INVALID_ARGUMENT, ex.getErrorCode());
        EXPECT_EQ("Test message", ex.getMessage());
        EXPECT_FALSE(ex.getFile().empty());
        EXPECT_GT(ex.getLine(), 0);
    }
}

TEST_F(ExceptionTest, Macro_KOOMESH_THROW_FILE_IO) {
    try {
        KOOMESH_THROW_FILE_IO(ErrorCode::FILE_NOT_FOUND, "File missing", "/path/file.k");
        FAIL() << "Exception should be thrown";
    } catch (const FileIOException& ex) {
        EXPECT_EQ(ErrorCode::FILE_NOT_FOUND, ex.getErrorCode());
        EXPECT_EQ("File missing", ex.getMessage());
        EXPECT_EQ("/path/file.k", ex.getFilePath());
        EXPECT_FALSE(ex.getFile().empty());
        EXPECT_GT(ex.getLine(), 0);
    }
}

TEST_F(ExceptionTest, Macro_KOOMESH_THROW_PARSE) {
    try {
        KOOMESH_THROW_PARSE(ErrorCode::SYNTAX_ERROR, "Syntax error", 10, 5);
        FAIL() << "Exception should be thrown";
    } catch (const ParseException& ex) {
        EXPECT_EQ(ErrorCode::SYNTAX_ERROR, ex.getErrorCode());
        EXPECT_EQ("Syntax error", ex.getMessage());
        EXPECT_EQ(10, ex.getLineNumber());
        EXPECT_EQ(5, ex.getColumnNumber());
        EXPECT_FALSE(ex.getFile().empty());
        EXPECT_GT(ex.getLine(), 0);
    }
}

TEST_F(ExceptionTest, Macro_KOOMESH_THROW_INVALID_DATA) {
    try {
        KOOMESH_THROW_INVALID_DATA(ErrorCode::INVALID_NODE_ID, "Bad node", "Node123");
        FAIL() << "Exception should be thrown";
    } catch (const InvalidDataException& ex) {
        EXPECT_EQ(ErrorCode::INVALID_NODE_ID, ex.getErrorCode());
        EXPECT_EQ("Bad node", ex.getMessage());
        EXPECT_EQ("Node123", ex.getDataName());
        EXPECT_FALSE(ex.getFile().empty());
        EXPECT_GT(ex.getLine(), 0);
    }
}

TEST_F(ExceptionTest, Macro_KOOMESH_THROW_OUT_OF_MEMORY) {
    try {
        KOOMESH_THROW_OUT_OF_MEMORY("Out of memory", 1024);
        FAIL() << "Exception should be thrown";
    } catch (const OutOfMemoryException& ex) {
        EXPECT_EQ(ErrorCode::OUT_OF_MEMORY, ex.getErrorCode());
        EXPECT_EQ("Out of memory", ex.getMessage());
        EXPECT_EQ(1024, ex.getRequestedSize());
        EXPECT_FALSE(ex.getFile().empty());
        EXPECT_GT(ex.getLine(), 0);
    }
}

TEST_F(ExceptionTest, Macro_KOOMESH_THROW_RUNTIME) {
    try {
        KOOMESH_THROW_RUNTIME(ErrorCode::UNKNOWN, "Runtime error");
        FAIL() << "Exception should be thrown";
    } catch (const RuntimeException& ex) {
        EXPECT_EQ(ErrorCode::UNKNOWN, ex.getErrorCode());
        EXPECT_EQ("Runtime error", ex.getMessage());
        EXPECT_FALSE(ex.getFile().empty());
        EXPECT_GT(ex.getLine(), 0);
    }
}

// ======================================================================
// 복합 시나리오 테스트
// ======================================================================

TEST_F(ExceptionTest, MultipleExceptions_DifferentTypes) {
    // 파일 I/O 예외
    try {
        throw FileIOException(ErrorCode::FILE_NOT_FOUND, "File not found", "/data/mesh.k");
    } catch (const KooMeshException& ex) {
        EXPECT_EQ(ErrorCode::FILE_NOT_FOUND, ex.getErrorCode());
    }

    // 파싱 예외
    try {
        throw ParseException(ErrorCode::SYNTAX_ERROR, "Syntax error", 10, 5);
    } catch (const KooMeshException& ex) {
        EXPECT_EQ(ErrorCode::SYNTAX_ERROR, ex.getErrorCode());
    }

    // 데이터 유효성 예외
    try {
        throw InvalidDataException(ErrorCode::INVALID_DATA, "Invalid data", "NodeData");
    } catch (const KooMeshException& ex) {
        EXPECT_EQ(ErrorCode::INVALID_DATA, ex.getErrorCode());
    }
}

TEST_F(ExceptionTest, ExceptionHierarchy_PolymorphicBehavior) {
    // 다형성 테스트
    KooMeshException* exPtr = nullptr;

    try {
        throw FileIOException(ErrorCode::FILE_READ_FAILED, "Read failed", "/tmp/file.k");
    } catch (KooMeshException& ex) {
        exPtr = &ex;
        EXPECT_STREQ("FileIOException", ex.getTypeName());
    }

    EXPECT_NE(nullptr, exPtr);
}

TEST_F(ExceptionTest, ExceptionMessage_Formatting) {
    FileIOException ex(ErrorCode::FILE_OPEN_FAILED,
                       "Cannot open file for reading",
                       "/path/to/mesh.k",
                       "test.cpp",
                       42);

    std::string whatMsg = ex.what();

    // 메시지에 모든 정보가 포함되어 있는지 확인
    EXPECT_NE(whatMsg.find("FileIOException"), std::string::npos);
    EXPECT_NE(whatMsg.find("FileIO"), std::string::npos);
    EXPECT_NE(whatMsg.find("FILE_OPEN_FAILED"), std::string::npos);
    EXPECT_NE(whatMsg.find("Cannot open file for reading"), std::string::npos);
    EXPECT_NE(whatMsg.find("/path/to/mesh.k"), std::string::npos);
    EXPECT_NE(whatMsg.find("test.cpp"), std::string::npos);
    EXPECT_NE(whatMsg.find("42"), std::string::npos);
}

// ======================================================================
// 에러 코드 범위 테스트
// ======================================================================

TEST_F(ExceptionTest, ErrorCode_Categories) {
    // 각 카테고리의 에러 코드가 올바른 범위에 있는지 확인
    EXPECT_EQ("General", getErrorCategory(ErrorCode::UNKNOWN));
    EXPECT_EQ("General", getErrorCategory(ErrorCode::OUT_OF_RANGE));

    EXPECT_EQ("FileIO", getErrorCategory(ErrorCode::FILE_NOT_FOUND));
    EXPECT_EQ("FileIO", getErrorCategory(ErrorCode::PERMISSION_DENIED));

    EXPECT_EQ("Parsing", getErrorCategory(ErrorCode::PARSE_ERROR));
    EXPECT_EQ("Parsing", getErrorCategory(ErrorCode::SYNTAX_ERROR));

    EXPECT_EQ("DataValidation", getErrorCategory(ErrorCode::INVALID_DATA));
    EXPECT_EQ("DataValidation", getErrorCategory(ErrorCode::INCONSISTENT_DATA));

    EXPECT_EQ("Memory", getErrorCategory(ErrorCode::OUT_OF_MEMORY));
    EXPECT_EQ("Memory", getErrorCategory(ErrorCode::BUFFER_OVERFLOW));

    EXPECT_EQ("Rendering", getErrorCategory(ErrorCode::RENDER_ERROR));
    EXPECT_EQ("Rendering", getErrorCategory(ErrorCode::TEXTURE_LOAD_FAILED));

    EXPECT_EQ("Selection", getErrorCategory(ErrorCode::SELECTION_ERROR));
    EXPECT_EQ("Selection", getErrorCategory(ErrorCode::EMPTY_SELECTION));

    EXPECT_EQ("Configuration", getErrorCategory(ErrorCode::CONFIG_ERROR));
    EXPECT_EQ("Configuration", getErrorCategory(ErrorCode::INVALID_CONFIG_VALUE));
}

// ======================================================================
// 메인 함수
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
