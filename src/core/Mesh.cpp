#include "core/Mesh.h"
#include "selection/ISpatialIndex.h"
#include "utils/Exception.h"
#include "utils/Logger.h"
#include <algorithm>
#include <limits>

namespace koomesh {
namespace core {

// ======================================================================
// 생성자 및 소멸자
// ======================================================================

Mesh::Mesh()
    : m_spatialIndex(nullptr)
    , m_cachedBoundingBox(nullptr) {
}

Mesh::~Mesh() = default;

// ======================================================================
// Commands (상태 변경)
// ======================================================================

void Mesh::addNode(const Node& node) {
    NodeId id = node.id();

    // 중복 ID 체크
    if (m_nodes.find(id) != m_nodes.end()) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::DUPLICATE_ID,
            "Node with ID " + std::to_string(id) + " already exists",
            "NodeID=" + std::to_string(id)
        );
    }

    m_nodes.emplace(id, node);
    invalidateBoundingBoxCache();
}

void Mesh::addNodesBatch(const std::vector<Node>& nodes) {
    // 배치 추가로 성능 향상
    m_nodes.reserve(m_nodes.size() + nodes.size());

    for (const Node& node : nodes) {
        NodeId id = node.id();

        // 중복 ID는 경고만 출력하고 건너뛰기
        if (m_nodes.find(id) != m_nodes.end()) {
            utils::Logger::warningf("Duplicate node ID {} skipped", id);
            continue;
        }

        m_nodes.emplace(id, node);
    }

    invalidateBoundingBoxCache();
}

void Mesh::addElement(std::unique_ptr<Element> element) {
    if (!element) {
        KOOMESH_THROW(
            utils::RuntimeException,
            utils::ErrorCode::NULL_POINTER,
            "Cannot add null element"
        );
    }

    ElementId id = element->id();

    // 중복 ID 체크
    if (m_elements.find(id) != m_elements.end()) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::DUPLICATE_ID,
            "Element with ID " + std::to_string(id) + " already exists",
            "ElementID=" + std::to_string(id)
        );
    }

    // 노드 ID 유효성 체크
    const std::vector<NodeId>& nodeIds = element->nodeIds();
    for (NodeId nodeId : nodeIds) {
        if (m_nodes.find(nodeId) == m_nodes.end()) {
            KOOMESH_THROW_INVALID_DATA(
                utils::ErrorCode::INVALID_NODE_ID,
                "Node ID " + std::to_string(nodeId) + " not found in mesh",
                "ElementID=" + std::to_string(id)
            );
        }

        // 노드에 요소 연결 정보 추가
        m_nodes.at(nodeId).addConnectedElement(id);
    }

    m_elements.emplace(id, std::move(element));
    invalidateBoundingBoxCache();
}

void Mesh::addElementsBatch(std::vector<std::unique_ptr<Element>> elements) {
    // 배치 추가
    for (auto& element : elements) {
        if (!element) {
            utils::Logger::warning("Null element in batch, skipping");
            continue;
        }

        ElementId id = element->id();

        // 중복 ID는 경고만 출력하고 건너뛰기
        if (m_elements.find(id) != m_elements.end()) {
            utils::Logger::warningf("Duplicate element ID {} skipped", id);
            continue;
        }

        // 노드 ID 유효성 체크 (경고만)
        const std::vector<NodeId>& nodeIds = element->nodeIds();
        bool validNodes = true;
        for (NodeId nodeId : nodeIds) {
            if (m_nodes.find(nodeId) == m_nodes.end()) {
                utils::Logger::warningf("Element {} references non-existent node {}", id, nodeId);
                validNodes = false;
                break;
            }
        }

        if (!validNodes) {
            continue;
        }

        // 노드에 요소 연결 정보 추가
        for (NodeId nodeId : nodeIds) {
            m_nodes.at(nodeId).addConnectedElement(id);
        }

        m_elements.emplace(id, std::move(element));
    }

    invalidateBoundingBoxCache();
}

void Mesh::addPart(const Part& part) {
    PartId id = part.id();

    // 중복 ID 체크
    if (m_parts.find(id) != m_parts.end()) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::DUPLICATE_ID,
            "Part with ID " + std::to_string(id) + " already exists",
            "PartID=" + std::to_string(id)
        );
    }

    m_parts.emplace(id, part);
}

bool Mesh::removeNode(NodeId id) {
    auto it = m_nodes.find(id);
    if (it == m_nodes.end()) {
        return false;
    }

    // 연결된 요소가 있으면 제거 불가
    const std::vector<ElementId>& connectedElements = it->second.connectedElements();
    if (!connectedElements.empty()) {
        KOOMESH_THROW_INVALID_DATA(
            utils::ErrorCode::INCONSISTENT_DATA,
            "Cannot remove node with connected elements",
            "NodeID=" + std::to_string(id)
        );
    }

    m_nodes.erase(it);
    invalidateBoundingBoxCache();
    return true;
}

bool Mesh::removeElement(ElementId id) {
    auto it = m_elements.find(id);
    if (it == m_elements.end()) {
        return false;
    }

    // 노드에서 요소 연결 정보 제거
    const std::vector<NodeId>& nodeIds = it->second->nodeIds();
    for (NodeId nodeId : nodeIds) {
        auto nodeIt = m_nodes.find(nodeId);
        if (nodeIt != m_nodes.end()) {
            // Note: Node 클래스에 removeConnectedElement 메서드가 없으므로
            // 현재는 연결 정보가 남아있음 (향후 개선 필요)
        }
    }

    m_elements.erase(it);
    invalidateBoundingBoxCache();
    return true;
}

void Mesh::clear() {
    m_nodes.clear();
    m_elements.clear();
    m_parts.clear();
    m_spatialIndex.reset();
    invalidateBoundingBoxCache();
}

void Mesh::buildSpatialIndex() {
    // 공간 인덱스 구축 (향후 구현)
    // m_spatialIndex = std::make_unique<Octree>();
    // m_spatialIndex->build(m_elements);
    utils::Logger::warning("Spatial index building not yet implemented");
}

// ======================================================================
// Queries (조회)
// ======================================================================

const Node* Mesh::getNode(NodeId id) const {
    auto it = m_nodes.find(id);
    return (it != m_nodes.end()) ? &it->second : nullptr;
}

Node* Mesh::getNode(NodeId id) {
    auto it = m_nodes.find(id);
    return (it != m_nodes.end()) ? &it->second : nullptr;
}

const Element* Mesh::getElement(ElementId id) const {
    auto it = m_elements.find(id);
    return (it != m_elements.end()) ? it->second.get() : nullptr;
}

Element* Mesh::getElement(ElementId id) {
    auto it = m_elements.find(id);
    return (it != m_elements.end()) ? it->second.get() : nullptr;
}

const Part* Mesh::getPart(PartId id) const {
    auto it = m_parts.find(id);
    return (it != m_parts.end()) ? &it->second : nullptr;
}

Part* Mesh::getPart(PartId id) {
    auto it = m_parts.find(id);
    return (it != m_parts.end()) ? &it->second : nullptr;
}

std::vector<NodeId> Mesh::getAllNodeIds() const {
    std::vector<NodeId> ids;
    ids.reserve(m_nodes.size());

    for (const auto& pair : m_nodes) {
        ids.push_back(pair.first);
    }

    return ids;
}

std::vector<ElementId> Mesh::getAllElementIds() const {
    std::vector<ElementId> ids;
    ids.reserve(m_elements.size());

    for (const auto& pair : m_elements) {
        ids.push_back(pair.first);
    }

    return ids;
}

BoundingBox Mesh::boundingBox() const {
    if (!m_cachedBoundingBox) {
        m_cachedBoundingBox = std::make_unique<BoundingBox>(computeBoundingBox());
    }
    return *m_cachedBoundingBox;
}

std::vector<ElementId> Mesh::findElementsInBox(const BoundingBox& box) const {
    std::vector<ElementId> result;

    // 공간 인덱스가 있으면 사용
    if (m_spatialIndex) {
        // return m_spatialIndex->findInBox(box);
    }

    // 없으면 brute-force 검색
    for (const auto& pair : m_elements) {
        BoundingBox elemBox = pair.second->computeBoundingBox(*this);
        if (elemBox.intersects(box)) {
            result.push_back(pair.first);
        }
    }

    return result;
}

ElementId Mesh::findNearestElement(const Eigen::Vector3d& point) const {
    ElementId nearestId = 0;
    double minDistance = std::numeric_limits<double>::max();

    for (const auto& pair : m_elements) {
        Eigen::Vector3d center = pair.second->computeCenter(*this);
        double distance = (center - point).norm();

        if (distance < minDistance) {
            minDistance = distance;
            nearestId = pair.first;
        }
    }

    return nearestId;
}

NodeId Mesh::findNearestNode(const Eigen::Vector3d& point) const {
    NodeId nearestId = 0;
    double minDistance = std::numeric_limits<double>::max();

    for (const auto& pair : m_nodes) {
        double distance = (pair.second.coordinates() - point).norm();

        if (distance < minDistance) {
            minDistance = distance;
            nearestId = pair.first;
        }
    }

    return nearestId;
}

std::unordered_map<ElementType, size_t> Mesh::getElementTypeStatistics() const {
    std::unordered_map<ElementType, size_t> stats;

    for (const auto& pair : m_elements) {
        ElementType type = pair.second->type();
        stats[type]++;
    }

    return stats;
}

// ======================================================================
// Private 메서드
// ======================================================================

BoundingBox Mesh::computeBoundingBox() const {
    BoundingBox bbox;

    if (m_nodes.empty()) {
        return bbox;
    }

    // 첫 번째 노드로 초기화
    auto it = m_nodes.begin();
    const Eigen::Vector3d& firstPos = it->second.coordinates();

    bbox.minX = bbox.maxX = firstPos.x();
    bbox.minY = bbox.maxY = firstPos.y();
    bbox.minZ = bbox.maxZ = firstPos.z();

    // 나머지 노드로 확장
    ++it;
    for (; it != m_nodes.end(); ++it) {
        bbox.expand(it->second.coordinates());
    }

    return bbox;
}

void Mesh::invalidateBoundingBoxCache() {
    m_cachedBoundingBox.reset();
}

} // namespace core
} // namespace koomesh
