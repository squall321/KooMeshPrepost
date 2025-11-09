#include "core/Node.h"
#include <algorithm>
#include <cmath>

namespace koomesh {
namespace core {

// ======================================================================
// 생성자
// ======================================================================

Node::Node(NodeId id, double x, double y, double z)
    : m_id(id)
    , m_coords(x, y, z) {
}

Node::Node(NodeId id, const Eigen::Vector3d& coords)
    : m_id(id)
    , m_coords(coords) {
}

// ======================================================================
// 연결된 요소 관리
// ======================================================================

void Node::addConnectedElement(ElementId elemId) {
    // 중복 체크 후 추가
    auto it = std::find(m_connectedElements.begin(), m_connectedElements.end(), elemId);
    if (it == m_connectedElements.end()) {
        m_connectedElements.push_back(elemId);
    }
}

// ======================================================================
// 유틸리티 메서드
// ======================================================================

double Node::distanceTo(const Node& other) const {
    return (m_coords - other.m_coords).norm();
}

void Node::translate(const Eigen::Vector3d& offset) {
    m_coords += offset;
}

void Node::scale(double factor) {
    m_coords *= factor;
}

} // namespace core
} // namespace koomesh
