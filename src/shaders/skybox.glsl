#pragma sokol @vs skybox_vs
in vec3 position;
out vec3 v_world_pos;

uniform skybox_vs_params {
  mat4 camera;
};
const mat4 model = mat4(
1.0f, 0.0f, 0.0f, 0.0f,
0.0f, 1.0f, 0.0f, 0.0f,
0.0f, 0.0f, 1.0f, 0.0f,
0.0f, 0.0f, 0.0f, 1.0f / 100.0f
);

void main() {
  v_world_pos = position;
  gl_Position = camera * model * vec4(position, 1.0f);
  gl_Position = gl_Position.xyww;
}
  #pragma sokol @end

  #pragma sokol @fs skybox_fs
in vec3 v_world_pos;

out vec4 color;

uniform samplerCube skybox_cube;

vec3 srgb_to_linear(vec3 srgb_color) {
  const float gamma = 2.2f;
  return pow(srgb_color, vec3(gamma));
}

void main() {
  color = vec4(texture(skybox_cube, v_world_pos).rgb, 1.0f);
}
  #pragma sokol @end

  #pragma sokol @program skybox skybox_vs skybox_fs