#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Scene.hpp"
#include <Eigen/Core>
#include <iostream>

using namespace std;
using namespace Eigen;

struct Vertex {
  Eigen::Vector3f positions;
  Eigen::Vector3f normal;
  Eigen::Vector2f uv;
};

auto Geometry::Box(float width, float height, float depth) -> Geometry {
  Geometry geometry{};

  const auto hw = width / 2.0f;
  const auto hh = height / 2.0f;
  const auto hd = depth / 2.0f;

  vector<Vertex> vertices = {
      // top
      {{-hw, hh, -hd}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
      {{-hw, hh, hd}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
      {{hw, hh, hd}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
      {{hw, hh, -hd}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      // bottom
      {{-hw, -hh, hd}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
      {{-hw, -hh, -hd}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
      {{hw, -hh, -hd}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
      {{hw, -hh, hd}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
      // left
      {{-hw, hh, -hd}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{-hw, -hh, -hd}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
      {{-hw, -hh, hd}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{-hw, hh, hd}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      // right
      {{hw, hh, hd}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
      {{hw, -hh, hd}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
      {{hw, -hh, -hd}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
      {{hw, hh, -hd}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
      // front
      {{-hw, hh, hd}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      {{-hw, -hh, hd}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
      {{hw, -hh, hd}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      {{hw, hh, hd}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      // back
      {{hw, hh, -hd}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
      {{hw, -hh, -hd}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
      {{-hw, -hh, -hd}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
      {{-hw, hh, -hd}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
  };

  sg_buffer_desc buffer_desc{};
  buffer_desc.size = static_cast<int32_t>(vertices.size()) * sizeof(Vertex);
  buffer_desc.content = vertices.data();

  geometry.vertices = sg_make_buffer(buffer_desc);

  vector<uint32_t> indices = {// top
                              0, 1, 2, 0, 2, 3,
                              // bottom
                              4, 5, 6, 4, 6, 7,
                              // left
                              8, 9, 10, 8, 10, 11,
                              // right
                              12, 13, 14, 12, 14, 15,
                              // front
                              16, 17, 18, 16, 18, 19,
                              // back
                              20, 21, 22, 20, 22, 23};

  buffer_desc.size = static_cast<int32_t>(indices.size()) * sizeof(uint32_t);
  buffer_desc.content = indices.data();

  geometry.indices = sg_make_buffer(buffer_desc);

  return geometry;
}

auto Geometry::Sphere(float radius, uint32_t subdivisions_axis,
                      uint32_t subdivisions_height,
                      float start_latitude_in_radians,
                      float end_latitude_in_radians,
                      float start_longitude_in_radians,
                      float end_longitude_in_radians) -> Geometry {
  Geometry geometry{};

  const auto lat_range = end_latitude_in_radians - start_latitude_in_radians;
  const auto long_range = end_longitude_in_radians - start_longitude_in_radians;

  const auto num_vertices = (subdivisions_axis + 1) * (subdivisions_height + 1);
  auto vertices = vector<Vertex>();
  vertices.reserve(num_vertices);

  for (auto y = 0u; y <= subdivisions_height; y++) {
    for (auto x = 0u; x <= subdivisions_axis; x++) {
      const auto u = static_cast<float>(x) / subdivisions_axis;
      const auto v = static_cast<float>(y) / subdivisions_height;
      const auto theta = long_range * u + start_longitude_in_radians;
      const auto phi = lat_range * v + start_latitude_in_radians;
      const auto sinTheta = std::sin(theta);
      const auto cosTheta = std::cos(theta);
      const auto sinPhi = std::sin(phi);
      const auto cosPhi = std::cos(phi);
      const auto ux = cosTheta * sinPhi;
      const auto uy = cosPhi;
      const auto uz = sinTheta * sinPhi;
      vertices.emplace_back(Vertex{
          {radius * ux, radius * uy, radius * uz}, {ux, uy, uz}, {1 - u, v}});
    }
  }

  const auto num_verts_around = subdivisions_axis + 1;
  auto indices = vector<uint32_t>();
  indices.reserve(3 * subdivisions_axis * subdivisions_height);
  for (auto x = 0u; x < subdivisions_axis; x++) {
    for (auto y = 0u; y < subdivisions_height; y++) {
      // Make triangle 1 of quad.
      indices.insert(indices.end(), {(y + 0) * num_verts_around + x,
                                     (y + 0) * num_verts_around + x + 1,
                                     (y + 1) * num_verts_around + x});

      // Make triangle 2 of quad.
      indices.insert(indices.end(), {(y + 1) * num_verts_around + x,
                                     (y + 0) * num_verts_around + x + 1,
                                     (y + 1) * num_verts_around + x + 1});
    }
  }

  sg_buffer_desc buffer_desc{};
  buffer_desc.size = static_cast<int32_t>(vertices.size()) * sizeof(Vertex);
  buffer_desc.content = vertices.data();
  geometry.vertices = sg_make_buffer(buffer_desc);

  buffer_desc.size = static_cast<int32_t>(indices.size()) * sizeof(uint32_t);
  buffer_desc.content = indices.data();
  geometry.indices = sg_make_buffer(buffer_desc);

  return geometry;
}

auto Scene::Load(const char *filename) -> Scene {
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
  bool res = false;
  if (ext == ".glb") {
    res = loader.LoadBinaryFromFile(&gltf_model, &err, &warn, path);
  } else {
    res = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, path);
  }
  if (!res) {
    throw runtime_error("Failed to load model" + err);
  }
  if (!warn.empty()) {
    cout << "Loading model warning: " << warn << endl;
  }

  Scene scene{};
  if (gltf_model.scenes.empty()) {
    return scene;
  }

  uint32_t default_scene = 0;
  if (gltf_model.defaultScene > 0) {
    assert(gltf_model.defaultScene < gltf_model.scenes.size());
    default_scene = static_cast<uint32_t>(gltf_model.defaultScene);
  }

  auto &scene_nodes = gltf_model.scenes[default_scene].nodes;
  if (scene_nodes.empty()) {
    return scene;
  }

  // load all meshes
  for (auto &gltf_buffer_view : gltf_model.bufferViews) {
    sg_buffer_desc buffer_desc{};
    if (gltf_buffer_view.target == TINYGLTF_TARGET_ARRAY_BUFFER) {
      buffer_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
    } else if (gltf_buffer_view.target ==
               TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER) {
      buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    }
    auto &buffer = gltf_model.buffers[gltf_buffer_view.buffer];

    buffer_desc.size = gltf_buffer_view.byteLength;
    buffer_desc.content = buffer.data.data() + gltf_buffer_view.byteOffset;

    scene.buffers.emplace_back(sg_make_buffer(buffer_desc));
  }

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

    scene.textures.emplace_back(sg_make_image(image_desc));
  }

  scene.model = gltf_model;

  return scene;
}