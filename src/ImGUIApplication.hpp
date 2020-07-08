#pragma once
#include <Application.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class ImGUIApplication : public Application {
public:
  explicit ImGUIApplication(const Application::Desc &desc);
  ~ImGUIApplication() override = default;

  void run() final;
};
