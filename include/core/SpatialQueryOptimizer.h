#pragma once

#include "core/ISpatialIndex.h"
#include "core/Mesh.h"
#include "core/SpatialIndexFactory.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace koomesh {
namespace core {

/**
 * @brief Query pattern statistics for optimization
 */
struct QueryPattern {
    size_t boundingBoxQueries = 0;
    size_t pointQueries = 0;
    size_t nearestNeighborQueries = 0;
    size_t kNearestQueries = 0;
    size_t radiusQueries = 0;
    size_t rayQueries = 0;

    double avgBoundingBoxSize = 0.0;
    double avgRadiusSize = 0.0;
    size_t avgKValue = 0;

    void reset() {
        boundingBoxQueries = 0;
        pointQueries = 0;
        nearestNeighborQueries = 0;
        kNearestQueries = 0;
        radiusQueries = 0;
        rayQueries = 0;
        avgBoundingBoxSize = 0.0;
        avgRadiusSize = 0.0;
        avgKValue = 0;
    }
};

/**
 * @brief Cache entry for query results
 */
struct CacheEntry {
    std::vector<ElementId> results;
    std::chrono::steady_clock::time_point timestamp;
    size_t hitCount = 0;
};

/**
 * @brief Optimizer for spatial queries with caching and pattern analysis
 *
 * This class provides intelligent query optimization by:
 * - Analyzing query patterns to recommend optimal index types
 * - Caching frequently used query results
 * - Adapting index selection based on workload
 * - Providing performance recommendations
 *
 * Example usage:
 * @code
 * SpatialQueryOptimizer optimizer;
 * optimizer.setMesh(mesh);
 *
 * // Enable caching
 * optimizer.enableCache(1000, 60.0);  // 1000 entries, 60 second TTL
 *
 * // Use optimizer for queries
 * auto results = optimizer.queryBoundingBox(bbox);
 *
 * // Get recommendations
 * auto indexType = optimizer.recommendIndexType();
 * auto report = optimizer.getOptimizationReport();
 * @endcode
 */
class SpatialQueryOptimizer {
public:
    /**
     * @brief Constructor
     */
    SpatialQueryOptimizer();

    /**
     * @brief Destructor
     */
    ~SpatialQueryOptimizer() = default;

    /**
     * @brief Set the mesh and build initial index
     * @param mesh The mesh to optimize queries for
     * @param initialType Initial index type to use
     */
    void setMesh(const Mesh& mesh, SpatialIndexType initialType = SpatialIndexType::OCTREE);

    /**
     * @brief Enable query result caching
     * @param maxEntries Maximum number of cache entries
     * @param ttlSeconds Time-to-live for cache entries in seconds
     */
    void enableCache(size_t maxEntries = 1000, double ttlSeconds = 60.0);

    /**
     * @brief Disable query result caching
     */
    void disableCache();

    /**
     * @brief Clear all cached results
     */
    void clearCache();

    /**
     * @brief Query bounding box (with caching and pattern tracking)
     * @param box Bounding box to query
     * @return Element IDs intersecting the box
     */
    std::vector<ElementId> queryBoundingBox(const BoundingBox& box);

    /**
     * @brief Query point (with caching and pattern tracking)
     * @param point Point to query
     * @return Element IDs containing the point
     */
    std::vector<ElementId> queryPoint(const Eigen::Vector3d& point);

    /**
     * @brief Find nearest neighbor (with pattern tracking)
     * @param point Query point
     * @return Nearest element ID
     */
    ElementId findNearest(const Eigen::Vector3d& point);

    /**
     * @brief Find k nearest neighbors (with pattern tracking)
     * @param point Query point
     * @param k Number of neighbors
     * @return K nearest element IDs
     */
    std::vector<ElementId> findKNearest(const Eigen::Vector3d& point, size_t k);

    /**
     * @brief Find elements within radius (with caching and pattern tracking)
     * @param point Center point
     * @param radius Search radius
     * @return Element IDs within radius
     */
    std::vector<ElementId> findWithinRadius(const Eigen::Vector3d& point, double radius);

    /**
     * @brief Recommend optimal index type based on query patterns
     * @return Recommended spatial index type
     */
    SpatialIndexType recommendIndexType() const;

    /**
     * @brief Adapt index type based on current query patterns
     *
     * Analyzes recent query patterns and rebuilds index with optimal type
     * if current type is suboptimal.
     */
    void adaptIndexType();

    /**
     * @brief Get query pattern statistics
     * @return Query pattern information
     */
    const QueryPattern& getQueryPattern() const { return m_queryPattern; }

    /**
     * @brief Reset query pattern statistics
     */
    void resetQueryPattern() { m_queryPattern.reset(); }

    /**
     * @brief Get optimization report
     * @return Human-readable optimization report
     */
    std::string getOptimizationReport() const;

    /**
     * @brief Get cache statistics
     * @return String with cache hit rate and other stats
     */
    std::string getCacheStatistics() const;

    /**
     * @brief Get current index type
     * @return Current spatial index type
     */
    SpatialIndexType getCurrentIndexType() const { return m_currentIndexType; }

private:
    /**
     * @brief Generate cache key for bounding box query
     */
    std::string generateCacheKey(const BoundingBox& box) const;

    /**
     * @brief Generate cache key for point query
     */
    std::string generateCacheKey(const Eigen::Vector3d& point) const;

    /**
     * @brief Generate cache key for radius query
     */
    std::string generateCacheKey(const Eigen::Vector3d& point, double radius) const;

    /**
     * @brief Check if cache entry is still valid
     */
    bool isCacheValid(const CacheEntry& entry) const;

    /**
     * @brief Evict oldest cache entries if cache is full
     */
    void evictOldestCacheEntries();

    /**
     * @brief Update cache entry or create new one
     */
    void updateCache(const std::string& key, const std::vector<ElementId>& results);

    /**
     * @brief Try to get results from cache
     * @return True if found in cache, false otherwise
     */
    bool tryGetFromCache(const std::string& key, std::vector<ElementId>& results);

    const Mesh* m_mesh;
    std::unique_ptr<ISpatialIndex> m_index;
    SpatialIndexType m_currentIndexType;

    QueryPattern m_queryPattern;

    // Cache settings
    bool m_cacheEnabled;
    size_t m_maxCacheEntries;
    double m_cacheTTL;  // Time-to-live in seconds
    std::map<std::string, CacheEntry> m_cache;
    size_t m_cacheHits;
    size_t m_cacheMisses;

    // Adaptation settings
    size_t m_queriesSinceLastAdaptation;
    size_t m_adaptationThreshold;  // Adapt after this many queries
};

} // namespace core
} // namespace koomesh
