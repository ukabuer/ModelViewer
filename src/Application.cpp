#define GLFW_INCLUDE_NONE
#include "Application.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <sokol_gfx.h>
#include <stdexcept>

using namespace std;

Application::Application(const Application::Desc &desc) {
  this->desc = desc;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  auto win = glfwCreateWindow(desc.width, desc.height,
                              desc.title ? "ModelViewer" : desc.title, nullptr,
                              nullptr);
  if (!win) {
    throw runtime_error("Failed to create window.");
  }

  this->window = win;
  glfwMakeContextCurrent(win);
  glfwSwapInterval(1);

  glfwSetKeyCallback(
      win, [](GLFWwindow *win, int key, int scan_code, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
          glfwSetWindowShouldClose(win, GLFW_TRUE);
          return;
        }
      });

  glfwSetWindowUserPointer(win, reinterpret_cast<void *>(this));
  glfwSetMouseButtonCallback(win, [](GLFWwindow *win, int button, int action,
                                     int mods) {
    double x, y;
    glfwGetCursorPos(win, &x, &y);
    Input::cursor_position = {static_cast<float>(x), static_cast<float>(y)};
    Input::mouse_down[GLFW_MOUSE_BUTTON_RIGHT == button] = action == GLFW_PRESS;
    auto raw = glfwGetWindowUserPointer(win);
    auto &app = *(reinterpret_cast<Application *>(raw));
    auto event =
        action == GLFW_PRESS ? InputEvent::MouseDown : InputEvent::MouseUp;
    auto it = app.callbacks.find(event);
    if (it == app.callbacks.end()) {
      return;
    }
    auto &list = it->second;
    for (auto &fn : list) {
      fn();
    }
  });

  glfwSetCursorPosCallback(win, [](GLFWwindow *win, double x, double y) {
    auto raw = glfwGetWindowUserPointer(win);
    auto &app = *(reinterpret_cast<Application *>(raw));

    Input::cursor_position = {static_cast<float>(x), static_cast<float>(y)};

    auto it = app.callbacks.find(InputEvent::MouseMove);
    if (it == app.callbacks.end()) {
      return;
    }

    auto &list = it->second;
    for (auto &fn : list) {
      fn();
    }
  });

  glfwSetScrollCallback(win, [](GLFWwindow *win, double x, double y) {
    if (y == 0.0) {
      return;
    }
    Input::scroll_offset = {static_cast<float>(x), static_cast<float>(y)};
    auto raw = glfwGetWindowUserPointer(win);
    auto &app = *(reinterpret_cast<Application *>(raw));

    auto it = app.callbacks.find(InputEvent::MouseWheel);
    if (it == app.callbacks.end()) {
      return;
    }

    auto &list = it->second;
    for (auto &fn : list) {
      fn();
    }
  });

  auto loader = reinterpret_cast<GLADloadproc>(&glfwGetProcAddress);
  if (!gladLoadGLLoader(loader)) {
    throw runtime_error("Failed to load OpenGL.");
  }

  sg_desc app_desc{};
  sg_setup(&app_desc);
}

Application::~Application() {
  sg_shutdown();
  glfwTerminate();
}

void Application::on(InputEvent event, const std::function<void()> &callback) {
  callbacks[event].emplace_back(callback);
}

void Application::run() {
  auto win = reinterpret_cast<GLFWwindow *>(window);
  while (!glfwWindowShouldClose(win)) {
    update();

    glfwSwapBuffers(win);
    glfwPollEvents();
  }
}

std::array<bool, 2> Application::Input::mouse_down = {};
std::array<float, 2> Application::Input::cursor_position = {};
std::array<float, 2> Application::Input::scroll_offset = {};
