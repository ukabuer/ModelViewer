#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Model.hpp"
#include "shaders/render.glsl.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <iostream>

using namespace std;
using namespace Eigen;

namespace {
auto get_attribute_format(const tinygltf::Accessor &gltf_accessor)
    -> sg_vertex_format {
  switch (gltf_accessor.componentType) {
  case TINYGLTF_COMPONENT_TYPE_BYTE:
    if (gltf_accessor.type == TINYGLTF_TYPE_VEC4) {
      return gltf_accessor.normalized ? SG_VERTEXFORMAT_BYTE4N
                                      : SG_VERTEXFORMAT_BYTE4;
    }
    break;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    if (gltf_accessor.type == TINYGLTF_TYPE_VEC4) {
      return gltf_accessor.normalized ? SG_VERTEXFORMAT_UBYTE4N
                                      : SG_VERTEXFORMAT_UBYTE4;
    }
    break;
  case TINYGLTF_COMPONENT_TYPE_SHORT:
    switch (gltf_accessor.type) {
    case TINYGLTF_TYPE_VEC2:
      return gltf_accessor.normalized ? SG_VERTEXFORMAT_SHORT2N
                                      : SG_VERTEXFORMAT_SHORT2;
    case TINYGLTF_TYPE_VEC4:
      return gltf_accessor.normalized ? SG_VERTEXFORMAT_SHORT4N
                                      : SG_VERTEXFORMAT_SHORT4;
    default:
      break;
    }
    break;
  case TINYGLTF_COMPONENT_TYPE_FLOAT:
    switch (gltf_accessor.type) {
    case TINYGLTF_TYPE_SCALAR:
      return SG_VERTEXFORMAT_FLOAT;
    case TINYGLTF_TYPE_VEC2:
      return SG_VERTEXFORMAT_FLOAT2;
    case TINYGLTF_TYPE_VEC3:
      return SG_VERTEXFORMAT_FLOAT3;
    case TINYGLTF_TYPE_VEC4:
      return SG_VERTEXFORMAT_FLOAT4;
    default:
      break;
    }
    break;
  }

  return SG_VERTEXFORMAT_INVALID;
}

auto get_primitive_type(const tinygltf::Primitive &gltf_primitive) {
  switch (gltf_primitive.mode) {
  case TINYGLTF_MODE_POINTS:
    return SG_PRIMITIVETYPE_POINTS;
  case TINYGLTF_MODE_LINE:
    return SG_PRIMITIVETYPE_LINES;
  case TINYGLTF_MODE_LINE_STRIP:
    return SG_PRIMITIVETYPE_LINE_STRIP;
  case TINYGLTF_MODE_TRIANGLES:
    return SG_PRIMITIVETYPE_TRIANGLES;
  case TINYGLTF_MODE_TRIANGLE_STRIP:
    return SG_PRIMITIVETYPE_TRIANGLE_STRIP;
  default:
    return SG_PRIMITIVETYPE_TRIANGLES;
  }
}

auto get_component_type(const tinygltf::Accessor &gltf_accessor) {
  switch (gltf_accessor.componentType) {
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    return SG_INDEXTYPE_UINT16;
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
  default:
    return SG_INDEXTYPE_UINT32;
  }
}
} // namespace

auto Model::Load(const char *filename) -> Model {
  string path = filename;
  auto ext_idx = path.find_last_of('.');
  if (ext_idx == string::npos) {
    throw runtime_error("should have extension");
  }
  auto ext = path.substr(ext_idx);
  if (ext != ".glb" && ext != ".gltf") {
    throw runtime_error("Only support GLTF/GLB format.");
  }

  tinygltf::TinyGLTF loader;
  tinygltf::Model gltf_model;
  string err, warn;
  if (ext == ".glb") {
    loader.LoadBinaryFromFile(&gltf_model, &err, &warn, path);
  } else {
    loader.LoadASCIIFromFile(&gltf_model, &err, &warn, path);
  }
  if (!err.empty()) {
    throw runtime_error("Failed to load model: " + err);
  }
  if (!warn.empty()) {
    cout << "Loading model warning: " << warn << endl;
  }

  auto shader_desc = deferred_shader_desc();
  auto shader = sg_make_shader(shader_desc);

  Model model{};

  // setup buffers
  for (auto &gltf_buffer_view : gltf_model.bufferViews) {
    auto &gltf_buffer = gltf_model.buffers[gltf_buffer_view.buffer];

    sg_buffer_desc buffer_desc{};
    switch (gltf_buffer_view.target) {
    case TINYGLTF_TARGET_ARRAY_BUFFER:
      buffer_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
      break;
    case TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER:
      buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    default:
      break;
    }
    buffer_desc.size = gltf_buffer_view.byteLength;
    buffer_desc.content = gltf_buffer.data.data() + gltf_buffer_view.byteOffset;
    model.buffers.emplace_back(sg_make_buffer(buffer_desc));
  }

  // setup textures
  for (auto &gltf_texture : gltf_model.textures) {
    auto &image = gltf_model.images[gltf_texture.source];
    auto &sampler = gltf_model.samplers[gltf_texture.sampler];

    sg_image_desc image_desc{};
    image_desc.width = image.width;
    image_desc.height = image.height;
    image_desc.content.subimage[0][0].ptr = image.image.data();
    image_desc.content.subimage[0][0].size = image.image.size();
    image_desc.min_filter = sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
                                ? SG_FILTER_NEAREST
                                : SG_FILTER_LINEAR;
    image_desc.mag_filter = sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
                                ? SG_FILTER_NEAREST
                                : SG_FILTER_LINEAR;
    image_desc.wrap_u = sampler.wrapS == TINYGLTF_TEXTURE_WRAP_REPEAT
                            ? SG_WRAP_REPEAT
                            : SG_WRAP_CLAMP_TO_EDGE;
    image_desc.wrap_v = sampler.wrapT == TINYGLTF_TEXTURE_WRAP_REPEAT
                            ? SG_WRAP_REPEAT
                            : SG_WRAP_CLAMP_TO_EDGE;

    model.textures.emplace_back(sg_make_image(image_desc));
  }

  // setup meshes
  for (uint32_t i = 0; i < gltf_model.meshes.size(); i++) {
    auto &gltf_mesh = gltf_model.meshes[i];
    for (uint32_t j = 0; j < gltf_mesh.primitives.size(); j++) {
      auto &primitive = gltf_mesh.primitives[j];
      assert(primitive.attributes.size() <= SG_MAX_VERTEX_ATTRIBUTES);

      Mesh mesh{};
      sg_pipeline_desc pipeline_desc{};
      pipeline_desc.primitive_type = get_primitive_type(primitive);
      if (primitive.indices >= 0) {
        auto &gltf_accessor = gltf_model.accessors[primitive.indices];
        auto &buffer = model.buffers[gltf_accessor.bufferView];
        mesh.geometry.indices = buffer;
        mesh.geometry.num = gltf_accessor.count;
        pipeline_desc.index_type = get_component_type(gltf_accessor);
      }
      auto position_pos = primitive.attributes.find("POSITION");
      if (position_pos != primitive.attributes.end()) {
        auto &gltf_accessor = gltf_model.accessors[position_pos->second];
        auto &buffer = model.buffers[gltf_accessor.bufferView];
        mesh.geometry.positions = buffer;
        if (mesh.geometry.num == 0) {
          mesh.geometry.num = gltf_accessor.count;
        }
        pipeline_desc.layout.attrs[ATTR_vs_position].format =
            get_attribute_format(gltf_accessor);
        pipeline_desc.layout.attrs[ATTR_vs_position].buffer_index = 0;
        pipeline_desc.layout.attrs[ATTR_vs_position].offset =
            gltf_accessor.byteOffset;
      } else {
        throw runtime_error("no pos");
      }
      auto normal_pos = primitive.attributes.find("NORMAL");
      if (normal_pos != primitive.attributes.end()) {
        auto &gltf_accessor = gltf_model.accessors[normal_pos->second];
        auto &buffer = model.buffers[gltf_accessor.bufferView];
        mesh.geometry.normals = buffer;
        pipeline_desc.layout.attrs[ATTR_vs_normal].format =
            get_attribute_format(gltf_accessor);
        pipeline_desc.layout.attrs[ATTR_vs_normal].buffer_index = 1;
        pipeline_desc.layout.attrs[ATTR_vs_normal].offset =
            gltf_accessor.byteOffset;
      } else {
        throw runtime_error("no normal");
      }
      auto uv_pos = primitive.attributes.find("TEXCOORD_0");
      if (uv_pos != primitive.attributes.end()) {
        auto &gltf_accessor = gltf_model.accessors[uv_pos->second];
        auto &buffer = model.buffers[gltf_accessor.bufferView];
        mesh.geometry.uvs = buffer;
        pipeline_desc.layout.attrs[ATTR_vs_uv].format =
            get_attribute_format(gltf_accessor);
        pipeline_desc.layout.attrs[ATTR_vs_uv].buffer_index = 2;
        pipeline_desc.layout.attrs[ATTR_vs_uv].offset =
            gltf_accessor.byteOffset;
      } else {
        throw runtime_error("no uv");
      }

      pipeline_desc.shader = shader;
      pipeline_desc.rasterizer.cull_mode = SG_CULLMODE_BACK;
      pipeline_desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
      pipeline_desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS;
      pipeline_desc.depth_stencil.depth_write_enabled = true;
      pipeline_desc.blend.color_attachment_count = 3;
      pipeline_desc.blend.color_format = SG_PIXELFORMAT_RGBA32F;
      pipeline_desc.blend.depth_format = SG_PIXELFORMAT_DEPTH;
      mesh.pipeline = sg_make_pipeline(pipeline_desc);

      // handle material
      auto material_idx = primitive.material;
      auto &gltf_material = gltf_model.materials[material_idx];
      auto &pbr_params = gltf_material.pbrMetallicRoughness;
      if (pbr_params.baseColorTexture.index == -1) {
        throw runtime_error("no base color texture");
      }
      mesh.albedo = model.textures[pbr_params.baseColorTexture.index];

      string id = to_string(i) + "-" + to_string(j);
      model.meshes.emplace(id, mesh);
    }
  }

  // handle node transform
  for (auto &gltf_node : gltf_model.nodes) {
    if (gltf_node.matrix.size() == 16) {
      continue;
    }

    gltf_node.matrix.resize(16);
    Eigen::Affine3f transform = Affine3f::Identity();
    Vector3f scale{1.0f, 1.0f, 1.0f};
    for (size_t i = 0; i < gltf_node.scale.size(); i++) {
      scale[i] = gltf_node.scale[i];
    }

    Quaternionf rotation(1.0f, 0.0f, 0.0f, 0.0f);
    if (gltf_node.rotation.size() >= 4) {
      rotation.x() = gltf_node.rotation[0];
      rotation.y() = gltf_node.rotation[1];
      rotation.z() = gltf_node.rotation[2];
      rotation.w() = gltf_node.rotation[3];
    }

    Translation3f translation{0.0f, 0.0f, 0.0f};
    for (size_t i = 0; i < gltf_node.translation.size(); i++) {
      translation.vector()[i] = gltf_node.translation[i];
    }

    transform.prescale(scale);
    transform.prerotate(rotation);
    transform.pretranslate(translation.vector());

    for (uint32_t i = 0; i < 16; i++) {
      gltf_node.matrix[i] = transform.data()[i];
    }
  }

  model.gltf = move(gltf_model);

  return model;
}