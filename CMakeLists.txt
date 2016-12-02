cmake_minimum_required(VERSION 2.8.11)

project(jinshan)

set(CMAKE_CXX_COMPILER "clang++")

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
add_definitions(-std=c++11 -stdlib=libc++)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt4 REQUIRED QtGui QtWebKit)

find_package(PkgConfig)
pkg_check_modules(DEPS REQUIRED libavcodec libavformat libavutil libavfilter)
find_package(PNG)
find_package(JPEG)

SET (LIB_SRC_LIST
    moviedecoder.h
    moviedecoder.cpp
    imagewriter.h
    imagewriterfactory.h
    stringoperations.h
    stringoperations.cpp
    videothumbnailer.cpp
    videothumbnailerc.cpp
    ifilter.h
    videoframe.h
    histogram.h
    grayscalefilter.h
    filmstrip.h
    filmstripfilter.h
    filmstripfilter.cpp
    pngwriter.h 
    pngwriter.cpp
    jpegwriter.h 
    jpegwriter.cpp
)


add_definitions(${DEPS_CFLAGS})
include_directories(${DEPS_INCLUDE_DIR})
include_directories(${PNG_INCLUDE_DIR})
include_directories(${JPEG_INCLUDE_DIR})

add_executable(jinshan main.cpp swfplayer.cpp ${LIB_SRC_LIST})

target_link_libraries(jinshan  Qt4::QtGui Qt4::QtWebKit
    X11 z ${DEPS_LIBRARIES} ${PNG_LIBRARIES} ${JPEG_LIBRARIES})
