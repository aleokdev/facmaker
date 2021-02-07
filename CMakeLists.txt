cmake_minimum_required(VERSION 3.18)

project(facmaker CXX)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Using debug mode")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
else()
    message(STATUS "Using build type ${CMAKE_BUILD_TYPE}")
endif()

add_executable(facmaker
    "src/main.cpp"
    "src/editor/factory_editor.cpp"
    "src/factory.cpp"
    "src/util/quantity_plot.cpp"
    "src/util/more_imgui.cpp"
)
target_include_directories(facmaker PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/facmaker")

add_subdirectory(ext)

target_link_libraries(facmaker PRIVATE ext)