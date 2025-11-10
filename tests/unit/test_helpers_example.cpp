#include <gtest/gtest.h>
#include "helpers/MeshBuilder.h"
#include "helpers/TestUtils.h"

using namespace koomesh::test;
using namespace koomesh::core;

/**
 * @brief 테스트 헬퍼 사용 예제
 *
 * 이 테스트는 MeshBuilder와 TestUtils의 사용법을 보여줍니다.
 */
class HelpersExampleTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ======================================================================
// MeshBuilder 예제
// ======================================================================

TEST_F(HelpersExampleTest, MeshBuilder_BuildSimpleMesh) {
    // MeshBuilder로 간단하게 메시 생성
    auto mesh = MeshBuilder()
        .addNode(1, 0.0, 0.0, 0.0)
        .addNode(2, 1.0, 0.0, 0.0)
        .addNode(3, 0.0, 1.0, 0.0)
        .addNode(4, 0.0, 0.0, 1.0)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .build();

    EXPECT_EQ(4, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());
}

TEST_F(HelpersExampleTest, SimpleMeshes_Tetrahedron) {
    // 미리 정의된 간단한 메시 사용
    auto mesh = SimpleMeshes::singleTetrahedron();

    EXPECT_EQ(4, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());

    // 요소 검증
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::TETRAHEDRON, elem->type());
}

TEST_F(HelpersExampleTest, SimpleMeshes_Hexahedron) {
    auto mesh = SimpleMeshes::singleHexahedron();

    EXPECT_EQ(8, mesh->nodeCount());
    EXPECT_EQ(1, mesh->elementCount());

    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);
    EXPECT_EQ(ElementType::HEXAHEDRON, elem->type());
}

// ======================================================================
// VectorAssert 예제
// ======================================================================

TEST_F(HelpersExampleTest, VectorAssert_Near) {
    Eigen::Vector3d v1(1.0, 2.0, 3.0);
    Eigen::Vector3d v2(1.0001, 2.0001, 3.0001);

    // 커스텀 tolerance 사용
    EXPECT_TRUE(VectorAssert::Near(v1, v2, 1e-3));

    // 기본 tolerance (1e-10)는 실패
    EXPECT_FALSE(VectorAssert::Near(v1, v2));
}

TEST_F(HelpersExampleTest, VectorAssert_Equal) {
    Eigen::Vector3d v(1.0, 2.0, 3.0);

    // 매크로 사용
    EXPECT_VECTOR_EQ(v, 1.0, 2.0, 3.0);
}

// ======================================================================
// FloatAssert 예제
// ======================================================================

TEST_F(HelpersExampleTest, FloatAssert_Near) {
    double a = 3.14159;
    double b = 3.14160;

    EXPECT_TRUE(FloatAssert::Near(a, b, 1e-4));
    EXPECT_FLOAT_NEAR(a, b, 1e-4);
}

// ======================================================================
// StringUtils 예제
// ======================================================================

TEST_F(HelpersExampleTest, StringUtils_Contains) {
    std::string str = "Hello, World!";

    EXPECT_STRING_CONTAINS(str, "World");
    EXPECT_STRING_CONTAINS(str, "Hello");
    EXPECT_FALSE(StringUtils::Contains(str, "Goodbye").operator bool());
}

TEST_F(HelpersExampleTest, StringUtils_StartsWith) {
    std::string str = "prefix_value";

    EXPECT_TRUE(StringUtils::StartsWith(str, "prefix"));
    EXPECT_FALSE(StringUtils::StartsWith(str, "value").operator bool());
}

// ======================================================================
// FileUtils 예제
// ======================================================================

TEST_F(HelpersExampleTest, FileUtils_WriteAndRead) {
    std::string tempPath = FileUtils::tempFilePath();

    // 파일 쓰기
    std::string content = "Test content\nLine 2";
    EXPECT_TRUE(FileUtils::writeFile(tempPath, content));

    // 파일 존재 확인
    EXPECT_TRUE(FileUtils::exists(tempPath));

    // 파일 읽기
    std::string readContent = FileUtils::readFile(tempPath);
    EXPECT_EQ(content, readContent);

    // 파일 삭제
    FileUtils::remove(tempPath);
    EXPECT_FALSE(FileUtils::exists(tempPath));
}

// ======================================================================
// CollectionUtils 예제
// ======================================================================

TEST_F(HelpersExampleTest, CollectionUtils_Contains) {
    std::vector<int> vec = {1, 2, 3, 4, 5};

    EXPECT_COLLECTION_CONTAINS(vec, 3);
    EXPECT_FALSE(CollectionUtils::Contains(vec, 10).operator bool());
}

TEST_F(HelpersExampleTest, CollectionUtils_UnorderedEqual) {
    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::vector<int> vec2 = {5, 4, 3, 2, 1};
    std::vector<int> vec3 = {1, 2, 3, 4, 6};

    EXPECT_TRUE(CollectionUtils::UnorderedEqual(vec1, vec2));
    EXPECT_FALSE(CollectionUtils::UnorderedEqual(vec1, vec3).operator bool());
}

// ======================================================================
// PerformanceTimer 예제
// ======================================================================

TEST_F(HelpersExampleTest, PerformanceTimer_Measure) {
    PerformanceTimer timer;

    // 시간이 걸리는 작업 시뮬레이션
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    long long elapsed = timer.elapsedMilliseconds();

    // 최소 10ms 이상 걸림
    EXPECT_GE(elapsed, 10);
    EXPECT_LT(elapsed, 100);  // 너무 오래 걸리면 안 됨
}

TEST_F(HelpersExampleTest, ScopedTimer_AutomaticMeasurement) {
    // 스코프 내에서 자동으로 시간 측정 및 출력
    {
        SCOPED_TIMER("Test operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }  // 스코프 종료 시 자동으로 시간 출력
}

// ======================================================================
// 통합 예제: 실제 사용 시나리오
// ======================================================================

TEST_F(HelpersExampleTest, RealScenario_MeshQualityCheck) {
    // MeshBuilder로 메시 생성
    auto mesh = MeshBuilder()
        .addNode(1, 0.0, 0.0, 0.0)
        .addNode(2, 1.0, 0.0, 0.0)
        .addNode(3, 0.0, 1.0, 0.0)
        .addNode(4, 0.0, 0.0, 1.0)
        .addTetrahedron(1, 1, {1, 2, 3, 4})
        .build();

    // 요소 품질 계산
    const Element* elem = mesh->getElement(1);
    ASSERT_NE(nullptr, elem);

    double quality = elem->computeQuality(*mesh);

    // 품질은 0.0 ~ 1.0 범위
    EXPECT_GE(quality, 0.0);
    EXPECT_LE(quality, 1.0);

    // 중심 계산
    Eigen::Vector3d center = elem->computeCenter(*mesh);

    // 사면체 중심은 대략 (0.25, 0.25, 0.25)
    EXPECT_VECTOR_NEAR(center, Eigen::Vector3d(0.25, 0.25, 0.25), 1e-10);
}

TEST_F(HelpersExampleTest, RealScenario_PerformanceBenchmark) {
    PerformanceTimer timer;

    // 대량의 노드 추가 성능 측정
    MeshBuilder builder;
    auto& mesh = builder.get();

    for (int i = 0; i < 10000; ++i) {
        mesh.addNode(Node(i, i * 1.0, 0.0, 0.0));
    }

    long long elapsed = timer.elapsedMicroseconds();

    std::cout << "Added 10,000 nodes in " << elapsed << " μs\n";

    // 성능 기준 확인 (10,000 노드 추가가 100ms 이내)
    EXPECT_LT(elapsed, 100000);  // 100,000 μs = 100 ms
}

// ======================================================================
// 메인 함수
// ======================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
