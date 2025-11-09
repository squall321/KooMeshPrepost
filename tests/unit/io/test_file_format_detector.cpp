#include "io/FileFormatDetector.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

using namespace koomesh::io;
namespace fs = std::filesystem;

// ======================================================================
// Test Fixture
// ======================================================================

class FileFormatDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        testDir = fs::temp_directory_path() / "koomesh_format_test";
        fs::create_directories(testDir);
    }

    void TearDown() override {
        // Clean up test files
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    // Helper to create test file
    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(testDir / filename);
        file << content;
        file.close();
    }

    fs::path testDir;
};

// ======================================================================
// Extension-based Detection Tests
// ======================================================================

TEST_F(FileFormatDetectorTest, DetectByExtension_LSDYNA) {
    EXPECT_EQ(FileFormat::LSDYNA, FileFormatDetector::detectByExtension("model.k"));
    EXPECT_EQ(FileFormat::LSDYNA, FileFormatDetector::detectByExtension("model.key"));
    EXPECT_EQ(FileFormat::LSDYNA, FileFormatDetector::detectByExtension("model.dyn"));
    EXPECT_EQ(FileFormat::LSDYNA, FileFormatDetector::detectByExtension("/path/to/model.K"));
}

TEST_F(FileFormatDetectorTest, DetectByExtension_STL) {
    EXPECT_EQ(FileFormat::STL, FileFormatDetector::detectByExtension("model.stl"));
    EXPECT_EQ(FileFormat::STL, FileFormatDetector::detectByExtension("MODEL.STL"));
}

TEST_F(FileFormatDetectorTest, DetectByExtension_VTK) {
    EXPECT_EQ(FileFormat::VTK, FileFormatDetector::detectByExtension("model.vtk"));
    EXPECT_EQ(FileFormat::VTK, FileFormatDetector::detectByExtension("model.vtu"));
    EXPECT_EQ(FileFormat::VTK, FileFormatDetector::detectByExtension("model.vtp"));
    EXPECT_EQ(FileFormat::VTK, FileFormatDetector::detectByExtension("model.vts"));
    EXPECT_EQ(FileFormat::VTK, FileFormatDetector::detectByExtension("model.vti"));
}

TEST_F(FileFormatDetectorTest, DetectByExtension_Nastran) {
    EXPECT_EQ(FileFormat::NASTRAN, FileFormatDetector::detectByExtension("model.bdf"));
    EXPECT_EQ(FileFormat::NASTRAN, FileFormatDetector::detectByExtension("model.nas"));
    EXPECT_EQ(FileFormat::NASTRAN, FileFormatDetector::detectByExtension("model.dat"));
}

TEST_F(FileFormatDetectorTest, DetectByExtension_Unknown) {
    EXPECT_EQ(FileFormat::UNKNOWN, FileFormatDetector::detectByExtension("model.txt"));
    EXPECT_EQ(FileFormat::UNKNOWN, FileFormatDetector::detectByExtension("model.xyz"));
    EXPECT_EQ(FileFormat::UNKNOWN, FileFormatDetector::detectByExtension("model"));
}

// ======================================================================
// Content-based Detection Tests
// ======================================================================

TEST_F(FileFormatDetectorTest, DetectByContent_STL_ASCII) {
    createTestFile("ascii.stl", "solid TestObject\n  facet normal 0 0 1\n");
    auto filepath = testDir / "ascii.stl";

    EXPECT_EQ(FileFormat::STL, FileFormatDetector::detectByContent(filepath.string()));
}

TEST_F(FileFormatDetectorTest, DetectByContent_VTK_Legacy) {
    createTestFile("legacy.vtk", "# vtk DataFile Version 3.0\nMesh Data\nASCII\n");
    auto filepath = testDir / "legacy.vtk";

    EXPECT_EQ(FileFormat::VTK, FileFormatDetector::detectByContent(filepath.string()));
}

TEST_F(FileFormatDetectorTest, DetectByContent_VTK_XML) {
    createTestFile("xml.vtu", "<?xml version=\"1.0\"?>\n<VTKFile type=\"UnstructuredGrid\">\n");
    auto filepath = testDir / "xml.vtu";

    EXPECT_EQ(FileFormat::VTK, FileFormatDetector::detectByContent(filepath.string()));
}

TEST_F(FileFormatDetectorTest, DetectByContent_LSDYNA) {
    createTestFile("dyna.k", "*KEYWORD\n*NODE\n1,0.0,0.0,0.0\n");
    auto filepath = testDir / "dyna.k";

    EXPECT_EQ(FileFormat::LSDYNA, FileFormatDetector::detectByContent(filepath.string()));
}

TEST_F(FileFormatDetectorTest, DetectByContent_Nastran) {
    createTestFile("nastran.bdf", "BEGIN BULK\nGRID,1,,0.0,0.0,0.0\n");
    auto filepath = testDir / "nastran.bdf";

    EXPECT_EQ(FileFormat::NASTRAN, FileFormatDetector::detectByContent(filepath.string()));
}

TEST_F(FileFormatDetectorTest, DetectByContent_NonExistentFile) {
    EXPECT_EQ(FileFormat::UNKNOWN, FileFormatDetector::detectByContent("/nonexistent/file.k"));
}

// ======================================================================
// Combined Detection Tests
// ======================================================================

TEST_F(FileFormatDetectorTest, Detect_PreferExtension) {
    createTestFile("model.k", "*KEYWORD\n*NODE\n");
    auto filepath = testDir / "model.k";

    EXPECT_EQ(FileFormat::LSDYNA, FileFormatDetector::detect(filepath.string()));
}

TEST_F(FileFormatDetectorTest, Detect_AmbiguousExtension_UseContent) {
    createTestFile("data.dat", "*KEYWORD\n*NODE\n");
    auto filepath = testDir / "data.dat";

    // .dat is ambiguous (could be Nastran), so should check content
    FileFormat format = FileFormatDetector::detect(filepath.string());
    EXPECT_EQ(FileFormat::LSDYNA, format);
}

// ======================================================================
// Utility Method Tests
// ======================================================================

TEST_F(FileFormatDetectorTest, FormatName) {
    EXPECT_EQ("LS-DYNA Keyword", FileFormatDetector::formatName(FileFormat::LSDYNA));
    EXPECT_EQ("STereoLithography (STL)", FileFormatDetector::formatName(FileFormat::STL));
    EXPECT_EQ("VTK (Visualization Toolkit)", FileFormatDetector::formatName(FileFormat::VTK));
    EXPECT_EQ("Nastran BDF", FileFormatDetector::formatName(FileFormat::NASTRAN));
    EXPECT_EQ("Unknown", FileFormatDetector::formatName(FileFormat::UNKNOWN));
}

TEST_F(FileFormatDetectorTest, FormatExtensions) {
    auto dynaExts = FileFormatDetector::formatExtensions(FileFormat::LSDYNA);
    EXPECT_EQ(3, dynaExts.size());
    EXPECT_NE(dynaExts.end(), std::find(dynaExts.begin(), dynaExts.end(), ".k"));

    auto stlExts = FileFormatDetector::formatExtensions(FileFormat::STL);
    EXPECT_EQ(1, stlExts.size());
    EXPECT_EQ(".stl", stlExts[0]);

    auto unknownExts = FileFormatDetector::formatExtensions(FileFormat::UNKNOWN);
    EXPECT_TRUE(unknownExts.empty());
}

TEST_F(FileFormatDetectorTest, FileExists) {
    createTestFile("exists.k", "test");
    auto filepath = testDir / "exists.k";

    EXPECT_TRUE(FileFormatDetector::fileExists(filepath.string()));
    EXPECT_FALSE(FileFormatDetector::fileExists("/nonexistent/file.k"));
}
