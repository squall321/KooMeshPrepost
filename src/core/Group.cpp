#include "core/Group.h"
#include <algorithm>

namespace koomesh {
namespace core {

// ======================================================================
// 생성자
// ======================================================================

Group::Group(const std::string& name)
    : m_name(name)
    , m_color(Part::Color::Gray())
    , m_visible(true) {
}

// ======================================================================
// 요소 관리
// ======================================================================

void Group::addElement(ElementId id) {
    m_elements.insert(id);
}

bool Group::removeElement(ElementId id) {
    return m_elements.erase(id) > 0;
}

bool Group::containsElement(ElementId id) const {
    return m_elements.find(id) != m_elements.end();
}

// ======================================================================
// 노드 관리
// ======================================================================

void Group::addNode(NodeId id) {
    m_nodes.insert(id);
}

bool Group::removeNode(NodeId id) {
    return m_nodes.erase(id) > 0;
}

bool Group::containsNode(NodeId id) const {
    return m_nodes.find(id) != m_nodes.end();
}

// ======================================================================
// 클리어 메서드
// ======================================================================

void Group::clear() {
    m_elements.clear();
    m_nodes.clear();
}

void Group::clearElements() {
    m_elements.clear();
}

void Group::clearNodes() {
    m_nodes.clear();
}

// ======================================================================
// 집합 연산
// ======================================================================

void Group::merge(const Group& other) {
    // 합집합: 다른 그룹의 모든 요소와 노드 추가
    for (ElementId id : other.m_elements) {
        m_elements.insert(id);
    }

    for (NodeId id : other.m_nodes) {
        m_nodes.insert(id);
    }
}

void Group::intersect(const Group& other) {
    // 교집합: 공통 요소만 유지
    std::unordered_set<ElementId> commonElements;
    for (ElementId id : m_elements) {
        if (other.containsElement(id)) {
            commonElements.insert(id);
        }
    }
    m_elements = std::move(commonElements);

    std::unordered_set<NodeId> commonNodes;
    for (NodeId id : m_nodes) {
        if (other.containsNode(id)) {
            commonNodes.insert(id);
        }
    }
    m_nodes = std::move(commonNodes);
}

void Group::subtract(const Group& other) {
    // 차집합: 다른 그룹에 있는 요소와 노드 제거
    for (ElementId id : other.m_elements) {
        m_elements.erase(id);
    }

    for (NodeId id : other.m_nodes) {
        m_nodes.erase(id);
    }
}

} // namespace core
} // namespace koomesh
