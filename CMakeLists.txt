cmake_minimum_required(VERSION 3.22)
project(Project2)

set(CMAKE_CXX_STANDARD 17)

# Catch2
include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)

include_directories(src)

add_executable(Main
        src/main.cpp
        src/FileNode.cpp
        src/FileNode.h
        src/BPlusTree.cpp
        src/BPlusTree.h
)

add_executable(Tests
        tests/test_nodes.cpp
        tests/test_bplus_tree.cpp
        src/FileNode.cpp
        src/FileNode.h
        src/BPlusTree.cpp
        src/BPlusTree.h
)

target_link_libraries(Tests PRIVATE Catch2::Catch2WithMain) 

include(CTest)
include(Catch)
catch_discover_tests(Tests) 

add_executable(SplayTests
        tests/test_splay.cpp
        src/FileNode.cpp
        src/SplayTree.cpp
        src/NaryTree.cpp
)
target_link_libraries(SplayTests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(SplayTests)
