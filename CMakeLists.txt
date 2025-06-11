cmake_minimum_required(VERSION 3.31)
project(AssistantNetworkTesting LANGUAGES CXX)

set(APP ant)
set(SRC ./src/main.cpp)

if (UNIX)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux)
endif()

if (WIN32)
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows)
endif()

find_package(boost_asio REQUIRED CONFIG)

add_executable(${APP} ${SRC})

target_link_libraries(${APP} PRIVATE Boost::asio)

target_compile_features(${APP} PUBLIC cxx_std_23)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    MESSAGE(">>>>> MSVC COMPILER SETUP")
    target_compile_options(${APP} PUBLIC
        /w
        /WX
        /W4
        /Wall
    )
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
endif()