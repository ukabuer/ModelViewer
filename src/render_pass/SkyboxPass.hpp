#pragma once
#include <Eigen/Core>
#include <sokol_gfx.h>

struct SkyboxPass {
public:
  sg_pass pass{};
  sg_pass_action pass_action{};
  sg_pipeline pipeline{};
  sg_bindings bindings{};
  sg_image environment{};
  sg_image irradiance_map{};
  sg_image prefilter_map{};

  SkyboxPass(const sg_image &color, const sg_image &depth);
  void run(const Eigen::Matrix4f &camera_matrix) const;
};
