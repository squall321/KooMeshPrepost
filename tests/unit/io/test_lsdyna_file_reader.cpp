#include <gtest/gtest.h>
#include "io/LSDynaFileReader.h"
#include "core/Mesh.h"
#include <fstream>
#include <sstream>
#include <thread>

#ifdef _WIN32
    #define unlink _unlink
#else
    #include <unistd.h>
#endif

using namespace koomesh::io;
using namespace koomesh::core;

// ======================================================================
// Test Fixture
// ======================================================================

class LSDynaFileReaderTest : public ::testing::Test {
protected:
    LSDynaFileReader reader;

    void TearDown() override {
        // Clean up test files
        unlink("test_simple.k");
        unlink("test_complete.k");
        unlink("test_with_parts.k");
        unlink("test_free_format.k");
        unlink("test_invalid.k");
        unlink("test_empty.k");
    }

    void createSimpleTestFile(const std::string& filename) {
        std::ofstream file(filename);
        file << "*KEYWORD\n";
        file << "$\n";
        file << "$ Simple test mesh\n";
        file << "$\n";
        file << "*NODE\n";
        file << "       1      0.0000      0.0000      0.0000\n";
        file << "       2      1.0000      0.0000      0.0000\n";
        file << "       3      0.0000      1.0000      0.0000\n";
        file << "       4      0.0000      0.0000      1.0000\n";
        file << "*ELEMENT_SOLID\n";
        file << "      10       1       1       2       3       4\n";
        file << "*END\n";
        file.close();
    }

    void createCompleteTestFile(const std::string& filename) {
        std::ofstream file(filename);
        file << "*KEYWORD\n";
        file << "$\n";
        file << "$ Complete test mesh with multiple element types\n";
        file << "$\n";
        file << "*NODE\n";
        file << "       1      0.0000      0.0000      0.0000\n";
        file << "       2      1.0000      0.0000      0.0000\n";
        file << "       3      1.0000      1.0000      0.0000\n";
        file << "       4      0.0000      1.0000      0.0000\n";
        file << "       5      0.0000      0.0000      1.0000\n";
        file << "       6      1.0000      0.0000      1.0000\n";
        file << "       7      1.0000      1.0000      1.0000\n";
        file << "       8      0.0000      1.0000      1.0000\n";
        file << "$\n";
        file << "*ELEMENT_SOLID\n";
        file << "$ Tetrahedron\n";
        file << "      10       1       1       2       3       5\n";
        file << "$ Hexahedron\n";
        file << "      11       1       1       2       3       4       5       6       7       8\n";
        file << "$\n";
        file << "*ELEMENT_SHELL\n";
        file << "$ Triangle\n";
        file << "     100       2       1       2       3\n";
        file << "$ Quadrilateral\n";
        file << "     101       2       1       2       3       4\n";
        file << "$\n";
        file << "*ELEMENT_BEAM\n";
        file << "     200       3       1       2\n";
        file << "*END\n";
        file.close();
    }

    void createFileWithParts(const std::string& filename) {
        std::ofstream file(filename);
        file << "*KEYWORD\n";
        file << "*PART\n";
        file << "Steel Plate\n";
        file << "         1         1         1\n";
        file << "*PART\n";
        file << "Aluminum Frame\n";
        file << "         2         2         2\n";
        file << "*NODE\n";
        file << "       1      0.0000      0.0000      0.0000\n";
        file << "       2      1.0000      0.0000      0.0000\n";
        file << "       3      0.0000      1.0000      0.0000\n";
        file << "       4      0.0000      0.0000      1.0000\n";
        file << "*ELEMENT_SOLID\n";
        file << "      10       1       1       2       3       4\n";
        file << "*END\n";
        file.close();
    }

    void createFreeFormatFile(const std::string& filename) {
        std::ofstream file(filename);
        file << "*KEYWORD\n";
        file << "*NODE\n";
        file << "1, 0.0, 0.0, 0.0\n";
        file << "2, 1.0, 0.0, 0.0\n";
        file << "3, 0.0, 1.0, 0.0\n";
        file << "4, 0.0, 0.0, 1.0\n";
        file << "*ELEMENT_SOLID\n";
        file << "10, 1, 1, 2, 3, 4\n";
        file << "*END\n";
        file.close();
    }

    void createInvalidFile(const std::string& filename) {
        std::ofstream file(filename);
        file << "*KEYWORD\n";
        file << "*NODE\n";
        file << "       1      0.0000      0.0000      0.0000\n";
        file << "*ELEMENT_SOLID\n";
        file << "$ Element referencing non-existent nodes\n";
        file << "      10       1     999     998     997     996\n";
        file << "*END\n";
        file.close();
    }

    void createEmptyFile(const std::string& filename) {
        std::ofstream file(filename);
        file.flush();
        file.close();
    }
};

// ======================================================================
// Basic Functionality Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, CanReadValidExtensions) {
    EXPECT_TRUE(reader.canRead("test.k"));
    EXPECT_TRUE(reader.canRead("test.key"));
    EXPECT_TRUE(reader.canRead("test.dyn"));
    EXPECT_TRUE(reader.canRead("test.keyword"));
    EXPECT_TRUE(reader.canRead("TEST.K"));  // Case insensitive
}

TEST_F(LSDynaFileReaderTest, CannotReadInvalidExtensions) {
    EXPECT_FALSE(reader.canRead("test.txt"));
    EXPECT_FALSE(reader.canRead("test.dat"));
    EXPECT_FALSE(reader.canRead("test.inp"));
}

TEST_F(LSDynaFileReaderTest, SupportedExtensions) {
    auto extensions = reader.supportedExtensions();

    ASSERT_GE(extensions.size(), 3);
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".k"), extensions.end());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".key"), extensions.end());
}

TEST_F(LSDynaFileReaderTest, FormatName) {
    EXPECT_EQ("LS-DYNA Keyword", reader.formatName());
}

// ======================================================================
// Reading Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, ReadSimpleFile) {
    createSimpleTestFile("test_simple.k");

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;  // Skip validation for speed

    auto result = reader.read("test_simple.k", mesh, options);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(4, result.nodesRead);
    EXPECT_EQ(1, result.elementsRead);
    EXPECT_EQ(0, result.partsRead);

    // Verify mesh content
    EXPECT_EQ(4, mesh.nodeCount());
    EXPECT_EQ(1, mesh.elementCount());

    const Node* node = mesh.getNode(1);
    ASSERT_NE(nullptr, node);
    EXPECT_DOUBLE_EQ(0.0, node->x());

    const Element* elem = mesh.getElement(10);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::TETRAHEDRON, elem->type());
}

TEST_F(LSDynaFileReaderTest, ReadCompleteFile) {
    createCompleteTestFile("test_complete.k");

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;

    auto result = reader.read("test_complete.k", mesh, options);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(8, result.nodesRead);
    EXPECT_EQ(5, result.elementsRead);  // 2 solid + 2 shell + 1 beam

    // Verify element types
    const Element* tet = mesh.getElement(10);
    ASSERT_NE(nullptr, tet);
    EXPECT_EQ(ElementType::TETRAHEDRON, tet->type());

    const Element* hex = mesh.getElement(11);
    ASSERT_NE(nullptr, hex);
    EXPECT_EQ(ElementType::HEXAHEDRON, hex->type());

    const Element* tri = mesh.getElement(100);
    ASSERT_NE(nullptr, tri);
    EXPECT_EQ(ElementType::TRIANGLE, tri->type());

    const Element* quad = mesh.getElement(101);
    ASSERT_NE(nullptr, quad);
    EXPECT_EQ(ElementType::QUADRILATERAL, quad->type());

    const Element* beam = mesh.getElement(200);
    ASSERT_NE(nullptr, beam);
    EXPECT_EQ(ElementType::BEAM, beam->type());
}

TEST_F(LSDynaFileReaderTest, ReadFileWithParts) {
    createFileWithParts("test_with_parts.k");

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;

    auto result = reader.read("test_with_parts.k", mesh, options);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(2, result.partsRead);

    const Part* part1 = mesh.getPart(1);
    ASSERT_NE(nullptr, part1);
    EXPECT_EQ("Steel Plate", part1->name());

    const Part* part2 = mesh.getPart(2);
    ASSERT_NE(nullptr, part2);
    EXPECT_EQ("Aluminum Frame", part2->name());
}

TEST_F(LSDynaFileReaderTest, ReadFreeFormatFile) {
    createFreeFormatFile("test_free_format.k");

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;

    auto result = reader.read("test_free_format.k", mesh, options);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(4, result.nodesRead);
    EXPECT_EQ(1, result.elementsRead);
}

// ======================================================================
// Progress Callback Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, ProgressCallback) {
    createSimpleTestFile("test_simple.k");

    Mesh mesh;
    ReadOptions options;

    int callbackCount = 0;
    double lastProgress = 0.0;

    auto callback = [&](double progress, const std::string& message) {
        ++callbackCount;
        lastProgress = progress;
        EXPECT_GE(progress, 0.0);
        EXPECT_LE(progress, 1.0);
        EXPECT_FALSE(message.empty());
        return true;  // Continue
    };

    auto result = reader.read("test_simple.k", mesh, options, callback);

    EXPECT_TRUE(result.success);
    EXPECT_GT(callbackCount, 0);
    EXPECT_DOUBLE_EQ(1.0, lastProgress);
}

TEST_F(LSDynaFileReaderTest, CancelViaCallback) {
    createCompleteTestFile("test_complete.k");

    Mesh mesh;
    ReadOptions options;

    int callbackCount = 0;

    auto callback = [&](double progress, const std::string& message) {
        ++callbackCount;
        if (progress > 0.5) {
            return false;  // Cancel
        }
        return true;
    };

    EXPECT_THROW({
        reader.read("test_complete.k", mesh, options, callback);
    }, OperationCancelledException);

    EXPECT_GT(callbackCount, 0);
}

TEST_F(LSDynaFileReaderTest, ManualCancel) {
    createCompleteTestFile("test_complete.k");

    Mesh mesh;
    ReadOptions options;

    std::thread readerThread([&]() {
        try {
            reader.read("test_complete.k", mesh, options);
        } catch (const OperationCancelledException&) {
            // Expected
        }
    });

    // Wait a bit then cancel
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    reader.cancel();

    readerThread.join();
}

// ======================================================================
// Progress Tracking Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, ProgressTracking) {
    createCompleteTestFile("test_complete.k");

    Mesh mesh;
    ReadOptions options;

    std::thread readerThread([&]() {
        try {
            reader.read("test_complete.k", mesh, options);
        } catch (...) {}
    });

    // Check progress updates
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    double progress1 = reader.progress();
    EXPECT_GE(progress1, 0.0);

    readerThread.join();

    double finalProgress = reader.progress();
    EXPECT_DOUBLE_EQ(1.0, finalProgress);
}

TEST_F(LSDynaFileReaderTest, IsReadingState) {
    // Create a larger file to ensure reading takes some time
    std::ofstream file("test_reading_state.k");
    file << "*KEYWORD\n";
    file << "*NODE\n";
    for (int i = 1; i <= 1000; ++i) {
        file << i << " " << i*0.1 << " " << i*0.2 << " " << i*0.3 << "\n";
    }
    file << "*ELEMENT_SOLID\n";
    for (int i = 1; i <= 100; ++i) {
        int base = (i-1) * 8 + 1;
        file << i << " 1 " << base << " " << (base+1) << " " << (base+2) << " " << (base+3)
             << " " << (base+4) << " " << (base+5) << " " << (base+6) << " " << (base+7) << "\n";
    }
    file << "*END\n";
    file.close();

    EXPECT_FALSE(reader.isReading());

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;  // Skip validation for speed

    std::thread readerThread([&]() {
        reader.read("test_reading_state.k", mesh, options);
    });

    // Wait a bit for thread to start reading
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_TRUE(reader.isReading());

    readerThread.join();
    EXPECT_FALSE(reader.isReading());
}

// ======================================================================
// Validation Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, ValidationOnRead) {
    createInvalidFile("test_invalid.k");

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = true;

    auto result = reader.read("test_invalid.k", mesh, options);

    // Should succeed in reading, but have validation errors
    EXPECT_FALSE(result.isOk());
    EXPECT_GT(result.errorCount, 0);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(LSDynaFileReaderTest, SkipValidation) {
    createInvalidFile("test_invalid.k");

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;

    auto result = reader.read("test_invalid.k", mesh, options);

    // Should succeed without validation
    EXPECT_TRUE(result.success);
}

TEST_F(LSDynaFileReaderTest, StrictMode) {
    createSimpleTestFile("test_simple.k");

    Mesh mesh;
    ReadOptions options;
    options.strictMode = true;
    options.validateOnRead = true;

    auto result = reader.read("test_simple.k", mesh, options);

    // In strict mode, warnings become errors
    EXPECT_TRUE(result.success);
}

// ======================================================================
// Error Handling Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, NonExistentFile) {
    Mesh mesh;
    ReadOptions options;

    EXPECT_THROW({
        reader.read("nonexistent.k", mesh, options);
    }, FileOpenException);
}

TEST_F(LSDynaFileReaderTest, EmptyFile) {
    createEmptyFile("test_empty.k");

    // Verify file was created
    std::ifstream check("test_empty.k");
    ASSERT_TRUE(check.good()) << "Failed to create empty test file";
    check.close();

    Mesh mesh;
    ReadOptions options;

    auto result = reader.read("test_empty.k", mesh, options);

    EXPECT_FALSE(result.success);
    EXPECT_GT(result.errorCount, 0);
}

// ======================================================================
// Result Report Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, ReadResultReport) {
    createCompleteTestFile("test_complete.k");

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;

    auto result = reader.read("test_complete.k", mesh, options);

    std::string report = result.generateReport();

    EXPECT_FALSE(report.empty());
    EXPECT_NE(std::string::npos, report.find("SUCCESS"));
    EXPECT_NE(std::string::npos, report.find("Nodes"));
    EXPECT_NE(std::string::npos, report.find("Elements"));
}

TEST_F(LSDynaFileReaderTest, PerformanceMetrics) {
    createCompleteTestFile("test_complete.k");

    Mesh mesh;
    ReadOptions options;

    auto result = reader.read("test_complete.k", mesh, options);

    EXPECT_GT(result.readTimeSeconds, 0.0);
}

// ======================================================================
// Integration Tests
// ======================================================================

TEST_F(LSDynaFileReaderTest, LargeFileSimulation) {
    // Create a file with many nodes and elements
    std::ofstream file("test_large.k");
    file << "*KEYWORD\n";
    file << "*NODE\n";

    // Write 1000 nodes
    for (int i = 1; i <= 1000; ++i) {
        file << std::setw(8) << i
             << std::setw(16) << (i * 0.1)
             << std::setw(16) << (i * 0.2)
             << std::setw(16) << (i * 0.3) << "\n";
    }

    file << "*ELEMENT_SOLID\n";

    // Write 200 tetrahedrons
    for (int i = 1; i <= 200; ++i) {
        int n1 = (i - 1) * 4 + 1;
        int n2 = n1 + 1;
        int n3 = n1 + 2;
        int n4 = n1 + 3;
        file << std::setw(8) << i
             << std::setw(8) << 1
             << std::setw(8) << n1
             << std::setw(8) << n2
             << std::setw(8) << n3
             << std::setw(8) << n4 << "\n";
    }

    file << "*END\n";
    file.close();

    Mesh mesh;
    ReadOptions options;
    options.validateOnRead = false;

    auto result = reader.read("test_large.k", mesh, options);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(1000, result.nodesRead);
    EXPECT_EQ(200, result.elementsRead);

    unlink("test_large.k");
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
