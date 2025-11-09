#include "core/SpatialQueryOptimizer.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace koomesh {
namespace core {

SpatialQueryOptimizer::SpatialQueryOptimizer()
    : m_mesh(nullptr)
    , m_currentIndexType(SpatialIndexType::OCTREE)
    , m_cacheEnabled(false)
    , m_maxCacheEntries(1000)
    , m_cacheTTL(60.0)
    , m_cacheHits(0)
    , m_cacheMisses(0)
    , m_queriesSinceLastAdaptation(0)
    , m_adaptationThreshold(100)
{
}

void SpatialQueryOptimizer::setMesh(const Mesh& mesh, SpatialIndexType initialType) {
    m_mesh = &mesh;
    m_currentIndexType = initialType;

    // Build initial index
    m_index = SpatialIndexFactory::create(initialType);
    m_index->build(mesh);

    // Reset statistics
    m_queryPattern.reset();
    clearCache();
    m_cacheHits = 0;
    m_cacheMisses = 0;
    m_queriesSinceLastAdaptation = 0;
}

void SpatialQueryOptimizer::enableCache(size_t maxEntries, double ttlSeconds) {
    m_cacheEnabled = true;
    m_maxCacheEntries = maxEntries;
    m_cacheTTL = ttlSeconds;
    clearCache();
}

void SpatialQueryOptimizer::disableCache() {
    m_cacheEnabled = false;
    clearCache();
}

void SpatialQueryOptimizer::clearCache() {
    m_cache.clear();
}

std::vector<ElementId> SpatialQueryOptimizer::queryBoundingBox(const BoundingBox& box) {
    if (!m_index) {
        return {};
    }

    std::vector<ElementId> results;

    // Try cache first
    if (m_cacheEnabled) {
        std::string key = generateCacheKey(box);
        if (tryGetFromCache(key, results)) {
            return results;
        }
    }

    // Perform query
    results = m_index->query(box);

    // Update cache
    if (m_cacheEnabled) {
        std::string key = generateCacheKey(box);
        updateCache(key, results);
    }

    // Update statistics
    m_queryPattern.boundingBoxQueries++;
    Eigen::Vector3d size = box.max() - box.min();
    double boxSize = size.norm();
    m_queryPattern.avgBoundingBoxSize =
        (m_queryPattern.avgBoundingBoxSize * (m_queryPattern.boundingBoxQueries - 1) + boxSize) /
        m_queryPattern.boundingBoxQueries;

    m_queriesSinceLastAdaptation++;

    return results;
}

std::vector<ElementId> SpatialQueryOptimizer::queryPoint(const Eigen::Vector3d& point) {
    if (!m_index) {
        return {};
    }

    std::vector<ElementId> results;

    // Try cache first
    if (m_cacheEnabled) {
        std::string key = generateCacheKey(point);
        if (tryGetFromCache(key, results)) {
            return results;
        }
    }

    // Perform query
    results = m_index->queryPoint(point);

    // Update cache
    if (m_cacheEnabled) {
        std::string key = generateCacheKey(point);
        updateCache(key, results);
    }

    // Update statistics
    m_queryPattern.pointQueries++;
    m_queriesSinceLastAdaptation++;

    return results;
}

ElementId SpatialQueryOptimizer::findNearest(const Eigen::Vector3d& point) {
    if (!m_index) {
        return 0;
    }

    ElementId result = m_index->findNearest(point);

    // Update statistics
    m_queryPattern.nearestNeighborQueries++;
    m_queriesSinceLastAdaptation++;

    return result;
}

std::vector<ElementId> SpatialQueryOptimizer::findKNearest(const Eigen::Vector3d& point, size_t k) {
    if (!m_index) {
        return {};
    }

    std::vector<ElementId> results = m_index->findKNearest(point, k);

    // Update statistics
    m_queryPattern.kNearestQueries++;
    m_queryPattern.avgKValue =
        (m_queryPattern.avgKValue * (m_queryPattern.kNearestQueries - 1) + k) /
        m_queryPattern.kNearestQueries;
    m_queriesSinceLastAdaptation++;

    return results;
}

std::vector<ElementId> SpatialQueryOptimizer::findWithinRadius(const Eigen::Vector3d& point, double radius) {
    if (!m_index) {
        return {};
    }

    std::vector<ElementId> results;

    // Try cache first
    if (m_cacheEnabled) {
        std::string key = generateCacheKey(point, radius);
        if (tryGetFromCache(key, results)) {
            return results;
        }
    }

    // Perform query
    results = m_index->findWithinRadius(point, radius);

    // Update cache
    if (m_cacheEnabled) {
        std::string key = generateCacheKey(point, radius);
        updateCache(key, results);
    }

    // Update statistics
    m_queryPattern.radiusQueries++;
    m_queryPattern.avgRadiusSize =
        (m_queryPattern.avgRadiusSize * (m_queryPattern.radiusQueries - 1) + radius) /
        m_queryPattern.radiusQueries;
    m_queriesSinceLastAdaptation++;

    return results;
}

SpatialIndexType SpatialQueryOptimizer::recommendIndexType() const {
    size_t totalQueries = m_queryPattern.boundingBoxQueries +
                         m_queryPattern.pointQueries +
                         m_queryPattern.nearestNeighborQueries +
                         m_queryPattern.kNearestQueries +
                         m_queryPattern.radiusQueries +
                         m_queryPattern.rayQueries;

    if (totalQueries == 0) {
        return SpatialIndexType::OCTREE;  // Default
    }

    // Calculate query type percentages
    double nnPercent = static_cast<double>(m_queryPattern.nearestNeighborQueries +
                                          m_queryPattern.kNearestQueries) / totalQueries;
    double rangePercent = static_cast<double>(m_queryPattern.boundingBoxQueries +
                                             m_queryPattern.radiusQueries) / totalQueries;
    double pointPercent = static_cast<double>(m_queryPattern.pointQueries) / totalQueries;

    // Recommendation heuristics:

    // 1. If dominated by point queries (>60%), use Uniform Grid
    if (pointPercent > 0.6) {
        return SpatialIndexType::UNIFORM_GRID;
    }

    // 2. If dominated by nearest neighbor queries (>50%), use K-d Tree
    if (nnPercent > 0.5) {
        return SpatialIndexType::KDTREE;
    }

    // 3. If dominated by range queries (>50%), use R-Tree
    if (rangePercent > 0.5) {
        return SpatialIndexType::RTREE;
    }

    // 4. If very large mesh and mixed queries, use Uniform Grid
    if (m_mesh && m_mesh->elementCount() > 10000) {
        return SpatialIndexType::UNIFORM_GRID;
    }

    // 5. Default to Octree for balanced workloads
    return SpatialIndexType::OCTREE;
}

void SpatialQueryOptimizer::adaptIndexType() {
    if (!m_mesh || m_queriesSinceLastAdaptation < m_adaptationThreshold) {
        return;
    }

    SpatialIndexType recommendedType = recommendIndexType();

    // Only rebuild if recommendation differs from current
    if (recommendedType != m_currentIndexType) {
        m_currentIndexType = recommendedType;
        m_index = SpatialIndexFactory::create(recommendedType);
        m_index->build(*m_mesh);

        // Clear cache as index changed
        clearCache();
    }

    m_queriesSinceLastAdaptation = 0;
}

std::string SpatialQueryOptimizer::getOptimizationReport() const {
    std::ostringstream oss;

    oss << "\n";
    oss << "================================================================\n";
    oss << " Spatial Query Optimizer Report\n";
    oss << "================================================================\n\n";

    // Current state
    oss << "Current Index Type: " << SpatialIndexFactory::typeToString(m_currentIndexType) << "\n";
    if (m_mesh) {
        oss << "Mesh Size: " << m_mesh->elementCount() << " elements\n";
    }
    oss << "\n";

    // Query pattern statistics
    size_t totalQueries = m_queryPattern.boundingBoxQueries +
                         m_queryPattern.pointQueries +
                         m_queryPattern.nearestNeighborQueries +
                         m_queryPattern.kNearestQueries +
                         m_queryPattern.radiusQueries +
                         m_queryPattern.rayQueries;

    oss << "Query Pattern Statistics:\n";
    oss << "  Total Queries: " << totalQueries << "\n";
    if (totalQueries > 0) {
        oss << "  Bounding Box:     " << std::setw(6) << m_queryPattern.boundingBoxQueries
            << " (" << std::fixed << std::setprecision(1)
            << (100.0 * m_queryPattern.boundingBoxQueries / totalQueries) << "%)\n";
        oss << "  Point Query:      " << std::setw(6) << m_queryPattern.pointQueries
            << " (" << std::fixed << std::setprecision(1)
            << (100.0 * m_queryPattern.pointQueries / totalQueries) << "%)\n";
        oss << "  Nearest Neighbor: " << std::setw(6) << m_queryPattern.nearestNeighborQueries
            << " (" << std::fixed << std::setprecision(1)
            << (100.0 * m_queryPattern.nearestNeighborQueries / totalQueries) << "%)\n";
        oss << "  K-Nearest:        " << std::setw(6) << m_queryPattern.kNearestQueries
            << " (" << std::fixed << std::setprecision(1)
            << (100.0 * m_queryPattern.kNearestQueries / totalQueries) << "%)\n";
        oss << "  Radius Search:    " << std::setw(6) << m_queryPattern.radiusQueries
            << " (" << std::fixed << std::setprecision(1)
            << (100.0 * m_queryPattern.radiusQueries / totalQueries) << "%)\n";
        oss << "  Ray Intersection: " << std::setw(6) << m_queryPattern.rayQueries
            << " (" << std::fixed << std::setprecision(1)
            << (100.0 * m_queryPattern.rayQueries / totalQueries) << "%)\n";
    }
    oss << "\n";

    // Query characteristics
    if (m_queryPattern.boundingBoxQueries > 0) {
        oss << "  Avg Bounding Box Size: " << std::fixed << std::setprecision(4)
            << m_queryPattern.avgBoundingBoxSize << "\n";
    }
    if (m_queryPattern.radiusQueries > 0) {
        oss << "  Avg Radius Size:       " << std::fixed << std::setprecision(4)
            << m_queryPattern.avgRadiusSize << "\n";
    }
    if (m_queryPattern.kNearestQueries > 0) {
        oss << "  Avg K Value:           " << std::fixed << std::setprecision(1)
            << m_queryPattern.avgKValue << "\n";
    }
    oss << "\n";

    // Cache statistics
    if (m_cacheEnabled) {
        oss << getCacheStatistics();
    } else {
        oss << "Cache: Disabled\n";
    }
    oss << "\n";

    // Recommendation
    SpatialIndexType recommended = recommendIndexType();
    oss << "Recommended Index Type: " << SpatialIndexFactory::typeToString(recommended) << "\n";
    if (recommended != m_currentIndexType) {
        oss << "  ⚠️  Consider switching from "
            << SpatialIndexFactory::typeToString(m_currentIndexType)
            << " to " << SpatialIndexFactory::typeToString(recommended) << "\n";
    } else {
        oss << "  ✓ Current index type is optimal for workload\n";
    }

    oss << "\n";
    oss << "================================================================\n";

    return oss.str();
}

std::string SpatialQueryOptimizer::getCacheStatistics() const {
    std::ostringstream oss;

    size_t totalCacheQueries = m_cacheHits + m_cacheMisses;
    double hitRate = totalCacheQueries > 0 ?
        (100.0 * m_cacheHits / totalCacheQueries) : 0.0;

    oss << "Cache Statistics:\n";
    oss << "  Status:         " << (m_cacheEnabled ? "Enabled" : "Disabled") << "\n";
    if (m_cacheEnabled) {
        oss << "  Entries:        " << m_cache.size() << " / " << m_maxCacheEntries << "\n";
        oss << "  TTL:            " << std::fixed << std::setprecision(1) << m_cacheTTL << " seconds\n";
        oss << "  Hits:           " << m_cacheHits << "\n";
        oss << "  Misses:         " << m_cacheMisses << "\n";
        oss << "  Hit Rate:       " << std::fixed << std::setprecision(2) << hitRate << "%\n";
    }

    return oss.str();
}

std::string SpatialQueryOptimizer::generateCacheKey(const BoundingBox& box) const {
    std::ostringstream oss;
    Eigen::Vector3d minPt = box.min();
    Eigen::Vector3d maxPt = box.max();

    oss << "bb_" << std::fixed << std::setprecision(6)
        << minPt.x() << "_" << minPt.y() << "_" << minPt.z() << "_"
        << maxPt.x() << "_" << maxPt.y() << "_" << maxPt.z();

    return oss.str();
}

std::string SpatialQueryOptimizer::generateCacheKey(const Eigen::Vector3d& point) const {
    std::ostringstream oss;
    oss << "pt_" << std::fixed << std::setprecision(6)
        << point.x() << "_" << point.y() << "_" << point.z();
    return oss.str();
}

std::string SpatialQueryOptimizer::generateCacheKey(const Eigen::Vector3d& point, double radius) const {
    std::ostringstream oss;
    oss << "rad_" << std::fixed << std::setprecision(6)
        << point.x() << "_" << point.y() << "_" << point.z() << "_" << radius;
    return oss.str();
}

bool SpatialQueryOptimizer::isCacheValid(const CacheEntry& entry) const {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - entry.timestamp;
    return elapsed.count() < m_cacheTTL;
}

void SpatialQueryOptimizer::evictOldestCacheEntries() {
    if (m_cache.size() <= m_maxCacheEntries) {
        return;
    }

    // Evict entries until we're under the limit
    size_t toEvict = m_cache.size() - m_maxCacheEntries;

    // Find oldest entries (or least used)
    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point>> entries;
    for (const auto& pair : m_cache) {
        entries.push_back({pair.first, pair.second.timestamp});
    }

    // Sort by timestamp (oldest first)
    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

    // Evict oldest entries
    for (size_t i = 0; i < toEvict && i < entries.size(); ++i) {
        m_cache.erase(entries[i].first);
    }
}

void SpatialQueryOptimizer::updateCache(const std::string& key, const std::vector<ElementId>& results) {
    if (!m_cacheEnabled) {
        return;
    }

    CacheEntry entry;
    entry.results = results;
    entry.timestamp = std::chrono::steady_clock::now();
    entry.hitCount = 0;

    m_cache[key] = entry;

    // Evict if necessary
    evictOldestCacheEntries();
}

bool SpatialQueryOptimizer::tryGetFromCache(const std::string& key, std::vector<ElementId>& results) {
    if (!m_cacheEnabled) {
        m_cacheMisses++;
        return false;
    }

    auto it = m_cache.find(key);
    if (it == m_cache.end()) {
        m_cacheMisses++;
        return false;
    }

    // Check if entry is still valid
    if (!isCacheValid(it->second)) {
        m_cache.erase(it);
        m_cacheMisses++;
        return false;
    }

    // Cache hit
    results = it->second.results;
    it->second.hitCount++;
    it->second.timestamp = std::chrono::steady_clock::now();  // Update timestamp
    m_cacheHits++;
    return true;
}

} // namespace core
} // namespace koomesh
