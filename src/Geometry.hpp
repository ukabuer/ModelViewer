#pragma once
#include <sokol_gfx.h>

struct Geometry {
  sg_buffer positions;
  sg_buffer normals;
  sg_buffer uvs;
  sg_buffer indices;
  uint32_t num;
};

class Quad {
public:
  static auto GetInstance() -> const sg_buffer &;

  Quad(Quad const &) = delete;
  void operator=(Quad const &) = delete;

private:
  sg_buffer quad_buffer{};
  sg_layout_desc layout{};

  Quad();
};

class Cube {
public:
  static auto GetInstance() -> const Cube &;

  Cube(Cube const &) = delete;
  void operator=(Cube const &) = delete;

  sg_buffer buffer{};
  sg_buffer index_buffer{};
  sg_index_type index_type{};
  sg_layout_desc layout{};

  Cube();
};
