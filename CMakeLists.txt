cmake_minimum_required(VERSION 3.18)

project(facmaker CXX)

add_executable(facmaker "src/main.cpp" "src/editor/factory_editor.cpp")
target_include_directories(facmaker PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/facmaker")

add_subdirectory(ext)

target_link_libraries(facmaker PRIVATE ext)