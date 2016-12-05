cmake_minimum_required(VERSION 2.8.11)

project(qswfplayer)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
add_definitions(-std=c++11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt4 REQUIRED QtGui QtWebKit)

find_package(PkgConfig)

add_library(swfplayer SHARED swfplayer.cpp)
set_target_properties(swfplayer PROPERTIES VERSION 1.0.1 SOVERSION 1.0)
target_link_libraries(swfplayer Qt4::QtGui Qt4::QtWebKit z)

add_executable(qswfplayer main.cpp)
link_directories(qswfplayer ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(qswfplayer Qt4::QtGui Qt4::QtWebKit swfplayer)

add_dependencies(qswfplayer swfplayer)

install(TARGETS swfplayer DESTINATION lib)
install(FILES swfplayer.h DESTINATION include/libswfplayer)
