/**
 * @file test_spatial_index_serializer.cpp
 * @brief Unit tests for SpatialIndexSerializer
 */

#include <gtest/gtest.h>
#include "core/SpatialIndexSerializer.h"
#include "core/Mesh.h"
#include "core/Element.h"
#include <Eigen/Dense>
#include <cstdio>

using namespace koomesh::core;

/**
 * @brief Test fixture for SpatialIndexSerializer tests
 */
class SpatialIndexSerializerTest : public ::testing::Test {
protected:
    void SetUp() override {
        createTestMesh();
        testCachePath = "/tmp/test_spatial_index.sidx";
    }

    void TearDown() override {
        // Clean up test cache files
        std::remove(testCachePath.c_str());
    }

    void createTestMesh() {
        mesh = std::make_unique<Mesh>();

        // Create simple 2x2x2 grid
        NodeId nodeId = 1;
        for (int k = 0; k < 3; ++k) {
            for (int j = 0; j < 3; ++j) {
                for (int i = 0; i < 3; ++i) {
                    Node node(nodeId++, Eigen::Vector3d(
                        static_cast<double>(i),
                        static_cast<double>(j),
                        static_cast<double>(k)
                    ));
                    mesh->addNode(node);
                }
            }
        }

        // Create 8 hexahedral elements
        ElementId elemId = 1;
        for (int k = 0; k < 2; ++k) {
            for (int j = 0; j < 2; ++j) {
                for (int i = 0; i < 2; ++i) {
                    NodeId n000 = 1 + i + j * 3 + k * 9;
                    NodeId n100 = n000 + 1;
                    NodeId n010 = n000 + 3;
                    NodeId n110 = n010 + 1;
                    NodeId n001 = n000 + 9;
                    NodeId n101 = n001 + 1;
                    NodeId n011 = n001 + 3;
                    NodeId n111 = n011 + 1;

                    std::vector<NodeId> nodes = {n000, n100, n110, n010, n001, n101, n111, n011};
                    HexahedronElement elem(elemId++, 1, nodes);
                    mesh->addElement(std::make_unique<HexahedronElement>(elem));
                }
            }
        }
    }

    std::unique_ptr<Mesh> mesh;
    std::string testCachePath;
};

// ============================================================================
// Cache Path Generation Tests
// ============================================================================

TEST_F(SpatialIndexSerializerTest, GenerateCachePath_Octree) {
    std::string meshPath = "/path/to/mesh.k";
    std::string cachePath = SpatialIndexSerializer::generateCachePath(
        meshPath,
        SpatialIndexType::OCTREE
    );

    EXPECT_NE(cachePath.find("octree"), std::string::npos);
    EXPECT_NE(cachePath.find(".sidx"), std::string::npos);
}

TEST_F(SpatialIndexSerializerTest, GenerateCachePath_KdTree) {
    std::string meshPath = "/path/to/mesh.dat";
    std::string cachePath = SpatialIndexSerializer::generateCachePath(
        meshPath,
        SpatialIndexType::KDTREE
    );

    EXPECT_NE(cachePath.find("kdtree"), std::string::npos);
    EXPECT_NE(cachePath.find(".sidx"), std::string::npos);
}

TEST_F(SpatialIndexSerializerTest, GenerateCachePath_DifferentTypes) {
    std::string meshPath = "/path/to/mesh.vtk";

    std::string octreePath = SpatialIndexSerializer::generateCachePath(
        meshPath, SpatialIndexType::OCTREE
    );
    std::string kdtreePath = SpatialIndexSerializer::generateCachePath(
        meshPath, SpatialIndexType::KDTREE
    );

    EXPECT_NE(octreePath, kdtreePath);
}

// ============================================================================
// Save and Load Tests
// ============================================================================

TEST_F(SpatialIndexSerializerTest, SaveAndLoad_Octree) {
    // Create index
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    // Save
    SerializationStats saveStats;
    bool saved = SpatialIndexSerializer::save(
        *index,
        *mesh,
        testCachePath,
        SerializationConfig(),
        &saveStats
    );

    ASSERT_TRUE(saved);
    EXPECT_GT(saveStats.serializedSize, 0);
    EXPECT_GT(saveStats.serializationTime, 0.0);

    // Load
    SerializationStats loadStats;
    auto loadedIndex = SpatialIndexSerializer::load(
        testCachePath,
        *mesh,
        SerializationConfig(),
        &loadStats
    );

    ASSERT_NE(loadedIndex, nullptr);
    EXPECT_GT(loadStats.deserializationTime, 0.0);
    EXPECT_TRUE(loadStats.checksumValid);
}

TEST_F(SpatialIndexSerializerTest, SaveAndLoad_WithConfig) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SerializationConfig config;
    config.includeMetadata = true;
    config.validateOnLoad = true;

    bool saved = SpatialIndexSerializer::save(
        *index,
        *mesh,
        testCachePath,
        config
    );

    ASSERT_TRUE(saved);

    auto loadedIndex = SpatialIndexSerializer::load(
        testCachePath,
        *mesh,
        config
    );

    ASSERT_NE(loadedIndex, nullptr);
}

TEST_F(SpatialIndexSerializerTest, Load_NonexistentFile) {
    auto index = SpatialIndexSerializer::load(
        "/nonexistent/path.sidx",
        *mesh
    );

    EXPECT_EQ(index, nullptr);
}

TEST_F(SpatialIndexSerializerTest, Save_InvalidPath) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    bool saved = SpatialIndexSerializer::save(
        *index,
        *mesh,
        "/invalid/path/that/does/not/exist/test.sidx"
    );

    EXPECT_FALSE(saved);
}

// ============================================================================
// Cache Validation Tests
// ============================================================================

TEST_F(SpatialIndexSerializerTest, IsCacheValid_ValidCache) {
    // Create and save index
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SpatialIndexSerializer::save(*index, *mesh, testCachePath);

    // Check validity
    bool valid = SpatialIndexSerializer::isCacheValid(testCachePath, *mesh);
    EXPECT_TRUE(valid);
}

TEST_F(SpatialIndexSerializerTest, IsCacheValid_NonexistentFile) {
    bool valid = SpatialIndexSerializer::isCacheValid(
        "/nonexistent/cache.sidx",
        *mesh
    );

    EXPECT_FALSE(valid);
}

TEST_F(SpatialIndexSerializerTest, IsCacheValid_ModifiedMesh) {
    // Create and save index
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SpatialIndexSerializer::save(*index, *mesh, testCachePath);

    // Modify mesh
    Node newNode(100, Eigen::Vector3d(10.0, 10.0, 10.0));
    mesh->addNode(newNode);

    // Cache should be invalid now
    bool valid = SpatialIndexSerializer::isCacheValid(testCachePath, *mesh);
    EXPECT_FALSE(valid);
}

// ============================================================================
// Cache Management Tests
// ============================================================================

TEST_F(SpatialIndexSerializerTest, GetCacheSize) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SpatialIndexSerializer::save(*index, *mesh, testCachePath);

    size_t size = SpatialIndexSerializer::getCacheSize(testCachePath);
    EXPECT_GT(size, 0);
}

TEST_F(SpatialIndexSerializerTest, GetCacheSize_NonexistentFile) {
    size_t size = SpatialIndexSerializer::getCacheSize("/nonexistent/cache.sidx");
    EXPECT_EQ(size, 0);
}

TEST_F(SpatialIndexSerializerTest, DeleteCache) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SpatialIndexSerializer::save(*index, *mesh, testCachePath);

    ASSERT_TRUE(SpatialIndexSerializer::isCacheValid(testCachePath, *mesh));

    bool deleted = SpatialIndexSerializer::deleteCache(testCachePath);
    EXPECT_TRUE(deleted);

    EXPECT_FALSE(SpatialIndexSerializer::isCacheValid(testCachePath, *mesh));
}

TEST_F(SpatialIndexSerializerTest, IsCacheExpired_Fresh) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SpatialIndexSerializer::save(*index, *mesh, testCachePath);

    // Cache should not be expired after 1 second
    bool expired = SpatialIndexSerializer::isCacheExpired(testCachePath, 3600.0);
    EXPECT_FALSE(expired);
}

TEST_F(SpatialIndexSerializerTest, IsCacheExpired_Old) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SpatialIndexSerializer::save(*index, *mesh, testCachePath);

    // Cache is immediately "expired" if threshold is 0
    bool expired = SpatialIndexSerializer::isCacheExpired(testCachePath, 0.0);
    EXPECT_TRUE(expired);
}

TEST_F(SpatialIndexSerializerTest, IsCacheExpired_NonexistentFile) {
    bool expired = SpatialIndexSerializer::isCacheExpired(
        "/nonexistent/cache.sidx",
        3600.0
    );

    EXPECT_TRUE(expired);  // Non-existent files are considered expired
}

// ============================================================================
// SpatialIndexCache Tests
// ============================================================================

TEST_F(SpatialIndexSerializerTest, Cache_GetOrBuild_NoCachePath) {
    auto index = SpatialIndexCache::getOrBuild(
        SpatialIndexType::OCTREE,
        *mesh
    );

    ASSERT_NE(index, nullptr);
}

TEST_F(SpatialIndexSerializerTest, Cache_GetOrBuild_WithCachePath) {
    // First call - build and cache
    auto index1 = SpatialIndexCache::getOrBuild(
        SpatialIndexType::OCTREE,
        *mesh,
        testCachePath
    );

    ASSERT_NE(index1, nullptr);

    // Second call - load from cache
    auto index2 = SpatialIndexCache::getOrBuild(
        SpatialIndexType::OCTREE,
        *mesh,
        testCachePath
    );

    ASSERT_NE(index2, nullptr);
}

TEST_F(SpatialIndexSerializerTest, Cache_GetOrBuild_InvalidCache) {
    // Create invalid cache file
    std::ofstream out(testCachePath, std::ios::binary);
    out << "invalid data";
    out.close();

    // Should rebuild index
    auto index = SpatialIndexCache::getOrBuild(
        SpatialIndexType::OCTREE,
        *mesh,
        testCachePath
    );

    ASSERT_NE(index, nullptr);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(SpatialIndexSerializerTest, Stats_SerializationTime) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SerializationStats stats;
    SpatialIndexSerializer::save(*index, *mesh, testCachePath, SerializationConfig(), &stats);

    EXPECT_GT(stats.serializationTime, 0.0);
    EXPECT_GT(stats.serializedSize, 0);
}

TEST_F(SpatialIndexSerializerTest, Stats_DeserializationTime) {
    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(*mesh);

    SpatialIndexSerializer::save(*index, *mesh, testCachePath);

    SerializationStats stats;
    SpatialIndexSerializer::load(testCachePath, *mesh, SerializationConfig(), &stats);

    EXPECT_GT(stats.deserializationTime, 0.0);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(SpatialIndexSerializerTest, SaveLoad_EmptyMesh) {
    Mesh emptyMesh;

    auto index = SpatialIndexFactory::create(SpatialIndexType::OCTREE);
    index->build(emptyMesh);

    bool saved = SpatialIndexSerializer::save(*index, emptyMesh, testCachePath);
    EXPECT_TRUE(saved);

    auto loadedIndex = SpatialIndexSerializer::load(testCachePath, emptyMesh);
    EXPECT_NE(loadedIndex, nullptr);
}

TEST_F(SpatialIndexSerializerTest, MultipleIndexTypes) {
    std::vector<SpatialIndexType> types = {
        SpatialIndexType::OCTREE,
        SpatialIndexType::KDTREE,
        SpatialIndexType::RTREE
    };

    for (auto type : types) {
        auto index = SpatialIndexFactory::create(type);
        index->build(*mesh);

        std::string cachePath = testCachePath + std::to_string(static_cast<int>(type));

        bool saved = SpatialIndexSerializer::save(*index, *mesh, cachePath);
        EXPECT_TRUE(saved);

        auto loadedIndex = SpatialIndexSerializer::load(cachePath, *mesh);
        EXPECT_NE(loadedIndex, nullptr);

        std::remove(cachePath.c_str());
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
