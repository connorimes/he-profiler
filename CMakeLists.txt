cmake_minimum_required(VERSION 2.8.9)

project(he-profiler)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/inc)

# Libraries

set(SRC src/he-profiler.c)
set(SRC_DUMMY src/he-profiler-dummy.c)

# force fPIC on static libs
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_library(he-profiler SHARED ${SRC})
add_library(he-profiler-static STATIC ${SRC})

add_library(he-profiler-dummy SHARED ${SRC_DUMMY})
add_library(he-profiler-dummy-static STATIC ${SRC_DUMMY})

# Binaries

add_executable(he-profiler-overhead src/he-profiler-overhead.c)
target_link_libraries(he-profiler-overhead he-profiler hbs-pow energymon-default pthread rt)

# Tests

add_executable(he-profiler-test test/he-profiler-test.c)
target_link_libraries(he-profiler-test he-profiler hbs-pow energymon-default pthread rt)

# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/${PROJECT_NAME}")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "heartbeats-energymon-profiler")
set(PKG_CONFIG_DESCRIPTION "Profiler using simple heartbeats and energymon")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler -lhbs-pow -lenergymon-default -lpthread -lrt")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler.pc"
)
set(PKG_CONFIG_NAME "heartbeats-energymon-profiler-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler-static -lhbs-pow-static -lenergymon-default-static -lpthread -lrt")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler-static.pc"
)

set(PKG_CONFIG_NAME "heartbeats-energymon-profiler-dummy")
set(PKG_CONFIG_DESCRIPTION "Dummy profiler")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler-dummy")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler-dummy.pc"
)
set(PKG_CONFIG_NAME "heartbeats-energymon-profiler-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler-dummy-static")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler-dummy-static.pc"
)

# Install

install(TARGETS he-profiler he-profiler-static he-profiler-dummy he-profiler-dummy-static DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inc/ DESTINATION include/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)

# Uninstall

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
