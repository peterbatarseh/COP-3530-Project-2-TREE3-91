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

# httplib
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG        v0.64.0
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(httplib)

include_directories(src)

add_executable(Main
        src/main.cpp
        src/FileNode.cpp
        src/DataGenerator.cpp
        src/BPlusTree.cpp
        src/SplayTree.cpp
        src/NaryTree.cpp
        src/TreeLoader.cpp
)
  if(WIN32)
      target_link_options(Main PRIVATE -static)
  endif()

# Web frontend thing
add_executable(ExportWeb
        src/export_main.cpp
        src/WebServer.cpp
        src/FileNode.cpp
        src/JsonExport.cpp
)
target_link_libraries(ExportWeb PRIVATE httplib::httplib)

if(WIN32)
    target_compile_definitions(ExportWeb PRIVATE _WIN32_WINNT=0x0A00)
    target_link_options(ExportWeb PRIVATE -static)
endif()

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
