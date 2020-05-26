#pragma sokol @vs vs
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color0;
out vec4 color;
void main() {
  gl_Position = vec4(position, 1.0f);
  color = color0;
}
#pragma sokol @end

#pragma sokol @fs fs
in vec4 color;
out vec4 frag_color;
void main() {
  frag_color = color;
}
#pragma sokol @end

#pragma sokol @program triangle vs fs