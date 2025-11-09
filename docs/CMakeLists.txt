# Documentation CMakeLists.txt

find_package(Doxygen REQUIRED dot)

if(DOXYGEN_FOUND)
    # Doxyfile 설정
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/api)

    # Doxyfile에서 변수 치환
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # Doxygen 타겟 생성
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    # docs 폴더를 build 디렉토리에 생성
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

    message(STATUS "Doxygen documentation will be generated at: ${DOXYGEN_OUTPUT_DIR}")
    message(STATUS "Run 'make docs' or 'cmake --build . --target docs' to generate documentation")

    # 문서 설치
    install(DIRECTORY ${DOXYGEN_OUTPUT_DIR}/html
        DESTINATION share/doc/koomesh
        OPTIONAL
    )
else()
    message(WARNING "Doxygen not found - documentation will not be generated")
endif()
