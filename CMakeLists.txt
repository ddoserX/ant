cmake_minimum_required(VERSION 3.31)
project(AssistantNetworkTesting LANGUAGES CXX)

set(APP ant)
set(SRC ./src/main.cpp)

if (UNIX)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux)
endif()

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

find_package(boost_asio REQUIRED CONFIG)
find_package(boost_program_options REQUIRED CONFIG)

add_executable(${APP} ${SRC})

target_include_directories(${APP} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${APP} PRIVATE Boost::asio Boost::program_options)

target_compile_features(${APP} PUBLIC cxx_std_23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    MESSAGE(">>>>> MSVC COMPILER SETUP")
    target_compile_options(${APP} PUBLIC
        /WX
        /W4
        /Wall
        /Qspectre-
        /wd4820
        /wd5045
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${APP} PUBLIC /O2 /wd4711 /wd4710)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    MESSAGE(">>>>> GNU OR Clang COMPILER SETUP")
    target_compile_options(${APP} PUBLIC
        -Wall
        -Werror
        -Wextra
        -Wold-style-cast
        -Weffc++
    )

    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(${APP} PUBLIC -O3)
    endif()
endif()