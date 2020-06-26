#include "Geometry.hpp"
#include <vector>

using namespace std;

auto Quad::GetInstance() -> const sg_buffer & {
  static Quad quad{};

  return quad.quad_buffer;
}

Quad::Quad() {
  const vector<float> screen_quad = {
      -1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0,
      -1.0, 1.0,  0.0, 1.0, 1.0, 1.0,  1.0, 1.0,
  };
  sg_buffer_desc screen_quad_desc = {};
  screen_quad_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
  screen_quad_desc.size = screen_quad.size() * sizeof(float);
  screen_quad_desc.content = screen_quad.data();
  quad_buffer = sg_make_buffer(screen_quad_desc);
}

auto Cube::GetInstance() -> const Cube & {
  static Cube cube{};
  return cube;
}

Cube::Cube() {
  vector<float> positions = {
      // top
      -1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      -1.0f,
      // bottom
      -1.0f,
      -1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      -1.0f,
      1.0f,
      // left
      -1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      -1.0f,
      1.0f,
      1.0f,
      // right
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      -1.0f,
      1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      1.0f,
      -1.0f,
      // front
      -1.0f,
      1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      1.0f,
      -1.0f,
      1.0f,
      1.0f,
      1.0f,
      1.0f,
      // back
      1.0f,
      1.0f,
      -1.0f,
      1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      -1.0f,
      1.0f,
      -1.0f,
  };
  // clang-format on
  vector<uint16_t> indices = {
      0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,  8,  10, 9,  8,  11, 10,
      12, 14, 13, 12, 15, 14, 16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22,
  };

  sg_buffer_desc position_buffer_desc = {};
  position_buffer_desc.type = SG_BUFFERTYPE_VERTEXBUFFER;
  position_buffer_desc.size = positions.size() * sizeof(float);
  position_buffer_desc.content = positions.data();
  buffer = sg_make_buffer(position_buffer_desc);

  sg_buffer_desc index_buffer_desc{};
  index_buffer_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
  index_buffer_desc.content = indices.data();
  index_buffer_desc.size = indices.size() * sizeof(uint16_t);
  index_buffer = sg_make_buffer(index_buffer_desc);

  layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
  index_type = SG_INDEXTYPE_UINT16;
}