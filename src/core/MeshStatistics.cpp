#include "core/MeshStatistics.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace koomesh {
namespace core {

// ======================================================================
// Statistics 헬퍼 메서드
// ======================================================================

std::string MeshStatistics::Statistics::getElementTypeName(ElementType type) {
    switch (type) {
        case ElementType::TETRAHEDRON: return "Tetrahedron";
        case ElementType::HEXAHEDRON: return "Hexahedron";
        case ElementType::PENTAHEDRON: return "Pentahedron (Wedge)";
        case ElementType::PYRAMID: return "Pyramid";
        case ElementType::TRIANGLE: return "Triangle Shell";
        case ElementType::QUADRILATERAL: return "Quadrilateral Shell";
        case ElementType::BEAM: return "Beam";
        case ElementType::UNKNOWN: return "Unknown";
        default: return "Invalid";
    }
}

size_t MeshStatistics::Statistics::getElementCount(ElementType type) const {
    auto it = elementTypeCounts.find(type);
    return (it != elementTypeCounts.end()) ? it->second : 0;
}

std::string MeshStatistics::Statistics::generateReport() const {
    std::ostringstream oss;

    oss << "=================================================\n";
    oss << "           MESH STATISTICS REPORT\n";
    oss << "=================================================\n\n";

    // 기본 카운트
    oss << "BASIC COUNTS:\n";
    oss << "  Total Nodes:    " << totalNodes << "\n";
    oss << "  Total Elements: " << totalElements << "\n";
    oss << "  Total Parts:    " << totalParts << "\n\n";

    // 요소 타입별
    if (!elementTypeCounts.empty()) {
        oss << "ELEMENT TYPES:\n";
        for (const auto& pair : elementTypeCounts) {
            oss << "  " << std::left << std::setw(25)
                << getElementTypeName(pair.first) << ": "
                << std::right << std::setw(8) << pair.second << "\n";
        }
        oss << "\n";
    }

    // 품질 통계
    if (avgElementQuality > 0.0) {
        oss << "ELEMENT QUALITY:\n";
        oss << "  Minimum:        " << std::fixed << std::setprecision(4)
            << minElementQuality << "\n";
        oss << "  Maximum:        " << maxElementQuality << "\n";
        oss << "  Average:        " << avgElementQuality << "\n";
        oss << "  Std Deviation:  " << stdDevElementQuality << "\n\n";
    }

    // 부피 통계
    if (totalVolume > 0.0) {
        oss << "VOLUME STATISTICS:\n";
        oss << "  Total Volume:   " << std::scientific << std::setprecision(6)
            << totalVolume << "\n";
        oss << "  Min Element:    " << minElementVolume << "\n";
        oss << "  Max Element:    " << maxElementVolume << "\n";
        oss << "  Avg Element:    " << avgElementVolume << "\n\n";
    }

    // 바운딩 박스
    oss << "BOUNDING BOX:\n";
    oss << "  X: [" << std::fixed << std::setprecision(3)
        << globalBounds.minX << ", " << globalBounds.maxX << "]  (size: "
        << (globalBounds.maxX - globalBounds.minX) << ")\n";
    oss << "  Y: [" << globalBounds.minY << ", " << globalBounds.maxY << "]  (size: "
        << (globalBounds.maxY - globalBounds.minY) << ")\n";
    oss << "  Z: [" << globalBounds.minZ << ", " << globalBounds.maxZ << "]  (size: "
        << (globalBounds.maxZ - globalBounds.minZ) << ")\n\n";

    // 연결성
    if (avgNodeConnections > 0.0) {
        oss << "NODE CONNECTIVITY:\n";
        oss << "  Min Connections: " << minNodeConnections << "\n";
        oss << "  Max Connections: " << maxNodeConnections << "\n";
        oss << "  Avg Connections: " << std::fixed << std::setprecision(2)
            << avgNodeConnections << "\n\n";
    }

    // 파트별 통계
    if (!partElementCounts.empty()) {
        oss << "PART STATISTICS:\n";
        for (const auto& pair : partElementCounts) {
            oss << "  Part " << std::setw(5) << pair.first << ": "
                << std::setw(8) << pair.second << " elements\n";
        }
        oss << "\n";
    }

    oss << "=================================================\n";

    return oss.str();
}

std::string MeshStatistics::Statistics::summary() const {
    std::ostringstream oss;
    oss << "Mesh: " << totalNodes << " nodes, "
        << totalElements << " elements, "
        << totalParts << " parts";

    if (avgElementQuality > 0.0) {
        oss << " | Quality: " << std::fixed << std::setprecision(3)
            << avgElementQuality;
    }

    return oss.str();
}

// ======================================================================
// 통계 계산
// ======================================================================

MeshStatistics::Statistics MeshStatistics::compute(
    const Mesh& mesh,
    const Options& options) {

    Statistics stats;

    // 기본 카운트
    computeBasicCounts(mesh, stats);

    // 요소 타입별 카운트
    computeElementTypeCounts(mesh, stats);

    // 품질 통계
    if (options.computeQuality) {
        computeQualityStatistics(mesh, stats);
    }

    // 부피 통계
    if (options.computeVolume) {
        computeVolumeStatistics(mesh, stats);
    }

    // 바운딩 박스
    computeBoundingBox(mesh, stats);

    // 연결성 통계
    if (options.computeConnectivity) {
        computeConnectivityStatistics(mesh, stats);
    }

    // 파트별 통계
    if (options.computePartStats) {
        computePartStatistics(mesh, stats);
    }

    return stats;
}

// ======================================================================
// 개별 통계 계산 함수
// ======================================================================

void MeshStatistics::computeBasicCounts(const Mesh& mesh, Statistics& stats) {
    stats.totalNodes = mesh.nodeCount();
    stats.totalElements = mesh.elementCount();
    stats.totalParts = mesh.partCount();
}

void MeshStatistics::computeElementTypeCounts(const Mesh& mesh, Statistics& stats) {
    for (const auto& pair : mesh.elements()) {
        const Element* elem = pair.second.get();
        if (elem) {
            stats.elementTypeCounts[elem->type()]++;
        }
    }
}

void MeshStatistics::computeQualityStatistics(const Mesh& mesh, Statistics& stats) {
    if (mesh.elementCount() == 0) {
        return;
    }

    std::vector<double> qualities;
    qualities.reserve(mesh.elementCount());

    double sum = 0.0;
    stats.minElementQuality = 1.0;
    stats.maxElementQuality = 0.0;

    for (const auto& pair : mesh.elements()) {
        const Element* elem = pair.second.get();
        if (!elem) continue;

        double quality = elem->computeQuality(mesh);
        qualities.push_back(quality);

        sum += quality;
        stats.minElementQuality = std::min(stats.minElementQuality, quality);
        stats.maxElementQuality = std::max(stats.maxElementQuality, quality);
    }

    if (!qualities.empty()) {
        stats.avgElementQuality = sum / qualities.size();
        stats.stdDevElementQuality = computeStandardDeviation(qualities, stats.avgElementQuality);
    }
}

void MeshStatistics::computeVolumeStatistics(const Mesh& mesh, Statistics& stats) {
    if (mesh.elementCount() == 0) {
        return;
    }

    stats.minElementVolume = std::numeric_limits<double>::max();
    stats.maxElementVolume = 0.0;
    stats.totalVolume = 0.0;

    size_t count = 0;

    for (const auto& pair : mesh.elements()) {
        const Element* elem = pair.second.get();
        if (!elem) continue;

        double volume = elem->computeVolume(mesh);
        if (volume > 0.0) {
            stats.totalVolume += volume;
            stats.minElementVolume = std::min(stats.minElementVolume, volume);
            stats.maxElementVolume = std::max(stats.maxElementVolume, volume);
            ++count;
        }
    }

    if (count > 0) {
        stats.avgElementVolume = stats.totalVolume / count;
    }

    if (stats.minElementVolume > std::numeric_limits<double>::max() / 2.0) {
        stats.minElementVolume = 0.0;
    }
}

void MeshStatistics::computeBoundingBox(const Mesh& mesh, Statistics& stats) {
    stats.globalBounds = mesh.boundingBox();
}

void MeshStatistics::computeConnectivityStatistics(const Mesh& mesh, Statistics& stats) {
    if (mesh.nodeCount() == 0) {
        return;
    }

    stats.minNodeConnections = std::numeric_limits<size_t>::max();
    stats.maxNodeConnections = 0;

    size_t totalConnections = 0;
    size_t nodeCount = 0;

    for (const auto& pair : mesh.nodes()) {
        const Node& node = pair.second;

        size_t connections = node.connectedElements().size();
        totalConnections += connections;
        ++nodeCount;

        stats.minNodeConnections = std::min(stats.minNodeConnections, connections);
        stats.maxNodeConnections = std::max(stats.maxNodeConnections, connections);
    }

    if (nodeCount > 0) {
        stats.avgNodeConnections = static_cast<double>(totalConnections) / nodeCount;
    }

    if (stats.minNodeConnections == std::numeric_limits<size_t>::max()) {
        stats.minNodeConnections = 0;
    }
}

void MeshStatistics::computePartStatistics(const Mesh& mesh, Statistics& stats) {
    for (const auto& pair : mesh.elements()) {
        const Element* elem = pair.second.get();
        if (elem) {
            stats.partElementCounts[elem->partId()]++;
        }
    }
}

double MeshStatistics::computeStandardDeviation(
    const std::vector<double>& values,
    double mean) {

    if (values.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (double value : values) {
        double diff = value - mean;
        sum += diff * diff;
    }

    return std::sqrt(sum / values.size());
}

} // namespace core
} // namespace koomesh
