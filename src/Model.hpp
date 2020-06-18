#pragma once
#include "Geometry.hpp"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cstdint>
#include <sokol_gfx.h>
#include <tinygltf/tiny_gltf.h>
#include <unordered_map>
#include <vector>

struct Mesh {
  Geometry geometry{};
  // Material material {};
  sg_image albedo;
  sg_pipeline pipeline{};
  sg_pipeline shadow_pass_pipeline{};
};

class Model {
public:
  tinygltf::Model gltf;
  std::vector<sg_buffer> buffers;
  std::vector<sg_image> textures;
  std::unordered_map<std::string, Mesh> meshes;

  static auto Load(const char *filename) -> Model;
};
