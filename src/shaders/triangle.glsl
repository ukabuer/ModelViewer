#pragma sokol @vs vs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 uv;

uniform vs_params {
  mat4 model;
  mat4 camera;
};

void main() {
  gl_Position = camera * model * vec4(position, 1.0f);
}
  #pragma sokol @end

#pragma sokol @fs fs
out vec4 frag_color;
void main() {
  frag_color = vec4(1.0f);
}
#pragma sokol @end

#pragma sokol @program triangle vs fs