#pragma once
#include <sokol_gfx.h>
#include "Model.hpp"

struct Light {
  Eigen::Vector3f direction;
  Eigen::Matrix4f matrix;
  sg_image shadow_map;
};

struct ShadowPass {
  Eigen::Vector3f direction;

  sg_pass pass {};
  sg_pass_action pass_action {};
  sg_bindings bindings {};

  Light light {};

  Eigen::Matrix4f light_space_matrix {};

  ShadowPass();
  void run(const Model &model);
};
