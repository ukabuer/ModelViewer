#pragma once
#define SOKOL_GLCORE33
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cstdint>
#include <sokol_gfx.h>
#include <tinygltf/tiny_gltf.h>
#include <unordered_map>
#include <vector>

struct Geometry {
  sg_buffer vertices;
  sg_buffer indices;
  uint32_t num;
};

struct Mesh {
  Geometry geometry{};
  // Material material {};
  sg_pipeline pipeline{};
};

class Model {
public:
  tinygltf::Model gltf;
  std::vector<sg_buffer> buffers;
  std::vector<sg_image> textures;
  std::unordered_map<std::string, Mesh> meshes;

  static auto Load(const char *filename) -> Model;
};
