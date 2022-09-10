#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>
#include <portable-file-dialogs.h>

#include "editor/factory_editor.hpp"

bool init_graphics(GLFWwindow** out_window);

static imnodes::Context* imnodes_context;

int main() {
    static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;
    plog::init(plog::verbose, &console_appender);

    GLFWwindow* window;
    if (!init_graphics(&window)) {
        PLOG_FATAL << "Couldn't initialize graphics.";
        return -1;
    }

    fmk::FactoryEditor editor;
    std::string path_being_edited;

    auto set_path_being_edited = [&path_being_edited, window](std::string&& str) {
        path_being_edited = str;
        if (str.empty()) {
            glfwSetWindowTitle(window, "facmaker");
        } else {
            glfwSetWindowTitle(window,
                               fmt::format("facmaker - {}",
                                           std::filesystem::path(path_being_edited).filename().c_str())
                                   .c_str());
        }
    };

    auto draw_main_menu_bar = [&editor, &path_being_edited, &set_path_being_edited]() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    editor = std::move(fmk::FactoryEditor());
                }
                if (ImGui::MenuItem("Open...")) {
                    auto selection = pfd::open_file("Open Factory").result();
                    if (!selection.empty()) {
                        std::ifstream file(selection[0]);
                        std::istringstream contents;
                        contents << file;
                        editor = std::move(contents);
                        PLOGD << "Imported data from '" << selection[0] << "'";

                        set_path_being_edited(std::move(selection[0]));
                    }
                }
                if (ImGui::MenuItem("Save", nullptr, false, !path_being_edited.empty())) {
                    std::ofstream file(path_being_edited);
                    editor.output_factory_json(file);
                    PLOGD << "Exported data to '" << path_being_edited << "'";
                }
                if (ImGui::MenuItem("Save As...")) {
                    auto destination = pfd::save_file("Save Factory").result();
                    if (!destination.empty()) {
                        std::ofstream file(destination);
                        editor.output_factory_json(file);
                        PLOGD << "Exported data to '" << destination << "'";

                        set_path_being_edited(std::move(destination));
                    }
                }
                if (ImGui::MenuItem("Import From Clipboard")) {
                    auto input = std::istringstream(ImGui::GetClipboardText());
                    editor = std::move(fmk::FactoryEditor(parse_factory_json(input)));
                    PLOGD << "Imported clipboard data";
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("Show ImGui Demo Window", nullptr, &show_imgui_demo_window);
                ImGui::MenuItem("Show ImPlot Demo Window", nullptr, &show_implot_demo_window);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        draw_main_menu_bar(window);
        editor.draw(window);

        ImGui::Render();
        {
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImPlot::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    imnodes::DestroyContext(imnodes_context);
    return 0;
}

bool init_graphics(GLFWwindow** out_window) {
    constexpr const char* glsl_version = "#version 450 core";

    if (!glfwInit()) {
        return false;
    }

    int window_x = 0, window_y = 0;
    const int window_w = 1020, window_h = 720;
    *out_window = glfwCreateWindow(window_w, window_h, "facmaker", nullptr, nullptr);
    if (!*out_window) {
        return false;
    }
    glfwSetWindowPos(*out_window, window_x, window_y);

    glfwMakeContextCurrent(*out_window);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = "assets/imgui.ini";
    ImPlot::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(*out_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    imnodes_context = imnodes::CreateContext();

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    // Setup ImGui font
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("assets/NotoSans-Medium.ttf", 72)->Scale = .3;

    return true;
}