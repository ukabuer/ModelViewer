#define SOKOL_GLCORE33
#include "SSAOPass.hpp"
#include "shaders/SSAO.glsl.h"
#include <Eigen/Core>
#include <random>
#include <vector>

using namespace std;

SSAOPass::SSAOPass(uint32_t width, uint32_t height, const sg_image &position,
                   const sg_image &normal) {
  screen_width = width;
  screen_height = height;

  uniform_real_distribution<float> random_floats(0.0, 1.0);
  default_random_engine generator{};
  vector<Eigen::Vector4f> noise{};
  for (uint32_t i = 0; i < 4 * 4; i++) {
    noise.emplace_back(Eigen::Vector4f{random_floats(generator) * 2.0f - 1.0f,
                                       random_floats(generator) * 2.0f - 1.0f,
                                       random_floats(generator) * 2.0f - 1.0f,
                                       0.0f});
  }
  sg_image_desc noise_image_desc{};
  noise_image_desc.width = 4;
  noise_image_desc.height = 4;
  noise_image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
  noise_image_desc.content.subimage[0][0].ptr = noise.data();
  noise_image_desc.content.subimage[0][0].size =
      noise.size() * sizeof(Eigen::Vector4f);
  auto noise_image = sg_make_image(noise_image_desc);

  sg_image_desc ao_image_desc{};
  ao_image_desc.render_target = true;
  ao_image_desc.width = width;
  ao_image_desc.height = height;
  ao_image_desc.pixel_format = SG_PIXELFORMAT_R32F;
  ao_image_desc.min_filter = ao_image_desc.mag_filter = SG_FILTER_LINEAR;
  origin_ao_map = sg_make_image(ao_image_desc);
  ao_map = sg_make_image(ao_image_desc);

  sg_pass_desc pass_desc{};
  pass_desc.color_attachments[0].image = origin_ao_map;
  pass = sg_make_pass(pass_desc);

  pass_desc.color_attachments[0].image = ao_map;
  blur_pass = sg_make_pass(pass_desc);

  pass_action.colors[0].action = SG_ACTION_CLEAR;
  pass_action.depth.action = SG_ACTION_DONTCARE;

  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(ssao_shader_desc());
  pipeline_desc.layout.attrs[0].buffer_index = 0;
  pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
  pipeline_desc.layout.attrs[1].buffer_index = 0;
  pipeline_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
  pipeline_desc.layout.attrs[1].offset = 2 * sizeof(float);
  pipeline_desc.layout.buffers[0].stride = 4 * sizeof(float);
  pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  pipeline_desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
  pipeline_desc.blend.color_format = ao_image_desc.pixel_format;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  pipeline = sg_make_pipeline(pipeline_desc);
  pipeline_desc.shader = sg_make_shader(ssao_blur_shader_desc());
  blur_pipeline = sg_make_pipeline(pipeline_desc);

  const vector<float> screen_quad = {
      -1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0,
      -1.0, 1.0,  0.0, 1.0, 1.0, 1.0,  1.0, 1.0,
  };

  sg_buffer_desc screen_quad_desc = {};
  screen_quad_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
  screen_quad_desc.size = screen_quad.size() * sizeof(float);
  screen_quad_desc.content = screen_quad.data();
  auto screen_quad_buffer = sg_make_buffer(screen_quad_desc);
  bindings.vertex_buffers[0] = screen_quad_buffer;
  bindings.fs_images[SLOT_position_tex] = position;
  bindings.fs_images[SLOT_normal_tex] = normal;
  bindings.fs_images[SLOT_noise_tex] = noise_image;
  blur_bindings.vertex_buffers[0] = screen_quad_buffer;
  blur_bindings.fs_images[SLOT_blur_image] = origin_ao_map;

  for (uint32_t i = 0; i < 64; ++i) {
    Eigen::Vector3f sample{random_floats(generator) * 2.0f - 1.0f,
                           random_floats(generator) * 2.0f - 1.0f,
                           random_floats(generator)};
    sample.normalize();
    sample *= random_floats(generator);
    ssao_kernel.emplace_back(move(sample));
  }
}

void SSAOPass::run(const Eigen::Matrix4f &view_matrix,
                   const Eigen::Matrix4f &projection_matrix) const {
  fs_params_t fs_params{};
  fs_params.view_matrix = view_matrix;
  fs_params.projection_matrix = projection_matrix;
  fs_params.screen_width = screen_width;
  fs_params.screen_height = screen_height;
  for (uint32_t i = 0; i < ssao_kernel.size(); i++) {
    for (uint32_t j = 0; j < 3; j++) {
      fs_params.samples[i][j] = ssao_kernel[i][j];
    }
  }

  sg_begin_pass(pass, pass_action);
  sg_apply_pipeline(pipeline);
  sg_apply_bindings(bindings);
  sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_fs_params, &fs_params,
                    sizeof(fs_params_t));
  sg_draw(0, 4, 1);
  sg_end_pass();

  ssao_blur_fs_params_t blur_fs_params{};
  blur_fs_params.texture_width = screen_width;
  blur_fs_params.texture_height = screen_height;
  sg_begin_pass(blur_pass, pass_action);
  sg_apply_pipeline(blur_pipeline);
  sg_apply_bindings(blur_bindings);
  sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_ssao_blur_fs_params,
                    &blur_fs_params, sizeof(blur_fs_params));
  sg_draw(0, 4, 1);
  sg_end_pass();
}