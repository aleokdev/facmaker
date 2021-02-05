#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <imnodes.h>
#include <implot.h>

#include "editor/factory_editor.hpp"

constexpr const char* glsl_version = "#version 450 core";

int main() {
    if (!glfwInit()) {
        return -1;
    }

    int window_x = 0, window_y = 0;
#ifndef NDEBUG
    {
        int monitor_count;
        auto monitors = glfwGetMonitors(&monitor_count);
        if (monitor_count > 0) {
            glfwGetMonitorPos(monitors[1], &window_x, &window_x);
        }
    }
#endif
    GLFWwindow* window = glfwCreateWindow(1020, 720, "facmaker", NULL, NULL);
    if (!window) {
        return -1;
    }
    glfwSetWindowPos(window, window_x, window_y);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress);

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    imnodes::Initialize();

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup ImGui style
    ImGui::StyleColorsDark();

    fmk::FactoryEditor editor;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        ImGui::ShowDemoWindow();
        ImPlot::ShowDemoWindow();
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
    imnodes::Shutdown();
    return 0;
}