add_subdirectory(glfw)
add_subdirectory(glad)

add_library(ext INTERFACE)

file(GLOB IMGUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
)
target_link_libraries(imgui PUBLIC glfw GL glad)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

add_library(imnodes STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imnodes/imnodes.cpp"
)
target_include_directories(imnodes PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imnodes"
)
target_link_libraries(imnodes PUBLIC imgui)

target_link_libraries(ext INTERFACE glfw imgui GL glad imnodes)