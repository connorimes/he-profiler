cmake_minimum_required(VERSION 2.8.9)

project(he-profiler)
set(PROJECT_VERSION 0.1.0)

find_package(PkgConfig REQUIRED)
pkg_check_modules(HBSPOW REQUIRED hbs-pow)
pkg_check_modules(ENERGYMON REQUIRED energymon-default)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas -Wextra -pedantic -pedantic-errors -std=gnu99")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

include_directories(${PROJECT_SOURCE_DIR}/inc ${HBSPOW_INCLUDE_DIRS} ${ENERGYMON_INCLUDE_DIRS})

# Libraries

set(SRC src/he-profiler.c)
set(SRC_DUMMY src/he-profiler-dummy.c)

# force fPIC on static libs
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

add_library(he-profiler SHARED ${SRC})
add_library(he-profiler-static STATIC ${SRC})

target_link_libraries(he-profiler ${HBSPOW_LIBRARIES} ${ENERGYMON_LIBRARIES} pthread rt)
target_link_libraries(he-profiler-static ${HBSPOW_LIBRARIES} ${ENERGYMON_LIBRARIES} pthread rd)

add_library(he-profiler-dummy SHARED ${SRC_DUMMY})
add_library(he-profiler-dummy-static STATIC ${SRC_DUMMY})

# Binaries

add_executable(he-profiler-overhead src/he-profiler-overhead.c)
target_link_libraries(he-profiler-overhead he-profiler)

# Tests

add_executable(he-profiler-test test/he-profiler-test.c)
target_link_libraries(he-profiler-test he-profiler)

add_executable(he-profiler-macro-disable-test test/he-profiler-macro-test.c)
target_link_libraries(he-profiler-macro-disable-test he-profiler)

add_executable(he-profiler-macro-enable-test test/he-profiler-macro-test.c)
target_compile_definitions(he-profiler-macro-enable-test PRIVATE HE_PROFILER_ENABLE)
target_link_libraries(he-profiler-macro-enable-test he-profiler)

# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include/${PROJECT_NAME}")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "heartbeats-energymon-profiler")
set(PKG_CONFIG_DESCRIPTION "Profiler using simple heartbeats and energymon")
set(PKG_CONFIG_REQUIRES_PRIVATE "hbs-pow energymon-default")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler")
set(PKG_CONFIG_LIBS_PRIVATE "-lpthread -lrt")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler.pc"
)
set(PKG_CONFIG_NAME "heartbeats-energymon-profiler-static")
set(PKG_CONFIG_REQUIRES_PRIVATE "hbs-pow-static energymon-default-static")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler-static")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler-static.pc"
)

set(PKG_CONFIG_NAME "heartbeats-energymon-profiler-dummy")
set(PKG_CONFIG_DESCRIPTION "Dummy profiler")
set(PKG_CONFIG_REQUIRES_PRIVATE "")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler-dummy")
set(PKG_CONFIG_LIBS_PRIVATE "")
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
