#include <Eigen/Core>
#define SOKOL_GLCORE33
#include "Geometry.hpp"
#include "SkyboxPass.hpp"
#include "render_pass/utils.hpp"
#include "shaders/skybox.glsl.h"

using namespace std;
using namespace Eigen;

SkyboxPass::SkyboxPass(const sg_image &color, const sg_image &depth) {
  auto &cube = Cube::GetInstance();

  environment = rect_image_to_cube_map("assets/textures/Mans_Outside_2k.hdr",
                                       cube.buffer, cube.index_buffer);
  irradiance_map = irradiance_convolution(environment);
  prefilter_map = prefilter_environment_map(environment);

  bindings.vertex_buffers[0] = cube.buffer;
  bindings.index_buffer = cube.index_buffer;
  bindings.fs_images[SLOT_skybox_cube] = environment;

  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(skybox_shader_desc());
  pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
  pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
  pipeline_desc.layout.attrs[ATTR_skybox_vs_position].format =
      SG_VERTEXFORMAT_FLOAT3;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_DEPTH;
  pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA32F;
  pipeline_desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL;
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