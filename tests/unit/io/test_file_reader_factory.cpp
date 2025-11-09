#include "io/FileReaderFactory.h"
#include "io/LSDynaFileReader.h"
#include "io/STLFileReader.h"
#include "core/Mesh.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

using namespace koomesh::io;
using namespace koomesh::core;
namespace fs = std::filesystem;

// ======================================================================
// Test Fixture
// ======================================================================

class FileReaderFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = fs::temp_directory_path() / "koomesh_factory_test";
        fs::create_directories(testDir);
    }

    void TearDown() override {
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(testDir / filename);
        file << content;
        file.close();
    }

    fs::path testDir;
};

// ======================================================================
// Reader Creation Tests
// ======================================================================

TEST_F(FileReaderFactoryTest, Create_LSDYNA) {
    auto reader = FileReaderFactory::create(FileFormat::LSDYNA);
    ASSERT_NE(nullptr, reader);
    EXPECT_EQ("LS-DYNA Keyword", reader->formatName());
}

TEST_F(FileReaderFactoryTest, Create_STL) {
    auto reader = FileReaderFactory::create(FileFormat::STL);
    ASSERT_NE(nullptr, reader);
    EXPECT_EQ("STL (STereoLithography)", reader->formatName());
}

TEST_F(FileReaderFactoryTest, Create_VTK) {
    auto reader = FileReaderFactory::create(FileFormat::VTK);
    ASSERT_NE(nullptr, reader);
    EXPECT_EQ("VTK (Visualization Toolkit)", reader->formatName());
}

TEST_F(FileReaderFactoryTest, Create_Nastran) {
    auto reader = FileReaderFactory::create(FileFormat::NASTRAN);
    ASSERT_NE(nullptr, reader);
    EXPECT_EQ("Nastran BDF (Bulk Data File)", reader->formatName());
}

TEST_F(FileReaderFactoryTest, Create_Unknown) {
    auto reader = FileReaderFactory::create(FileFormat::UNKNOWN);
    EXPECT_EQ(nullptr, reader);
}

// ======================================================================
// Auto-detection Tests
// ======================================================================

TEST_F(FileReaderFactoryTest, CreateFromFile_LSDYNA) {
    createTestFile("model.k", "*KEYWORD\n*NODE\n");
    auto filepath = testDir / "model.k";

    auto reader = FileReaderFactory::createFromFile(filepath.string());
    ASSERT_NE(nullptr, reader);
    EXPECT_EQ("LS-DYNA Keyword", reader->formatName());
}

TEST_F(FileReaderFactoryTest, CreateFromFile_STL) {
    createTestFile("model.stl", "solid TestObject\n");
    auto filepath = testDir / "model.stl";

    auto reader = FileReaderFactory::createFromFile(filepath.string());
    ASSERT_NE(nullptr, reader);
    EXPECT_EQ("STL (STereoLithography)", reader->formatName());
}

TEST_F(FileReaderFactoryTest, CreateFromFile_UnknownFormat_ThrowsException) {
    createTestFile("unknown.xyz", "random content");
    auto filepath = testDir / "unknown.xyz";

    EXPECT_THROW(
        FileReaderFactory::createFromFile(filepath.string()),
        FileIOException
    );
}

TEST_F(FileReaderFactoryTest, CreateFromFile_NonExistent_ThrowsException) {
    EXPECT_THROW(
        FileReaderFactory::createFromFile("/nonexistent/file.k"),
        FileIOException
    );
}

// ======================================================================
// Support Query Tests
// ======================================================================

TEST_F(FileReaderFactoryTest, IsFormatSupported) {
    EXPECT_TRUE(FileReaderFactory::isFormatSupported(FileFormat::LSDYNA));
    EXPECT_TRUE(FileReaderFactory::isFormatSupported(FileFormat::STL));
    EXPECT_TRUE(FileReaderFactory::isFormatSupported(FileFormat::VTK));
    EXPECT_TRUE(FileReaderFactory::isFormatSupported(FileFormat::NASTRAN));
    EXPECT_FALSE(FileReaderFactory::isFormatSupported(FileFormat::UNKNOWN));
}

TEST_F(FileReaderFactoryTest, GetSupportedFormats) {
    auto formats = FileReaderFactory::getSupportedFormats();

    EXPECT_GE(formats.size(), 4);  // At least LS-DYNA, STL, VTK, Nastran

    EXPECT_NE(formats.end(), std::find(formats.begin(), formats.end(), FileFormat::LSDYNA));
    EXPECT_NE(formats.end(), std::find(formats.begin(), formats.end(), FileFormat::STL));
    EXPECT_NE(formats.end(), std::find(formats.begin(), formats.end(), FileFormat::VTK));
    EXPECT_NE(formats.end(), std::find(formats.begin(), formats.end(), FileFormat::NASTRAN));
}

// ======================================================================
// Registration Tests
// ======================================================================

TEST_F(FileReaderFactoryTest, RegisterCustomReader) {
    // Create a mock reader
    class MockReader : public IFileReader {
    public:
        bool canRead(const std::string&) const override { return true; }
        std::vector<std::string> supportedExtensions() const override { return {".mock"}; }
        std::string formatName() const override { return "Mock Format"; }
        ReadResult read(const std::string&, Mesh&, const ReadOptions&, ProgressCallback) override {
            return ReadResult{true, "Mock read"};
        }
        double progress() const override { return 0.0; }
        void cancel() override {}
        bool isReading() const override { return false; }
    };

    // Register mock reader (using UNKNOWN as placeholder for custom format)
    FileReaderFactory::registerReader(FileFormat::UNKNOWN, []() {
        return std::make_unique<MockReader>();
    });

    // Verify registration
    auto reader = FileReaderFactory::create(FileFormat::UNKNOWN);
    ASSERT_NE(nullptr, reader);
    EXPECT_EQ("Mock Format", reader->formatName());
}

// ======================================================================
// ReadFile Convenience Method Tests
// ======================================================================

TEST_F(FileReaderFactoryTest, ReadFile_ValidSTL) {
    // Create a simple ASCII STL file
    std::string stlContent =
        "solid TestCube\n"
        "  facet normal 0 0 1\n"
        "    outer loop\n"
        "      vertex 0 0 0\n"
        "      vertex 1 0 0\n"
        "      vertex 1 1 0\n"
        "    endloop\n"
        "  endfacet\n"
        "endsolid TestCube\n";

    createTestFile("cube.stl", stlContent);
    auto filepath = testDir / "cube.stl";

    Mesh mesh;
    ReadResult result = FileReaderFactory::readFile(filepath.string(), mesh);

    EXPECT_TRUE(result.success);
    EXPECT_GT(mesh.nodeCount(), 0);
}

TEST_F(FileReaderFactoryTest, ReadFile_InvalidFile_ReturnsFailure) {
    createTestFile("invalid.xyz", "not a valid format");
    auto filepath = testDir / "invalid.xyz";

    Mesh mesh;
    EXPECT_THROW(
        FileReaderFactory::readFile(filepath.string(), mesh),
        FileIOException
    );
}
