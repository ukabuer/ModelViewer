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