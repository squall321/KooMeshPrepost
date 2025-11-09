# 테스트 CMake 설정

# Google Test 찾기 (메인 CMakeLists.txt에서 이미 처리됨)
if(NOT TARGET GTest::GTest)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "Google Test already configured via FetchContent")
    endif()
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# 테스트 헬퍼 라이브러리 (향후 구현)
# add_library(test_helpers STATIC
#     helpers/MeshBuilder.cpp
#     helpers/SampleData.cpp
# )
#
# target_link_libraries(test_helpers
#     PRIVATE
#         koomesh_lib
# )

# ======================================================================
# 단위 테스트
# ======================================================================

# Logger 테스트
add_executable(test_logger
    unit/test_logger.cpp
)

target_link_libraries(test_logger
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

add_test(NAME LoggerTests COMMAND test_logger)
set_tests_properties(LoggerTests PROPERTIES TIMEOUT 30)

# ConfigManager 테스트
add_executable(test_config
    unit/test_config.cpp
)

target_link_libraries(test_config
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

add_test(NAME ConfigTests COMMAND test_config)
set_tests_properties(ConfigTests PROPERTIES TIMEOUT 30)

# Exception 테스트
add_executable(test_exception
    unit/test_exception.cpp
)

target_link_libraries(test_exception
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

add_test(NAME ExceptionTests COMMAND test_exception)
set_tests_properties(ExceptionTests PROPERTIES TIMEOUT 30)

# Core 테스트 (Node, Element, Mesh, Part)
add_executable(test_core
    unit/test_core.cpp
)

target_link_libraries(test_core
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

add_test(NAME CoreTests COMMAND test_core)
set_tests_properties(CoreTests PROPERTIES TIMEOUT 30)

# MemoryPool 테스트
add_executable(test_memory_pool
    unit/test_memory_pool.cpp
)

target_link_libraries(test_memory_pool
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

add_test(NAME MemoryPoolTests COMMAND test_memory_pool)
set_tests_properties(MemoryPoolTests PROPERTIES TIMEOUT 30)

# 향후 추가될 테스트들 (현재는 주석 처리)
# add_executable(test_mesh
#     unit/test_mesh.cpp
#     unit/test_node.cpp
#     unit/test_element.cpp
# )
#
# target_link_libraries(test_mesh
#     PRIVATE
#         koomesh_lib
#         GTest::GTest
#         GTest::Main
# )
#
# add_executable(test_octree
#     unit/test_octree.cpp
#     unit/test_spatial_index.cpp
# )
#
# target_link_libraries(test_octree
#     PRIVATE
#         koomesh_lib
#         GTest::GTest
#         GTest::Main
# )
#
# add_executable(test_file_io
#     unit/test_dyna_reader.cpp
#     unit/test_dyna_writer.cpp
# )
#
# target_link_libraries(test_file_io
#     PRIVATE
#         koomesh_lib
#         GTest::GTest
#         GTest::Main
# )

# ======================================================================
# 통합 테스트 (향후 구현)
# ======================================================================

# add_executable(integration_tests
#     integration/test_full_workflow.cpp
#     integration/test_large_mesh.cpp
# )
#
# target_link_libraries(integration_tests
#     PRIVATE
#         koomesh_lib
#         GTest::GTest
#         GTest::Main
# )
#
# add_test(NAME IntegrationTests COMMAND integration_tests)
# set_tests_properties(IntegrationTests PROPERTIES TIMEOUT 300)
