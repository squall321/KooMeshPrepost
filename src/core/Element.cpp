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

void BoundingBox::expand(const BoundingBox& other) {
    minX = std::min(minX, other.minX);
    minY = std::min(minY, other.minY);
    minZ = std::min(minZ, other.minZ);
    maxX = std::max(maxX, other.maxX);
    maxY = std::max(maxY, other.maxY);
    maxZ = std::max(maxZ, other.maxZ);
}

double BoundingBox::distanceSquared(const Eigen::Vector3d& point) const {
    double distSq = 0.0;

    if (point.x() < minX) {
        double d = minX - point.x();
        distSq += d * d;
    } else if (point.x() > maxX) {
        double d = point.x() - maxX;
        distSq += d * d;
    }

    if (point.y() < minY) {
        double d = minY - point.y();
        distSq += d * d;
    } else if (point.y() > maxY) {
        double d = point.y() - maxY;
        distSq += d * d;
    }

    if (point.z() < minZ) {
        double d = minZ - point.z();
        distSq += d * d;
    } else if (point.z() > maxZ) {
        double d = point.z() - maxZ;
        distSq += d * d;
    }

    return distSq;
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
// PentahedronElement 구현 (6-node wedge)
// ======================================================================

PentahedronElement::PentahedronElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : Element(id, partId, nodeIds) {
    if (nodeIds.size() != 6) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INVALID_DATA,
            "Pentahedron element must have exactly 6 nodes",
            "PentahedronElement"
        );
    }
}

double PentahedronElement::computeVolume(const Mesh& mesh) const {
    // Wedge를 3개의 사면체로 분할하여 부피 계산
    // Wedge nodes: 0,1,2 (bottom triangle), 3,4,5 (top triangle)
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);
    const Node* n3 = mesh.getNode(m_nodeIds[3]);
    const Node* n4 = mesh.getNode(m_nodeIds[4]);
    const Node* n5 = mesh.getNode(m_nodeIds[5]);

    if (!n0 || !n1 || !n2 || !n3 || !n4 || !n5) {
        return 0.0;
    }

    // 3개의 사면체로 분할
    auto tetVolume = [](const Eigen::Vector3d& v0, const Eigen::Vector3d& v1,
                        const Eigen::Vector3d& v2, const Eigen::Vector3d& v3) {
        Eigen::Vector3d e1 = v1 - v0;
        Eigen::Vector3d e2 = v2 - v0;
        Eigen::Vector3d e3 = v3 - v0;
        return std::abs(e1.dot(e2.cross(e3))) / 6.0;
    };

    double vol1 = tetVolume(n0->coordinates(), n1->coordinates(), n2->coordinates(), n3->coordinates());
    double vol2 = tetVolume(n1->coordinates(), n2->coordinates(), n3->coordinates(), n4->coordinates());
    double vol3 = tetVolume(n2->coordinates(), n3->coordinates(), n4->coordinates(), n5->coordinates());

    return vol1 + vol2 + vol3;
}

double PentahedronElement::computeQuality(const Mesh& mesh) const {
    // 간단한 품질: 부피 기반
    double volume = computeVolume(mesh);
    if (volume < 1e-12) {
        return 0.0;
    }

    // BoundingBox 부피와의 비율
    BoundingBox bbox = computeBoundingBox(mesh);
    double bboxVolume = bbox.volume();

    if (bboxVolume < 1e-12) {
        return 0.0;
    }

    return std::min(1.0, volume / bboxVolume);
}

bool PentahedronElement::containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const {
    // 간단한 구현: AABB 체크
    BoundingBox bbox = computeBoundingBox(mesh);
    return bbox.contains(point);
}

// ======================================================================
// PyramidElement 구현 (5-node pyramid)
// ======================================================================

PyramidElement::PyramidElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : Element(id, partId, nodeIds) {
    if (nodeIds.size() != 5) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INVALID_DATA,
            "Pyramid element must have exactly 5 nodes",
            "PyramidElement"
        );
    }
}

double PyramidElement::computeVolume(const Mesh& mesh) const {
    // Pyramid: base (0,1,2,3), apex (4)
    // Volume = (base_area * height) / 3
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);
    const Node* n3 = mesh.getNode(m_nodeIds[3]);
    const Node* n4 = mesh.getNode(m_nodeIds[4]);

    if (!n0 || !n1 || !n2 || !n3 || !n4) {
        return 0.0;
    }

    // 2개의 사면체로 분할
    auto tetVolume = [](const Eigen::Vector3d& v0, const Eigen::Vector3d& v1,
                        const Eigen::Vector3d& v2, const Eigen::Vector3d& v3) {
        Eigen::Vector3d e1 = v1 - v0;
        Eigen::Vector3d e2 = v2 - v0;
        Eigen::Vector3d e3 = v3 - v0;
        return std::abs(e1.dot(e2.cross(e3))) / 6.0;
    };

    double vol1 = tetVolume(n0->coordinates(), n1->coordinates(), n2->coordinates(), n4->coordinates());
    double vol2 = tetVolume(n0->coordinates(), n2->coordinates(), n3->coordinates(), n4->coordinates());

    return vol1 + vol2;
}

double PyramidElement::computeQuality(const Mesh& mesh) const {
    double volume = computeVolume(mesh);
    if (volume < 1e-12) {
        return 0.0;
    }

    BoundingBox bbox = computeBoundingBox(mesh);
    double bboxVolume = bbox.volume();

    if (bboxVolume < 1e-12) {
        return 0.0;
    }

    return std::min(1.0, volume / bboxVolume);
}

bool PyramidElement::containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const {
    BoundingBox bbox = computeBoundingBox(mesh);
    return bbox.contains(point);
}

// ======================================================================
// TriangleElement 구현 (3-node shell)
// ======================================================================

TriangleElement::TriangleElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : Element(id, partId, nodeIds) {
    if (nodeIds.size() != 3) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INVALID_DATA,
            "Triangle element must have exactly 3 nodes",
            "TriangleElement"
        );
    }
}

double TriangleElement::computeArea(const Mesh& mesh) const {
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);

    if (!n0 || !n1 || !n2) {
        return 0.0;
    }

    Eigen::Vector3d v0 = n0->coordinates();
    Eigen::Vector3d v1 = n1->coordinates();
    Eigen::Vector3d v2 = n2->coordinates();

    Eigen::Vector3d edge1 = v1 - v0;
    Eigen::Vector3d edge2 = v2 - v0;

    // Area = |edge1 × edge2| / 2
    return edge1.cross(edge2).norm() * 0.5;
}

Eigen::Vector3d TriangleElement::computeNormal(const Mesh& mesh) const {
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);

    if (!n0 || !n1 || !n2) {
        return Eigen::Vector3d::Zero();
    }

    Eigen::Vector3d v0 = n0->coordinates();
    Eigen::Vector3d v1 = n1->coordinates();
    Eigen::Vector3d v2 = n2->coordinates();

    Eigen::Vector3d edge1 = v1 - v0;
    Eigen::Vector3d edge2 = v2 - v0;

    Eigen::Vector3d normal = edge1.cross(edge2);
    double norm = normal.norm();

    if (norm < 1e-12) {
        return Eigen::Vector3d::Zero();
    }

    return normal / norm;
}

double TriangleElement::computeVolume(const Mesh& mesh) const {
    // Shell 요소는 부피가 아닌 면적 반환
    return computeArea(mesh);
}

double TriangleElement::computeQuality(const Mesh& mesh) const {
    // 품질 = (실제 면적) / (정삼각형 면적)
    double area = computeArea(mesh);
    if (area < 1e-12) {
        return 0.0;
    }

    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);

    if (!n0 || !n1 || !n2) {
        return 0.0;
    }

    // 변 길이
    double a = n0->distanceTo(*n1);
    double b = n1->distanceTo(*n2);
    double c = n2->distanceTo(*n0);

    double maxEdge = std::max({a, b, c});
    if (maxEdge < 1e-12) {
        return 0.0;
    }

    // 정삼각형 면적 = sqrt(3)/4 * edge^2
    double idealArea = 0.433012702 * maxEdge * maxEdge;

    return std::min(1.0, area / idealArea);
}

bool TriangleElement::containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const {
    // 2D 투영으로 내부 판정 (간단한 구현)
    BoundingBox bbox = computeBoundingBox(mesh);
    return bbox.contains(point);
}

// ======================================================================
// QuadrilateralElement 구현 (4-node shell)
// ======================================================================

QuadrilateralElement::QuadrilateralElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : Element(id, partId, nodeIds) {
    if (nodeIds.size() != 4) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INVALID_DATA,
            "Quadrilateral element must have exactly 4 nodes",
            "QuadrilateralElement"
        );
    }
}

double QuadrilateralElement::computeArea(const Mesh& mesh) const {
    // 사각형을 2개의 삼각형으로 분할
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

    // Triangle 1: 0-1-2
    Eigen::Vector3d edge1 = v1 - v0;
    Eigen::Vector3d edge2 = v2 - v0;
    double area1 = edge1.cross(edge2).norm() * 0.5;

    // Triangle 2: 0-2-3
    Eigen::Vector3d edge3 = v2 - v0;
    Eigen::Vector3d edge4 = v3 - v0;
    double area2 = edge3.cross(edge4).norm() * 0.5;

    return area1 + area2;
}

Eigen::Vector3d QuadrilateralElement::computeNormal(const Mesh& mesh) const {
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);

    if (!n0 || !n1 || !n2) {
        return Eigen::Vector3d::Zero();
    }

    Eigen::Vector3d v0 = n0->coordinates();
    Eigen::Vector3d v1 = n1->coordinates();
    Eigen::Vector3d v2 = n2->coordinates();

    Eigen::Vector3d edge1 = v1 - v0;
    Eigen::Vector3d edge2 = v2 - v0;

    Eigen::Vector3d normal = edge1.cross(edge2);
    double norm = normal.norm();

    if (norm < 1e-12) {
        return Eigen::Vector3d::Zero();
    }

    return normal / norm;
}

double QuadrilateralElement::computeVolume(const Mesh& mesh) const {
    // Shell 요소는 부피가 아닌 면적 반환
    return computeArea(mesh);
}

double QuadrilateralElement::computeQuality(const Mesh& mesh) const {
    // 간단한 품질: 최소 변 / 최대 변
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);
    const Node* n2 = mesh.getNode(m_nodeIds[2]);
    const Node* n3 = mesh.getNode(m_nodeIds[3]);

    if (!n0 || !n1 || !n2 || !n3) {
        return 0.0;
    }

    double e0 = n0->distanceTo(*n1);
    double e1 = n1->distanceTo(*n2);
    double e2 = n2->distanceTo(*n3);
    double e3 = n3->distanceTo(*n0);

    double minEdge = std::min({e0, e1, e2, e3});
    double maxEdge = std::max({e0, e1, e2, e3});

    if (maxEdge < 1e-12) {
        return 0.0;
    }

    return minEdge / maxEdge;
}

bool QuadrilateralElement::containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const {
    BoundingBox bbox = computeBoundingBox(mesh);
    return bbox.contains(point);
}

// ======================================================================
// BeamElement 구현 (2-node beam)
// ======================================================================

BeamElement::BeamElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds)
    : Element(id, partId, nodeIds) {
    if (nodeIds.size() != 2) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INVALID_DATA,
            "Beam element must have exactly 2 nodes",
            "BeamElement"
        );
    }
}

double BeamElement::computeLength(const Mesh& mesh) const {
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);

    if (!n0 || !n1) {
        return 0.0;
    }

    return n0->distanceTo(*n1);
}

Eigen::Vector3d BeamElement::computeDirection(const Mesh& mesh) const {
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);

    if (!n0 || !n1) {
        return Eigen::Vector3d::Zero();
    }

    Eigen::Vector3d dir = n1->coordinates() - n0->coordinates();
    double norm = dir.norm();

    if (norm < 1e-12) {
        return Eigen::Vector3d::Zero();
    }

    return dir / norm;
}

double BeamElement::computeVolume(const Mesh& mesh) const {
    // Beam 요소는 부피가 아닌 길이 반환
    return computeLength(mesh);
}

double BeamElement::computeQuality(const Mesh& mesh) const {
    // Beam은 항상 품질 1.0 (두 점을 잇는 선분)
    double length = computeLength(mesh);
    return (length > 1e-12) ? 1.0 : 0.0;
}

bool BeamElement::containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const {
    const Node* n0 = mesh.getNode(m_nodeIds[0]);
    const Node* n1 = mesh.getNode(m_nodeIds[1]);

    if (!n0 || !n1) {
        return false;
    }

    Eigen::Vector3d v0 = n0->coordinates();
    Eigen::Vector3d v1 = n1->coordinates();

    // 선분과 점 사이의 거리 계산
    Eigen::Vector3d v = v1 - v0;
    Eigen::Vector3d w = point - v0;

    double c1 = w.dot(v);
    if (c1 <= 0) {
        return false;  // point is before v0
    }

    double c2 = v.dot(v);
    if (c1 >= c2) {
        return false;  // point is after v1
    }

    // 점이 선분 위에 있는지 확인 (tolerance)
    double t = c1 / c2;
    Eigen::Vector3d projection = v0 + t * v;
    double distance = (point - projection).norm();

    return distance < 1e-6;  // tolerance
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

        case ElementType::PENTAHEDRON:
            return std::make_unique<PentahedronElement>(id, partId, nodeIds);

        case ElementType::PYRAMID:
            return std::make_unique<PyramidElement>(id, partId, nodeIds);

        case ElementType::TRIANGLE:
            return std::make_unique<TriangleElement>(id, partId, nodeIds);

        case ElementType::QUADRILATERAL:
            return std::make_unique<QuadrilateralElement>(id, partId, nodeIds);

        case ElementType::BEAM:
            return std::make_unique<BeamElement>(id, partId, nodeIds);

        case ElementType::UNKNOWN:
        default:
            KOOMESH_THROW_RUNTIME(
                utils::ErrorCode::NOT_IMPLEMENTED,
                "Unknown or unsupported element type: " +
                std::to_string(static_cast<int>(type))
            );
            return nullptr;  // Never reached, but silences compiler warning
    }
}

} // namespace core
} // namespace koomesh
