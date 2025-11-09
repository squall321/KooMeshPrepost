#pragma once

#include <cstdint>
#include <vector>
#include <Eigen/Core>

namespace koomesh {
namespace core {

using NodeId = uint64_t;
using ElementId = uint64_t;

/**
 * @brief Node 클래스 - FEM mesh의 노드를 표현
 *
 * 각 노드는 고유 ID와 3D 좌표를 가지며, 연결된 요소들에 대한 역참조를 유지합니다.
 */
class Node {
public:
    /**
     * @brief Node 생성자
     * @param id 노드 고유 ID
     * @param x X 좌표
     * @param y Y 좌표
     * @param z Z 좌표
     */
    Node(NodeId id, double x, double y, double z);

    /**
     * @brief Eigen Vector를 사용한 생성자
     */
    Node(NodeId id, const Eigen::Vector3d& coords);

    // Getter 메서드
    NodeId id() const { return m_id; }
    const Eigen::Vector3d& coordinates() const { return m_coords; }
    double x() const { return m_coords.x(); }
    double y() const { return m_coords.y(); }
    double z() const { return m_coords.z(); }

    /**
     * @brief 노드에 연결된 요소 추가
     */
    void addConnectedElement(ElementId elemId);

    /**
     * @brief 연결된 요소 목록 조회
     */
    const std::vector<ElementId>& connectedElements() const { return m_connectedElements; }

    /**
     * @brief 다른 노드와의 거리 계산
     */
    double distanceTo(const Node& other) const;

    /**
     * @brief 좌표 변환 (이동)
     */
    void translate(const Eigen::Vector3d& offset);

    /**
     * @brief 좌표 변환 (스케일링)
     */
    void scale(double factor);

private:
    NodeId m_id;                            ///< 노드 고유 ID
    Eigen::Vector3d m_coords;               ///< 3D 좌표
    std::vector<ElementId> m_connectedElements;  ///< 연결된 요소 ID 목록 (역참조)
};

} // namespace core
} // namespace koomesh
