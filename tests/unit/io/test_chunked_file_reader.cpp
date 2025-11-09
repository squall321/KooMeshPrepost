#include <gtest/gtest.h>
#include "io/ChunkedFileReader.h"
#include "core/Mesh.h"
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace koomesh::io;
using namespace koomesh::core;

class ChunkedFileReaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = std::filesystem::temp_directory_path() / "chunked_reader_test";
        std::filesystem::create_directories(testDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir);
    }

    std::filesystem::path createTestFile(const std::string& filename, const std::string& content) {
        auto filepath = testDir / filename;
        std::ofstream file(filepath);
        file << content;
        file.close();
        return filepath;
    }

    std::filesystem::path createLargeTestFile(const std::string& filename, size_t numNodes, size_t numElements) {
        auto filepath = testDir / filename;
        std::ofstream file(filepath);

        file << "*KEYWORD\n";
        file << "$# Large test file with " << numNodes << " nodes\n";
        file << "*NODE\n";

        for (size_t i = 1; i <= numNodes; ++i) {
            double x = static_cast<double>(i) * 0.1;
            double y = static_cast<double>(i) * 0.2;
            double z = static_cast<double>(i) * 0.3;
            file << i << "," << x << "," << y << "," << z << "\n";
        }

        file << "*ELEMENT_SOLID\n";
        for (size_t i = 1; i <= numElements; ++i) {
            size_t n1 = (i - 1) * 8 + 1;
            size_t n2 = n1 + 1;
            size_t n3 = n1 + 2;
            size_t n4 = n1 + 3;
            size_t n5 = n1 + 4;
            size_t n6 = n1 + 5;
            size_t n7 = n1 + 6;
            size_t n8 = n1 + 7;

            if (n8 > numNodes) break;

            file << i << ",1," << n1 << "," << n2 << "," << n3 << "," << n4 << ","
                 << n5 << "," << n6 << "," << n7 << "," << n8 << "\n";
        }

        file << "*END\n";
        file.close();
        return filepath;
    }

    std::filesystem::path testDir;
};

// ======================================================================
// Thread Pool Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, ThreadPoolCreation) {
    ThreadPool pool(4);
    EXPECT_EQ(pool.size(), 4);
}

TEST_F(ChunkedFileReaderTest, ThreadPoolAutoDetect) {
    ThreadPool pool(0);  // Auto-detect
    EXPECT_GT(pool.size(), 0);
}

TEST_F(ChunkedFileReaderTest, ThreadPoolTaskExecution) {
    ThreadPool pool(2);

    std::atomic<int> counter{0};

    for (int i = 0; i < 10; ++i) {
        pool.enqueue([&counter]() {
            ++counter;
        });
    }

    pool.wait();

    EXPECT_EQ(counter, 10);
}

TEST_F(ChunkedFileReaderTest, ThreadPoolParallelExecution) {
    ThreadPool pool(4);

    std::atomic<int> concurrent{0};
    std::atomic<int> maxConcurrent{0};

    for (int i = 0; i < 8; ++i) {
        pool.enqueue([&concurrent, &maxConcurrent]() {
            ++concurrent;

            int current = concurrent.load();
            int max = maxConcurrent.load();
            while (max < current && !maxConcurrent.compare_exchange_weak(max, current)) {}

            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            --concurrent;
        });
    }

    pool.wait();

    EXPECT_GT(maxConcurrent, 1);  // Should have parallel execution
}

// ======================================================================
// Basic Functionality Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, SupportedExtensions) {
    ChunkedFileReader reader;

    auto extensions = reader.supportedExtensions();

    EXPECT_EQ(extensions.size(), 4);
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".k"), extensions.end());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".key"), extensions.end());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".dyn"), extensions.end());
    EXPECT_NE(std::find(extensions.begin(), extensions.end(), ".keyword"), extensions.end());
}

TEST_F(ChunkedFileReaderTest, CanReadValidExtension) {
    ChunkedFileReader reader;

    EXPECT_TRUE(reader.canRead("model.k"));
    EXPECT_TRUE(reader.canRead("model.key"));
    EXPECT_TRUE(reader.canRead("model.KEY"));  // Case insensitive
    EXPECT_FALSE(reader.canRead("model.txt"));
}

TEST_F(ChunkedFileReaderTest, FormatName) {
    ChunkedFileReader reader;
    EXPECT_EQ(reader.formatName(), "LS-DYNA Keyword (Chunked)");
}

// ======================================================================
// Reading Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, ReadSimpleFile) {
    std::string content = R"(*KEYWORD
*NODE
1,0.0,0.0,0.0
2,1.0,0.0,0.0
3,0.0,1.0,0.0
*END
)";

    auto filepath = createTestFile("simple.k", content);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;

    auto result = reader.read(filepath.string(), mesh, options.baseOptions);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodesRead, 3);
}

TEST_F(ChunkedFileReaderTest, ReadWithChunkedOptions) {
    std::string content = R"(*KEYWORD
*NODE
1,0.0,0.0,0.0
2,1.0,0.0,0.0
3,0.0,1.0,0.0
4,1.0,1.0,0.0
*ELEMENT_SHELL
1,1,1,2,4,3
*END
)";

    auto filepath = createTestFile("with_elements.k", content);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;
    options.chunkSize = 1024;  // Small chunks
    options.numThreads = 2;

    auto result = reader.readChunked(filepath.string(), mesh, options);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodesRead, 4);
    EXPECT_EQ(result.elementsRead, 1);
}

TEST_F(ChunkedFileReaderTest, ReadLargeFile) {
    auto filepath = createLargeTestFile("large.k", 1000, 100);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;
    options.chunkSize = 10 * 1024;  // 10KB chunks
    options.numThreads = 4;

    auto result = reader.readChunked(filepath.string(), mesh, options);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodesRead, 1000);
    EXPECT_EQ(result.elementsRead, 100);

    // Check chunk statistics
    const auto& stats = reader.getLastChunkStats();
    EXPECT_GT(stats.totalChunks, 1);
    EXPECT_EQ(stats.chunksProcessed, stats.totalChunks);
    EXPECT_EQ(stats.totalLines, 1000 + 100 + 4);  // nodes + elements + keywords
}

TEST_F(ChunkedFileReaderTest, ReadVeryLargeFile) {
    auto filepath = createLargeTestFile("very_large.k", 10000, 1000);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;
    options.chunkSize = 50 * 1024;  // 50KB chunks
    options.numThreads = 0;  // Auto-detect

    auto startTime = std::chrono::high_resolution_clock::now();
    auto result = reader.readChunked(filepath.string(), mesh, options);
    auto endTime = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(endTime - startTime).count();

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.nodesRead, 10000);
    EXPECT_EQ(result.elementsRead, 1000);

    // Check performance
    const auto& stats = reader.getLastChunkStats();
    EXPECT_GT(stats.totalChunks, 1);

    std::cout << "Large file read performance:\n";
    std::cout << "  Total time: " << elapsed << " seconds\n";
    std::cout << "  Chunks: " << stats.totalChunks << "\n";
    std::cout << "  Chunking: " << stats.chunkingTimeSeconds << " seconds\n";
    std::cout << "  Parsing: " << stats.parsingTimeSeconds << " seconds\n";
    std::cout << "  Merging: " << stats.mergingTimeSeconds << " seconds\n";
    std::cout << "  Throughput: " << (result.nodesRead / elapsed) << " nodes/sec\n";
}

// ======================================================================
// Progress Tracking Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, ProgressTracking) {
    auto filepath = createLargeTestFile("progress.k", 1000, 100);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;
    options.chunkSize = 10 * 1024;

    std::vector<double> progressValues;
    std::vector<std::string> messages;

    auto result = reader.readChunked(filepath.string(), mesh, options,
        [&](double progress, const std::string& message) {
            progressValues.push_back(progress);
            messages.push_back(message);
            return true;
        });

    EXPECT_TRUE(result.success);
    EXPECT_GT(progressValues.size(), 0);

    // Check progress is monotonically increasing
    for (size_t i = 1; i < progressValues.size(); ++i) {
        EXPECT_GE(progressValues[i], progressValues[i - 1]);
    }

    // Check progress starts near 0 and ends at 1
    EXPECT_LE(progressValues.front(), 0.1);
    EXPECT_GE(progressValues.back(), 0.9);
}

TEST_F(ChunkedFileReaderTest, ProgressQuery) {
    auto filepath = createLargeTestFile("query.k", 5000, 500);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;

    std::thread readerThread([&]() {
        reader.readChunked(filepath.string(), mesh, options);
    });

    // Query progress from main thread
    std::vector<double> progressValues;

    while (reader.isReading()) {
        progressValues.push_back(reader.progress());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    readerThread.join();

    EXPECT_GT(progressValues.size(), 0);
}

// ======================================================================
// Cancellation Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, CancellationViaCallback) {
    auto filepath = createLargeTestFile("cancel.k", 10000, 1000);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;

    int callbackCount = 0;

    EXPECT_THROW({
        reader.readChunked(filepath.string(), mesh, options,
            [&](double progress, const std::string& message) {
                ++callbackCount;
                return callbackCount <= 3;  // Cancel after 3 callbacks
            });
    }, OperationCancelledException);

    EXPECT_EQ(callbackCount, 4);  // One more call after returning false
}

TEST_F(ChunkedFileReaderTest, CancellationManual) {
    auto filepath = createLargeTestFile("manual_cancel.k", 10000, 1000);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;

    std::thread readerThread([&]() {
        try {
            reader.readChunked(filepath.string(), mesh, options);
            FAIL() << "Should have thrown OperationCancelledException";
        } catch (const OperationCancelledException&) {
            // Expected
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    reader.cancel();

    readerThread.join();
}

// ======================================================================
// Error Handling Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, NonExistentFile) {
    ChunkedFileReader reader;
    Mesh mesh;

    EXPECT_THROW(
        reader.read("/nonexistent/file.k", mesh),
        FileOpenException
    );
}

TEST_F(ChunkedFileReaderTest, EmptyFile) {
    auto filepath = createTestFile("empty.k", "");

    ChunkedFileReader reader;
    Mesh mesh;

    auto result = reader.read(filepath.string(), mesh);

    EXPECT_FALSE(result.success);
    EXPECT_GT(result.errorCount, 0);
}

TEST_F(ChunkedFileReaderTest, InvalidFile) {
    std::string content = R"(*KEYWORD
*NODE
invalid data here
*END
)";

    auto filepath = createTestFile("invalid.k", content);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;

    auto result = reader.readChunked(filepath.string(), mesh, options);

    // Should complete but with warnings/errors
    EXPECT_TRUE(result.success);  // Parsing continues despite errors
}

// ======================================================================
// Chunk Statistics Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, ChunkStatistics) {
    auto filepath = createLargeTestFile("stats.k", 5000, 500);

    ChunkedFileReader reader;
    Mesh mesh;
    ChunkedReadOptions options;
    options.chunkSize = 20 * 1024;  // 20KB chunks

    auto result = reader.readChunked(filepath.string(), mesh, options);

    EXPECT_TRUE(result.success);

    const auto& stats = reader.getLastChunkStats();

    EXPECT_GT(stats.totalChunks, 0);
    EXPECT_EQ(stats.chunksProcessed, stats.totalChunks);
    EXPECT_GT(stats.totalLines, 0);
    EXPECT_GT(stats.avgChunkSize, 0);
    EXPECT_GT(stats.chunkingTimeSeconds, 0.0);
    EXPECT_GT(stats.parsingTimeSeconds, 0.0);
    EXPECT_GT(stats.mergingTimeSeconds, 0.0);

    std::cout << "Chunk statistics:\n";
    std::cout << "  Total chunks: " << stats.totalChunks << "\n";
    std::cout << "  Total lines: " << stats.totalLines << "\n";
    std::cout << "  Avg chunk size: " << stats.avgChunkSize << " bytes\n";
    std::cout << "  Chunking time: " << stats.chunkingTimeSeconds << " s\n";
    std::cout << "  Parsing time: " << stats.parsingTimeSeconds << " s\n";
    std::cout << "  Merging time: " << stats.mergingTimeSeconds << " s\n";
}

// ======================================================================
// Multi-threading Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, SingleThreadedVsMultiThreaded) {
    auto filepath = createLargeTestFile("comparison.k", 5000, 500);

    // Single-threaded
    ChunkedFileReader reader1;
    Mesh mesh1;
    ChunkedReadOptions options1;
    options1.numThreads = 1;

    auto start1 = std::chrono::high_resolution_clock::now();
    auto result1 = reader1.readChunked(filepath.string(), mesh1, options1);
    auto end1 = std::chrono::high_resolution_clock::now();
    double time1 = std::chrono::duration<double>(end1 - start1).count();

    // Multi-threaded
    ChunkedFileReader reader2;
    Mesh mesh2;
    ChunkedReadOptions options2;
    options2.numThreads = 4;

    auto start2 = std::chrono::high_resolution_clock::now();
    auto result2 = reader2.readChunked(filepath.string(), mesh2, options2);
    auto end2 = std::chrono::high_resolution_clock::now();
    double time2 = std::chrono::duration<double>(end2 - start2).count();

    EXPECT_TRUE(result1.success);
    EXPECT_TRUE(result2.success);

    EXPECT_EQ(result1.nodesRead, result2.nodesRead);
    EXPECT_EQ(result1.elementsRead, result2.elementsRead);

    std::cout << "Performance comparison:\n";
    std::cout << "  Single-threaded: " << time1 << " seconds\n";
    std::cout << "  Multi-threaded (4 threads): " << time2 << " seconds\n";
    std::cout << "  Speedup: " << (time1 / time2) << "x\n";
}

TEST_F(ChunkedFileReaderTest, DifferentThreadCounts) {
    auto filepath = createLargeTestFile("threads.k", 3000, 300);

    for (size_t numThreads : {1, 2, 4, 8}) {
        ChunkedFileReader reader;
        Mesh mesh;
        ChunkedReadOptions options;
        options.numThreads = numThreads;

        auto start = std::chrono::high_resolution_clock::now();
        auto result = reader.readChunked(filepath.string(), mesh, options);
        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();

        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.nodesRead, 3000);

        std::cout << "Threads: " << numThreads
                  << ", Time: " << elapsed << " s"
                  << ", Throughput: " << (result.nodesRead / elapsed) << " nodes/s\n";
    }
}

// ======================================================================
// Correctness Tests
// ======================================================================

TEST_F(ChunkedFileReaderTest, CorrectnessSingleVsChunked) {
    // Create test file
    auto filepath = createLargeTestFile("correctness.k", 1000, 100);

    // Read with single-threaded standard reader
    LSDynaFileReader standardReader;
    Mesh standardMesh;
    auto standardResult = standardReader.read(filepath.string(), standardMesh);

    // Read with chunked reader
    ChunkedFileReader chunkedReader;
    Mesh chunkedMesh;
    ChunkedReadOptions options;
    options.chunkSize = 5 * 1024;  // Small chunks to test splitting
    options.numThreads = 4;
    auto chunkedResult = chunkedReader.readChunked(filepath.string(), chunkedMesh, options);

    EXPECT_TRUE(standardResult.success);
    EXPECT_TRUE(chunkedResult.success);

    // Should read same number of nodes and elements
    EXPECT_EQ(standardResult.nodesRead, chunkedResult.nodesRead);
    EXPECT_EQ(standardResult.elementsRead, chunkedResult.elementsRead);
    EXPECT_EQ(standardMesh.nodeCount(), chunkedMesh.nodeCount());
    EXPECT_EQ(standardMesh.elementCount(), chunkedMesh.elementCount());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
