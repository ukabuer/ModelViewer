#pragma once
#include "ShadowPass.hpp"
#include <Eigen/Core>
#include <sokol_gfx.h>

struct LightingPass {
  sg_pass pass{};
  sg_pass_action pass_action{};
  sg_pipeline pipeline{};
  sg_bindings bindings{};
  sg_image result{};

  LightingPass(uint32_t width, uint32_t height,
               const sg_image &gbuffer_position, const sg_image &gbuffer_normal,
               const sg_image &gbuffer_albedo);
  void run(const Eigen::Vector3f &view_pos, const Light &light);
};
