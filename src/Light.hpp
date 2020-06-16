#pragma once
#include <sokol_gfx.h>

struct Light {
  Eigen::Vector3f direction;
  Eigen::Matrix4f matrix;
  sg_image shadow_map;
};
