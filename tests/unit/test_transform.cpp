#include <gtest/gtest.h>
#include "core/Transform.h"
#include "helpers/TestUtils.h"
#include <cmath>

using namespace koomesh::core;

// ======================================================================
// Transform Basic Tests
// ======================================================================

class TransformTest : public ::testing::Test {
protected:
    Transform transform;
    const double PI = 3.14159265358979323846;
};

TEST_F(TransformTest, DefaultConstruction) {
    Eigen::Vector3d point(1, 2, 3);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(point, result, 1e-10);
}

TEST_F(TransformTest, Translation) {
    transform.translate(Eigen::Vector3d(10, 20, 30));

    Eigen::Vector3d point(1, 2, 3);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(11, 22, 33), result, 1e-10);
}

TEST_F(TransformTest, MultipleTranslations) {
    transform.translate(Eigen::Vector3d(10, 0, 0));
    transform.translate(Eigen::Vector3d(0, 20, 0));
    transform.translate(Eigen::Vector3d(0, 0, 30));

    Eigen::Vector3d point(1, 2, 3);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(11, 22, 33), result, 1e-10);
}

TEST_F(TransformTest, UniformScale) {
    transform.scale(2.0);

    Eigen::Vector3d point(1, 2, 3);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(2, 4, 6), result, 1e-10);
}

TEST_F(TransformTest, NonUniformScale) {
    transform.scale(2.0, 3.0, 4.0);

    Eigen::Vector3d point(1, 2, 3);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(2, 6, 12), result, 1e-10);
}

TEST_F(TransformTest, RotationX) {
    // 90도 회전 (Z축 -> -Y축, right-hand rule)
    transform.rotate(Eigen::Vector3d::UnitX(), PI / 2.0);

    Eigen::Vector3d point(0, 0, 1);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(0, -1, 0), result, 1e-10);
}

TEST_F(TransformTest, RotationY) {
    // 90도 회전 (X축 -> Z축)
    transform.rotate(Eigen::Vector3d::UnitY(), PI / 2.0);

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(0, 0, -1), result, 1e-10);
}

TEST_F(TransformTest, RotationZ) {
    // 90도 회전 (X축 -> Y축)
    transform.rotate(Eigen::Vector3d::UnitZ(), PI / 2.0);

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(0, 1, 0), result, 1e-10);
}

TEST_F(TransformTest, EulerRotation) {
    // ZYX 오일러 각도
    transform.rotateEuler(0, 0, PI / 2.0);  // Z축 90도

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(0, 1, 0), result, 1e-10);
}

// ======================================================================
// Transform Composition Tests
// ======================================================================

TEST_F(TransformTest, TranslateAndScale) {
    // 순서: Scale -> Translate
    transform.scale(2.0);
    transform.translate(Eigen::Vector3d(10, 0, 0));

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = transform.apply(point);

    // (1*2) + 10 = 12
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(12, 0, 0), result, 1e-10);
}

TEST_F(TransformTest, RotateAndTranslate) {
    transform.rotate(Eigen::Vector3d::UnitZ(), PI / 2.0);
    transform.translate(Eigen::Vector3d(10, 0, 0));

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = transform.apply(point);

    // Rotate (1,0,0) -> (0,1,0), then translate
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(10, 1, 0), result, 1e-10);
}

TEST_F(TransformTest, CombineTransforms) {
    Transform t1;
    t1.translate(Eigen::Vector3d(10, 0, 0));

    Transform t2;
    t2.rotate(Eigen::Vector3d::UnitZ(), PI / 2.0);

    Transform combined = t1 * t2;

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = combined.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(10, 1, 0), result, 1e-10);
}

// ======================================================================
// Vector and Normal Transform Tests
// ======================================================================

TEST_F(TransformTest, ApplyVector) {
    transform.translate(Eigen::Vector3d(10, 20, 30));
    transform.scale(2.0);

    Eigen::Vector3d vector(1, 0, 0);
    Eigen::Vector3d result = transform.applyVector(vector);

    // Vector ignores translation, only scaled
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(2, 0, 0), result, 1e-10);
}

TEST_F(TransformTest, ApplyNormal) {
    transform.scale(2.0, 1.0, 1.0);  // Non-uniform scale

    Eigen::Vector3d normal(1, 0, 0);
    Eigen::Vector3d result = transform.applyNormal(normal);

    // Normal should be normalized
    EXPECT_FLOAT_NEAR(1.0, result.norm(), 1e-6);
}

// ======================================================================
// Inverse Transform Tests
// ======================================================================

TEST_F(TransformTest, InverseTranslation) {
    transform.translate(Eigen::Vector3d(10, 20, 30));

    Transform inverse = transform.inverse();

    Eigen::Vector3d point(11, 22, 33);
    Eigen::Vector3d result = inverse.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(1, 2, 3), result, 1e-10);
}

TEST_F(TransformTest, InverseScale) {
    transform.scale(2.0);

    Transform inverse = transform.inverse();

    Eigen::Vector3d point(2, 4, 6);
    Eigen::Vector3d result = inverse.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(1, 2, 3), result, 1e-10);
}

TEST_F(TransformTest, InverseRotation) {
    transform.rotate(Eigen::Vector3d::UnitZ(), PI / 2.0);

    Transform inverse = transform.inverse();

    Eigen::Vector3d point(0, 1, 0);
    Eigen::Vector3d result = inverse.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(1, 0, 0), result, 1e-10);
}

// ======================================================================
// Decomposition Tests
// ======================================================================

TEST_F(TransformTest, DecomposeTranslation) {
    transform.translate(Eigen::Vector3d(10, 20, 30));

    Eigen::Vector3d t = transform.translation();
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(10, 20, 30), t, 1e-10);
}

TEST_F(TransformTest, DecomposeScale) {
    transform.scale(2.0, 3.0, 4.0);

    Eigen::Vector3d s = transform.scaleFactors();
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(2, 3, 4), s, 1e-10);
}

TEST_F(TransformTest, DecomposeTRS) {
    Eigen::Vector3d expectedT(10, 20, 30);
    Eigen::Quaterniond expectedR(Eigen::AngleAxisd(PI / 4.0, Eigen::Vector3d::UnitZ()));
    Eigen::Vector3d expectedS(2, 3, 4);

    Transform t = Transform::compose(expectedT, expectedR, expectedS);

    Eigen::Vector3d outT, outS;
    Eigen::Quaterniond outR;
    t.decompose(outT, outR, outS);

    EXPECT_VECTOR_NEAR(expectedT, outT, 1e-10);
    EXPECT_VECTOR_NEAR(expectedS, outS, 1e-10);
    EXPECT_FLOAT_NEAR(expectedR.w(), outR.w(), 1e-6);
    EXPECT_FLOAT_NEAR(expectedR.x(), outR.x(), 1e-6);
    EXPECT_FLOAT_NEAR(expectedR.y(), outR.y(), 1e-6);
    EXPECT_FLOAT_NEAR(expectedR.z(), outR.z(), 1e-6);
}

// ======================================================================
// Interpolation Tests
// ======================================================================

TEST_F(TransformTest, InterpolateTranslation) {
    Transform t1;
    t1.translate(Eigen::Vector3d(0, 0, 0));

    Transform t2;
    t2.translate(Eigen::Vector3d(10, 10, 10));

    Transform mid = t1.interpolate(t2, 0.5);

    Eigen::Vector3d t = mid.translation();
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(5, 5, 5), t, 1e-10);
}

TEST_F(TransformTest, InterpolateScale) {
    Transform t1;
    t1.scale(1.0);

    Transform t2;
    t2.scale(3.0);

    Transform mid = t1.interpolate(t2, 0.5);

    Eigen::Vector3d s = mid.scaleFactors();
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(2, 2, 2), s, 1e-10);
}

TEST_F(TransformTest, InterpolateRotation) {
    Transform t1;
    t1.rotate(Eigen::Vector3d::UnitZ(), 0.0);

    Transform t2;
    t2.rotate(Eigen::Vector3d::UnitZ(), PI / 2.0);

    Transform mid = t1.interpolate(t2, 0.5);

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = mid.apply(point);

    // 45도 회전 결과
    double cos45 = std::cos(PI / 4.0);
    double sin45 = std::sin(PI / 4.0);
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(cos45, sin45, 0), result, 1e-6);
}

// ======================================================================
// LookAt Tests
// ======================================================================

TEST_F(TransformTest, LookAtBasic) {
    Transform lookAt = Transform::lookAt(
        Eigen::Vector3d(0, 0, 0),    // eye
        Eigen::Vector3d(0, 0, -1),   // target
        Eigen::Vector3d(0, 1, 0)     // up
    );

    Eigen::Vector3d t = lookAt.translation();
    EXPECT_VECTOR_NEAR(Eigen::Vector3d(0, 0, 0), t, 1e-10);
}

// ======================================================================
// Reset Tests
// ======================================================================

TEST_F(TransformTest, Reset) {
    transform.translate(Eigen::Vector3d(10, 20, 30));
    transform.scale(2.0);
    transform.rotate(Eigen::Vector3d::UnitZ(), PI / 2.0);

    transform.reset();

    Eigen::Vector3d point(1, 2, 3);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(point, result, 1e-10);
}

// ======================================================================
// Edge Cases
// ======================================================================

TEST_F(TransformTest, ZeroScale) {
    // Zero scale should be handled gracefully
    transform.scale(0.0);

    Eigen::Vector3d point(1, 2, 3);
    Eigen::Vector3d result = transform.apply(point);

    EXPECT_VECTOR_NEAR(Eigen::Vector3d(0, 0, 0), result, 1e-10);
}

TEST_F(TransformTest, VerySmallRotation) {
    transform.rotate(Eigen::Vector3d::UnitZ(), 1e-10);

    Eigen::Vector3d point(1, 0, 0);
    Eigen::Vector3d result = transform.apply(point);

    // Should be nearly unchanged
    EXPECT_VECTOR_NEAR(point, result, 1e-9);
}

// ======================================================================
// Main
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
