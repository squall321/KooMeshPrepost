#include "io/AsyncFileLoader.h"
#include "core/Mesh.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace koomesh::io;
using namespace koomesh::core;
namespace fs = std::filesystem;

// ======================================================================
// Test Fixture
// ======================================================================

class AsyncFileLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        testDir = fs::temp_directory_path() / "koomesh_async_test";
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

    // Create a simple STL test file
    void createSimpleSTL(const std::string& filename) {
        std::string content =
            "solid TestObject\n"
            "  facet normal 0 0 1\n"
            "    outer loop\n"
            "      vertex 0 0 0\n"
            "      vertex 1 0 0\n"
            "      vertex 0 1 0\n"
            "    endloop\n"
            "  endfacet\n"
            "endsolid TestObject\n";
        createTestFile(filename, content);
    }

    fs::path testDir;
};

// ======================================================================
// Basic Async Loading Tests
// ======================================================================

TEST_F(AsyncFileLoaderTest, LoadAsync_ValidFile_Success) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    auto future = loader.loadAsync(filepath.string(), mesh);

    EXPECT_TRUE(loader.isLoading());

    AsyncLoadResult result = future.get();

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(loader.isLoading());
    EXPECT_GT(mesh.nodeCount(), 0);
    EXPECT_GT(result.elapsedSeconds, 0.0);
}

TEST_F(AsyncFileLoaderTest, LoadAsync_InvalidFile_Failure) {
    createTestFile("invalid.xyz", "not a valid file");
    auto filepath = testDir / "invalid.xyz";

    AsyncFileLoader loader;
    Mesh mesh;

    auto future = loader.loadAsync(filepath.string(), mesh);
    AsyncLoadResult result = future.get();

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.message.empty());
}

TEST_F(AsyncFileLoaderTest, LoadAsync_NonExistentFile_Failure) {
    AsyncFileLoader loader;
    Mesh mesh;

    auto future = loader.loadAsync("/nonexistent/file.stl", mesh);
    AsyncLoadResult result = future.get();

    EXPECT_FALSE(result.success);
}

// ======================================================================
// Progress Tracking Tests
// ======================================================================

TEST_F(AsyncFileLoaderTest, ProgressTracking) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    bool progressCalled = false;
    double lastProgress = 0.0;

    auto future = loader.loadAsync(filepath.string(), mesh,
        [&](double progress, const std::string& message) {
            progressCalled = true;
            lastProgress = progress;
            EXPECT_GE(progress, 0.0);
            EXPECT_LE(progress, 1.0);
            return true;  // Continue
        });

    AsyncLoadResult result = future.get();

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(progressCalled);
    EXPECT_GT(lastProgress, 0.0);
}

TEST_F(AsyncFileLoaderTest, GetProgress) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    auto future = loader.loadAsync(filepath.string(), mesh);

    // Progress should be between 0 and 1 while loading
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    double progress = loader.progress();
    EXPECT_GE(progress, 0.0);
    EXPECT_LE(progress, 1.0);

    future.get();

    // Progress should be 1.0 after completion
    EXPECT_DOUBLE_EQ(1.0, loader.progress());
}

// ======================================================================
// Cancellation Tests
// ======================================================================

TEST_F(AsyncFileLoaderTest, Cancel_StopsLoading) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    auto future = loader.loadAsync(filepath.string(), mesh);

    // Cancel immediately
    loader.cancel();

    AsyncLoadResult result = future.get();

    // Result may be success or cancelled depending on timing
    // but should not crash
    EXPECT_FALSE(loader.isLoading());
}

TEST_F(AsyncFileLoaderTest, CancelViaCallback) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    int callbackCount = 0;

    auto future = loader.loadAsync(filepath.string(), mesh,
        [&](double progress, const std::string& message) {
            callbackCount++;
            return false;  // Request cancellation
        });

    AsyncLoadResult result = future.get();

    EXPECT_GT(callbackCount, 0);
    EXPECT_FALSE(loader.isLoading());
}

// ======================================================================
// State Management Tests
// ======================================================================

TEST_F(AsyncFileLoaderTest, IsLoading_StateTracking) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    EXPECT_FALSE(loader.isLoading());

    auto future = loader.loadAsync(filepath.string(), mesh);

    EXPECT_TRUE(loader.isLoading());

    future.get();

    EXPECT_FALSE(loader.isLoading());
}

TEST_F(AsyncFileLoaderTest, MultipleLoads_Sequential) {
    createSimpleSTL("test1.stl");
    createSimpleSTL("test2.stl");

    AsyncFileLoader loader;
    Mesh mesh1, mesh2;

    // First load
    auto future1 = loader.loadAsync((testDir / "test1.stl").string(), mesh1);
    AsyncLoadResult result1 = future1.get();
    EXPECT_TRUE(result1.success);

    // Second load (should work after first completes)
    auto future2 = loader.loadAsync((testDir / "test2.stl").string(), mesh2);
    AsyncLoadResult result2 = future2.get();
    EXPECT_TRUE(result2.success);
}

TEST_F(AsyncFileLoaderTest, MultipleLoads_Concurrent_ReturnsError) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh1, mesh2;

    // Start first load
    auto future1 = loader.loadAsync(filepath.string(), mesh1);

    // Try to start second load while first is in progress
    auto future2 = loader.loadAsync(filepath.string(), mesh2);

    AsyncLoadResult result2 = future2.get();

    // Second load should fail because first is in progress
    EXPECT_FALSE(result2.success);
    EXPECT_NE(result2.message.find("in progress"), std::string::npos);

    // Clean up first load
    future1.get();
}

// ======================================================================
// Wait Tests
// ======================================================================

TEST_F(AsyncFileLoaderTest, Wait_BlocksUntilComplete) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    auto future = loader.loadAsync(filepath.string(), mesh);

    EXPECT_TRUE(loader.isLoading());

    loader.wait();

    EXPECT_FALSE(loader.isLoading());

    // Should be able to get result immediately now
    AsyncLoadResult result = future.get();
    EXPECT_TRUE(result.success);
}

// ======================================================================
// Options Tests
// ======================================================================

TEST_F(AsyncFileLoaderTest, LoadAsync_WithOptions) {
    createSimpleSTL("test.stl");
    auto filepath = testDir / "test.stl";

    AsyncFileLoader loader;
    Mesh mesh;

    ReadOptions options;
    options.strictMode = false;

    auto future = loader.loadAsync(filepath.string(), mesh, options);
    AsyncLoadResult result = future.get();

    EXPECT_TRUE(result.success);
}
