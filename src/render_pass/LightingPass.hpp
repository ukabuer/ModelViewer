#pragma once
#include "Light.hpp"
#include <Eigen/Core>
#include <sokol_gfx.h>

struct LightingPass {
  sg_pass pass{};
  sg_pass_action pass_action{};
  sg_pipeline pipeline{};
  sg_bindings bindings{};
  sg_image result{};
  sg_image fake_ao_map{};

  LightingPass(uint32_t width, uint32_t height,
               const sg_image &gbuffer_position, const sg_image &gbuffer_normal,
               const sg_image &gbuffer_albedo,
               const sg_image &gbuffer_emissive);
  void run(const Eigen::Vector3f &view_pos, const Light &light);
  void set_irradiance_map(const sg_image &irradiance_map);
  void set_prefilter_map(const sg_image &prefilter_map);

  void disable_ssao();
  void enable_ssao(const sg_image &ao_map);
};
