#include "Camera.hpp"
#include "ImGUIApplication.hpp"
#include "Model.hpp"
#include "TrackballController.hpp"
#include "render_pass/BlurPass.hpp"
#include "render_pass/GBufferPass.hpp"
#include "render_pass/LightingPass.hpp"
#include "render_pass/PostProcessPass.hpp"
#include "render_pass/SSAOPass.hpp"
#include "render_pass/ShadowPass.hpp"
#include "render_pass/SkyboxPass.hpp"
#include <imgui.h>
#include <portable-file-dialogs.h>
#define SOKOL_GLCORE33
#define SOKOL_IMPL
#include <glad/glad.h>
#include <sokol_gfx.h>

using namespace std;

class ModelViewer : public ImGUIApplication {
public:
  explicit ModelViewer(const Desc &desc) : ImGUIApplication(desc) {}

  void init() {
    auto &desc = get_desc();
    auto width = desc.width;
    auto height = desc.height;

    light.direction = {0.0f, -1.0f, 0.0f};

    camera.setProjection(45.0f,
                         static_cast<float>(width) / static_cast<float>(height),
                         0.1f, 100.0f);
    controller.position[2] += 5.0f;

    gbuffer_pass = make_unique<GBufferPass>(width, height);
    ssao_pass = make_unique<SSAOPass>(width, height, gbuffer_pass->position,
                                      gbuffer_pass->normal);
    lighting_pass = make_unique<LightingPass>(
        width, height, gbuffer_pass->position, gbuffer_pass->normal,
        gbuffer_pass->albedo, gbuffer_pass->emissive);
    skybox_pass =
        make_unique<SkyboxPass>(lighting_pass->result, gbuffer_pass->depth);
    postprocess_pass = make_unique<PostProccesPass>(
        lighting_pass->result, lighting_pass->bright_color);
    blur_pass = make_unique<BlurPass>(lighting_pass->bright_color);

    lighting_pass->set_irradiance_map(skybox_pass->irradiance_map);
    lighting_pass->set_prefilter_map(skybox_pass->prefilter_map);

    on(InputEvent::MouseDown, [&controller = this->controller]() {
      auto &mouse_down = Application::Input::mouse_down;
      auto &cursor_position = Application::Input::cursor_position;
      controller.onMouseDown(mouse_down[0],
                             static_cast<int>(cursor_position[0]),
                             static_cast<int>(cursor_position[1]));
    });
    on(InputEvent::MouseUp, [&controller = this->controller]() {
      auto &mouse_down = Application::Input::mouse_down;
      auto &cursor_position = Application::Input::cursor_position;
      controller.onMouseUp(mouse_down[0], static_cast<int>(cursor_position[0]),
                           static_cast<int>(cursor_position[1]));
    });
    on(InputEvent::MouseMove, [&controller = this->controller]() {
      auto &cursor_position = Application::Input::cursor_position;
      controller.onMouseMove(static_cast<int>(cursor_position[0]),
                             static_cast<int>(cursor_position[1]));
    });
    on(InputEvent::MouseWheel, [&controller = this->controller]() {
      auto &scroll_offset = Application::Input::scroll_offset;
      controller.onMouseWheelScroll(scroll_offset[1]);
    });
  }

  void update() override {
    auto &desc = get_desc();
    auto width = desc.width;
    auto height = desc.height;

    if (!ImGui::GetIO().WantCaptureMouse) {
      controller.update();
      camera.lookAt(controller.position, controller.target, controller.up);
    }

    ImGui::NewFrame();

    ImGui::Begin("Parameters");
    ImGui::Text("Directional Light Direction");
    ImGui::SliderFloat("x", &(light.direction.data()[0]), -3.0f, 3.0f);
    ImGui::SliderFloat("y", &(light.direction.data()[1]), -3.0f, 3.0f);
    ImGui::SliderFloat("z", &(light.direction.data()[2]), -3.0f, 3.0f);
    ImGui::Checkbox("Use SSAO", &use_ssao);
    ImGui::Checkbox("Use Bloom", &use_bloom);
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
      gbuffer_pass->run(model, camera_matrix);
    }
    if (use_ssao) {
      ssao_pass->run(view_matrix, projection_matrix);
      lighting_pass->enable_ssao(ssao_pass->ao_map);
    } else {
      lighting_pass->disable_ssao();
    }
    lighting_pass->run(controller.position, light);
    skybox_pass->run(camera_matrix);
    if (use_bloom) {
      blur_pass->run(2);
    }
    postprocess_pass->run(width, height);
    sg_commit();
  }

private:
  unique_ptr<GBufferPass> gbuffer_pass;
  unique_ptr<LightingPass> lighting_pass;
  unique_ptr<SSAOPass> ssao_pass;
  unique_ptr<SkyboxPass> skybox_pass;
  unique_ptr<BlurPass> blur_pass;
  unique_ptr<PostProccesPass> postprocess_pass;

  bool show_demo_window = false;
  bool use_ssao = true;
  bool use_bloom = false;

  vector<Model> models;
  Camera camera{};
  TrackballController controller{800, 600};
  Light light{};
};

int main() {
  ModelViewer::Desc desc{};
  desc.width = 1280;
  desc.height = 720;
  desc.title = "GLTF Model Viewer";

  ModelViewer viewer{desc};
  viewer.init();
  viewer.run();

  return 0;
}
