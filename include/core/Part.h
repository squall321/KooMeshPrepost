#pragma once

#include "Element.h"
#include <string>
#include <unordered_set>

namespace koomesh {
namespace core {

/**
 * @brief Part 클래스 - 요소 그룹
 *
 * LS-DYNA의 PART에 해당하며, 관련된 요소들을 그룹화합니다.
 */
class Part {
public:
    /**
     * @brief Part 생성자
     * @param id 파트 ID
     * @param name 파트 이름
     */
    Part(PartId id, const std::string& name = "");

    // Getter 메서드
    PartId id() const { return m_id; }
    const std::string& name() const { return m_name; }
    const std::unordered_set<ElementId>& elements() const { return m_elementIds; }
    size_t elementCount() const { return m_elementIds.size(); }

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
     * @brief 모든 요소 제거
     */
    void clear();

    /**
     * @brief Material 속성 (향후 확장)
     */
    struct MaterialProperties {
        double density = 0.0;
        double youngModulus = 0.0;
        double poissonRatio = 0.0;
    };

    void setMaterialProperties(const MaterialProperties& props) { m_material = props; }
    const MaterialProperties& materialProperties() const { return m_material; }

private:
    PartId m_id;                                ///< 파트 ID
    std::string m_name;                         ///< 파트 이름
    std::unordered_set<ElementId> m_elementIds; ///< 요소 ID 집합
    MaterialProperties m_material;              ///< Material 속성
};

} // namespace core
} // namespace koomesh
