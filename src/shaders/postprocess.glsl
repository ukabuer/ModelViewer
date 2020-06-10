#pragma sokol @vs postprocess_vs
in vec2 position;
in vec2 uv;

out vec2 v_uv;

void main() {
  gl_Position = vec4(position, 0.0, 1.0);
  v_uv = uv;
}
  #pragma sokol @end

  #pragma sokol @fs postprocess_fs
in vec2 v_uv;

out vec4 color;

uniform sampler2D rendered;

void main() {
  color = texture(rendered, v_uv);
}
  #pragma sokol @end

  #pragma sokol @program postprocess postprocess_vs postprocess_fs