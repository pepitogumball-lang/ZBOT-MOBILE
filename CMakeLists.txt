cmake_minimum_required(VERSION 3.25)

project(ZBOT_MOBILE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
if(NOT CPM_BINARY_DIR)
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
        file(DOWNLOAD
            https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
            "${CPM_DOWNLOAD_LOCATION}"
        )
    endif()
    include("${CPM_DOWNLOAD_LOCATION}")
endif()

CPMAddPackage("gh:maxnut/GDReplayFormat#9c32b5529067901418ea6e4dfbd36437dc8712bc")
CPMAddPackage("gh:matcool/gd-imgui-cocos#1ed2f931b49e6d00e3f03abe54fb995b6f02007b")

set(SOURCES
    src/main.cpp
    src/zBot.cpp
    src/gui.cpp
    src/recordmanager.cpp
    src/playbackmanager.cpp
    src/speedhack.cpp
    src/speedhackaudio.cpp
    src/updatecheck.cpp
)

add_library(ZBOT_MOBILE SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "No se encuentra el Geode SDK")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_link_libraries(ZBOT_MOBILE libGDR imgui-cocos)

if (libGDR_SOURCE_DIR)
    target_include_directories(ZBOT_MOBILE PRIVATE "${libGDR_SOURCE_DIR}/include")
endif()
if (imgui-cocos_SOURCE_DIR)
    target_include_directories(ZBOT_MOBILE PRIVATE "${imgui-cocos_SOURCE_DIR}/include")
endif()

setup_geode_mod(ZBOT_MOBILE)
