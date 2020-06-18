#define SOKOL_GLCORE33
#include "SkyboxPass.hpp"
#include "shaders/skybox.glsl.h"
#include <string>
#include <tinygltf/stb_image.h>
#include <vector>
#include <Eigen/Core>

using namespace std;

SkyboxPass::SkyboxPass(const sg_image &color, const sg_image &depth) {
  vector<string> faces = {
      "assets/textures/skybox/right.jpg", "assets/textures/skybox/left.jpg",
      "assets/textures/skybox/top.jpg",   "assets/textures/skybox/bottom.jpg",
      "assets/textures/skybox/front.jpg", "assets/textures/skybox/back.jpg",
  };
  sg_image_desc skybox_texture_desc{};
  skybox_texture_desc.type = SG_IMAGETYPE_CUBE;
  skybox_texture_desc.width = 0;
  skybox_texture_desc.height = 0;
  auto image_channels = 0;
  for (uint32_t i = 0; i < 6; i++) {
    auto data = stbi_load(faces[i].c_str(), &skybox_texture_desc.width,
                          &skybox_texture_desc.height, &image_channels, 4);
    skybox_texture_desc.content.subimage[i][0].ptr = data;
    skybox_texture_desc.content.subimage[i][0].size =
        skybox_texture_desc.width * skybox_texture_desc.width * image_channels;
  }
  auto skybox_cube = sg_make_image(skybox_texture_desc);

  // clang-format off
  vector<float> skybox_positions = {
      // top
      -1.0f,  1.0f, -1.0f,
      -1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,
      1.0f,  1.0f, -1.0f,
      // bottom
      -1.0f, -1.0f,  1.0f,
      -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f,  1.0f,
      // left
      -1.0f,  1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f,  1.0f,
      -1.0f,  1.0f,  1.0f,
      // right
      1.0f,  1.0f,  1.0f,
      1.0f, -1.0f,  1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f,  1.0f, -1.0f,
      // front
      -1.0f,  1.0f,  1.0f,
      -1.0f, -1.0f,  1.0f,
      1.0f,  -1.0f,  1.0f,
      1.0f,   1.0f,  1.0f,
      // back
      1.0f,  1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f,  1.0f, -1.0f,
  };
  // clang-format on
  vector<uint16_t> skybox_indices = {
      0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,  8,  10, 9,  8,  11, 10,
      12, 14, 13, 12, 15, 14, 16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22,
  };
  sg_buffer_desc skybox_vertices_buffer_desc{};
  skybox_vertices_buffer_desc.content = skybox_positions.data();
  skybox_vertices_buffer_desc.size = skybox_positions.size() * sizeof(float);

  sg_buffer_desc skybox_indices_buffer_desc{};
  skybox_indices_buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
  skybox_indices_buffer_desc.content = skybox_indices.data();
  skybox_indices_buffer_desc.size = skybox_indices.size() * sizeof(uint16_t);

  bindings.vertex_buffers[0] = sg_make_buffer(skybox_vertices_buffer_desc);
  bindings.index_buffer = sg_make_buffer(skybox_indices_buffer_desc);
  bindings.fs_images[SLOT_skybox_cube] = skybox_cube;

  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(skybox_shader_desc());
  pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
  pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
  pipeline_desc.layout.attrs[ATTR_skybox_vs_position].format =
      SG_VERTEXFORMAT_FLOAT3;
  pipeline_desc.layout.attrs[ATTR_skybox_vs_position].buffer_index = 0;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_DEPTH;
  pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA32F;
  pipeline_desc.depth_stencil.depth_compare_func =
      SG_COMPAREFUNC_LESS_EQUAL;
  pipeline = sg_make_pipeline(pipeline_desc);

  pass_action.colors[0].action = SG_ACTION_DONTCARE;
  pass_action.depth.action = SG_ACTION_DONTCARE;
  pass_action.stencil.action = SG_ACTION_DONTCARE;

  sg_pass_desc skybox_pass_desc{};
  skybox_pass_desc.color_attachments[0].image = color;
  skybox_pass_desc.depth_stencil_attachment.image = depth;
  pass = sg_make_pass(skybox_pass_desc);
}

void SkyboxPass::run(const Eigen::Matrix4f &camera_matrix) const {
  sg_begin_pass(pass, pass_action);
  sg_apply_pipeline(pipeline);
  sg_apply_bindings(bindings);
  sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skybox_vs_params,
                    camera_matrix.data(), sizeof(Eigen::Matrix4f));
  sg_draw(0, 36, 1);
  sg_end_pass();
}