#pragma once
#include <sokol_gfx.h>
#include <Eigen/Core>

struct SkyboxPass {
public:
  sg_pass pass {};
  sg_pass_action pass_action {};
  sg_pipeline pipeline {};
  sg_bindings bindings {};

  SkyboxPass(const sg_image &color, const sg_image &depth);
  void run(const Eigen::Matrix4f &camera_matrix) const;
};
