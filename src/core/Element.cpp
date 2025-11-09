#include "core/Element.h"
#include "core/Mesh.h"
#include "utils/Exception.h"
#include <algorithm>
#include <limits>

namespace koomesh {
namespace core {

// ======================================================================
// BoundingBox 구현
// ======================================================================

bool BoundingBox::intersects(const BoundingBox& other) const {
    return !(maxX < other.minX || minX > other.maxX ||
             maxY < other.minY || minY > other.maxY ||
             maxZ < other.minZ || minZ > other.maxZ);
}

bool BoundingBox::contains(const Eigen::Vector3d& point) const {
    return point.x() >= minX && point.x() <= maxX &&
           point.y() >= minY && point.y() <= maxY &&
           point.z() >= minZ && point.z() <= maxZ;
}

void BoundingBox::expand(const Eigen::Vector3d& point) {
    minX = std::min(minX, point.x());
    minY = std::min(minY, point.y());
    minZ = std::min(minZ, point.z());
    maxX = std::max(maxX, point.x());
    maxY = std::max(maxY, point.y());
    maxZ = std::max(maxZ, point.z());
}

Eigen::Vector3d BoundingBox::center() const {
    return Eigen::Vector3d(
        (minX + maxX) * 0.5,
        (minY + maxY) * 0.5,
        (minZ + maxZ) * 0.5
    );
}

double BoundingBox::volume() const {
    double dx = maxX - minX;
    double dy = maxY - minY;
    double dz = maxZ - minZ;
    return dx * dy * dz;
}

// ======================================================================
// Element 기본 클래스 구현
// ======================================================================

Element::Element(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : m_id(id)
    , m_partId(partId)
    , m_nodeIds(nodeIds) {
}

BoundingBox Element::computeBoundingBox(const Mesh& mesh) const {
    BoundingBox bbox;

    if (m_nodeIds.empty()) {
        return bbox;
    }

    // 첫 번째 노드로 초기화
    const Node* firstNode = mesh.getNode(m_nodeIds[0]);
    if (!firstNode) {
        return bbox;
    }

    const Eigen::Vector3d& firstPos = firstNode->coordinates();
    bbox.minX = bbox.maxX = firstPos.x();
    bbox.minY = bbox.maxY = firstPos.y();
    bbox.minZ = bbox.maxZ = firstPos.z();

    // 나머지 노드로 확장
    for (size_t i = 1; i < m_nodeIds.size(); ++i) {
        const Node* node = mesh.getNode(m_nodeIds[i]);
        if (node) {
            bbox.expand(node->coordinates());
        }
    }

    return bbox;
}

Eigen::Vector3d Element::computeCenter(const Mesh& mesh) const {
    Eigen::Vector3d center(0.0, 0.0, 0.0);

    if (m_nodeIds.empty()) {
        return center;
    }

    for (NodeId nodeId : m_nodeIds) {
        const Node* node = mesh.getNode(nodeId);
        if (node) {
            center += node->coordinates();
        }
    }

    center /= static_cast<double>(m_nodeIds.size());
    return center;
}

// ======================================================================
// TetrahedronElement 구현
// ======================================================================

TetrahedronElement::TetrahedronElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : Element(id, partId, nodeIds) {
    if (nodeIds.size() != 4) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INVALID_DATA,
            "Tetrahedron element must have exactly 4 nodes",
            "TetrahedronElement"
        );
    }
}

double TetrahedronElement::computeVolume(const Mesh& mesh) const {
    // 사면체 부피 = |det(v1-v0, v2-v0, v3-v0)| / 6
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);
    const Node* n3 = mesh.getNode(m_nodeIds[3]);

    if (!n0 || !n1 || !n2 || !n3) {
        return 0.0;
    }

    Eigen::Vector3d v0 = n0->coordinates();
    Eigen::Vector3d v1 = n1->coordinates();
    Eigen::Vector3d v2 = n2->coordinates();
    Eigen::Vector3d v3 = n3->coordinates();

    Eigen::Vector3d e1 = v1 - v0;
    Eigen::Vector3d e2 = v2 - v0;
    Eigen::Vector3d e3 = v3 - v0;

    // 행렬식 계산
    double det = e1.dot(e2.cross(e3));

    return std::abs(det) / 6.0;
}

double TetrahedronElement::computeQuality(const Mesh& mesh) const {
    // 간단한 품질 지표: 부피 / (평균 edge 길이)^3
    // 정사면체의 경우 최대값 1.0에 가까워짐

    double volume = computeVolume(mesh);
    if (volume < 1e-12) {
        return 0.0;
    }

    // 평균 edge 길이 계산
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);
    const Node* n3 = mesh.getNode(m_nodeIds[3]);

    if (!n0 || !n1 || !n2 || !n3) {
        return 0.0;
    }

    double edgeLength = 0.0;
    edgeLength += n0->distanceTo(*n1);
    edgeLength += n0->distanceTo(*n2);
    edgeLength += n0->distanceTo(*n3);
    edgeLength += n1->distanceTo(*n2);
    edgeLength += n1->distanceTo(*n3);
    edgeLength += n2->distanceTo(*n3);
    edgeLength /= 6.0;

    double normalizedVolume = volume / (edgeLength * edgeLength * edgeLength);

    // 정사면체의 정규화된 부피는 약 0.1178
    // 이를 1.0으로 스케일링
    double quality = normalizedVolume / 0.1178;

    return std::min(1.0, std::max(0.0, quality));
}

bool TetrahedronElement::containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const {
    // Barycentric coordinates를 사용한 내부 판정
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);
    const Node* n3 = mesh.getNode(m_nodeIds[3]);

    if (!n0 || !n1 || !n2 || !n3) {
        return false;
    }

    Eigen::Vector3d v0 = n0->coordinates();
    Eigen::Vector3d v1 = n1->coordinates();
    Eigen::Vector3d v2 = n2->coordinates();
    Eigen::Vector3d v3 = n3->coordinates();

    // 각 면에 대해 점이 같은 쪽에 있는지 확인
    auto sameSide = [](const Eigen::Vector3d& p1, const Eigen::Vector3d& p2,
                       const Eigen::Vector3d& a, const Eigen::Vector3d& b) {
        Eigen::Vector3d cp1 = (b - a).cross(p1 - a);
        Eigen::Vector3d cp2 = (b - a).cross(p2 - a);
        return cp1.dot(cp2) >= 0;
    };

    // 4개의 면 체크
    return sameSide(point, v3, v0, v1) &&
           sameSide(point, v3, v1, v2) &&
           sameSide(point, v3, v2, v0) &&
           sameSide(point, v0, v1, v2);
}

// ======================================================================
// HexahedronElement 구현
// ======================================================================

HexahedronElement::HexahedronElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : Element(id, partId, nodeIds) {
    if (nodeIds.size() != 8) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INVALID_DATA,
            "Hexahedron element must have exactly 8 nodes",
            "HexahedronElement"
        );
    }
}

double HexahedronElement::computeVolume(const Mesh& mesh) const {
    // 육면체를 5개의 사면체로 분할하여 부피 계산
    // 간단한 구현: AABB 부피로 근사
    BoundingBox bbox = computeBoundingBox(mesh);
    return bbox.volume();
}

double HexahedronElement::computeQuality(const Mesh& mesh) const {
    // 간단한 품질 지표: 최소 edge / 최대 edge 비율
    std::vector<double> edgeLengths;

    // 모든 edge 길이 계산 (12개)
    const std::vector<std::pair<int, int>> edges = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, // bottom face
        {4, 5}, {5, 6}, {6, 7}, {7, 4}, // top face
        {0, 4}, {1, 5}, {2, 6}, {3, 7}  // vertical edges
    };

    for (const auto& edge : edges) {
        const Node* n1 = mesh.getNode(m_nodeIds[edge.first]);
        const Node* n2 = mesh.getNode(m_nodeIds[edge.second]);

        if (n1 && n2) {
            edgeLengths.push_back(n1->distanceTo(*n2));
        }
    }

    if (edgeLengths.empty()) {
        return 0.0;
    }

    double minEdge = *std::min_element(edgeLengths.begin(), edgeLengths.end());
    double maxEdge = *std::max_element(edgeLengths.begin(), edgeLengths.end());

    if (maxEdge < 1e-12) {
        return 0.0;
    }

    return minEdge / maxEdge;
}

bool HexahedronElement::containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const {
    // 간단한 구현: AABB 체크
    BoundingBox bbox = computeBoundingBox(mesh);
    return bbox.contains(point);
}

// ======================================================================
// ElementFactory 구현
// ======================================================================

std::unique_ptr<Element> ElementFactory::create(
    ElementType type,
    ElementId id,
    PartId partId,
    const std::vector<NodeId>& nodeIds) {

    switch (type) {
        case ElementType::TETRAHEDRON:
            return std::make_unique<TetrahedronElement>(id, partId, nodeIds);

        case ElementType::HEXAHEDRON:
            return std::make_unique<HexahedronElement>(id, partId, nodeIds);

        // 추가 요소 타입들은 향후 구현
        case ElementType::PENTAHEDRON:
        case ElementType::PYRAMID:
        case ElementType::TRIANGLE:
        case ElementType::QUADRILATERAL:
        case ElementType::BEAM:
        case ElementType::UNKNOWN:
        default:
            KOOMESH_THROW_RUNTIME(
                utils::ErrorCode::NOT_IMPLEMENTED,
                "Element type not yet implemented"
            );
    }
}

} // namespace core
} // namespace koomesh
