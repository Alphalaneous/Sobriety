cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(bridge VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

include(cmake/CPM.cmake)

add_executable(${PROJECT_NAME} ${SOURCES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

target_include_directories(${PROJECT_NAME} PRIVATE ${DBUS_INCLUDE_DIRS})
target_link_directories(${PROJECT_NAME} PRIVATE ${DBUS_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${DBUS_LIBRARIES})
target_link_options(${PROJECT_NAME} PRIVATE -static-libstdc++ -static-libgcc)

set(USE_ZLIB OFF)
CPMAddPackage("gh:machinezone/IXWebSocket#v11.4.6")
CPMAddPackage("gh:geode-sdk/json@3.3.0")
CPMAddPackage("gh:fmtlib/fmt#12.1.0")

target_link_libraries(${PROJECT_NAME} PRIVATE ixwebsocket mat-json fmt)