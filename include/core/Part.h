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
     * @brief Material 속성
     */
    struct MaterialProperties {
        // 기본 물성
        double density = 0.0;           ///< 밀도 (kg/m^3)
        double youngModulus = 0.0;      ///< 영계수 (Pa)
        double poissonRatio = 0.0;      ///< 포아송비

        // 추가 물성
        double yieldStrength = 0.0;     ///< 항복강도 (Pa)
        double ultimateStrength = 0.0;  ///< 극한강도 (Pa)
        double thermalExpansion = 0.0;  ///< 열팽창계수 (1/K)
        double thermalConductivity = 0.0; ///< 열전도율 (W/m·K)
        double specificHeat = 0.0;      ///< 비열 (J/kg·K)

        // 메타데이터
        std::string materialName;       ///< 재료명 (예: "Steel", "Aluminum")
        int materialType = 0;           ///< 재료 타입 (1: Elastic, 2: Plastic, 3: Composite, etc.)

        /**
         * @brief 유효성 검증
         */
        bool isValid() const {
            return density > 0.0 && youngModulus > 0.0 &&
                   poissonRatio >= -1.0 && poissonRatio <= 0.5;
        }

        /**
         * @brief 기본 재료 속성 설정 (강철)
         */
        static MaterialProperties Steel() {
            MaterialProperties props;
            props.density = 7850.0;              // kg/m^3
            props.youngModulus = 200.0e9;        // Pa (200 GPa)
            props.poissonRatio = 0.3;
            props.yieldStrength = 250.0e6;       // Pa (250 MPa)
            props.ultimateStrength = 400.0e6;    // Pa (400 MPa)
            props.thermalExpansion = 12.0e-6;    // 1/K
            props.thermalConductivity = 50.0;    // W/m·K
            props.specificHeat = 500.0;          // J/kg·K
            props.materialName = "Steel";
            props.materialType = 1;
            return props;
        }

        /**
         * @brief 기본 재료 속성 설정 (알루미늄)
         */
        static MaterialProperties Aluminum() {
            MaterialProperties props;
            props.density = 2700.0;              // kg/m^3
            props.youngModulus = 70.0e9;         // Pa (70 GPa)
            props.poissonRatio = 0.33;
            props.yieldStrength = 95.0e6;        // Pa (95 MPa)
            props.ultimateStrength = 110.0e6;    // Pa (110 MPa)
            props.thermalExpansion = 23.0e-6;    // 1/K
            props.thermalConductivity = 205.0;   // W/m·K
            props.specificHeat = 900.0;          // J/kg·K
            props.materialName = "Aluminum";
            props.materialType = 1;
            return props;
        }
    };

    void setMaterialProperties(const MaterialProperties& props) { m_material = props; }
    const MaterialProperties& materialProperties() const { return m_material; }
    MaterialProperties& materialProperties() { return m_material; }

    /**
     * @brief 요소 타입별 통계
     */
    struct ElementTypeStats {
        size_t tetrahedronCount = 0;
        size_t hexahedronCount = 0;
        size_t pentahedronCount = 0;
        size_t pyramidCount = 0;
        size_t triangleCount = 0;
        size_t quadrilateralCount = 0;
        size_t beamCount = 0;
        size_t unknownCount = 0;

        size_t totalCount() const {
            return tetrahedronCount + hexahedronCount + pentahedronCount +
                   pyramidCount + triangleCount + quadrilateralCount +
                   beamCount + unknownCount;
        }
    };

    /**
     * @brief 부품 색상 (시각화용)
     */
    struct Color {
        float r = 0.7f;  ///< Red (0.0 ~ 1.0)
        float g = 0.7f;  ///< Green (0.0 ~ 1.0)
        float b = 0.7f;  ///< Blue (0.0 ~ 1.0)
        float a = 1.0f;  ///< Alpha (0.0 ~ 1.0)

        Color() = default;
        Color(float red, float green, float blue, float alpha = 1.0f)
            : r(red), g(green), b(blue), a(alpha) {}

        static Color Red() { return Color(1.0f, 0.0f, 0.0f); }
        static Color Green() { return Color(0.0f, 1.0f, 0.0f); }
        static Color Blue() { return Color(0.0f, 0.0f, 1.0f); }
        static Color Yellow() { return Color(1.0f, 1.0f, 0.0f); }
        static Color Cyan() { return Color(0.0f, 1.0f, 1.0f); }
        static Color Magenta() { return Color(1.0f, 0.0f, 1.0f); }
        static Color Gray() { return Color(0.7f, 0.7f, 0.7f); }
    };

    void setColor(const Color& color) { m_color = color; }
    const Color& color() const { return m_color; }

    /**
     * @brief 비활성화 상태 설정 (시각화에서 숨김)
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief 비교 연산자
     */
    bool operator==(const Part& other) const {
        return m_id == other.m_id;
    }

    bool operator!=(const Part& other) const {
        return !(*this == other);
    }

private:
    PartId m_id;                                ///< 파트 ID
    std::string m_name;                         ///< 파트 이름
    std::unordered_set<ElementId> m_elementIds; ///< 요소 ID 집합
    MaterialProperties m_material;              ///< Material 속성
    Color m_color;                              ///< 시각화용 색상
    bool m_enabled = true;                      ///< 활성화 상태
};

} // namespace core
} // namespace koomesh
