#include <Eigen/Core>
#define SOKOL_GLCORE33
#include "Camera.hpp"
#include "Geometry.hpp"
#include "SkyboxPass.hpp"
#include "shaders/irradiance_map.glsl.h"
#include "shaders/prefilter.glsl.h"
#include "shaders/rect_to_cube.glsl.h"
#include "shaders/skybox.glsl.h"
#include <Eigen/Geometry>
#include <string>
#include <tinygltf/stb_image.h>
#include <vector>

using namespace std;
using namespace Eigen;

static sg_image irradiance_convolution(const sg_image &environment_map) {
  sg_image_desc irradiance_map_desc{};
  irradiance_map_desc.type = SG_IMAGETYPE_CUBE;
  irradiance_map_desc.render_target = true;
  irradiance_map_desc.width = 512;
  irradiance_map_desc.height = 512;
  irradiance_map_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
  irradiance_map_desc.min_filter = irradiance_map_desc.mag_filter =
      SG_FILTER_LINEAR;
  auto irradiance_map = sg_make_image(irradiance_map_desc);

  sg_pass_desc pass_desc{};
  pass_desc.color_attachments[0].image = irradiance_map;

  sg_pass_action pass_action{};
  pass_action.colors[0].action = SG_ACTION_CLEAR;

  auto &cube = Cube::GetInstance();
  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(irradiance_map_shader_desc());
  pipeline_desc.layout = cube.layout;
  pipeline_desc.index_type = cube.index_type;
  pipeline_desc.blend.color_format = irradiance_map_desc.pixel_format;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  auto pipeline = sg_make_pipeline(pipeline_desc);

  sg_bindings bindings{};
  bindings.fs_images[SLOT_environment] = environment_map;
  bindings.vertex_buffers[0] = cube.buffer;
  bindings.index_buffer = cube.index_buffer;

  Camera camera{};
  camera.setProjection(90.0f, 1.0f, 0.1f, 10.0f);
  Vector3f targets[] = {
      Vector3f(1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f),
      Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
      Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f)};
  Vector3f ups[] = {Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
                    Vector3f(0.0f, 0.0f, 1.0f),  Vector3f(0.0f, 0.0f, -1.0f),
                    Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f)};

  auto projection_matrix = camera.getCullingProjectionMatrix();
  irradiance_map_vs_params_t vs_params{};
  for (int i = 0; i < 6; i++) {
    pass_desc.color_attachments[0].face = SG_CUBEFACE_POS_X + i;
    camera.lookAt(Vector3f::Zero(), targets[i], ups[i]);
    auto view_matrix = camera.getViewMatrix().inverse();
    vs_params.camera = projection_matrix * view_matrix;
    auto pass = sg_make_pass(pass_desc);

    sg_begin_pass(pass, pass_action);
    sg_apply_pipeline(pipeline);
    sg_apply_bindings(bindings);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_irradiance_map_vs_params,
                      &vs_params, sizeof(irradiance_map_vs_params_t));
    sg_draw(0, 36, 1);
    sg_end_pass();

    sg_destroy_pass(pass);
  }

  return irradiance_map;
}

static sg_image rect_image_to_cube_map(const char *filepath,
                                       sg_buffer cube_buffer,
                                       sg_buffer cube_index_buffer) {
  sg_pass_desc pass_desc{};
  sg_pass_action pass_action{};
  sg_pipeline pipeline{};
  sg_bindings bindings{};
  Camera camera{};
  sg_image output{};

  int image_width = 0, image_height = 0, image_channel = 0;
  stbi_set_flip_vertically_on_load(true);
  auto image_data =
      stbi_loadf(filepath, &image_width, &image_height, &image_channel, 4);
  if (image_data == nullptr) {
    throw runtime_error("Load image failed" + string(filepath));
  }
  stbi_set_flip_vertically_on_load(false);

  sg_image_desc input_image_desc{};
  input_image_desc.width = image_width;
  input_image_desc.height = image_height;
  input_image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
  input_image_desc.min_filter = SG_FILTER_LINEAR;
  input_image_desc.mag_filter = SG_FILTER_LINEAR;
  input_image_desc.content.subimage[0][0].size =
      image_width * image_height * 4 * sizeof(float);
  input_image_desc.content.subimage[0][0].ptr = image_data;
  auto input = sg_make_image(input_image_desc);

  sg_image_desc output_image_desc{};
  output_image_desc.render_target = true;
  output_image_desc.type = SG_IMAGETYPE_CUBE;
  output_image_desc.width = 512;
  output_image_desc.height = 512;
  output_image_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
  input_image_desc.min_filter = SG_FILTER_LINEAR;
  input_image_desc.mag_filter = SG_FILTER_LINEAR;
  output = sg_make_image(output_image_desc);

  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(rect_to_cube_shader_desc());
  pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
  pipeline_desc.layout.attrs[0].buffer_index = 0;
  pipeline_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
  pipeline_desc.blend.color_format = output_image_desc.pixel_format;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  pipeline = sg_make_pipeline(pipeline_desc);

  bindings.vertex_buffers[ATTR_rect_to_cube_vs_position] = cube_buffer;
  bindings.index_buffer = cube_index_buffer;
  bindings.fs_images[SLOT_equirectangular_map] = input;

  pass_action.colors[0].action = SG_ACTION_CLEAR;

  pass_desc.color_attachments[0].image = output;
  camera.setProjection(90.0f, 1.0f, 0.1f, 10.0f);
  Vector3f targets[] = {
      Vector3f(1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f),
      Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
      Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f)};
  Vector3f ups[] = {Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
                    Vector3f(0.0f, 0.0f, 1.0f),  Vector3f(0.0f, 0.0f, -1.0f),
                    Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f)};

  auto projection_matrix = camera.getCullingProjectionMatrix();
  vs_params_t vs_params{};
  for (int i = 0; i < 6; i++) {
    pass_desc.color_attachments[0].face = SG_CUBEFACE_POS_X + i;
    camera.lookAt(Vector3f::Zero(), targets[i], ups[i]);
    auto view_matrix = camera.getViewMatrix().inverse();
    vs_params.camera = (projection_matrix * view_matrix);
    auto pass = sg_make_pass(pass_desc);

    sg_begin_pass(pass, pass_action);
    sg_apply_pipeline(pipeline);
    sg_apply_bindings(bindings);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_vs_params, &vs_params,
                      sizeof(vs_params_t));
    sg_draw(0, 36, 1);
    sg_end_pass();

    sg_destroy_pass(pass);
  }

  return output;
}

static sg_image prefilter_environment_map(const sg_image &environment_map) {
  sg_image_desc prefilter_map_desc{};
  prefilter_map_desc.type = SG_IMAGETYPE_CUBE;
  prefilter_map_desc.render_target = true;
  prefilter_map_desc.width = 128;
  prefilter_map_desc.height = 128;
  prefilter_map_desc.pixel_format = SG_PIXELFORMAT_RGBA32F;
  prefilter_map_desc.min_filter = SG_FILTER_LINEAR_MIPMAP_LINEAR;
  prefilter_map_desc.mag_filter = SG_FILTER_LINEAR;
  prefilter_map_desc.num_mipmaps = 5;
  auto prefilter_map = sg_make_image(prefilter_map_desc);

  auto &cube = Cube::GetInstance();
  sg_bindings bindings{};
  bindings.vertex_buffers[0] = cube.buffer;
  bindings.index_buffer = cube.index_buffer;
  bindings.fs_images[SLOT_environment_map] = environment_map;

  sg_pass_action pass_action{};
  pass_action.colors[0].action = SG_ACTION_CLEAR;

  sg_pipeline_desc pipeline_desc{};
  pipeline_desc.shader = sg_make_shader(prefilter_shader_desc());
  pipeline_desc.layout.attrs[ATTR_prefilter_vs_position].format =
      SG_VERTEXFORMAT_FLOAT3;
  pipeline_desc.layout.attrs[ATTR_prefilter_vs_position].buffer_index = 0;
  pipeline_desc.blend.color_format = prefilter_map_desc.pixel_format;
  pipeline_desc.blend.depth_format = SG_PIXELFORMAT_NONE;
  pipeline_desc.index_type = cube.index_type;
  auto pipeline = sg_make_pipeline(pipeline_desc);

  Camera camera{};
  camera.setProjection(90.0f, 1.0f, 0.1f, 10.0f);
  Vector3f targets[] = {
      Vector3f(1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f),
      Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
      Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f)};
  Vector3f ups[] = {Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
                    Vector3f(0.0f, 0.0f, 1.0f),  Vector3f(0.0f, 0.0f, -1.0f),
                    Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f)};

  prefilter_vs_params_t vs_params{};
  prefilter_fs_params_t fs_params{};
  sg_pass_desc pass_desc{};
  const auto &projection_matrix = camera.getCullingProjectionMatrix();
  pass_desc.color_attachments[0].image = prefilter_map;
  for (int mip = 0; mip < prefilter_map_desc.num_mipmaps; mip++) {
    pass_desc.color_attachments[0].mip_level = mip;
    fs_params.roughness =
        static_cast<float>(mip) /
        static_cast<float>(prefilter_map_desc.num_mipmaps - 1);
    for (int i = 0; i < 6; i++) {
      pass_desc.color_attachments[0].face = SG_CUBEFACE_POS_X + i;
      auto pass = sg_make_pass(pass_desc);

      camera.lookAt(Vector3f::Zero(), targets[i], ups[i]);
      auto view_matrix = camera.getViewMatrix().inverse();

      vs_params.camera = projection_matrix * view_matrix;
      sg_begin_pass(pass, pass_action);
      sg_apply_pipeline(pipeline);
      sg_apply_bindings(bindings);
      sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_prefilter_vs_params, &vs_params,
                        sizeof(vs_params));
      sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_prefilter_fs_params, &fs_params,
                        sizeof(fs_params));
      sg_draw(0, 36, 1);
      sg_end_pass();

      sg_destroy_pass(pass);
    }
  }

  return prefilter_map;
}

SkyboxPass::SkyboxPass(const sg_image &color, const sg_image &depth) {
  auto &cube = Cube::GetInstance();

  environment = rect_image_to_cube_map("assets/textures/Mans_Outside_2k.hdr",
                                       cube.buffer, cube.index_buffer);
  irradiance_map = irradiance_convolution(environment);
  prefilter_map = prefilter_environment_map(environment);

  bindings.vertex_buffers[0] = cube.buffer;
  bindings.index_buffer = cube.index_buffer;
  bindings.fs_images[SLOT_skybox_cube] = prefilter_map;

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