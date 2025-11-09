/**
 * @file SpatialIndexSerializer.h
 * @brief Serialization and caching for spatial indexes
 */

#pragma once

#include "core/ISpatialIndex.h"
#include "core/SpatialIndexFactory.h"
#include "core/Mesh.h"
#include <string>
#include <memory>
#include <fstream>
#include <vector>
#include <cstdint>

namespace koomesh {
namespace core {

/**
 * @brief File header for serialized spatial index
 */
struct SpatialIndexHeader {
    uint32_t magic = 0x4B4D5349;  // "KMSI" - KooMesh Spatial Index
    uint32_t version = 1;
    uint32_t indexType = 0;
    uint64_t elementCount = 0;
    uint64_t nodeCount = 0;
    double meshBoundsMinX = 0.0;
    double meshBoundsMinY = 0.0;
    double meshBoundsMinZ = 0.0;
    double meshBoundsMaxX = 0.0;
    double meshBoundsMaxY = 0.0;
    double meshBoundsMaxZ = 0.0;
    uint64_t dataSize = 0;
    uint32_t checksum = 0;
    uint8_t reserved[64] = {0};  // Reserved for future use
};

/**
 * @brief Serialization configuration
 */
struct SerializationConfig {
    bool useCompression = false;
    bool includeMetadata = true;
    bool validateOnLoad = true;
    size_t bufferSize = 1024 * 1024;  // 1MB default buffer
};

/**
 * @brief Statistics from serialization operations
 */
struct SerializationStats {
    size_t originalSize = 0;
    size_t serializedSize = 0;
    double compressionRatio = 1.0;
    double serializationTime = 0.0;
    double deserializationTime = 0.0;
    bool checksumValid = true;
};

/**
 * @brief Utility class for saving and loading spatial indexes
 *
 * This class provides functionality to serialize spatial indexes to disk
 * for caching purposes, avoiding repeated index construction for the same mesh.
 * Supports:
 * - Binary serialization with checksum validation
 * - Optional compression
 * - Metadata validation
 * - Cache management
 */
class SpatialIndexSerializer {
public:
    /**
     * @brief Save spatial index to file
     * @param index Index to save
     * @param mesh Associated mesh (for validation)
     * @param filepath Output file path
     * @param config Serialization configuration
     * @param stats Optional pointer to receive statistics
     * @return True if save successful
     */
    static bool save(
        const ISpatialIndex& index,
        const Mesh& mesh,
        const std::string& filepath,
        const SerializationConfig& config = SerializationConfig(),
        SerializationStats* stats = nullptr
    );

    /**
     * @brief Load spatial index from file
     * @param filepath Input file path
     * @param mesh Associated mesh (for validation)
     * @param config Serialization configuration
     * @param stats Optional pointer to receive statistics
     * @return Loaded index, or nullptr if load failed
     */
    static std::unique_ptr<ISpatialIndex> load(
        const std::string& filepath,
        const Mesh& mesh,
        const SerializationConfig& config = SerializationConfig(),
        SerializationStats* stats = nullptr
    );

    /**
     * @brief Check if cached index exists and is valid
     * @param filepath Cache file path
     * @param mesh Mesh to validate against
     * @return True if valid cache exists
     */
    static bool isCacheValid(
        const std::string& filepath,
        const Mesh& mesh
    );

    /**
     * @brief Generate cache file path for given mesh
     * @param meshPath Path to mesh file
     * @param indexType Type of spatial index
     * @return Suggested cache file path
     */
    static std::string generateCachePath(
        const std::string& meshPath,
        SpatialIndexType indexType
    );

    /**
     * @brief Delete cache file
     * @param filepath Cache file to delete
     * @return True if deletion successful
     */
    static bool deleteCache(const std::string& filepath);

    /**
     * @brief Get cache file size
     * @param filepath Cache file path
     * @return File size in bytes, or 0 if file doesn't exist
     */
    static size_t getCacheSize(const std::string& filepath);

    /**
     * @brief Check if cache file is older than threshold
     * @param filepath Cache file path
     * @param maxAgeSeconds Maximum age in seconds
     * @return True if cache is too old
     */
    static bool isCacheExpired(
        const std::string& filepath,
        double maxAgeSeconds
    );

private:
    /**
     * @brief Write header to file
     */
    static bool writeHeader(
        std::ofstream& out,
        const SpatialIndexHeader& header
    );

    /**
     * @brief Read header from file
     */
    static bool readHeader(
        std::ifstream& in,
        SpatialIndexHeader& header
    );

    /**
     * @brief Create header from mesh and index
     */
    static SpatialIndexHeader createHeader(
        const ISpatialIndex& index,
        const Mesh& mesh,
        SpatialIndexType type
    );

    /**
     * @brief Validate header against mesh
     */
    static bool validateHeader(
        const SpatialIndexHeader& header,
        const Mesh& mesh
    );

    /**
     * @brief Calculate checksum for data
     */
    static uint32_t calculateChecksum(
        const std::vector<uint8_t>& data
    );

    /**
     * @brief Compress data (if compression enabled)
     */
    static std::vector<uint8_t> compress(
        const std::vector<uint8_t>& data
    );

    /**
     * @brief Decompress data
     */
    static std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& data,
        size_t originalSize
    );

    /**
     * @brief Serialize index data
     */
    static std::vector<uint8_t> serializeIndexData(
        const ISpatialIndex& index
    );

    /**
     * @brief Deserialize index data
     */
    static std::unique_ptr<ISpatialIndex> deserializeIndexData(
        const std::vector<uint8_t>& data,
        SpatialIndexType type,
        const Mesh& mesh
    );
};

/**
 * @brief Cache manager for spatial indexes
 */
class SpatialIndexCache {
public:
    /**
     * @brief Get or build index with automatic caching
     * @param type Index type
     * @param mesh Mesh to index
     * @param cachePath Optional cache file path
     * @param config Serialization config
     * @return Spatial index (from cache or newly built)
     */
    static std::unique_ptr<ISpatialIndex> getOrBuild(
        SpatialIndexType type,
        const Mesh& mesh,
        const std::string& cachePath = "",
        const SerializationConfig& config = SerializationConfig()
    );

    /**
     * @brief Clear all expired caches in directory
     * @param directory Cache directory
     * @param maxAgeSeconds Maximum age for cache files
     * @return Number of caches deleted
     */
    static size_t clearExpiredCaches(
        const std::string& directory,
        double maxAgeSeconds
    );

    /**
     * @brief Get total size of all caches in directory
     * @param directory Cache directory
     * @return Total size in bytes
     */
    static size_t getTotalCacheSize(const std::string& directory);
};

} // namespace core
} // namespace koomesh
