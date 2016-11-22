cmake_minimum_required(VERSION 2.8.11)

project(jinshan)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
add_definitions(-std=c++11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5WebKitWidgets)
find_package(Qt5X11Extras)
find_package(PkgConfig)

add_executable(jinshan main.cpp swfplayer.cpp)

target_link_libraries(jinshan Qt5::WebKitWidgets Qt5::X11Extras X11)
