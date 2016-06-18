cmake_minimum_required(VERSION 2.8.9)

project(he-profiler)
set(PROJECT_VERSION 0.1.0)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas -Wextra -pedantic -pedantic-errors -std=gnu99")

# force fPIC unless overridden
if (NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif()


# Dependencies

find_package(PkgConfig REQUIRED)
pkg_check_modules(HBS REQUIRED heartbeats-simple)
pkg_check_modules(ENERGYMON REQUIRED energymon-default)

# Determine if we should link with librt
include(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(clock_gettime HAVE_CLOCK_GETTIME)
if(NOT HAVE_CLOCK_GETTIME)
  find_library(LIBRT NAMES rt)
endif()

# Must be set to OFF to prevent doing a try_run() during cross-compiling
set(THREADS_PTHREAD_ARG OFF)
find_package(Threads)

include_directories(${PROJECT_SOURCE_DIR}/inc ${HBS_INCLUDE_DIRS} ${ENERGYMON_INCLUDE_DIRS})


# Libraries

set(SRC src/he-profiler.c)
set(SRC_DUMMY src/he-profiler-dummy.c)

add_library(he-profiler ${SRC})
target_link_libraries(he-profiler -L${HBS_LIBDIR} ${HBS_LIBRARIES} -L${ENERGYMON_LIBDIR} ${ENERGYMON_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${LIBRT})

add_library(he-profiler-dummy ${SRC_DUMMY})


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

enable_testing()
macro(add_unit_test target)
  add_test(${target} ${EXECUTABLE_OUTPUT_PATH}/${target})
endmacro(add_unit_test)

add_unit_test(he-profiler-test)
add_unit_test(he-profiler-macro-disable-test)
add_unit_test(he-profiler-macro-enable-test)


# pkg-config

set(PKG_CONFIG_EXEC_PREFIX "\${prefix}")
set(PKG_CONFIG_LIBDIR "\${prefix}/lib")
set(PKG_CONFIG_INCLUDEDIR "\${prefix}/include")
set(PKG_CONFIG_CFLAGS "-I\${includedir}")

set(PKG_CONFIG_NAME "heartbeats-energymon-profiler")
set(PKG_CONFIG_DESCRIPTION "Profiler using simple heartbeats and energymon")
set(PKG_CONFIG_REQUIRES_PRIVATE "heartbeats-simple >= 0.3.0, energymon-default >= 0.2.0")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler")

if(CMAKE_THREAD_LIBS_INIT)
  set(PKG_CONFIG_LIBS_PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
endif()
if(LIBRT)
  set(PKG_CONFIG_LIBS_PRIVATE "${PKG_CONFIG_LIBS_PRIVATE} -lrt")
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler.pc
)

set(PKG_CONFIG_NAME "heartbeats-energymon-profiler-dummy")
set(PKG_CONFIG_DESCRIPTION "Dummy profiler")
set(PKG_CONFIG_REQUIRES_PRIVATE "")
set(PKG_CONFIG_LIBS "-L\${libdir} -lhe-profiler-dummy")
set(PKG_CONFIG_LIBS_PRIVATE "")

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/pkgconfig.in
  ${CMAKE_CURRENT_BINARY_DIR}/pkgconfig/he-profiler-dummy.pc
)


# Install

install(TARGETS he-profiler he-profiler-dummy DESTINATION lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/inc/ DESTINATION include)
install(DIRECTORY ${CMAKE_BINARY_DIR}/pkgconfig/ DESTINATION lib/pkgconfig)


# Uninstall

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  @ONLY
)

add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
