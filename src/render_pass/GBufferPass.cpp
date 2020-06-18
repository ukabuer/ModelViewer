#define SOKOL_GLCORE33
#include "GBufferPass.hpp"
#include "shaders/gbuffer.glsl.h"
#include <stack>
#include <unordered_set>

using namespace std;

GBufferPass::GBufferPass(uint32_t width, uint32_t height) {
  sg_image_desc image_desc{};
  image_desc.render_target = true;
  image_desc.width = width;
  image_desc.height = height;
  image_desc.min_filter = SG_FILTER_LINEAR;
  image_desc.mag_filter = SG_FILTER_LINEAR;
  image_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  image_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;

  sg_image_desc depth_desc = image_desc;
  depth_desc.pixel_format = SG_PIXELFORMAT_DEPTH;

  position = sg_make_image(image_desc);
  normal = sg_make_image(image_desc);
  albedo = sg_make_image(image_desc);
  depth = sg_make_image(depth_desc);

  sg_pass_desc gbuffer_pass_desc{};
  gbuffer_pass_desc.color_attachments[0].image = position;
  gbuffer_pass_desc.color_attachments[1].image = normal;
  gbuffer_pass_desc.color_attachments[2].image = albedo;
  gbuffer_pass_desc.depth_stencil_attachment.image = depth;
  pass = sg_make_pass(gbuffer_pass_desc);

  for (int i = 0; i < 3; i++) {
    pass_action.colors[i].action = SG_ACTION_CLEAR;
  }
  pass_action.depth.action = SG_ACTION_CLEAR;
  pass_action.depth.val = 1.0f;
}

void GBufferPass::run(const Model &model,
                      const Eigen::Matrix4f &camera_matrix) {
  gbuffer_vs_params_t gbuffer_vs_params{};
  gbuffer_vs_params.camera = camera_matrix;
  auto scene = model.gltf.scenes[model.gltf.defaultScene];

  sg_begin_pass(pass, pass_action);
  auto gltf_node_idxs = scene.nodes;
  unordered_set<int> processed{};
  if (!gltf_node_idxs.empty()) {
    stack<int> remain_nodes{};
    stack<Eigen::Matrix4f> transforms{};
    remain_nodes.emplace(gltf_node_idxs[0]);

    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
    for (auto i = 0; i < 16; i++) {
      transform.data()[i] = model.gltf.nodes[remain_nodes.top()].matrix[i];
    }
    transforms.emplace(move(transform));
    while (!remain_nodes.empty()) {
      auto &current_idx = remain_nodes.top();
      auto &current = model.gltf.nodes[current_idx];

      auto has_new = false;
      for (auto child_idx : current.children) {
        if (processed.find(child_idx) == processed.end()) {
          remain_nodes.emplace(child_idx);
          auto &child_node = model.gltf.nodes[child_idx];
          auto &parent_transform = transforms.top();
          Eigen::Matrix4f child_transform = Eigen::Matrix4f::Identity();
          for (auto i = 0; i < 16; i++) {
            child_transform.data()[i] = child_node.matrix[i];
          }
          transforms.emplace(parent_transform * child_transform);
          has_new = true;
          break;
        }
      }

      if (has_new) {
        continue;
      }

      if (current.mesh != -1) {
        auto &gltf_mesh = model.gltf.meshes[current.mesh];
        for (uint32_t idx = 0; idx < gltf_mesh.primitives.size(); idx++) {
          string id = to_string(current.mesh) + "-" + to_string(idx);
          auto mesh_pos = model.meshes.find(id);
          if (mesh_pos == model.meshes.end()) {
            continue;
          }
          auto &mesh = mesh_pos->second;

          sg_apply_pipeline(mesh.pipeline);

          bindings.index_buffer = mesh.geometry.indices;
          bindings.vertex_buffers[ATTR_gbuffer_vs_position] =
              mesh.geometry.positions;
          bindings.vertex_buffers[ATTR_gbuffer_vs_normal] =
              mesh.geometry.normals;
          bindings.vertex_buffers[ATTR_gbuffer_vs_uv] = mesh.geometry.uvs;
          bindings.fs_images[SLOT_albedo] = mesh.albedo;
          sg_apply_bindings(bindings);

          gbuffer_vs_params.model = Eigen::Matrix4f::Identity();
          for (int i = 0; i < current.matrix.size(); i++) {
            gbuffer_vs_params.model.data()[i] = current.matrix[i];
          }
          sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_gbuffer_vs_params,
                            &gbuffer_vs_params, sizeof(gbuffer_vs_params_t));
          sg_draw(0, mesh.geometry.num, 1);
        }
      }

      processed.emplace(remain_nodes.top());
      remain_nodes.pop();
      transforms.pop();
    }
  }
  sg_end_pass();
}
