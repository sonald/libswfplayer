cmake_minimum_required(VERSION 2.8.10)

project(qswfplayer)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)
add_definitions(-std=c++11)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(PkgConfig)
pkg_check_modules(Qt4 REQUIRED QtGui QtWebKit)

add_definitions(${Qt4_CFLAGS})
include_directories(${Qt4_INCLUDE_DIRS})

#set(MOC1 moc_swfplayer.cpp)
#add_custom_target(${MOC1})
#add_custom_command(TARGET ${MOC1} PRE_BUILD
    #COMMAND moc-qt4 -o ${MOC1} ${CMAKE_SOURCE_DIR}/swfplayer.h
    #WORKING_DIRECTORY ${CMAKE_BUILD_DIR})
#set_source_files_properties(
    #${MOC1} PROPERTIES EXTERNAL_OBJECT TRUE  GENERATED TRUE)

qt_wrap_cpp(swfplayer MOC1 swfplayer.h)
add_library(swfplayer SHARED swfplayer.cpp ${MOC1})
#add_dependencies(swfplayer ${MOC1})
set_target_properties(swfplayer PROPERTIES VERSION 1.0.1 SOVERSION 1.0)
target_link_libraries(swfplayer ${Qt4_LIBRARIES} z)

qt_wrap_cpp(qswfplayer MOC2 main.cpp)
add_executable(qswfplayer main.cpp)
link_directories(qswfplayer ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(qswfplayer ${Qt4_LIBRARIES})

add_dependencies(qswfplayer swfplayer)

install(TARGETS swfplayer DESTINATION lib)
install(FILES swfplayer.h DESTINATION include/libswfplayer)
