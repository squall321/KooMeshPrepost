# 테스트 CMake 설정

# Google Test 찾기
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 테스트 헬퍼 라이브러리
add_library(test_helpers STATIC
    helpers/MeshBuilder.cpp
    helpers/SampleData.cpp
)

target_link_libraries(test_helpers
    PRIVATE
        ${PROJECT_NAME}_lib
)

# 단위 테스트
add_executable(test_mesh
    unit/test_mesh.cpp
    unit/test_node.cpp
    unit/test_element.cpp
)

target_link_libraries(test_mesh
    PRIVATE
        ${PROJECT_NAME}_lib
        GTest::GTest
        GTest::Main
        test_helpers
)

add_executable(test_octree
    unit/test_octree.cpp
    unit/test_spatial_index.cpp
)

target_link_libraries(test_octree
    PRIVATE
        ${PROJECT_NAME}_lib
        GTest::GTest
        GTest::Main
        test_helpers
)

add_executable(test_file_io
    unit/test_dyna_reader.cpp
    unit/test_dyna_writer.cpp
)

target_link_libraries(test_file_io
    PRIVATE
        ${PROJECT_NAME}_lib
        GTest::GTest
        GTest::Main
        test_helpers
)

# 통합 테스트
add_executable(integration_tests
    integration/test_full_workflow.cpp
    integration/test_large_mesh.cpp
)

target_link_libraries(integration_tests
    PRIVATE
        ${PROJECT_NAME}_lib
        GTest::GTest
        GTest::Main
        test_helpers
)

# CTest에 테스트 추가
add_test(NAME MeshTests COMMAND test_mesh)
add_test(NAME OctreeTests COMMAND test_octree)
add_test(NAME FileIOTests COMMAND test_file_io)
add_test(NAME IntegrationTests COMMAND integration_tests)

# 테스트 타임아웃 설정
set_tests_properties(
    MeshTests OctreeTests FileIOTests
    PROPERTIES TIMEOUT 30
)

set_tests_properties(
    IntegrationTests
    PROPERTIES TIMEOUT 300
)
