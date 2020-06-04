#include "utils.hpp"
#include <cfloat>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

auto load_gltf_model(const char *filename) -> tinygltf::Model {
  std::string path = filename;
  auto ext_idx = path.find_last_of('.');
  if (ext_idx == std::string::npos) {
    throw runtime_error("should have extension");
  }
  auto ext = path.substr(ext_idx);
  if (ext != ".glb" && ext != ".gltf") {
    throw runtime_error("Only support GLTF/GLB format.");
  }
  tinygltf::TinyGLTF gltf_loader;
  tinygltf::Model gltf_model;
  string err, warn;
  bool res = false;
  if (ext == ".glb") {
    res = gltf_loader.LoadBinaryFromFile(&gltf_model, &err, &warn, path);
  } else {
    res = gltf_loader.LoadASCIIFromFile(&gltf_model, &err, &warn, path);
  }
  if (!res) {
    throw runtime_error("Failed to load model" + err);
  }
  if (!warn.empty()) {
    cout << "Loading model warning: " << warn << endl;
  }
}

auto get_gltf_scene_bound(const tinygltf::Model &gltf_model, int32_t scene_idx)
    -> Eigen::AlignedBox3f {
  Eigen::AlignedBox3f bound;

  for (int i = 0; i < 3; i++) {
    bound.min()[i] = FLT_MAX;
    bound.max()[i] = -FLT_MAX;
  }

  if (scene_idx >= gltf_model.scenes.size()) {
    throw runtime_error("invalid scene idx");
  }

  auto scene = gltf_model.scenes[scene_idx];
  vector<int32_t> node_indices;
  for (auto node_idx : scene.nodes) {
    auto &gltf_node = gltf_model.nodes[node_idx];
    node_indices.insert(node_indices.end(), gltf_node.children.begin(),
                        gltf_node.children.end());

    if (gltf_node.mesh >= gltf_model.meshes.size()) {
      continue;
    }

    auto &gltf_mesh = gltf_model.meshes[gltf_node.mesh];
    for (auto &gltf_primitive : gltf_mesh.primitives) {
      auto attribute_pos = gltf_primitive.attributes.find("POSITION");
      if (attribute_pos == gltf_primitive.attributes.end()) {
        continue;
      }

      auto accessor_idx = attribute_pos->second;
      if (accessor_idx >= gltf_model.accessors.size()) {
        continue;
      }

      auto accessor = gltf_model.accessors[accessor_idx];
      Eigen::Matrix4f transform;
      for (size_t i = 0; i < gltf_node.matrix.size(); i++) {
        transform.data()[i] = gltf_node.matrix[i];
      }
      Eigen::Vector3f min{accessor.minValues[0], accessor.minValues[1],
                          accessor.minValues[2]};
      Eigen::Vector3f max{accessor.maxValues[0], accessor.maxValues[1],
                          accessor.maxValues[2]};
      Eigen::Vector4f box_min =
          transform * Eigen::Vector4f(min[0], min[1], min[2], 1.0f);
      Eigen::Vector4f box_max =
          transform * Eigen::Vector4f(max[0], max[1], max[2], 1.0f);
      box_max /= box_max[3];
      box_min /= box_min[3];
      Eigen::Vector4f center = (box_min + box_max) / 2.0f;
      Eigen::Vector4f center_to_surface = box_max - center;
      center_to_surface[3] = 0.0f;
      auto distance = center_to_surface.norm();

      for (int i = 0; i < 3; i++) {
        bound.min()[i] = std::min(bound.min()[i], center[i] - distance);
        bound.max()[i] = std::max(bound.max()[i], center[i] + distance);
      }
    }
  }

  return bound;
}