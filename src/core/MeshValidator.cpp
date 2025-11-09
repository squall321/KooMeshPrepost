#include "core/MeshValidator.h"
#include <sstream>
#include <unordered_set>
#include <cmath>

namespace koomesh {
namespace core {

// ======================================================================
// 전체 검증
// ======================================================================

MeshValidator::ValidationResult MeshValidator::validate(
    const Mesh& mesh,
    const ValidationOptions& options) {

    ValidationResult result;

    // 기본 정보
    std::ostringstream oss;
    oss << "Validating mesh with " << mesh.nodeCount()
        << " nodes and " << mesh.elementCount() << " elements";
    result.addInfo(oss.str());

    // 노드 참조 검증
    if (options.checkNodeReferences) {
        checkNodeReferences(mesh, result);
    }

    // 연결성 검증
    if (options.checkConnectivity) {
        checkConnectivity(mesh, result);
    }

    // 중복 노드 검증
    if (options.checkDuplicateNodes) {
        checkDuplicateNodes(mesh, result, options.coincidentNodeTolerance);
    }

    // 중복 요소 검증
    if (options.checkDuplicateElements) {
        checkDuplicateElements(mesh, result);
    }

    // 퇴화 요소 검증
    if (options.checkDegenerateElements) {
        checkDegenerateElements(mesh, result);
    }

    // 요소 품질 검증
    if (options.checkElementQuality) {
        checkElementQuality(mesh, result, options.minQualityThreshold);
    }

    // 파트 참조 검증
    if (options.checkPartReferences) {
        checkPartReferences(mesh, result);
    }

    // 바운딩 박스 검증
    if (options.checkBoundingBox) {
        checkBoundingBox(mesh, result);
    }

    // 최종 결과
    if (result.isValid) {
        result.addInfo("Mesh validation completed successfully");
    } else {
        std::ostringstream summary;
        summary << "Mesh validation failed with "
                << result.errorCount() << " errors and "
                << result.warningCount() << " warnings";
        result.addInfo(summary.str());
    }

    return result;
}

// ======================================================================
// 개별 검증 함수
// ======================================================================

bool MeshValidator::checkNodeReferences(const Mesh& mesh, ValidationResult& result) {
    bool valid = true;
    size_t invalidCount = 0;

    // 모든 요소의 노드 참조 확인
    for (size_t i = 0; i < mesh.elementCount(); ++i) {
        const Element* elem = mesh.getElement(i);
        if (!elem) continue;

        for (NodeId nodeId : elem->nodeIds()) {
            const Node* node = mesh.getNode(nodeId);
            if (!node) {
                std::ostringstream oss;
                oss << "Element " << elem->id()
                    << " references non-existent node " << nodeId;
                result.addError(oss.str());
                valid = false;
                ++invalidCount;
            }
        }
    }

    if (invalidCount > 0) {
        std::ostringstream oss;
        oss << "Found " << invalidCount << " invalid node references";
        result.addInfo(oss.str());
    }

    return valid;
}

bool MeshValidator::checkConnectivity(const Mesh& mesh, ValidationResult& result) {
    size_t isolatedNodes = 0;

    // 고립된 노드 찾기 (어떤 요소에도 연결되지 않은 노드)
    for (size_t i = 0; i < mesh.nodeCount(); ++i) {
        const Node* node = mesh.getNode(i);
        if (!node) continue;

        if (node->connectedElements().empty()) {
            ++isolatedNodes;
        }
    }

    if (isolatedNodes > 0) {
        std::ostringstream oss;
        oss << "Found " << isolatedNodes << " isolated nodes (not connected to any element)";
        result.addWarning(oss.str());
    }

    return true;  // 경고만, 오류는 아님
}

bool MeshValidator::checkDuplicateNodes(const Mesh& mesh,
                                       ValidationResult& result,
                                       double tolerance) {
    auto duplicates = findCoincidentNodes(mesh, tolerance);

    if (!duplicates.empty()) {
        std::ostringstream oss;
        oss << "Found " << duplicates.size()
            << " pairs of coincident nodes (tolerance: " << tolerance << ")";
        result.addWarning(oss.str());

        // 처음 몇 개만 상세 출력
        size_t maxReport = std::min(static_cast<size_t>(5), duplicates.size());
        for (size_t i = 0; i < maxReport; ++i) {
            std::ostringstream detail;
            detail << "  Nodes " << duplicates[i].first
                   << " and " << duplicates[i].second << " are coincident";
            result.addInfo(detail.str());
        }
    }

    return true;  // 경고만
}

bool MeshValidator::checkDuplicateElements(const Mesh& mesh, ValidationResult& result) {
    // 간단한 구현: ID 중복만 확인 (Mesh가 이미 처리함)
    // 실제로는 같은 노드 집합을 가진 요소를 찾아야 함

    std::unordered_set<ElementId> seenIds;
    size_t duplicateCount = 0;

    for (size_t i = 0; i < mesh.elementCount(); ++i) {
        const Element* elem = mesh.getElement(i);
        if (!elem) continue;

        if (seenIds.find(elem->id()) != seenIds.end()) {
            std::ostringstream oss;
            oss << "Duplicate element ID: " << elem->id();
            result.addError(oss.str());
            ++duplicateCount;
        }
        seenIds.insert(elem->id());
    }

    return duplicateCount == 0;
}

bool MeshValidator::checkDegenerateElements(const Mesh& mesh, ValidationResult& result) {
    size_t degenerateCount = 0;

    for (size_t i = 0; i < mesh.elementCount(); ++i) {
        const Element* elem = mesh.getElement(i);
        if (!elem) continue;

        // 부피가 0인 요소 찾기
        double volume = elem->computeVolume(mesh);
        if (volume < 1e-12) {
            std::ostringstream oss;
            oss << "Degenerate element " << elem->id()
                << " (volume: " << volume << ")";
            result.addError(oss.str());
            ++degenerateCount;
        }
    }

    if (degenerateCount > 0) {
        std::ostringstream oss;
        oss << "Found " << degenerateCount << " degenerate elements";
        result.addInfo(oss.str());
    }

    return degenerateCount == 0;
}

bool MeshValidator::checkElementQuality(const Mesh& mesh,
                                       ValidationResult& result,
                                       double minQuality) {
    size_t lowQualityCount = 0;
    double minFound = 1.0;

    for (size_t i = 0; i < mesh.elementCount(); ++i) {
        const Element* elem = mesh.getElement(i);
        if (!elem) continue;

        double quality = elem->computeQuality(mesh);
        minFound = std::min(minFound, quality);

        if (quality < minQuality) {
            ++lowQualityCount;

            // 처음 몇 개만 상세 출력
            if (lowQualityCount <= 5) {
                std::ostringstream oss;
                oss << "Element " << elem->id()
                    << " has low quality: " << quality
                    << " (threshold: " << minQuality << ")";
                result.addWarning(oss.str());
            }
        }
    }

    if (lowQualityCount > 0) {
        std::ostringstream oss;
        oss << "Found " << lowQualityCount
            << " elements with quality below " << minQuality
            << " (minimum: " << minFound << ")";
        result.addInfo(oss.str());
    }

    return true;  // 경고만
}

bool MeshValidator::checkPartReferences(const Mesh& mesh, ValidationResult& result) {
    std::unordered_set<PartId> usedParts;

    // 요소에서 사용된 파트 수집
    for (size_t i = 0; i < mesh.elementCount(); ++i) {
        const Element* elem = mesh.getElement(i);
        if (!elem) continue;

        usedParts.insert(elem->partId());
    }

    // 메시에 정의된 파트 확인
    for (PartId partId : usedParts) {
        const Part* part = mesh.getPart(partId);
        if (!part) {
            std::ostringstream oss;
            oss << "Elements reference non-existent part " << partId;
            result.addWarning(oss.str());
        }
    }

    // 사용되지 않는 파트 확인
    size_t unusedParts = 0;
    for (size_t i = 0; i < mesh.partCount(); ++i) {
        const Part* part = mesh.getPart(i);
        if (!part) continue;

        if (usedParts.find(part->id()) == usedParts.end()) {
            ++unusedParts;
        }
    }

    if (unusedParts > 0) {
        std::ostringstream oss;
        oss << "Found " << unusedParts << " unused parts";
        result.addInfo(oss.str());
    }

    return true;
}

bool MeshValidator::checkBoundingBox(const Mesh& mesh, ValidationResult& result) {
    if (mesh.nodeCount() == 0) {
        result.addWarning("Mesh has no nodes");
        return true;
    }

    BoundingBox bbox = mesh.boundingBox();

    // 바운딩 박스 크기 확인
    double dx = bbox.maxX - bbox.minX;
    double dy = bbox.maxY - bbox.minY;
    double dz = bbox.maxZ - bbox.minZ;

    if (dx < 1e-10 || dy < 1e-10 || dz < 1e-10) {
        std::ostringstream oss;
        oss << "Mesh is degenerate (very small bounding box): "
            << "(" << dx << ", " << dy << ", " << dz << ")";
        result.addWarning(oss.str());
    }

    // 바운딩 박스 정보 출력
    std::ostringstream oss;
    oss << "Bounding box: [" << bbox.minX << ", " << bbox.maxX << "] x ["
        << bbox.minY << ", " << bbox.maxY << "] x ["
        << bbox.minZ << ", " << bbox.maxZ << "]";
    result.addInfo(oss.str());

    return true;
}

// ======================================================================
// 헬퍼 함수
// ======================================================================

std::vector<std::pair<NodeId, NodeId>>
MeshValidator::findCoincidentNodes(const Mesh& mesh, double tolerance) {
    std::vector<std::pair<NodeId, NodeId>> duplicates;

    // 간단한 O(n^2) 알고리즘 (실제로는 공간 인덱싱 필요)
    // 대량 데이터에는 부적합하지만, 검증 용도로는 충분

    double tol2 = tolerance * tolerance;

    for (size_t i = 0; i < mesh.nodeCount(); ++i) {
        const Node* node1 = mesh.getNode(i);
        if (!node1) continue;

        for (size_t j = i + 1; j < mesh.nodeCount(); ++j) {
            const Node* node2 = mesh.getNode(j);
            if (!node2) continue;

            double dist2 = (node1->coordinates() - node2->coordinates()).squaredNorm();
            if (dist2 < tol2) {
                duplicates.emplace_back(node1->id(), node2->id());
            }
        }
    }

    return duplicates;
}

} // namespace core
} // namespace koomesh
