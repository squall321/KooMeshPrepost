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

# Helpers 예제 테스트
add_executable(test_helpers_example
    unit/test_helpers_example.cpp
)

target_link_libraries(test_helpers_example
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_helpers_example
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME HelpersExampleTests COMMAND test_helpers_example)
set_tests_properties(HelpersExampleTests PROPERTIES TIMEOUT 30)

# Additional Element Types 테스트
add_executable(test_additional_elements
    unit/test_additional_elements.cpp
)

target_link_libraries(test_additional_elements
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_additional_elements
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME AdditionalElementsTests COMMAND test_additional_elements)
set_tests_properties(AdditionalElementsTests PROPERTIES TIMEOUT 30)

# Part 테스트
add_executable(test_part
    unit/test_part.cpp
)

target_link_libraries(test_part
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_part
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME PartTests COMMAND test_part)
set_tests_properties(PartTests PROPERTIES TIMEOUT 30)

# Group 및 GroupManager 테스트
add_executable(test_group
    unit/test_group.cpp
)

target_link_libraries(test_group
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_group
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME GroupTests COMMAND test_group)
set_tests_properties(GroupTests PROPERTIES TIMEOUT 30)

# Transform 테스트
add_executable(test_transform
    unit/test_transform.cpp
)

target_link_libraries(test_transform
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_transform
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME TransformTests COMMAND test_transform)
set_tests_properties(TransformTests PROPERTIES TIMEOUT 30)

# MeshValidator 및 MeshStatistics 테스트
add_executable(test_mesh_analysis
    unit/test_mesh_analysis.cpp
)

target_link_libraries(test_mesh_analysis
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_mesh_analysis
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME MeshAnalysisTests COMMAND test_mesh_analysis)
set_tests_properties(MeshAnalysisTests PROPERTIES TIMEOUT 30)

# File I/O 인터페이스 테스트
add_executable(test_file_io_interfaces
    unit/io/test_file_io_interfaces.cpp
)

target_link_libraries(test_file_io_interfaces
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_file_io_interfaces
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME FileIOInterfacesTests COMMAND test_file_io_interfaces)
set_tests_properties(FileIOInterfacesTests PROPERTIES TIMEOUT 60)

# Memory Mapped File 테스트
add_executable(test_memory_mapped_file
    unit/io/test_memory_mapped_file.cpp
)

target_link_libraries(test_memory_mapped_file
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_memory_mapped_file
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME MemoryMappedFileTests COMMAND test_memory_mapped_file)
set_tests_properties(MemoryMappedFileTests PROPERTIES TIMEOUT 60)

# LS-DYNA Keyword Parser 테스트
add_executable(test_lsdyna_keyword_parser
    unit/io/test_lsdyna_keyword_parser.cpp
)

target_link_libraries(test_lsdyna_keyword_parser
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_lsdyna_keyword_parser
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME LSDynaKeywordParserTests COMMAND test_lsdyna_keyword_parser)
set_tests_properties(LSDynaKeywordParserTests PROPERTIES TIMEOUT 60)

# LS-DYNA File Reader 테스트
add_executable(test_lsdyna_file_reader
    unit/io/test_lsdyna_file_reader.cpp
)

target_link_libraries(test_lsdyna_file_reader
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_lsdyna_file_reader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME LSDynaFileReaderTests COMMAND test_lsdyna_file_reader)
set_tests_properties(LSDynaFileReaderTests PROPERTIES TIMEOUT 60)

# Chunked File Reader 테스트
add_executable(test_chunked_file_reader
    unit/io/test_chunked_file_reader.cpp
)

target_link_libraries(test_chunked_file_reader
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_chunked_file_reader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME ChunkedFileReaderTests COMMAND test_chunked_file_reader)
set_tests_properties(ChunkedFileReaderTests PROPERTIES TIMEOUT 120)

# Error Handler 테스트
add_executable(test_error_handler
    unit/io/test_error_handler.cpp
)

target_link_libraries(test_error_handler
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_error_handler
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME ErrorHandlerTests COMMAND test_error_handler)
set_tests_properties(ErrorHandlerTests PROPERTIES TIMEOUT 60)

# File Format Detector 테스트 (Phase 39)
add_executable(test_file_format_detector
    unit/io/test_file_format_detector.cpp
)

target_link_libraries(test_file_format_detector
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_file_format_detector
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME FileFormatDetectorTests COMMAND test_file_format_detector)
set_tests_properties(FileFormatDetectorTests PROPERTIES TIMEOUT 60)

# File Reader Factory 테스트 (Phase 39)
add_executable(test_file_reader_factory
    unit/io/test_file_reader_factory.cpp
)

target_link_libraries(test_file_reader_factory
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_file_reader_factory
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME FileReaderFactoryTests COMMAND test_file_reader_factory)
set_tests_properties(FileReaderFactoryTests PROPERTIES TIMEOUT 60)

# Async File Loader 테스트 (Phase 39)
add_executable(test_async_file_loader
    unit/io/test_async_file_loader.cpp
)

target_link_libraries(test_async_file_loader
    PRIVATE
        koomesh_lib
        GTest::GTest
        GTest::Main
)

target_include_directories(test_async_file_loader
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME AsyncFileLoaderTests COMMAND test_async_file_loader)
set_tests_properties(AsyncFileLoaderTests PROPERTIES TIMEOUT 120)

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
