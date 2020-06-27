#define GLFW_INCLUDE_NONE
#include "Camera.hpp"
#include "Model.hpp"
#include "TrackballController.hpp"
#include "render_pass/BlurPass.hpp"
#include "render_pass/GBufferPass.hpp"
#include "render_pass/LightingPass.hpp"
#include "render_pass/PostProcessPass.hpp"
#include "render_pass/SSAOPass.hpp"
#include "render_pass/ShadowPass.hpp"
#include "render_pass/SkyboxPass.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <portable-file-dialogs.h>
#define SOKOL_GLCORE33
#define SOKOL_IMPL
#include <sokol_gfx.h>

using namespace std;

void setup_event_callback(GLFWwindow *window, TrackballController &controller);

int main() {
  constexpr int32_t width = 1280;
  constexpr int32_t height = 720;

  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  auto window =
      glfwCreateWindow(width, height, "GLTF Model Viewer", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  auto loader = reinterpret_cast<GLADloadproc>(&glfwGetProcAddress);
  if (!gladLoadGLLoader(loader)) {
    cerr << "Failed to load OpenGL." << endl;
    return -1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGuiStyle style{};
  style.WindowRounding = 0.0f;
  ImGui::StyleColorsDark(&style);
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 150");

  sg_desc app_desc{};
  sg_setup(&app_desc);

  // load scene
  vector<Model> models;

  // setup camera
  Camera camera{};
  camera.setProjection(45.0f,
                       static_cast<float>(width) / static_cast<float>(height),
                       0.1f, 100.0f);

  TrackballController controller{800, 600};
  controller.position[2] += 5.0f;
  setup_event_callback(window, controller);

  auto gbuffer_pass = GBufferPass(width, height);
  auto ssao_pass =
      SSAOPass(width, height, gbuffer_pass.position, gbuffer_pass.normal);
  auto lighting_pass =
      LightingPass(width, height, gbuffer_pass.position, gbuffer_pass.normal,
                   gbuffer_pass.albedo, gbuffer_pass.emissive);
  auto skybox_pass = SkyboxPass(lighting_pass.result, gbuffer_pass.depth);
  auto postprocess_pass = PostProccesPass(lighting_pass.result);
  auto ssao_blur_pass = BlurPass(ssao_pass.ao_map);

  lighting_pass.set_irradiance_map(skybox_pass.irradiance_map);
  lighting_pass.set_prefilter_map(skybox_pass.prefilter_map);

  Light light{};
  light.direction = {0.0f, -1.0f, 0.0f};
  bool show_demo_window = false;
  bool use_ssao = true;
  while (!glfwWindowShouldClose(window)) {
    if (!ImGui::GetIO().WantCaptureMouse) {
      controller.update();
      camera.lookAt(controller.position, controller.target, controller.up);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Parameters");
    ImGui::Text("Directional Light Direction");
    ImGui::SliderFloat("x", &(light.direction.data()[0]), -3.0f, 3.0f);
    ImGui::SliderFloat("y", &(light.direction.data()[1]), -3.0f, 3.0f);
    ImGui::SliderFloat("z", &(light.direction.data()[2]), -3.0f, 3.0f);
    ImGui::Checkbox("Use SSAO", &use_ssao);
    if (ImGui::Button("Select model...")) {
      auto result = pfd::open_file(
          "Choose models to read", "",
          {"GLTF Models (.gltf .glb)", "*.gltf *.glb", "All Files", "*"},
          pfd::opt::none);
      for (const auto &filepath : result.result()) {
        models.emplace_back(Model::Load(filepath.c_str()));
      }
    }

    ImGui::Checkbox("Demo Window", &show_demo_window);
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    if (show_demo_window) {
      ImGui::ShowDemoWindow(&show_demo_window);
    }
    ImGui::Render();

    const Eigen::Matrix4f view_matrix = camera.getViewMatrix().inverse();
    auto &projection_matrix = camera.getCullingProjectionMatrix();
    const Eigen::Matrix4f camera_matrix = projection_matrix * view_matrix;

    for (auto &model : models) {
      light.shadow_pass->run(model, light);
      gbuffer_pass.run(model, camera_matrix);
    }
    if (use_ssao) {
      ssao_pass.run(view_matrix, projection_matrix);
      lighting_pass.enable_ssao(ssao_pass.ao_map);
      ssao_blur_pass.run();
    } else {
      lighting_pass.disable_ssao();
    }
    lighting_pass.run(controller.position, light);
    skybox_pass.run(camera_matrix);
    postprocess_pass.run(width, height);
    sg_commit();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  sg_shutdown();
  glfwTerminate();
  return 0;
}

void setup_event_callback(GLFWwindow *window, TrackballController &controller) {
  glfwSetWindowUserPointer(window, reinterpret_cast<void *>(&controller));
  glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scan_code,
                                int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      return;
    }
  });
  glfwSetCursorPosCallback(window, [](GLFWwindow *window, double x, double y) {
    auto raw = glfwGetWindowUserPointer(window);
    auto &control = *(reinterpret_cast<TrackballController *>(raw));
    control.onMouseMove(static_cast<int>(x), static_cast<int>(y));
  });
  glfwSetScrollCallback(
      window, [](GLFWwindow *window, double x_offset, double y_offset) {
        if (y_offset == 0.0) {
          return;
        }
        auto raw = glfwGetWindowUserPointer(window);
        auto &control = *(reinterpret_cast<TrackballController *>(raw));
        control.onMouseWheelScroll(static_cast<float>(y_offset));
      });
  glfwSetMouseButtonCallback(
      window, [](GLFWwindow *window, int button, int action, int mods) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        auto raw = glfwGetWindowUserPointer(window);
        auto &control = *(reinterpret_cast<TrackballController *>(raw));
        if (action == GLFW_PRESS) {
          control.onMouseDown(GLFW_MOUSE_BUTTON_LEFT == button,
                              static_cast<int>(x), static_cast<int>(y));
        } else if (action == GLFW_RELEASE) {
          control.onMouseUp(GLFW_MOUSE_BUTTON_LEFT == button,
                            static_cast<int>(x), static_cast<int>(y));
        }
      });
}