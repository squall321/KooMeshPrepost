# ============================================================================
# Source Files Collection
# ============================================================================

# Core module
set(CORE_SOURCES
    core/Node.cpp
    core/Element.cpp
    core/Mesh.cpp
    core/Part.cpp
    core/Group.cpp
    core/GroupManager.cpp
    core/Transform.cpp
    core/MeshValidator.cpp
    core/MeshStatistics.cpp
    core/Octree.cpp
    core/RTree.cpp
    core/KdTree.cpp
    core/UniformGrid.cpp
    core/SpatialIndexFactory.cpp
    core/SpatialIndexBenchmark.cpp
    core/SpatialQueryOptimizer.cpp
    core/PerformanceDocGenerator.cpp
    core/ParallelIndexBuilder.cpp
    core/SpatialIndexSerializer.cpp
)

set(CORE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/core/Node.h
    ${CMAKE_SOURCE_DIR}/include/core/Element.h
    ${CMAKE_SOURCE_DIR}/include/core/Mesh.h
    ${CMAKE_SOURCE_DIR}/include/core/Part.h
    ${CMAKE_SOURCE_DIR}/include/core/Group.h
    ${CMAKE_SOURCE_DIR}/include/core/GroupManager.h
    ${CMAKE_SOURCE_DIR}/include/core/Transform.h
    ${CMAKE_SOURCE_DIR}/include/core/MeshValidator.h
    ${CMAKE_SOURCE_DIR}/include/core/MeshStatistics.h
    ${CMAKE_SOURCE_DIR}/include/core/ISpatialIndex.h
    ${CMAKE_SOURCE_DIR}/include/core/Octree.h
    ${CMAKE_SOURCE_DIR}/include/core/RTree.h
    ${CMAKE_SOURCE_DIR}/include/core/KdTree.h
    ${CMAKE_SOURCE_DIR}/include/core/UniformGrid.h
    ${CMAKE_SOURCE_DIR}/include/core/SpatialIndexFactory.h
    ${CMAKE_SOURCE_DIR}/include/core/SpatialIndexBenchmark.h
    ${CMAKE_SOURCE_DIR}/include/core/SpatialQueryOptimizer.h
    ${CMAKE_SOURCE_DIR}/include/core/PerformanceDocGenerator.h
    ${CMAKE_SOURCE_DIR}/include/core/ParallelIndexBuilder.h
    ${CMAKE_SOURCE_DIR}/include/core/SpatialIndexSerializer.h
)

# I/O module
set(IO_SOURCES
    io/FileIO.cpp
    io/MemoryMappedFile.cpp
    io/LSDynaKeywordParser.cpp
    io/LSDynaFileReader.cpp
    io/ChunkedFileReader.cpp
    io/ErrorHandler.cpp
    io/LSDynaFileWriter.cpp
    io/STLFileReader.cpp
    io/STLFileWriter.cpp
    io/VTKFileReader.cpp
    io/VTKFileWriter.cpp
    io/NastranFileReader.cpp
    io/NastranFileWriter.cpp
    io/FileFormatDetector.cpp
    io/FileReaderFactory.cpp
    io/AsyncFileLoader.cpp
)

set(IO_HEADERS
    ${CMAKE_SOURCE_DIR}/include/io/FileIOException.h
    ${CMAKE_SOURCE_DIR}/include/io/IFileReader.h
    ${CMAKE_SOURCE_DIR}/include/io/IFileWriter.h
    ${CMAKE_SOURCE_DIR}/include/io/MemoryMappedFile.h
    ${CMAKE_SOURCE_DIR}/include/io/LSDynaKeywordParser.h
    ${CMAKE_SOURCE_DIR}/include/io/LSDynaFileReader.h
    ${CMAKE_SOURCE_DIR}/include/io/ChunkedFileReader.h
    ${CMAKE_SOURCE_DIR}/include/io/ErrorHandler.h
    ${CMAKE_SOURCE_DIR}/include/io/LSDynaFileWriter.h
    ${CMAKE_SOURCE_DIR}/include/io/STLFileReader.h
    ${CMAKE_SOURCE_DIR}/include/io/STLFileWriter.h
    ${CMAKE_SOURCE_DIR}/include/io/VTKFileReader.h
    ${CMAKE_SOURCE_DIR}/include/io/VTKFileWriter.h
    ${CMAKE_SOURCE_DIR}/include/io/NastranFileReader.h
    ${CMAKE_SOURCE_DIR}/include/io/NastranFileWriter.h
    ${CMAKE_SOURCE_DIR}/include/io/FileFormatDetector.h
    ${CMAKE_SOURCE_DIR}/include/io/FileReaderFactory.h
    ${CMAKE_SOURCE_DIR}/include/io/AsyncFileLoader.h
)

# Visualization module
set(VISUALIZATION_SOURCES
    visualization/VTKRenderer.cpp
    visualization/MeshToVTK.cpp
    visualization/CameraController.cpp
    visualization/ActorManager.cpp
    visualization/SelectionHighlighter.cpp
    visualization/AreaSelector.cpp
    visualization/ColorMapper.cpp
    visualization/LightingManager.cpp
)

set(VISUALIZATION_HEADERS
    ${CMAKE_SOURCE_DIR}/include/visualization/VTKRenderer.h
    ${CMAKE_SOURCE_DIR}/include/visualization/MeshToVTK.h
)

# Selection module
set(SELECTION_SOURCES
    selection/SelectionManager.cpp
)

set(SELECTION_HEADERS
    ${CMAKE_SOURCE_DIR}/include/selection/SelectionManager.h
)

# UI module
set(UI_SOURCES
    ui/MainWindow.cpp
    ui/PartsPanel.cpp
    ui/GroupsPanel.cpp
    ui/PropertiesPanel.cpp
    ui/SelectionToolBar.cpp
    ui/ProgressDialog.cpp
    ui/SettingsDialog.cpp
)

set(UI_HEADERS
    ${CMAKE_SOURCE_DIR}/include/ui/MainWindow.h
    ${CMAKE_SOURCE_DIR}/include/ui/PartsPanel.h
)

# Utils module
set(UTILS_SOURCES
    utils/Logger.cpp
    utils/ConfigManager.cpp
    utils/Timer.cpp
    utils/FileUtils.cpp
    utils/StringUtils.cpp
    utils/MathUtils.cpp
)

set(UTILS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/utils/Logger.h
    ${CMAKE_SOURCE_DIR}/include/utils/ConfigManager.h
    ${CMAKE_SOURCE_DIR}/include/utils/Exception.h
)

# ============================================================================
# Library Target
# ============================================================================

# 라이브러리 생성 (실행 파일과 테스트에서 재사용)
add_library(koomesh_lib
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${IO_SOURCES}
    ${IO_HEADERS}
    ${UTILS_SOURCES}
    ${UTILS_HEADERS}
)

# Visualization 모듈 (VTK 사용 가능한 경우만)
if(VTK_FOUND)
    target_sources(koomesh_lib PRIVATE
        ${VISUALIZATION_SOURCES}
        ${VISUALIZATION_HEADERS}
    )
endif()

# Selection 모듈
target_sources(koomesh_lib PRIVATE
    ${SELECTION_SOURCES}
    ${SELECTION_HEADERS}
)

# UI 모듈 (Qt 사용 가능한 경우만)
if(Qt6_FOUND)
    target_sources(koomesh_lib PRIVATE
        ${UI_SOURCES}
        ${UI_HEADERS}
    )
endif()

# 라이브러리 인클루드 디렉토리
target_include_directories(koomesh_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/koomesh>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# 라이브러리 의존성
target_link_libraries(koomesh_lib
    PUBLIC
        Eigen3::Eigen
    PRIVATE
        $<$<BOOL:${Boost_FOUND}>:Boost::filesystem Boost::system>
        $<$<BOOL:${TBB_FOUND}>:TBB::tbb>
)

# VTK 링크
if(VTK_FOUND)
    target_link_libraries(koomesh_lib PRIVATE ${VTK_LIBRARIES})
    target_include_directories(koomesh_lib PRIVATE ${VTK_INCLUDE_DIRS})

    # VTK 모듈 자동 초기화
    vtk_module_autoinit(
        TARGETS koomesh_lib
        MODULES ${VTK_LIBRARIES}
    )
endif()

# Qt 링크
if(Qt6_FOUND)
    target_link_libraries(koomesh_lib PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )
endif()

# ============================================================================
# Executable Target
# ============================================================================

# 메인 실행 파일
add_executable(KooMeshPrepost
    main.cpp
)

# 라이브러리 링크
target_link_libraries(KooMeshPrepost
    PRIVATE
        koomesh_lib
)

# ============================================================================
# Compiler Features and Definitions
# ============================================================================

# C++17 features
target_compile_features(koomesh_lib PUBLIC cxx_std_17)
target_compile_features(KooMeshPrepost PRIVATE cxx_std_17)

# 조건부 컴파일 정의
if(VTK_FOUND)
    target_compile_definitions(koomesh_lib PUBLIC KOOMESH_HAS_VTK)
endif()

if(Qt6_FOUND)
    target_compile_definitions(koomesh_lib PUBLIC KOOMESH_HAS_QT)
endif()

if(TBB_FOUND)
    target_compile_definitions(koomesh_lib PUBLIC KOOMESH_HAS_TBB)
endif()

if(Boost_FOUND)
    target_compile_definitions(koomesh_lib PUBLIC KOOMESH_HAS_BOOST)
endif()

# Debug/Release 정의
target_compile_definitions(koomesh_lib PUBLIC
    $<$<CONFIG:Debug>:KOOMESH_DEBUG>
    $<$<CONFIG:Release>:KOOMESH_RELEASE>
)

# ============================================================================
# Properties
# ============================================================================

# Position Independent Code (공유 라이브러리용)
set_target_properties(koomesh_lib PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# 버전 정보
set_target_properties(KooMeshPrepost PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Output name
set_target_properties(KooMeshPrepost PROPERTIES
    OUTPUT_NAME "koomesh"
)

# ============================================================================
# Installation
# ============================================================================

# 라이브러리 설치
install(TARGETS koomesh_lib
    EXPORT KooMeshPrepostTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include/koomesh
)

# Export targets (다른 프로젝트에서 사용 가능)
install(EXPORT KooMeshPrepostTargets
    FILE KooMeshPrepostTargets.cmake
    NAMESPACE KooMesh::
    DESTINATION lib/cmake/KooMeshPrepost
)

# CMake config 파일
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/KooMeshPrepostConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KooMeshPrepostConfig.cmake
    INSTALL_DESTINATION lib/cmake/KooMeshPrepost
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/KooMeshPrepostConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/KooMeshPrepostConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KooMeshPrepostConfigVersion.cmake
    DESTINATION lib/cmake/KooMeshPrepost
)
