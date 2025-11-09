#include "core/Transform.h"
#include <cmath>

namespace koomesh {
namespace core {

// ======================================================================
// 생성자
// ======================================================================

Transform::Transform()
    : m_transform(Eigen::Affine3d::Identity()) {
}

Transform::Transform(const Eigen::Affine3d& transform)
    : m_transform(transform) {
}

// ======================================================================
// 변환 연산
// ======================================================================

void Transform::translate(const Eigen::Vector3d& offset) {
    m_transform.translate(offset);
}

void Transform::rotate(const Eigen::Vector3d& axis, double angleRadians) {
    // 축을 정규화
    Eigen::Vector3d normalizedAxis = axis.normalized();

    // AngleAxis를 사용한 회전
    Eigen::AngleAxisd rotation(angleRadians, normalizedAxis);
    m_transform.rotate(rotation);
}

void Transform::rotateEuler(double roll, double pitch, double yaw) {
    // ZYX 오일러 각도 (Yaw-Pitch-Roll)
    Eigen::AngleAxisd rollAngle(roll, Eigen::Vector3d::UnitX());
    Eigen::AngleAxisd pitchAngle(pitch, Eigen::Vector3d::UnitY());
    Eigen::AngleAxisd yawAngle(yaw, Eigen::Vector3d::UnitZ());

    Eigen::Quaterniond q = yawAngle * pitchAngle * rollAngle;
    m_transform.rotate(q);
}

void Transform::scale(double factor) {
    m_transform.scale(factor);
}

void Transform::scale(double scaleX, double scaleY, double scaleZ) {
    Eigen::Vector3d scaleVec(scaleX, scaleY, scaleZ);
    m_transform.scale(scaleVec);
}

// ======================================================================
// 적용 메서드
// ======================================================================

Eigen::Vector3d Transform::apply(const Eigen::Vector3d& point) const {
    return m_transform * point;
}

Eigen::Vector3d Transform::applyVector(const Eigen::Vector3d& vector) const {
    // 평행이동 무시 (선형 부분만 적용)
    return m_transform.linear() * vector;
}

Eigen::Vector3d Transform::applyNormal(const Eigen::Vector3d& normal) const {
    // 법선 벡터는 역전치 행렬로 변환
    Eigen::Matrix3d normalMatrix = m_transform.linear().inverse().transpose();
    Eigen::Vector3d transformed = normalMatrix * normal;
    return transformed.normalized();
}

// ======================================================================
// 유틸리티 메서드
// ======================================================================

void Transform::reset() {
    m_transform = Eigen::Affine3d::Identity();
}

Transform Transform::inverse() const {
    return Transform(m_transform.inverse());
}

Transform& Transform::operator*=(const Transform& other) {
    m_transform = m_transform * other.m_transform;
    return *this;
}

Transform Transform::operator*(const Transform& other) const {
    return Transform(m_transform * other.m_transform);
}

// ======================================================================
// 성분 추출
// ======================================================================

Eigen::Vector3d Transform::translation() const {
    return m_transform.translation();
}

Eigen::Quaterniond Transform::rotation() const {
    // 회전 행렬에서 쿼터니언 추출
    Eigen::Matrix3d rotationMatrix = m_transform.linear();

    // 스케일 제거 (각 열의 노름으로 나눔)
    Eigen::Vector3d scaleX = rotationMatrix.col(0);
    Eigen::Vector3d scaleY = rotationMatrix.col(1);
    Eigen::Vector3d scaleZ = rotationMatrix.col(2);

    double sx = scaleX.norm();
    double sy = scaleY.norm();
    double sz = scaleZ.norm();

    if (sx > 1e-10) rotationMatrix.col(0) /= sx;
    if (sy > 1e-10) rotationMatrix.col(1) /= sy;
    if (sz > 1e-10) rotationMatrix.col(2) /= sz;

    return Eigen::Quaterniond(rotationMatrix);
}

Eigen::Vector3d Transform::scaleFactors() const {
    Eigen::Matrix3d linear = m_transform.linear();

    double sx = linear.col(0).norm();
    double sy = linear.col(1).norm();
    double sz = linear.col(2).norm();

    return Eigen::Vector3d(sx, sy, sz);
}

void Transform::decompose(Eigen::Vector3d& translation,
                         Eigen::Quaterniond& rotation,
                         Eigen::Vector3d& scale) const {
    translation = this->translation();
    scale = scaleFactors();
    rotation = this->rotation();
}

Transform Transform::compose(const Eigen::Vector3d& translation,
                            const Eigen::Quaterniond& rotation,
                            const Eigen::Vector3d& scale) {
    Eigen::Affine3d transform = Eigen::Affine3d::Identity();

    // TRS 순서: Scale -> Rotate -> Translate
    transform.scale(scale);
    transform.rotate(rotation);
    transform.translate(translation);

    return Transform(transform);
}

// ======================================================================
// 보간
// ======================================================================

Transform Transform::interpolate(const Transform& other, double t) const {
    // 클램핑
    t = std::max(0.0, std::min(1.0, t));

    // 각 성분 분해
    Eigen::Vector3d t1, t2, s1, s2;
    Eigen::Quaterniond r1, r2;

    decompose(t1, r1, s1);
    other.decompose(t2, r2, s2);

    // 선형 보간
    Eigen::Vector3d translation = (1.0 - t) * t1 + t * t2;
    Eigen::Vector3d scale = (1.0 - t) * s1 + t * s2;

    // 구면 선형 보간 (SLERP)
    Eigen::Quaterniond rotation = r1.slerp(t, r2);

    return compose(translation, rotation, scale);
}

// ======================================================================
// 특수 변환
// ======================================================================

Transform Transform::lookAt(const Eigen::Vector3d& eye,
                           const Eigen::Vector3d& target,
                           const Eigen::Vector3d& up) {
    // 카메라 좌표계 구성
    Eigen::Vector3d forward = (target - eye).normalized();
    Eigen::Vector3d right = forward.cross(up).normalized();
    Eigen::Vector3d correctedUp = right.cross(forward);

    // 회전 행렬 구성
    Eigen::Matrix3d rotationMatrix;
    rotationMatrix.col(0) = right;
    rotationMatrix.col(1) = correctedUp;
    rotationMatrix.col(2) = -forward;  // OpenGL 규약 (-Z가 forward)

    // Affine 변환 구성
    Eigen::Affine3d transform = Eigen::Affine3d::Identity();
    transform.linear() = rotationMatrix;
    transform.translation() = eye;

    return Transform(transform);
}

} // namespace core
} // namespace koomesh
