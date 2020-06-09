#pragma sokol @ctype vec3 Eigen::Vector3f
#pragma sokol @ctype mat4 Eigen::Matrix4f

#pragma sokol @vs vs
in vec3 position;
in vec3 normal;
in vec2 uv;

out vec3 v_normal;
out vec2 v_uv;
out vec3 v_world_pos;

uniform geometry_vs_params {
  mat4 model;
  mat4 camera;
};

void main() {
  vec4 world_pos = model * vec4(position, 1.0f);
  v_uv = uv;
  v_normal = mat3(transpose(inverse(model))) * normal;
  v_world_pos = world_pos.xyz / world_pos.w;
  gl_Position = camera * world_pos;
}
  #pragma sokol @end

  #pragma sokol @fs fs
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_world_pos;

out vec3 g_world_pos;
out vec3 g_normal;
out vec4 g_albedo;

uniform sampler2D albedo;

void main() {
  g_world_pos = v_world_pos;
  g_normal = v_normal;
  g_albedo.rgb = texture(albedo, v_uv).rgb;
  g_albedo.a = 0.6f;
}
  #pragma sokol @end

  #pragma sokol @program deferred vs fs