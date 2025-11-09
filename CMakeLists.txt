cmake_minimum_required(VERSION 3.20)
project(KooMeshPrepost VERSION 1.0.0 LANGUAGES CXX)

# C++17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 빌드 타입 기본값
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 컴파일러 최적화 옵션
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
endif()

# 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Qt 자동 처리
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 필수 패키지 찾기
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGLWidgets)
find_package(VTK 9.0 REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    FiltersCore
    FiltersGeometry
    InteractionStyle
    RenderingCore
    RenderingOpenGL2
    RenderingAnnotation
    GUISupportQt
    IOLegacy
)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(TBB REQUIRED)
find_package(Boost 1.70 REQUIRED COMPONENTS filesystem system)

# 프로젝트 인클루드 디렉토리
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 소스 파일 수집
set(CORE_SOURCES
    src/core/Node.cpp
    src/core/Element.cpp
    src/core/Mesh.cpp
    src/core/Part.cpp
    src/core/Group.cpp
    src/core/GroupManager.cpp
    src/core/ConfigManager.cpp
    src/core/MeshValidator.cpp
)

set(IO_SOURCES
    src/io/DynaFileReader.cpp
    src/io/DynaFileWriter.cpp
    src/io/FileFormatDetector.cpp
    src/io/AsyncFileLoader.cpp
)

set(VISUALIZATION_SOURCES
    src/visualization/VTKRenderer.cpp
    src/visualization/MeshToVTK.cpp
    src/visualization/CameraController.cpp
    src/visualization/ActorManager.cpp
    src/visualization/SelectionHighlighter.cpp
    src/visualization/AreaSelector.cpp
)

set(SELECTION_SOURCES
    src/selection/SelectionManager.cpp
    src/selection/Octree.cpp
    src/selection/SpatialIndexFactory.cpp
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/PartsPanel.cpp
    src/ui/GroupsPanel.cpp
    src/ui/PropertiesPanel.cpp
    src/ui/SelectionToolBar.cpp
    src/ui/ProgressDialog.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
    src/utils/BoundingBox.cpp
)

# 헤더 파일
set(HEADERS
    include/core/Node.h
    include/core/Element.h
    include/core/Mesh.h
    include/core/Part.h
    include/core/Group.h
    include/ui/MainWindow.h
)

# 메인 실행 파일
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${CORE_SOURCES}
    ${IO_SOURCES}
    ${VISUALIZATION_SOURCES}
    ${SELECTION_SOURCES}
    ${UI_SOURCES}
    ${UTILS_SOURCES}
    ${HEADERS}
)

# VTK 모듈 포함
vtk_module_autoinit(
    TARGETS ${PROJECT_NAME}
    MODULES ${VTK_LIBRARIES}
)

# 라이브러리 링크
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGLWidgets
        ${VTK_LIBRARIES}
        Eigen3::Eigen
        TBB::tbb
        Boost::filesystem
        Boost::system
)

# 컴파일 옵션
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# 인클루드 디렉토리
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${VTK_INCLUDE_DIRS}
)

# 설치 규칙
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 테스트 활성화
enable_testing()
add_subdirectory(tests)

# CPack 패키징
set(CPACK_PACKAGE_NAME "KooMeshPrepost")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Koo Engineering")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-Performance Mesh Preprocessing Tool")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
else()
    set(CPACK_GENERATOR "DEB;RPM")
endif()

include(CPack)

# 개발 정보 출력
message(STATUS "========================================")
message(STATUS "KooMeshPrepost Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Qt version: ${Qt6_VERSION}")
message(STATUS "VTK version: ${VTK_VERSION}")
message(STATUS "Eigen version: ${Eigen3_VERSION}")
message(STATUS "========================================")
