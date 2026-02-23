cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(MyProject LANGUAGES C CXX)

# Build an executable called "MyFirmware"
add_executable(MyFirmware main.c) #lv_font_montserrat_12_ukrainian.c)

# Specify path to own LVGL config header
set(LV_CONF_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h
    CACHE STRING "" FORCE)

# Fetch LVGL from GitHub
FetchContent_Declare(lvgl URL https://github.com/lvgl/lvgl/archive/refs/tags/v9.3.0.zip)
FetchContent_MakeAvailable(lvgl)

# Find the SDL2 package. The COMPONENTS are crucial.
find_package(SDL2 REQUIRED)

# The target "MyFirmware" depends on LVGL
target_link_libraries(MyFirmware PRIVATE lvgl::lvgl SDL2::SDL2)
