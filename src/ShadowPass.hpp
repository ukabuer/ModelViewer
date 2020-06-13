#pragma once
#include <sokol_gfx.h>
#include "Model.hpp"

class ShadowPass {
public:
  Eigen::Vector3f direction;

  sg_pass pass {};
  sg_pass_action pass_action {};
  sg_bindings bindings {};

  ShadowPass();
  void run(const Model &model);
};
