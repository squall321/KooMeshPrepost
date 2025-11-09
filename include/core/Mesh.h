#pragma once

#include "Element.h"
#include "Node.h"
#include "Part.h"
#include <memory>
#include <unordered_map>
#include <vector>

namespace koomesh {
namespace core {

// Forward declarations
class ISpatialIndex;

/**
 * @brief Mesh 클래스 - Aggregate Root
 *
 * FEM mesh의 전체 데이터를 관리하는 핵심 클래스입니다.
 * CQRS 패턴을 따라 Command(상태 변경)와 Query(조회)를 분리합니다.
 */
class Mesh {
public:
    Mesh();
    ~Mesh();

    // ==================== Commands (상태 변경) ====================

    /**
     * @brief 노드 추가
     */
    void addNode(const Node& node);

    /**
     * @brief 노드 배치 추가 (성능 최적화)
     */
    void addNodesBatch(const std::vector<Node>& nodes);

    /**
     * @brief 요소 추가
     */
    void addElement(std::unique_ptr<Element> element);

    /**
     * @brief 요소 배치 추가
     */
    void addElementsBatch(std::vector<std::unique_ptr<Element>> elements);

    /**
     * @brief 파트 추가
     */
    void addPart(const Part& part);

    /**
     * @brief 노드 제거
     */
    bool removeNode(NodeId id);

    /**
     * @brief 요소 제거
     */
    bool removeElement(ElementId id);

    /**
     * @brief 모든 데이터 삭제
     */
    void clear();

    /**
     * @brief 공간 인덱스 구축 (파일 로딩 후 호출)
     */
    void buildSpatialIndex();

    // ==================== Queries (조회) ====================

    /**
     * @brief 노드 조회
     * @return 노드 포인터 (없으면 nullptr)
     */
    const Node* getNode(NodeId id) const;
    Node* getNode(NodeId id);

    /**
     * @brief 요소 조회
     */
    const Element* getElement(ElementId id) const;
    Element* getElement(ElementId id);

    /**
     * @brief 파트 조회
     */
    const Part* getPart(PartId id) const;
    Part* getPart(PartId id);

    /**
     * @brief 전체 노드 수
     */
    size_t nodeCount() const { return m_nodes.size(); }

    /**
     * @brief 전체 요소 수
     */
    size_t elementCount() const { return m_elements.size(); }

    /**
     * @brief 전체 파트 수
     */
    size_t partCount() const { return m_parts.size(); }

    /**
     * @brief 모든 노드 ID 목록
     */
    std::vector<NodeId> getAllNodeIds() const;

    /**
     * @brief 모든 요소 ID 목록
     */
    std::vector<ElementId> getAllElementIds() const;

    /**
     * @brief 전체 경계 상자
     */
    BoundingBox boundingBox() const;

    /**
     * @brief 특정 영역 내의 요소 찾기 (공간 인덱스 사용)
     */
    std::vector<ElementId> findElementsInBox(const BoundingBox& box) const;

    /**
     * @brief 특정 점에 가장 가까운 요소 찾기
     */
    ElementId findNearestElement(const Eigen::Vector3d& point) const;

    /**
     * @brief 특정 점에 가장 가까운 노드 찾기
     */
    NodeId findNearestNode(const Eigen::Vector3d& point) const;

    /**
     * @brief 요소 타입별 개수
     */
    std::unordered_map<ElementType, size_t> getElementTypeStatistics() const;

    // ==================== Iterators ====================

    /**
     * @brief 노드 순회를 위한 const iterator
     */
    auto nodesBegin() const { return m_nodes.begin(); }
    auto nodesEnd() const { return m_nodes.end(); }

    /**
     * @brief 요소 순회를 위한 const iterator
     */
    auto elementsBegin() const { return m_elements.begin(); }
    auto elementsEnd() const { return m_elements.end(); }

    /**
     * @brief 모든 노드 컬렉션 접근 (read-only)
     */
    const std::unordered_map<NodeId, Node>& nodes() const { return m_nodes; }

    /**
     * @brief 모든 요소 컬렉션 접근 (read-only)
     */
    const std::unordered_map<ElementId, std::unique_ptr<Element>>& elements() const { return m_elements; }

    /**
     * @brief 모든 파트 컬렉션 접근 (read-only)
     */
    const std::unordered_map<PartId, Part>& parts() const { return m_parts; }

private:
    // 효율적인 조회를 위한 hash map
    std::unordered_map<NodeId, Node> m_nodes;
    std::unordered_map<ElementId, std::unique_ptr<Element>> m_elements;
    std::unordered_map<PartId, Part> m_parts;

    // 공간 인덱싱 (Strategy 패턴)
    std::unique_ptr<ISpatialIndex> m_spatialIndex;

    // 캐시된 경계 상자 (지연 평가)
    mutable std::unique_ptr<BoundingBox> m_cachedBoundingBox;

    /**
     * @brief 경계 상자 계산
     */
    BoundingBox computeBoundingBox() const;

    /**
     * @brief 경계 상자 캐시 무효화
     */
    void invalidateBoundingBoxCache();
};

} // namespace core
} // namespace koomesh
