#pragma once

#include "Mesh.h"
#include <vector>
#include <string>

namespace koomesh {
namespace core {

/**
 * @brief MeshValidator 클래스
 *
 * 메시 데이터의 유효성을 검증하고, 문제점을 보고합니다.
 */
class MeshValidator {
public:
    /**
     * @brief 검증 결과
     */
    struct ValidationResult {
        bool isValid = true;                    ///< 전체 유효성
        std::vector<std::string> errors;        ///< 치명적 오류
        std::vector<std::string> warnings;      ///< 경고
        std::vector<std::string> info;          ///< 정보

        /**
         * @brief 오류 개수
         */
        size_t errorCount() const { return errors.size(); }

        /**
         * @brief 경고 개수
         */
        size_t warningCount() const { return warnings.size(); }

        /**
         * @brief 정보 개수
         */
        size_t infoCount() const { return info.size(); }

        /**
         * @brief 문제 총 개수
         */
        size_t totalIssueCount() const {
            return errors.size() + warnings.size();
        }

        /**
         * @brief 오류 추가
         */
        void addError(const std::string& message) {
            errors.push_back(message);
            isValid = false;
        }

        /**
         * @brief 경고 추가
         */
        void addWarning(const std::string& message) {
            warnings.push_back(message);
        }

        /**
         * @brief 정보 추가
         */
        void addInfo(const std::string& message) {
            info.push_back(message);
        }
    };

    /**
     * @brief 검증 옵션
     */
    struct ValidationOptions {
        bool checkNodeReferences = true;       ///< 노드 참조 유효성
        bool checkConnectivity = true;         ///< 연결성 검사
        bool checkDuplicateNodes = true;       ///< 중복 노드 검사
        bool checkDuplicateElements = true;    ///< 중복 요소 검사
        bool checkDegenerateElements = true;   ///< 퇴화 요소 검사
        bool checkElementQuality = true;       ///< 요소 품질 검사
        bool checkPartReferences = true;       ///< 파트 참조 검사
        bool checkBoundingBox = true;          ///< 바운딩 박스 검사

        double minQualityThreshold = 0.1;      ///< 최소 품질 임계값
        double coincidentNodeTolerance = 1e-6; ///< 중복 노드 판정 거리
    };

    MeshValidator() = default;

    /**
     * @brief 메시 전체 검증
     */
    ValidationResult validate(const Mesh& mesh,
                             const ValidationOptions& options);

    /**
     * @brief 노드 참조 검증
     */
    static bool checkNodeReferences(const Mesh& mesh, ValidationResult& result);

    /**
     * @brief 연결성 검증
     */
    static bool checkConnectivity(const Mesh& mesh, ValidationResult& result);

    /**
     * @brief 중복 노드 검증
     */
    static bool checkDuplicateNodes(const Mesh& mesh,
                                   ValidationResult& result,
                                   double tolerance);

    /**
     * @brief 중복 요소 검증
     */
    static bool checkDuplicateElements(const Mesh& mesh, ValidationResult& result);

    /**
     * @brief 퇴화 요소 검증
     */
    static bool checkDegenerateElements(const Mesh& mesh, ValidationResult& result);

    /**
     * @brief 요소 품질 검증
     */
    static bool checkElementQuality(const Mesh& mesh,
                                   ValidationResult& result,
                                   double minQuality);

    /**
     * @brief 파트 참조 검증
     */
    static bool checkPartReferences(const Mesh& mesh, ValidationResult& result);

    /**
     * @brief 바운딩 박스 검증
     */
    static bool checkBoundingBox(const Mesh& mesh, ValidationResult& result);

private:
    /**
     * @brief 중복 노드 찾기 (간단한 구현)
     */
    static std::vector<std::pair<NodeId, NodeId>> findCoincidentNodes(
        const Mesh& mesh,
        double tolerance);
};

} // namespace core
} // namespace koomesh
