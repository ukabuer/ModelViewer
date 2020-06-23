#define SOKOL_GLCORE33
#include "LightingPass.hpp"
#include "Geometry.hpp"
#include "shaders/shading.glsl.h"
#include <render_pass/ShadowPass.hpp>

using namespace std;

LightingPass::LightingPass(uint32_t width, uint32_t height,
                           const sg_image &gbuffer_position,
                           const sg_image &gbuffer_normal,
                           const sg_image &gbuffer_albedo) {
  array<float, 4> fake_ao_values = {1.0f, 1.0f, 1.0f, 1.0f};
  sg_image_desc fake_ao_image_desc{};
  fake_ao_image_desc.width = 2;
  fake_ao_image_desc.height = 2;
  fake_ao_image_desc.pixel_format = SG_PIXELFORMAT_R32F;
  fake_ao_image_desc.content.subimage[0][0].ptr = fake_ao_values.data();
  fake_ao_image_desc.content.subimage[0][0].size = 2 * 2 * sizeof(float);
  fake_ao_map = sg_make_image(fake_ao_image_desc);

  sg_image_desc result_image_desc{};
  result_image_desc.render_target = true;
  result_image_desc.width = width;
  result_image_desc.height = height;
  result_image_desc.min_filter = SG_FILTER_LINEAR;
  result_image_desc.mag_filter = SG_FILTER_LINEAR;
  result_image_desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
  result_image_desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
  result_image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;

  sg_pass_desc shading_pass_desc{};
  result = sg_make_image(result_image_desc);
  shading_pass_desc.color_attachments[0].image = result;
  pass = sg_make_pass(shading_pass_desc);

  pass_action.colors[0].action = SG_ACTION_CLEAR;

  sg_pipeline_desc shading_pipeline_desc{};
  shading_pipeline_desc.shader = sg_make_shader(shading_shader_desc());
  shading_pipeline_desc.layout.attrs[ATTR_shading_vs_position].format =
      SG_VERTEXFORMAT_FLOAT2;
  shading_pipeline_desc.layout.attrs[ATTR_shading_vs_uv].format =
      SG_VERTEXFORMAT_FLOAT2;
  shading_pipeline_desc.layout.attrs[ATTR_shading_vs_uv].offset =
      2 * sizeof(float);
  shading_pipeline_desc.layout.buffers[0].stride = 4 * sizeof(float);
  shading_pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  shading_pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  shading_pipeline_desc.blend.color_attachment_count = 1;
  shading_pipeline_desc.blend.color_format = result_image_desc.pixel_format;
  pipeline = sg_make_pipeline(shading_pipeline_desc);

  bindings.fs_images[SLOT_g_world_pos] = gbuffer_position;
  bindings.fs_images[SLOT_g_normal] = gbuffer_normal;
  bindings.fs_images[SLOT_g_albedo] = gbuffer_albedo;
  bindings.vertex_buffers[0] = Quad::GetInstance();
}

void LightingPass::run(const Eigen::Vector3f &view_pos, const Light &light) {
  shading_fs_params_t shading_fs_params{};
  shading_fs_params.view_pos = view_pos;
  shading_fs_params.light_direction = light.direction;
  shading_fs_params.light_matrix = light.matrix;
  bindings.fs_images[SLOT_shadow_map] = light.shadow_pass->shadow_map;

  sg_begin_pass(pass, pass_action);
  sg_apply_pipeline(pipeline);
  sg_apply_bindings(bindings);
  sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_shading_fs_params,
                    &shading_fs_params, sizeof(shading_fs_params_t));
  sg_draw(0, 4, 1);
  sg_end_pass();
}

void LightingPass::disable_ssao() {
  bindings.fs_images[SLOT_ao_map] = fake_ao_map;
}

void LightingPass::enable_ssao(const sg_image &ao_map) {
  bindings.fs_images[SLOT_ao_map] = ao_map;
}