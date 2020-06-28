#define SOKOL_GLCORE33
#include "BlurPass.hpp"
#include "Geometry.hpp"
#include "shaders/blur.glsl.h"

using namespace std;

BlurPass::BlurPass(const sg_image &input) {
  auto image_info = sg_query_image_info(input);
  sg_image_desc image_desc{};
  image_desc.render_target = true;
  width = image_desc.width = image_info.width;
  height = image_desc.height = image_info.height;
  image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
  ping_pong_images = {sg_make_image(image_desc), input};
  sg_pass_desc pass_desc{};
  for (int i = 0; i < 2; i++) {
    pass_desc.color_attachments[0].image = ping_pong_images[i];
    ping_pong_passes[i] = sg_make_pass(pass_desc);
  }

  pass_action.colors[0].action = SG_ACTION_CLEAR;

  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(blur_shader_desc());
  pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  pipeline_desc.layout.attrs[ATTR_blur_vs_position].format =
      SG_VERTEXFORMAT_FLOAT2;
  pipeline_desc.layout.attrs[ATTR_blur_vs_uv].format = SG_VERTEXFORMAT_FLOAT2;
  pipeline_desc.layout.attrs[ATTR_blur_vs_uv].offset = 2 * sizeof(float);
  pipeline_desc.layout.buffers[0].stride = 4 * sizeof(float);
  pipeline_desc.blend.color_format = image_desc.pixel_format;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  pipeline = sg_make_pipeline(pipeline_desc);

  bindings.vertex_buffers[0] = Quad::GetInstance();

  blur_fs_params_t fs_params{};
  fs_params.texture_width = static_cast<float>(image_info.width);
  fs_params.texture_height = static_cast<float>(image_info.height);
}

void BlurPass::run(uint8_t times) {
  blur_fs_params_t fs_params{};
  fs_params.texture_width = static_cast<float>(width);
  fs_params.texture_height = static_cast<float>(height);

  if (times % 2 == 1) {
    times += 1;
  }
  for (uint8_t i = 0; i < times; i++) {
    bindings.fs_images[SLOT_blur_image] = ping_pong_images[i % 2 ? 0 : 1];
    sg_begin_pass(ping_pong_passes[i % 2], pass_action);
    sg_apply_pipeline(pipeline);
    sg_apply_bindings(bindings);
    sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_blur_fs_params, &fs_params,
                      sizeof(fs_params));
    sg_draw(0, 4, 1);
    sg_end_pass();
  }
}