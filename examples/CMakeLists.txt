# Examples CMakeLists.txt

# 각 예제 디렉토리
set(EXAMPLES
    01_basic_loading
    02_element_selection
    03_group_management
    04_visualization
)

# 공통 예제 라이브러리 (샘플 데이터 생성기 등)
add_library(example_common STATIC
    common/SampleMeshGenerator.cpp
)

target_link_libraries(example_common
    PUBLIC koomesh_lib
)

target_include_directories(example_common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# 각 예제 빌드 (향후 구현)
foreach(example ${EXAMPLES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${example}/main.cpp)
        add_executable(${example}
            ${example}/main.cpp
        )

        target_link_libraries(${example}
            PRIVATE
                koomesh_lib
                example_common
        )

        # 예제 설치
        install(TARGETS ${example}
            RUNTIME DESTINATION share/koomesh/examples
        )
    endif()
endforeach()

# Spatial Index Demo (Phase 50)
add_executable(spatial_index_demo
    10_spatial_indexes/spatial_index_demo.cpp
)

target_link_libraries(spatial_index_demo
    PRIVATE
        koomesh_lib
)

# 샘플 데이터 파일 설치
install(DIRECTORY sample_data/
    DESTINATION share/koomesh/examples/sample_data
    FILES_MATCHING PATTERN "*.k" PATTERN "*.key"
)
