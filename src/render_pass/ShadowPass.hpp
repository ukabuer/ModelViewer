#pragma once
#include "Camera.hpp"
#include "Light.hpp"
#include "Model.hpp"
#include <sokol_gfx.h>

struct ShadowPass {
  sg_pass pass{};
  sg_pass_action pass_action{};
  sg_bindings bindings{};
  sg_image shadow_map{};

  Camera camera{};

  ShadowPass();
  void run(const Model &model, Light &light);
};
