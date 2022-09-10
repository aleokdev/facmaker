cmake_minimum_required(VERSION 3.18)

project(facmaker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Using debug mode")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
else ()
    message(STATUS "Using build type ${CMAKE_BUILD_TYPE}")
endif (CMAKE_BUILD_TYPE MATCHES Debug)


add_executable(
        facmaker
        "src/main.cpp"
        "src/editor/factory_editor.cpp"
        "src/factory.cpp"
        "src/util/quantity_plot.cpp"
        "src/util/more_imgui.cpp"
        "src/uid.cpp"
        "src/editor/parsing.cpp")
target_include_directories(facmaker PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/facmaker" "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(facmaker PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_subdirectory(ext)

add_library(pfd STATIC "src/pfd.cpp")
target_include_directories(pfd PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(pfd PRIVATE portable_file_dialogs)

target_link_libraries(facmaker PRIVATE ext pfd)

# Copy assets dir on build
file(
        GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/CopyConfigurationData$<CONFIG>.cmake"
        CONTENT "file(
                    COPY \"${PROJECT_SOURCE_DIR}/assets/\"
                    DESTINATION \"$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets\"
                    PATTERN \"CMakeLists.txt\" EXCLUDE
                 )"
)

add_custom_target(
        copy_assets
        COMMAND "${CMAKE_COMMAND}" -P "CopyConfigurationData$<CONFIG>.cmake"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_dependencies(facmaker copy_assets)
