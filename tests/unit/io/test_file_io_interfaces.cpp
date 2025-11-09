#include <gtest/gtest.h>
#include "io/IFileReader.h"
#include "io/IFileWriter.h"
#include "io/FileIOException.h"
#include "core/Mesh.h"
#include "helpers/TestUtils.h"
#include <atomic>
#include <chrono>
#include <thread>

using namespace koomesh::io;
using namespace koomesh::core;

// ======================================================================
// Mock Reader for Testing
// ======================================================================

class MockFileReader : public IFileReader {
private:
    std::atomic<double> m_progress{0.0};
    std::atomic<bool> m_isReading{false};
    std::atomic<bool> m_cancelRequested{false};

public:
    bool canRead(const std::string& filename) const override {
        return filename.find(".mock") != std::string::npos;
    }

    std::vector<std::string> supportedExtensions() const override {
        return {".mock", ".test"};
    }

    std::string formatName() const override {
        return "Mock Test Format";
    }

    ReadResult read(
        const std::string& filename,
        Mesh& mesh,
        const ReadOptions& options,
        ProgressCallback progressCallback) override {

        m_isReading = true;
        m_cancelRequested = false;
        m_progress = 0.0;

        ReadResult result;
        result.success = true;

        auto startTime = std::chrono::high_resolution_clock::now();

        try {
            // Simulate reading process
            for (int i = 0; i < 100; ++i) {
                if (m_cancelRequested) {
                    throw OperationCancelledException();
                }

                m_progress = i / 100.0;

                if (progressCallback) {
                    bool shouldContinue = progressCallback(
                        m_progress,
                        "Reading line " + std::to_string(i));
                    if (!shouldContinue) {
                        m_cancelRequested = true;
                        throw OperationCancelledException();
                    }
                }

                // Small delay to simulate I/O
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            // Add some test data
            mesh.addNode(std::make_unique<Node>(1, 0, 0, 0));
            mesh.addNode(std::make_unique<Node>(2, 1, 0, 0));
            mesh.addNode(std::make_unique<Node>(3, 0, 1, 0));

            result.nodesRead = 3;
            result.elementsRead = 0;
            result.partsRead = 0;

            m_progress = 1.0;

        } catch (const OperationCancelledException&) {
            result.success = false;
            result.message = "Operation cancelled";
            throw;
        } catch (const std::exception& e) {
            result.success = false;
            result.message = e.what();
            result.errorCount = 1;
            result.errors.push_back(e.what());
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.readTimeSeconds = std::chrono::duration<double>(endTime - startTime).count();

        m_isReading = false;
        return result;
    }

    double progress() const override {
        return m_progress.load();
    }

    void cancel() override {
        m_cancelRequested = true;
    }

    bool isReading() const override {
        return m_isReading.load();
    }
};

// ======================================================================
// Mock Writer for Testing
// ======================================================================

class MockFileWriter : public IFileWriter {
private:
    std::atomic<double> m_progress{0.0};
    std::atomic<bool> m_isWriting{false};
    std::atomic<bool> m_cancelRequested{false};

public:
    bool canWrite(const std::string& filename) const override {
        return filename.find(".mock") != std::string::npos;
    }

    std::vector<std::string> supportedExtensions() const override {
        return {".mock", ".test"};
    }

    std::string formatName() const override {
        return "Mock Test Format";
    }

    WriteResult write(
        const std::string& filename,
        const Mesh& mesh,
        const WriteOptions& options,
        ProgressCallback progressCallback) override {

        m_isWriting = true;
        m_cancelRequested = false;
        m_progress = 0.0;

        WriteResult result;
        result.success = true;

        auto startTime = std::chrono::high_resolution_clock::now();

        try {
            size_t totalItems = mesh.nodeCount() + mesh.elementCount();

            for (size_t i = 0; i < totalItems; ++i) {
                if (m_cancelRequested) {
                    throw OperationCancelledException();
                }

                m_progress = static_cast<double>(i) / totalItems;

                if (progressCallback && i % options.progressUpdateFrequency == 0) {
                    bool shouldContinue = progressCallback(
                        m_progress,
                        "Writing item " + std::to_string(i));
                    if (!shouldContinue) {
                        m_cancelRequested = true;
                        throw OperationCancelledException();
                    }
                }

                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }

            result.nodesWritten = mesh.nodeCount();
            result.elementsWritten = mesh.elementCount();
            result.partsWritten = mesh.partCount();
            result.bytesWritten = totalItems * 100;  // Mock size

            m_progress = 1.0;

        } catch (const OperationCancelledException&) {
            result.success = false;
            result.message = "Operation cancelled";
            throw;
        } catch (const std::exception& e) {
            result.success = false;
            result.message = e.what();
            result.errorCount = 1;
            result.errors.push_back(e.what());
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        result.writeTimeSeconds = std::chrono::duration<double>(endTime - startTime).count();

        m_isWriting = false;
        return result;
    }

    double progress() const override {
        return m_progress.load();
    }

    void cancel() override {
        m_cancelRequested = true;
    }

    bool isWriting() const override {
        return m_isWriting.load();
    }
};

// ======================================================================
// Exception Tests
// ======================================================================

TEST(FileIOExceptionTest, FileOpenException) {
    FileOpenException ex("test.k", "File not found");

    EXPECT_STREQ("Failed to open file 'test.k': File not found", ex.what());
    EXPECT_EQ("test.k", ex.filename());
}

TEST(FileIOExceptionTest, FileReadException) {
    FileReadException ex("test.k", 42, "Invalid format");

    EXPECT_EQ("test.k", ex.filename());
    EXPECT_EQ(42, ex.lineNumber());
    EXPECT_STRING_CONTAINS(std::string(ex.what()), "line 42");
}

TEST(FileIOExceptionTest, FileWriteException) {
    FileWriteException ex("output.k", "Disk full");

    EXPECT_EQ("output.k", ex.filename());
    EXPECT_STRING_CONTAINS(std::string(ex.what()), "Disk full");
}

TEST(FileIOExceptionTest, InvalidFormatException) {
    InvalidFormatException ex("test.k", 10, "*NODE", "*ELEMENT");

    EXPECT_EQ("test.k", ex.filename());
    EXPECT_EQ(10, ex.lineNumber());
    EXPECT_EQ("*NODE", ex.expected());
    EXPECT_EQ("*ELEMENT", ex.actual());
}

TEST(FileIOExceptionTest, OperationCancelledException) {
    OperationCancelledException ex;

    EXPECT_STRING_CONTAINS(std::string(ex.what()), "cancelled");
}

TEST(FileIOExceptionTest, UnsupportedFormatException) {
    UnsupportedFormatException ex("ABAQUS");

    EXPECT_EQ("ABAQUS", ex.format());
    EXPECT_STRING_CONTAINS(std::string(ex.what()), "ABAQUS");
}

// ======================================================================
// IFileReader Tests
// ======================================================================

class FileReaderTest : public ::testing::Test {
protected:
    std::unique_ptr<MockFileReader> reader;
    Mesh mesh;

    void SetUp() override {
        reader = std::make_unique<MockFileReader>();
    }
};

TEST_F(FileReaderTest, CanRead) {
    EXPECT_TRUE(reader->canRead("test.mock"));
    EXPECT_TRUE(reader->canRead("data.test"));
    EXPECT_FALSE(reader->canRead("data.k"));
}

TEST_F(FileReaderTest, SupportedExtensions) {
    auto extensions = reader->supportedExtensions();

    EXPECT_EQ(2, extensions.size());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".mock"), extensions.end());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".test"), extensions.end());
}

TEST_F(FileReaderTest, FormatName) {
    EXPECT_EQ("Mock Test Format", reader->formatName());
}

TEST_F(FileReaderTest, BasicRead) {
    ReadOptions options;
    auto result = reader->read("test.mock", mesh, options, nullptr);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(3, result.nodesRead);
    EXPECT_EQ(0, result.errorCount);
    EXPECT_GT(result.readTimeSeconds, 0.0);
}

TEST_F(FileReaderTest, ProgressCallback) {
    int callbackCount = 0;
    double lastProgress = 0.0;

    auto callback = [&](double progress, const std::string& message) {
        ++callbackCount;
        lastProgress = progress;
        EXPECT_GE(progress, 0.0);
        EXPECT_LE(progress, 1.0);
        return true;
    };

    ReadOptions options;
    auto result = reader->read("test.mock", mesh, options, callback);

    EXPECT_TRUE(result.success);
    EXPECT_GT(callbackCount, 0);
    EXPECT_DOUBLE_EQ(1.0, lastProgress);
}

TEST_F(FileReaderTest, CancelOperation) {
    auto callback = [&](double progress, const std::string& message) {
        if (progress > 0.5) {
            return false;  // Cancel
        }
        return true;
    };

    ReadOptions options;

    EXPECT_THROW({
        reader->read("test.mock", mesh, options, callback);
    }, OperationCancelledException);
}

TEST_F(FileReaderTest, ManualCancel) {
    std::thread readerThread([&]() {
        try {
            ReadOptions options;
            reader->read("test.mock", mesh, options, nullptr);
        } catch (const OperationCancelledException&) {
            // Expected
        }
    });

    // Wait a bit then cancel
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    reader->cancel();

    readerThread.join();
}

TEST_F(FileReaderTest, ProgressTracking) {
    std::thread readerThread([&]() {
        ReadOptions options;
        try {
            reader->read("test.mock", mesh, options, nullptr);
        } catch (...) {}
    });

    // Check progress updates
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    double progress1 = reader->progress();
    EXPECT_GT(progress1, 0.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    double progress2 = reader->progress();
    EXPECT_GE(progress2, progress1);

    readerThread.join();
}

TEST_F(FileReaderTest, IsReadingState) {
    EXPECT_FALSE(reader->isReading());

    std::thread readerThread([&]() {
        ReadOptions options;
        reader->read("test.mock", mesh, options, nullptr);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(reader->isReading());

    readerThread.join();
    EXPECT_FALSE(reader->isReading());
}

// ======================================================================
// IFileWriter Tests
// ======================================================================

class FileWriterTest : public ::testing::Test {
protected:
    std::unique_ptr<MockFileWriter> writer;
    Mesh mesh;

    void SetUp() override {
        writer = std::make_unique<MockFileWriter>();

        // Create test mesh
        mesh.addNode(std::make_unique<Node>(1, 0, 0, 0));
        mesh.addNode(std::make_unique<Node>(2, 1, 0, 0));
        mesh.addNode(std::make_unique<Node>(3, 0, 1, 0));
        mesh.addNode(std::make_unique<Node>(4, 0, 0, 1));
    }
};

TEST_F(FileWriterTest, CanWrite) {
    EXPECT_TRUE(writer->canWrite("output.mock"));
    EXPECT_TRUE(writer->canWrite("data.test"));
    EXPECT_FALSE(writer->canWrite("output.k"));
}

TEST_F(FileWriterTest, SupportedExtensions) {
    auto extensions = writer->supportedExtensions();

    EXPECT_EQ(2, extensions.size());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".mock"), extensions.end());
}

TEST_F(FileWriterTest, FormatName) {
    EXPECT_EQ("Mock Test Format", writer->formatName());
}

TEST_F(FileWriterTest, BasicWrite) {
    WriteOptions options;
    auto result = writer->write("output.mock", mesh, options, nullptr);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.isOk());
    EXPECT_EQ(4, result.nodesWritten);
    EXPECT_GT(result.bytesWritten, 0);
    EXPECT_GT(result.writeTimeSeconds, 0.0);
}

TEST_F(FileWriterTest, ProgressCallback) {
    int callbackCount = 0;

    auto callback = [&](double progress, const std::string& message) {
        ++callbackCount;
        EXPECT_GE(progress, 0.0);
        EXPECT_LE(progress, 1.0);
        return true;
    };

    WriteOptions options;
    auto result = writer->write("output.mock", mesh, options, callback);

    EXPECT_TRUE(result.success);
    EXPECT_GT(callbackCount, 0);
}

TEST_F(FileWriterTest, CancelOperation) {
    auto callback = [&](double progress, const std::string& message) {
        if (progress > 0.3) {
            return false;  // Cancel
        }
        return true;
    };

    WriteOptions options;

    EXPECT_THROW({
        writer->write("output.mock", mesh, options, callback);
    }, OperationCancelledException);
}

TEST_F(FileWriterTest, IsWritingState) {
    EXPECT_FALSE(writer->isWriting());

    std::thread writerThread([&]() {
        WriteOptions options;
        writer->write("output.mock", mesh, options, nullptr);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_TRUE(writer->isWriting());

    writerThread.join();
    EXPECT_FALSE(writer->isWriting());
}

// ======================================================================
// ReadResult Tests
// ======================================================================

TEST(ReadResultTest, IsOk) {
    ReadResult result;

    result.success = true;
    result.errorCount = 0;
    EXPECT_TRUE(result.isOk());

    result.errorCount = 1;
    EXPECT_FALSE(result.isOk());

    result.success = false;
    result.errorCount = 0;
    EXPECT_FALSE(result.isOk());
}

TEST(ReadResultTest, GenerateReport) {
    ReadResult result;
    result.success = true;
    result.nodesRead = 100;
    result.elementsRead = 50;
    result.partsRead = 5;
    result.readTimeSeconds = 1.234;

    std::string report = result.generateReport();

    EXPECT_STRING_CONTAINS(report, "SUCCESS");
    EXPECT_STRING_CONTAINS(report, "100");
    EXPECT_STRING_CONTAINS(report, "50");
    EXPECT_STRING_CONTAINS(report, "5");
    EXPECT_STRING_CONTAINS(report, "1.234");
}

TEST(ReadResultTest, GenerateReportWithErrors) {
    ReadResult result;
    result.success = false;
    result.errorCount = 2;
    result.errors.push_back("Error 1");
    result.errors.push_back("Error 2");
    result.warningCount = 1;
    result.warnings.push_back("Warning 1");

    std::string report = result.generateReport();

    EXPECT_STRING_CONTAINS(report, "FAILED");
    EXPECT_STRING_CONTAINS(report, "Error 1");
    EXPECT_STRING_CONTAINS(report, "Error 2");
    EXPECT_STRING_CONTAINS(report, "Warning 1");
}

// ======================================================================
// WriteResult Tests
// ======================================================================

TEST(WriteResultTest, IsOk) {
    WriteResult result;

    result.success = true;
    result.errorCount = 0;
    EXPECT_TRUE(result.isOk());

    result.errorCount = 1;
    EXPECT_FALSE(result.isOk());
}

TEST(WriteResultTest, GenerateReport) {
    WriteResult result;
    result.success = true;
    result.nodesWritten = 200;
    result.elementsWritten = 150;
    result.partsWritten = 3;
    result.bytesWritten = 1024 * 1024;  // 1 MB
    result.writeTimeSeconds = 0.5;

    std::string report = result.generateReport();

    EXPECT_STRING_CONTAINS(report, "SUCCESS");
    EXPECT_STRING_CONTAINS(report, "200");
    EXPECT_STRING_CONTAINS(report, "150");
    EXPECT_STRING_CONTAINS(report, "MB");
}

// ======================================================================
// Options Tests
// ======================================================================

TEST(ReadOptionsTest, Defaults) {
    ReadOptions options;

    EXPECT_TRUE(options.validateOnRead);
    EXPECT_FALSE(options.strictMode);
    EXPECT_FALSE(options.skipInvalidElements);
    EXPECT_FALSE(options.mergeNodes);
    EXPECT_DOUBLE_EQ(1e-6, options.mergeTolerance);
}

TEST(WriteOptionsTest, Defaults) {
    WriteOptions options;

    EXPECT_TRUE(options.writeComments);
    EXPECT_TRUE(options.prettyPrint);
    EXPECT_TRUE(options.validateBeforeWrite);
    EXPECT_FALSE(options.strictMode);
    EXPECT_EQ(6, options.precision);
    EXPECT_FALSE(options.scientificNotation);
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
