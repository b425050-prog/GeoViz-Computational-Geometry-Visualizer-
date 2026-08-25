# GeoViz - Computational Geometry Visualizer
# A C++20 desktop application submitted for the IIIT Bhubaneswar OOPS Lab.
cmake_minimum_required(VERSION 3.20)

project(
    GeoViz
    VERSION 1.0.1
    DESCRIPTION "Interactive object-oriented computational geometry visualizer"
    LANGUAGES CXX
)

option(GEOVIZ_BUILD_APP "Build the raylib graphical desktop application" ON)
option(GEOVIZ_BUILD_TESTS "Build the dependency-free algorithm test suite" ON)
option(GEOVIZ_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(GEOVIZ_CORE_SOURCES
    src/geometry/Primitives.cpp
    src/algorithms/Predicates.cpp
    src/algorithms/ConvexHull.cpp
    src/algorithms/Delaunay.cpp
    src/algorithms/Voronoi.cpp
    src/algorithms/HalfPlaneIntersection.cpp
    src/algorithms/PolygonAlgorithms.cpp
    src/app/SceneModel.cpp
    src/app/Commands.cpp
    src/app/AlgorithmDemo.cpp
    src/io/SceneSerializer.cpp
)

add_library(geoviz_core STATIC ${GEOVIZ_CORE_SOURCES})
add_library(GeoViz::Core ALIAS geoviz_core)
target_include_directories(
    geoviz_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(geoviz_core PUBLIC cxx_std_20)

function(geoviz_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /EHsc)
        if(GEOVIZ_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(GEOVIZ_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

geoviz_enable_warnings(geoviz_core)

if(GEOVIZ_BUILD_APP)
    # Prefer an installed raylib package.  If none exists, CMake downloads the
    # pinned stable release automatically during configuration.
    find_package(raylib 5.5 QUIET)
    if(NOT raylib_FOUND)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        set(FETCHCONTENT_QUIET FALSE)
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(raylib)
    endif()

    if(WIN32)
        # WIN32 selects the GUI subsystem, so opening GeoViz does not create a
        # second terminal window on Windows.
        add_executable(GeoViz WIN32
            src/main.cpp
            src/app/Application.cpp
            src/app/Renderer.cpp
        )
    else()
        add_executable(GeoViz
            src/main.cpp
            src/app/Application.cpp
            src/app/Renderer.cpp
        )
    endif()
    target_link_libraries(GeoViz PRIVATE GeoViz::Core raylib)
    target_compile_features(GeoViz PRIVATE cxx_std_20)
    geoviz_enable_warnings(GeoViz)

    install(TARGETS GeoViz RUNTIME DESTINATION bin)
endif()

if(GEOVIZ_BUILD_TESTS)
    include(CTest)
    add_executable(geoviz_tests tests/test_main.cpp)
    target_link_libraries(geoviz_tests PRIVATE GeoViz::Core)
    target_compile_features(geoviz_tests PRIVATE cxx_std_20)
    geoviz_enable_warnings(geoviz_tests)
    add_test(NAME geoviz_algorithms COMMAND geoviz_tests)
endif()

install(DIRECTORY include/ DESTINATION include)
install(TARGETS geoviz_core ARCHIVE DESTINATION lib)

set(CPACK_PACKAGE_NAME "GeoViz")
set(CPACK_PACKAGE_VENDOR "Satyam Dhal - IIIT Bhubaneswar")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Computational geometry visualizer in modern C++")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP")
include(CPack)
