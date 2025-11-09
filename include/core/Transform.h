#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>

namespace koomesh {
namespace core {

/**
 * @brief Transform 클래스
 *
 * 3D 변환 행렬을 관리하며, 평행이동, 회전, 스케일링을 지원합니다.
 * Eigen의 Affine3d를 사용하여 효율적인 변환 연산을 제공합니다.
 */
class Transform {
public:
    /**
     * @brief 기본 생성자 (항등 변환)
     */
    Transform();

    /**
     * @brief 변환 행렬로부터 생성
     */
    explicit Transform(const Eigen::Affine3d& transform);

    /**
     * @brief 평행이동
     * @param offset 이동 벡터
     */
    void translate(const Eigen::Vector3d& offset);

    /**
     * @brief 회전 (축-각도 방식)
     * @param axis 회전 축 (정규화됨)
     * @param angleRadians 회전 각도 (라디안)
     */
    void rotate(const Eigen::Vector3d& axis, double angleRadians);

    /**
     * @brief 오일러 각도 회전 (ZYX 순서)
     * @param roll X축 회전 (라디안)
     * @param pitch Y축 회전 (라디안)
     * @param yaw Z축 회전 (라디안)
     */
    void rotateEuler(double roll, double pitch, double yaw);

    /**
     * @brief 스케일링 (균일)
     * @param factor 스케일 인수
     */
    void scale(double factor);

    /**
     * @brief 스케일링 (비균일)
     * @param scaleX X축 스케일
     * @param scaleY Y축 스케일
     * @param scaleZ Z축 스케일
     */
    void scale(double scaleX, double scaleY, double scaleZ);

    /**
     * @brief 점 변환 적용
     * @param point 원본 점
     * @return 변환된 점
     */
    Eigen::Vector3d apply(const Eigen::Vector3d& point) const;

    /**
     * @brief 벡터 변환 적용 (평행이동 무시)
     * @param vector 원본 벡터
     * @return 변환된 벡터
     */
    Eigen::Vector3d applyVector(const Eigen::Vector3d& vector) const;

    /**
     * @brief 법선 벡터 변환 (역전치 행렬 사용)
     * @param normal 원본 법선
     * @return 변환된 법선
     */
    Eigen::Vector3d applyNormal(const Eigen::Vector3d& normal) const;

    /**
     * @brief 변환 행렬 조회
     */
    const Eigen::Affine3d& matrix() const { return m_transform; }

    /**
     * @brief 변환 행렬 설정
     */
    void setMatrix(const Eigen::Affine3d& transform) { m_transform = transform; }

    /**
     * @brief 항등 변환으로 리셋
     */
    void reset();

    /**
     * @brief 역변환
     */
    Transform inverse() const;

    /**
     * @brief 변환 합성 (this = this * other)
     */
    Transform& operator*=(const Transform& other);

    /**
     * @brief 변환 합성
     */
    Transform operator*(const Transform& other) const;

    /**
     * @brief 평행이동 성분 추출
     */
    Eigen::Vector3d translation() const;

    /**
     * @brief 회전 성분 추출 (쿼터니언)
     */
    Eigen::Quaterniond rotation() const;

    /**
     * @brief 스케일 성분 추출 (근사)
     */
    Eigen::Vector3d scaleFactors() const;

    /**
     * @brief TRS 분해 (Translation, Rotation, Scale)
     */
    void decompose(Eigen::Vector3d& translation,
                   Eigen::Quaterniond& rotation,
                   Eigen::Vector3d& scale) const;

    /**
     * @brief TRS로부터 구성
     */
    static Transform compose(const Eigen::Vector3d& translation,
                            const Eigen::Quaterniond& rotation,
                            const Eigen::Vector3d& scale);

    /**
     * @brief 두 변환 사이를 보간 (SLERP)
     * @param other 대상 변환
     * @param t 보간 계수 (0.0 ~ 1.0)
     */
    Transform interpolate(const Transform& other, double t) const;

    /**
     * @brief LookAt 변환 생성
     * @param eye 카메라 위치
     * @param target 목표 지점
     * @param up 상향 벡터
     */
    static Transform lookAt(const Eigen::Vector3d& eye,
                           const Eigen::Vector3d& target,
                           const Eigen::Vector3d& up);

private:
    Eigen::Affine3d m_transform;  ///< 변환 행렬 (4x4 동차 좌표)
};

} // namespace core
} // namespace koomesh
