add_subdirectory(glfw)
add_subdirectory(glad)
set(BOOST_JSON_BUILD_EXAMPLES OFF)
set(BOOST_JSON_BUILD_FUZZERS OFF)
set(BOOST_JSON_BUILD_TESTS OFF)
set(BOOST_JSON_STANDALONE ON)
add_subdirectory(json)
add_subdirectory(fmt)
set(PLOG_BUILD_SAMPLES OFF)
add_subdirectory(plog)
add_subdirectory(portable-file-dialogs)

add_library(ext INTERFACE)

# imgui
file(GLOB IMGUI_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
        )
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        )
if (UNIX)
    target_link_libraries(imgui PUBLIC GL)
endif (UNIX)
target_link_libraries(imgui PUBLIC glfw glad)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

# imnodes
add_library(imnodes STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/imnodes/imnodes.cpp"
        )
target_include_directories(imnodes PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/imnodes"
        )
target_link_libraries(imnodes PUBLIC imgui)

# imgui-node-editor
add_library(imgui-node-editor STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor/imgui_canvas.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor/imgui_canvas.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor/imgui_node_editor.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor/imgui_node_editor_api.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor/crude_json.cpp"
        )
target_include_directories(imgui-node-editor PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor")
target_link_libraries(imgui-node-editor PRIVATE imgui)

# implot
add_library(implot STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/implot/implot.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/implot/implot_items.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/implot/implot_demo.cpp"
        )
target_include_directories(implot PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/implot"
        )
target_link_libraries(implot PUBLIC imgui)

if (UNIX)
    target_link_libraries(ext INTERFACE GL)
endif (UNIX)

target_link_libraries(ext INTERFACE glfw imgui glad imnodes boost_json implot fmt plog portable_file_dialogs imgui-node-editor)