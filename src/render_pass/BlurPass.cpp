#define SOKOL_GLCORE33
#include "BlurPass.hpp"
#include "Geometry.hpp"
#include "shaders/blur.glsl.h"

BlurPass::BlurPass(const sg_image &input) {
  auto input_info = sg_query_image_info(input);
  width = input_info.width;
  height = input_info.height;

  sg_pass_desc pass_desc{};
  pass_desc.color_attachments[0].image = input;
  pass = sg_make_pass(pass_desc);

  pass_action.colors[0].action = SG_ACTION_DONTCARE;
  pass_action.depth.action = SG_ACTION_DONTCARE;

  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(blur_shader_desc());
  pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  pipeline_desc.layout.attrs[ATTR_blur_vs_position].format =
      SG_VERTEXFORMAT_FLOAT2;
  pipeline_desc.layout.attrs[ATTR_blur_vs_uv].format = SG_VERTEXFORMAT_FLOAT2;
  pipeline_desc.layout.attrs[ATTR_blur_vs_uv].offset = 2 * sizeof(float);
  pipeline_desc.layout.buffers[0].stride = 4 * sizeof(float);
  pipeline_desc.blend.color_format = SG_PIXELFORMAT_R32F;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  pipeline = sg_make_pipeline(pipeline_desc);

  bindings.fs_images[SLOT_tex] = input;
  bindings.vertex_buffers[0] = Quad::GetInstance();
}

void BlurPass::run() const {
  blur_fs_params_t blur_fs_params{};
  blur_fs_params.texture_width = static_cast<float>(width);
  blur_fs_params.texture_height = static_cast<float>(height);

  sg_begin_pass(pass, pass_action);
  sg_apply_pipeline(pipeline);
  sg_apply_bindings(bindings);
  sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_blur_fs_params, &blur_fs_params,
                    sizeof(blur_fs_params_t));
  sg_draw(0, 4, 1);
  sg_end_pass();
}