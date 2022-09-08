#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

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

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        editor.draw();

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
#ifndef NDEBUG
    {
        int monitor_count;
        auto monitors = glfwGetMonitors(&monitor_count);
        if (monitor_count > 1) {
            int xpos, ypos, width, height;
            glfwGetMonitorWorkarea(monitors[1], &xpos, &ypos, &width, &height);
            window_x = xpos + width / 2 - window_w / 2;
            window_y = ypos + height / 2 - window_h / 2;
        }
    }
#endif
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