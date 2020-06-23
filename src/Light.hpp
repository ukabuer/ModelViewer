#pragma once
#include <Eigen/Core>
#include <memory>
#include <sokol_gfx.h>

struct ShadowPass;

struct Light {
  Eigen::Vector3f direction;
  Eigen::Matrix4f matrix;
  std::unique_ptr<ShadowPass> shadow_pass;

  Light();
};
