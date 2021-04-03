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
target_link_libraries(imgui PUBLIC glfw GL glad)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

# imnodes
add_library(imnodes STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imnodes/imnodes.cpp"
)
target_include_directories(imnodes PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/imnodes"
)
target_link_libraries(imnodes PUBLIC imgui)

# imtext
add_library(imtext STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/ImGuiColorTextEdit/TextEditor.cpp"
)
target_include_directories(imtext PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/ImGuiColorTextEdit"
)
target_link_libraries(imtext PUBLIC imgui)

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

target_link_libraries(ext INTERFACE glfw imgui GL glad imnodes imtext boost_json implot fmt plog)