cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(ZBOT-MOBILE VERSION 1.0.0 LANGUAGES CXX)

file(GLOB SOURCES
    src/*.cpp
    src/zClickbot/*.cpp
    src/zKeyChecker/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()

add_subdirectory($ENV{GEODE_SDK} $ENV{GEODE_SDK}/build)

# CPM for dependencies
if(NOT EXISTS "${CMAKE_BINARY_DIR}/CPM.cmake")
    message(STATUS "Downloading CPM.cmake to ${CMAKE_BINARY_DIR}/CPM.cmake")
    file(DOWNLOAD "https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/CPM.cmake" "${CMAKE_BINARY_DIR}/CPM.cmake")
endif()
include("${CMAKE_BINARY_DIR}/CPM.cmake")

CPMAddPackage("gh:maxnut/GDReplayFormat#9c32b5529067901418ea6e4dfbd36437dc8712bc")
CPMAddPackage("gh:matcool/gd-imgui-cocos#8973953519fa05d81c61634e585559ab87ca559d")

target_compile_definitions(${PROJECT_NAME} PRIVATE MOD_VERSION="${CMAKE_PROJECT_VERSION}")

if (WIN32)
    target_link_libraries(${PROJECT_NAME} imgui-cocos libGDR)
elseif (ANDROID)
    target_link_libraries(${PROJECT_NAME} imgui-cocos libGDR)
endif()

setup_geode_mod(${PROJECT_NAME})
