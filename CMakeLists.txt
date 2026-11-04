cmake_minimum_required(VERSION 3.22)
project(Project2)

set(CMAKE_CXX_STANDARD 17)

# Catch2
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # Or a later version
)

FetchContent_MakeAvailable(Catch2)

include_directories(src)

add_executable(Main
        src/main.cpp
)

# These tests can use the Catch2-provided main
add_executable(Tests
        tests/test_nodes.cpp
)

target_link_libraries(Tests PRIVATE Catch2::Catch2WithMain) #link catch to test.cpp file
# the name here must match that of your testing executable (the one that has test.cpp)
