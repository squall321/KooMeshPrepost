#pragma once

#include "Node.h"
#include "Element.h"
#include "Part.h"
#include <string>
#include <unordered_set>
#include <vector>

namespace koomesh {
namespace core {

/**
 * @brief Group 클래스 - 요소 및 노드 그룹핑
 *
 * 사용자 정의 선택 그룹을 관리합니다.
 * Part와 달리 임시적인 선택/필터링 용도로 사용됩니다.
 */
class Group {
public:
    /**
     * @brief Group 생성자
     * @param name 그룹 이름
     */
    explicit Group(const std::string& name = "");

    // Getter 메서드
    const std::string& name() const { return m_name; }
    const std::unordered_set<ElementId>& elements() const { return m_elements; }
    const std::unordered_set<NodeId>& nodes() const { return m_nodes; }

    size_t elementCount() const { return m_elements.size(); }
    size_t nodeCount() const { return m_nodes.size(); }
    bool isEmpty() const { return m_elements.empty() && m_nodes.empty(); }

    // Setter 메서드
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief 요소 추가
     */
    void addElement(ElementId id);

    /**
     * @brief 요소 제거
     */
    bool removeElement(ElementId id);

    /**
     * @brief 요소 포함 여부
     */
    bool containsElement(ElementId id) const;

    /**
     * @brief 노드 추가
     */
    void addNode(NodeId id);

    /**
     * @brief 노드 제거
     */
    bool removeNode(NodeId id);

    /**
     * @brief 노드 포함 여부
     */
    bool containsNode(NodeId id) const;

    /**
     * @brief 모든 요소와 노드 제거
     */
    void clear();

    /**
     * @brief 요소만 제거
     */
    void clearElements();

    /**
     * @brief 노드만 제거
     */
    void clearNodes();

    /**
     * @brief 그룹 색상
     */
    void setColor(const Part::Color& color) { m_color = color; }
    const Part::Color& color() const { return m_color; }

    /**
     * @brief 가시성 설정
     */
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }

    /**
     * @brief 다른 그룹과 합집합
     */
    void merge(const Group& other);

    /**
     * @brief 다른 그룹과 교집합
     */
    void intersect(const Group& other);

    /**
     * @brief 다른 그룹과 차집합
     */
    void subtract(const Group& other);

    /**
     * @brief 비교 연산자
     */
    bool operator==(const Group& other) const {
        return m_name == other.m_name;
    }

    bool operator!=(const Group& other) const {
        return !(*this == other);
    }

private:
    std::string m_name;                       ///< 그룹 이름
    std::unordered_set<ElementId> m_elements; ///< 요소 ID 집합
    std::unordered_set<NodeId> m_nodes;       ///< 노드 ID 집합
    Part::Color m_color;                      ///< 시각화용 색상
    bool m_visible = true;                    ///< 가시성 플래그
};

} // namespace core
} // namespace koomesh
