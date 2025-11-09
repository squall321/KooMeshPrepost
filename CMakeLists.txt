cmake_minimum_required(VERSION 3.20)
project(KooMeshPrepost
    VERSION 1.0.0
    DESCRIPTION "High-Performance Mesh Visualization and Preprocessing Tool"
    LANGUAGES CXX
)

# ============================================================================
# Project Options
# ============================================================================

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_TESTS "Build unit and integration tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_DOCS "Build documentation (Doxygen)" OFF)
option(ENABLE_PROFILING "Enable profiling support" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers (address, undefined)" OFF)
option(ENABLE_LTO "Enable Link-Time Optimization" OFF)
option(USE_SYSTEM_LIBRARIES "Use system libraries instead of bundled ones" ON)

# ============================================================================
# C++ Standard and Compiler Settings
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 빌드 타입 기본값
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# ============================================================================
# Compiler Flags
# ============================================================================

# 공통 컴파일러 플래그
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# GCC/Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Release 빌드 최적화
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

    # Debug 빌드
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")

    # 경고 설정
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wcast-qual
        -Wformat=2
        -Wundef
        -Werror=float-equal
        -Wshadow
        -Wcast-align
        -Wunused
        -Wnull-dereference
        -Wdouble-promotion
        -Wimplicit-fallthrough
    )

    # Clang 전용
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wmost)
    endif()

# MSVC
elseif(MSVC)
    # Release 빌드
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG")

    # Debug 빌드
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /DDEBUG")

    # 경고 설정
    add_compile_options(
        /W4
        /permissive-
        /Zc:__cplusplus
    )

    # UTF-8 소스 코드
    add_compile_options(/utf-8)
endif()

# ============================================================================
# Sanitizers (Debug 빌드에서만)
# ============================================================================

if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling sanitizers: address, undefined")
        add_compile_options(-fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address -fsanitize=undefined)
    endif()
endif()

# ============================================================================
# Link-Time Optimization
# ============================================================================

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
    if(ipo_supported)
        message(STATUS "Link-Time Optimization (LTO) enabled")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "LTO not supported: ${ipo_error}")
    endif()
endif()

# ============================================================================
# Output Directories
# ============================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Multi-config generators (Visual Studio, Xcode)
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${CONFIG})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib/${CONFIG})
endforeach()

# ============================================================================
# Dependencies
# ============================================================================

# Qt6
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 QUIET COMPONENTS Core Widgets OpenGLWidgets)
if(Qt6_FOUND)
    message(STATUS "Found Qt6: ${Qt6_VERSION}")
else()
    message(WARNING "Qt6 not found - UI will not be built")
    set(BUILD_GUI OFF)
endif()

# VTK
find_package(VTK 9.0 QUIET COMPONENTS
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
if(VTK_FOUND)
    message(STATUS "Found VTK: ${VTK_VERSION}")
else()
    message(WARNING "VTK not found - visualization features will be limited")
    set(BUILD_VISUALIZATION OFF)
endif()

# Eigen3 (필수)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
message(STATUS "Found Eigen3: ${Eigen3_VERSION}")

# Intel TBB
find_package(TBB QUIET)
if(TBB_FOUND)
    message(STATUS "Found TBB: ${TBB_VERSION}")
else()
    message(WARNING "TBB not found - parallel processing will be disabled")
    set(ENABLE_TBB OFF)
endif()

# Boost
find_package(Boost 1.70 QUIET COMPONENTS filesystem system)
if(Boost_FOUND)
    message(STATUS "Found Boost: ${Boost_VERSION}")
else()
    message(WARNING "Boost not found - some file utilities will be limited")
endif()

# Google Test (테스트 빌드 시)
if(BUILD_TESTS)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "Google Test not found - downloading from GitHub")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.12.1
        )
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

# ============================================================================
# Include Directories
# ============================================================================

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# ============================================================================
# Subdirectories
# ============================================================================

# 소스 코드
add_subdirectory(src)

# 테스트
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 예제
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 문서
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    else()
        message(WARNING "Doxygen not found - documentation will not be built")
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

# 실행 파일
install(TARGETS KooMeshPrepost
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 헤더 파일 (라이브러리 사용 시)
install(DIRECTORY include/
    DESTINATION include/koomesh
    FILES_MATCHING PATTERN "*.h"
)

# 설정 파일
install(FILES README.md LICENSE
    DESTINATION share/doc/koomesh
)

# ============================================================================
# Packaging (CPack)
# ============================================================================

set(CPACK_PACKAGE_NAME "KooMeshPrepost")
set(CPACK_PACKAGE_VENDOR "Koo Engineering")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-Performance Mesh Visualization and Preprocessing Tool")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "KooMeshPrepost")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "KooMeshPrepost")
    set(CPACK_NSIS_PACKAGE_NAME "KooMeshPrepost")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://kooengineering.com")
    set(CPACK_NSIS_HELP_LINK "https://docs.kooengineering.com")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_VOLUME_NAME "KooMeshPrepost")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")

    # Debian
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Koo Engineering <support@kooengineering.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "science")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6widgets6, libvtk9, libeigen3-dev")

    # RPM
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Engineering")
    set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, vtk, eigen3")
endif()

include(CPack)

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  KooMeshPrepost Configuration Summary")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
message(STATUS "Options:")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "  BUILD_DOCS: ${BUILD_DOCS}")
message(STATUS "  ENABLE_SANITIZERS: ${ENABLE_SANITIZERS}")
message(STATUS "  ENABLE_LTO: ${ENABLE_LTO}")
message(STATUS "")
message(STATUS "Dependencies:")
if(Qt6_FOUND)
    message(STATUS "  Qt6: ${Qt6_VERSION}")
else()
    message(STATUS "  Qt6: NOT FOUND")
endif()
if(VTK_FOUND)
    message(STATUS "  VTK: ${VTK_VERSION}")
else()
    message(STATUS "  VTK: NOT FOUND")
endif()
message(STATUS "  Eigen3: ${Eigen3_VERSION}")
if(TBB_FOUND)
    message(STATUS "  TBB: ${TBB_VERSION}")
else()
    message(STATUS "  TBB: NOT FOUND")
endif()
if(Boost_FOUND)
    message(STATUS "  Boost: ${Boost_VERSION}")
else()
    message(STATUS "  Boost: NOT FOUND")
endif()
message(STATUS "")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "")
