#pragma once

#include "Node.h"
#include <array>
#include <memory>
#include <vector>

namespace koomesh {
namespace core {

class Mesh; // Forward declaration

/**
 * @brief 요소 타입 열거형
 */
enum class ElementType {
    TETRAHEDRON,    ///< 4-node tetrahedron
    HEXAHEDRON,     ///< 8-node hexahedron
    PENTAHEDRON,    ///< 6-node pentahedron (wedge)
    PYRAMID,        ///< 5-node pyramid
    TRIANGLE,       ///< 3-node triangle shell
    QUADRILATERAL,  ///< 4-node quadrilateral shell
    BEAM,           ///< 2-node beam
    UNKNOWN
};

/**
 * @brief 경계 상자 (AABB - Axis-Aligned Bounding Box)
 */
struct BoundingBox {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;

    BoundingBox() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {}

    BoundingBox(const Eigen::Vector3d& minPt, const Eigen::Vector3d& maxPt)
        : minX(minPt.x()), minY(minPt.y()), minZ(minPt.z())
        , maxX(maxPt.x()), maxY(maxPt.y()), maxZ(maxPt.z()) {}

    bool intersects(const BoundingBox& other) const;
    bool contains(const Eigen::Vector3d& point) const;
    void expand(const Eigen::Vector3d& point);
    void expand(const BoundingBox& other);
    Eigen::Vector3d center() const;
    double volume() const;

    // Additional methods for spatial indexes
    Eigen::Vector3d min() const { return Eigen::Vector3d(minX, minY, minZ); }
    Eigen::Vector3d max() const { return Eigen::Vector3d(maxX, maxY, maxZ); }

    double distanceSquared(const Eigen::Vector3d& point) const;
};

using PartId = uint32_t;

/**
 * @brief Element 추상 클래스
 *
 * Template Method 패턴을 사용하여 공통 인터페이스를 정의하고,
 * 구체적인 요소 타입은 하위 클래스에서 구현합니다.
 */
class Element {
public:
    Element(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);
    virtual ~Element() = default;

    // Getter 메서드
    ElementId id() const { return m_id; }
    PartId partId() const { return m_partId; }
    const std::vector<NodeId>& nodeIds() const { return m_nodeIds; }

    /**
     * @brief 요소 타입 반환 (순수 가상 함수)
     */
    virtual ElementType type() const = 0;

    /**
     * @brief 노드 개수 반환
     */
    virtual size_t nodeCount() const = 0;

    /**
     * @brief 경계 상자 계산
     */
    virtual BoundingBox computeBoundingBox(const Mesh& mesh) const;

    /**
     * @brief 요소 중심 계산
     */
    virtual Eigen::Vector3d computeCenter(const Mesh& mesh) const;

    /**
     * @brief 요소 품질 계산 (0.0 ~ 1.0, 1.0이 최상)
     */
    virtual double computeQuality(const Mesh& mesh) const = 0;

    /**
     * @brief 요소 부피 계산
     */
    virtual double computeVolume(const Mesh& mesh) const = 0;

    /**
     * @brief 특정 점이 요소 내부에 있는지 확인
     */
    virtual bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const = 0;

protected:
    ElementId m_id;                 ///< 요소 고유 ID
    PartId m_partId;                ///< 소속 파트 ID
    std::vector<NodeId> m_nodeIds;  ///< 노드 ID 목록
};

/**
 * @brief 사면체 요소 (4-node tetrahedron)
 */
class TetrahedronElement : public Element {
public:
    TetrahedronElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);

    ElementType type() const override { return ElementType::TETRAHEDRON; }
    size_t nodeCount() const override { return 4; }
    double computeQuality(const Mesh& mesh) const override;
    double computeVolume(const Mesh& mesh) const override;
    bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const override;
};

/**
 * @brief 육면체 요소 (8-node hexahedron)
 */
class HexahedronElement : public Element {
public:
    HexahedronElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);

    ElementType type() const override { return ElementType::HEXAHEDRON; }
    size_t nodeCount() const override { return 8; }
    double computeQuality(const Mesh& mesh) const override;
    double computeVolume(const Mesh& mesh) const override;
    bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const override;
};

/**
 * @brief 오각기둥 요소 (6-node pentahedron/wedge)
 */
class PentahedronElement : public Element {
public:
    PentahedronElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);

    ElementType type() const override { return ElementType::PENTAHEDRON; }
    size_t nodeCount() const override { return 6; }
    double computeQuality(const Mesh& mesh) const override;
    double computeVolume(const Mesh& mesh) const override;
    bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const override;
};

/**
 * @brief 피라미드 요소 (5-node pyramid)
 */
class PyramidElement : public Element {
public:
    PyramidElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);

    ElementType type() const override { return ElementType::PYRAMID; }
    size_t nodeCount() const override { return 5; }
    double computeQuality(const Mesh& mesh) const override;
    double computeVolume(const Mesh& mesh) const override;
    bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const override;
};

/**
 * @brief 삼각형 쉘 요소 (3-node triangle shell)
 */
class TriangleElement : public Element {
public:
    TriangleElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);

    ElementType type() const override { return ElementType::TRIANGLE; }
    size_t nodeCount() const override { return 3; }
    double computeQuality(const Mesh& mesh) const override;
    double computeVolume(const Mesh& mesh) const override;  // 면적 반환
    bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const override;

    /**
     * @brief 법선 벡터 계산
     */
    Eigen::Vector3d computeNormal(const Mesh& mesh) const;

    /**
     * @brief 면적 계산
     */
    double computeArea(const Mesh& mesh) const;
};

/**
 * @brief 사각형 쉘 요소 (4-node quadrilateral shell)
 */
class QuadrilateralElement : public Element {
public:
    QuadrilateralElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);

    ElementType type() const override { return ElementType::QUADRILATERAL; }
    size_t nodeCount() const override { return 4; }
    double computeQuality(const Mesh& mesh) const override;
    double computeVolume(const Mesh& mesh) const override;  // 면적 반환
    bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const override;

    /**
     * @brief 법선 벡터 계산
     */
    Eigen::Vector3d computeNormal(const Mesh& mesh) const;

    /**
     * @brief 면적 계산
     */
    double computeArea(const Mesh& mesh) const;
};

/**
 * @brief 빔 요소 (2-node beam)
 */
class BeamElement : public Element {
public:
    BeamElement(ElementId id, PartId partId, const std::vector<NodeId>& nodeIds);

    ElementType type() const override { return ElementType::BEAM; }
    size_t nodeCount() const override { return 2; }
    double computeQuality(const Mesh& mesh) const override;
    double computeVolume(const Mesh& mesh) const override;  // 길이 반환
    bool containsPoint(const Eigen::Vector3d& point, const Mesh& mesh) const override;

    /**
     * @brief 길이 계산
     */
    double computeLength(const Mesh& mesh) const;

    /**
     * @brief 방향 벡터 계산 (정규화됨)
     */
    Eigen::Vector3d computeDirection(const Mesh& mesh) const;
};

/**
 * @brief Element Factory - Factory 패턴
 *
 * 요소 타입에 따라 적절한 Element 객체를 생성합니다.
 */
class ElementFactory {
public:
    /**
     * @brief 요소 생성
     * @param type 요소 타입
     * @param id 요소 ID
     * @param partId 파트 ID
     * @param nodeIds 노드 ID 목록
     * @return 생성된 Element 객체 (unique_ptr)
     */
    static std::unique_ptr<Element> create(
        ElementType type,
        ElementId id,
        PartId partId,
        const std::vector<NodeId>& nodeIds
    );
};

} // namespace core
} // namespace koomesh
