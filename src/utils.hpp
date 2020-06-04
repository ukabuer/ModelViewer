#pragma once
#include <tinygltf/tiny_gltf.h>
#include <Eigen/Geometry>

auto load_gltf_model(const char* name) -> tinygltf::Model;

auto get_gltf_scene_bound(const tinygltf::Model &gltf_model, int32_t scene_idx) -> Eigen::AlignedBox3f;