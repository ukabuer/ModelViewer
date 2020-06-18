#pragma once
#include <sokol_gfx.h>

struct BlurPass {
  sg_pass pass{};
  sg_pass_action pass_action{};
  sg_pipeline pipeline{};
  sg_bindings bindings{};
  uint32_t width = 0;
  uint32_t height = 0;

  explicit BlurPass(const sg_image &input);
  void run() const;
};