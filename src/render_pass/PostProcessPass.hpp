#pragma once
#include <sokol_gfx.h>

struct PostProccesPass {
  sg_pass_action pass_action{};
  sg_bindings bindings{};
  sg_pipeline pipeline{};

  explicit PostProccesPass(const sg_image &rendered,
                           const sg_image &bright_color);
  void run(uint32_t width, uint32_t height);
};