#pragma once
#include "Model.hpp"
#include <Eigen/Core>
#include <sokol_gfx.h>

struct GBufferPass {
  sg_pass pass{};
  sg_pass_action pass_action{};
  sg_pipeline pipeline{};
  sg_bindings bindings{};

  sg_image position{};
  sg_image normal{};
  sg_image albedo{};
  sg_image depth{};

  GBufferPass(uint32_t width, uint32_t height);
  void run(const Model &model, const Eigen::Matrix4f &camera_matrix);
};