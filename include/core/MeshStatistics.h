#pragma once

#include "Mesh.h"
#include "Element.h"
#include <map>
#include <string>

namespace koomesh {
namespace core {

/**
 * @brief MeshStatistics 클래스
 *
 * 메시 데이터의 통계 정보를 수집하고 분석합니다.
 */
class MeshStatistics {
public:
    /**
     * @brief 통계 정보 구조체
     */
    struct Statistics {
        // 기본 카운트
        size_t totalNodes = 0;
        size_t totalElements = 0;
        size_t totalParts = 0;

        // 요소 타입별 카운트
        std::map<ElementType, size_t> elementTypeCounts;

        // 품질 통계
        double minElementQuality = 1.0;
        double maxElementQuality = 0.0;
        double avgElementQuality = 0.0;
        double stdDevElementQuality = 0.0;

        // 부피/면적 통계
        double minElementVolume = 0.0;
        double maxElementVolume = 0.0;
        double totalVolume = 0.0;
        double avgElementVolume = 0.0;

        // 바운딩 박스
        BoundingBox globalBounds;

        // 연결성 통계
        size_t minNodeConnections = 0;
        size_t maxNodeConnections = 0;
        double avgNodeConnections = 0.0;

        // 파트별 통계
        std::map<PartId, size_t> partElementCounts;

        /**
         * @brief 요소 타입 이름 가져오기
         */
        static std::string getElementTypeName(ElementType type);

        /**
         * @brief 요소 타입별 개수 가져오기
         */
        size_t getElementCount(ElementType type) const;

        /**
         * @brief 텍스트 보고서 생성
         */
        std::string generateReport() const;

        /**
         * @brief 간단한 요약
         */
        std::string summary() const;
    };

    /**
     * @brief 통계 수집 옵션
     */
    struct Options {
        bool computeQuality = true;         ///< 품질 계산 (느림)
        bool computeVolume = true;          ///< 부피 계산
        bool computeConnectivity = true;    ///< 연결성 계산
        bool computePartStats = true;       ///< 파트별 통계
    };

    MeshStatistics() = default;

    /**
     * @brief 메시 통계 계산
     */
    static Statistics compute(const Mesh& mesh, const Options& options);

private:
    /**
     * @brief 기본 카운트 수집
     */
    static void computeBasicCounts(const Mesh& mesh, Statistics& stats);

    /**
     * @brief 요소 타입별 카운트
     */
    static void computeElementTypeCounts(const Mesh& mesh, Statistics& stats);

    /**
     * @brief 품질 통계 계산
     */
    static void computeQualityStatistics(const Mesh& mesh, Statistics& stats);

    /**
     * @brief 부피 통계 계산
     */
    static void computeVolumeStatistics(const Mesh& mesh, Statistics& stats);

    /**
     * @brief 바운딩 박스 계산
     */
    static void computeBoundingBox(const Mesh& mesh, Statistics& stats);

    /**
     * @brief 연결성 통계 계산
     */
    static void computeConnectivityStatistics(const Mesh& mesh, Statistics& stats);

    /**
     * @brief 파트별 통계 계산
     */
    static void computePartStatistics(const Mesh& mesh, Statistics& stats);

    /**
     * @brief 표준 편차 계산
     */
    static double computeStandardDeviation(const std::vector<double>& values, double mean);
};

} // namespace core
} // namespace koomesh
