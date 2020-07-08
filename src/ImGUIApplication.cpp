#define GLFW_INCLUDE_NONE
#include "ImGUIApplication.hpp"
#include <GLFW/glfw3.h>

ImGUIApplication::ImGUIApplication(const Application::Desc &desc)
    : Application(desc) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGuiStyle style{};
  style.WindowRounding = 0.0f;
  ImGui::StyleColorsDark(&style);

  auto win = reinterpret_cast<GLFWwindow *>(window);
  ImGui_ImplGlfw_InitForOpenGL(win, true);
  ImGui_ImplOpenGL3_Init("#version 150");
}

void ImGUIApplication::run() {
  auto win = reinterpret_cast<GLFWwindow *>(window);
  while (!glfwWindowShouldClose(win)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    update();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(win);
    glfwPollEvents();
  }
}