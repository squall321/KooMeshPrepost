/**
 * @file SpatialIndexSerializer.cpp
 * @brief Implementation of spatial index serialization
 */

#include "core/SpatialIndexSerializer.h"
#include <chrono>
#include <cstring>
#include <sys/stat.h>
#include <algorithm>

namespace koomesh {
namespace core {

// ============================================================================
// SpatialIndexSerializer Implementation
// ============================================================================

bool SpatialIndexSerializer::save(
    const ISpatialIndex& index,
    const Mesh& mesh,
    const std::string& filepath,
    const SerializationConfig& config,
    SerializationStats* stats
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        return false;
    }

    // Determine index type (for now, use placeholder)
    // Note: ISpatialIndex doesn't have a getType() method
    // This should be added to the interface
    SpatialIndexType type = SpatialIndexType::OCTREE;  // Placeholder

    // Create header
    auto header = createHeader(index, mesh, type);

    // Serialize index data (placeholder implementation)
    std::vector<uint8_t> data = serializeIndexData(index);

    if (config.useCompression) {
        data = compress(data);
    }

    header.dataSize = data.size();
    header.checksum = calculateChecksum(data);

    // Write header
    if (!writeHeader(out, header)) {
        return false;
    }

    // Write data
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out) {
        return false;
    }

    auto endTime = std::chrono::high_resolution_clock::now();

    if (stats) {
        stats->originalSize = data.size();
        stats->serializedSize = sizeof(SpatialIndexHeader) + data.size();
        stats->serializationTime = std::chrono::duration<double>(endTime - startTime).count();
        if (stats->originalSize > 0) {
            stats->compressionRatio = static_cast<double>(stats->serializedSize) /
                                     static_cast<double>(stats->originalSize);
        }
    }

    return true;
}

std::unique_ptr<ISpatialIndex> SpatialIndexSerializer::load(
    const std::string& filepath,
    const Mesh& mesh,
    const SerializationConfig& config,
    SerializationStats* stats
) {
    auto startTime = std::chrono::high_resolution_clock::now();

    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        return nullptr;
    }

    // Read header
    SpatialIndexHeader header;
    if (!readHeader(in, header)) {
        return nullptr;
    }

    // Validate magic number
    if (header.magic != 0x4B4D5349) {
        return nullptr;
    }

    // Validate version
    if (header.version > 1) {
        return nullptr;  // Unsupported version
    }

    // Validate against mesh if requested
    if (config.validateOnLoad && !validateHeader(header, mesh)) {
        return nullptr;
    }

    // Read data
    std::vector<uint8_t> data(header.dataSize);
    in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(header.dataSize));
    if (!in) {
        return nullptr;
    }

    // Validate checksum
    uint32_t checksum = calculateChecksum(data);
    if (checksum != header.checksum) {
        if (stats) {
            stats->checksumValid = false;
        }
        if (config.validateOnLoad) {
            return nullptr;
        }
    }

    // Decompress if needed
    if (config.useCompression) {
        data = decompress(data, header.dataSize);
    }

    // Deserialize index
    auto indexType = static_cast<SpatialIndexType>(header.indexType);
    auto index = deserializeIndexData(data, indexType, mesh);

    auto endTime = std::chrono::high_resolution_clock::now();

    if (stats) {
        stats->serializedSize = sizeof(SpatialIndexHeader) + header.dataSize;
        stats->deserializationTime = std::chrono::duration<double>(endTime - startTime).count();
    }

    return index;
}

bool SpatialIndexSerializer::isCacheValid(
    const std::string& filepath,
    const Mesh& mesh
) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        return false;
    }

    SpatialIndexHeader header;
    if (!readHeader(in, header)) {
        return false;
    }

    return validateHeader(header, mesh);
}

std::string SpatialIndexSerializer::generateCachePath(
    const std::string& meshPath,
    SpatialIndexType indexType
) {
    std::string cachePath = meshPath;

    // Replace extension with .cache
    size_t dotPos = cachePath.find_last_of('.');
    if (dotPos != std::string::npos) {
        cachePath = cachePath.substr(0, dotPos);
    }

    // Append index type
    const char* indexTypeName = "";
    switch (indexType) {
        case SpatialIndexType::OCTREE: indexTypeName = "octree"; break;
        case SpatialIndexType::RTREE: indexTypeName = "rtree"; break;
        case SpatialIndexType::KDTREE: indexTypeName = "kdtree"; break;
        case SpatialIndexType::UNIFORM_GRID: indexTypeName = "uniformgrid"; break;
    }

    cachePath += ".";
    cachePath += indexTypeName;
    cachePath += ".sidx";  // Spatial Index cache

    return cachePath;
}

bool SpatialIndexSerializer::deleteCache(const std::string& filepath) {
    return std::remove(filepath.c_str()) == 0;
}

size_t SpatialIndexSerializer::getCacheSize(const std::string& filepath) {
    struct stat st;
    if (stat(filepath.c_str(), &st) == 0) {
        return static_cast<size_t>(st.st_size);
    }
    return 0;
}

bool SpatialIndexSerializer::isCacheExpired(
    const std::string& filepath,
    double maxAgeSeconds
) {
    struct stat st;
    if (stat(filepath.c_str(), &st) != 0) {
        return true;  // File doesn't exist
    }

    auto now = std::chrono::system_clock::now();
    auto fileTime = std::chrono::system_clock::from_time_t(st.st_mtime);
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - fileTime).count();

    return static_cast<double>(age) > maxAgeSeconds;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool SpatialIndexSerializer::writeHeader(
    std::ofstream& out,
    const SpatialIndexHeader& header
) {
    out.write(reinterpret_cast<const char*>(&header), sizeof(SpatialIndexHeader));
    return out.good();
}

bool SpatialIndexSerializer::readHeader(
    std::ifstream& in,
    SpatialIndexHeader& header
) {
    in.read(reinterpret_cast<char*>(&header), sizeof(SpatialIndexHeader));
    return in.good();
}

SpatialIndexHeader SpatialIndexSerializer::createHeader(
    const ISpatialIndex& index,
    const Mesh& mesh,
    SpatialIndexType type
) {
    SpatialIndexHeader header;

    header.magic = 0x4B4D5349;
    header.version = 1;
    header.indexType = static_cast<uint32_t>(type);
    header.elementCount = mesh.elementCount();
    header.nodeCount = mesh.nodeCount();

    auto bounds = mesh.boundingBox();
    header.meshBoundsMinX = bounds.minX;
    header.meshBoundsMinY = bounds.minY;
    header.meshBoundsMinZ = bounds.minZ;
    header.meshBoundsMaxX = bounds.maxX;
    header.meshBoundsMaxY = bounds.maxY;
    header.meshBoundsMaxZ = bounds.maxZ;

    return header;
}

bool SpatialIndexSerializer::validateHeader(
    const SpatialIndexHeader& header,
    const Mesh& mesh
) {
    // Check element and node counts
    if (header.elementCount != mesh.elementCount()) {
        return false;
    }

    if (header.nodeCount != mesh.nodeCount()) {
        return false;
    }

    // Check bounding box (with tolerance)
    const double tolerance = 1e-6;
    auto bounds = mesh.boundingBox();

    if (std::abs(header.meshBoundsMinX - bounds.minX) > tolerance ||
        std::abs(header.meshBoundsMinY - bounds.minY) > tolerance ||
        std::abs(header.meshBoundsMinZ - bounds.minZ) > tolerance ||
        std::abs(header.meshBoundsMaxX - bounds.maxX) > tolerance ||
        std::abs(header.meshBoundsMaxY - bounds.maxY) > tolerance ||
        std::abs(header.meshBoundsMaxZ - bounds.maxZ) > tolerance) {
        return false;
    }

    return true;
}

uint32_t SpatialIndexSerializer::calculateChecksum(
    const std::vector<uint8_t>& data
) {
    // Simple CRC32-like checksum
    uint32_t checksum = 0xFFFFFFFF;

    for (uint8_t byte : data) {
        checksum ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (checksum & 1) {
                checksum = (checksum >> 1) ^ 0xEDB88320;
            } else {
                checksum >>= 1;
            }
        }
    }

    return ~checksum;
}

std::vector<uint8_t> SpatialIndexSerializer::compress(
    const std::vector<uint8_t>& data
) {
    // Placeholder: Compression not implemented yet
    // TODO: Implement compression (e.g., zlib)
    return data;
}

std::vector<uint8_t> SpatialIndexSerializer::decompress(
    const std::vector<uint8_t>& data,
    size_t originalSize
) {
    // Placeholder: Decompression not implemented yet
    // TODO: Implement decompression
    (void)originalSize;  // Unused
    return data;
}

std::vector<uint8_t> SpatialIndexSerializer::serializeIndexData(
    const ISpatialIndex& index
) {
    // Placeholder implementation
    // Note: Actual serialization requires ISpatialIndex to provide
    // serialization methods or access to internal data structures

    // For now, return empty data
    // TODO: Implement proper serialization for each index type
    (void)index;  // Unused
    std::vector<uint8_t> data;
    return data;
}

std::unique_ptr<ISpatialIndex> SpatialIndexSerializer::deserializeIndexData(
    const std::vector<uint8_t>& data,
    SpatialIndexType type,
    const Mesh& mesh
) {
    // Placeholder implementation
    // For now, rebuild the index from scratch
    (void)data;  // Unused

    auto index = SpatialIndexFactory::create(type);
    index->build(mesh);

    return index;
}

// ============================================================================
// SpatialIndexCache Implementation
// ============================================================================

std::unique_ptr<ISpatialIndex> SpatialIndexCache::getOrBuild(
    SpatialIndexType type,
    const Mesh& mesh,
    const std::string& cachePath,
    const SerializationConfig& config
) {
    // If no cache path specified, just build
    if (cachePath.empty()) {
        auto index = SpatialIndexFactory::create(type);
        index->build(mesh);
        return index;
    }

    // Try to load from cache
    if (SpatialIndexSerializer::isCacheValid(cachePath, mesh)) {
        auto index = SpatialIndexSerializer::load(cachePath, mesh, config);
        if (index) {
            return index;
        }
    }

    // Build and save to cache
    auto index = SpatialIndexFactory::create(type);
    index->build(mesh);

    SpatialIndexSerializer::save(*index, mesh, cachePath, config);

    return index;
}

size_t SpatialIndexCache::clearExpiredCaches(
    const std::string& directory,
    double maxAgeSeconds
) {
    // Placeholder: Directory scanning not implemented
    // TODO: Implement directory scanning and cache cleanup
    (void)directory;
    (void)maxAgeSeconds;
    return 0;
}

size_t SpatialIndexCache::getTotalCacheSize(const std::string& directory) {
    // Placeholder: Directory scanning not implemented
    // TODO: Implement directory scanning
    (void)directory;
    return 0;
}

} // namespace core
} // namespace koomesh
