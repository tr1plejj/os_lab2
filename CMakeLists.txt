cmake_minimum_required(VERSION 3.24)
project(os_lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
endif()

find_package(Threads REQUIRED)

add_library(matrix src/matrix.cc src/experiment.cc)
target_include_directories(matrix PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(matrix PUBLIC Threads::Threads)

add_executable(matrix_demo src/main.cc)
target_link_libraries(matrix_demo PRIVATE matrix)

add_executable(loop_order_benchmark EXCLUDE_FROM_ALL tools/loop_order_benchmark.cc)
target_link_libraries(loop_order_benchmark PRIVATE matrix)

include(CTest)
if(BUILD_TESTING)
  include(FetchContent)
  set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.17.0
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    DOWNLOAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.cache/dependencies"
    DOWNLOAD_NAME googletest-v1.17.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(matrix_tests tests/matrix_test.cc tests/experiment_test.cc)
  target_link_libraries(matrix_tests PRIVATE matrix GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(matrix_tests)
endif()

foreach(target matrix matrix_demo matrix_tests loop_order_benchmark)
  if(TARGET ${target} AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endforeach()

