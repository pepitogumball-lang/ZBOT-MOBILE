cmake_minimum_required(VERSION 3.25)

project(ZBOT_MOBILE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CPM for dependencies
include(FetchContent)
if(NOT CPM_BINARY_DIR)
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
        message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
        file(DOWNLOAD
            https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
            "${CPM_DOWNLOAD_LOCATION}"
        )
    endif()
    include("${CPM_DOWNLOAD_LOCATION}")
endif()

CPMAddPackage("gh:maxnut/GDReplayFormat#9c32b5529067901418ea6e4dfbd36437dc8712bc")
CPMAddPackage("gh:matcool/gd-imgui-cocos#8973953519fa05d81c61634e585559ab87ca559d")

# Add the sources
set(SOURCES
    src/main.cpp
    src/zBot.cpp
    src/gui.cpp
)

# Set up the mod binary
add_library(ZBOT_MOBILE SHARED ${SOURCES})

# Geode SDK setup
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Link dependencies and setup Geode
# Using plain signature to avoid mixing with Geode's internal calls
target_link_libraries(ZBOT_MOBILE libGDR imgui-cocos)
setup_geode_mod(ZBOT_MOBILE)
