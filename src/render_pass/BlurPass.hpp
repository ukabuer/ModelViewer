#pragma once
#include <array>
#include <sokol_gfx.h>

struct BlurPass {
  std::array<sg_pass, 2> ping_pong_passes{};
  sg_pass_action pass_action{};
  sg_pipeline pipeline{};
  sg_bindings bindings{};
  std::array<sg_image, 2> ping_pong_images{};
  uint32_t width = 0;
  uint32_t height = 0;

  explicit BlurPass(const sg_image &input);
  void run(uint8_t times);
};