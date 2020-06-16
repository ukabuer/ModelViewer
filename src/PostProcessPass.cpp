#define SOKOL_GLCORE33
#include "PostProcessPass.hpp"
#include "shaders/postprocess.glsl.h"
#include <vector>

using namespace std;

PostProccesPass::PostProccesPass(const sg_image &rendered) {
  pass_action.colors[0].action = SG_ACTION_CLEAR;

  sg_pipeline_desc postprocess_pipeline_desc{};
  postprocess_pipeline_desc.shader = sg_make_shader(postprocess_shader_desc());
  postprocess_pipeline_desc.layout.attrs[ATTR_postprocess_vs_position].format =
      SG_VERTEXFORMAT_FLOAT2;
  postprocess_pipeline_desc.layout.attrs[ATTR_postprocess_vs_uv].format =
      SG_VERTEXFORMAT_FLOAT2;
  postprocess_pipeline_desc.layout.attrs[ATTR_postprocess_vs_uv].offset =
      2 * sizeof(float);
  postprocess_pipeline_desc.layout.buffers[0].stride = 4 * sizeof(float);
  postprocess_pipeline_desc.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  postprocess_pipeline_desc.blend.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
  postprocess_pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA8;

  pipeline = sg_make_pipeline(postprocess_pipeline_desc);

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
  bindings.fs_images[SLOT_rendered] = rendered;
}

void PostProccesPass::run(uint32_t width, uint32_t height) const {
  sg_begin_default_pass(pass_action, width, height);
  sg_apply_pipeline(pipeline);
  sg_apply_bindings(bindings);
  sg_draw(0, 4, 1);
  sg_end_pass();
}