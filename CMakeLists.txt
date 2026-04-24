cmake_minimum_required(VERSION 3.25)

project(ZBOT_MOBILE LANGUAGES CXX)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
endif()

# Link the Geode SDK
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Setup the mod (ID from mod.json)
setup_geode_mod(pepitogumball.zbot_mobile)

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

# Note: add_geode_mod automatically adds the SHARED library target
add_geode_mod(
    ZBOT_MOBILE
    RESOURCES resources
    SOURCES
        src/main.cpp
        src/zBot.cpp
        src/gui.cpp
)

target_link_libraries(ZBOT_MOBILE PUBLIC libGDR imgui-cocos)
