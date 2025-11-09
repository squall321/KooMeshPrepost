#include <gtest/gtest.h>
#include "io/MemoryMappedFile.h"
#include "io/FileIOException.h"
#include <fstream>
#include <cstring>
#include <string>

#ifdef _WIN32
    #include <windows.h>
    #define unlink _unlink
#else
    #include <unistd.h>
#endif

using namespace koomesh::io;

// ======================================================================
// Test Fixture
// ======================================================================

class MemoryMappedFileTest : public ::testing::Test {
protected:
    std::string testFilename = "test_mmap.dat";
    std::string testContent = "Hello, Memory Mapped File!\nThis is a test file with multiple lines.\nLine 3\nLine 4\nEnd of file.";

    void SetUp() override {
        // Create test file
        createTestFile(testFilename, testContent);
    }

    void TearDown() override {
        // Clean up test file
        unlink(testFilename.c_str());
    }

    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream ofs(filename, std::ios::binary);
        ASSERT_TRUE(ofs.is_open());
        ofs.write(content.data(), content.size());
        ofs.close();
    }

    bool filesEqual(const std::string& file1, const std::string& file2) {
        std::ifstream f1(file1, std::ios::binary);
        std::ifstream f2(file2, std::ios::binary);

        if (!f1.is_open() || !f2.is_open()) {
            return false;
        }

        return std::equal(
            std::istreambuf_iterator<char>(f1),
            std::istreambuf_iterator<char>(),
            std::istreambuf_iterator<char>(f2));
    }
};

// ======================================================================
// Basic Functionality Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, ConstructDefault) {
    MemoryMappedFile mmf;

    EXPECT_FALSE(mmf.isValid());
    EXPECT_EQ(nullptr, mmf.data());
    EXPECT_EQ(0, mmf.size());
    EXPECT_EQ(0, mmf.fileSize());
}

TEST_F(MemoryMappedFileTest, OpenReadOnly) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    EXPECT_TRUE(mmf.isValid());
    EXPECT_NE(nullptr, mmf.data());
    EXPECT_EQ(testContent.size(), mmf.size());
    EXPECT_EQ(testContent.size(), mmf.fileSize());
    EXPECT_EQ(0, mmf.offset());
    EXPECT_EQ(MemoryMapMode::READ_ONLY, mmf.mode());
    EXPECT_EQ(testFilename, mmf.filename());
}

TEST_F(MemoryMappedFileTest, ReadContent) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf.isValid());

    std::string content(mmf.data(), mmf.size());
    EXPECT_EQ(testContent, content);
}

TEST_F(MemoryMappedFileTest, OpenNonExistentFile) {
    EXPECT_THROW({
        MemoryMappedFile mmf("nonexistent_file.dat", MemoryMapMode::READ_ONLY);
    }, FileOpenException);
}

TEST_F(MemoryMappedFileTest, CloseFile) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf.isValid());

    mmf.close();

    EXPECT_FALSE(mmf.isValid());
    EXPECT_EQ(nullptr, mmf.data());
    EXPECT_EQ(0, mmf.size());
}

TEST_F(MemoryMappedFileTest, ReopenFile) {
    MemoryMappedFile mmf;

    EXPECT_FALSE(mmf.isValid());

    bool success = mmf.open(testFilename, MemoryMapMode::READ_ONLY);

    EXPECT_TRUE(success);
    EXPECT_TRUE(mmf.isValid());

    std::string content(mmf.data(), mmf.size());
    EXPECT_EQ(testContent, content);
}

// ======================================================================
// Move Semantics Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, MoveConstructor) {
    MemoryMappedFile mmf1(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf1.isValid());
    const char* data1 = mmf1.data();
    size_t size1 = mmf1.size();

    MemoryMappedFile mmf2(std::move(mmf1));

    EXPECT_FALSE(mmf1.isValid());
    EXPECT_EQ(nullptr, mmf1.data());

    EXPECT_TRUE(mmf2.isValid());
    EXPECT_EQ(data1, mmf2.data());
    EXPECT_EQ(size1, mmf2.size());
}

TEST_F(MemoryMappedFileTest, MoveAssignment) {
    MemoryMappedFile mmf1(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf1.isValid());
    const char* data1 = mmf1.data();
    size_t size1 = mmf1.size();

    MemoryMappedFile mmf2;
    mmf2 = std::move(mmf1);

    EXPECT_FALSE(mmf1.isValid());
    EXPECT_TRUE(mmf2.isValid());
    EXPECT_EQ(data1, mmf2.data());
    EXPECT_EQ(size1, mmf2.size());
}

// ======================================================================
// Partial Mapping Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, PartialMapping) {
    size_t offset = 7;  // "Memory Mapped File..."
    size_t size = 20;

    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY, offset, size);

    ASSERT_TRUE(mmf.isValid());
    EXPECT_EQ(offset, mmf.offset());
    EXPECT_EQ(size, mmf.size());

    std::string content(mmf.data(), mmf.size());
    std::string expected = testContent.substr(offset, size);
    EXPECT_EQ(expected, content);
}

TEST_F(MemoryMappedFileTest, MappingFromMiddle) {
    size_t offset = 50;

    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY, offset);

    ASSERT_TRUE(mmf.isValid());
    EXPECT_EQ(offset, mmf.offset());
    EXPECT_EQ(testContent.size() - offset, mmf.size());

    std::string content(mmf.data(), mmf.size());
    std::string expected = testContent.substr(offset);
    EXPECT_EQ(expected, content);
}

// ======================================================================
// Write Mode Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, ReadWriteMode) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_WRITE);

    ASSERT_TRUE(mmf.isValid());
    EXPECT_NE(nullptr, mmf.dataMutable());

    // Modify content
    char* data = mmf.dataMutable();
    std::memcpy(data, "HELLO", 5);

    // Flush changes
    EXPECT_TRUE(mmf.flush());

    mmf.close();

    // Verify changes were written
    std::ifstream ifs(testFilename, std::ios::binary);
    std::string modified((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    EXPECT_TRUE(modified.substr(0, 5) == "HELLO");
}

TEST_F(MemoryMappedFileTest, ReadOnlyNoMutableAccess) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf.isValid());
    EXPECT_EQ(nullptr, mmf.dataMutable());
}

TEST_F(MemoryMappedFileTest, CopyOnWrite) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::COPY_ON_WRITE);

    ASSERT_TRUE(mmf.isValid());
    EXPECT_NE(nullptr, mmf.dataMutable());

    // Modify content (should not affect original file)
    char* data = mmf.dataMutable();
    std::memcpy(data, "HELLO", 5);

    mmf.close();

    // Verify original file is unchanged
    std::ifstream ifs(testFilename, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());

    EXPECT_EQ(testContent, content);
}

// ======================================================================
// Advice Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, AdviseSequential) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf.isValid());

    // Just test that it doesn't crash
    // Return value depends on platform
    mmf.advise(MemoryMappedFile::Advice::SEQUENTIAL);
}

TEST_F(MemoryMappedFileTest, AdviseRandom) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf.isValid());

    mmf.advise(MemoryMappedFile::Advice::RANDOM);
}

TEST_F(MemoryMappedFileTest, AdviseWillNeed) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf.isValid());

    mmf.advise(MemoryMappedFile::Advice::WILLNEED);
}

// ======================================================================
// Utility Functions Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, GetPageSize) {
    size_t pageSize = MemoryMappedFile::getPageSize();

    EXPECT_GT(pageSize, 0);
    EXPECT_TRUE(pageSize == 4096 || pageSize == 8192 || pageSize == 16384 || pageSize == 65536);
}

TEST_F(MemoryMappedFileTest, AlignToPage) {
    size_t pageSize = MemoryMappedFile::getPageSize();

    EXPECT_EQ(0, MemoryMappedFile::alignToPage(0));
    EXPECT_EQ(0, MemoryMappedFile::alignToPage(1));
    EXPECT_EQ(0, MemoryMappedFile::alignToPage(pageSize - 1));
    EXPECT_EQ(pageSize, MemoryMappedFile::alignToPage(pageSize));
    EXPECT_EQ(pageSize, MemoryMappedFile::alignToPage(pageSize + 1));
    EXPECT_EQ(pageSize * 2, MemoryMappedFile::alignToPage(pageSize * 2));
}

// ======================================================================
// Large File Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, LargeFile) {
    std::string largeFilename = "test_large.dat";
    size_t largeSize = 10 * 1024 * 1024;  // 10 MB

    // Create large file
    std::ofstream ofs(largeFilename, std::ios::binary);
    for (size_t i = 0; i < largeSize; ++i) {
        ofs.put(static_cast<char>(i % 256));
    }
    ofs.close();

    {
        MemoryMappedFile mmf(largeFilename, MemoryMapMode::READ_ONLY);

        ASSERT_TRUE(mmf.isValid());
        EXPECT_EQ(largeSize, mmf.size());

        // Verify some content
        const char* data = mmf.data();
        for (size_t i = 0; i < 1000; ++i) {
            EXPECT_EQ(static_cast<char>(i % 256), data[i]);
        }
    }

    unlink(largeFilename.c_str());
}

// ======================================================================
// ScopedMemoryMap Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, ScopedMemoryMap) {
    {
        ScopedMemoryMap scoped(testFilename, MemoryMapMode::READ_ONLY);

        EXPECT_TRUE(scoped.isValid());
        EXPECT_NE(nullptr, scoped.data());
        EXPECT_EQ(testContent.size(), scoped.size());

        std::string content(scoped.data(), scoped.size());
        EXPECT_EQ(testContent, content);
    }

    // File should be automatically closed when scope ends
}

TEST_F(MemoryMappedFileTest, ScopedMemoryMapMutable) {
    {
        ScopedMemoryMap scoped(testFilename, MemoryMapMode::READ_WRITE);

        ASSERT_TRUE(scoped.isValid());
        EXPECT_NE(nullptr, scoped.dataMutable());

        char* data = scoped.dataMutable();
        std::memcpy(data, "SCOPED", 6);

        scoped.get().flush();
    }

    // Verify changes
    std::ifstream ifs(testFilename, std::ios::binary);
    std::string modified((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    EXPECT_TRUE(modified.substr(0, 6) == "SCOPED");
}

// ======================================================================
// Error Handling Tests
// ======================================================================

TEST_F(MemoryMappedFileTest, InvalidOffset) {
    size_t invalidOffset = testContent.size() + 100;

    EXPECT_THROW({
        MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY, invalidOffset);
    }, FileOpenException);
}

TEST_F(MemoryMappedFileTest, InvalidSize) {
    size_t offset = 10;
    size_t invalidSize = testContent.size();  // offset + size > fileSize

    EXPECT_THROW({
        MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY, offset, invalidSize);
    }, FileOpenException);
}

TEST_F(MemoryMappedFileTest, FlushReadOnlyFails) {
    MemoryMappedFile mmf(testFilename, MemoryMapMode::READ_ONLY);

    ASSERT_TRUE(mmf.isValid());
    EXPECT_FALSE(mmf.flush());
}

// ======================================================================
// Multiple Mappings
// ======================================================================

TEST_F(MemoryMappedFileTest, MultipleReadOnlyMappings) {
    MemoryMappedFile mmf1(testFilename, MemoryMapMode::READ_ONLY);
    MemoryMappedFile mmf2(testFilename, MemoryMapMode::READ_ONLY);

    EXPECT_TRUE(mmf1.isValid());
    EXPECT_TRUE(mmf2.isValid());

    std::string content1(mmf1.data(), mmf1.size());
    std::string content2(mmf2.data(), mmf2.size());

    EXPECT_EQ(content1, content2);
    EXPECT_EQ(testContent, content1);
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
